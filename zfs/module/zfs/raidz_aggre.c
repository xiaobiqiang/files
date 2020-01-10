#include <sys/zfs_context.h>
#include <sys/dmu.h>
#include <sys/dmu_impl.h>
#include <sys/dbuf.h>
#include <sys/dmu_objset.h>
#include <sys/dsl_dataset.h>
#include <sys/dsl_dir.h>
#include <sys/dmu_tx.h>
#include <sys/spa.h>
#include <sys/spa_impl.h>
#include <sys/zap.h>
#include <sys/zio.h>
#include <sys/dmu_zfetch.h>
#include <sys/sa.h>
#include <sys/sa_impl.h>
#include <sys/zvol.h>
#include <sys/zfs_znode.h>
#include <sys/raidz_aggre.h>
#include <sys/vdev_impl.h>
#include <sys/dsl_synctask.h>
#ifdef _KERNEL
#include <sys/ddi.h>
#endif

int bptg_reclaim_dva = 0;
int bptg_store_map = 1;
kmem_cache_t *aggre_io_cache=NULL;
uint64_t aggre_io_alloc_n = 0;
uint64_t aggre_io_free_n=0;

typedef enum {
	DEBUG_NORMAL = 0,
	DEBUG_DETAIL
} raidz_debug_level;

typedef enum {
	RECLAIM_PROGRESSIVE = 0,
	RECLAIM_CONSERVATIVE
} reclaim_mode;

int raidz_reclaim_enable = 1;
unsigned long raidz_space_reclaim_gap = 60 * 2;	/* unit: s */
unsigned long raidz_avail_map_thresh = 0x40000000;
int raidz_reclaim_count = 3000;
int raidz_filled_timeout = 60 * 5;
int raidz_cleared_timeout = 60 * 2;
extern int spa_use_together_lock;
int hole_adjust = 0;
extern const zio_vsd_ops_t vdev_raidz_vsd_ops;
extern void vdev_raidz_generate_parity(raidz_map_t *rm);
void reclaim_map_free_range(spa_t *spa, aggre_map_t *map, dmu_tx_t *tx);

static int aggre_io_cons(void *vdb, void *unused, int kmflag)
{
        aggre_io_t *io = (aggre_io_t*)vdb;
        bzero(io, sizeof(aggre_io_t));
        mutex_init(&io->ai_lock, NULL, MUTEX_DEFAULT, NULL);
        aggre_io_alloc_n++;
        return 0;
}

static void
aggre_io_dest(void *vdb, void *unused)
{
        aggre_io_t *io = (aggre_io_t*)vdb;
        mutex_destroy(&io->ai_lock);
        aggre_io_free_n++;
}

static vdev_raidz_aggre_conf_t *
raidz_aggre_search_conf(list_t *list, int data, int parity)
{
	vdev_raidz_aggre_conf_t *conf;
	for (conf = list_head(list); conf; conf = list_next(list, conf)) {
		if (conf->ndata == data && conf->nparity == parity)
			return (conf);
	}
	return (NULL);
}

void 
raidz_aggre_check(spa_t *spa)
{
	vdev_t *root_vdev = spa->spa_root_vdev;
	vdev_t *vdev;
	int c = 0, i = 0;
	int nchild = root_vdev->vdev_children;
	boolean_t has_normal_config = B_FALSE;
	vdev_raidz_aggre_conf_t *conf;
	together_item_t *item;

	if (spa_use_together_lock)
		rw_enter(&spa->spa_together_lock, RW_WRITER);
	while (!list_is_empty(&spa->spa_raidz_aggre_config)) {
		conf = list_remove_head(&spa->spa_raidz_aggre_config);
		kmem_free(conf, sizeof(vdev_raidz_aggre_conf_t));
	}
	if (spa->spa_together_arr)
		kmem_free(spa->spa_together_arr, sizeof(together_item_t) * spa->spa_together_arr_num);
	spa->spa_together_arr = NULL;
	spa->spa_raidz_aggre_mixed = 0;
	spa->spa_together_arr_num = 0;
	spa->spa_together_index = 0;
	if (spa_use_together_lock)
		rw_exit(&spa->spa_together_lock);
	
	for (c = 0; c < nchild; c++) {
		vdev = spa->spa_root_vdev->vdev_child[c];
		if (vdev->vdev_ops == &vdev_raidz_aggre_ops &&
			!vdev->vdev_ismeta) {
			conf = raidz_aggre_search_conf(&spa->spa_raidz_aggre_config,
				vdev->vdev_children - vdev->vdev_nparity,
				vdev->vdev_nparity);
			if (conf) {
				conf->groups++;
				spa->spa_together_arr_num++;
				continue;
			}
			
			conf = kmem_zalloc(sizeof(vdev_raidz_aggre_conf_t), KM_SLEEP);
			conf->ndata = vdev->vdev_children - vdev->vdev_nparity;
			conf->nparity = vdev->vdev_nparity;
			conf->groups = 1;
			spa->spa_together_arr_num++;
			if (!list_is_empty(&spa->spa_raidz_aggre_config))
				spa->spa_raidz_aggre_mixed |= RAIDZ_AGGRE_DIFF_CONFIG;
			
			list_insert_tail(&spa->spa_raidz_aggre_config, conf);
		} else if (!vdev->vdev_ismeta) {
			has_normal_config = B_TRUE;
		}
	}
	
	if (spa_is_raidz_aggre(spa) && has_normal_config)
		spa->spa_raidz_aggre_mixed |= RAIDZ_AGGRE_MIX_NORMAL;

	if (spa_use_together_lock)
		rw_enter(&spa->spa_together_lock, RW_WRITER);
	if (spa->spa_together_arr_num > 0) {
		spa->spa_together_arr = kmem_zalloc(
			sizeof(together_item_t) * spa->spa_together_arr_num,
			KM_SLEEP);
	}
	for (conf = list_head(&spa->spa_raidz_aggre_config);
		conf; 
		conf = list_next(&spa->spa_raidz_aggre_config, conf)) {
		for (c = 0; c < conf->groups; c++) {
			item = &spa->spa_together_arr[i];
			item->together = conf->ndata;
			item->state = 1;
			i++;
		}
	}
	if (spa_use_together_lock)
		rw_exit(&spa->spa_together_lock);
	cmn_err(CE_NOTE, "%s raidz_aggre=%d mixed=0x%x", __func__, 
		spa_is_raidz_aggre(spa), spa->spa_raidz_aggre_mixed);
}

uint64_t
raidz_aggre_get_astart(const vdev_t *vd, const uint64_t start)
{
	uint64_t astart;

	if (vd->vdev_ops != &vdev_raidz_aggre_ops)
		return (start);

	astart = roundup(start, vd->vdev_children << vd->vdev_ashift);
	return (astart);
}

int raidz_aggre_init(void)
{
    aggre_io_cache = kmem_cache_create("raidz_aggre_t", sizeof (aggre_io_t),
    	0, aggre_io_cons, aggre_io_dest, NULL, NULL, NULL, 0);
    return 0;
}

