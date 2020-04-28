#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <sys/cluster_san.h>
#include <sys/mltsas/mlt_sas.h>

#define strtoul             simple_strtoul

static uint32_t Mlsas_npending = 0;

static Mlsas_t Mlsas;
static Mlsas_t *gMlsas_ptr = &Mlsas;

const char *Mlsas_devst_name[Mlsas_Devst_Last] = {
	[Mlsas_Devst_Standalone] 	= 	"Standalone",
	[Mlsas_Devst_Attaching]	 	= 	"Attaching",
	[Mlsas_Devst_Failed] 		= 	"Failed",
	[Mlsas_Devst_Degraded] 		= 	"Degraded",
	[Mlsas_Devst_Healthy] 		= 	"Healthy",
};

const char *Mlsas_devevt_name[Mlsas_Devevt_Last] = {
	[Mlsas_Devevt_To_Attach] 		= 	"To_Attach_Phys",
	[Mlsas_Devevt_Attach_Error]		= 	"Attach_Error",
	[Mlsas_Devevt_Attach_OK]		=	"Attach_OK"
};

static int Mlsas_Disk_Open(struct block_device *, fmode_t);
static void Mlsas_Disk_Release(struct gendisk *, fmode_t);

static struct block_device_operations Mlsas_disk_ops = {
	.owner =   THIS_MODULE,
	.open =    Mlsas_Disk_Open,
	.release = Mlsas_Disk_Release,
};

static void __Mlsas_RQ_Retry(struct work_struct *w);
static void __Mlsas_Complete_Master_Bio(Mlsas_request_t *rq, 
		Mlsas_bio_and_error_t *Mlbi);
static void __Mlsas_Complete_RQ(Mlsas_request_t *rq, 
		Mlsas_bio_and_error_t *Mlbi);
static void __Mlsas_Restart_DelayedRQ(Mlsas_request_t *rq);
static uint32_t __Mlsas_Put_RQcomplete_Ref(Mlsas_request_t *rq,
		uint32_t c_put, Mlsas_bio_and_error_t *Mlbi);
static void __Mlsas_Req_St(Mlsas_request_t *rq, 
		Mlsas_bio_and_error_t *Mlbi, 
		uint32_t clear, uint32_t set);
static void __Mlsas_Req_Stmt(Mlsas_request_t *rq, uint32_t what,
		Mlsas_bio_and_error_t *Mlbi);
static void Mlsas_Request_endio(struct bio *bio);
static void Mlsas_Put_Request(Mlsas_request_t *rq, uint32_t k_put);
static Mlsas_request_t *Mlsas_New_Request(Mlsas_blkdev_t *Mlb, 
		struct bio *bio, uint64_t start_jif);
static void Mlsas_Mkrequest_Prepare(Mlsas_blkdev_t *Mlb, struct bio *bio, 
		uint64_t start_jif, Mlsas_request_t **rqpp);
static void Mlsas_Submit_Local_Prepare(Mlsas_request_t *rq);
static void Mlsas_Submit_Backing_Bio(Mlsas_request_t *rq);
static void __Mlsas_Do_Policy(Mlsas_blkdev_t *Mlb, 
		boolean_t *do_local, boolean_t *do_remote);
static void Mlsas_Submit_or_Send(Mlsas_request_t *req);
static void __Mlsas_Make_Request_impl(Mlsas_blkdev_t *Mlb, 
		struct bio *bio, uint64_t start_jif);
static blk_qc_t Mlsas_Make_Request_fn(struct request_queue *rq, struct bio *bio);
static int Mlsas_Path_to_Hashkey(const char *path, uint64_t *hash_key);
static void Mlsas_Destroy_Gendisk(Mlsas_blkdev_t *Mlbp);
static int Mlsas_New_Gendisk(Mlsas_blkdev_t *Mlbp);
static void Mlsas_Destroy_Blockdevice(struct kref *ref);
static int Mlsas_New_Blockdevice(char *path, Mlsas_blkdev_t **Mlbpp);
static int Mlsas_Do_NewMinor(Mlsas_iocdt_t *Mlip);
static int Mlsas_Do_EnableSvc(void);
static int Mlsas_Do_Attach(Mlsas_iocdt_t *Mlip);
static void __Mlsas_Devst_St(Mlsas_blkdev_t *Mlb, 
		Mlsas_devst_e newst, uint32_t what);
static void __Mlsas_Devst_Stmt(Mlsas_blkdev_t *Mlb, uint32_t what);
int Mlsas_Attach_Phys(const char *path, struct block_device *bdev,
		struct block_device **new_bdev);


static int Mlsas_Disk_Open(struct block_device *bdev, fmode_t mode)
{
	Mlsas_blkdev_t *Mlb = bdev->bd_disk->private_data;
	
	atomic_inc_64(&Mlb->Mlb_nopen);

	return 0;
}

static void Mlsas_Disk_Release(struct gendisk *vdisk, fmode_t mode)
{
	Mlsas_blkdev_t *Mlb = vdisk->private_data;

	atomic_dec_64(&Mlb->Mlb_nopen);
}

static inline void Mlsas_Make_Backing_Bio(Mlsas_request_t *rq, struct bio *bio_src)
{
	struct bio *backing = bio_clone(bio_src, GFP_NOIO);
	
	rq->Mlrq_back_bio = backing;
	backing->bi_next = NULL;
	backing->bi_end_io = Mlsas_Request_endio;
	backing->bi_private = rq;
}

static inline void __Mlsas_Start_Ioacct(Mlsas_blkdev_t *Mlb, Mlsas_request_t *rq)
{
	generic_start_io_acct(bio_data_dir(rq->Mlrq_master_bio),
		rq->Mlrq_bsize >> 9, &Mlb->Mlb_gdisk->part0);
}

