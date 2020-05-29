#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <sys/cluster_san.h>
#include <sys/mltsas/mlt_sas.h>

#ifdef kmem_cache_t
#undef kmem_cache_t
#endif
#ifdef kmem_cache_create
#undef kmem_cache_create
#endif
#ifdef kmem_cache_alloc
#undef kmem_cache_alloc
#endif
#ifdef kmem_cache_free
#undef kmem_cache_free
#endif
#ifdef kmem_cache_destroy
#undef kmem_cache_destroy
#endif

#define strtoul             simple_strtoul

static void __Mlsas_Retry(struct work_struct *w);
static void __Mlsas_Complete_RQ(Mlsas_request_t *rq, 
		Mlsas_bio_and_error_t *Mlbi);
static void __Mlsas_Restart_DelayedRQ(Mlsas_Delayed_obj_t *obj);
static uint32_t __Mlsas_Put_RQcomplete_Ref(Mlsas_request_t *rq,
		uint32_t c_put, Mlsas_bio_and_error_t *Mlbi);
static void __Mlsas_Req_St(Mlsas_request_t *rq, 
		Mlsas_bio_and_error_t *Mlbi, 
		uint32_t clear, uint32_t set);
static void __Mlsas_Request_endio(struct bio *bio);
static void Mlsas_RQ_put(Mlsas_request_t *rq, uint32_t k_put);
static Mlsas_request_t *__Mlsas_New_Request(Mlsas_blkdev_t *Mlb, 
		struct bio *bio, uint64_t start_jif);
static void __Mlsas_Mkrequest_Prepare(Mlsas_blkdev_t *Mlb, struct bio *bio, 
		uint64_t start_jif, Mlsas_request_t **rqpp);
static void __Mlsas_Submit_Local_Prepare(Mlsas_request_t *rq);
static void __Mlsas_Submit_Net_Prepare(Mlsas_request_t *rq);
static void __Mlsas_Submit_Backing_Bio(Mlsas_request_t *rq);
static void __Mlsas_Do_Policy(Mlsas_blkdev_t *Mlb, Mlsas_request_t *rq,
		boolean_t *do_local, boolean_t *do_remote);
static void __Mlsas_Submit_or_Send(Mlsas_request_t *req);
static void __Mlsas_Make_Request_impl(Mlsas_blkdev_t *Mlb, 
		struct bio *bio, uint64_t start_jif);
static blk_qc_t __Mlsas_Make_Request_fn(struct request_queue *rq, struct bio *bio);
static int Mlsas_Path_to_Hashkey(const char *path, uint64_t *hash_key);
static void Mlsas_Destroy_Gendisk(Mlsas_blkdev_t *Mlbp);
static void __Mlsas_Alloc_Virt_disk(Mlsas_blkdev_t *Mlbp);
static void __Mlsas_Rele_Virt(struct kref *ref);
static void __Mlsas_New_Virt(uint64_t hash_key, Mlsas_blkdev_t **Mlbpp);
static int Mlsas_Do_NewMinor(Mlsas_iocdt_t *Mlip);
static int Mlsas_Do_Failoc(Mlsas_iocdt_t *Mlip);
static int Mlsas_Do_EnableSvc(void);
static int Mlsas_Do_Attach(Mlsas_iocdt_t *Mlip);
static int Mlsas_Do_Virtinfo(Mlsas_iocdt_t *dt);
static void __Mlsas_Do_Virtinfo_impl(Mlsas_blkdev_t *vt, 
		Mlsas_virtinfo_return_t *vti);
static int __Mlsas_Resume_failoc_virt(Mlsas_blkdev_t *vt);
static int __Mlsas_New_minor_impl(const char *path, uint64_t hashkey, 
		Mlsas_blkdev_t **vtptr);
static int __Mlsas_Attach_BDI(Mlsas_backdev_info_t *bdi,
		const char *path, 
		struct block_device *phys_dev);
static void __Mlsas_Attach_Virt_Queue(Mlsas_blkdev_t *Mlb);
static void __Mlsas_Devst_St(Mlsas_blkdev_t *Mlb, 
		Mlsas_devst_e newst, uint32_t what);
static void __Mlsas_Devst_Stmt(Mlsas_blkdev_t *Mlb, uint32_t what);
int Mlsas_Attach_Phys(const char *path, struct block_device *bdev,
		struct block_device **new_bdev);
static void __Mlsas_Free_Mms(Mlsas_Msh_t *mms);
static Mlsas_Msh_t *__Mlsas_Alloc_Mms(uint32_t extsz, uint8_t type, 
		uint64_t hashkey, void **dt_ptr);
static void Mlsas_RX(cs_rx_data_t *xd, void *priv);
int Mlsas_TX(void *session, void *header, uint32_t hdlen,
		void *dt, uint32_t dtlen, boolean_t io);
static int __Mlsas_Do_Aggregate_Virt_impl(const char *path,
		Mlsas_blkdev_t *Mlb);
static void __Mlsas_Attach_Local_Phys(struct block_device *phys, 
		const char *path, Mlsas_blkdev_t *Mlb);
static void __Mlsas_Update_PR_st(Mlsas_pr_device_t *pr, 
		Mlsas_devst_e st);
static Mlsas_pr_device_t *__Mlsas_Lookup_PR_locked(
		Mlsas_blkdev_t *Mlb, uint32_t hostid);
static Mlsas_pr_device_t *__Mlsas_Alloc_PR(uint32_t id, Mlsas_devst_e st,
		Mlsas_blkdev_t *Mlb, Mlsas_rh_t *rh);
static void __Mlsas_RX_Attach_impl(cs_rx_data_t *xd);
static void __Mlsas_RX_Attach(Mlsas_Msh_t *mms,
		cs_rx_data_t *xd);
static Mlsas_rh_t *__Mlsas_Alloc_RH(uint32_t id, void *tran_ss);
static boolean_t __Mlsas_Has_Active_PR(Mlsas_blkdev_t *Mlb, 
		Mlsas_pr_device_t *prflt);
static void __Mlsas_Release_RH(struct kref *ref);
static void __Mlsas_PR_RQ_Read_endio(Mlsas_pr_req_t *prr);
static void __Mlsas_PR_RQ_Write_endio(Mlsas_pr_req_t *prr);
static void __Mlsas_Complete_PR_RQ(Mlsas_pr_req_t *prr);
static void __Mlsas_PR_RQ_endio(struct bio *bio);
struct bio *__Mlsas_PR_RQ_bio(Mlsas_pr_req_t *prr, unsigned long rw, 
		void *data, uint64_t dlen);
static void __Mlsas_PR_Read_Rsp_copy(struct bio *bio, 
		void *rbuf, uint32_t rblen);
static int __Mlsas_RX_Brw_Rsp_impl(Mlsas_rtx_wk_t *w);
static void __Mlsas_RX_Brw_Rsp(Mlsas_Msh_t *mms, cs_rx_data_t *xd);
static void __Mlsas_RX_Bio_RW(Mlsas_Msh_t *mms, cs_rx_data_t *xd);
static void __Mlsas_Partion_Map_toTtl(Mlsas_request_t *rq);
static void __Mlsas_Do_Failoc_impl(Mlsas_blkdev_t *Mlb);
static void __Mlsas_Attach_Phys_size(Mlsas_blkdev_t *Mlb);
static void __Mlsas_Release_PR_RQ(struct kref *);
static void __Mlsas_Conn_Evt_fn(cluster_san_hostinfo_t *cshi,
		cts_link_evt_t link_evt, void *arg);
static void __Mlsas_PR_RQ_st(Mlsas_pr_req_t *prr, uint32_t c, uint32_t s);
static int __Mlsas_PR_RQ_Put_Completion_ref(Mlsas_pr_req_t *prr, uint32_t c_put);
static void __Mlsas_PR_RQ_complete(Mlsas_pr_req_t *prr);

static uint32_t Mlsas_npending = 0;
static uint32_t Mlsas_minors = 16; 
static Mlsas_t Mlsas;
static Mlsas_t *gMlsas_ptr = &Mlsas;

static const char *Mlsas_devst_name[Mlsas_Devst_Last] = {
	[Mlsas_Devst_Standalone] 	= 	"Standalone",
	[Mlsas_Devst_Failed] 		= 	"Failed",
	[Mlsas_Devst_Degraded] 		= 	"Degraded",
	[Mlsas_Devst_Attached]		= 	"Attached",
	[Mlsas_Devst_Healthy] 		= 	"Healthy",
};

static const char *Mlsas_devevt_name[Mlsas_Devevt_Last] = {
	[Mlsas_Devevt_To_Attach] 		= 	"To_Attach_Phys",
	[Mlsas_Devevt_Attach_Error]		= 	"Attach_Error",
	[Mlsas_Devevt_Attach_OK]		=	"Attach_OK",
	[Mlsas_Devevt_Attach_Local]		=	"Local_Attach",
	[Mlsas_Devevt_Attach_PR]		=	"Peer_Attach",
	[Mlsas_Devevt_Error_Switch]		=	"Error_Switch",
	[Mlsas_Devevt_PR_Error_Sw]		=	"Peer_Error_Switch"
};

static Mlsas_RX_pfn_t Mlsas_rx_hdl[Mlsas_Mms_Last] = {
	[Mlsas_Mms_None] 		=	NULL,
	[Mlsas_Mms_Attach]		=	__Mlsas_RX_Attach,
	[Mlsas_Mms_Bio_RW]		= 	__Mlsas_RX_Bio_RW,
	[Mlsas_Mms_Brw_Rsp]		=	__Mlsas_RX_Brw_Rsp,
};

static int Mlsas_Disk_Open(struct block_device *, fmode_t);
static void Mlsas_Disk_Release(struct gendisk *, fmode_t);

static struct block_device_operations Mlsas_disk_ops = {
	.owner =   THIS_MODULE,
	.open =    Mlsas_Disk_Open,
	.release = Mlsas_Disk_Release,
};

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