void raidz_aggre_fini(void)
{
    kmem_cache_destroy(aggre_io_cache);
}

void raidz_aggre_zio_create(zio_t *pio, zio_t *zio)
{
    if (pio != NULL && pio->io_aggre_io) {
        VERIFY(pio->io_prop.zp_type == DMU_OT_PLAIN_FILE_CONTENTS ||
                pio->io_prop.zp_type == DMU_OT_ZVOL);
        zio->io_aggre_io = pio->io_aggre_io;
        zio->io_aggre_order = pio->io_aggre_order;
        bcopy(&pio->io_prop, &zio->io_prop, sizeof(zio_prop_t));
		zio->io_aggre_root = B_FALSE;
    } else {
        zio->io_aggre_io = NULL;
        zio->io_aggre_order = 0;
        zio->io_aggre_root = B_FALSE;
    }
}

void 
raidz_aggre_zio_done(zio_t *zio)
{
    aggre_io_t *ai = zio->io_aggre_io;
	
    if (B_TRUE == zio->io_aggre_root) {
        VERIFY(ai);
        mutex_enter(&ai->ai_lock);
        ai->ai_ref--;
        if (ai->ai_ref){
            mutex_exit(&ai->ai_lock);
        } else {
            mutex_exit(&ai->ai_lock);
			if (bptg_store_map == 1) {
				spa_t *spa = zio->io_spa;
				uint64_t txg = spa->spa_syncing_txg;
				aggre_map_elem_t *elem;
				raidz_aggre_elem_init(spa, ai, &elem);
				clist_append(&spa->spa_aggre_maplist[txg & TXG_MASK], elem);
			}	
			kmem_cache_free(aggre_io_cache, ai);
        }
    }
}

void dbuf_aggre_leaf(void **drarray, uint8_t ntogether)
{
	uint8_t blkindex = 0;
	aggre_io_t *an_cur_io;
	dbuf_dirty_record_t *dr = (dbuf_dirty_record_t *)drarray[blkindex];
	
	an_cur_io = kmem_cache_alloc(aggre_io_cache, KM_SLEEP);
	an_cur_io->ai_syncdone = 0;
	an_cur_io->ai_wtoterr = 0;
	an_cur_io->ai_together = ntogether;
	#if 0
	mutex_init(&an_cur_io->ai_synclock, NULL, MUTEX_DEFAULT, NULL); 
	cv_init(&an_cur_io->ai_synccv, NULL, CV_DEFAULT, NULL);
	#endif
	TGM_INIT_MAP(an_cur_io, ntogether, dr->dr_zio->io_bookmark.zb_objset,
		dr->dr_zio->io_bookmark.zb_object);

	while (dr != NULL) {
		VERIFY(blkindex < ntogether);
		VERIFY(dr->dr_zio!=NULL);
		an_cur_io->ai_buf_array[blkindex] = dr->dr_zio->io_data;
		an_cur_io->ai_map.tgm_blockid[blkindex] = dr->dr_zio->io_bookmark.zb_blkid;	
		/*dr->dr_zio->io_flags |= ZIO_FLAG_DONT_AGGREGATE;*/
		
		dr->dr_zio->io_aggre_io = an_cur_io;
		dr->dr_zio->io_aggre_root = B_TRUE;
		dr->dr_zio->io_aggre_order = blkindex;
		blkindex++;
		dr = drarray[blkindex];				
	}

	blkindex = 0;
	dr = drarray[blkindex];
	while (dr != NULL) {
		zio_nowait(dr->dr_zio);
		blkindex++;
		dr = drarray[blkindex];		
	}
}

int dva_alloc_aggre = 1;
raidz_map_t *
raidz_aggre_map_alloc(zio_t *zio, uint64_t unit_shift, uint64_t dcols,
    uint64_t nparity)
{

	raidz_map_t *rm;
	uint64_t b = zio->io_offset >> unit_shift;
	uint64_t s = zio->io_size >> unit_shift;
	uint64_t f = b % dcols;
	uint64_t o = (b / dcols) << unit_shift;
	uint32_t nskip;
	uint32_t indexid = (uint32_t)BP_GET_BLKID(zio->io_bp);
	/*uint64_t q, r, c, bc, acols, scols, asize, tot, firstdatacol;*/
	uint64_t c, bc, col, acols, scols, coff, devidx, asize, tot, firstdatacol;

	dva_t *dva = zio->io_bp->blk_dva;
	uint64_t dvaasize;
	
	if (zio->io_type == ZIO_TYPE_WRITE) {
        if(zio->io_prop.zp_type != DMU_OT_PLAIN_FILE_CONTENTS &&
                zio->io_prop.zp_type != DMU_OT_ZVOL)
			cmn_err(CE_PANIC,"%s %p dnode type (%d) err ",__func__, zio, zio->io_type  );
	}
	
	dvaasize = DVA_GET_ASIZE(dva);
	tot = s * dcols ;
	if ((dvaasize >> unit_shift) < tot){
		cmn_err(CE_PANIC,"%s %p type (%d) asize error %ld %ld ",__func__, zio, zio->io_type, (long)(asize >> unit_shift) ,(long)tot);	
	}

	bc = (dvaasize >> unit_shift) - tot;
	if (bc > nparity + 1) {
		cmn_err(CE_PANIC,"%s %p type (%d) nparity error %ld %ld ",__func__, zio, zio->io_type, (long)(asize >> unit_shift) ,(long)tot);	
	}
	
	if (zio->io_type == ZIO_TYPE_WRITE && indexid == 0) {
	        acols = scols = 1 + nparity;
            firstdatacol =  nparity;
	} else {
	        acols = scols = 1;
            firstdatacol = 0;
	}

	rm = kmem_alloc(offsetof(raidz_map_t, rm_col[scols]), KM_SLEEP);

	rm->rm_cols = acols;
	rm->rm_scols = scols;
	rm->rm_bigcols = 0;
	rm->rm_skipstart = 0;
	rm->rm_missingdata = 0;
	rm->rm_missingparity = 0;
	rm->rm_firstdatacol = firstdatacol;
	rm->rm_datacopy = NULL;
	rm->rm_reports = 0;
	rm->rm_freed = 0;
	rm->rm_ecksuminjected = 0;
    rm->rm_aggre_col = 0;
	rm->rm_nskip = 0;
	
	asize = 0;

	if (indexid == 0)
		rm->rm_nskip = roundup(tot, nparity + 1) - tot;
	
	for (c = 0; c < scols; c++) {
		coff = o;
		if (scols == 1)
        	col = f + nparity + indexid;
		else
			col = f + c;

		if (col >= dcols) {
			col -= dcols;
			coff += 1ULL << unit_shift;
		}
		
		rm->rm_col[c].rc_devidx = col;
		rm->rm_col[c].rc_offset = coff;
			
		rm->rm_col[c].rc_data = NULL;
		rm->rm_col[c].rc_gdata = NULL;
		rm->rm_col[c].rc_error = 0;
		rm->rm_col[c].rc_tried = 0;
		rm->rm_col[c].rc_skipped = 0;
		rm->rm_col[c].rc_need_try = 0;
        rm->rm_col[c].rc_size = zio->io_size;

		asize += rm->rm_col[c].rc_size;
	}
	
	rm->rm_asize = asize;
	
	for (c = 0; c < rm->rm_firstdatacol; c++)
		rm->rm_col[c].rc_data = zio_buf_alloc(rm->rm_col[c].rc_size);

	rm->rm_col[c].rc_data = zio->io_data;

	//for (c = c + 1; c < acols; c++)
	//	rm->rm_col[c].rc_data = (char *)rm->rm_col[c - 1].rc_data +
	//	    rm->rm_col[c - 1].rc_size;

	/*
	 * If all data stored spans all columns, there's a danger that parity
	 * will always be on the same device and, since parity isn't read
	 * during normal operation, that that device's I/O bandwidth won't be
	 * used effectively. We therefore switch the parity every 1MB.
	 *
	 * ... at least that was, ostensibly, the theory. As a practical
	 * matter unless we juggle the parity between all devices evenly, we
	 * won't see any benefit. Further, occasional writes that aren't a
	 * multiple of the LCM of the number of children and the minimum
	 * stripe width are sufficient to avoid pessimal behavior.
	 * Unfortunately, this decision created an implicit on-disk format
	 * requirement that we need to support for all eternity, but only
	 * for single-parity RAID-Z.
	 *
	 * If we intend to skip a sector in the zeroth column for padding
	 * we must make sure to note this swap. We will never intend to
	 * skip the first column since at least one data and one parity
	 * column must appear in each row.
	 */
	/*ASSERT(rm->rm_cols >= 2);
	ASSERT(rm->rm_col[0].rc_size == rm->rm_col[1].rc_size);

	if (rm->rm_firstdatacol == 1 && (zio->io_offset & (1ULL << 20))) {
		devidx = rm->rm_col[0].rc_devidx;
		o = rm->rm_col[0].rc_offset;
		rm->rm_col[0].rc_devidx = rm->rm_col[1].rc_devidx;
		rm->rm_col[0].rc_offset = rm->rm_col[1].rc_offset;
		rm->rm_col[1].rc_devidx = devidx;
		rm->rm_col[1].rc_offset = o;

		if (rm->rm_skipstart == 0)
			rm->rm_skipstart = 1;
	}*/

	zio->io_vsd = rm;
	zio->io_vsd_ops = &vdev_raidz_vsd_ops;
	return (rm);
}

