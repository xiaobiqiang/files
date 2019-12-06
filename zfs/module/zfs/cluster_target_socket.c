#include <sys/ddi.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/netdevice.h>
#include <linux/spinlock.h>
#include <linux/in.h>  
#include <linux/inet.h>  
#include <linux/socket.h>  
#include <uapi/linux/tcp.h>
#include <net/sock.h>
#include <sys/taskq.h>
#include <sys/list.h>
#include <sys/cluster_san.h>
#include <sys/cluster_target_socket.h>
#include <sys/fs/zfs.h>

typedef struct cluster_target_socket_session_sm_ctx {
	cluster_target_session_socket_t *smc_tsso;
	cluster_target_socket_session_event_e smc_event;
	uintptr_t smc_extinfo;
	char smc_tag[128];
} cluster_target_socket_session_sm_ctx_t;

typedef struct cluster_tso_global_s {
	krwlock_t ctso_rw;
	list_t ctso_tsso_list;
	list_t ctso_tpso_list;
	kmem_cache_t *ctso_rx_data_cache;
	taskq_t *ctso_refasync_taskq;
} cts_global_t;	

static cts_global_t ctso_global;
static cts_global_t *ctso = &ctso_global;
static uint32_t cluster_target_socket_port_rx_process_nworker = 16;

static cluster_status_t
cluster_target_socket_session_rx_iov(struct socket *so, struct kvec *kvp,
        uint32_t kvlen, uint32_t totlen);
static void
cluster_target_socket_session_sm_event(cluster_target_session_socket_t *tsso,
        cluster_target_socket_session_event_e event, uintptr_t extinfo, void *tag);

void 
cluster_target_socket_refcnt_reset(cluster_target_socket_refcnt_t *refcnt)
{
	refcnt->tr_nref = 0;
	refcnt->tr_wait = CTSO_REF_NOWAIT;
}

void 
cluster_target_socket_refcnt_init(cluster_target_socket_refcnt_t *refcnt, void *obj)
{
	bzero(refcnt, sizeof(cluster_target_socket_refcnt_t));
	refcnt->tr_refcnt_obj = obj;
	cluster_target_socket_refcnt_reset(refcnt);
	mutex_init(&refcnt->tr_mtx, NULL, MUTEX_DEFAULT, NULL);
	cv_init(&refcnt->tr_cv, NULL, CV_DRIVER, NULL);
	refcnt->tr_audit.adt_max_depth = CTSO_REFAUDIT_MAX_RECORD - 1;
}

void 
cluster_target_socket_refcnt_destroy(cluster_target_socket_refcnt_t *refcnt)
{
	mutex_enter(&refcnt->tr_mtx);
	ctso_assert(refcnt->tr_nref == 0);
	mutex_exit(&refcnt->tr_mtx);
	mutex_destroy(&refcnt->tr_mtx);
	cv_destroy(&refcnt->tr_cv);
}

void 
cluster_target_socket_refcnt_async_unref(cluster_target_socket_refcnt_t *refcnt)
{
	refcnt->tr_fn(refcnt->tr_refcnt_obj);
}

void 
cluster_target_socket_refcnt_hold(cluster_target_socket_refcnt_t *refcnt, void *ftag)
{
	mutex_enter(&refcnt->tr_mtx);
	ctso_assert(refcnt->tr_wait == CTSO_REF_NOWAIT);
	refcnt->tr_nref++;
	CTSO_REFAUDIT(refcnt, ftag, "hold");
	mutex_exit(&refcnt->tr_mtx);
}

void 
cluster_target_socket_refcnt_rele(cluster_target_socket_refcnt_t *refcnt, void *ftag)
{
	mutex_enter(&refcnt->tr_mtx);
	ctso_assert(--refcnt->tr_nref >= 0);
	CTSO_REFAUDIT(refcnt, ftag, "rele");
	
	if (refcnt->tr_nref == 0) {
		/*
		 * someone wait for refcnt->tr_cv there
		 */
		if (refcnt->tr_wait == CTSO_REF_WAIT_SYNC)
			cv_broadcast(&refcnt->tr_cv);
		/*
		 * someone assign a callback function and wait rele async.
		 */
		else if (refcnt->tr_wait == CTSO_REF_WAIT_ASYNC) {
			ctso_assert(refcnt->tr_fn != NULL);
			if (taskq_dispatch(ctso->ctso_refasync_taskq, 
					cluster_target_socket_refcnt_async_unref, 
					refcnt, TQ_SLEEP) == NULL)
				cmn_err(CE_WARN, "%s unref task dispatch failed, fn_tag[%s]",
					__func__, (char *)ftag);
		}
	}
	mutex_exit(&refcnt->tr_mtx);
}

void 
cluster_target_socket_refcnt_rele_and_destroy(
	cluster_target_socket_refcnt_t *refcnt, 
	void *ftag, cluster_target_socket_refcnt_cb_t *fn)
{
	mutex_enter(&refcnt->tr_mtx);
	ctso_assert(--refcnt->tr_nref >= 0);
	CTSO_REFAUDIT(refcnt, ftag, "rele_and_destroy");

	if (refcnt->tr_nref == 0) {
		refcnt->tr_fn = fn;
		refcnt->tr_wait = CTSO_REF_WAIT_ASYNC;
		if (taskq_dispatch(ctso->ctso_refasync_taskq, 
				cluster_target_socket_refcnt_async_unref, 
				refcnt, TQ_SLEEP) == NULL)
			cmn_err(CE_WARN, "%s unref task dispatch failed, fn_tag[%s]",
				__func__, (char *)ftag);
	}
	mutex_exit(&refcnt->tr_mtx);
}

