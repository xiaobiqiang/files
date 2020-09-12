#define Mlsas_Vmpt_Module_Name	"Mlsas_vmpt"

#define Mlsas_Vmpt_Probe_None		0x00
#define Mlsas_Vmpt_Probe_Inproc		0x01
#define Mlsas_Vmpt_Probe_Complete	0x02

#define Mlsas_Vmpt_REQ_Magic		0xdeaddead

#define Mlsas_Vmpt_REQ_First		0x00
#define Mlsas_Vmpt_REQ_IO			0x05
#define Mlsas_Vmpt_REQ_Last			0x00

#define Mlsas_Vmpt_Support_Max_CDB	32

#define Mlsas_Vmpt_Spflag_Term		0x01

#define Mlsas_Vmpt_IOReq_nCtx		32
#define Mlsas_Vmpt_IOReq_Ctx_Mask	(Mlsas_Vmpt_IOReq_nCtx - 1)

#define Mlsas_Vmpt_Scsih_ID(hostid, scsih_no)	\
	(((hostid) << 16) | (scsih_no))
#define Mlsas_Vmpt_Scsih_DeID(scsih_id, hostid, scsih_no)	\
	hostid = (((scsih_id) >> 16) & 0x0000ffff);	\
	scsih_no = ((scsih_id) & 0x0000ffff);

typedef struct Mlsas_Vmpt_Scsih_priv Mlsas_Vmpt_Scsih_priv_t;
typedef struct Mlsas_Vmpt_Scsih_cmd Mlsas_Vmpt_Scsih_cmd_t;
typedef struct Mlsas_Vmpt_Scsih_req_head Mlsas_Vmpt_Scsih_req_head_t;
typedef struct Mlsas_Vmpt_Scsih_ioreq Mlsas_Vmpt_Scsih_ioreq_t;
typedef struct Mlsas_Vmpt_proxy_cmd Mlsas_Vmpt_proxy_cmd_t;

struct Mlsas_Vmpt_Scsih_cmd {
	struct list_head Sc_node;
	
	struct scsi_cmnd *Sc_scmd;
	struct Scsi_Host *Sc_scsih;
	uint64_t Sc_tag;
};

struct Mlsas_Vmpt_Scsih_priv {
	list_node_t Sp_node;
	struct Scsi_Host *Sp_scsih;

	spinlock_t Sp_lock;

	Mlsas_Vmpt_Scsih_cmd_t *Sp_cmd_pool;
	struct percpu_ida Sp_tag_pool;

	struct list_head Sp_runing_cmds;

	struct workqueue_struct *Sp_queue_work;
	struct work_struct Sp_work;

	Mlsas_Vmpt_Scsih_cmd_t *Sp_current_cmd;

	Mlsas_rh_t *Sp_rhost;

	void *Sp_tx_buff;
	uint32_t Sp_tx_bufflen;
	uint32_t Sp_tx_buff_used;

	struct kref Sp_ref;

	uint32_t Sp_flags;
	uint32_t Sp_tags;
	
	uint32_t Sp_scsih_id;
	
	uint32_t Sp_num_device;
};

struct Mlsas_Vmpt_Scsih_req_head {
	uint32_t hd_cmd;
	uint32_t hd_magic;
};

/* must be fitst two elements */
struct Mlsas_Vmpt_Scsih_ioreq {
	Mlsas_Vmpt_Scsih_req_head_t io_head;
	uint64_t io_Sp;
	uint32_t io_tag;
	uint32_t io_scsih;
	uint32_t io_channel;
	uint32_t io_id;
	uint32_t io_lun;
	uint32_t io_cdb_len;
	uint8_t io_cdb[Mlsas_Vmpt_Support_Max_CDB];
	uint32_t io_data_len;
	uint32_t io_data_direction;
};