static inline void __Mlsas_Make_Backing_Bio(Mlsas_request_t *rq, struct bio *bio_src)
{
	struct bio *backing = bio_clone(bio_src, GFP_NOIO);
	
	rq->Mlrq_back_bio = backing;
	backing->bi_next = NULL;
	backing->bi_end_io = __Mlsas_Request_endio;
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
		VERIFY(NULL != iocp->Mlioc_ibufptr);
		
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
		VERIFY(NULL != iocp->Mlioc_obufptr);
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
		VERIFY(NULL != iocp->Mlioc_obufptr);
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
		VERIFY(!ioc.Mlioc_nibuf && !ioc.Mlioc_nobuf);
		rval = Mlsas_Do_EnableSvc();
		break;
	case Mlsas_Ioc_Newminor:
		rval = Mlsas_Do_NewMinor(&ioc);
		break;
	case Mlsas_Ioc_Attach:
		rval = Mlsas_Do_Attach(&ioc);
		break;
	case Mlsas_Ioc_Failoc:
		rval = Mlsas_Do_Failoc(&ioc);
		break;
	case Mlsas_Ioc_Virtinfo:
		rval = Mlsas_Do_Virtinfo(&ioc);
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
	gMlsas_ptr->Ml_rhs = mod_hash_create_idhash("Mlsas_rhs", 64, NULL);
	
	gMlsas_ptr->Ml_skc = kmem_cache_create("Mlsas_req_skc",
		sizeof(Mlsas_request_t), 0, 
		SLAB_RECLAIM_ACCOUNT | SLAB_PANIC, NULL);
	gMlsas_ptr->Ml_request_mempool = mempool_create_slab_pool(
		256, gMlsas_ptr->Ml_skc);
	
	gMlsas_ptr->Ml_prr_skc = kmem_cache_create("Mlsas_prr_skc",
		sizeof(Mlsas_pr_req_t), 0, 
		SLAB_RECLAIM_ACCOUNT | SLAB_PANIC, NULL);
	gMlsas_ptr->Ml_prr_mempool = mempool_create_slab_pool(
		256, gMlsas_ptr->Ml_prr_skc);
	
	gMlsas_ptr->Ml_minor = 0;
	gMlsas_ptr->Ml_async_tq = taskq_create("Mlsas_async_tq", 4, 
		minclsyspri, 4, 8, TASKQ_PREPOPULATE);

	spin_lock_init(&retry->Mlt_lock);
	list_create(&retry->Mlt_writes, sizeof(Mlsas_Delayed_obj_t),
		offsetof(Mlsas_Delayed_obj_t, dlo_node));
	retry->Mlt_workq = create_singlethread_workqueue("Mlsas_retry");
	INIT_WORK(&retry->Mlt_work, __Mlsas_Retry);

	(void) csh_rx_hook_add(CLUSTER_SAN_MSGTYPE_MLTSAS, Mlsas_RX, NULL);
	(void) csh_link_evt_hook_add(__Mlsas_Conn_Evt_fn, NULL);
		
	gMlsas_ptr->Ml_state = Mlsas_St_Enabled;
	
	cmn_err(CE_NOTE, "Mltsas Enable Complete.");
	return 0;
}

static int Mlsas_Do_NewMinor(Mlsas_iocdt_t *Mlip)
{
	int rval = 0;
	nvlist_t *invlp = NULL;
	char *path = NULL;
	Mlsas_blkdev_t *vt = NULL;
	uint64_t hash_key = 0;
	struct block_device *bdev = NULL;
	
	if (((rval = nvlist_unpack(Mlip->Mlioc_ibufptr, 
			Mlip->Mlioc_nibuf, &invlp, KM_SLEEP)) != 0) || 
		((rval = nvlist_lookup_string(invlp, 
			"path", &path)) != 0) ||
		((rval = Mlsas_Path_to_Hashkey(path, 
			&hash_key)) != 0)) {
		cmn_err(CE_NOTE, "%s New Minor FAIL By Unpack(%p) or"
			" None Nvl Path(%p) Nvpair or Invalid Path(%s),Error(%d)", 
			__func__, invlp, path, path, rval);
		goto failed_out;
	}

	mutex_enter(&gMlsas_ptr->Ml_mtx);
	if (((rval = mod_hash_find(gMlsas_ptr->Ml_devices, 
			hash_key, &vt)) == 0))
		rval = __Mlsas_Resume_failoc_virt(vt);
	else {
		rval = __Mlsas_New_minor_impl(path, hash_key, &vt);
		if (vt && !rval)
			VERIFY(mod_hash_insert(gMlsas_ptr->Ml_devices, 
				hash_key, vt) == 0);
	}
	mutex_exit(&gMlsas_ptr->Ml_mtx);

failed_out:	
	if (invlp)
		nvlist_free(invlp);
	return rval;
}