void 
cluster_target_socket_refcnt_wait_ref(cluster_target_socket_refcnt_t *refcnt, void *ftag)
{
	mutex_enter(&refcnt->tr_mtx);
	CTSO_REFAUDIT(refcnt, ftag, "sync_wait");
	
	refcnt->tr_wait= CTSO_REF_WAIT_SYNC;
	while (refcnt->tr_nref != 0)
		cv_wait(&refcnt->tr_cv, &refcnt->tr_mtx);
	mutex_exit(&refcnt->tr_mtx);
}

void 
cluster_target_socket_refcnt_wait_ref_async(
	cluster_target_socket_refcnt_t *refcnt, 
	void *ftag, cluster_target_socket_refcnt_cb_t cb)
{
	mutex_enter(&refcnt->tr_mtx);
	CTSO_REFAUDIT(refcnt, ftag, "async_wait");
	refcnt->tr_wait= CTSO_REF_WAIT_ASYNC;
	refcnt->tr_fn = cb;

	if (refcnt->tr_nref == 0) {
		if (taskq_dispatch(ctso->ctso_refasync_taskq, 
				cluster_target_socket_refcnt_async_unref, 
				refcnt, TQ_SLEEP) == NULL)
			cmn_err(CE_WARN, "%s unref task dispatch failed, fn_tag[%s]",
				__func__, (char *)ftag);
	}
	mutex_exit(&refcnt->tr_mtx);
}


cluster_status_t
cluster_target_socket_create(int family, int type, int prot, struct socket **skp)
{
	return sock_create_kern(&init_net, family, type, prot, skp);
}

static void
cluster_target_socket_destroy(struct socket *so)
{
	sock_release(so);
}

static void
cluster_target_socket_shutdown(struct socket *so)
{
	kernel_sock_shutdown(so, SHUT_RDWR);
}

cluster_status_t
cluster_target_socket_setsockopt(struct socket *so)
{
	cluster_status_t rval;
	int sndbuf = 256 * 1024, rcvbuf = 256 *1024;
	int on = 1, off = 0;
	
	if (((rval = kernel_setsockopt(so, SOL_SOCKET, SO_REUSEADDR, 
					&on, sizeof(on))) != CLUSTER_STATUS_SUCCESS) || 
		((rval = kernel_setsockopt(so, SOL_SOCKET, SO_REUSEPORT, 
					&on, sizeof(on))) != CLUSTER_STATUS_SUCCESS) ||
		((rval = kernel_setsockopt(so, SOL_SOCKET, SO_SNDBUF, 
					&sndbuf, sizeof(sndbuf))) != CLUSTER_STATUS_SUCCESS) ||
		((rval = kernel_setsockopt(so, SOL_SOCKET, SO_RCVBUF, 
					&rcvbuf, sizeof(rcvbuf))) != CLUSTER_STATUS_SUCCESS) ||
		((rval = kernel_setsockopt(so, SOL_TCP, TCP_NODELAY, 
					&on, sizeof(on))) != CLUSTER_STATUS_SUCCESS))
		return rval;
	return CLUSTER_STATUS_SUCCESS;
}	


static void
cluster_target_socket_session_hold(cluster_target_session_socket_t *tsso, void *tag)
{
	cluster_target_socket_refcnt_hold(&tsso->tsso_refcnt, tag);
}

static void
cluster_target_socket_session_rele(cluster_target_session_socket_t *tsso, void *tag)
{
	cluster_target_socket_refcnt_rele(&tsso->tsso_refcnt, tag);
}

static void
cluster_target_socket_session_new(cluster_target_port_socket_t *tpso,
		cluster_target_session_t *cts, struct socket *so,
		cluster_target_session_socket_t **tssop)
{
	int socklen = 0;
	struct sockaddr addr_in4;
	cluster_target_session_socket_t *tsso;

	bzero(&addr_in4, sizeof(struct sockaddr_in));
	kernel_getpeername(so, &addr_in4, &socklen);
	tsso = kmem_zalloc(sizeof(cluster_target_session_socket_t), KM_SLEEP);
	tsso->tsso_cts = cts;
	tsso->tsso_tpso = tpso;
	tsso->tsso_so = so;
	snprintf(tsso->tsso_ipaddr, 16, "%d.%d.%d.%d",
                addr_in4.sa_data[2], addr_in4.sa_data[3],
                addr_in4.sa_data[4], addr_in4.sa_data[5]);
	cmn_err(CE_NOTE, "%s new session ip(%s)", __func__, tsso->tsso_ipaddr);
	cluster_target_socket_refcnt_init(&tsso->tsso_refcnt, tsso);
	mutex_init(&tsso->tsso_rx_mtx, NULL, MUTEX_DEFAULT, NULL);
	cv_init(&tsso->tsso_rx_cv, NULL, CV_DRIVER, NULL);

	*tssop = tsso;
}

static void
cluster_target_socket_session_free(cluster_target_session_socket_t *tsso)
{
	cluster_target_socket_refcnt_destroy(&tsso->tsso_refcnt);
	kmem_free(tsso, sizeof(cluster_target_session_socket_t));
}