static inline void __Mlsas_End_Ioacct(Mlsas_blkdev_t *Mlb, Mlsas_request_t *rq)
{
	generic_end_io_acct(bio_data_dir(rq->Mlrq_master_bio),
		&Mlb->Mlb_gdisk->part0, rq->Mlrq_start_jif);
}


static int Mlsas_Copyin_Iocdt(uint64_t caddr, Mlsas_iocdt_t *iocp)
{
	int rval = 0;
	__user Mlsas_iocdt_t *uiocp = (Mlsas_iocdt_t *)caddr;

	bzero(iocp, sizeof(Mlsas_iocdt_t));

	if (!caddr)
		return (0);
	
	iocp->Mlioc_magic = Mlsas_Ioc_Magic;
	
	if (uiocp->Mlioc_magic != Mlsas_Ioc_Magic)
		return -EINVAL;
	
	if (uiocp->Mlioc_nibuf) {
		iocp->Mlioc_nibuf = uiocp->Mlioc_nibuf;
		iocp->Mlioc_ibufptr = kmem_zalloc(iocp->Mlioc_nibuf, KM_SLEEP);
		ASSERT(NULL != iocp->Mlioc_ibufptr);
		
		if ((rval = ddi_copyin(uiocp->Mlioc_ibufptr, iocp->Mlioc_ibufptr, 
				iocp->Mlioc_nibuf, 0)) != 0) {
			kmem_free(iocp->Mlioc_ibufptr, iocp->Mlioc_nibuf);
			return rval;
		}
	}

	if (uiocp->Mlioc_nobuf) {
		iocp->Mlioc_nobuf = uiocp->Mlioc_nobuf;
		iocp->Mlioc_nofill = 0;
		iocp->Mlioc_obufptr = kmem_zalloc(iocp->Mlioc_nobuf, KM_SLEEP);
		ASSERT(NULL != iocp->Mlioc_obufptr);
	}

	return 0;
}

static int Mlsas_Copyout_Iocdt(uint64_t caddr, Mlsas_iocdt_t *iocp)
{
	int rval;
	__user Mlsas_iocdt_t *uiocp = (Mlsas_iocdt_t *)caddr;

	if (!uiocp)
		return 0;
	
	if (iocp->Mlioc_magic != Mlsas_Ioc_Magic)
		return -EINVAL;

	uiocp->Mlioc_nofill = iocp->Mlioc_nofill;
	
	if (iocp->Mlioc_nobuf) {
		ASSERT(NULL != iocp->Mlioc_obufptr);
		if ((rval = ddi_copyout(iocp->Mlioc_obufptr, uiocp->Mlioc_obufptr, 
				iocp->Mlioc_nofill, 0)) != 0)
			return rval;
	}

	return 0;
}


static int Mlsas_Open(struct inode *inode, struct file *f)
{
	return (0);
}

static int Mlsas_Ioctl(struct file *fp, unsigned int cmd, 
		unsigned long priv)
{
	int rval = 0, old_St;
	Mlsas_iocdt_t ioc;
	
	if ((cmd <= Mlsas_Ioc_First) || 
		(cmd >= Mlsas_Ioc_Last))
		return -EINVAL;

	mutex_enter(&gMlsas_ptr->Ml_mtx);
	old_St = gMlsas_ptr->Ml_state;
	switch (old_St) {
	case Mlsas_St_Disabled:
		if (cmd != Mlsas_Ioc_Ensvc)
			rval = -EINVAL;
		else
			gMlsas_ptr->Ml_state = Mlsas_St_Enabling;
		break;
	case Mlsas_St_Enabled:
		if (cmd == Mlsas_Ioc_Ensvc)
			rval = -EALREADY;
		else if (cmd != Mlsas_Ioc_Diesvc)
			gMlsas_ptr->Ml_state = Mlsas_St_Busy;
		break;
	case Mlsas_St_Enabling:
	case Mlsas_St_Busy:
		rval = -EAGAIN;
		break;
	}
	mutex_exit(&gMlsas_ptr->Ml_mtx);

	if (rval != 0)
		return rval;
	
	if ((rval = Mlsas_Copyin_Iocdt(
			priv, &ioc)) != 0)
		goto out;

	switch (cmd) {
	case Mlsas_Ioc_Ensvc:
		ASSERT(!ioc.Mlioc_nibuf && !ioc.Mlioc_nobuf);
		rval = Mlsas_Do_EnableSvc();
		break;
	case Mlsas_Ioc_Newminor:
		rval = Mlsas_Do_NewMinor(&ioc);
		break;
	case Mlsas_Ioc_Attach:
		rval = Mlsas_Do_Attach(&ioc);
		break;
	}

	if (rval == 0) {
		if ((rval = Mlsas_Copyout_Iocdt(
				priv, &ioc)) != 0)
			cmn_err(CE_WARN, "%s cmd(%d) copy out error(%d)",
				__func__, cmd, rval);
	}

	if (ioc.Mlioc_ibufptr && ioc.Mlioc_nibuf)
		kmem_free(ioc.Mlioc_ibufptr, ioc.Mlioc_nibuf);
	if (ioc.Mlioc_obufptr && ioc.Mlioc_nobuf)
		kmem_free(ioc.Mlioc_obufptr, ioc.Mlioc_nobuf);
out:
	if (gMlsas_ptr->Ml_state == Mlsas_St_Busy)
		gMlsas_ptr->Ml_state = old_St;
	return rval;
}