void 
raidz_aggre_generate_parity(zio_t *zio, raidz_map_t *rm_old)
{
    int c = 0;
	raidz_map_t *rm;
    uint32_t scols = zio->io_vd->vdev_children;
    
    VERIFY(zio->io_prop.zp_type == DMU_OT_PLAIN_FILE_CONTENTS ||
            zio->io_prop.zp_type == DMU_OT_ZVOL);
 
    rm = kmem_zalloc(offsetof(raidz_map_t, rm_col[scols]), KM_SLEEP);

	rm->rm_cols = scols;
	rm->rm_scols = scols;
    rm->rm_asize = zio->io_size * scols;
    rm->rm_firstdatacol = rm_old->rm_firstdatacol;
    rm->rm_aggre_col = 0;
    
    for (c = 0; c < scols; c++) {
            
		rm->rm_col[c].rc_devidx = 0;
		rm->rm_col[c].rc_offset = 0;
		rm->rm_col[c].rc_gdata = NULL;
		rm->rm_col[c].rc_error = 0;
		rm->rm_col[c].rc_tried = 0;
		rm->rm_col[c].rc_skipped = 0;
        rm->rm_col[c].rc_size = zio->io_size;
        if (c < rm->rm_firstdatacol) 
           	rm->rm_col[c].rc_data = rm_old->rm_col[c].rc_data;
        else {
            VERIFY(zio->io_aggre_io->ai_buf_array[c - rm->rm_firstdatacol]);
            rm->rm_col[c].rc_data = *(zio->io_aggre_io->ai_buf_array + c - rm->rm_firstdatacol);
        }
	}
    
    vdev_raidz_generate_parity(rm);
    kmem_free(rm, offsetof(raidz_map_t, rm_col[scols]));
}

void raidz_aggre_raidz_done(zio_t *zio, raidz_map_t ** rmp_old)
{
    uint32_t scols = zio->io_vd->vdev_children;
    uint32_t dcols = scols;
    uint64_t unit_shift = zio->io_vd->vdev_top->vdev_ashift;
    uint64_t devindex = (*rmp_old)->rm_col[0].rc_devidx;
    uint64_t iosize = zio->io_size;

    uint64_t b = zio->io_offset >> unit_shift;
    uint64_t s = zio->io_size >> unit_shift;
    uint64_t f = b % dcols;
    uint64_t o = (b / dcols) << unit_shift;
    int c = 0;
    vdev_t *vd = zio->io_vd;
    raidz_map_t *rm;
    vdev_t *cvd;
    uint64_t coff, col;

    if ((*rmp_old)->rm_cols == dcols) {
		rm = *rmp_old;
        for (c = 0; c < scols; c++) {
	    	col = f + c;
            if (col >= dcols) {
				col -= dcols;
            }
            cvd = vd->vdev_child[col];
            if (vdev_dtl_contains(cvd, DTL_MISSING, zio->io_txg, 1) &&
				rm->rm_col[c].rc_need_try) {
	        	rm->rm_col[c].rc_tried = 0;
				rm->rm_col[c].rc_need_try = 0;
            }
		}
		return;
    }


    rm = kmem_zalloc(offsetof(raidz_map_t, rm_col[scols]), KM_SLEEP);
    rm->rm_cols = scols;
    rm->rm_scols = scols;
    rm->rm_asize = zio->io_size * scols;
    rm->rm_firstdatacol = zio->io_vd->vdev_nparity;

    for (c = 0; c < scols; c++) {
        coff = o;
        col = f + c;

        if (col >= dcols) {
            col -= dcols;
            coff += 1ULL << unit_shift;
        }

        rm->rm_col[c].rc_devidx = col;
        rm->rm_col[c].rc_offset = coff;

        rm->rm_col[c].rc_data = NULL;
        rm->rm_col[c].rc_gdata = NULL;
        rm->rm_col[c].rc_error = 0;
        rm->rm_col[c].rc_tried = 0;
        rm->rm_col[c].rc_skipped = 0;
        rm->rm_col[c].rc_size = zio->io_size;

        if (devindex == col) {
            rm->rm_col[c].rc_data = zio->io_data;
            rm->rm_aggre_col = c;
        } else {
            rm->rm_col[c].rc_data = zio_buf_alloc(iosize);
        }
            
        cvd = vd->vdev_child[col];
        if (!vdev_readable(cvd)) {
            if (c >= rm->rm_firstdatacol)
                rm->rm_missingdata++;
            else
                rm->rm_missingparity++;
        
            rm->rm_col[c].rc_error = ENXIO;
            rm->rm_col[c].rc_tried = 1;    /* don't even try */
            rm->rm_col[c].rc_skipped = 1;
                    
            continue;
        }
        if (vdev_dtl_contains(cvd, DTL_MISSING, zio->io_txg, 1)) {
            if (c >= rm->rm_firstdatacol)
                rm->rm_missingdata++;
            else
                rm->rm_missingparity++;
            rm->rm_col[c].rc_error = ESTALE;
            rm->rm_col[c].rc_skipped = 1;
            rm->rm_col[c].rc_tried = 1;
			rm->rm_col[c].rc_need_try = 1;
            continue;
        }
    }
        
    zio->io_vsd_ops->vsd_free(zio);
    zio->io_vsd = rm;
    zio->io_error = 0;
    *rmp_old = rm;
}

