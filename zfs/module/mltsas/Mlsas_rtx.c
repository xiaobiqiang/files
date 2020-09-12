#include <sys/mltsas/mlt_sas.h>

#define Mlsas_Sig_Kill		SIGHUP

static void __Mlsas_Tx_Drain_RQ(Mlsas_pr_req_t *prr);
static int __Mlsas_Thread_Run_impl(Mlsas_thread_t *thi);
static void ____Mlsas_Thread_Stop(Mlsas_thread_t *thi, boolean_t wait);
static void __Mlsas_Setup_RWrsp_msg(Mlsas_pr_req_t *prr);
static void __Mlsas_Split_RTX(Mlsas_thread_t *thi, Mlsas_rtx_wq_t *wq, list_t *l);
static boolean_t __Mlsas_Dequeue_RTX(Mlsas_rtx_wq_t *wq, list_t *l);

void __Mlsas_Thread_Init(Mlsas_thread_t *thi,
	int (*fn) (Mlsas_thread_t *), 
	Mlsas_ttype_e type, const char *name)
{
	spin_lock_init(&thi->Mt_lock);
	thi->Mt_w    = NULL;
	thi->Mt_state = Mt_None;
	thi->Mt_fn = fn;
	thi->Mt_name = name;
	thi->Mt_type = type;
}

void __Mlsas_Thread_Start(Mlsas_thread_t *thi)
{
	unsigned long flags;
	struct task_struct *nt = NULL;

	spin_lock_irqsave(&thi->Mt_lock, flags);
	switch (thi->Mt_state) {
	case Mt_None:
		cmn_err(CE_NOTE, "%s Start Thread(%s).",
			__func__, thi->Mt_name);

		init_completion(&thi->Ml_stop);
		thi->Mt_state = Mt_Run;
		spin_unlock_irqrestore(&thi->Mt_lock, flags);
		
		flush_signals(current);
		
		VERIFY(!IS_ERR_OR_NULL(nt = kthread_create(
			__Mlsas_Thread_Run_impl,
			(void *)thi, thi->Mt_name)));
		thi->Mt_w = nt;
		wake_up_process(nt);
		wait_for_completion(&thi->Ml_stop);
		break;
	case Mt_Exit:
		thi->Mt_state = Mt_Restart;
		cmn_err(CE_NOTE, "%s Restart Thread(%s).",
			__func__, thi->Mt_name);
		/* FULLTHROUGH */
	case Mt_Run:
	case Mt_Restart:
	default:
		spin_unlock_irqrestore(&thi->Mt_lock, flags);
		break;
	}
}

void __Mlsas_Thread_Stop(Mlsas_thread_t *thi)
{
	____Mlsas_Thread_Stop(thi, B_TRUE);
}

void __Mlsas_Thread_Stop_nowait(Mlsas_thread_t *thi)
{
	____Mlsas_Thread_Stop(thi, B_FALSE);
}

static void ____Mlsas_Thread_Stop(Mlsas_thread_t *thi, boolean_t wait)
{
	unsigned long flags;

	spin_lock_irqsave(&thi->Mt_lock, flags);
	switch (thi->Mt_state) {
	case Mt_Restart:
	case Mt_Run:
		thi->Mt_state = Mt_Exit;
		smp_mb();
		init_completion(&thi->Ml_stop);
		if (thi->Mt_w && thi->Mt_w != current)
			force_sig(Mlsas_Sig_Kill, thi->Mt_w);
		/* FULLTHROUGH */
	case Mt_Exit:
	case Mt_None:
		spin_unlock_irqrestore(&thi->Mt_lock, flags);
		break;
	}

	if (wait)
		wait_for_completion(&thi->Ml_stop);
}