static void
cluster_target_socket_session_sminit(cluster_target_session_socket_t *tsso)
{
	char sm_tq_name[64] = {0};
	cluster_target_socket_sm_audit_t *sm_audit = &tsso->tsso_sm_audit;
	
	mutex_init(&tsso->tsso_sm_mtx, NULL, MUTEX_DEFAULT, NULL);
	cv_init(&tsso->tsso_sm_cv, NULL, CV_DRIVER, NULL);
	snprintf(sm_tq_name, 64, "tsso_sm_%p", tsso);
	tsso->tsso_sm_ctx = taskq_create(sm_tq_name, 1, minclsyspri, 
			8, 16384, TASKQ_PREPOPULATE);

	tsso->tsso_last_state = SOS_S1_FREE;
	tsso->tsso_curr_state = SOS_S1_FREE;
	sm_audit->sa_max_depth = TSSO_SM_AUDIT_DEPTH - 1;
}

static void
cluster_target_socket_session_smfini(cluster_target_session_socket_t *tsso)
{
	if (tsso->tsso_sm_ctx == NULL)
		return ;

	mutex_destroy(&tsso->tsso_sm_mtx);
	taskq_destroy(tsso->tsso_sm_ctx);
	tsso->tsso_sm_ctx = NULL;
}

static void
cluster_target_socket_session_connect_timedout(cluster_target_session_socket_t *tsso)
{
	/*
	 * wake up kernel_connect
	 */
	cluster_target_socket_shutdown(tsso->tsso_so);
}

static cluster_status_t
cluster_target_socket_session_rx_buf(struct socket *so, void *buf, uint32_t buflen)
{
	struct kvec kv;

	if (!buf || !buflen)
		return CLUSTER_STATUS_SUCCESS;

	kv.iov_base = buf;
	kv.iov_len = buflen;
	return cluster_target_socket_session_rx_iov(so, &kv, 1, buflen);
}

static cluster_status_t
cluster_target_socket_session_rx_iov(struct socket *so, struct kvec *kvp, 
	uint32_t kvlen, uint32_t totlen)
{
	cluster_status_t rval;
	struct msghdr mhdr;
	int flags = MSG_WAITALL | MSG_NOSIGNAL;
	
	bzero(&mhdr, sizeof(struct msghdr));
	mhdr.msg_flags = flags;

	if ((rval = kernel_recvmsg(so, &mhdr, kvp, kvlen, 
			totlen, flags)) < totlen) {
		if (rval == 0)	/* broken pipe */
			return EPIPE;
		if (rval > 0)	/* IO error */
			return EIO;
		return rval;
	}
	return CLUSTER_STATUS_SUCCESS;
}

static void
cluster_target_socket_session_rx_new_csdata(
	cluster_target_session_socket_t *tsso,
	cluster_target_msg_header_t *hdrp,
	cs_rx_data_t **cs_datap)
{
	cs_rx_data_t *cs_data;
	cluster_san_hostinfo_t *cshi = tsso->tsso_cts->sess_host_private;

	cs_data = cts_rx_data_alloc(hdrp->total_len);
	cs_data->data_index = hdrp->index;
	cs_data->msg_type = hdrp->msg_type;
	cs_data->cs_private = cshi;
	cs_data->need_reply = B_FALSE;
	cs_data->ex_len = hdrp->ex_len;
	cs_data->ex_head = kmem_alloc(hdrp->ex_len, KM_SLEEP);
	cluster_san_hostinfo_hold(cshi);
	*cs_datap = cs_data;
}

static void
cluster_target_socket_session_rx_process(
	cluster_target_session_socket_t *tsso, cs_rx_data_t *cs_data)
{
	uint32_t worker_idx;
	cluster_target_port_socket_t *tpso = tsso->tsso_tpso;
	cluster_target_socket_worker_t *worker;

	cluster_target_socket_session_hold(tsso, CTSO_FTAG);

	worker_idx = cs_data->data_index & tpso->tpso_rx_process_nthread;
	worker = tpso->tpso_rx_process_ctx + worker_idx;
	mutex_enter(&worker->worker_mtx);
	if (!worker->worker_running) {
		mutex_exit(&worker->worker_mtx);
		cluster_target_socket_session_rele(tsso, CTSO_FTAG);
		csh_rx_data_free_ext(cs_data);
		return ;
	}

	list_insert_tail(worker->worker_list_w, cs_data);
	cv_signal(&worker->worker_cv);
	mutex_exit(&worker->worker_mtx);
}