void raidz_aggre_metaslab_align(vdev_t *vd, uint64_t *start, uint64_t *size)
{
    uint64_t offset = 0;
    if (strcmp(vd->vdev_ops->vdev_op_type, VDEV_TYPE_RAIDZ_AGGRE) == 0) {
        if (512*1024 % (vd->vdev_children - vd->vdev_nparity)) {
        	return;
        }
        if (*start) {
            offset = roundup(*start, 512*1024*vd->vdev_children/(vd->vdev_children - vd->vdev_nparity));
            VERIFY(offset - *start < 512*1024*vd->vdev_children/(vd->vdev_children - vd->vdev_nparity));
            *size  -= offset - *start;
            *start = offset;
        }
    }
}

int raidz_tgbp_compare(const void *a, const void *b)
{
	tg_freebp_entry_t *sa = (tg_freebp_entry_t *)a;
	tg_freebp_entry_t *sb = (tg_freebp_entry_t *)b;
	int ret;

	ret = bcmp(&sa->tf_blk.blk_dva[0], &sb->tf_blk.blk_dva[0],
	    sizeof (dva_t));

	if (ret < 0)
		return (-1);
	else if (ret > 0)
		return (1);
	else
		return (0);
}

void raidz_tgbp_combine(tg_freebp_entry_t *a, tg_freebp_entry_t *b)
{
	a->tf_blk.blk_pad[1] |=	BP_GET_BLKID((&(b->tf_blk)));
}

void
raidz_aggre_free_bp(spa_t *spa, dva_t dva, int aggre_num, uint64_t txg, dmu_tx_t *tx)
{
	blkptr_t bp;
	BP_ZERO(&bp);
	memcpy(&bp.blk_dva[0], &dva, sizeof (dva_t));
	BP_SET_BIRTH(&bp, txg, txg);
	BP_SET_TOGTHER(&bp);
	BP_SET_FREEFLAG(&bp, 1);
	ASSERT(aggre_num > 1);
	BP_SET_AGGRENUM(&bp, aggre_num);
	bplist_append(&spa->spa_free_bplist[tx->tx_txg & TXG_MASK], &bp);
}

void
set_reclaim_state(spa_t *spa, int map_index, int state, 
	uint64_t cur_time, uint64_t txg)
{
	int index = txg & TXG_MASK;
	mutex_enter(&spa->spa_aggre_reclaim_state[index].mtx);
	spa->spa_aggre_reclaim_state[index].map_index = map_index;
	spa->spa_aggre_reclaim_state[index].state = state;
	spa->spa_aggre_reclaim_state[index].cur_time = cur_time;
	spa->spa_aggre_reclaim_state[index].flag |= MAP_STATE_FLAG;
	mutex_exit(&spa->spa_aggre_reclaim_state[index].mtx);
}

void
set_reclaim_pos(spa_t *spa, uint64_t pos, uint64_t txg)
{
	int index = txg & TXG_MASK;
	mutex_enter(&spa->spa_aggre_reclaim_state[index].mtx);
	spa->spa_aggre_reclaim_state[index].pos = pos;
	spa->spa_aggre_reclaim_state[index].flag |= MAP_POS_FLAG;
	mutex_exit(&spa->spa_aggre_reclaim_state[index].mtx);
}

void
update_reclaim_map(spa_t *spa, dmu_tx_t *tx)
{
	int index = tx->tx_txg & TXG_MASK;
	int reclaimed_obj;
	int state;
	uint64_t time;
	uint64_t pos;
	aggre_map_t *map;

	reclaimed_obj = (spa->spa_map_manager.mm_active_obj_index == 0) ? 1 : 0;
	map = spa->spa_aggre_map_arr[reclaimed_obj];

	/* read from disk */
	if (map->hdr->state == AGGRE_MAP_OBJ_RECLAIMED)
		goto free_range;
	
	mutex_enter(&spa->spa_aggre_reclaim_state[index].mtx);
	if (spa->spa_aggre_reclaim_state[index].flag & MAP_STATE_FLAG) {
		state = spa->spa_aggre_reclaim_state[index].state;
		time = spa->spa_aggre_reclaim_state[index].cur_time;
		VERIFY(state == MAP_RECLAIM_START || state == MAP_RECLAIM_END);
		dmu_buf_will_dirty(map->dbuf_hdr, tx);
		map->hdr->state = (state == MAP_RECLAIM_START) ? 
			AGGRE_MAP_OBJ_RECLAIMING : AGGRE_MAP_OBJ_RECLAIMED;
		map->hdr->time = time;
		spa->spa_aggre_reclaim_state[index].flag &= ~MAP_STATE_FLAG;
	}

	if (spa->spa_aggre_reclaim_state[index].flag & MAP_POS_FLAG) {
		pos = spa->spa_aggre_reclaim_state[index].pos;
		dmu_buf_will_dirty(map->dbuf_hdr, tx);
		map->hdr->process_index = pos + 1;
		spa->spa_aggre_reclaim_state[index].flag &= ~MAP_POS_FLAG;
	}
	mutex_exit(&spa->spa_aggre_reclaim_state[index].mtx);
	
free_range:
	if (map->hdr->state == AGGRE_MAP_OBJ_RECLAIMED) {
		uint64_t now;
#ifdef _KERNEL
		now = ddi_get_time();
#endif
		if (now - map->hdr->time > raidz_cleared_timeout)
			reclaim_map_free_range(spa, map, tx);
	}
}

void
reclaim_map_free_range(spa_t *spa, aggre_map_t *map, dmu_tx_t *tx)
{
	dmu_buf_will_dirty(map->dbuf_hdr, tx);
	map->hdr->state = AGGRE_MAP_OBJ_CLEAR;
	map->hdr->total_count = 0;	
	map->hdr->avail_count = 0;	
	map->hdr->process_index = 0;
#ifdef _KERNEL
	map->hdr->time = ddi_get_time();
#endif

	map->dbuf_rehold = 1;
	map->dbuf_id = 0;
	map->free_txg = tx->tx_txg;
	
	cmn_err(CE_NOTE, "%s %s dmu_free_range obj=%d", __func__,
		spa->spa_name, map->index);
 	dmu_free_range(map->os, map->object, 0, DMU_OBJECT_END, tx);	
}