static int Mlsas_Do_EnableSvc(void)
{
	Mlsas_retry_t *retry = &gMlsas_ptr->Ml_retry;
	
	gMlsas_ptr->Ml_devices = mod_hash_create_idhash(
		"Mlsas_devices", 1024, NULL);
	gMlsas_ptr->Ml_skc = kmem_cache_create("Mlsas_req_skc",
		sizeof(Mlsas_request_t), 0, 
		SLAB_RECLAIM_ACCOUNT | SLAB_PANIC, NULL);
	gMlsas_ptr->Ml_request_mempool = mempool_create_slab_pool(
		256, gMlsas_ptr->Ml_skc);
	gMlsas_ptr->Ml_minor = 0;

	spin_lock_init(&retry->Mlt_lock);
	list_create(&retry->Mlt_writes, sizeof(Mlsas_request_t),
		offsetof(Mlsas_request_t, Mlrq_node));
	retry->Mlt_workq = create_singlethread_workqueue("Mlsas_retry");
	INIT_WORK(&retry->Mlt_work, __Mlsas_RQ_Retry);
	
	gMlsas_ptr->Ml_state = Mlsas_St_Enabled;

	cmn_err(CE_NOTE, "Mltsas Enable Complete.");
	return 0;
}

static int Mlsas_Do_NewMinor(Mlsas_iocdt_t *Mlip)
{
	int rval = 0;
	nvlist_t *invlp = NULL;
	char *dpath = NULL;
	Mlsas_blkdev_t *Mlb = NULL;
	uint64_t hash_key = 0;
	
	if (!Mlip->Mlioc_ibufptr || !Mlip->Mlioc_nibuf ||
		((rval = nvlist_unpack(Mlip->Mlioc_ibufptr, 
			Mlip->Mlioc_nibuf, &invlp, 
			KM_SLEEP)) != 0)) {
		cmn_err(CE_NOTE, "%s Nullptr(%p) or Zerolen(%u) "
			"or Unpack(%p) Fail, Error(%d)", __func__,
			Mlip->Mlioc_ibufptr, Mlip->Mlioc_nibuf,
			invlp, rval);
		return -EINVAL;
	}

	if (((rval = nvlist_lookup_string(invlp, 
			"path", &dpath)) != 0) ||
		((rval = Mlsas_Path_to_Hashkey(
			dpath, &hash_key)) != 0) ||
		((rval = mod_hash_find(gMlsas_ptr->Ml_devices,
			hash_key, &Mlb)) == 0) ||
		((rval = Mlsas_New_Blockdevice(
			dpath, &Mlb)) != 0)) {
		cmn_err(CE_WARN, "%s New Minor Fail by Path(%s) or "
			"PathtoHash(0x%llx) or Already Exist(%p) or "
			"New Blockdevice(%p), Error(%d)", __func__, 
			dpath ?: "nullpath", hash_key, Mlb, Mlb, rval);
		goto failed_out;
	}

	kref_get(&Mlb->Mlb_ref);

	if ((rval = mod_hash_insert(gMlsas_ptr->Ml_devices, 
			Mlb->Mlb_hashkey, Mlb)) != 0) {
		cmn_err(CE_NOTE, "%s Almost Duplicate Create Device(%s)",
			__func__, dpath);
		goto failed_destroy;
	} 

	cmn_err(CE_NOTE, "%s New Minor Device(%s 0x%llx) Complete.",
		__func__, dpath, hash_key);

	nvlist_free(invlp);
	return (0);
	
failed_destroy:
	kref_put(&Mlb->Mlb_ref, Mlsas_Destroy_Blockdevice);
	Mlsas_Destroy_Blockdevice(&Mlb->Mlb_ref);
failed_out:
	nvlist_free(invlp);
	return rval;
}

static int Mlsas_Do_Attach(Mlsas_iocdt_t *Mlip)
{
	int rval = 0;
	nvlist_t *invlp = NULL;
	char *path = NULL;
	struct block_device *phys = NULL, *virt = NULL;
	
	if (!Mlip->Mlioc_ibufptr || !Mlip->Mlioc_nibuf ||
		((rval = nvlist_unpack(Mlip->Mlioc_ibufptr, 
			Mlip->Mlioc_nibuf, &invlp, 
			KM_SLEEP)) != 0) ||
		((rval = nvlist_lookup_string(invlp, 
			"path", &path)) != 0)) {
		cmn_err(CE_NOTE, "%s Nullptr(%p) or Zerolen(%u) "
			"or Unpack(%p) or Nullpath(%s) Fail, Error(%d)", 
			__func__, Mlip->Mlioc_ibufptr, Mlip->Mlioc_nibuf,
			invlp, path ?: "Nullpath", rval);
		goto out;
	}

	cmn_err(CE_NOTE, "%s To Attach Disk Path(%s)",
		__func__, path);

	if (IS_ERR_OR_NULL(phys = blkdev_get_by_path(path, 
			FMODE_READ | FMODE_WRITE, Mlip)) ||
		((rval = Mlsas_Attach_Phys(path, 
			phys, &virt)) != 0)) {
		cmn_err(CE_NOTE, "%s blkdev_get_by_path(%d) or "
			"Mlsas_Attach_Phys Fail, Error(%d)", 
			__func__, PTR_ERR(phys), rval);
		if (!IS_ERR_OR_NULL(phys))
			blkdev_put(phys, FMODE_READ | 
				FMODE_WRITE);
		rval = -EFAULT;
		goto out;
	}

	cmn_err(CE_NOTE, "%s Attached Phys Disk(%s) Complete.",
		__func__, path);

out:
	if (invlp)
		nvlist_free(invlp);
	return rval;
}