struct Mlsas_Vmpt_proxy_cmd {
	struct work_struct xy_work;
	uint32_t xy_tag;
	uint32_t xy_scsih;
	uint32_t xy_channel;
	uint32_t xy_id;
	uint32_t xy_lun;
	uint32_t xy_ddirection;
	uint32_t xy_cdb[Mlsas_Vmpt_Support_Max_CDB];
	uint32_t xy_cdb_len;
	int xy_result; 
	uint8_t xy_sense[24];
	void *xy_data;
	uint32_t xy_dlen;
	uint32_t xy_remote_tag;
	uint64_t xy_Sp;
	struct scsi_device *xy_sdev;
	void *xy_session;	
};

static taskq_t *Mlsas_Vmpt_async_tq = NULL;
static int Mlsas_Vmpt_new_virt_delay = 1000; 
static int Mlsas_Vmpt_scsih_max_sector = 2048;	/* max io: 1M */
static int Mlsas_Vmpt_scsih_max_sg_tablesize = SCSI_MAX_SG_SEGMENTS;
/* 
 * a Scsi_Host contains 24 disks, every disk speed to 200M/s,
 * so in a second, most cmd is 24 * 200M/128K = 38400.
 */
static int Mlsas_Vmpt_scsih_cmd_tags = 38400;

static int Mlsas_Vmpt_proxy_cmd_tags = 38400;
static struct percpu_ida Mlsas_Vmpt_proxy_ida;
static Mlsas_Vmpt_proxy_cmd_t *Mlsas_Vmpt_proxy_cmds = NULL;

static kmutex_t Mlsas_Vmpt_scsih_list_mtx;
static list_t Mlsas_Vmpt_scsih_list;

/*
 * all scsi disk use commonly, we use id as mapping index.
 */
static struct workqueue_struct *Mlsas_Vmpt_ioreq_ctx_wq[Mlsas_Vmpt_IOReq_nCtx];

/* shost template for SAS 3.0 HBA devices */
static struct scsi_host_template Mlsas_Vmpt_scsih_template = {
	.module				= THIS_MODULE,
	.name				= Mlsas_Vmpt_Module_Name,
	.proc_name			= Mlsas_Vmpt_Module_Name,
	.queuecommand			= __Mlsas_Vmpt_Scsih_queue_command,
	.target_alloc			= NULL,
	.slave_alloc			= __Mlsas_Vmpt_Scsih_slave_alloc,
	.slave_configure		= __Mlsas_Vmpt_Scsih_slave_configure,
	.target_destroy			= NULL,
	.slave_destroy			= __Mlsas_Vmpt_Scsih_slave_destroy,
	.scan_finished			= NULL,
	.scan_start			= NULL,
	.change_queue_depth		= NULL,
	.eh_abort_handler		= NULL,
	.eh_device_reset_handler	= NULL,
	.eh_target_reset_handler	= NULL,
	.eh_host_reset_handler		= NULL,
	.bios_param			= NULL,
	.can_queue			= 1,
	.this_id			= -1,
	.sg_tablesize			= SCSI_MAX_SG_SEGMENTS,
	.max_sectors			= 32767,
	.cmd_per_lun			= 7,
	.use_clustering			= ENABLE_CLUSTERING,
	.shost_attrs			= NULL,
	.sdev_attrs			= NULL,
	.track_queue_depth		= 1,
};

static inline void __Mlsas_Vmpt_Sp_ref_get(Mlsas_Vmpt_Scsih_priv_t *Sp)
{
	kref_get(&Sp->Sp_ref);
}

static inline void __Mlsas_Vmpt_Sp_ref_put(Mlsas_Vmpt_Scsih_priv_t *Sp)
{
	kref_put(&Sp->Sp_ref, __Mlsas_Vmpt_release_Sp);
}

static int __Mlsas_Vmpt_Scsih_slave_alloc(struct scsi_device *sdev)
{
	
}

static int __Mlsas_Vmpt_Scsih_slave_configure(struct scsi_device *sdev)
{
	
}

static void __Mlsas_Vmpt_Scsih_slave_destroy(struct scsi_device *sdev)
{
	
}