static int Mlsas_Do_Attach(Mlsas_iocdt_t *Mlip)
{
	int rval = -EINVAL;
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

static int Mlsas_Do_Failoc(Mlsas_iocdt_t *Mlip)
{
	int rval = 0;
	nvlist_t *invlp = NULL;
	char *path = NULL;
	Mlsas_blkdev_t *Mlb = NULL;
	uint64_t hash_key = 0;
	
	if (((rval = nvlist_unpack(Mlip->Mlioc_ibufptr, 
			Mlip->Mlioc_nibuf, &invlp, KM_SLEEP)) != 0) || 
		((rval = nvlist_lookup_string(invlp, 
			"path", &path)) != 0) ||
		((rval = Mlsas_Path_to_Hashkey(path, 
			&hash_key)) != 0)) {
		cmn_err(CE_NOTE, "%s Failoc FAIL By Unpack(%p) or"
			" None Nvl Path(%p) Nvpair or Invalid Path(%s),Error(%d)", 
			__func__, invlp, path, path, rval);
		goto failed_out;
	}

	mutex_enter(&gMlsas_ptr->Ml_mtx);
	VERIFY(mod_hash_find(gMlsas_ptr->Ml_devices, hash_key, &Mlb) == 0);
	kref_get(&Mlb->Mlb_ref);
	mutex_exit(&gMlsas_ptr->Ml_mtx);

	__Mlsas_Do_Failoc_impl(Mlb);
	kref_put(&Mlb->Mlb_ref, __Mlsas_Rele_Virt);
failed_out:
	if (invlp)
		nvlist_free(invlp);
	return rval;
}

static int Mlsas_Do_Virtinfo(Mlsas_iocdt_t *dt)
{
	int rval = 0;
	nvlist_t *invlp = NULL;
	uint64_t hash_key = 0;
	const char *path = NULL;
	Mlsas_blkdev_t *Mlb = NULL;
	Mlsas_virtinfo_return_t vti;

	if (((rval = nvlist_unpack(dt->Mlioc_ibufptr, dt->Mlioc_nibuf,
			&invlp, KM_SLEEP)) != 0) ||
		((rval = nvlist_lookup_string(invlp, 
			"path", &path)) != 0) ||
		((rval = Mlsas_Path_to_Hashkey(path, 
			&hash_key)) != 0)) {
		cmn_err(CE_NOTE, "%s Virtinfo FAIL by Unpack(%p) or"
			" None Nvl Path(%p) Nvpair or Invalid Path(%s),Error(%d)",
			__func__, invlp, path, path, rval);
		goto failed_out;
	}

	mutex_enter(&gMlsas_ptr->Ml_mtx);
	VERIFY(mod_hash_find(gMlsas_ptr->Ml_devices, hash_key, &Mlb) == 0);
	kref_get(&Mlb->Mlb_ref);
	mutex_exit(&gMlsas_ptr->Ml_mtx);

	bzero(&vti, sizeof(Mlsas_virtinfo_return_t));
	__Mlsas_Do_Virtinfo_impl(Mlb, &vti);
	kref_put(&Mlb->Mlb_ref, __Mlsas_Rele_Virt);

	bcopy(&vti, dt->Mlioc_obufptr, sizeof(Mlsas_virtinfo_return_t));
	dt->Mlioc_nofill = sizeof(Mlsas_virtinfo_return_t);
failed_out:
	if (invlp)
		nvlist_free(invlp);
	return rval;
}

typedef struct Mlsas_getblock_arg {
	char path[64];
	struct block_device *backing;
	struct completion notify;
	int error;
	uint32_t waiting;
} Mlsas_getblock_arg_t;

static void __Mlsas_Get_backing_device(Mlsas_getblock_arg_t *getblk)
{
	struct block_device *bdev = NULL;
	
	if (IS_ERR_OR_NULL(bdev = blkdev_get_by_path(getblk->path, 
			FMODE_READ | FMODE_WRITE, getblk->path)))
		getblk->error = PTR_ERR(bdev);

	if (!getblk->error)
		getblk->backing = bdev;

	if (getblk->waiting)
		complete(&getblk->notify);
	else {
		if (!getblk->error)
			blkdev_put(bdev, FMODE_READ | FMODE_WRITE);
		kmem_free(getblk, sizeof(*getblk));
	}
}

static int __Mlsas_New_minor_impl(const char *path, uint64_t hashkey, 
		Mlsas_blkdev_t **vtptr)
{
	Mlsas_blkdev_t *vt = NULL;
	struct block_device *bdev = NULL;
	struct task_struct *ts = NULL;
	Mlsas_getblock_arg_t *arg = NULL;
	int rval = 0;

	arg = kmem_zalloc(sizeof(Mlsas_getblock_arg_t), KM_SLEEP);
	arg->backing = NULL;
	arg->error = 0;
	arg->waiting = 1;
	strncpy(arg->path, path, sizeof(arg->path));
	init_completion(&arg->notify);

	cmn_err(CE_NOTE, "Requesting to NEW MINOR(%s)", path);

	if (IS_ERR_OR_NULL(ts = kthread_run(__Mlsas_Get_backing_device, arg,
			"Mlsas_get_backing_%s", arg->path))) {
		cmn_err(CE_NOTE, "%s CREATE GET_BACKING(%s) thread FAIL,"
			" DIRECTLY!", __func__, arg->path);
		__Mlsas_Get_backing_device(arg);
	}

	if ((rval = wait_for_completion_timeout(&arg->notify, 
			MSEC_TO_TICK(500))) == 0) {
		cmn_err(CE_NOTE, "%s GET_BACKING(%s) TIMEOUT, "
			"maybe a noresp disk", __func__, arg->path);
		arg->waiting = 0;
		return -ETIMEDOUT;
	}

	if ((rval = arg->error) != 0) {
		cmn_err(CE_NOTE, "%s NEW MINOR(%s) FAIL by GET BACKING, "
			"ERROR(%d)", __func__, path, rval);
		kmem_free(arg, sizeof(Mlsas_getblock_arg_t));
		return rval;
	}

	bdev = arg->backing;
	kmem_free(arg, sizeof(Mlsas_getblock_arg_t));

	__Mlsas_New_Virt(hashkey, &vt);

	__Mlsas_Attach_Local_Phys(bdev, path, vt);

	*vtptr = vt;

	cmn_err(CE_NOTE, "%s New Minor Device(%s 0x%llx) Complete.",
		__func__, path, hashkey); 

	return (0);
}

static int __Mlsas_Resume_failoc_virt(Mlsas_blkdev_t *vt)
{
	int rval = 0;
	
	spin_lock_irq(&vt->Mlb_rq_spin);
	if (!__Mlsas_Get_ldev_if_state_between(vt, 
			Mlsas_Devst_Standalone,
			Mlsas_Devst_Degraded))
		rval = -EINVAL;
	if (rval == 0) {
		VERIFY(list_is_empty(&vt->Mlb_local_rqs));
		__Mlsas_Devst_Stmt(vt, Mlsas_Devevt_Attach_OK);
	}
	spin_lock_irq(&vt->Mlb_rq_spin);

	return rval;
}

static void __Mlsas_Do_Virtinfo_impl(Mlsas_blkdev_t *vt, 
		Mlsas_virtinfo_return_t *vti)
{
	spin_lock_irq(&vt->Mlb_rq_spin);
	vti->vti_devst = vt->Mlb_st;
	vti->vti_error_cnt = vt->Mlb_error_cnt;
	vti->vti_hashkey = vt->Mlb_hashkey;
	vti->vti_io_npending = vt->Mlb_npending;
	vti->vti_switch = vt->Mlb_switch;
	strncpy(vti->vti_scsi_protocol, vt->Mlb_scsi,
		sizeof(vti->vti_scsi_protocol));
	spin_unlock_irq(&vt->Mlb_rq_spin);
}

static void __Mlsas_Do_Failoc_impl(Mlsas_blkdev_t *Mlb)
{
	Mlsas_request_t *rq, *next;
	Mlsas_bio_and_error_t m;
	
	spin_lock_irq(&Mlb->Mlb_rq_spin);
	/* no more local request */
	__Mlsas_Devst_Stmt(Mlb, Mlsas_Devevt_Error_Switch);
	
	rq = list_head(&Mlb->Mlb_local_rqs);
	while (rq) {
		next = list_next(&Mlb->Mlb_local_rqs, rq);
		if (!(rq->Mlrq_flags & Mlsas_RQ_Local_Pending))
			continue;
		if (rq->Mlrq_bdev != Mlb)
			continue;
		__Mlsas_Req_Stmt(rq, Mlsas_Rst_Abort_Diskio, &m);
		if (m.Mlbi_bio)
			__Mlsas_Complete_Master_Bio(rq, &m);
		rq = next;
	}
	spin_unlock_irq(&Mlb->Mlb_rq_spin);
}

static void __Mlsas_Init_Virt_MLB(Mlsas_blkdev_t *Mlb)
{
	spin_lock_init(&Mlb->Mlb_rq_spin);
	kref_init(&Mlb->Mlb_ref);
	
	list_create(&Mlb->Mlb_local_rqs, sizeof(Mlsas_request_t),
		offsetof(Mlsas_request_t, Mlrq_node));
	list_create(&Mlb->Mlb_peer_rqs, sizeof(Mlsas_pr_req_t),
		offsetof(Mlsas_pr_req_t, prr_mlb_node));
	list_create(&Mlb->Mlb_topr_rqs, sizeof(Mlsas_request_t),
		offsetof(Mlsas_request_t, Mlrq_node));
	list_create(&Mlb->Mlb_pr_devices, sizeof(Mlsas_pr_device_t),
		offsetof(Mlsas_pr_device_t, Mlpd_node));

	Mlb->Mlb_txbuf 		= kmem_zalloc(Mlb->Mlb_txbuf_len, 	KM_SLEEP);
	Mlb->Mlb_astx_buf 	= kmem_zalloc(Mlb->Mlb_astxbuf_len, KM_SLEEP);
	Mlb->Mlb_stx_buf 	= kmem_zalloc(Mlb->Mlb_astxbuf_len, KM_SLEEP);

	__Mlsas_Thread_Init(&Mlb->Mlb_tx, 		__Mlsas_RTx, Mtt_Tx, 		"__Mlsas_Tx");
	__Mlsas_Thread_Init(&Mlb->Mlb_rx, 		__Mlsas_RTx, Mtt_Rx, 		"__Mlsas_Rx");
	__Mlsas_Thread_Init(&Mlb->Mlb_asender, 	__Mlsas_RTx, Mtt_Asender, 	"__Mlsas_Asender");
	__Mlsas_Thread_Init(&Mlb->Mlb_sender, 	__Mlsas_RTx, Mtt_Sender, 	"__Mlsas_Sender");

	__Mlsas_RTx_Init_WQ(&Mlb->Mlb_tx_wq);
	__Mlsas_RTx_Init_WQ(&Mlb->Mlb_rx_wq);
	__Mlsas_RTx_Init_WQ(&Mlb->Mlb_asender_wq);
	__Mlsas_RTx_Init_WQ(&Mlb->Mlb_sender_wq);
}

static void __Mlsas_New_Virt(uint64_t hash_key, Mlsas_blkdev_t **Mlbpp)
{
	Mlsas_blkdev_t *Mlbp = NULL;
	
	VERIFY((Mlbp = kzalloc(sizeof(Mlsas_blkdev_t), 
			GFP_KERNEL)) != NULL);
	Mlbp->Mlb_st		 	= Mlsas_Devst_Standalone;
	Mlbp->Mlb_hashkey 		= hash_key;
	Mlbp->Mlb_astxbuf_len 	= 32 << 10;
	Mlbp->Mlb_txbuf_len 	= 32 << 10;
	Mlbp->Mlb_stxbuf_len 	= 32 << 10;
	Mlbp->Mlb_switch 		= 16;
	
	__Mlsas_Init_Virt_MLB(Mlbp);

	__Mlsas_Alloc_Virt_disk(Mlbp);
	
	*Mlbpp = Mlbp;
}

static void __Mlsas_Rele_Virt(struct kref *ref)
{
	
}

static void __Mlsas_Alloc_Virt_disk(Mlsas_blkdev_t *Mlbp)
{
	int rval = -ENOMEM;
	struct gendisk *disk = NULL;
	struct request_queue *rq = NULL;
	uint32_t minor = atomic_inc_32_nv(&gMlsas_ptr->Ml_minor);
	
	VERIFY(((disk = alloc_disk(1)) != NULL) &&
		((rq = blk_alloc_queue(GFP_KERNEL)) != NULL));

	set_disk_ro(disk, true);
	disk->queue = rq;
	disk->minors = Mlsas_minors;
	disk->major = Mlsas_MAJOR;
	disk->first_minor = minor;
	disk->fops = &Mlsas_disk_ops;
	sprintf(disk->disk_name, "Mlsas%llx", Mlbp->Mlb_hashkey);
	disk->private_data = Mlbp;

	blk_queue_make_request(rq, __Mlsas_Make_Request_fn);
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
	
	Mlbp->Mlb_this = bdget(MKDEV(Mlsas_MAJOR, minor));
	Mlbp->Mlb_this->bd_contains = Mlbp->Mlb_this;
	Mlbp->Mlb_rq = rq;
	Mlbp->Mlb_gdisk = disk;
	
	add_disk(disk);
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

/*
 * attach local total disk
 */
static void __Mlsas_Attach_Local_Phys(struct block_device *phys, 
		const char *path, Mlsas_blkdev_t *Mlb)
{
	Mlsas_Msh_t *mms = NULL;
	Mlsas_Attach_msg_t *Atm = NULL;
	Mlsas_devst_e st;
	
	kref_get(&Mlb->Mlb_ref);

	__Mlsas_Attach_BDI(&Mlb->Mlb_bdi, path, phys);
	__Mlsas_Attach_Virt_Queue(Mlb);

	__Mlsas_Thread_Start(&Mlb->Mlb_tx);
	__Mlsas_Thread_Start(&Mlb->Mlb_rx);
	__Mlsas_Thread_Start(&Mlb->Mlb_asender);
	__Mlsas_Thread_Start(&Mlb->Mlb_sender);

	spin_lock_irq(&Mlb->Mlb_rq_spin);
	__Mlsas_Devst_Stmt(Mlb, Mlsas_Devevt_Attach_OK);
	st = Mlb->Mlb_st;
	spin_unlock_irq(&Mlb->Mlb_rq_spin);

	/*
	 * start aync page read
	 */
	__Mlsas_Attach_Phys_size(Mlb);
	set_disk_ro(Mlb->Mlb_gdisk, 0);

	mms = __Mlsas_Alloc_Mms(sizeof(Mlsas_Attach_msg_t),
		Mlsas_Mms_Attach, Mlb->Mlb_hashkey, &Atm);
	Atm->Atm_st = st;
	Atm->Atm_rsp = 0;
	(void)Mlsas_TX(Mlsas_Noma_Session, mms, 
		mms->Mms_len, NULL, 0, B_FALSE);

	kref_put(&Mlb->Mlb_ref, __Mlsas_Rele_Virt);
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

	blk_queue_stack_limits(virt_rq, phys_rq);

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
	if (__Mlsas_Get_ldev_if_state(Mlb, Mlsas_Devst_Attached)) {
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
		kref_put(&Mlb->Mlb_ref, __Mlsas_Rele_Virt);
		return rval;
	}

	return rval;
}
EXPORT_SYMBOL(Mlsas_Attach_Phys);

static void Mlsas_RX(cs_rx_data_t *xd, void *priv)
{
	Mlsas_Msh_t *mms = NULL;
	Mlsas_RX_pfn_t Rx_fn = NULL;
	
	if (!xd->ex_head || (xd->ex_len < sizeof(*mms))) {
		cmn_err(CE_NOTE, "%s RX Less Header, Header(%p %llx)", 
			__func__, xd->ex_head, xd->ex_len);
		goto free_xd;
	}

	mms = (Mlsas_Msh_t *)xd->ex_head;
	if ((mms->Mms_ck != Mlsas_Mms_Magic) ||
		(mms->Mms_type <= Mlsas_Mms_None) ||
		(mms->Mms_type >= Mlsas_Mms_Last)) {
		cmn_err(CE_NOTE, "%s RX Wrong Mms Magic(%04x %04x)"
			" or Wrong Mms Type(%02x)", __func__, 
			mms->Mms_ck, Mlsas_Mms_Magic, mms->Mms_type);
		goto free_xd;
	}

	Rx_fn = Mlsas_rx_hdl[mms->Mms_type];
	Rx_fn(mms, xd);
	return ;
	
free_xd:
	csh_rx_data_free_ext(xd);
}

int Mlsas_TX(void *session, void *header, uint32_t hdlen,
		void *dt, uint32_t dtlen, boolean_t io)
{
	int rval = 0;
	uint32_t msg_type = CLUSTER_SAN_MSGTYPE_MLTSAS;
	
	if (!session)
		session = Mlsas_Noma_Session;
	
	if (session == Mlsas_Noma_Session)
		cluster_san_broadcast_send(dt, dtlen, 
			header, hdlen, msg_type, 0);
	else if (likely(io))
		rval = cluster_san_host_send_bio(session, 
			dt, dtlen, header, hdlen, 
			msg_type,  0, B_TRUE, 3);
	else 
		rval = cluster_san_host_send(session, 
			dt, dtlen, header, hdlen, 
			msg_type, 0, B_TRUE, 3);

	return rval;
}

static void __Mlsas_Conn_Evt_fn(cluster_san_hostinfo_t *cshi, 
		cts_link_evt_t link_evt, void *arg)
{
	switch (link_evt) {
	case LINK_EVT_UP_TO_DOWN:
		break;
	case LINK_EVT_DOWN_TO_UP:
		break;
	default:
		VERIFY(0);
	}
}

static blk_qc_t __Mlsas_Make_Request_fn(struct request_queue *rq, struct bio *bio)
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
	
	__Mlsas_Mkrequest_Prepare(Mlb, bio, start_jif, &req);
	if (IS_ERR_OR_NULL(req))
		return ;

	__Mlsas_Submit_or_Send(req);
}

static void __Mlsas_Submit_or_Send(Mlsas_request_t *req)
{
	Mlsas_blkdev_t *Mlb = req->Mlrq_bdev;
	boolean_t do_local = B_FALSE, do_remote = B_FALSE;
	boolean_t submit_backing = B_FALSE;
	uint32_t what;

	__Mlsas_Partion_Map_toTtl(req);
	
	spin_lock_irq(&Mlb->Mlb_rq_spin);
	__Mlsas_Do_Policy(Mlb, req, &do_local, &do_remote);

	switch ((do_local << 1) | do_remote) {
	case 0x0:
		break;
	case 0x1:
		__Mlsas_Submit_Net_Prepare(req);
		what = bio_data_dir(req->Mlrq_master_bio) == WRITE ?
			Mlsas_Rst_Queue_Net_Write : Mlsas_Rst_Queue_Net_Read;
		__Mlsas_Req_Stmt(req, Mlsas_Rst_Submit_Net, NULL);
		__Mlsas_Req_Stmt(req, what, NULL);
		break;
	case 0x2:
		__Mlsas_Submit_Local_Prepare(req);
		__Mlsas_Req_Stmt(req, Mlsas_Rst_Submit_Local, NULL);
		submit_backing = B_TRUE;
		break;
	default:
		VERIFY(0);
	}

	spin_unlock_irq(&Mlb->Mlb_rq_spin);

	if (submit_backing)
		__Mlsas_Submit_Backing_Bio(req);
}

static void __Mlsas_Do_Policy(Mlsas_blkdev_t *Mlb, 
		Mlsas_request_t *rq, boolean_t *do_local, 
		boolean_t *do_remote)
{
	Mlsas_pr_device_t *pr;
	
	if (__Mlsas_Get_ldev_if_state(Mlb, Mlsas_Devst_Attached))
		*do_local = 1;

	if (*do_local)
		return ;

	pr = list_head(&Mlb->Mlb_pr_devices);
	while (pr != NULL) {
		if (__Mlsas_Get_PR_if_state(pr, Mlsas_Devst_Attached))
			break;
		pr = list_next(&Mlb->Mlb_pr_devices, pr);
	}

	if (NULL == pr)
		return ;
	
	rq->Mlrq_pr = pr;
	*do_remote = B_TRUE;
}

static void __Mlsas_Submit_Backing_Bio(Mlsas_request_t *rq)
{
	Mlsas_blkdev_t *Mlb = rq->Mlrq_bdev;
	struct bio *bio = rq->Mlrq_back_bio;
	
	VERIFY(rq->Mlrq_back_bio != NULL);

	if (!__Mlsas_Get_ldev(Mlb)) 
		bio_io_error(bio);
	else {
		rq->Mlrq_submit_jif = jiffies;
		generic_make_request(bio);
	}
}

static void __Mlsas_Submit_Net_Bio(Mlsas_request_t *rq,
		int (*cb)(Mlsas_rtx_wk_t *))
{
	Mlsas_blkdev_t *Mlb = rq->Mlrq_bdev;
	Mlsas_pr_device_t *pr = rq->Mlrq_pr;
	
	VERIFY(__Mlsas_Get_PR_if_state(pr, Mlsas_Devst_Attached));

	rq->Mlrq_wk.rtw_fn = cb;
	__Mlsas_Queue_RTX(&Mlb->Mlb_tx_wq, &rq->Mlrq_wk);
}

static void __Mlsas_Partion_Map_toTtl(Mlsas_request_t *rq)
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
		}
	}
}	