static int __Mlsas_Thread_Run_impl(Mlsas_thread_t *thi)
{
	int rval;
	unsigned long flags;

	if (!completion_done(&thi->Ml_stop))
		complete(&thi->Ml_stop);

again:
	rval = thi->Mt_fn(thi);
	
	spin_lock_irqsave(&thi->Mt_lock, flags);
	if (thi->Mt_state == Mt_Restart) {
		thi->Mt_state = Mt_Run;
		spin_unlock_irqrestore(&thi->Mt_lock, flags);
		cmn_err(CE_NOTE, "%s Restarting Thread(%s)",
			__func__, thi->Mt_name);
		goto again;
	}

	thi->Mt_state = Mt_None;
	thi->Mt_w = NULL;
	smp_mb();
	complete_all(&thi->Ml_stop);
	spin_unlock_irqrestore(&thi->Mt_lock, flags);

	cmn_err(CE_NOTE, "%s Thread(%s) Is Terminating.",
		__func__, thi->Mt_name);

	return rval;
}

Mlsas_tst_e __Mlsas_Thread_State(Mlsas_thread_t *thi)
{
	smp_rmb();
	return thi->Mt_state;
}

static inline unsigned long __Mlsas_RTX_rxfl_bfl(uint32_t rxfl)
{
	return	(rxfl & Mlsas_RXfl_Sync ? REQ_SYNC : 0) |
			(rxfl & Mlsas_RXfl_Fua ? REQ_FUA : 0) |
			(rxfl & Mlsas_RXfl_Flush ? REQ_FLUSH : 0) |
			(rxfl & Mlsas_RXfl_Disc ? REQ_DISCARD : 0) |
			(rxfl & Mlsas_RXfl_Write ? REQ_WRITE : 0);
}

static inline uint32_t __Mlsas_RTX_bfl_rxfl(unsigned long rw)
{
	return  (rw & REQ_SYNC ? Mlsas_RXfl_Sync : 0) |
			(rw & REQ_FUA ? Mlsas_RXfl_Fua : 0) |
			(rw & REQ_FLUSH ? Mlsas_RXfl_Flush : 0) |
			(rw & REQ_DISCARD ? Mlsas_RXfl_Disc : 0) |
			(rw & REQ_WRITE ? Mlsas_RXfl_Write : 0);
}

int __Mlsas_Tx_biow(Mlsas_rtx_wk_t *w)
{
	int rval = 0, what;
	Mlsas_request_t *rq = container_of(w, Mlsas_request_t, Mlrq_wk);
	Mlsas_rh_t *rh = rq->Mlrq_pr->Mlpd_rh;
	uint32_t rwfl;
	Mlsas_bio_and_error_t m;

/*	cmn_err(CE_NOTE, "%s master bio rw_flags(0x%llx)", __func__,
		rq->Mlrq_master_bio->bi_rw); */

	rwfl = __Mlsas_Setup_RWmsg(rq)->rw_flags;

	if (rwfl & REQ_DISCARD)
		rval = __Mlsas_TX(rh->Mh_session, 
			rq->Mlrq_bdev->Mlb_txbuf, 
			rq->Mlrq_bdev->Mlb_txbuf_used, 
			NULL, 0, Mlsas_TX_Type_Normal);
	else if (rwfl & REQ_WRITE)
		rval = __Mlsas_TX(rh->Mh_session, 
			rq->Mlrq_bdev->Mlb_txbuf, 
			rq->Mlrq_bdev->Mlb_txbuf_used, 
			rq->Mlrq_master_bio, 
			rq->Mlrq_bsize, Mlsas_TX_Type_Bio);
	else 
		cmn_err(CE_PANIC, "!(rwfl & (Mlsas_RXfl_Disc | Mlsas_RXfl_Write))");

	spin_lock_irq(&rq->Mlrq_bdev->Mlb_rq_spin);
	if (rq->Mlrq_pr->Mlpd_rh->Mh_state == Mlsas_RHS_New)
		what = rval ? Mlsas_Rst_Net_Send_Error : 
			Mlsas_Rst_Net_Send_OK;
	else
		what = Mlsas_Rst_Net_Send_Error;
	rq->Mlrq_back_bio = (what == Mlsas_Rst_Net_Send_Error) ?
		ERR_PTR(-EIO) : NULL;
	__Mlsas_Req_Stmt(rq, what, &m);
	spin_unlock_irq(&rq->Mlrq_bdev->Mlb_rq_spin);

	if (m.Mlbi_bio)
		__Mlsas_Complete_Master_Bio(rq, &m);

	return (0);
}