static int __Mlsas_Vmpt_Scsih_queue_command(
		struct Scsi_Host *scsih, 
		struct scsi_cmnd *sc)
{
	Mlsas_Vmpt_Scsih_priv_t *Sp;
	Mlsas_Vmpt_Scsih_cmd_t *Scmd = NULL;
	int tag = 0, error = 0;
	
	VERIFY((Sp = shost_priv(scsih)) != NULL);

	spin_lock_irq(&Sp->Sp_lock);
	if (unlikely(Sp->Sp_flags & Mlsas_Vmpt_Spflag_Term))
		error = DID_NO_CONNECT;

	if ((tag = percpu_ida_alloc(&Sp->Sp_tag_pool, 
			TASK_INTERRUPTIBLE)) < 0)
		error = DID_SOFT_ERROR;

	if (unlikely(error)) {
		spin_unlock_irq(&Sp->Sp_lock);
		goto failed_out;
	}	

	__Mlsas_Vmpt_Sp_ref_get(Sp);

	Scmd = &Sp->Sp_cmd_pool[tag];
	VERIFY(list_empty(&Scmd->Sc_node));
	Scmd->Sc_tag = tag;
	Scmd->Sc_scmd = sc;
	Scmd->Sc_scsih = scsih;

	list_add_tail(&Scmd->Sc_node, &Sp->Sp_runing_cmds);

	spin_unlock_irq(&Sp->Sp_lock);

	__Mlsas_Vmpt_Scsih_queue_command_impl(Sp);

	return (0);

failed_out:
	sc->result = (error << 16);
	if (!scsi_bidi_cmnd(sc))
		scsi_set_resid(sc, scsi_bufflen(sc));
	else {
		scsi_out(sc)->resid = scsi_out(sc)->length;
		scsi_in(sc)->resid = scsi_in(sc)->length;
	}
	sc->scsi_done(sc);
	return (0);
}

static inline void __Mlsas_Vmpt_Scsih_queue_command_impl(
		Mlsas_Vmpt_Scsih_priv_t *Sp)
{
	queue_work(Sp->Sp_queue_work, &Sp->Sp_work);
}

static inline void __Mlsas_Vmpt_Scsih_fail_command(int e_did, 
		Mlsas_Vmpt_Scsih_cmd_t *Sc, 
		boolean_t rele_Sp)
{
	struct scsi_cmnd *sc = Sc->Sc_scmd;
	Mlsas_Vmpt_Scsih_priv_t *Sp = shost_priv(Sc->Sc_scsih);
	
	if (rele_Sp)
		__Mlsas_Vmpt_Sp_ref_put(Sp);

	percpu_ida_free(&Sp->Sp_tag_pool, Sc->Sc_tag);
	
	sc->result = (e_did << 16);
	if (!scsi_bidi_cmnd(sc))
		scsi_set_resid(sc, scsi_bufflen(sc));
	else {
		scsi_out(sc)->resid = scsi_out(sc)->length;
		scsi_in(sc)->resid = scsi_in(sc)->length;
	}
	sc->scsi_done(sc);
}

static inline void __Mlsas_Vmpt_Scsih_setup_ioreq(
		Mlsas_Vmpt_Scsih_ioreq_t *ioreq, 
		Mlsas_Vmpt_Scsih_priv_t *Sp)
{
	Mlsas_Vmpt_Scsih_cmd_t *Sc = Sp->Sp_current_cmd;
	uint32_t hostid;

	ioreq->io_head.hd_cmd = Mlsas_Vmpt_REQ_IO;
	ioreq->io_head.hd_magic = Mlsas_Vmpt_REQ_Magic;
	ioreq->io_Sp = Sp;
	ioreq->io_tag = Sc->Sc_tag;
	Mlsas_Vmpt_Scsih_DeID(Sp->Sp_scsih_id, hostid, ioreq->io_scsih);
	ioreq->io_channel = Sc->Sc_scmd->device->channel;
	ioreq->io_id = Sc->Sc_scmd->device->id;
	ioreq->io_lun = Sc->Sc_scmd->device->lun;
	ioreq->io_cdb_len = Sc->Sc_scmd->cmd_len;
	bcopy(Sc->Sc_scmd->cmnd, ioreq->io_cdb, ioreq->io_cdb_len);
	ioreq->io_data_len = Sc->Sc_scmd->sdb.length;
	ioreq->io_data_direction = Sc->Sc_scmd->sc_data_direction;
}