static void
cluster_target_socket_session_rx(cluster_target_session_socket_t *tsso)
{
	cluster_status_t rval;
	cs_rx_data_t *cs_data = NULL;
	cluster_target_msg_header_t msghdr;
	boolean_t conn_break = B_FALSE;

	cmn_err(CE_NOTE, "session(%p) rx thread is running", tsso);
	
	mutex_enter(&tsso->tsso_rx_mtx);
	tsso->tsso_rx_running = B_TRUE;
	cv_signal(&tsso->tsso_rx_cv);

	while (tsso->tsso_rx_running) {
		mutex_exit(&tsso->tsso_rx_mtx);

		if ((rval = cluster_target_socket_session_rx_buf(tsso->tsso_so,
				&msghdr, sizeof(msghdr))) != CLUSTER_STATUS_SUCCESS) {
			mutex_enter(&tsso->tsso_rx_mtx);
			tsso->tsso_rx_running = B_FALSE;
			conn_break = B_TRUE;
			continue;
		}
		
		cluster_target_socket_session_rx_new_csdata(tsso,
				&msghdr, &cs_data);
		if (((rval = cluster_target_socket_session_rx_buf(tsso->tsso_so,
				cs_data->ex_head, cs_data->ex_len)) != CLUSTER_STATUS_SUCCESS) ||
			((rval = cluster_target_socket_session_rx_buf(tsso->tsso_so,
				cs_data->data, cs_data->data_len)) != CLUSTER_STATUS_SUCCESS)) {
			csh_rx_data_free_ext(cs_data);
			mutex_enter(&tsso->tsso_rx_mtx);
			tsso->tsso_rx_running = B_FALSE;
			conn_break = B_TRUE;
			continue;
		}
		
		cluster_target_socket_session_rx_process(tsso, cs_data);
		mutex_enter(&tsso->tsso_rx_mtx);
	}
}


static void
cluster_target_socket_session_connect_finish(cluster_target_session_socket_t *tsso)
{
	/* tx and rx thread */
	list_create(&tsso->tsso_rx_data_list, sizeof(cs_rx_data_t),
		offsetof(cs_rx_data_t, node));
	cluster_target_socket_session_hold(tsso, CTSO_FTAG);
	tsso->tsso_rx = kthread_run(cluster_target_socket_session_rx,
		tsso, "tsso_rx_%p", tsso);
	mutex_enter(&tsso->tsso_rx_mtx);
	while (!tsso->tsso_rx_running)
		cv_wait(&tsso->tsso_rx_cv, &tsso->tsso_rx_mtx);
	mutex_exit(&tsso->tsso_rx_mtx);
	/*
	 * we used cts->sess_tran_worker for tx, so we needn't
	 * tx thread anymore.
	 */
}

static void
cluster_target_socket_session_new_state(cluster_target_session_socket_t *tsso,
	cluster_target_socket_session_state_e new_state, 
	cluster_target_socket_session_sm_ctx_t *smc)
{
	cluster_status_t rval;
	struct sockaddr_in addr_in4;
	cluster_target_session_socket_t *iter_tsso;

	cmn_err(CE_NOTE, "tsso state(%d --> %d --> %d) cause(%d)",
		tsso->tsso_last_state, tsso->tsso_curr_state, 
		new_state, smc->smc_event);

	mutex_enter(&tsso->tsso_sm_mtx);
	CTSO_SESS_SM_AUDIT(tsso, smc->smc_event, smc->smc_tag);
	tsso->tsso_last_state = tsso->tsso_curr_state;
	tsso->tsso_curr_state = new_state;
	mutex_exit(&tsso->tsso_sm_mtx);

	switch (tsso->tsso_curr_state) {
		case SOS_S1_FREE:
			break;
		case SOS_S2_XPRT_WAIT:
			rw_enter(&ctso->ctso_rw, RW_WRITER);
			for (iter_tsso = list_head(&ctso->ctso_tsso_list); iter_tsso;
				iter_tsso = list_next(&ctso->ctso_tsso_list, iter_tsso)) {
				if (strcmp(tsso->tsso_ipaddr, iter_tsso->tsso_ipaddr) == 0)
					break;
			}

			if (iter_tsso != NULL)
				cluster_target_socket_session_sm_event(tsso, 
					SOE_SESS_CONFLICT, NULL, CTSO_FTAG);
			else {
				cluster_target_socket_session_hold(tsso, CTSO_FTAG);
				list_insert_tail(&tsso->tsso_tpso->tpso_sess_list, tsso);
				list_insert_tail(&ctso->ctso_tsso_list, tsso);
			}
			rw_exit(&ctso->ctso_rw);
			break;
		case SOS_S3_CONN_SND:
			bzero(&addr_in4, sizeof(struct sockaddr_in));
			addr_in4.sin_family = AF_INET;
			addr_in4.sin_port = htons(0);
			addr_in4.sin_addr.s_addr = in_aton(tsso->tsso_local_ip);
			if ((rval = kernel_bind(tsso->tsso_so, &addr_in4,
					sizeof(struct sockaddr_in))) != 0) {
				cmn_err(CE_NOTE, "%s kernel bind ip(%s) error(%d)",
					__func__, tsso->tsso_local_ip, rval);

				tsso->tsso_connect_status = rval;
				cluster_target_socket_session_sm_event(tsso,
					SOE_CN_BIND_ERROR, NULL, CTSO_FTAG);
				break;
			}

			bzero(&addr_in4, sizeof(struct sockaddr_in));
			addr_in4.sin_family = AF_INET;
			addr_in4.sin_port = htons(tsso->tsso_port);
			addr_in4.sin_addr.s_addr = in_aton(tsso->tsso_ipaddr);
			tsso->tsso_conn_tmhdl = timeout(
				cluster_target_socket_session_connect_timedout,
				tsso, SEC_TO_TICK(3));
			if ((rval = kernel_connect(tsso->tsso_so, &addr_in4,
					sizeof(struct sockaddr_in), 0)) != 0) {
				cmn_err(CE_NOTE, "%s connect ip(%s) error(%d)",
					__func__, tsso->tsso_ipaddr, rval);

				tsso->tsso_connect_status = rval;
				cluster_target_socket_session_sm_event(tsso,
					SOE_CONNECT_ERROR, NULL, CTSO_FTAG);
				break;
			}
			cluster_target_socket_session_sm_event(tsso,
					SOE_CONNECT_SUCCESS, NULL, CTSO_FTAG);
			break;
		case SOS_S4_XPRT_UP:
			cv_signal(&tsso->tsso_sm_cv);
			cmn_err(CE_NOTE, "%s session ip(%s) come into SOS_S4_XPRT_UP",
				__func__, tsso->tsso_ipaddr);
			cluster_target_socket_session_connect_finish(tsso);
			taskq_dispatch(clustersan->cs_async_taskq, 
				cts_link_down_to_up_handle, tsso->tsso_cts, TQ_SLEEP);
			break;
	}
}