static void __Mlsas_Submit_Local_Prepare(Mlsas_request_t *rq)
{
	Mlsas_blkdev_t *Mlb = rq->Mlrq_bdev;
	
	kref_get(&rq->Mlrq_ref);
	list_insert_tail(&Mlb->Mlb_local_rqs, rq);
}

static void __Mlsas_Submit_Net_Prepare(Mlsas_request_t *rq)
{
	Mlsas_blkdev_t *Mlb = rq->Mlrq_bdev;
	
	kref_get(&rq->Mlrq_ref);
	list_insert_tail(&Mlb->Mlb_topr_rqs, rq);
	list_insert_tail(&rq->Mlrq_pr->Mlpd_rqs, rq);
	
	if (rq->Mlrq_back_bio) {
		bio_put(rq->Mlrq_back_bio);
		rq->Mlrq_back_bio = NULL;
	}
}

static void __Mlsas_Mkrequest_Prepare(Mlsas_blkdev_t *Mlb, 
		struct bio *bio, uint64_t start_jif, 
		Mlsas_request_t **rqpp)
{
	int rw = bio_data_dir(bio);
 	Mlsas_request_t *Mlr = NULL;
	int bi_error = -ENOMEM;

	if (!(Mlr = __Mlsas_New_Request(Mlb, 
			bio, start_jif)))
		goto failed;

	bi_error = -ENODEV;
	if (!__Mlsas_Get_ldev(Mlb)) 
		goto free_rq;

	__Mlsas_Start_Ioacct(Mlb, Mlr);

	*rqpp = Mlr;
	return ;
	
free_rq:
	Mlsas_RQ_put(Mlr, 1);
failed:
	atomic_dec_32(&Mlsas_npending);
	atomic_dec_32(&Mlb->Mlb_npending);
	bio->bi_error = bi_error;
	bio_endio(bio);
}

static Mlsas_request_t *__Mlsas_New_Request(Mlsas_blkdev_t *Mlb, 
		struct bio *bio, uint64_t start_jif)
{
	Mlsas_request_t *rq = NULL;
	int rw = bio_data_dir(bio);
	
	if ((rq = mempool_alloc(gMlsas_ptr->Ml_request_mempool,
			GFP_NOIO | __GFP_ZERO)) == NULL)
		return NULL;

	kref_init(&rq->Mlrq_ref);
	rq->Mlrq_master_bio = bio;
	rq->Mlrq_flags = (rw == WRITE ? Mlsas_RQ_Write : 0);
	kref_get(&Mlb->Mlb_ref);
	rq->Mlrq_bdev = Mlb;
	rq->Mlrq_start_jif = start_jif;
	rq->Mlrq_sector = bio->bi_iter.bi_sector;
	rq->Mlrq_bsize = bio->bi_iter.bi_size;
	
	__Mlsas_Make_Backing_Bio(rq, bio);

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
			__Mlsas_Rele_Virt);
	rq->Mlrq_back_bio = NULL;
	rq->Mlrq_bdev = NULL;
	mempool_free(rq, gMlsas_ptr->Ml_request_mempool);
}

static void Mlsas_RQ_put(Mlsas_request_t *rq, uint32_t k_put)
{
	kref_sub(&rq->Mlrq_ref, k_put, __Mlsas_Release_RQ);
}