static void __Mlsas_Vmpt_Scsih_queuecommand_TX_work(struct work_struct *work)
{
	int error = 0;
	Mlsas_Vmpt_Scsih_priv_t *Sp = container_of(work,
		Mlsas_Vmpt_Scsih_priv_t, Sp_work);
	Mlsas_Msh_t *mms = NULL;
	struct scsi_cmnd *sc = NULL;

retry:	
	spin_lock_irq(&Sp->Sp_lock);
	if (!Sp->Sp_current_cmd) {
		if (!list_empty(&Sp->Sp_runing_cmds)) {
			Sp->Sp_current_cmd = list_first_entry(&Sp->Sp_runing_cmds,
				Mlsas_Vmpt_Scsih_cmd_t, Sc_node);
			list_del_init(&Sp->Sp_current_cmd->Sc_node);
		} else {
			spin_unlock_irq(&Sp->Sp_lock);
			return ;
		}
	}

	/* Sp->Sp_current_cmd can't be NULL */
	sc = Sp->Sp_current_cmd->Sc_scmd;
	
	if (unlikely(Sp->Sp_flags & Mlsas_Vmpt_Spflag_Term)) {
		error = DID_NO_CONNECT;
		goto fail_retry;
	}

	spin_unlock_irq(&Sp->Sp_lock);

	mms = (Mlsas_Msh_t *)Sp->Sp_tx_buff;
	mms->Mms_ck = Mlsas_Mms_Magic;
	mms->Mms_rh = Sp->Sp_rhost;
	mms->Mms_type = Mlsas_Mms_Vmpt;
	mms->Mms_len = sizeof(Mlsas_Msh_t) + sizeof(
		Mlsas_Vmpt_Scsih_ioreq_t);
	__Mlsas_Vmpt_Scsih_setup_ioreq(mms + 1, Sp);
	Sp->Sp_tx_buff_used = mms->Mms_len;
	
	if ((error = __Mlsas_TX(Sp->Sp_rhost->Mh_session, Sp->Sp_tx_buff, 
			Sp->Sp_tx_buff_used, &sc->sdb.table, sc->sdb.length,
			Mlsas_TX_Type_Sgl)) != 0) {
		error = DID_TRANSPORT_DISRUPTED;
		goto fail_retry;
	}

	Sp->Sp_current_cmd = NULL;
	
	return ;

fail_retry:
	__Mlsas_Vmpt_Scsih_fail_command(error, 
			Sp->Sp_current_cmd, B_TRUE);
	Sp->Sp_current_cmd = NULL;
	
	spin_unlock_irq(&Sp->Sp_lock);
	
	goto retry;
}

static void __Mlsas_Vmpt_io_req_HDL_impl(struct work_struct *work)
{
	struct request *scsi_req;
	Mlsas_Vmpt_proxy_cmd_t *xycmd = container_of(work,
			Mlsas_Vmpt_proxy_cmd_t, xy_work);
	struct scsi_device *sdev = xycmd->xy_sdev;

	if (!sdev || xycmd->xy_result) {
		
	} else {
		scsi_req = blk_get_request(sdev);
	}
}