static int Mlsas_New_Blockdevice(char *path, Mlsas_blkdev_t **Mlbpp)
{
	int rval = 0;
	Mlsas_blkdev_t *Mlbp = NULL;
	struct block_device *block_device = NULL;
	uint64_t hash_key = 0;
	
	if ((Mlbp = kzalloc(sizeof(Mlsas_blkdev_t), 
			GFP_KERNEL)) == NULL)
		return -ENOMEM;
	
	Mlbp->Mlb_st = Mlsas_Devst_Standalone;
	spin_lock_init(&Mlbp->Mlb_rq_spin);
	kref_init(&Mlbp->Mlb_ref);
	list_create(&Mlbp->Mlb_local_rqs, sizeof(Mlsas_request_t),
		offsetof(Mlsas_request_t, Mlrq_node));
	list_create(&Mlbp->Mlb_peer_rqs, sizeof(Mlsas_request_t),
		offsetof(Mlsas_request_t, Mlrq_node));
	list_create(&Mlbp->Mlb_pr_devices, sizeof(Mlsas_pr_device_t),
		offsetof(Mlsas_pr_device_t, Mlpd_node));

	if (((rval = Mlsas_Path_to_Hashkey(path, 
			&hash_key)) != 0) ||
		((rval = Mlsas_New_Gendisk(Mlbp)) != 0)) {
		cmn_err(CE_WARN, "%s Invalid Path(%s) or "
			"New Disk Fail, ERR(%d)", __func__,
			path, rval);
		
		kfree(Mlbp);
		return rval;
	}
	
	Mlbp->Mlb_hashkey = hash_key;
	*Mlbpp = Mlbp;
	
	return rval;
}

static void Mlsas_Destroy_Blockdevice(struct kref *ref)
{
	
}

static int Mlsas_New_Gendisk(Mlsas_blkdev_t *Mlbp)
{
	int rval = -ENOMEM;
	struct gendisk *disk = NULL;
	struct request_queue *rq = NULL;
	
	if ((disk = alloc_disk(1)) == NULL)
		goto failed;
	set_disk_ro(disk, true);

	if ((rq = blk_alloc_queue(GFP_KERNEL)) == NULL)
		goto free_disk;
	blk_queue_make_request(rq, Mlsas_Make_Request_fn);
	blk_queue_flush(rq, REQ_FLUSH | REQ_FUA);
	/* 1024KB max request bio */
	blk_queue_max_hw_sectors(rq, Mlsas_Mxbio_Size);
	/* max sglist phys segment */
	blk_queue_max_segments(rq, Mlsas_Mxbio_Segment);
	/* dma alignment */
	blk_queue_dma_alignment(rq, Mlsas_DMA_Alimask);
	/* no limits */
	blk_queue_bounce_limit(rq, BLK_BOUNCE_ANY);
	rq->queue_lock = &Mlbp->Mlb_rq_spin;
	rq->queuedata = Mlbp;
	
	disk->queue = rq;
	disk->major = Mlsas_MAJOR;
	disk->first_minor = atomic_inc_32_nv(&gMlsas_ptr->Ml_minor);
	disk->fops = &Mlsas_disk_ops;
	sprintf(disk->disk_name, "Mlsas%u", disk->first_minor);
	disk->private_data = Mlbp;
	Mlbp->Mlb_this = bdget(MKDEV(Mlsas_MAJOR, disk->first_minor));
	Mlbp->Mlb_this->bd_contains = Mlbp->Mlb_this;
	Mlbp->Mlb_rq = rq;
	Mlbp->Mlb_gdisk = disk;
	
	add_disk(disk);
	
	return 0;
	
free_disk:
	put_disk(disk);
failed:
	return rval;
}

static void Mlsas_Destroy_Gendisk(Mlsas_blkdev_t *Mlbp)
{
	
}

static int Mlsas_Path_to_Hashkey(const char *path, uint64_t *hash_key)
{
	const char *delim1 = "scsi-3";
	const char *delim2 = "-part";
	char *ptr = NULL, *ptr2 = NULL, *ptr3 = NULL;
	uint64_t hash = 0;
	int len = 0, rval = -EINVAL;
	
	if ((ptr = strstr(path, delim1)) == NULL)
		goto out;

	ptr += strlen(delim1);
	if ((len = strlen(ptr)) < 16)
		goto out;
	else if (len == 16) {
		hash = strtoul(ptr, &ptr2, 16);
		if (ptr2 < ptr + 16)
			goto out;
	} else {
		if (((ptr2 = strstr(ptr, delim2)) == NULL) ||
				((ptr2 - ptr) != 16ul))
			goto out;

		hash = strtoul(ptr, &ptr3, 16);
		if (ptr3 != ptr2)
			goto out;
	}

	*hash_key = hash;
	rval = 0;
	
out:
	return rval;
}

static int __Mlsas_Attach_BDI(Mlsas_backdev_info_t *bdi,
		const char *path, 
		struct block_device *phys_dev)
{
	struct block_device *total_bdev = phys_dev;
	struct block_device *part_bdev = NULL;
	
	if (phys_dev->bd_contains && 
		phys_dev->bd_contains != phys_dev) {
		part_bdev = phys_dev;
		total_bdev = phys_dev->bd_contains;
	}

	if (part_bdev && IS_ERR(total_bdev = blkdev_get(
			total_bdev, FMODE_READ | FMODE_WRITE,
			bdi))) {
		cmn_err(CE_NOTE, "blkdev_get Total(%s) Fail, Err(%d)",
			path, PTR_ERR(total_bdev));
		return PTR_ERR(total_bdev);
	}

	strncpy(bdi->Mlbd_path, path, 128);
	bdi->Mlbd_part_bdev = part_bdev;
	bdi->Mlbd_bdev = total_bdev;
	bdi->Mlbd_gdisk = total_bdev->bd_disk;

	return (0);
}