static void __Mlsas_Request_endio(struct bio *bio)
{
	uint32_t what;
	unsigned long flags;
	Mlsas_request_t *rq = bio->bi_private;
	Mlsas_blkdev_t *Mlb = rq->Mlrq_bdev;
	Mlsas_bio_and_error_t Mlbi;

	if (unlikely(rq->Mlrq_flags & Mlsas_RQ_Local_Aborted))
		bio->bi_error = bio->bi_error ?: -EIO;
	
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
	Mlsas_devst_e next = Mlsas_Devst_Last;
	
	switch (Mlb->Mlb_st) {
	case Mlsas_Devst_Standalone:
		if (what == Mlsas_Devevt_Attach_OK)
			next = Mlsas_Devst_Attached;
		else if (what == Mlsas_Devevt_Attach_PR)
			next = Mlsas_Devst_Degraded;
		break;
	case Mlsas_Devst_Degraded:
		if (what == Mlsas_Devevt_Attach_OK)
			next = Mlsas_Devst_Healthy;
		if (what == Mlsas_Devevt_PR_Error_Sw)
			next = __Mlsas_Has_Active_PR(Mlb, NULL) ?
				Mlsas_Devst_Last : Mlsas_Devst_Failed;
		break;
	case Mlsas_Devst_Attached:
		if (what == Mlsas_Devevt_Attach_PR)
			next = Mlsas_Devst_Healthy;
		if (what == Mlsas_Devevt_Error_Switch)
			next = Mlsas_Devst_Failed;
		break;
	case Mlsas_Devst_Healthy:
		if (what == Mlsas_Devevt_Error_Switch)
			next = __Mlsas_Has_Active_PR(Mlb, NULL) ?
				Mlsas_Devst_Degraded : Mlsas_Devst_Failed;
		if (what == Mlsas_Devevt_PR_Error_Sw)
			next = __Mlsas_Has_Active_PR(Mlb, NULL) ?
				Mlsas_Devst_Healthy : Mlsas_Devst_Attached;
		break;
	}

	if (next < Mlsas_Devst_Last)
		__Mlsas_Devst_St(Mlb, next, what);
}

static void __Mlsas_Devst_St(Mlsas_blkdev_t *Mlb, 
		Mlsas_devst_e newst, uint32_t what)
{
	cmn_err(CE_NOTE, "Mlsas Device(%s) Devst(%s --> %s), event(%s)",
		Mlb->Mlb_bdi.Mlbd_path, Mlsas_devst_name[Mlb->Mlb_st],
		Mlsas_devst_name[newst], Mlsas_devevt_name[what]);

	Mlb->Mlb_st = newst;
}

void __Mlsas_Req_Stmt(Mlsas_request_t *rq, uint32_t what,
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
	case Mlsas_Rst_Submit_Net:
		__Mlsas_Req_St(rq, Mlbi, 0, Mlsas_RQ_Net_Pending);
		break;
	case Mlsas_Rst_Queue_Net_Write:
		__Mlsas_Req_St(rq, Mlbi, 0, Mlsas_RQ_Net_Queued);
		__Mlsas_Submit_Net_Bio(rq, __Mlsas_Tx_biow);
		break;
	case Mlsas_Rst_Queue_Net_Read:
		__Mlsas_Req_St(rq, Mlbi, 0, Mlsas_RQ_Net_Queued);
		__Mlsas_Submit_Net_Bio(rq, __Mlsas_Tx_bior);
		break;
	case Mlsas_Rst_Net_Send_Error:
		__Mlsas_Req_St(rq, Mlbi, Mlsas_RQ_Net_Queued | 
			Mlsas_RQ_Net_Pending, 0);
		break;
	case Mlsas_Rst_Net_Send_OK:
		__Mlsas_Req_St(rq, Mlbi, Mlsas_RQ_Net_Queued, 
			Mlsas_RQ_Net_Sent);
		break;
	case Mlsas_Rst_PR_Write_Error:
	case Mlsas_Rst_PR_Read_Error:
	case Mlsas_Rst_PR_ReadA_Error:
		__Mlsas_Req_St(rq, Mlbi, Mlsas_RQ_Net_Pending, 
			Mlsas_RQ_Net_Done);
		break;
	case Mlsas_Rst_PR_Complete_OK:
		__Mlsas_Req_St(rq, Mlbi, Mlsas_RQ_Net_Pending, 
			Mlsas_RQ_Net_Done | Mlsas_RQ_Net_OK);
		break;
	case Mlsas_Rst_Abort_Diskio:
		__Mlsas_Req_St(rq, Mlbi, 0, Mlsas_RQ_Local_Aborted);
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

	if (!(oflg & Mlsas_RQ_Local_Pending) && (set & Mlsas_RQ_Local_Pending))
		atomic_inc_32(&rq->Mlrq_completion_ref);

	if (!(oflg & Mlsas_RQ_Net_Pending) && (set & Mlsas_RQ_Net_Pending))
		atomic_inc_32(&rq->Mlrq_completion_ref);

	if (!(oflg & Mlsas_RQ_Net_Queued) && (set & Mlsas_RQ_Net_Queued))
		atomic_inc_32(&rq->Mlrq_completion_ref);

	if (!(oflg & Mlsas_RQ_Local_Aborted) && (set & Mlsas_RQ_Local_Aborted)) {
		c_put++;
		kref_get(&rq->Mlrq_ref);
	}

	if ((oflg & Mlsas_RQ_Local_Pending) &&
		(clear & Mlsas_RQ_Local_Pending)) {
		if (!(rq->Mlrq_flags & Mlsas_RQ_Local_Aborted))
			c_put++;
		else 
			k_put++;
		k_put++;
		list_remove(&Mlb->Mlb_local_rqs, rq);
	}

	if ((oflg & Mlsas_RQ_Net_Pending) &&
		(clear & Mlsas_RQ_Net_Pending)) {
		c_put++;
		k_put++;
		list_remove(&Mlb->Mlb_topr_rqs, rq);
		list_remove(&rq->Mlrq_pr->Mlpd_rqs, rq);
	}

	if ((oflg & Mlsas_RQ_Net_Queued) &&
		(clear & Mlsas_RQ_Net_Queued))
		c_put++;

	if (c_put)
		k_put += __Mlsas_Put_RQcomplete_Ref(rq, c_put, Mlbi);
	if (Mlbi && Mlbi->Mlbi_bio)
		Mlbi->k_put = k_put;
	else if (k_put)
		kref_sub(&rq->Mlrq_ref, k_put, __Mlsas_Release_RQ);
}

static uint32_t __Mlsas_Put_RQcomplete_Ref(Mlsas_request_t *rq,
		uint32_t c_put, Mlsas_bio_and_error_t *Mlbi)
{
	VERIFY(Mlbi || rq->Mlrq_flags & Mlsas_RQ_Delayed);

	if (atomic_sub_32_nv(&rq->Mlrq_completion_ref, c_put))
		return 0;

	__Mlsas_Complete_RQ(rq, Mlbi);

	if (rq->Mlrq_flags & Mlsas_RQ_Delayed) {
		__Mlsas_Restart_DelayedRQ(rq);
		return 0;
	}

	return 1;
}

static void __Mlsas_Restart_DelayedRQ(Mlsas_Delayed_obj_t *obj)
{
	unsigned long flags;
	Mlsas_retry_t *retry = &gMlsas_ptr->Ml_retry;
	
	spin_lock_irqsave(&retry->Mlt_lock, flags);
	list_insert_tail(&retry->Mlt_writes, obj);
	spin_unlock_irqrestore(&retry->Mlt_lock, flags);

	queue_work(retry->Mlt_workq, &retry->Mlt_work);
}

static void __Mlsas_Complete_RQ(Mlsas_request_t *rq, 
		Mlsas_bio_and_error_t *Mlbi)
{
	uint32_t flags = rq->Mlrq_flags;
	Mlsas_blkdev_t *Mlb = rq->Mlrq_bdev;
	Mlsas_pr_device_t *pr = rq->Mlrq_pr;
	int ok, error;
	
	VERIFY(rq->Mlrq_master_bio != NULL);
	
	if (((flags & Mlsas_RQ_Local_Pending) && 
		!(flags & Mlsas_RQ_Local_Aborted))) {
		cmn_err(CE_PANIC, "%s Mlsas_RQ_Local_Pending && "
			"!Mlsas_RQ_Local_Aborted", __func__);
	}

	__Mlsas_End_Ioacct(Mlb, rq);

	ok = flags & (Mlsas_RQ_Local_OK |
		Mlsas_RQ_Net_OK);
	if (!(flags & Mlsas_RQ_Local_Aborted)) {
		error = PTR_ERR(rq->Mlrq_back_bio);
		rq->Mlrq_back_bio = NULL;
	}

	if (!ok) {
		/*
		 * Maybe PR report degraded or FAIL state at soon, but
		 * avoid to choose the same PR for restarting RQ,
		 * set to Mlsas_Devst_Degraded now.
		 */
		if (pr && (++pr->Mlpd_error_now >= pr->Mlpd_switch)) {
			__Mlsas_Update_PR_st(pr, Mlsas_Devst_Degraded);
		} else if (!(rq->Mlrq_flags & Mlsas_RQ_Local_Aborted)){
			if (++Mlb->Mlb_error_cnt >= Mlb->Mlb_switch)
				__Mlsas_Devst_Stmt(Mlb, Mlsas_Devevt_Error_Switch);
		}

		if (__Mlsas_Get_ldev(Mlb))
			rq->Mlrq_flags |= Mlsas_RQ_Delayed; 
		
	}
	if (!(rq->Mlrq_flags & Mlsas_RQ_Delayed)) {
		Mlbi->Mlbi_bio = rq->Mlrq_master_bio;
		Mlbi->Mlbi_error = ok ? 0 :
			(error ?: -EIO);
		rq->Mlrq_master_bio = NULL;
	}
}