static void
cluster_target_socket_session_s1_free(cluster_target_session_socket_t *tsso,
	cluster_target_socket_session_sm_ctx_t *smc)
{
	switch (smc->smc_event) {
		case SOE_RX_CONNECT:
			cluster_target_socket_session_new_state(tsso, 
				SOS_S2_XPRT_WAIT, smc);
			break;
		case SOE_USER_CONNECT:
			cluster_target_socket_session_new_state(tsso,
				SOS_S3_CONN_SND, smc);
			break;
	}
}

static void
cluster_target_socket_session_s2_xprt_wait(cluster_target_session_socket_t *tsso,
	cluster_target_socket_session_sm_ctx_t *smc)
{
	switch (smc->smc_event) {
		case SOE_USER_CONNECT:
			cluster_target_socket_session_new_state(tsso,
				SOS_S4_XPRT_UP, smc);
			break;
	}
}

static void
cluster_target_socket_session_s3_conn_send(cluster_target_session_socket_t *tsso,
	cluster_target_socket_session_sm_ctx_t *smc)
{
	switch (smc->smc_event) {
		case SOE_CONNECT_SUCCESS:
			untimeout(tsso->tsso_conn_tmhdl);
			cluster_target_socket_session_new_state(tsso,
				SOS_S4_XPRT_UP, smc);
			break;
		case SOE_CONNECT_ERROR:
			untimeout(tsso->tsso_conn_tmhdl);
			/* @FALLTHROUGH */
		case SOE_CN_BIND_ERROR:
			cv_signal(&tsso->tsso_sm_cv);
			cluster_target_socket_session_new_state(tsso,
				SOS_S5_CLEANUP, smc);
			break;
	}
}

static void
cluster_target_socket_session_event_handle(void *arg)
{
	cluster_target_socket_session_sm_ctx_t *smc = arg;
	cluster_target_session_socket_t *tsso = smc->smc_tsso;

	switch (tsso->tsso_curr_state) {
		case SOS_S1_FREE:
			cluster_target_socket_session_s1_free(tsso, smc);
			break;
		case SOS_S2_XPRT_WAIT:
			cluster_target_socket_session_s2_xprt_wait(tsso, smc);
			break;
		case SOS_S3_CONN_SND:
			cluster_target_socket_session_s3_conn_send(tsso, smc);
			break;
		case SOS_S4_XPRT_UP:
			break;
	}

	cluster_target_socket_session_rele(tsso, CTSO_FTAG);
}

static void
cluster_target_socket_session_sm_event_locked(
	cluster_target_session_socket_t *tsso,
	cluster_target_socket_session_event_e event, 
	uintptr_t extinfo, void *tag)
{
	cluster_target_socket_session_sm_ctx_t *smc;

	cluster_target_socket_session_hold(tsso, tag);
	smc = kmem_alloc(sizeof(cluster_target_socket_session_sm_ctx_t), KM_SLEEP);
	smc->smc_tsso = tsso;
	smc->smc_event = event;
	smc->smc_extinfo = extinfo;
	strncpy(smc->smc_tag, tag, 128);

	(void) taskq_dispatch(tsso->tsso_sm_ctx, 
		cluster_target_socket_session_event_handle, 
		smc, TQ_SLEEP);
}

static void
cluster_target_socket_session_sm_event(cluster_target_session_socket_t *tsso,
	cluster_target_socket_session_event_e event, uintptr_t extinfo, void *tag)
{
	mutex_enter(&tsso->tsso_sm_mtx);
	cluster_target_socket_session_sm_event_locked(tsso, event, extinfo, tag);
	mutex_exit(&tsso->tsso_sm_mtx);
}

static cluster_status_t
cluster_target_socket_session_init_nonexist(cluster_target_session_socket_t **tssop,
	cluster_target_session_t *cts, cluster_target_socket_param_t *param)
{
	cluster_status_t rval;
	struct socket *so;
	cluster_target_session_socket_t *tsso;
	cluster_target_port_socket_t *tpso;
	
	for (tpso = list_head(&ctso->ctso_tpso_list); tpso;
		tpso = list_next(&ctso->ctso_tpso_list, tpso)) {
		if (strcmp(tpso->tpso_ipaddr, param->ipaddr) == 0)
			break;
	}

	if (tpso == NULL) {
		cmn_err(CE_NOTE, "%s ip(%s) is not enabled yet",
			__func__, param->ipaddr);
		return EOFFLINE;
	}

	if ((rval = cluster_target_socket_create(AF_INET, SOCK_STREAM, 0, 
			&so)) != CLUSTER_STATUS_SUCCESS)
		return rval;
	cluster_target_socket_setsockopt(so);
	cluster_target_socket_session_new(tpso, cts, so, &tsso);
	cluster_target_socket_session_sminit(tsso);
	strncpy(tsso->tsso_local_ip, param->local, 16);
	strncpy(tsso->tsso_ipaddr, param->ipaddr, 16);
	tsso->tsso_port = param->port;

	cts->sess_target_private = tsso;
	
	*tssop = tsso;
	return CLUSTER_STATUS_SUCCESS;
}