static void __Mlsas_Attach_Virt_Queue(Mlsas_blkdev_t *Mlb)
{
	Mlsas_backdev_info_t *bdi = &Mlb->Mlb_bdi;
	struct request_queue *phys_rq = bdi->Mlbd_gdisk->queue;
	struct request_queue *virt_rq = Mlb->Mlb_rq;
	uint32_t max_hw_sectors = Mlsas_Mxbio_Size;
	uint32_t max_bio_segments = Mlsas_Mxbio_Segment;

	blk_set_stacking_limits(&virt_rq->limits);
	
	max_hw_sectors = min(queue_max_hw_sectors(phys_rq), max_hw_sectors >> 9);
	max_bio_segments = min(queue_max_segments(phys_rq), max_bio_segments);
	blk_queue_max_hw_sectors(virt_rq, max_hw_sectors);
	blk_queue_max_segments(virt_rq, max_bio_segments);
	blk_queue_max_write_same_sectors(virt_rq, 0);
	blk_queue_logical_block_size(virt_rq, 512);
	blk_queue_segment_boundary(virt_rq, PAGE_CACHE_SIZE-1);

	/*
	 * capable of discard.
	 */
	if (blk_queue_discard(phys_rq)) {
		blk_queue_max_discard_sectors(virt_rq, 0);
		queue_flag_set_unlocked(QUEUE_FLAG_DISCARD, virt_rq);
		if (blk_queue_secdiscard(phys_rq))
			queue_flag_set_unlocked(QUEUE_FLAG_SECDISCARD, virt_rq);
	}

	ASSERT(blk_queue_stack_limits(virt_rq, phys_rq) == 0);

	if (virt_rq->backing_dev_info.ra_pages != 
			phys_rq->backing_dev_info.ra_pages) {
		cmn_err(CE_NOTE, "Disk(%s) RQ ra_pages adjust from %u to %u",
			bdi->Mlbd_path, virt_rq->backing_dev_info.ra_pages,
			phys_rq->backing_dev_info.ra_pages);
		virt_rq->backing_dev_info.ra_pages = phys_rq->backing_dev_info.ra_pages;
	}
}

static void __Mlsas_Attach_Phys_size(Mlsas_blkdev_t *Mlb)
{
	Mlsas_backdev_info_t *bdi = &Mlb->Mlb_bdi;
	struct block_device *backing = bdi->Mlbd_bdev;
	struct block_device *virt_bdev = Mlb->Mlb_this;
	sector_t nr_sect = 0;
	
	if (bdi->Mlbd_part_bdev)
		backing = bdi->Mlbd_part_bdev;
	
	nr_sect = i_size_read(backing->bd_inode) >> 9;
	set_capacity(Mlb->Mlb_gdisk, nr_sect);
	i_size_write(virt_bdev->bd_inode, nr_sect << 9);
}

static int __Mlsas_Attach_Phys_impl(Mlsas_blkdev_t *Mlb, 
		const char *path, 
		struct block_device *phys_dev,
		struct block_device **virt_dev)
{
	int rval = 0;
	uint32_t what = 0;
	Mlsas_backdev_info_t *bdi = NULL;

	spin_lock_irq(&Mlb->Mlb_rq_spin);
	if (__Mlsas_Get_ldev_if_state(Mlb, Mlsas_Devst_Attaching)) {
		spin_unlock_irq(&Mlb->Mlb_rq_spin);
		return -EALREADY;
	}
	__Mlsas_Devst_Stmt(Mlb, Mlsas_Devevt_To_Attach);
	spin_unlock_irq(&Mlb->Mlb_rq_spin);

	if ((rval = __Mlsas_Attach_BDI(&Mlb->Mlb_bdi, 
			path, phys_dev)) != 0) {
		what = Mlsas_Devevt_Attach_Error;
		goto out;
	}
	
	__Mlsas_Attach_Virt_Queue(Mlb);
	__Mlsas_Attach_Phys_size(Mlb);
	
	set_disk_ro(Mlb->Mlb_gdisk, 0);

	*virt_dev = Mlb->Mlb_this;
	what = Mlsas_Devevt_Attach_OK;

out:
	__Mlsas_Devst_Stmt(Mlb, what);
	return rval;
}

int Mlsas_Attach_Phys(const char *path, struct block_device *bdev,
		struct block_device **new_bdev)
{
	int rval = 0;
	uint64_t hash_key = 0;
	Mlsas_blkdev_t *Mlb = NULL;
	
	if (((rval = Mlsas_Path_to_Hashkey(path, 
			&hash_key)) != 0) ||
		((rval = mod_hash_find(gMlsas_ptr->Ml_devices,
			hash_key, &Mlb)) != 0)) {
		cmn_err(CE_WARN, "%s Invalid Path(%s) or "
			"Mod_hash Find None(%p), Error(%d)", 
			__func__, path, Mlb, rval);
		return rval;
	}

	if (!kref_get_unless_zero(&Mlb->Mlb_ref))
		return -EFAULT;

	if ((rval = __Mlsas_Attach_Phys_impl(Mlb, path, 
			bdev, new_bdev)) != 0) {
		cmn_err(CE_NOTE, "%s Attach Phys Disk fail, "
			"Maybe Already or Blkdev_get Fail, Error(%d)",
			__func__, rval);
		kref_put(&Mlb->Mlb_ref, Mlsas_Destroy_Blockdevice);
		return rval;
	}

	return rval;
}
EXPORT_SYMBOL(Mlsas_Attach_Phys);

static blk_qc_t Mlsas_Make_Request_fn(struct request_queue *rq, struct bio *bio)
{
	uint64_t start_jif = 0;
	Mlsas_blkdev_t *Mlb = rq->queuedata;
	
	blk_queue_split(rq, &bio, rq->bio_split);
	
	start_jif = jiffies;
	atomic_inc_32(&Mlb->Mlb_npending);
	atomic_inc_32(&Mlsas_npending);
	
	__Mlsas_Make_Request_impl(Mlb, bio, start_jif);

	return BLK_QC_T_NONE;
}