void
raidz_aggre_create_map_obj(spa_t *spa, dmu_tx_t *tx, int aggre_num)
{
	objset_t *mos = spa->spa_meta_objset;
	dmu_buf_t *dbp;
	aggre_map_hdr_t *hdr;
	int i;

	for (i = 0; i < AGGRE_MAP_MAX_OBJ_NUM; i++) {
		spa->spa_map_obj_arr[i] = dmu_object_alloc(mos, DMU_OT_RAIDZ_AGGRE_MAP,
	    	SPA_MAXBLOCKSIZE, DMU_OT_RAIDZ_AGGRE_MAP_HDR, sizeof (aggre_map_hdr_t), tx);

		VERIFY(0 == dmu_bonus_hold(mos, spa->spa_map_obj_arr[i], FTAG, &dbp));
	
		hdr = dbp->db_data;
		dmu_buf_will_dirty(dbp, tx);
		
		hdr->aggre_num = aggre_num;
		hdr->recsize = offsetof(aggre_map_elem_t, blkid) + aggre_num * sizeof(uint64_t);
		hdr->blksize = SPA_MAXBLOCKSIZE;
		hdr->state = (i == 0) ? AGGRE_MAP_OBJ_FILLING : AGGRE_MAP_OBJ_CLEAR;
		hdr->total_count = 0;
		hdr->avail_count = 0;
		hdr->process_index = 0;
#ifdef _KERNEL
		hdr->time = ddi_get_time();
#endif
		dmu_buf_rele(dbp, FTAG);
	}
	VERIFY(zap_add(mos, DMU_POOL_DIRECTORY_OBJECT,
	    DMU_POOL_RAIDZ_AGGRE_MAP, sizeof (uint64_t), AGGRE_MAP_MAX_OBJ_NUM,
	    &spa->spa_map_obj_arr[0], tx) == 0);
}

int
raidz_aggre_map_open(spa_t *spa)
{
	dmu_object_info_t doi;
	aggre_map_t *map;
	uint64_t offset;
	int err, i, loop;
	boolean_t found_filling = B_FALSE;
	
	for (loop = 0; loop < AGGRE_MAP_MAX_OBJ_NUM; loop++) {
		err = dmu_object_info(spa->spa_meta_objset, spa->spa_map_obj_arr[loop], &doi);
		if (err) {
			cmn_err(CE_WARN, "%s get object info failed", __func__);
			return (err);
		}
		
		ASSERT3U(doi.doi_type, ==, DMU_OT_RAIDZ_AGGRE_MAP);
		ASSERT3U(doi.doi_bonus_type, ==, DMU_OT_RAIDZ_AGGRE_MAP_HDR);
	
		map = spa->spa_aggre_map_arr[loop] = kmem_zalloc(sizeof(aggre_map_t), KM_SLEEP);
		err = dmu_bonus_hold(spa->spa_meta_objset, spa->spa_map_obj_arr[loop], 
			map, &map->dbuf_hdr);
		if (err) {
			cmn_err(CE_WARN, "%s hold bonus buf failed", __func__);
			return (err);
		}
		
		map->hdr = (aggre_map_hdr_t *)map->dbuf_hdr->db_data;
		map->index = loop;
		map->os = spa->spa_meta_objset;
		map->object = spa->spa_map_obj_arr[loop];
		map->dbuf_num = AGGRE_MAP_MAX_DBUF_NUM;
		map->dbuf_array = (dmu_buf_t **)kmem_zalloc(sizeof(dmu_buf_t *) * map->dbuf_num, 
			KM_SLEEP);
		map->dbuf_size = doi.doi_data_block_size;
		map->dbuf_rehold = 1;	/* first time need to hold buf */
		map->dbuf_id = 0;
		map->free_txg = 0;
		mutex_init(&map->aggre_lock, NULL, MUTEX_DEFAULT, NULL);
		
		cmn_err(CE_NOTE, "%s %s map[%d].state=%d",
			__func__, spa->spa_name, loop, map->hdr->state);

		if (map->hdr->state == AGGRE_MAP_OBJ_FILLING) {
			int blk_rec_count;
			if (found_filling) {
				cmn_err(CE_PANIC, "%s %s has two active objs, panic",
					__func__, spa->spa_name);
			}
			blk_rec_count = map->hdr->blksize / map->hdr->recsize;
			map->dbuf_id = map->hdr->total_count / blk_rec_count;
			spa->spa_map_manager.mm_active_obj_index = loop;
			found_filling = B_TRUE;
			cmn_err(CE_NOTE, "%s %s active obj=%d", 
				__func__, spa->spa_name, spa->spa_map_manager.mm_active_obj_index);
		}
	}

	if (!found_filling)
		cmn_err(CE_PANIC, "%s %s don't have active obj, panic",
			__func__, spa->spa_name);
	
	return (0);
}

int 
raidz_aggre_elem_enqueue_cb(void *arg, void *data, dmu_tx_t *tx)
{
	aggre_map_elem_t *elem = (aggre_map_elem_t *)data;
	aggre_map_t *map = (aggre_map_t *)arg;
	spa_t *spa = map->os->os_spa;
	int blk_rec_count, i;
	uint64_t dbuf_id, offset, blkoff;
	uint8_t *dest;
	
	blk_rec_count = map->hdr->blksize / map->hdr->recsize;	
	dbuf_id = map->hdr->total_count / blk_rec_count;

	/* read more block */
	if (map->dbuf_rehold ||
		dbuf_id >= map->dbuf_id + map->dbuf_num) {
		for (i = 0; i < map->dbuf_num; i++) {
			if (map->dbuf_array[i])
				dmu_buf_rele(map->dbuf_array[i], map);
		}

		map->dbuf_id = dbuf_id;
		for (i = 0; i < map->dbuf_num; i++) {
			offset = (map->dbuf_id + i) * map->dbuf_size;
			dmu_buf_hold(map->os, map->object, offset, map, 
				&map->dbuf_array[i], 0);
		}

		if (map->dbuf_rehold)
			map->dbuf_rehold = 0;
	}

	dbuf_id = (dbuf_id - map->dbuf_id) % map->dbuf_num;
	dmu_buf_will_dirty(map->dbuf_array[dbuf_id], tx);
	
	blkoff = (map->hdr->total_count % blk_rec_count) * map->hdr->recsize;
	dest = (uint8_t *)map->dbuf_array[dbuf_id]->db_data + blkoff;
	bcopy(elem, dest, map->hdr->recsize);
	
	dmu_buf_will_dirty(map->dbuf_hdr, tx);
	
	mutex_enter(&map->aggre_lock);
	map->hdr->total_count++;
	map->hdr->avail_count++;
	
	mutex_exit(&map->aggre_lock);
	kmem_free(elem, map->hdr->recsize);
	return (0);
}