Mlsas_RW_msg_t *__Mlsas_Setup_RWmsg(Mlsas_request_t *rq)
{
	Mlsas_blkdev_t *Mlb = rq->Mlrq_bdev;
	Mlsas_Msh_t *mms = rq->Mlrq_bdev->Mlb_txbuf;
	Mlsas_RW_msg_t *rwm = mms + 1;

	mms->Mms_ck = Mlsas_Mms_Magic;
	mms->Mms_hashkey = Mlb->Mlb_hashkey;
	mms->Mms_type = Mlsas_Mms_Bio_RW;
	mms->Mms_len = sizeof(Mlsas_Msh_t) + 
		((sizeof(Mlsas_RW_msg_t) + 7) & ~7);

//	rwm->rw_flags = __Mlsas_RTX_bfl_rxfl(rq->Mlrq_master_bio->bi_rw);
	rwm->rw_flags = rq->Mlrq_master_bio->bi_rw;
	rwm->rw_sector = rq->Mlrq_sector;
	rwm->rw_rqid = (uint64_t)rq;
	rwm->rw_mlbpr = rq->Mlrq_pr->Mlpd_pr;

	if ((rwm->rw_flags & REQ_DISCARD) ||
		!(rwm->rw_flags & REQ_WRITE))
		((Mlsas_Disc_msg_t *)rwm)->t_size = rq->Mlrq_bsize;

	VERIFY((Mlb->Mlb_txbuf_used = mms->Mms_len) <= Mlb->Mlb_txbuf_len);

	return rwm;
}

int __Mlsas_Tx_bior(Mlsas_rtx_wk_t *w)
{
	int rval = 0, what;
	Mlsas_request_t *rq = container_of(w, Mlsas_request_t, Mlrq_wk);
	Mlsas_rh_t *rh = rq->Mlrq_pr->Mlpd_rh;
	Mlsas_bio_and_error_t m;

	__Mlsas_Setup_RWmsg(rq);

	rval = __Mlsas_TX(rh->Mh_session, 
		rq->Mlrq_bdev->Mlb_txbuf, 
		rq->Mlrq_bdev->Mlb_txbuf_used, 
		NULL, 0, Mlsas_TX_Type_Normal);

	spin_lock_irq(&rq->Mlrq_bdev->Mlb_rq_spin);
	if (rq->Mlrq_pr->Mlpd_rh->Mh_state == Mlsas_RHS_New)
		what = rval ? Mlsas_Rst_Net_Send_Error : 
			Mlsas_Rst_Net_Send_OK;
	else
		what = Mlsas_Rst_Net_Send_Error;
	rq->Mlrq_back_bio = (what == Mlsas_Rst_Net_Send_Error) ?
		ERR_PTR(-EIO) : NULL;
	__Mlsas_Req_Stmt(rq, what, &m);
	spin_unlock_irq(&rq->Mlrq_bdev->Mlb_rq_spin);
	
	if (m.Mlbi_bio)
		__Mlsas_Complete_Master_Bio(rq, &m);

	return (0);
}