static cluster_status_t
cluster_target_socket_session_init_exist(cluster_target_session_socket_t *tsso,
	cluster_target_session_t *cts, cluster_target_socket_param_t *param)
{
	tsso->tsso_port = param->port;
	tsso->tsso_cts = cts;
	strncpy(tsso->tsso_local_ip, param->local, 16);

	cts->sess_target_private = tsso;
	/*
	 * other member need be inited.
	 */
}

static int
cluster_target_socket_session_init(cluster_target_session_t *cts, 
	cluster_target_socket_param_t *param)
{
	int rval;
	cluster_target_session_socket_t *tsso;

	rw_enter(&ctso->ctso_rw, RW_WRITER);
	for (tsso = list_head(&ctso->ctso_tsso_list); tsso;
		tsso = list_next(&ctso->ctso_tsso_list, tsso)) {
		if (strcmp(tsso->tsso_ipaddr, param->ipaddr) == 0)
			break;
	}

	if (tsso != NULL)
		rval = cluster_target_socket_session_init_exist(tsso, cts, param);
	else {
		rval = cluster_target_socket_session_init_nonexist(&tsso, cts, param);
		if (tsso != NULL) {
			cluster_target_socket_session_hold(tsso, CTSO_FTAG);
			list_insert_tail(&tsso->tsso_tpso->tpso_sess_list, tsso);
			list_insert_tail(&ctso->ctso_tsso_list, tsso);
		}
	}

	if (rval != CLUSTER_STATUS_SUCCESS)
		goto failed_out;

	cluster_target_socket_session_sm_event(tsso, 
		SOE_USER_CONNECT, NULL, CTSO_FTAG);

	mutex_enter(&tsso->tsso_sm_mtx);
	while ((tsso->tsso_curr_state != SOS_S4_XPRT_UP))
		cv_wait(&tsso->tsso_sm_cv, &tsso->tsso_sm_mtx);
	mutex_exit(&tsso->tsso_sm_mtx);

failed_out:
	rw_exit(&ctso->ctso_rw);
	return rval;
}

static void
cluster_target_socket_port_hold(cluster_target_port_socket_t *tpso, void *tag)
{
	cluster_target_socket_refcnt_hold(&tpso->tpso_refcnt, tag);
}

static void
cluster_target_socket_port_rele(cluster_target_port_socket_t *tpso, void *tag)
{
	cluster_target_socket_refcnt_rele(&tpso->tpso_refcnt, tag);
}

static void
cluster_target_socket_port_rx_handle(cluster_target_socket_worker_t *worker)
{
	list_t *exchange_list;
	cs_rx_data_t *cs_data, *cs_data_next;
	cluster_target_port_socket_t *tpso = worker->worker_private;

	cmn_err(CE_NOTE, "TARGET SOCKET RX HANDLE THREAD RUNNING");

	mutex_enter(&worker->worker_mtx);
	worker->worker_running = B_TRUE;
	cv_signal(&worker->worker_cv);

	while (worker->worker_running) {
		if (list_is_empty(worker->worker_list_w))
			cv_wait(&worker->worker_cv, &worker->worker_mtx);
		exchange_list = worker->worker_list_w;
		worker->worker_list_w = worker->worker_list_r;
		worker->worker_list_r = exchange_list;
		mutex_exit(&worker->worker_mtx);

		cmn_err(CE_NOTE, "%s", __func__);
		while ((cs_data = list_remove_head(worker->worker_list_r)) != NULL)
			cluster_san_host_rx_handle(cs_data);

		mutex_enter(&worker->worker_mtx);
	}

	cmn_err(CE_NOTE, "TARGET SOCKET RX HANDLE THREAD EXIT");
	worker->worker_running = B_FALSE;
	worker->worker_stopped = B_TRUE;
	cv_signal(&worker->worker_cv);
	cluster_target_socket_port_rele(tpso, CTSO_FTAG);
	mutex_exit(&worker->worker_mtx);
}

static void
cluster_target_socket_port_new_rx_worker(cluster_target_port_socket_t *tpso)
{
	int idx = 0;
	cluster_target_socket_worker_t *worker;

	tpso->tpso_rx_process_ctx = kmem_zalloc(sizeof(cluster_target_socket_worker_t) *
		tpso->tpso_rx_process_nthread, KM_SLEEP);

	for (; idx < tpso->tpso_rx_process_nthread; idx++) {
		cluster_target_socket_port_hold(tpso, CTSO_FTAG);
		worker = tpso->tpso_rx_process_ctx + idx;
		worker->worker_private = tpso;
		worker->worker_list_r = &worker->worker_list1;
		worker->worker_list_w = &worker->worker_list2;
		list_create(&worker->worker_list1, sizeof(cs_rx_data_t),
			offsetof(cs_rx_data_t, node));
		list_create(&worker->worker_list2, sizeof(cs_rx_data_t),
			offsetof(cs_rx_data_t, node));
		mutex_init(&worker->worker_mtx, NULL, MUTEX_DEFAULT, NULL);
		cv_init(&worker->worker_cv, NULL, CV_DRIVER, NULL);
		worker->worker_ctx = kthread_run(cluster_target_socket_port_rx_handle,
			worker, "tpso_rx_%p_%d", tpso, idx);
		mutex_enter(&worker->worker_mtx);
		while (!worker->worker_running)
			cv_wait(&worker->worker_cv, &worker->worker_mtx);
		mutex_exit(&worker->worker_mtx);
	}
}