static void __Mlsas_Vmpt_setup_proxy_cmd(uint32_t tag, 
		Mlsas_Vmpt_proxy_cmd_t *xycmd,
		Mlsas_Vmpt_Scsih_ioreq_t *ioreq, 
		cs_rx_data_t *xd)
{
	xycmd->xy_tag = tag;
	xycmd->xy_session = xd->cs_private;
	__Mlsas_clustersan_hostinfo_hold(xd->cs_private);
	xycmd->xy_Sp = ioreq->io_Sp;
	xycmd->xy_remote_tag = ioreq->io_tag;
	xycmd->xy_scsih = ioreq->io_scsih;
	xycmd->xy_channel = ioreq->io_channel;
	xycmd->xy_id = ioreq->io_id;
	xycmd->xy_lun = ioreq->io_lun;
	xycmd->xy_ddirection = ioreq->io_data_direction;
	xycmd->xy_cdb_len = ioreq->io_cdb_len;
	bcopy(ioreq->io_cdb, xycmd->xy_cdb, ioreq->io_cdb_len);
	xycmd->xy_data = xd->data;
	xycmd->xy_dlen = ioreq->io_data_len;
	xycmd->xy_result = DID_OK;

	if (ioreq->io_data_direction == DMA_FROM_DEVICE) {
		VERIFY(!xd->data && !xd->data_len);
		VERIFY((xycmd->xy_data = cs_kmem_alloc(
			xycmd->xy_dlen)) != NULL);
	}

	INIT_WORK(&xycmd->xy_work, __Mlsas_Vmpt_io_req_HDL_impl);
}

static void __Mlsas_Vmpt_RX_io_req_HDL(
		Mlsas_Vmpt_Scsih_ioreq_t *ioreq,
		cs_rx_data_t *xd)
{
	uint32_t io_nctx = 0, tag;
	struct workqueue_struct *io_ctx = NULL;
	Mlsas_Vmpt_proxy_cmd_t *xycmd = NULL;
	struct Scsi_Host *scsih = NULL;
	struct scsi_device *sdev = NULL;
	int error = DID_OK;
	
	if (((scsih = scsi_host_lookup(ioreq->io_scsih)) == NULL) || 
		((sdev = scsi_device_lookup(scsih, ioreq->io_channel,
			ioreq->io_id, ioreq->io_lun)) == NULL)) {
		cmn_err(CE_NOTE, "%s CAN'T FIND SCSI DEVICE(%u:%u:%u:%u)",
			__func__, ioreq->io_scsih, ioreq->io_channel,
			ioreq->io_id, ioreq->io_lun);
		error = -ENXIO;
	}

	if (!IS_ERR_OR_NULL(scsih))
		scsi_host_put(scsih);
	
	io_nctx = ioreq->io_id & Mlsas_Vmpt_IOReq_Ctx_Mask;
	io_ctx = Mlsas_Vmpt_ioreq_ctx_wq[io_nctx];

	VERIFY((tag = percpu_ida_alloc(&Mlsas_Vmpt_proxy_ida, 
			TASK_INTERRUPTIBLE)) >= 0);

	xycmd = &Mlsas_Vmpt_proxy_cmds[tag];
	__Mlsas_Vmpt_setup_proxy_cmd(tag, xycmd, ioreq, xd);
	xycmd->xy_result = error;
	xycmd->xy_sdev = sdev;

	queue_work(io_ctx, &xycmd->xy_work);

	xd->data = NULL;
	xd->data_len = 0;
	__Mlsas_clustersan_rx_data_free_ext(xd);
}

static void __Mlsas_RX_Vmpt(Mlsas_Msh_t *mms, cs_rx_data_t *xd)
{
	Mlsas_Vmpt_Scsih_req_head_t *reqhd = mms + 1;

	if ((reqhd->hd_magic != Mlsas_Vmpt_REQ_Magic) ||
		(reqhd->hd_cmd <= Mlsas_Vmpt_REQ_First) ||
		(reqhd->hd_cmd >= Mlsas_Vmpt_REQ_Last)) {
		cmn_err(CE_NOTE, "%s WRONG MAGIC(%u) or CMD(%u)",
			__func__, reqhd->hd_magic, reqhd->hd_cmd);
		goto free_xd;
	}

	switch (reqhd->hd_cmd) {
	case Mlsas_Vmpt_REQ_IO:
		__Mlsas_Vmpt_RX_io_req_HDL(reqhd + 1, xd);
		break;
	}

free_xd:
	__Mlsas_clustersan_rx_data_free_ext(xd);
}