void
raidz_aggre_map_close(spa_t *spa)
{
	aggre_map_t *map;
	int i;
	int j;
	
	cmn_err(CE_WARN, "raidz_aggre_map_close %s", spa->spa_name);
	for(i=0; i<AGGRE_MAP_MAX_OBJ_NUM; i++){
		map = spa->spa_aggre_map_arr[i];
		if (map == NULL) {
			return;
		}
		mutex_destroy(&map->aggre_lock);
		
		if (map->dbuf_hdr)
			dmu_buf_rele(map->dbuf_hdr, map);
		
		for (j = 0; j < map->dbuf_num; j++) {
			if(map->dbuf_array[j]!=NULL)
				dmu_buf_rele(map->dbuf_array[j], map);
		}

		if (map->dbuf_array)
			kmem_free(map->dbuf_array, sizeof(dmu_buf_t *) * map->dbuf_num);

		kmem_free(map, sizeof(aggre_map_t));
	}
}

void
raidz_aggre_elem_init(spa_t *spa, aggre_io_t *aio, 
	aggre_map_elem_t **pelem)
{
	int i;
	aggre_map_t *map;
	aggre_map_elem_t *elem;

	map = spa->spa_aggre_map_arr[0];
	*pelem = kmem_zalloc(map->hdr->recsize, KM_SLEEP);

	elem = *pelem;
	elem->txg = spa->spa_syncing_txg;
	elem->timestamp = gethrtime();
	elem->objsetid = aio->ai_map.tgm_objsetid;
	elem->objectid = aio->ai_map.tgm_dnodeid;
	elem->dva = aio->ai_dva[0];
	elem->valid_num = aio->ai_together;
	
	for (i = 0; i < TG_MAX_DISK_NUM; i++) {
		elem->blkid[i] = aio->ai_map.tgm_blockid[i];
	}
}

void
raidz_aggre_elem_clone(spa_t *spa, aggre_map_elem_t *src,
	aggre_map_elem_t **dst)
{
	aggre_map_t *map;

	map = spa->spa_aggre_map_arr[0];
	*dst = kmem_zalloc(map->hdr->recsize, KM_SLEEP);
	bcopy(src, *dst, map->hdr->recsize);
}

dmu_tx_t *
create_meta_tx(spa_t *spa)
{
	dmu_tx_t *tx;
	int err = 0;
	
	tx = dmu_tx_create_dd(NULL);
	tx->tx_pool = spa->spa_dsl_pool;
	tx->tx_objset = spa->spa_meta_objset;
	err = dmu_tx_assign(tx, TXG_WAIT);
	if (err) {
		cmn_err(CE_WARN, "%s txg assign failed, err=%d",
			__func__, err);
		dmu_tx_abort(tx);
		return (NULL);
	}

	return (tx);
}

void
commit_meta_tx(dmu_tx_t *tx)
{
	dmu_tx_commit(tx);
}

#ifdef _KERNEL
int
raidz_aggre_process_elem(spa_t *spa, uint64_t pos, aggre_map_elem_t *elem, 
	aggre_elem_state *state)
{
	int i, size, err = 0;
	aggre_map_t *map = spa->spa_aggre_map_arr[0];
	dsl_pool_t *dp = spa->spa_dsl_pool;
	dsl_dataset_t *ds = NULL;
	objset_t *os = NULL;
	dnode_t *dn = NULL;
	dmu_buf_impl_t **dbp = NULL;
	dmu_tx_t *tx = NULL;
	int change_num = 0;
	int hole_thresh;
 	*state = ELEM_STATE_ERR;

	do {
		rrw_enter(&dp->dp_config_rwlock, RW_READER, FTAG);
		err = dsl_dataset_hold_obj(spa->spa_dsl_pool, elem->objsetid, FTAG, &ds);
		if (err) {
			rrw_exit(&dp->dp_config_rwlock, FTAG);
			cmn_err(CE_WARN, "%s %s dsl_dataset_hold_obj err=%d dsl_pool=%p"
				" objsetid=%"PRIu64" objectid=%"PRIu64" dva[0]=0x%"PRIx64
				" dva[1]=0x%"PRIx64, __func__, spa->spa_name, err, 
				spa->spa_dsl_pool, elem->objsetid, elem->objectid,
				elem->dva.dva_word[0], elem->dva.dva_word[1]);
			if (ds != NULL) {
				cmn_err(CE_WARN, "%s %s dataset=%p", __func__, 
					spa->spa_name, ds);
			}
			ds = NULL;
			if (err == ENOENT)
				*state = ELEM_STATE_FREE;
			break;
		}

		rrw_exit(&dp->dp_config_rwlock, FTAG);
		err = dmu_objset_from_ds(ds, &os);
		if (err) {
			cmn_err(CE_WARN, "%s %s dmu_objset_from_ds err=%d", 
				__func__, spa->spa_name, err);
			break;
		}
		
		err = dnode_hold(os, elem->objectid, FTAG, &dn);
		if (err) {
			cmn_err(CE_WARN, "%s %s dnode_hold err=%d os=%p objsetid=%"PRIu64
				" objectid=%"PRIu64" dva[0]=0x%"PRIx64" dva[1]=0x%"PRIx64,
				__func__, spa->spa_name, err, os, elem->objsetid, elem->objectid,
				elem->dva.dva_word[0], elem->dva.dva_word[1]);
			
			if (err == ENOENT)
				*state = ELEM_STATE_FREE; 
			break;
		}

		size = sizeof(dmu_buf_impl_t *) * elem->valid_num;
		dbp = (dmu_buf_impl_t **)kmem_zalloc(size, KM_SLEEP);
		
		for (i = 0; i < elem->valid_num; i++) {
			rw_enter(&dn->dn_struct_rwlock, RW_READER);
			dbp[i] = dbuf_hold(dn, elem->blkid[i], FTAG);
			rw_exit(&dn->dn_struct_rwlock);
			if ((dbp[i] && (dbp[i]->db_blkptr == NULL ||
				dbp[i]->db_blkptr->blk_birth != elem->txg)) ||
				(dbp[i] == NULL)) {
				change_num++;
			}
		}

		hole_thresh = elem->valid_num / 2;
		if (hole_adjust)
			hole_thresh += hole_adjust;

		if (change_num == elem->valid_num) {
			*state = ELEM_STATE_FREE;
		} else if (change_num == 0) {
			*state = ELEM_STATE_NO_CHANGE;
		} else {
			if (change_num < hole_thresh) {
				/*DTRACE_PROBE2(hole__notfree, int, change_num, int, hole_thresh);*/
				*state = ELEM_STATE_NO_CHANGE;
			} else {
				*state = ELEM_STATE_REWRITE;
			}
		} 
		
	} while (0);

	if (*state == ELEM_STATE_REWRITE) {
	
		tx = dmu_tx_create(dn->dn_objset);
		for (i = 0; i < elem->valid_num; i++) {
			if (dbp[i] && dbp[i]->db_blkptr &&
				dbp[i]->db_blkptr->blk_birth == elem->txg) {
				dmu_tx_hold_write(tx, dn->dn_object, 
					dbp[i]->db_blkid << dn->dn_datablkshift,
					dn->dn_datablksz);
			}
		}

		err = dmu_tx_assign(tx, TXG_WAIT);
		if (err) {
			cmn_err(CE_WARN, "%s %s txg assign failed, err=%d",
				__func__, spa->spa_name, err);

			dmu_tx_abort(tx);
		
		} else {
			for (i = 0; i < elem->valid_num; i++) {
				if (dbp[i] && dbp[i]->db_blkptr &&
					dbp[i]->db_blkptr->blk_birth == elem->txg) {
					dmu_buf_will_dirty((dmu_buf_t *)dbp[i], tx);
					#if 0
					DTRACE_PROBE4(dirty__dbuf, uint64_t, elem->objsetid,
						uint64_t, elem->objectid, 
						uint64_t, elem->blkid[i],
						uint64_t, tx->tx_txg);
					#endif
				}
			}

			raidz_aggre_free_bp(spa, elem->dva, elem->valid_num, elem->txg, tx);
			set_reclaim_pos(spa, pos, tx->tx_txg);
			dmu_tx_commit(tx);
		}
	} else if (*state == ELEM_STATE_NO_CHANGE ||
		*state == ELEM_STATE_FREE) {
		tx = create_meta_tx(spa);
		if (!tx) {
			cmn_err(CE_WARN, "%s %s process pos=%"PRIu64" create meta tx failed",
				__func__, spa->spa_name, pos);
		} else {
			if (*state == ELEM_STATE_NO_CHANGE) {
				aggre_map_elem_t *new_elem;
				raidz_aggre_elem_clone(spa, elem, &new_elem);
				clist_append(&spa->spa_aggre_maplist[tx->tx_txg & TXG_MASK], new_elem);
			} else {
				raidz_aggre_free_bp(spa, elem->dva, elem->valid_num, elem->txg, tx);
			}
			
			set_reclaim_pos(spa, pos, tx->tx_txg);
			commit_meta_tx(tx);
		}
	}

	if (dbp) {
		for (i = 0; i < elem->valid_num; i++)
			dbuf_rele(dbp[i], FTAG);

		kmem_free(dbp, size);
	}
	
	if (dn)
		dnode_rele(dn, FTAG);

	if (ds)
		dsl_dataset_rele(ds, FTAG);

	if (err == ENOENT)
		err = 0;
	
	return (err);
}