static void __Mlsas_Make_Request_impl(Mlsas_blkdev_t *Mlb, 
		struct bio *bio, uint64_t start_jif)
{
	Mlsas_request_t *req = NULL;
	
	Mlsas_Mkrequest_Prepare(Mlb, bio, start_jif, &req);
	if (IS_ERR_OR_NULL(req))
		return ;

	Mlsas_Submit_or_Send(req);
}

static void Mlsas_Submit_or_Send(Mlsas_request_t *req)
{
	Mlsas_blkdev_t *Mlb = req->Mlrq_bdev;
	boolean_t do_local = B_FALSE, do_remote = B_FALSE;
	boolean_t submit_backing = B_FALSE;
	
	spin_lock_irq(&Mlb->Mlb_rq_spin);
	__Mlsas_Do_Policy(Mlb, &do_local, &do_remote);

	switch ((do_local << 1) | do_remote) {
	case 0x0:
		break;
	case 0x1:
		break;
	case 0x2:
		Mlsas_Submit_Local_Prepare(req);
		__Mlsas_Req_Stmt(req, Mlsas_Rst_Submit_Local, NULL);
		submit_backing = B_TRUE;
		break;
	default:
		ASSERT(0);
	}

	spin_unlock_irq(&Mlb->Mlb_rq_spin);

	if (submit_backing)
		Mlsas_Submit_Backing_Bio(req);
}

static void __Mlsas_Do_Policy(Mlsas_blkdev_t *Mlb, 
		boolean_t *do_local, boolean_t *do_remote)
{
	*do_local = 1;
	*do_remote = 0;
}

static void Mlsas_Submit_Backing_Bio(Mlsas_request_t *rq)
{
	Mlsas_blkdev_t *Mlb = rq->Mlrq_bdev;
	struct bio *bio = rq->Mlrq_back_bio;
	
	ASSERT(rq->Mlrq_back_bio != NULL);

	if (!Mlsas_Get_ldev(Mlb)) 
		bio_io_error(bio);
	else {
		rq->Mlrq_submit_jif = jiffies;
		generic_make_request(bio);
	}
}

static void Mlsas_Submit_Local_Prepare(Mlsas_request_t *rq)
{
	Mlsas_blkdev_t *Mlb = rq->Mlrq_bdev;
	Mlsas_backdev_info_t *bdi = &Mlb->Mlb_bdi;
	struct bio *bio = rq->Mlrq_back_bio;
	struct block_device *bdev;
	struct hd_struct *p;

	bio->bi_bdev = bdi->Mlbd_bdev;
	/*
	 * it's a partion
	 */
	if (bdi->Mlbd_part_bdev != NULL) {
		bdev = bdi->Mlbd_part_bdev;

		if (bio_sectors(bio) && bdev != bdev->bd_contains) {
			p = bdev->bd_part;
			bio->bi_iter.bi_sector += p->start_sect;
			rq->Mlrq_sector = bio->bi_iter.bi_sector;
		}
	}
	
//	list_insert_tail(&bdi->Mldb_pending_rqs, rq);
	kref_get(&rq->Mlrq_ref);
	list_insert_tail(&Mlb->Mlb_local_rqs, rq);
}

static void Mlsas_Mkrequest_Prepare(Mlsas_blkdev_t *Mlb, struct bio *bio, 
		uint64_t start_jif, Mlsas_request_t **rqpp)
{
	int rw = bio_data_dir(bio);
 	Mlsas_request_t *Mlr = NULL;
	int bi_error = -ENOMEM;

	if (!(Mlr = Mlsas_New_Request(Mlb, 
			bio, start_jif)))
		goto failed;

	bi_error = -ENODEV;
	if (!Mlsas_Get_ldev(Mlb)) 
		goto free_rq;

	__Mlsas_Start_Ioacct(Mlb, Mlr);

	*rqpp = Mlr;
	return ;
	
free_rq:
	Mlsas_Put_Request(Mlr, 1);
failed:
	atomic_dec_32(&Mlsas_npending);
	atomic_dec_32(&Mlb->Mlb_npending);
	bio->bi_error = bi_error;
	bio_endio(bio);
}

static Mlsas_request_t *Mlsas_New_Request(Mlsas_blkdev_t *Mlb, 
		struct bio *bio, uint64_t start_jif)
{
	Mlsas_request_t *rq = NULL;
	int rw = bio_data_dir(bio);
	
	if ((rq = mempool_alloc(
			gMlsas_ptr->Ml_request_mempool,
			GFP_NOIO)) == NULL)
		return NULL;

	bzero(rq, sizeof(Mlsas_request_t));
	kref_init(&rq->Mlrq_ref);
	rq->Mlrq_master_bio = bio;
	rq->Mlrq_flags = (rw == WRITE ? Mlsas_RQ_Write : 0);
	kref_get(&Mlb->Mlb_ref);
	rq->Mlrq_bdev = Mlb;
	rq->Mlrq_start_jif = start_jif;
	rq->Mlrq_sector = bio->bi_iter.bi_sector;
	rq->Mlrq_bsize = bio->bi_iter.bi_size;
	Mlsas_Make_Backing_Bio(rq, bio);

	return rq;
}

static void __Mlsas_Release_RQ(struct kref *ref)
{
	Mlsas_request_t *rq = container_of(ref, 
		Mlsas_request_t, Mlrq_ref);

	if (rq->Mlrq_back_bio)
		bio_put(rq->Mlrq_back_bio);
	if (rq->Mlrq_bdev)
		kref_put(&rq->Mlrq_bdev->Mlb_ref, 
			Mlsas_Destroy_Blockdevice);
	rq->Mlrq_back_bio = NULL;
	rq->Mlrq_bdev = NULL;
	mempool_free(rq, gMlsas_ptr->Ml_request_mempool);
}