static void __Mlsas_Vmpt_deactive_SP(Mlsas_Vmpt_Scsih_priv_t *Sp)
{
	mutex_enter(&Mlsas_Vmpt_scsih_list_mtx);
	
	if (Sp->Sp_remote_session)
		cluster_san_hostinfo_rele(Sp->Sp_remote_session);
	if (Sp->Sp_cmd_pool)
		kvfree(Sp->Sp_cmd_pool);
	if (list_link_active(&Sp->Sp_node))
		list_remove(&Mlsas_Vmpt_scsih_list, Sp);

	percpu_ida_destroy(&Sp->Sp_tag_pool);
	
	mutex_exit(&Mlsas_Vmpt_scsih_list_mtx);
}

static void __Mlsas_Vmpt_remove_vmpt_device(Mlsas_blkdev_t *vt)
{
	Mlsas_Vmpt_Scsih_priv_t *Sp = NULL;
	struct Scsi_Host *scsih = NULL;
	struct scsi_device *sdev = NULL;
	
	if ((Sp = __Mlsas_Vmpt_lookup_SP(Mlsas_Vmpt_Scsih_ID(
			vt->Mlb_hostid, vt->Mlb_shostno))) == NULL)
		cmn_err(CE_NOTE, "Maybe remote disk is removing");

	if (Sp && Sp->Sp_scsih) {
		sdev = scsi_device_lookup(Sp->Sp_scsih, 
				vt->Mlb_channel, 
				vt->Mlb_id, vt->Mlb_lun);
		scsi_remove_device(sdev);
		scsi_device_put(sdev);

		if (!atomic_dec_32_nv(&Sp->Sp_num_device)) {
			scsi_remove_host(Sp->Sp_scsih);

			__Mlsas_Vmpt_deactive_SP(Sp);
			
			scsi_host_put(Sp->Sp_scsih);
		}
	}
}

static Mlsas_Vmpt_Scsih_priv_t *
		__Mlsas_Vmpt_lookup_SP_locked(uint32_t scsih_id)
{
	Mlsas_Vmpt_Scsih_priv_t *Sp = NULL;

	VERIFY(MUTEX_HELD(&Mlsas_Vmpt_scsih_list_mtx));
	
	for (Sp = list_head(&Mlsas_Vmpt_scsih_list_mtx); Sp;
			Sp = list_next(&Mlsas_Vmpt_scsih_list_mtx, Sp)) {
		if (Sp->Sp_scsih_id == scsih_id)
			break;
	}

	return Sp;
}

static Mlsas_Vmpt_Scsih_priv_t *
		__Mlsas_Vmpt_lookup_SP(uint32_t scsih_id)
{
	Mlsas_Vmpt_Scsih_priv_t *Sp = NULL;
	
	mutex_enter(&Mlsas_Vmpt_scsih_list_mtx);
	Sp = __Mlsas_Vmpt_lookup_SP_locked(scsih_id);
	mutex_exit(&Mlsas_Vmpt_scsih_list_mtx);

	return Sp;
}