void __Mlsas_Complete_Master_Bio(Mlsas_request_t *rq, 
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

static void __Mlsas_Retry(struct work_struct *w)
{
	list_t writes;
	Mlsas_Delayed_obj_t *obj;
	Mlsas_request_t *rq;
	Mlsas_pr_req_t *prr;
	Mlsas_retry_t *retry = 
		container_of(w, Mlsas_retry_t, Mlt_work);
	
	list_create(&writes, sizeof(Mlsas_Delayed_obj_t),
		offsetof(Mlsas_Delayed_obj_t, dlo_node));

	spin_lock_irq(&retry->Mlt_lock);
	list_move_tail(&writes, &retry->Mlt_writes);
	spin_unlock_irq(&retry->Mlt_lock);

	while ((obj = list_remove_head(&writes)) != NULL) {
		switch (obj->dlo_magic) {
		case Mlsas_Delayed_RQ: 
			rq = (Mlsas_request_t *)obj;
			atomic_inc_32(&rq->Mlrq_bdev->Mlb_npending);

			__Mlsas_Make_Request_impl(rq->Mlrq_bdev, 
				rq->Mlrq_master_bio, 
				rq->Mlrq_start_jif);
			Mlsas_RQ_put(rq, 1);
			break;
		case Mlsas_Delayed_PR_RQ:
			prr = (Mlsas_pr_req_t *)obj;
			spin_lock_irq(&prr->prr_pr->Mlpd_mlb->Mlb_rq_spin);
			prr->prr_flags &= ~Mlsas_PRRfl_Delayed;
			if (prr->prr_flags & Mlsas_PRRfl_Write)
				prr->prr_flags &= ~Mlsas_PRRfl_Addl_Kmem;
			__Mlsas_PR_RQ_stmt(prr, prr->prr_flags & Mlsas_PRRfl_Write ?
				Mlsas_PRRst_Queue_Net_Wrsp : Mlsas_PRRst_Queue_Net_Rrsp);
			__Mlsas_Queue_RTX(&prr->prr_pr->Mlpd_mlb->Mlb_asender_wq,
				&prr->prr_wk);
			spin_unlock_irq(&prr->prr_pr->Mlpd_mlb->Mlb_rq_spin);
			break;
		}
	}
}

static void __Mlsas_RX_Attach(Mlsas_Msh_t *mms,
		cs_rx_data_t *xd)
{
	int rval = 0;
	Mlsas_Attach_msg_t *Atm = mms + 1;
	Mlsas_blkdev_t *Mlb = NULL;
	
	mutex_enter(&gMlsas_ptr->Ml_mtx);
	if (((rval = mod_hash_find(gMlsas_ptr->Ml_devices, 
			mms->Mms_hashkey, &Mlb)) != 0)) {
		mutex_exit(&gMlsas_ptr->Ml_mtx);
		cmn_err(CE_NOTE, "%s None Local Virt(0x%llx)",
			__func__, mms->Mms_hashkey);
		goto failed;
	}

	kref_get(&Mlb->Mlb_ref);
	mutex_exit(&gMlsas_ptr->Ml_mtx);

	Atm->Atm_ext = (uint64_t)Mlb;

	if (taskq_dispatch(gMlsas_ptr->Ml_async_tq, 
			__Mlsas_RX_Attach_impl,
			xd, TQ_NOSLEEP) == NULL)
		__Mlsas_RX_Attach_impl(xd);
	return ;

failed:
	csh_rx_data_free_ext(xd);
}

static void __Mlsas_RX_Attach_impl(cs_rx_data_t *xd)
{
	int rval = 0;
	Mlsas_Msh_t *mms = (Mlsas_Msh_t *)xd->ex_head;
	Mlsas_Attach_msg_t *Atm = mms + 1;
	Mlsas_blkdev_t *Mlb = Atm->Atm_ext;
	cluster_san_hostinfo_t *cshi = xd->cs_private;
	Mlsas_pr_device_t *pr = NULL, *exist = NULL;
	Mlsas_rh_t *rh = NULL;
	Mlsas_devst_e newst, devst;
	boolean_t new_rh = B_FALSE;

	mutex_enter(&gMlsas_ptr->Ml_mtx);
	if ((rval = mod_hash_find(gMlsas_ptr->Ml_rhs, 
			cshi->hostid, &rh)) != 0) {
		VERIFY(rh = __Mlsas_Alloc_RH(cshi->hostid, cshi));
		VERIFY(mod_hash_insert(gMlsas_ptr->Ml_rhs, 
			cshi->hostid, rh) == 0);
		new_rh = B_TRUE;
	}

	pr = __Mlsas_Alloc_PR(cshi->hostid, Atm->Atm_st, Mlb, rh);
	if (Atm->Atm_rsp)
		pr->Mlpd_pr = Atm->Atm_pr;
	kref_get(&rh->Mh_ref);
	
	mutex_enter(&rh->Mh_mtx);
	spin_lock(&Mlb->Mlb_rq_spin);
	if (!__Mlsas_Get_ldev_if_state(Mlb, Mlsas_Devst_Attached)) 
		goto failed;

	if ((exist = __Mlsas_Lookup_PR_locked(Mlb, 
			cshi->hostid)) != NULL) {
		newst = exist->Mlpd_st;
		devst = Mlb->Mlb_st;
		if (Atm->Atm_rsp)
			exist->Mlpd_pr = Atm->Atm_pr;
		__Mlsas_Update_PR_st(exist, Atm->Atm_st);
		goto failed;
	}

	list_insert_tail(&rh->Mh_devices, pr);
	list_insert_tail(&Mlb->Mlb_pr_devices, pr);
	if (__Mlsas_Get_PR_if_state(pr, Mlsas_Devst_Attached))
		__Mlsas_Devst_Stmt(Mlb, Mlsas_Devevt_Attach_PR);
	newst = Mlb->Mlb_st;
	spin_unlock(&Mlb->Mlb_rq_spin);
	mutex_exit(&rh->Mh_mtx);
	mutex_exit(&gMlsas_ptr->Ml_mtx);
	
	Atm->Atm_st = newst;
	Atm->Atm_rsp = 1;
	Atm->Atm_pr = pr;
	(void) Mlsas_TX(cshi, mms, mms->Mms_len, 
		NULL, 0, B_FALSE);

	cmn_err(CE_NOTE, "%s New PR(0x%llx St(%s)) of "
		"Mlsas_blkdev(St(%s))", __func__, 
		mms->Mms_hashkey, Mlsas_devst_name[Atm->Atm_st], 
		Mlsas_devst_name[newst]);
	csh_rx_data_free(xd, B_FALSE);
	return ;

failed:
	spin_unlock(&Mlb->Mlb_rq_spin);
	mutex_exit(&rh->Mh_mtx);
	mutex_exit(&gMlsas_ptr->Ml_mtx);
	if (exist)
		cmn_err(CE_NOTE, "%s Update exist PR(oSt(%s) --> nSt(%s)),"
			"Virt Device St(%s)", __func__, Mlsas_devst_name[newst], 
			Mlsas_devst_name[exist->Mlpd_st], Mlsas_devst_name[devst]);
	kfree(pr);
	if (new_rh)
		__Mlsas_Release_RH(&rh->Mh_ref);
	csh_rx_data_free_ext(xd);
}

static void __Mlsas_RX_Bio_RW(Mlsas_Msh_t *mms, 
		cs_rx_data_t *xd)
{
	Mlsas_RW_msg_t *RWm = mms + 1;
	Mlsas_pr_device_t *pr = RWm->rw_mlbpr;
	Mlsas_rtx_wk_t *w = (Mlsas_rtx_wk_t *)xd;

	if (RWm->rw_flags & REQ_WRITE)
		w->rtw_fn = __Mlsas_Rx_biow;
	else 
		w->rtw_fn = __Mlsas_Rx_bior;
	
	__Mlsas_Queue_RTX(&pr->Mlpd_mlb->Mlb_rx_wq, xd); 
}

static void __Mlsas_RX_Brw_Rsp(Mlsas_Msh_t *mms, 
		cs_rx_data_t *xd)
{
	Mlsas_RWrsp_msg_t *rsp = mms + 1;
	Mlsas_request_t *rq = rsp->rsp_rqid;
	Mlsas_rtx_wk_t *w = (Mlsas_rtx_wk_t *)xd;

	w->rtw_fn = __Mlsas_RX_Brw_Rsp_impl;
	__Mlsas_Queue_RTX(&rq->Mlrq_pr->Mlpd_mlb->Mlb_asender_wq, w);
}

static int __Mlsas_RX_Brw_Rsp_impl(Mlsas_rtx_wk_t *w)
{
	cs_rx_data_t *xd = (cs_rx_data_t *)w;
	Mlsas_RWrsp_msg_t *rsp = ((Mlsas_Msh_t *)xd->ex_head) + 1;
	Mlsas_request_t *rq = rsp->rsp_rqid;
	Mlsas_blkdev_t *Mlb = rq->Mlrq_bdev;
	Mlsas_bio_and_error_t m;
	boolean_t is_write = rq->Mlrq_flags & Mlsas_RQ_Write;
	uint32_t what;
	
	if (unlikely(rsp->rsp_error))
		what = is_write ? Mlsas_Rst_PR_Write_Error :
			(bio_rw(rq->Mlrq_master_bio) == READ ? 
				Mlsas_Rst_PR_Read_Error :
				Mlsas_Rst_PR_ReadA_Error);
	else {
		what = Mlsas_Rst_PR_Complete_OK;
		if (!is_write)
			__Mlsas_PR_Read_Rsp_copy(rq->Mlrq_master_bio, 
				xd->data, xd->data_len);
	}

	rq->Mlrq_back_bio = rsp->rsp_error;

	spin_lock_irq(&Mlb->Mlb_rq_spin);
	__Mlsas_Req_Stmt(rq, what, &m);
	spin_unlock_irq(&Mlb->Mlb_rq_spin);

	if (m.Mlbi_bio)
		__Mlsas_Complete_Master_Bio(rq, &m);
}

static void __Mlsas_PR_Read_Rsp_copy(struct bio *bio, 
		void *rbuf, uint32_t rblen)
{
	struct bio_vec bvec;
	struct bvec_iter iter;
	uint32_t offset = 0;
	
	VERIFY(rblen == bio->bi_iter.bi_size);

	bio_for_each_segment(bvec, bio, iter) {
		void *mapped = kmap(bvec.bv_page) + bvec.bv_offset;
		bcopy(rbuf + offset, mapped, bvec.bv_len);
		offset += bvec.bv_len;
		kunmap(bvec.bv_page);
	}
}

Mlsas_pr_req_t *__Mlsas_Alloc_PR_RQ(Mlsas_pr_device_t *pr, 
		uint64_t reqid, sector_t sec, size_t len)
{
	Mlsas_pr_req_t *prr = NULL;

	if ((prr = mempool_alloc(gMlsas_ptr->Ml_prr_mempool,
			GFP_NOIO | __GFP_ZERO)) == NULL)
		return NULL;

	prr->prr_bsector = sec;
	prr->prr_bsize = len;
	prr->prr_pr_rq = reqid;
	prr->prr_pr = pr;
	prr->prr_delayed_magic = Mlsas_Delayed_PR_RQ;

	kref_init(&prr->prr_ref);
	return prr;
}

void __Mlsas_Free_PR_RQ(Mlsas_pr_req_t *prr)
{
	mempool_free(prr, gMlsas_ptr->Ml_prr_mempool);
}

static void __Mlsas_Release_PR_RQ(struct kref *ref)
{
	Mlsas_pr_req_t *prr = container_of(ref,
		Mlsas_pr_req_t, prr_ref);

	if (!(prr->prr_flags & Mlsas_PRRfl_Write) &&
		(prr->prr_flags & Mlsas_PRRfl_Addl_Kmem))
		kmem_free(prr->prr_dt, prr->prr_dtlen);
	
	__Mlsas_Free_PR_RQ(prr);
}

static inline unsigned long
__Mlsas_Bio_nrpages(void *bio_ptr, unsigned int bio_size)
{
	return ((((unsigned long)bio_ptr + bio_size + PAGE_SIZE - 1) >>
	    PAGE_SHIFT) - ((unsigned long)bio_ptr >> PAGE_SHIFT));
}

static uint32_t
__Mlsas_Bio_map(struct bio *bio, void *bio_ptr, unsigned int bio_size)
{
	unsigned int offset, size, i;
	struct page *page;

	offset = offset_in_page(bio_ptr);
	for (i = 0; i < bio->bi_max_vecs; i++) {
		size = PAGE_SIZE - offset;

		if (bio_size <= 0)
			break;

		if (size > bio_size)
			size = bio_size;
		
		page = virt_to_page(bio_ptr);

		ASSERT3S(page_count(page), >, 0);

		if (bio_add_page(bio, page, size, offset) != size)
			break;

		bio_ptr  += size;
		bio_size -= size;
		offset = 0;
	}

	return (bio_size);
}

static struct bio *__Mlsas_PR_RQ_New_bio(uint32_t bv_cnt, sector_t sect,
		uint32_t rw, struct block_device *bdev)
{
	struct bio *bio;

	if (bv_cnt == 0)
		bv_cnt = 1;

	bio = bio_alloc(GFP_NOIO, bv_cnt);
	bio->bi_iter.bi_sector = sect;
	bio->bi_bdev = bdev;
	bio->bi_rw = rw;
	bio->bi_end_io = __Mlsas_PR_RQ_endio;

	return bio;
}

struct bio *__Mlsas_PR_RQ_bio(Mlsas_pr_req_t *prr, unsigned long rw, 
		void *data, uint64_t dlen)
{
	uint32_t remained;
	struct bio *bio = NULL, *bios = NULL;
	sector_t sector = prr->prr_bsector;
	uint32_t bv_cnt = __Mlsas_Bio_nrpages(data, dlen); 
	Mlsas_blkdev_t *Mlb = prr->prr_pr->Mlpd_mlb;
	struct block_device *backing_dev = Mlb->Mlb_bdi.Mlbd_bdev;

new_bio:
	bv_cnt = __Mlsas_Bio_nrpages(data, dlen);
	bio = __Mlsas_PR_RQ_New_bio(bv_cnt, sector, rw, backing_dev);
	bio->bi_private = prr;
	bio->bi_next = bios;
	bios = bio;

	if (rw & REQ_DISCARD) {
		bio->bi_iter.bi_size = prr->prr_bsize;
		return bio;
	}
	
	if ((remained = __Mlsas_Bio_map(bio, data, dlen)) != 0) {
		data += (dlen - remained);
		dlen = remained;
		sector += ((dlen - remained) >> 9);
		goto new_bio;
	}

	return bio;
}

static void __Mlsas_PR_RQ_endio(struct bio *bio)
{
	Mlsas_pr_req_t *prr = bio->bi_private;
	Mlsas_blkdev_t *Mlb = prr->prr_pr->Mlpd_mlb;
	uint32_t what;
	
	if (unlikely(bio->bi_error)) {
		/* TODO */
		prr->prr_error = bio->bi_error;
		if (unlikely(bio->bi_rw & REQ_DISCARD))
			what = Mlsas_PRRst_Discard_Error;
		else
			what = (prr->prr_flags & Mlsas_PRRfl_Write) ? 
				Mlsas_PRRst_Write_Error : (bio_rw(bio) == READ) ? 
					Mlsas_PRRst_Read_Error : Mlsas_PRRst_ReadA_Error;
	} else
		what = Mlsas_PRRst_Complete_OK;

	bio_put(bio);

	if (!atomic_dec_32_nv(&prr->prr_pending_bios)) {
		spin_lock_irq(&Mlb->Mlb_rq_spin);
		__Mlsas_PR_RQ_stmt(prr, what);
		__Mlsas_PR_RQ_stmt(prr, prr->prr_flags & Mlsas_PRRfl_Write ?
			Mlsas_PRRst_Queue_Net_Wrsp : Mlsas_PRRst_Queue_Net_Rrsp);
		spin_unlock_irq(&Mlb->Mlb_rq_spin);
	}

//	__Mlsas_Complete_PR_RQ(prr);
}

void __Mlsas_PR_RQ_stmt(Mlsas_pr_req_t *prr, uint32_t what)
{
	switch (what) {
	case Mlsas_PRRst_Tobe_Submit:
		kref_get(&prr->prr_ref);
		list_insert_tail(&prr->prr_pr->Mlpd_mlb->Mlb_peer_rqs, prr);
		list_insert_tail(&prr->prr_pr->Mlpd_pr_rqs, prr);
		__Mlsas_PR_RQ_st(prr, 0, Mlsas_PRRfl_Pending);
		break;
	case Mlsas_PRRst_Discard_Error:
	case Mlsas_PRRst_Write_Error:
	case Mlsas_PRRst_Read_Error:
	case Mlsas_PRRst_ReadA_Error:
		__Mlsas_PR_RQ_st(prr, Mlsas_PRRfl_Pending, Mlsas_PRRfl_IO_Done);
		break;
	case Mlsas_PRRst_Complete_OK:
		__Mlsas_PR_RQ_st(prr, Mlsas_PRRfl_Pending, 
			Mlsas_PRRfl_IO_Done | Mlsas_PRRfl_IO_OK);
		break;
	case Mlsas_PRRst_Queue_Net_Wrsp:
		__Mlsas_PR_RQ_st(prr, 0, Mlsas_PRRfl_Netpending);
		__Mlsas_PR_RQ_Write_endio(prr);
		break;
	case Mlsas_PRRst_Queue_Net_Rrsp:
		__Mlsas_PR_RQ_st(prr, 0, Mlsas_PRRfl_Netpending);
		__Mlsas_PR_RQ_Read_endio(prr);
		break;
	case Mlsas_PRRst_Send_Error:
		__Mlsas_PR_RQ_st(prr, Mlsas_PRRfl_Netpending, Mlsas_PRRfl_Net_Done);
		break;
	case Mlsas_PRRst_Send_OK:
		__Mlsas_PR_RQ_st(prr, Mlsas_PRRfl_Netpending, 
			Mlsas_PRRfl_Net_Done | Mlsas_PRRfl_OK);
		break;
	}
}

static void __Mlsas_PR_RQ_st(Mlsas_pr_req_t *prr, uint32_t c, uint32_t s)
{
	uint32_t ofl = prr->prr_flags;
	uint32_t c_put = 0;
	uint32_t k_put = 0;
	Mlsas_pr_device_t *pr = prr->prr_pr;
		
	prr->prr_flags &= ~c;
	prr->prr_flags |= s;

	if (prr->prr_flags == ofl)
		return ;
	
	if (!(ofl & Mlsas_PRRfl_Pending) && (s & Mlsas_PRRfl_Pending))
		atomic_inc_32(&prr->prr_completion_ref);

	if (!(ofl & Mlsas_PRRfl_Netpending) && (s & Mlsas_PRRfl_Netpending))
		atomic_inc_32(&prr->prr_completion_ref);

	if (!(ofl & Mlsas_PRRfl_OK) && (s & Mlsas_PRRfl_OK)) {
		list_remove(&pr->Mlpd_pr_rqs, prr);
		list_remove(&pr->Mlpd_mlb->Mlb_peer_rqs, prr);
		k_put++;
	}

	if ((ofl & Mlsas_PRRfl_Pending) && (c & Mlsas_PRRfl_Pending)) 
		c_put++;

	if ((ofl & Mlsas_PRRfl_Netpending) && (c & Mlsas_PRRfl_Netpending))
		c_put++;

	if (c_put)
		k_put += __Mlsas_PR_RQ_Put_Completion_ref(prr, c_put);
	if (k_put)
		kref_sub(&prr->prr_ref, k_put, __Mlsas_Release_PR_RQ);
	
}

static int __Mlsas_PR_RQ_Put_Completion_ref(Mlsas_pr_req_t *prr,
		uint32_t c_put)
{
	if (atomic_sub_32_nv(&prr->prr_completion_ref, c_put))
		return 0;

	__Mlsas_PR_RQ_complete(prr);

	if (prr->prr_flags & Mlsas_PRRfl_Continue)
		return (0);
	if (prr->prr_flags & Mlsas_PRRfl_Delayed) {
		__Mlsas_Restart_DelayedRQ(prr);
		return (0);
	}

	return (1);
}

static void __Mlsas_PR_RQ_complete(Mlsas_pr_req_t *prr)
{
	boolean_t iook, ok;
	Mlsas_blkdev_t *Mlb = prr->prr_pr->Mlpd_mlb;
	
	iook = prr->prr_flags & Mlsas_PRRfl_IO_OK;
	ok = prr->prr_flags & Mlsas_PRRfl_OK;
	/*
	 * IO pending done
	 */
	if (!(prr->prr_flags & Mlsas_PRRfl_Net_Done)) {
		if (!iook && (++Mlb->Mlb_error_cnt >= Mlb->Mlb_switch))
			__Mlsas_Devst_Stmt(Mlb, Mlsas_Devevt_Error_Switch);
		else if (iook)
			Mlb->Mlb_error_cnt = 0;
		prr->prr_flags |= Mlsas_PRRfl_Continue;
	} else {
		prr->prr_flags &= ~Mlsas_PRRfl_Continue;
		if (!ok)
			prr->prr_flags |= Mlsas_PRRfl_Delayed;
	}
}

static void __Mlsas_Complete_PR_RQ(Mlsas_pr_req_t *prr)
{
	Mlsas_blkdev_t *Mlb = prr->prr_pr->Mlpd_mlb;
	boolean_t is_write = prr->prr_flags & Mlsas_PRRfl_Write;
	int remained_bios = atomic_dec_32_nv(&prr->prr_pending_bios);
	
	spin_lock_irq(&Mlb->Mlb_rq_spin);
	if (prr->prr_flags & Mlsas_PRRfl_Ee_Error) {
		if (++Mlb->Mlb_error_cnt >= Mlb->Mlb_switch)
			__Mlsas_Devst_Stmt(Mlb, Mlsas_Devevt_Error_Switch);
	} else
		Mlb->Mlb_error_cnt = 0;
	spin_unlock_irq(&Mlb->Mlb_rq_spin);

	if (!remained_bios) {
		if (is_write)
			__Mlsas_PR_RQ_Write_endio(prr);
		else 
			__Mlsas_PR_RQ_Read_endio(prr);
	}
}

static void __Mlsas_PR_RQ_Write_endio(Mlsas_pr_req_t *prr)
{
	Mlsas_blkdev_t *Mlb = prr->prr_pr->Mlpd_mlb;
	Mlsas_rtx_wk_t *w = &prr->prr_wk;

	w->rtw_fn = __Mlsas_Tx_PR_RQ_rsp;
	__Mlsas_Queue_RTX(&Mlb->Mlb_asender_wq, &prr->prr_wk);
}

static void __Mlsas_PR_RQ_Read_endio(Mlsas_pr_req_t *prr)
{
	Mlsas_blkdev_t *Mlb = prr->prr_pr->Mlpd_mlb;
	Mlsas_rtx_wk_t *w = &prr->prr_wk;

	w->rtw_fn = __Mlsas_Tx_PR_RQ_rsp;
	__Mlsas_Queue_RTX(&Mlb->Mlb_sender_wq, &prr->prr_wk);
}

static Mlsas_rh_t *__Mlsas_Alloc_RH(uint32_t id, void *tran_ss)
{
	Mlsas_rh_t *rh = NULL;
	
	if ((rh = kzalloc(sizeof(Mlsas_rh_t), 
			GFP_KERNEL)) == NULL)
		return NULL;
	
	rh->Mh_session = tran_ss;
	rh->Mh_hostid = id;
	rh->Mh_state = Mlsas_RHS_New;
	kref_init(&rh->Mh_ref);
	mutex_init(&rh->Mh_mtx, NULL, MUTEX_DEFAULT, NULL);
	list_create(&rh->Mh_devices, sizeof(Mlsas_pr_device_t),
		offsetof(Mlsas_pr_device_t, Mlpd_rh_node));

	return rh;
}

static void __Mlsas_Release_RH(struct kref *ref)
{
	
}

static Mlsas_pr_device_t *__Mlsas_Alloc_PR(uint32_t id, Mlsas_devst_e st,
		Mlsas_blkdev_t *Mlb, Mlsas_rh_t *rh)
{
	Mlsas_pr_device_t *pr = NULL;
	
	if ((pr = kzalloc(sizeof(Mlsas_pr_device_t),
			GFP_KERNEL)) == NULL)
		return NULL;

	kref_init(&pr->Mlpd_ref);
	list_create(&pr->Mlpd_rqs, sizeof(Mlsas_request_t),
		offsetof(Mlsas_request_t, Mlrq_pr_node));
	list_create(&pr->Mlpd_pr_rqs, sizeof(Mlsas_pr_req_t),
		offsetof(Mlsas_pr_req_t, prr_node));
	
	pr->Mlpd_st = st;
	pr->Mlpd_hostid = id;
	pr->Mlpd_mlb = Mlb;
	pr->Mlpd_rh = rh;
	pr->Mlpd_switch = Mlb->Mlb_switch;
	return pr;
}

static Mlsas_Msh_t *__Mlsas_Alloc_Mms(uint32_t extsz, 
		uint8_t type, uint64_t hashkey, 
		void **dt_ptr)
{
	uint32_t size = 0;
	uint32_t Mms_sz = sizeof(Mlsas_Msh_t);
	Mlsas_Msh_t *mms = NULL;
	
	extsz = (extsz + 7) & ~7;
	if ((mms = kzalloc(Mms_sz + extsz, 
			GFP_NOIO)) == NULL) 
		return NULL;

	mms->Mms_ck = Mlsas_Mms_Magic;
	mms->Mms_len = Mms_sz + extsz;
	mms->Mms_hashkey = hashkey;
	mms->Mms_type = type;

	if (dt_ptr)
		*dt_ptr = mms + 1;
	return mms;
}

static void __Mlsas_Free_Mms(Mlsas_Msh_t *mms)
{
	kfree(mms);
}

static Mlsas_pr_device_t *__Mlsas_Lookup_PR_locked(
		Mlsas_blkdev_t *Mlb, uint32_t hostid)
{
	Mlsas_pr_device_t *pr, *pr_next;

	pr = list_head(&Mlb->Mlb_pr_devices);
	while (pr) {
		pr_next = list_next(&Mlb->Mlb_pr_devices, pr);
		if (pr->Mlpd_hostid == hostid)
			break;
		pr = pr_next;
	}

	return pr;
}

static void __Mlsas_Update_PR_st(Mlsas_pr_device_t *pr, 
		Mlsas_devst_e st)
{
	pr->Mlpd_st = st;
	if (__Mlsas_Get_PR_if_state(pr, Mlsas_Devst_Attached))
		__Mlsas_Devst_Stmt(pr->Mlpd_mlb, Mlsas_Devevt_Attach_PR);
	if (pr->Mlpd_st == Mlsas_Devst_Degraded)
		__Mlsas_Devst_Stmt(pr->Mlpd_mlb, Mlsas_Devevt_PR_Error_Sw);
}

void __Mlsas_Submit_PR_request(Mlsas_blkdev_t *Mlb,
		unsigned long rw, Mlsas_pr_req_t *prr)
{
	int rval = 0;
	struct bio *bio = NULL, *bt = NULL;

	if (prr->prr_flags & Mlsas_PRRfl_Zero_Out) {
		uint32_t what = Mlsas_PRRst_Complete_OK;
		atomic_inc_32(&prr->prr_pending_bios);
		if ((rval = blkdev_issue_zeroout(Mlb->Mlb_bdi.Mlbd_bdev,
				prr->prr_bsector, prr->prr_bsize >> 9, 
				GFP_NOIO, false)) != 0) {
			prr->prr_flags |= Mlsas_PRRfl_Ee_Error;
			prr->prr_error = rval;
			what = Mlsas_PRRst_Discard_Error;
		}
		spin_lock_irq(&Mlb->Mlb_rq_spin);
		__Mlsas_PR_RQ_stmt(prr, what);
		__Mlsas_PR_RQ_stmt(prr, Mlsas_PRRst_Queue_Net_Wrsp);
		spin_unlock_irq(&Mlb->Mlb_rq_spin);
		return ;
	}

	VERIFY(bio = __Mlsas_PR_RQ_bio(prr, rw, 
		prr->prr_dt, prr->prr_dtlen));

//	cmn_err(CE_NOTE, "%s rw_flags(0x%llx)", __func__, bio->bi_rw);
	
	do {
		bt = bio;
		bio = bio->bi_next;
		bt->bi_next = NULL;
		(void) generic_make_request(bt);
		
		atomic_inc_32(&prr->prr_pending_bios);
	} while (bio);

	return ;
}

static boolean_t __Mlsas_Has_Active_PR(Mlsas_blkdev_t *Mlb, 
		Mlsas_pr_device_t *prflt)
{
	boolean_t rval = B_FALSE;
	Mlsas_pr_device_t *pr = list_head(&Mlb->Mlb_pr_devices);

	while (pr && !rval) {
		if (__Mlsas_Get_PR_if_state(pr, Mlsas_Devst_Attached)
			&& (pr != prflt)  && 
			(!prflt || (pr->Mlpd_hostid != prflt->Mlpd_hostid)))
			rval = B_TRUE;
	}

	return rval;
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
module_param(Mlsas_minors, int, 0644);

module_init(__Mlsas_Init);
module_exit(__Mlsas_Exit);
MODULE_LICENSE("GPL");