static void Mlsas_Put_Request(Mlsas_request_t *rq, uint32_t k_put)
{
	kref_sub(&rq->Mlrq_ref, k_put, __Mlsas_Release_RQ);
}

static void Mlsas_Request_endio(struct bio *bio)
{
	uint32_t what;
	unsigned long flags;
	Mlsas_request_t *rq = bio->bi_private;
	Mlsas_blkdev_t *Mlb = rq->Mlrq_bdev;
	Mlsas_bio_and_error_t Mlbi;
	
	if (unlikely(bio->bi_error)) {
		if (unlikely(bio->bi_rw & REQ_DISCARD))
			what = Mlsas_Rst_Discard_Error;
		else
			what = (rq->Mlrq_flags & Mlsas_RQ_Write) ? 
				Mlsas_Rst_Write_Error : (bio_rw(bio) == READ) ? 
					Mlsas_Rst_Read_Error : Mlsas_Rst_ReadA_Error;
	} else
		what = Mlsas_Rst_Complete_OK;

	bio_put(rq->Mlrq_back_bio);
	rq->Mlrq_back_bio = ERR_PTR(bio->bi_error);

	spin_lock_irqsave(&Mlb->Mlb_rq_spin, flags);
	__Mlsas_Req_Stmt(rq, what, &Mlbi);
	spin_unlock_irqrestore(&Mlb->Mlb_rq_spin, flags);

	if (Mlbi.Mlbi_bio)
		__Mlsas_Complete_Master_Bio(rq, &Mlbi);
}

static void __Mlsas_Devst_Stmt(Mlsas_blkdev_t *Mlb, uint32_t what)
{
	switch (Mlb->Mlb_st) {
	case Mlsas_Devst_Standalone:
		if (what != Mlsas_Devevt_To_Attach)
			break;
		/*
		 * only single thread to attach
		 */
		__Mlsas_Devst_St(Mlb, Mlsas_Devst_Attaching, what);
		break;
	case Mlsas_Devst_Attaching:
		if (likely(what == Mlsas_Devevt_Attach_OK))
			__Mlsas_Devst_St(Mlb, Mlsas_Devst_Healthy, what);
		else if (what == Mlsas_Devevt_Attach_Error)
			__Mlsas_Devst_St(Mlb, Mlsas_Devst_Standalone, what);
		break;
	}
}

static void __Mlsas_Devst_St(Mlsas_blkdev_t *Mlb, 
		Mlsas_devst_e newst, uint32_t what)
{
	cmn_err(CE_NOTE, "Mlsas Device(%s) Devst(%s --> %s), event(%s)",
		Mlb->Mlb_bdi.Mlbd_path, Mlsas_devst_name[Mlb->Mlb_st],
		Mlsas_devst_name[newst], Mlsas_devevt_name[what]);

	Mlb->Mlb_st = newst;
}

static void __Mlsas_Req_Stmt(Mlsas_request_t *rq, uint32_t what,
		Mlsas_bio_and_error_t *Mlbi)
{
	Mlsas_blkdev_t *Mlb = rq->Mlrq_bdev;

	if (Mlbi)
		Mlbi->Mlbi_bio = NULL;

	rq->Mlrq_state = what;
	
	switch (what) {
	case Mlsas_Rst_Submit_Local:
		__Mlsas_Req_St(rq, Mlbi, 0, Mlsas_RQ_Local_Pending);
		break;
	case Mlsas_Rst_Submit_Net:
		__Mlsas_Req_St(rq, Mlbi, 0, Mlsas_RQ_Net_Pending);
		break;
	/*
	 * nothing to differ each now, todo.
	 */
	case Mlsas_Rst_Write_Error:
	case Mlsas_Rst_Read_Error:
	case Mlsas_Rst_ReadA_Error:
	case Mlsas_Rst_Discard_Error:
		__Mlsas_Req_St(rq, Mlbi, Mlsas_RQ_Local_Pending, 
			Mlsas_RQ_Local_Done);
		break;
	case Mlsas_Rst_Complete_OK:
		__Mlsas_Req_St(rq, Mlbi, Mlsas_RQ_Local_Pending, 
			Mlsas_RQ_Local_Done | Mlsas_RQ_Local_OK);
		break;
	}
}

static void __Mlsas_Req_St(Mlsas_request_t *rq, 
		Mlsas_bio_and_error_t *Mlbi, 
		uint32_t clear, uint32_t set)
{
	Mlsas_blkdev_t *Mlb = rq->Mlrq_bdev;
	uint32_t oflg = rq->Mlrq_flags;
	uint32_t c_put = 0, k_put = 0;
	
	rq->Mlrq_flags &= ~clear;
	rq->Mlrq_flags |= set;

	if (rq->Mlrq_flags == oflg)
		return ;

	if (!(oflg & Mlsas_RQ_Local_Pending) &&
			(set & Mlsas_RQ_Local_Pending))
		atomic_inc_32(&rq->Mlrq_completion_ref);

	if (!(oflg & Mlsas_RQ_Net_Pending) &&
			(set & Mlsas_RQ_Net_Pending))
		atomic_inc_32(&rq->Mlrq_completion_ref);

	if ((oflg & Mlsas_RQ_Local_Pending) &&
		(clear & Mlsas_RQ_Local_Pending)) {
		c_put++;
		k_put++;
		list_remove(&Mlb->Mlb_local_rqs, rq);
	}

	if ((oflg & Mlsas_RQ_Net_Pending) &&
		(clear & Mlsas_RQ_Net_Pending))
		c_put++;

	if ((oflg & Mlsas_RQ_Net_Queued) &&
		(clear & Mlsas_RQ_Net_Queued))
		c_put++;

	if (c_put)
		k_put += __Mlsas_Put_RQcomplete_Ref(rq, c_put, Mlbi);
	if (Mlbi && Mlbi->Mlbi_bio)
		Mlbi->k_put = k_put;
	else if (k_put)
		ASSERT(kref_sub(&rq->Mlrq_ref, k_put, 
			__Mlsas_Release_RQ) != 0);
}