static void
cluster_target_socket_port_free_rx_worker(cluster_target_port_socket_t *tpso)
{
	int idx = 0;
	cluster_target_socket_worker_t *worker;

	for (; idx < tpso->tpso_rx_process_nthread; idx++) {
		worker = tpso->tpso_rx_process_ctx + idx;
		mutex_enter(&worker->worker_mtx);
		if (worker->worker_running) {
			worker->worker_running = B_FALSE;
			cv_broadcast(&worker->worker_cv);
		}
		mutex_exit(&worker->worker_mtx);

		cmn_err(CE_NOTE, "%s wait for rx handle thread exit", __func__);
		
		mutex_enter(&worker->worker_mtx);
		while (!worker->worker_stopped)
			cv_wait(&worker->worker_cv, &worker->worker_mtx);
		mutex_exit(&worker->worker_mtx);
		cmn_err(CE_NOTE, "%s rx handle thread exit", __func__);
	}
	kmem_free(tpso->tpso_rx_process_ctx, sizeof(cluster_target_socket_worker_t) *
		tpso->tpso_rx_process_nthread);
}


static void
cluster_target_socket_port_new(cluster_target_port_socket_t **tpsop,
	cluster_target_port_t *ctp, char *ip, uint32_t port)
{
	cluster_target_port_socket_t *tpso;

	tpso = kmem_zalloc(sizeof(cluster_target_port_socket_t), KM_SLEEP);
	tpso->tpso_ctp = ctp;
	tpso->tpso_port = port;
	tpso->tpso_rx_process_nthread = cluster_target_socket_port_rx_process_nworker;
	strncpy(tpso->tpso_ipaddr, ip, 16);
	mutex_init(&tpso->tpso_mtx, NULL, MUTEX_DEFAULT, NULL);
	cv_init(&tpso->tpso_cv, NULL, CV_DRIVER, NULL);
	cluster_target_socket_refcnt_init(&tpso->tpso_refcnt, tpso);
	list_create(&tpso->tpso_sess_list, sizeof(cluster_target_session_socket_t),
		offsetof(cluster_target_session_socket_t, tsso_tpso_node));
	cluster_target_socket_port_new_rx_worker(tpso);
	*tpsop = tpso;
}

static void
cluster_target_socket_port_free(cluster_target_port_socket_t *tpso)
{
	cluster_target_socket_port_free_rx_worker(tpso);
	cluster_target_socket_refcnt_destroy(&tpso->tpso_refcnt);
	kmem_free(tpso, sizeof(cluster_target_port_socket_t));
}

static void
cluster_target_socket_port_watcher(cluster_target_port_socket_t *tpso)
{
	cluster_status_t rval;
	struct socket *new_so;
	cluster_target_session_socket_t *tsso;
	
	cmn_err(CE_NOTE, "CLUSTER TARGET SOCKET PORT(%d) ONLINE",
		tpso->tpso_port);
	
	mutex_enter(&tpso->tpso_mtx);
	tpso->tpso_accepter_running = B_TRUE;
	cv_signal(&tpso->tpso_cv);

	while (tpso->tpso_accepter_running) {
		mutex_exit(&tpso->tpso_mtx);

		if ((rval = kernel_accept(tpso->tpso_so, &new_so, 
				0)) != CLUSTER_STATUS_SUCCESS) {
			mutex_enter(&tpso->tpso_mtx);
			if ((rval != -ECONNABORTED) && 
				(rval != -EAGAIN)) 
				break;
			continue;
		}

		cluster_target_socket_session_new(tpso, NULL, new_so, &tsso);
		cluster_target_socket_session_sminit(tsso);
		cluster_target_socket_session_sm_event(tsso, 
			SOE_RX_CONNECT, NULL, CTSO_FTAG);
	}

	tpso->tpso_accepter_running = B_FALSE;
	mutex_exit(&tpso->tpso_mtx);
}

static cluster_status_t
cluster_target_socket_port_online(cluster_target_port_socket_t *tpso)
{
	cluster_status_t rval;
	struct sockaddr_in addr_in4;

	bzero(&addr_in4, sizeof(struct sockaddr_in));
	addr_in4.sin_family = AF_INET;
	addr_in4.sin_port = htons(tpso->tpso_port);
	addr_in4.sin_addr.s_addr = in_aton(tpso->tpso_ipaddr);

	if (((rval = kernel_bind(tpso->tpso_so, &addr_in4,
			sizeof(addr_in4))) != CLUSTER_STATUS_SUCCESS) ||
		((rval = kernel_listen(tpso->tpso_so, 
			24)) != CLUSTER_STATUS_SUCCESS)) {
		cluster_target_socket_destroy(tpso->tpso_so);
		cmn_err(CE_NOTE, "%s bind so error(%d)", __func__, rval);
		return rval;
	}

	cmn_err(CE_NOTE, "%s going to watch port(%d)", __func__, tpso->tpso_port);

	tpso->tpso_accepter = kthread_run(cluster_target_socket_port_watcher,
		tpso, "ctp_watcher_%d", tpso->tpso_port);
	mutex_enter(&tpso->tpso_mtx);
	while (!tpso->tpso_accepter_running)
		cv_wait(&tpso->tpso_cv, &tpso->tpso_mtx);
	mutex_exit(&tpso->tpso_mtx);
	
	return CLUSTER_STATUS_SUCCESS;
}

