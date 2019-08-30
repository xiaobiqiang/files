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
uint64_t rzaggre_map_pos_last = 0;

typedef enum {
	DEBUG_NORMAL = 0,
	DEBUG_DETAIL
} raidz_debug_level;

typedef enum {
	RECLAIM_PROGRESSIVE = 0,
	RECLAIM_CONSERVATIVE
} reclaim_mode;

int raidz_reclaim_enable = 1;
unsigned long raidz_space_reclaim_gap = 60*2;	/* unit: s */
unsigned long raidz_avail_map_thresh = 0x40000000;
int raidz_reclaim_count = 3000;
int raidz_filledtime_count = 60*60*24;
int raidz_clearedtime_count = 60*2;
extern const zio_vsd_ops_t vdev_raidz_vsd_ops;
extern void vdev_raidz_generate_parity(raidz_map_t *rm);
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

void 
raidz_aggre_check(spa_t *spa)
{
	vdev_t *root_vdev = spa->spa_root_vdev;
	vdev_t *vdev;
	int c = 0;
	int nchild = root_vdev->vdev_children;
	
	for (c = 0; c < nchild; c++) {
		vdev = spa->spa_root_vdev->vdev_child[c];
		if (strcmp(vdev->vdev_ops->vdev_op_type, 
			VDEV_TYPE_RAIDZ_AGGRE) == 0) {
			spa->spa_raidz_aggre_num = vdev->vdev_children - vdev->vdev_nparity;
			spa->spa_raidz_aggre_nparity = vdev->vdev_nparity;
			spa->spa_raidz_ashift = vdev->vdev_ashift;
			spa->spa_raidz_aggre = B_TRUE;
			cmn_err(CE_WARN, "%s %s is raidz aggre: aggre_num=%d parity=%d",
				__func__, spa->spa_name, spa->spa_raidz_aggre_num,
				spa->spa_raidz_aggre_nparity);
			break;
		}
	}
	spa->sap_map_pos_last = 0;

	if (!spa->spa_raidz_aggre)
		cmn_err(CE_WARN, "%s %s isn't raidz aggre", __func__, spa->spa_name);
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
	mutex_init(&an_cur_io->ai_synclock, NULL, MUTEX_DEFAULT, NULL); 
	cv_init(&an_cur_io->ai_synccv, NULL, CV_DEFAULT, NULL);
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
    uint64_t b, s, f, o;
	uint64_t q, r, c, bc, acols, scols, asize, tot, firstdatacol;
    uint32_t indexid = (uint32_t)BP_GET_BLKID(zio->io_bp);
	
    if (zio->io_type == ZIO_TYPE_WRITE) {
        if(zio->io_prop.zp_type != DMU_OT_PLAIN_FILE_CONTENTS &&
                zio->io_prop.zp_type != DMU_OT_ZVOL)
			cmn_err(CE_PANIC,"%s %p dnode type (%d) err ",__func__, zio, zio->io_type  );
	}
        
	b = zio->io_offset >> unit_shift;
	s = zio->io_size >> unit_shift;
	f = b % dcols;
	o = (b / dcols) << unit_shift;
        
    if(f!=0){
		cmn_err(CE_PANIC,"%s %p  b=%lx f=%lx dcols=%lx io_offset=%lx size=%lx ushift=%lx setobj %lx.%lx  ",
			__func__, zio, (long)b, (long)f, (long)dcols,
			(long)zio->io_offset,(long)zio->io_size,(long)unit_shift,
			(long)zio->io_bookmark.zb_objset,(long)zio->io_bookmark.zb_object);
	}
        
	q = s / (dcols - nparity);
	r = s - q * (dcols - nparity);
	bc = 0;
	tot = s + nparity * (q + (r == 0 ? 0 : 1));

	/*if (q == 0) {
		acols = bc;
		scols = MIN(dcols, roundup(bc, nparity + 1));
	} else {
		acols = dcols;
		scols = dcols;
	}*/
	if (zio->io_type == ZIO_TYPE_WRITE) {
	        acols = scols = indexid ? 1 : 1 + nparity;
            firstdatacol = indexid ? 0 : nparity;
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

	asize = 0;

	for (c = 0; c < scols; c++) {
                
		if (zio->io_type == ZIO_TYPE_WRITE)
			rm->rm_col[c].rc_devidx = indexid ? nparity + indexid : c;
		else
			rm->rm_col[c].rc_devidx = nparity + indexid ;
		rm->rm_col[c].rc_offset = o;
		rm->rm_col[c].rc_data = NULL;
		rm->rm_col[c].rc_gdata = NULL;
		rm->rm_col[c].rc_error = 0;
		rm->rm_col[c].rc_tried = 0;
		rm->rm_col[c].rc_skipped = 0;
        rm->rm_col[c].rc_size = zio->io_size;

		asize += rm->rm_col[c].rc_size;
	}

	ASSERT3U(acols, <=, scols); /* ? */
	
	rm->rm_asize = asize;//roundup(asize, (nparity + 1) << unit_shift);
	rm->rm_nskip = 0;//roundup(tot, nparity + 1) - tot;
	ASSERT3U(rm->rm_asize - asize, ==, rm->rm_nskip << unit_shift);
	ASSERT3U(rm->rm_nskip, <=, nparity);

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
        int c=0;
		raidz_map_t *rm;
        uint32_t scols = zio->io_vd->vdev_children;
		uint64_t iosize;
		uint64_t offset;
		uint64_t devindex;
      	vdev_t *cvd;
		vdev_t *vd = zio->io_vd;
		
		if ((*rmp_old)->rm_cols == scols)
			return;

		iosize = zio->io_size;
		offset = (*rmp_old)->rm_col[0].rc_offset;
		devindex = (*rmp_old)->rm_col[0].rc_devidx;

		rm = kmem_zalloc(offsetof(raidz_map_t, rm_col[scols]), KM_SLEEP);
		rm->rm_cols = scols;
		rm->rm_scols = scols;
        rm->rm_asize = zio->io_size * scols;
        rm->rm_firstdatacol = zio->io_vd->vdev_nparity;
        for (c = 0; c < scols; c++) {
                
			rm->rm_col[c].rc_devidx = c;
			rm->rm_col[c].rc_offset = offset;
			rm->rm_col[c].rc_gdata = NULL;
			rm->rm_col[c].rc_error = 0;
			rm->rm_col[c].rc_tried = 0;
			rm->rm_col[c].rc_skipped = 0;
        	rm->rm_col[c].rc_size = iosize;
            if (devindex == c) {
                    rm->rm_col[c].rc_data = zio->io_data;
                    rm->rm_aggre_col = c;
            } else {
                    rm->rm_col[c].rc_data = zio_buf_alloc(iosize);
            }
			
			cvd = vd->vdev_child[c];
			if (!vdev_readable(cvd)) {
				if (c >= rm->rm_firstdatacol)
					rm->rm_missingdata++;
				else
					rm->rm_missingparity++;
				rm->rm_col[c].rc_error = ENXIO;
				rm->rm_col[c].rc_tried = 1;	/* don't even try */
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
				rm->rm_col[c].rc_tried = 1;	/* don't even try */
				
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
raidz_aggre_free_bp(spa_t *spa, dva_t dva, uint64_t txg, dmu_tx_t *tx)
{
	blkptr_t bp;
	BP_ZERO(&bp);
	memcpy(&bp.blk_dva[0], &dva, sizeof (dva_t));
	BP_SET_BIRTH(&bp, txg, txg);
	bplist_append(&spa->spa_free_bplist[tx->tx_txg & TXG_MASK], &bp);
}

void 
set_aggre_map_process_pos(spa_t *spa, uint64_t pos, uint64_t txg)
{
	int index = txg & TXG_MASK;
	mutex_enter(&spa->spa_map_process_pos[index].mtx);
	spa->spa_map_process_pos[index].pos = pos;
	spa->spa_map_process_pos[index].valid = B_TRUE;
	mutex_exit(&spa->spa_map_process_pos[index].mtx);
}

void raidz_aggre_map_free_range_all(spa_t *spa, dmu_tx_t *tx)
{
	int filling_obj;
	int reclaimed_obj;
	aggre_map_t *map ;
	
	filling_obj = spa->spa_map_manager.mm_active_obj_index;
	reclaimed_obj = filling_obj==0?1:0;

	if (spa->spa_space_reclaim_state != (SPACE_RECLAIM_START |SPACE_RECLAIM_RUN)) {
		cmn_err(CE_WARN, "%s %s return not run %x\n",	
			__func__, spa->spa_name, spa->spa_space_reclaim_state);
		return;
	}

	map = spa->spa_aggre_map_arr[reclaimed_obj];
	if (map->hdr->aggre_map_state != AGGRE_MAP_OBJ_RECLAIMED) {
		return;
	}
	if (ddi_get_time()-map->hdr->aggre_map_filltime <raidz_clearedtime_count) {
		return;
	}
	
 	dmu_buf_will_dirty(map->dbuf_hdr, tx);
	map->dbuf_id = 0;
	map->hdr->free_index = 0;
	map->hdr->total_count = 0;	
	map->hdr->avail_count = 0;	
	map->hdr->process_index = 0;
	map->hdr->aggre_map_state = AGGRE_MAP_OBJ_CLEAR;
	#ifdef _KERNEL 
	map->hdr->aggre_map_filltime = ddi_get_time();
	#endif
	cmn_err(CE_WARN, "%s %s dmu_free_range obj:%d \n", __func__,spa->spa_name, reclaimed_obj);
 	dmu_free_range(map->os, map->object, 0, DMU_OBJECT_END, tx);
}

void raidz_aggre_map_free_1open(spa_t *spa, dmu_tx_t *tx)
{
	aggre_map_t *map;
	uint64_t offset;
	int err, i;
	int loop;
	
	for (loop = 0; loop < AGGRE_MAP_MAX_OBJ_NUM; loop++) {
		map = spa->spa_aggre_map_arr[loop]; 
	
		dmu_buf_will_dirty(map->dbuf_hdr, tx);
		map->hdr->free_index = 0;
		map->hdr->total_count = 0;	
		map->hdr->avail_count = 0;
		map->hdr->process_index = 0;
		map->hdr->aggre_map_state = AGGRE_MAP_OBJ_CLEAR;
	#ifdef _KERNEL 
			map->hdr->aggre_map_filltime = ddi_get_time();
	#endif
		dmu_free_range(map->os, map->object, 0, DMU_OBJECT_END, tx);
		
		map->dbuf_id = 0;
		for (i = 0; i < map->dbuf_num; i++) {
			dmu_buf_impl_t *db ;
			offset = (map->dbuf_id + i) * map->dbuf_size;
			err = dmu_buf_hold(spa->spa_meta_objset, spa->spa_map_obj_arr[loop], offset, spa->spa_aggre_map_arr[loop], 
				&map->dbuf_array[i], 0);
			if (err) 
				cmn_err(CE_WARN, "%s dmu_buf_hold i=%d err=%d", __func__, i, err);
			
			db = (dmu_buf_impl_t *)map->dbuf_array[i];
			cmn_err(CE_WARN, "%s  db=%p stat = %d",	__func__, db, db->db_state );
		}
	}
	
	return;
}

void
raidz_aggre_create_map_obj(spa_t *spa, dmu_tx_t *tx, int aggre_num)
{
	objset_t *mos = spa->spa_meta_objset;
	dmu_buf_t *dbp;
	aggre_map_hdr_t *hdr;
	int i;

	for (i=0; i<AGGRE_MAP_MAX_OBJ_NUM; i++) {
		spa->spa_map_obj_arr[i] = dmu_object_alloc(mos, DMU_OT_RAIDZ_AGGRE_MAP,
	    	SPA_MAXBLOCKSIZE, DMU_OT_RAIDZ_AGGRE_MAP_HDR, sizeof (aggre_map_hdr_t), tx);

		VERIFY(0 == dmu_bonus_hold(mos, spa->spa_map_obj_arr[i], FTAG, &dbp));
	
		hdr = dbp->db_data;
		dmu_buf_will_dirty(dbp, tx);

		hdr->aggre_num = aggre_num;
		hdr->recsize = offsetof(aggre_map_elem_t, blkid) + aggre_num * sizeof(uint64_t);
		hdr->blksize = SPA_MAXBLOCKSIZE;
		hdr->total_count = 0;
		hdr->avail_count = 0;
		hdr->process_index = 0;
		hdr->free_index = 0;
		hdr->aggre_map_state = AGGRE_MAP_OBJ_CLEAR;
		#ifdef _KERNEL
		hdr->aggre_map_filltime = ddi_get_time();
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
	int err, i;
	int loop;
	for (loop = 0; loop < AGGRE_MAP_MAX_OBJ_NUM; loop++) {
		err = dmu_object_info(spa->spa_meta_objset, spa->spa_map_obj_arr[loop], &doi);
		if (err) {
			cmn_err(CE_WARN, "%s get object info failed", __func__);
			return (err);
		}
		
		ASSERT3U(doi.doi_type, ==, DMU_OT_RAIDZ_AGGRE_MAP);
		ASSERT3U(doi.doi_bonus_type, ==, DMU_OT_RAIDZ_AGGRE_MAP_HDR);
	
		map = spa->spa_aggre_map_arr[loop] = kmem_zalloc(sizeof(aggre_map_t), KM_SLEEP);
		err = dmu_bonus_hold(spa->spa_meta_objset, spa->spa_map_obj_arr[loop], map, &map->dbuf_hdr);
		if (err) {
			cmn_err(CE_WARN, "%s hold bonus buf failed", __func__);
			return (err);
		}
	
		map->dbuf_num = AGGRE_MAP_MAX_DBUF_NUM;
		map->dbuf_array = (dmu_buf_t **)kmem_zalloc(sizeof(dmu_buf_t *) * map->dbuf_num, KM_SLEEP);
		mutex_init(&map->aggre_lock, NULL, MUTEX_DEFAULT, NULL);

		map->hdr = (aggre_map_hdr_t *)map->dbuf_hdr->db_data;
		map->os = spa->spa_meta_objset;
		map->object = spa->spa_map_obj_arr[loop];
		map->dbuf_size = doi.doi_data_block_size;
		map->dbuf_id = 0;
		
		cmn_err(CE_WARN, "%s %s  id:%d active_obj_index:%d \n", 
			__func__, spa->spa_name, loop , spa->spa_map_manager.mm_active_obj_index );
		if (spa->spa_map_manager.mm_active_obj_index == -1)
			continue;
					
		cmn_err(CE_WARN, "%s %s dbuf_num[%d] dbuf_size[%d] dmu_free_range \n", 
			__func__, spa->spa_name, map->dbuf_num, map->dbuf_size);
		for (i = 0; i < map->dbuf_num; i++) {
			dmu_buf_impl_t *db ;
			offset = (map->dbuf_id + i) * map->dbuf_size;
			err = dmu_buf_hold(spa->spa_meta_objset, spa->spa_map_obj_arr[loop], offset, spa->spa_aggre_map_arr[loop], 
				&map->dbuf_array[i], 0);
			if (err) {
				cmn_err(CE_WARN, "%s dmu_buf_hold i=%d err=%d", __func__, i, err);
			}
			db = (dmu_buf_impl_t *)map->dbuf_array[i];
			
			cmn_err(CE_WARN, "%s  db=%p stat = %d", __func__,db,db->db_state );
		}
	}
	
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
	if (dbuf_id >= map->dbuf_id + map->dbuf_num) {
		for (i = 0; i < map->dbuf_num; i++) {
			if (map->dbuf_array[i]!=NULL)
				dmu_buf_rele(map->dbuf_array[i], map);
		}

		map->dbuf_id = dbuf_id;
		for (i = 0; i < map->dbuf_num; i++) {
			offset = (map->dbuf_id + i) * map->dbuf_size;
			dmu_buf_hold(map->os, map->object, offset, map, 
				&map->dbuf_array[i], 0);
		}
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

	/*
	if (raidz_avail_map_thresh > 0) {
		if (map->hdr->avail_count > raidz_avail_map_thresh)
			cv_signal(&spa->spa_space_reclaim_cv);
	}*/
	
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
	
	cmn_err(CE_WARN, "[SGG] raidz_aggre_map_close %s", spa->spa_name);
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
	
	for (i = 0; i < map->hdr->aggre_num; i++) {
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
 	*state = ELEM_STATE_ERR;

	do {
		rrw_enter(&dp->dp_config_rwlock, RW_READER, FTAG);
		err = dsl_dataset_hold_obj(spa->spa_dsl_pool, elem->objsetid, FTAG, &ds);
		if (err) {
			rrw_exit(&dp->dp_config_rwlock, FTAG);
			cmn_err(CE_WARN, "%s dsl_dataset_hold_obj err=%d dsl_pool=%p objsetid=%"PRIu64, 
				__func__, err, spa->spa_dsl_pool, elem->objsetid);
			if (err == ENOENT)
				*state = ELEM_STATE_FREE;
			break;
		}

		rrw_exit(&dp->dp_config_rwlock, FTAG);
		err = dmu_objset_from_ds(ds, &os);
		if (err) {
			cmn_err(CE_WARN, "%s dmu_objset_from_ds err=%d", __func__, err);
			break;
		}
		
		err = dnode_hold(os, elem->objectid, FTAG, &dn);
		if (err) {
			cmn_err(CE_WARN, "%s dnode_hold err=%d os=%p objectid=%"PRIu64, 
				__func__, err, os, elem->objectid);
			
			if (err == ENOENT)
				*state = ELEM_STATE_FREE; 
			break;
		}

		size = sizeof(dmu_buf_impl_t *) * map->hdr->aggre_num;
		dbp = (dmu_buf_impl_t **)kmem_zalloc(size, KM_SLEEP);
		
		for (i = 0; i < map->hdr->aggre_num; i++) {
			rw_enter(&dn->dn_struct_rwlock, RW_READER);
			dbp[i] = dbuf_hold(dn, elem->blkid[i], FTAG);
			rw_exit(&dn->dn_struct_rwlock);
			if ((dbp[i] && (dbp[i]->db_blkptr == NULL ||
				dbp[i]->db_blkptr->blk_birth != elem->txg)) ||
				(dbp[i] == NULL)) {
				change_num++;
			}
		}

		if (change_num == map->hdr->aggre_num) {
			*state = ELEM_STATE_FREE;
		} else if (change_num == 0) {
			*state = ELEM_STATE_NO_CHANGE;
		} else {
			*state = ELEM_STATE_REWRITE;
		} 
		
	} while (0);

	if (*state == ELEM_STATE_REWRITE) {
	
		tx = dmu_tx_create(dn->dn_objset);
		for (i = 0; i < map->hdr->aggre_num; i++) {
			if (dbp[i] && dbp[i]->db_blkptr &&
				dbp[i]->db_blkptr->blk_birth == elem->txg) {
				dmu_tx_hold_write(tx, dn->dn_object, 
					dbp[i]->db_blkid << dn->dn_datablkshift,
					dn->dn_datablksz);
			}
		}

		err = dmu_tx_assign(tx, TXG_WAIT);
		if (err) {
			cmn_err(CE_WARN, "%s txg assign failed, err=%d",
				__func__, err);

			dmu_tx_abort(tx);
		
		} else {
			for (i = 0; i < map->hdr->aggre_num; i++) {
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

			raidz_aggre_free_bp(spa, elem->dva, elem->txg, tx);

			/* update map meta data */
			set_aggre_map_process_pos(spa, pos, tx->tx_txg);
			dmu_tx_commit(tx);
		}
	} else if (*state == ELEM_STATE_NO_CHANGE ||
		*state == ELEM_STATE_FREE) {
		tx = dmu_tx_create_dd(NULL);
		tx->tx_pool = spa->spa_dsl_pool;
		tx->tx_objset = spa->spa_meta_objset;
		
		err = dmu_tx_assign(tx, TXG_WAIT);
		if (err) {
			cmn_err(CE_WARN, "%s txg assign failed, err=%d",
				__func__, err);
			dmu_tx_abort(tx);
			
		} else {
			if (*state == ELEM_STATE_NO_CHANGE) {
				aggre_map_elem_t *new_elem;
				raidz_aggre_elem_clone(spa, elem, &new_elem);
				clist_append(&spa->spa_aggre_maplist[tx->tx_txg & TXG_MASK], new_elem);
			} else {
				raidz_aggre_free_bp(spa, elem->dva, elem->txg, tx);
			}
		
			/* update map meta data */
			set_aggre_map_process_pos(spa, pos, tx->tx_txg);
			dmu_tx_commit(tx);
		}
	}

	if (dbp) {
		for (i = 0; i < map->hdr->aggre_num; i++)
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
	
	obj_index = obj_index==0 ? 1: 0;
	
	return spa->spa_aggre_map_arr[obj_index];
}

aggre_map_t *raidz_aggre_map_current(spa_t *spa)
{
	int filling_obj;
	int reclaimed_obj;
	aggre_map_t *mapfilling;
	aggre_map_t *mapreclaimed;
	uint64_t nows;
	aggre_map_manager_t *map_manager;

	if(!(spa->spa_space_reclaim_state & SPACE_RECLAIM_START))
		return NULL;
		
	map_manager = &spa->spa_map_manager;
	
	filling_obj = map_manager->mm_active_obj_index;
	reclaimed_obj = filling_obj==0?1:0;
	
	mapfilling = spa->spa_aggre_map_arr[filling_obj];
	mapreclaimed = spa->spa_aggre_map_arr[reclaimed_obj];
	#ifdef _KERNEL	
	nows = ddi_get_time();
	#endif
	
	if (spa->spa_sync_pass>1) {
		return mapfilling;
	}
	if ( ( mapfilling->hdr->avail_count>raidz_reclaim_count &&
				nows-mapfilling->hdr->aggre_map_filltime>raidz_filledtime_count)
			&& mapreclaimed->hdr->aggre_map_state == AGGRE_MAP_OBJ_CLEAR
			 && (nows-mapreclaimed->hdr->aggre_map_filltime>raidz_clearedtime_count))
	{
		map_manager->mm_active_obj_index = reclaimed_obj;
		mapreclaimed->hdr->aggre_map_filltime = nows;
		mapreclaimed->hdr->aggre_map_state = AGGRE_MAP_OBJ_FILLING;
		mapfilling->hdr->aggre_map_state = AGGRE_MAP_OBJ_FILLED;
		cmn_err(CE_WARN, "%s %s , ativeobj=%d\n", __func__, spa->spa_name, reclaimed_obj);
		return mapreclaimed;
	} else {
		return mapfilling;
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
	hrtime_t timestampbegin;
	hrtime_t timestampnow;
	uint32_t elapsed_ms = 0;
	
	map = raidz_aggre_get_reclaim_map(spa);
	if (map->hdr->aggre_map_state != AGGRE_MAP_OBJ_FILLED)
	{
		cmn_err(CE_WARN, "%s %s ,obj=%d stat=%d not reclaim\n", __func__, spa->spa_name,
			 map->hdr->aggre_map_state,spa->spa_map_manager.mm_active_obj_index);
		return;
	}

	map->hdr->aggre_map_state = AGGRE_MAP_OBJ_RECLAIMING;	
	avail_count = map->hdr->avail_count;
	count = map->hdr->blksize / map->hdr->recsize;
	pos = 0;
	timestampbegin = gethrtime();
	cmn_err(CE_WARN, "%s %s , avail_count=%d\n", __func__, spa->spa_name,avail_count);
	for (i = 0; i < avail_count; ) {
		mutex_enter(&spa->spa_space_reclaim_lock);
		tq_state = spa->spa_space_reclaim_state;
		mutex_exit(&spa->spa_space_reclaim_lock);
		if (tq_state & (SPACE_RECLAIM_STOP | SPACE_RECLAIM_PAUSE)) {
			cmn_err(CE_NOTE, "%s tq_state=0x%x, go out", __func__, tq_state);
			break;
		}
		
		index = pos % count;
		if (dbuf && index == 0)
			dmu_buf_rele(dbuf, FTAG);

		if (i == 0 || index == 0) {
			offset = pos / count * map->hdr->blksize;
			err = dmu_buf_hold(map->os, map->object, offset, FTAG, &dbuf, 0);
			if (err) {
				cmn_err(CE_WARN, "%s dmu_buf_hold err=%d, offset=0x%"PRIx64,
					__func__, err, offset);
				break;
			}
			data = (uint8_t *)dbuf->db_data + map->hdr->recsize * index;
		}

		ASSERT(data != NULL);
		elem = (aggre_map_elem_t *)data;
		err = raidz_aggre_process_elem(spa, pos + 1, elem, &state);
		#if 0
		DTRACE_PROBE3(process__result, spa_t *, spa, 
			uint64_t, pos,
			aggre_elem_state, state);
		#endif
		
		if (err) {
			cmn_err(CE_WARN, "%s pos=%"PRIu64" process elem err=%d", 
				__func__, pos, err);
		}
		pos++;
		i++;
		data += map->hdr->recsize;
		map->hdr->process_index++;
		
	}
	if (dbuf)
		dmu_buf_rele(dbuf, FTAG);
	
	map->hdr->aggre_map_state = AGGRE_MAP_OBJ_RECLAIMED;	
	#ifdef _KERNEL	
		map->hdr->aggre_map_filltime = ddi_get_time();
	#endif
	timestampnow = gethrtime();
	elapsed_ms =  (timestampnow - timestampbegin) / 1000000;
  	
	cmn_err(CE_WARN, "%s %s ,elapsed:%d avail_count=%d\n", __func__, spa->spa_name,elapsed_ms,avail_count);
}

void
raidz_aggre_space_reclaim(void *arg)
{
	spa_t *spa = (spa_t *)arg;
	mutex_enter(&spa->spa_space_reclaim_lock);
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
	if (!spa->spa_raidz_aggre)
		return;

	spa->spa_space_reclaim_state = SPACE_RECLAIM_START;
	taskq_dispatch(spa->spa_space_reclaim_tq, raidz_aggre_space_reclaim, 
		spa, TQ_SLEEP);
}

void
stop_space_reclaim_thread(spa_t *spa)
{
	if (!spa->spa_raidz_aggre)
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

aggre_map_t *raidz_aggre_map_current(spa_t *spa)
{
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

module_param(raidz_filledtime_count, int, 0644);
MODULE_PARM_DESC(raidz_filledtime_count, "raidz filledtime count");

module_param(raidz_clearedtime_count, int, 0644);
MODULE_PARM_DESC(raidz_clearedtime_count, "raidz clearedtime count");

#endif