static struct Scsi_Host *__Mlsas_Vmpt_hold_vmpt_scsih(
		uint32_t scsih_id, Mlsas_rh_t *rh)
{
	struct Scsi_Host *scsih = NULL;
	Mlsas_Vmpt_Scsih_priv_t *Sp = NULL;

	mutex_enter(&Mlsas_Vmpt_scsih_list_mtx);
	for (Sp = list_head(&Mlsas_Vmpt_scsih_list_mtx); Sp;
			Sp = list_next(&Mlsas_Vmpt_scsih_list_mtx, Sp)) {
		if (Sp->Sp_scsih_id == scsih_id)
			break;
	}

	if (Sp) {
		if (Sp->Sp_rhost != rh) {
			__Mlsas_put_rhost(Sp->Sp_rhost);
			Sp->Sp_rhost = rh;
			__Mlsas_get_rhost(rh);
		}
		goto out;
	}
	
	VERIFY((scsih = scsi_host_alloc(&Mlsas_Vmpt_scsih_template, 
		sizeof(Mlsas_Vmpt_Scsih_priv_t))) != NULL);
	scsih->max_cmd_len = 16;
	scsih->max_id = 128;
	scsih->max_lun = 16;
	scsih->max_channel = 0;
	scsih->max_sectors = Mlsas_Vmpt_scsih_max_sector;
	scsih->sg_tablesize = Mlsas_Vmpt_scsih_max_sg_tablesize;
	
	Sp = shost_priv(scsih);
	Sp->Sp_tags = Mlsas_Vmpt_scsih_cmd_tags;
	Sp->Sp_scsih = scsih;
	Sp->Sp_scsih_id = scsih_id;
	Sp->Sp_remote_session = cshi;
	Sp->Sp_num_device = 0;
	Sp->Sp_current_cmd = NULL;
	Sp->Sp_tx_bufflen = 32 << 10;

	list_link_init(&Sp->Sp_node);
	spin_lock_init(&Sp->Sp_lock);
	INIT_LIST_HEAD(&Sp->Sp_runing_cmds);
	kref_init(&Sp->Sp_ref);
	INIT_WORK(&Sp->Sp_work, __Mlsas_Vmpt_Scsih_queuecommand_TX_work);

	VERIFY((Sp->Sp_tx_buff = kzalloc(Sp->Sp_tx_bufflen, 
		GFP_KERNEL)) != NULL);
	VERIFY((Sp->Sp_queue_work = create_singlethread_workqueue(
		"Mltsas_qcmd_workq")) != NULL);
	VERIFY(percpu_ida_init(&Sp->Sp_tag_pool, Sp->Sp_tags) == 0);
	if ((Sp->Sp_cmd_pool = kzalloc(sizeof(Mlsas_Vmpt_Scsih_cmd_t) * 
			Sp->Sp_tags, GFP_KERNEL)) == NULL) {
		VERIFY((Sp->Sp_cmd_pool = vzalloc(Sp->Sp_tags *
			sizeof(Mlsas_Vmpt_Scsih_cmd_t))) != NULL);
	}
	
	cluster_san_hostinfo_hold(cshi);
	
	list_insert_head(&Mlsas_Vmpt_scsih_list_mtx, Sp);
	__Mlsas_Vmpt_Sp_ref_get(Sp);

	(void) scsi_add_host(Sp->Sp_scsih, NULL);

out:
	mutex_exit(&Mlsas_Vmpt_scsih_list_mtx);

	(void) scsi_host_get(Sp->Sp_scsih);
	return Sp->Sp_scsih;
}