int __Mlsas_Rx_biow(Mlsas_rtx_wk_t *w)
{
	cs_rx_data_t *xd = container_of(w, cs_rx_data_t, node);
	Mlsas_RW_msg_t *Wm = ((Mlsas_Msh_t *)xd->ex_head) + 1;
	Mlsas_blkdev_t *Mlb = ((Mlsas_pr_device_t *)Wm->rw_mlbpr)->Mlpd_mlb;
	uint32_t bsize = xd->data_len;
	Mlsas_pr_req_t *prr = NULL;

	if (Wm->rw_flags & REQ_DISCARD)
		bsize = ((Mlsas_Disc_msg_t *)Wm)->t_size;
	
	VERIFY((prr = __Mlsas_Alloc_PR_RQ(Wm->rw_mlbpr, 
			Wm->rw_rqid, Wm->rw_sector, 
			bsize)) != NULL);	
	prr->prr_flags |= Mlsas_PRRfl_Write;
	
	if (!(Wm->rw_flags & REQ_DISCARD)) {
		prr->prr_flags |= Mlsas_PRRfl_Addl_Kmem;
		prr->prr_dt = xd->data;
		prr->prr_dtlen = xd->data_len;
		xd->data = NULL;
		xd->data_len = 0;
	} else if (blk_queue_discard(
			bdev_get_queue(Mlb->Mlb_bdi.Mlbd_bdev))) {
		prr->prr_flags |= Mlsas_PRRfl_Zero_Out;
		VERIFY(prr->prr_bsize > 0);
	}
	
	spin_lock_irq(&Mlb->Mlb_rq_spin);
	if (!__Mlsas_Get_ldev_if_state(Mlb, Mlsas_Devst_Attached)) {
		__Mlsas_Tx_Drain_RQ(prr);
		spin_unlock_irq(&Mlb->Mlb_rq_spin);
		return (0);
	}
	__Mlsas_PR_RQ_stmt(prr, Mlsas_PRRst_Submit_Local, NULL);
	spin_unlock_irq(&Mlb->Mlb_rq_spin);
	
	__Mlsas_Submit_PR_request(Mlb, Wm->rw_flags, prr);

	__Mlsas_clustersan_rx_data_free_ext(xd);
	return (0);
}

static void __Mlsas_Tx_Drain_RQ(Mlsas_pr_req_t *prr)
{
	uint32_t what;
	Mlsas_pr_device_t *pr = prr->prr_pr;
	Mlsas_pr_req_free_t fr = {.k_put = 0};
	
	prr->prr_flags |= Mlsas_PRRfl_Ee_Error;
	prr->prr_error = -ENODEV;

	if (pr->Mlpd_rh->Mh_state == Mlsas_RHS_Corrupt) 
		fr.k_put = 1;
	else if (pr->Mlpd_rh->Mh_state == Mlsas_RHS_New) {
		what = (prr->prr_flags & Mlsas_PRRfl_Write) ? 
			Mlsas_PRRst_Queue_Net_Wrsp :
			Mlsas_PRRst_Queue_Net_Rrsp;
		__Mlsas_PR_RQ_stmt(prr, Mlsas_PRRst_Subimit_Net, NULL);
		__Mlsas_PR_RQ_stmt(prr, what, NULL);
	}

	if (fr.k_put)
		__Mlsas_sub_PR_RQ(prr, fr.k_put);
}

int __Mlsas_Rx_bior(Mlsas_rtx_wk_t *w)
{
	cs_rx_data_t *xd = container_of(w, cs_rx_data_t, node);
	Mlsas_Disc_msg_t *Rm = ((Mlsas_Msh_t *)xd->ex_head) + 1;
	Mlsas_blkdev_t *Mlb = ((Mlsas_pr_device_t *)Rm->rw_mlbpr)->Mlpd_mlb;
	unsigned long rw = Rm->rw_flags;
	Mlsas_pr_req_t *prr = NULL;
	
	VERIFY((prr = __Mlsas_Alloc_PR_RQ(Rm->rw_mlbpr, 
			Rm->rw_rqid, Rm->rw_sector, 
			Rm->t_size)) != NULL);	

	if (Rm->t_size) {
		prr->prr_flags |= Mlsas_PRRfl_Addl_Kmem;
		prr->prr_dtlen = Rm->t_size;
		VERIFY(prr->prr_dt = __Mlsas_clustersan_kmem_alloc(prr->prr_dtlen));
	}
	
	spin_lock_irq(&Mlb->Mlb_rq_spin);
	if (!__Mlsas_Get_ldev_if_state(Mlb, Mlsas_Devst_Attached)) {
		__Mlsas_Tx_Drain_RQ(prr);
		spin_unlock_irq(&Mlb->Mlb_rq_spin);
		return (0);
	}
	__Mlsas_PR_RQ_stmt(prr, Mlsas_PRRst_Submit_Local, NULL);
	spin_unlock_irq(&Mlb->Mlb_rq_spin);
	
	__Mlsas_Submit_PR_request(Mlb, Rm->rw_flags, prr);

	__Mlsas_clustersan_rx_data_free_ext(xd);
	return (0);
}