static uint32_t __Mlsas_Put_RQcomplete_Ref(Mlsas_request_t *rq,
		uint32_t c_put, Mlsas_bio_and_error_t *Mlbi)
{
	ASSERT(Mlbi || rq->Mlrq_flags & Mlsas_RQ_Delayed);

	if (atomic_sub_32_nv(&rq->Mlrq_completion_ref, c_put))
		return 0;

	__Mlsas_Complete_RQ(rq, Mlbi);

	if (rq->Mlrq_flags & Mlsas_RQ_Delayed) {
		__Mlsas_Restart_DelayedRQ(rq);
		return 0;
	}

	return 1;
}

static void __Mlsas_Restart_DelayedRQ(Mlsas_request_t *rq)
{
	unsigned long flags;
	Mlsas_retry_t *retry = &gMlsas_ptr->Ml_retry;
	
	spin_lock_irqsave(&retry->Mlt_lock, flags);
	list_insert_tail(&retry->Mlt_writes, rq);
	spin_unlock_irqrestore(&retry->Mlt_lock, flags);

	queue_work(retry->Mlt_workq, &retry->Mlt_work);
}

static void __Mlsas_Complete_RQ(Mlsas_request_t *rq, 
		Mlsas_bio_and_error_t *Mlbi)
{
	uint32_t flags = rq->Mlrq_flags;
	Mlsas_blkdev_t *Mlb = rq->Mlrq_bdev;
	int ok, error;
	
	ASSERT(rq->Mlrq_master_bio != NULL);
	
	if (((flags & Mlsas_RQ_Local_Pending) && 
		!(flags & Mlsas_RQ_Local_Aborted))) {
		cmn_err(CE_PANIC, "%s Mlsas_RQ_Local_Pending && "
			"!Mlsas_RQ_Local_Aborted", __func__);
	}

	__Mlsas_End_Ioacct(Mlb, rq);

	ok = rq->Mlrq_flags & (Mlsas_RQ_Local_OK |
		Mlsas_RQ_Net_OK);
	error = PTR_ERR(rq->Mlrq_back_bio);
	rq->Mlrq_back_bio = NULL;

	if (!ok && Mlsas_Get_ldev(Mlb))
		rq->Mlrq_flags |= Mlsas_RQ_Delayed;

	if (!(rq->Mlrq_flags & Mlsas_RQ_Delayed)) {
		Mlbi->Mlbi_bio = rq->Mlrq_master_bio;
		Mlbi->Mlbi_error = ok ? 0 :
			(error ?: -EIO);
		rq->Mlrq_master_bio = NULL;
	}
}

static void __Mlsas_Complete_Master_Bio(Mlsas_request_t *rq, 
		Mlsas_bio_and_error_t *Mlbi)
{
	atomic_dec_32(&rq->Mlrq_bdev->Mlb_npending);
	atomic_dec_32(&Mlsas_npending);
	if (Mlbi->k_put)
		kref_sub(&rq->Mlrq_ref, Mlbi->k_put, 
			__Mlsas_Release_RQ);
	Mlbi->Mlbi_bio->bi_error = Mlbi->Mlbi_error;
	bio_endio(Mlbi->Mlbi_bio);
}

static void __Mlsas_RQ_Retry(struct work_struct *w)
{
	list_t writes;
	Mlsas_request_t *rq;
	Mlsas_retry_t *retry = container_of(
		w, Mlsas_retry_t, Mlt_work);
	
	list_create(&writes, sizeof(Mlsas_request_t),
		offsetof(Mlsas_request_t, Mlrq_node));

	spin_lock_irq(&retry->Mlt_lock);
	list_move_tail(&writes, &retry->Mlt_writes);
	spin_unlock_irq(&retry->Mlt_lock);

	while ((rq = list_remove_head(&writes)) != NULL) {
		Mlsas_blkdev_t *Mlb = rq->Mlrq_bdev;
		struct bio *bio = rq->Mlrq_master_bio;
		uint64_t start_jif = rq->Mlrq_start_jif;

		cmn_err(CE_NOTE, "request restart");

		Mlsas_Put_Request(rq, 1);
		
		atomic_inc_32(&Mlb->Mlb_npending);

		__Mlsas_Make_Request_impl(Mlb, bio, start_jif);
	}
}

static void Mlsas_Init(Mlsas_t *Mlsp)
{	
	bzero(Mlsp, sizeof(Mlsas_t));
	Mlsp->Ml_state = Mlsas_St_Disabled;
	mutex_init(&Mlsp->Ml_mtx, NULL, MUTEX_DEFAULT, NULL);
}

static struct file_operations Mlsas_drv_fops = {
	.owner = THIS_MODULE,
	.open = Mlsas_Open,
	.unlocked_ioctl = Mlsas_Ioctl,
}; 

static struct miscdevice Mlsas_dev = {
	.minor	= MISC_DYNAMIC_MINOR,
	.name   = MLSAS_NAME,
	.fops   = &Mlsas_drv_fops,
};

static int __init __Mlsas_Init(void)
{
	int iRet = 0;
	
	if ((iRet = misc_register(&Mlsas_dev)) != 0) {
		pr_warn(MLSAS_NAME " register error(%d)", iRet);
		goto out;
	}

	Mlsas_Init(gMlsas_ptr);
out:
	return iRet;
}

static void __exit __Mlsas_Exit(void)
{
	return ;
}

module_param(Mlsas_npending, int, 0644);

module_init(__Mlsas_Init);
module_exit(__Mlsas_Exit);
MODULE_LICENSE("GPL");