static void __Mlsas_Vmpt_Rx_Attach_new_vmpt_impl(Mlsas_blkdev_t *Mlb)
{
	struct Scsi_Host *scsih = NULL;
	Mlsas_rh_t *rh = NULL;
	
	spin_lock_irq(&Mlb->Mlb_rq_spin);
	if (Mlb->Mlb_in_resume_virt || 
		!Mlb->Mlb_vmpt_probe_qid ||
		!__Mlsas_Get_ldev_if_state_between(Mlb, 
			Mlsas_Devst_Standalone,
			Mlsas_Devst_Degraded)) {
		spin_unlock_irq(&Mlb->Mlb_rq_spin);
		return ;
	}

	VERIFY(Mlb->Mlb_vmpt_probe_qid > 0);

	Mlb->Mlb_vmpt_probe_stage = Mlsas_Vmpt_Probe_Inproc;

	rh = ((Mlsas_pr_device_t *)list_head(&Mlb->Mlb_pr_devices))->Mlpd_rh;
	__Mlsas_get_rhost(rh);

	spin_unlock_irq(&Mlb->Mlb_rq_spin);

	scsih = __Mlsas_Vmpt_hold_vmpt_scsih(Mlsas_Vmpt_Scsih_ID(
		Mlb->Mlb_hostid, Mlb->Mlb_shostno), rh);

	__Mlsas_put_rhost(rh);

	(void) scsi_add_device(scsih, Mlb->Mlb_channel, 
		Mlb->Mlb_id, Mlb->Mlb_lun);

	scsi_host_put(scsih);

	spin_lock_irq(&Mlb->Mlb_rq_spin);
	Mlb->Mlb_vmpt_probe_stage = Mlsas_Vmpt_Probe_Complete;
	Mlb->Mlb_vmpt_probe_qid = NULL;
	wake_up_all(&Mlb->Mlb_wait);
	spin_unlock_irq(&Mlb->Mlb_rq_spin);

	__Mlsas_put_virt(Mlb);
}

void __Mlsas_Vmpt_Rx_Attach_new_vmpt(Mlsas_blkdev_t *Mlb)
{
	spin_lock_irq(&Mlb->Mlb_rq_spin);
	if (!Mlb->Mlb_in_resume_virt &&
		__Mlsas_Get_ldev_if_state_between(Mlb, 
			Mlsas_Devst_Standalone,
			Mlsas_Devst_Degraded))
		VERIFY((Mlb->Mlb_vmpt_probe_qid = taskq_dispatch_delay(
			Mlsas_Vmpt_async_tq, 
			__Mlsas_Vmpt_Rx_Attach_new_vmpt_impl, 
			Mlb, KM_SLEEP, 
			MSEC_TO_TICK(Mlsas_Vmpt_new_virt_delay))) > 0);
	spin_unlock_irq(&Mlb->Mlb_rq_spin);

	/* a another reference to Mlb released by impl or Resume_wait */
}

void __Mlsas_Resume_wait_new_vmpt_inprocess(Mlsas_blkdev_t *vt)
{
	taskqid_t qid_tocancel = NULL;
	boolean_t to_remove_vmpt_scsih = FALSE;
	
	DEFINE_WAIT(wait);
	
	spin_lock_irq(&vt->Mlb_rq_spin);
	if (vt->Mlb_vmpt_probe_qid) {
		while (vt->Mlb_vmpt_probe_stage == Mlsas_Vmpt_Probe_Inproc) {
			prepare_to_wait(&vt->Mlb_wait, &wait, TASK_UNINTERRUPTIBLE);
			spin_unlock_irq(&vt->Mlb_rq_spin);

			io_schedule();

			spin_lock_irq(&vt->Mlb_rq_spin);
		}
			
		qid_tocancel = vt->Mlb_vmpt_probe_qid;
		vt->Mlb_vmpt_probe_qid = NULL;
	}

	if (vt->Mlb_vmpt_probe_stage == 
			Mlsas_Vmpt_Probe_Complete)
		to_remove_vmpt_scsih = TRUE;

	vt->Mlb_vmpt_probe_stage = Mlsas_Vmpt_Probe_None;
	spin_unlock_irq(&vt->Mlb_rq_spin);

	if (qid_tocancel) {
		taskq_cancel_id(Mlsas_Vmpt_async_tq, qid_tocancel);
		/* release reference held by Rx_Attach */
		__Mlsas_put_virt(vt);
	}
	if (to_remove_vmpt_scsih) 
		__Mlsas_Vmpt_remove_vmpt_device(vt);
}

module_param(Mlsas_Vmpt_new_virt_delay, int, 0644);
module_param(Mlsas_Vmpt_scsih_max_sector, int, 0644);
module_param(Mlsas_Vmpt_scsih_max_sg_tablesize, int, 0644);