aggre_map_t *raidz_aggre_get_reclaim_map(spa_t *spa)
{
	int obj_index;
	aggre_map_manager_t *map_manager = &spa->spa_map_manager;
	obj_index = map_manager->mm_active_obj_index;
	obj_index = (obj_index == 0) ? 1 : 0;
	return spa->spa_aggre_map_arr[obj_index];
}

aggre_map_t *
raidz_aggre_map_current(spa_t *spa, dmu_tx_t *tx)
{
	int filling_obj;
	int reclaimed_obj;
	aggre_map_t *mapfilling;
	aggre_map_t *mapreclaimed;
	aggre_map_manager_t *map_manager;
	uint64_t nows;
	boolean_t need_switch = B_FALSE;

	if(!(spa->spa_space_reclaim_state & SPACE_RECLAIM_START))
		return (NULL);
		
	map_manager = &spa->spa_map_manager;
	filling_obj = map_manager->mm_active_obj_index;
	reclaimed_obj = (filling_obj == 0) ? 1 : 0;
	mapfilling = spa->spa_aggre_map_arr[filling_obj];
	mapreclaimed = spa->spa_aggre_map_arr[reclaimed_obj];
	if (spa->spa_sync_pass > 1)
		return (mapfilling);
	
	do {
		if (mapfilling->hdr->avail_count < raidz_reclaim_count)
			break;
		
		if (mapreclaimed->hdr->state != AGGRE_MAP_OBJ_CLEAR)
			break;

		if (mapreclaimed->free_txg == tx->tx_txg)
			break;

#ifdef _KERNEL
		nows = ddi_get_time();
#endif
		if (nows - mapfilling->hdr->time < raidz_filled_timeout)
			break;
		
		if (nows - mapreclaimed->hdr->time < raidz_cleared_timeout)
			break;
		
		need_switch = B_TRUE;
	} while (0);

	if (need_switch) {
		dmu_buf_will_dirty(mapreclaimed->dbuf_hdr, tx);
		dmu_buf_will_dirty(mapfilling->dbuf_hdr, tx);
		map_manager->mm_active_obj_index = reclaimed_obj;
		mapreclaimed->free_txg = 0;
		mapreclaimed->hdr->state = AGGRE_MAP_OBJ_FILLING;
		mapreclaimed->hdr->time = nows;
		mapfilling->hdr->state = AGGRE_MAP_OBJ_FILLED;
		mapfilling->hdr->time = nows;
		cmn_err(CE_NOTE, "%s %s active obj=%d", __func__,
			spa->spa_name, reclaimed_obj);
		return (mapreclaimed);
	} else {
		return (mapfilling);
	}
}