int __Mlsas_Tx_PR_RQ_rsp(Mlsas_rtx_wk_t *w)
{
	int rval = 0, what;
	Mlsas_pr_req_t *prr = container_of(w, Mlsas_pr_req_t, prr_wk);
	Mlsas_pr_device_t *pr = prr->prr_pr;
	Mlsas_pr_req_free_t fr;
	
	__Mlsas_Setup_RWrsp_msg(prr);

/*	if (!prr->prr_dt || !prr->prr_dtlen)
		cmn_err(CE_NOTE, "Zero_PRR_DATA, sector(%llu) size(%u) "
			"flags(%x) prr_pr_rq(%p) error(%d)", prr->prr_bsector,
			prr->prr_bsize, prr->prr_flags, prr->prr_pr_rq,
			prr->prr_error); */

	if (prr->prr_flags & Mlsas_PRRfl_Write) {
		rval = __Mlsas_TX(pr->Mlpd_rh->Mh_session, 
			pr->Mlpd_mlb->Mlb_astx_buf,
			pr->Mlpd_mlb->Mlb_astxbuf_used,
			NULL, 0, Mlsas_TX_Type_Normal);
		if (prr->prr_flags & Mlsas_PRRfl_Addl_Kmem) {
			__Mlsas_clustersan_kmem_free(prr->prr_dt, prr->prr_dtlen);
			prr->prr_dt = NULL;
			prr->prr_dtlen = 0;
		}
	}
	else 
		rval = __Mlsas_TX(pr->Mlpd_rh->Mh_session, 
			pr->Mlpd_mlb->Mlb_astx_buf,
			pr->Mlpd_mlb->Mlb_astxbuf_used,
			prr->prr_dt, prr->prr_dtlen, 
			Mlsas_TX_Type_Normal);

	what = rval ? Mlsas_PRRst_Send_Error :
		Mlsas_PRRst_Send_OK;
	spin_lock_irq(&pr->Mlpd_mlb->Mlb_rq_spin);
	__Mlsas_PR_RQ_stmt(prr, what, &fr);
	spin_unlock_irq(&pr->Mlpd_mlb->Mlb_rq_spin);

	if (fr.k_put)
		__Mlsas_sub_PR_RQ(prr, fr.k_put);
	
	return (0);
}

static void __Mlsas_Setup_RWrsp_msg(Mlsas_pr_req_t *prr)
{
	Mlsas_blkdev_t *Mlb = prr->prr_pr->Mlpd_mlb;
	Mlsas_Msh_t *mms = Mlb->Mlb_astx_buf;
	Mlsas_RWrsp_msg_t *rsp = mms + 1;

	rsp->rsp_error = 0;
	if (prr->prr_flags & Mlsas_PRRfl_Ee_Error)
		rsp->rsp_error = prr->prr_error;
	rsp->rsp_rqid = prr->prr_pr_rq;

	mms->Mms_ck = Mlsas_Mms_Magic;
	mms->Mms_hashkey = Mlb->Mlb_hashkey;
	mms->Mms_len = sizeof(Mlsas_Msh_t) + 
		((sizeof(Mlsas_RWrsp_msg_t) + 7) & ~7);
	mms->Mms_type = Mlsas_Mms_Brw_Rsp;

	Mlb->Mlb_astxbuf_used = mms->Mms_len;
}

void __Mlsas_Queue_RTX(Mlsas_rtx_wq_t *wq, Mlsas_rtx_wk_t *wk)
{
	unsigned long flags;
	
	spin_lock_irqsave(&wq->rtq_lock, flags);
	list_insert_tail(&wq->rtq_list, wk);
	spin_unlock_irqrestore(&wq->rtq_lock, flags);
	
	wake_up(&wq->rtq_wake);
}