static cluster_status_t
cluster_target_socket_port_portal_up(cluster_target_port_socket_t *tpso)
{
	int rval;
	struct socket *so;

	cmn_err(CE_NOTE, "coming into %s", __func__);

	if ((rval = cluster_target_socket_create(AF_INET,
			SOCK_STREAM, 0, &so)) != CLUSTER_STATUS_SUCCESS) {
		cmn_err(CE_NOTE, "%s socket_create failed", __func__);
		return rval;
	}
	
	tpso->tpso_so = so;
	cluster_target_socket_setsockopt(tpso->tpso_so);
	if ((rval = cluster_target_socket_port_online(tpso))
			!= CLUSTER_STATUS_SUCCESS) {
		cluster_target_socket_destroy(so);
		cmn_err(CE_NOTE, "%s port_online failed", __func__);
		return rval;
	}

	cmn_err(CE_NOTE, "%s success", __func__);
	return CLUSTER_STATUS_SUCCESS;
}

int cluster_target_socket_port_init(cluster_target_port_t *ctp, 
		char *link_name, nvlist_t *nvl_conf)
{
	int rval, link_pri;
	uint16_t port;
	char *ipaddr;
	cluster_target_port_socket_t *tpso = NULL;

	if (nvl_conf == NULL) {
		cmn_err(CE_NOTE, "%s cluster san socket "
			"need a ipaddr at least", __func__);
		return EINVAL;
	}
	
    if ((rval = nvlist_lookup_string(nvl_conf, 
			"ipaddr", &ipaddr)) != 0)
        return rval;
	
    if (nvlist_lookup_int32(nvl_conf, "port", &port) != 0)
        port = 1866;
    if ((nvlist_lookup_int32(nvl_conf, "link_pri", 
			&link_pri) == 0) && link_pri)
		ctp->pri = link_pri;

	cmn_err(CE_NOTE, "%s ip(%s) port(%d)", 
		__func__, ipaddr, port);


	cluster_target_socket_port_new(&tpso, ctp, ipaddr, port);

	cluster_target_socket_port_hold(tpso, CTSO_FTAG);
	
	rval = cluster_target_socket_port_portal_up(tpso);
	if (rval != CLUSTER_STATUS_SUCCESS)
		goto failed_out;
	
/*	ctp->f_send_msg = cluster_target_socket_send;
	ctp->f_tran_free = cluster_target_socket_tran_data_free;
	ctp->f_tran_fragment = cluster_target_socket_tran_data_fragment;
	ctp->f_session_tran_start = cts_socket_tran_start;
	ctp->f_session_init = cluster_tso_session_init;
	ctp->f_session_fini = cluster_target_socket_session_fini;
	ctp->f_rxmsg_to_fragment = cts_socket_rxframe_to_fragment;
	ctp->f_fragment_free = cts_socket_fragment_free;
	ctp->f_rxmsg_free = cluster_target_socket_rxmsg_free;
	ctp->f_cts_compare = cts_socket_compare;
	ctp->f_ctp_get_info = ctp_socket_get_info;
	ctp->f_cts_get_info = cts_socket_get_info; */
	ctp->target_private = tpso;

	rw_enter(&ctso->ctso_rw, RW_WRITER);
	list_insert_tail(&ctso->ctso_tpso_list, tpso);
	rw_exit(&ctso->ctso_rw);
	return CLUSTER_STATUS_SUCCESS;
	
failed_out:
	cmn_err(CE_NOTE, "coming into cluster_target_socket_port_rele");
	cluster_target_socket_port_rele(tpso, CTSO_FTAG);
	cmn_err(CE_NOTE, "coming into cluster_target_socket_port_free");
	cluster_target_socket_port_free(tpso);
	cmn_err(CE_NOTE, "go out cluster_target_socket_port_init");
	return rval;
}

cluster_status_t
cluster_target_socket_init(void)
{
	bzero(ctso, sizeof(cts_global_t));
	
	rw_init(&ctso->ctso_rw, NULL, RW_DRIVER, NULL);
	list_create(&ctso->ctso_tpso_list, sizeof(cluster_target_port_socket_t),
		offsetof(cluster_target_port_socket_t, tpso_node));
	list_create(&ctso->ctso_tsso_list, sizeof(cluster_target_session_socket_t),
		offsetof(cluster_target_session_socket_t, tsso_node));
	ctso->ctso_refasync_taskq = taskq_create("ctso_refasync_taskq", 1,
		minclsyspri, 8, 16, TASKQ_PREPOPULATE);
	if (ctso->ctso_refasync_taskq == NULL)
		return -1;
	return CLUSTER_STATUS_SUCCESS;
}

void
cluster_target_socket_port_fini(cluster_target_port_t *ctp)
{

}