void
raidz_check_and_reclaim_space(spa_t *spa)
{
	aggre_map_t *map ;
	uint64_t pos, count, offset, i, avail_count;
	dmu_buf_t *dbuf = NULL;
	aggre_map_elem_t *elem;
	aggre_elem_state state;
	int err, index, tq_state;
	uint8_t *data = NULL;
	uint64_t timestampbegin;
	uint64_t timestampnow;
	uint64_t elapsed;
	dmu_tx_t *tx = NULL;
	int finish;
	
	map = raidz_aggre_get_reclaim_map(spa);
	if (map->hdr->state != AGGRE_MAP_OBJ_FILLED &&
		map->hdr->state != AGGRE_MAP_OBJ_RECLAIMING) {
		cmn_err(CE_NOTE, "%s %s obj=%d state=%d not reclaim", __func__, spa->spa_name,
			 spa->spa_map_manager.mm_active_obj_index, map->hdr->state);
		return;
	}
	
	avail_count = map->hdr->total_count - map->hdr->process_index;
	count = map->hdr->blksize / map->hdr->recsize;
	pos = map->hdr->process_index;
#ifdef _KERNEL	
	timestampbegin = ddi_get_time();
#endif
	tx = create_meta_tx(spa);
	if (!tx) {
		cmn_err(CE_WARN, "%s %s reclaim start, create meta tx failed", 
			__func__, spa->spa_name);
		return;
	}

	set_reclaim_state(spa, map->index, MAP_RECLAIM_START, timestampbegin, tx->tx_txg);
	commit_meta_tx(tx);
	cmn_err(CE_NOTE, "%s %s start pos=%"PRIu64" total=%"PRIu64" avail=%"PRIu64, 
		__func__, spa->spa_name, pos, map->hdr->total_count, avail_count);
	
	for (i = 0; i < avail_count; i++) {
		mutex_enter(&spa->spa_space_reclaim_lock);
		tq_state = spa->spa_space_reclaim_state;
		mutex_exit(&spa->spa_space_reclaim_lock);
		if (tq_state & (SPACE_RECLAIM_STOP | SPACE_RECLAIM_PAUSE)) {
			cmn_err(CE_NOTE, "%s %s tq_state=0x%x, go out", 
				__func__, spa->spa_name, tq_state);
			break;
		}
		
		index = pos % count;
		if (dbuf && index == 0)
			dmu_buf_rele(dbuf, FTAG);

		if (i == 0 || index == 0) {
			offset = pos / count * map->hdr->blksize;
			err = dmu_buf_hold(map->os, map->object, offset, FTAG, &dbuf, 0);
			if (err) {
				cmn_err(CE_WARN, "%s %s dmu_buf_hold err=%d, offset=0x%"PRIx64,
					__func__, spa->spa_name, err, offset);
				break;
			}
			data = (uint8_t *)dbuf->db_data + map->hdr->recsize * index;
		}

		ASSERT(data != NULL);
		elem = (aggre_map_elem_t *)data;
		err = raidz_aggre_process_elem(spa, pos , elem, &state);
		#if 0
		DTRACE_PROBE3(process__result, spa_t *, spa, 
			uint64_t, pos,
			aggre_elem_state, state);
		#endif
		
		if (err) {
			cmn_err(CE_WARN, "%s %s pos=%"PRIu64" process elem err=%d", 
				__func__, spa->spa_name, pos, err);
		}
		pos++;
		data += map->hdr->recsize;		
	}
	if (dbuf)
		dmu_buf_rele(dbuf, FTAG);
#ifdef _KERNEL
	timestampnow = ddi_get_time();
#endif
	elapsed = timestampnow - timestampbegin;

	finish = (i >= avail_count) ? 1 : 0;
	cmn_err(CE_NOTE, "%s %s end elapsed=%"PRIu64"(s) processed=%"PRIu64
		" finish=%d", __func__, spa->spa_name, elapsed, i, finish);

	if (finish) {
		tx = create_meta_tx(spa);
		if (!tx) {
			cmn_err(CE_WARN, "%s %s reclaim end, create meta tx failed", 
				__func__, spa->spa_name);
			return;
		}
	
		set_reclaim_state(spa, map->index, MAP_RECLAIM_END, timestampnow, tx->tx_txg);
		commit_meta_tx(tx);
	}
}

void
raidz_aggre_space_reclaim(void *arg)
{
	spa_t *spa = (spa_t *)arg;
	mutex_enter(&spa->spa_space_reclaim_lock);
	spa->spa_space_reclaim_state |= SPACE_RECLAIM_RUN;
	cmn_err(CE_WARN, "%s %s reclaim_state %x\n",__func__, spa->spa_name, spa->spa_space_reclaim_state);
	while (!(spa->spa_space_reclaim_state & SPACE_RECLAIM_STOP)) {		
		cv_timedwait(&spa->spa_space_reclaim_cv, &spa->spa_space_reclaim_lock,
			ddi_get_lbolt() + raidz_space_reclaim_gap * hz);
		
		cmn_err(CE_WARN, "%s %s %x\n",__func__, spa->spa_name, spa->spa_space_reclaim_state);
		if (spa->spa_space_reclaim_state & SPACE_RECLAIM_STOP)
			break;
	
		mutex_exit(&spa->spa_space_reclaim_lock);
		if (raidz_reclaim_enable && (spa->spa_space_reclaim_state & SPACE_RECLAIM_RUN))
			raidz_check_and_reclaim_space(spa);
		
		mutex_enter(&spa->spa_space_reclaim_lock);
	}

	spa->spa_space_reclaim_state &= ~(SPACE_RECLAIM_START | SPACE_RECLAIM_RUN | SPACE_RECLAIM_PAUSE);
	mutex_exit(&spa->spa_space_reclaim_lock);
	
	cmn_err(CE_WARN, "%s %s %x process_exit \n",__func__, spa->spa_name, spa->spa_space_reclaim_state);
}

void
start_space_reclaim_thread(spa_t *spa)
{
	if (!spa_is_raidz_aggre(spa))
		return;

	mutex_enter(&spa->spa_space_reclaim_lock);
	if (spa->spa_space_reclaim_state & SPACE_RECLAIM_RUN) {
		/* reclaim thread is running, no need to start */
		mutex_exit(&spa->spa_space_reclaim_lock);
		return;
	}
	
	spa->spa_space_reclaim_state = SPACE_RECLAIM_START;
	mutex_exit(&spa->spa_space_reclaim_lock);
	
	taskq_dispatch(spa->spa_space_reclaim_tq, raidz_aggre_space_reclaim, 
		spa, TQ_SLEEP);
}

void
stop_space_reclaim_thread(spa_t *spa)
{
	if (!spa_is_raidz_aggre(spa))
		return;
	
	mutex_enter(&spa->spa_space_reclaim_lock);
	if (spa->spa_space_reclaim_state == 0) {
		mutex_exit(&spa->spa_space_reclaim_lock);
		return;
	}
	
	spa->spa_space_reclaim_state |= SPACE_RECLAIM_STOP;
	cv_signal(&spa->spa_space_reclaim_cv);
	mutex_exit(&spa->spa_space_reclaim_lock);

	while (spa->spa_space_reclaim_state & SPACE_RECLAIM_RUN) {
		delay(drv_usectohz(100000));
	}

	mutex_enter(&spa->spa_space_reclaim_lock);
	spa->spa_space_reclaim_state = 0;
	mutex_exit(&spa->spa_space_reclaim_lock);
	
	cmn_err(CE_WARN, "%s %s %x\n",__func__, spa->spa_name, spa->spa_space_reclaim_state);
}

#else

void
start_space_reclaim_thread(spa_t *spa)
{

}

void
stop_space_reclaim_thread(spa_t *spa)
{

}

aggre_map_t *raidz_aggre_map_current(spa_t *spa, dmu_tx_t *tx)
{
	return (NULL);
}

#endif

#if defined(_KERNEL) && defined(HAVE_SPL)
module_param(raidz_reclaim_enable, int, 0644);
MODULE_PARM_DESC(raidz_reclaim_enable, "raidz reclaim switch");

module_param(raidz_space_reclaim_gap, ulong, 0644);
MODULE_PARM_DESC(raidz_space_reclaim_gap, "raidz reclaim time gap");

module_param(raidz_avail_map_thresh, ulong, 0644);
MODULE_PARM_DESC(raidz_avail_map_thresh, "raidz avail map threshold");

module_param(raidz_reclaim_count, int, 0644);
MODULE_PARM_DESC(raidz_reclaim_count, "raidz reclaim count");

module_param(raidz_filled_timeout, int, 0644);
MODULE_PARM_DESC(raidz_filled_timeout, "raidz filled timeout");

module_param(raidz_cleared_timeout, int, 0644);
MODULE_PARM_DESC(raidz_cleared_timeout, "raidz cleared timeout");

module_param(hole_adjust, int, 0644);
MODULE_PARM_DESC(hole_adjust, "raidz aggre map hole adjust param");

#endif