int __Mlsas_RTx(Mlsas_thread_t *thi)
{
	list_t wl;
	Mlsas_blkdev_t *Mlb = NULL;
	Mlsas_rtx_wq_t *wq = NULL;
	Mlsas_rtx_wk_t *w = NULL;
	Mlsas_t *Ml = NULL;
	
	list_create(&wl, sizeof(Mlsas_rtx_wk_t),
		offsetof(Mlsas_rtx_wk_t, rtw_node));

	switch (thi->Mt_type) {
	case Mtt_Tx:
		Mlb = container_of(thi, Mlsas_blkdev_t, Mlb_tx);
		wq = &Mlb->Mlb_tx_wq;
		break;
	case Mtt_Rx:
		Mlb = container_of(thi, Mlsas_blkdev_t, Mlb_rx);
		wq = &Mlb->Mlb_rx_wq;
		break;
	case Mtt_Asender:
		Mlb = container_of(thi, Mlsas_blkdev_t, Mlb_asender);
		wq = &Mlb->Mlb_asender_wq;
		break;
	case Mtt_Sender:
		Mlb = container_of(thi, Mlsas_blkdev_t, Mlb_sender);
		wq = &Mlb->Mlb_sender_wq;
		break;
	case Mtt_WatchDog:
		Ml = container_of(thi, Mlsas_t, Ml_wd);
		wq = &Ml->Ml_wd_wq;
		break;
	case Mtt_Link_Evt_HDL:
		Ml = container_of(thi, Mlsas_t, Ml_link_evt_hdl);
		wq = &Ml->Ml_link_evt_hdl_wq;
		break;
	default:
		cmn_err(CE_NOTE, "%s Wrong Mlsas Thread Type(%02x)",
			__func__, thi->Mt_type);
		break;
	}

	if ((!Mlb && !Ml) || !wq)
		return -EINVAL;
	
	while (__Mlsas_Thread_State(thi) == Mt_Run) {
		if (list_is_empty(&wl))
			__Mlsas_Split_RTX(thi, wq, &wl);
		/* maybe shall exit */
		if (signal_pending(current)) {
			flush_signals(current);
			if (__Mlsas_Thread_State(thi) == Mt_Run)
				continue;
			break;
		} else if (__Mlsas_Thread_State(thi) != Mt_Run)
			break;

		if (!list_is_empty(&wl)) {
			w = list_remove_head(&wl);
			w->rtw_fn(w);
		}
	}

	do {
		if (!list_is_empty(&wl)) {
			w = list_remove_head(&wl);
			w->rtw_fn(w);
		} else 
			__Mlsas_Dequeue_RTX(wq, &wl);
	} while (!list_is_empty(&wl));

	__Mlsas_put_virt(Mlb);

	return (0);
}

void __Mlsas_RTx_Init_WQ(Mlsas_rtx_wq_t *wq)
{
	spin_lock_init(&wq->rtq_lock);
	init_waitqueue_head(&wq->rtq_wake);
	list_create(&wq->rtq_list, sizeof(Mlsas_rtx_wk_t),
		offsetof(Mlsas_rtx_wk_t, rtw_node));
}

static boolean_t __Mlsas_Dequeue_RTX(Mlsas_rtx_wq_t *wq, list_t *l)
{
	spin_lock_irq(&wq->rtq_lock);
	list_move_tail(l, &wq->rtq_list);
	spin_unlock_irq(&wq->rtq_lock);

	return !list_is_empty(l);
}

static void __Mlsas_Split_RTX(Mlsas_thread_t *thi, 
		Mlsas_rtx_wq_t *wq, list_t *l)
{
	DEFINE_WAIT(wait);

	if (__Mlsas_Dequeue_RTX(wq, l))
		return ;

	for ( ; ; ) {
		prepare_to_wait(&wq->rtq_wake, &wait, TASK_INTERRUPTIBLE);
		if (__Mlsas_Dequeue_RTX(wq, l) ||
			signal_pending(current) ||
			__Mlsas_Thread_State(thi) != Mt_Run)
			break;
		schedule();
	}
	finish_wait(&wq->rtq_wake, &wait);
}