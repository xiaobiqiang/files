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

typedef struct cluster_target_socket_session_conn_sm_ctx {
	cluster_target_session_socket_conn_t *smc_tssp;
	cluster_target_socket_session_conn_event_e smc_event;
	uintptr_t smc_extinfo;
	char smc_tag[128];
} cluster_target_socket_session_conn_sm_ctx_t;

typedef struct cluster_target_socket_session_sm_ctx {
	list_node_t smc_node;
	cluster_target_session_socket_t *smc_tsso;
	cluster_target_socket_session_event_e smc_event;
	uintptr_t smc_extinfo;
	char smc_tag[128];
} cluster_target_socket_session_sm_ctx_t;

typedef struct cluster_target_socket_session_conn_cleanup {
	cluster_target_session_socket_conn_t *cu_tssp;
	
} cluster_target_socket_session_conn_cleanup_t;

typedef struct cluster_tso_global_s {
	krwlock_t ctso_rw;
	list_t ctso_tsso_list;
	list_t ctso_tpso_list;
	kmem_cache_t *ctso_tx_data_cache;
	taskq_t *ctso_refasync_taskq;
} cts_global_t;	

static cts_global_t ctso_global;
static cts_global_t *ctso = &ctso_global;
static uint32_t cluster_target_socket_port_rx_process_nworker = 16;

static cluster_status_t
cluster_target_socket_session_rx_iov(struct socket *so, struct kvec *kvp,
        uint32_t kvlen, uint32_t totlen);
static void
cluster_target_socket_session_conn_sm_event(cluster_target_session_socket_conn_t *tssp,
        cluster_target_socket_session_conn_event_e event, uintptr_t extinfo, void *tag);
static cluster_status_t
cluster_target_socket_session_rx_buf(struct socket *so, void *buf, uint32_t buflen);
static void
cluster_target_socket_session_rx_new_csdata(cluster_target_session_socket_t *tsso,
	cluster_target_msg_header_t *hdrp, void *rcv_data, void *rcv_hdr, cs_rx_data_t **cs_datap);
static void
cluster_target_socket_session_rx_process(cluster_target_session_socket_t *tsso, cs_rx_data_t *cs_data);
static void
cluster_target_socket_port_hold(cluster_target_port_socket_t *tpso, void *tag);
static void
cluster_target_socket_port_rele(cluster_target_port_socket_t *tpso, void *tag);
static void
cluster_target_socket_session_conn_sminit(cluster_target_session_socket_conn_t *tssp);
static void
cluster_target_socket_session_conn_smfini(cluster_target_session_socket_conn_t *tssp);
static void
cluster_target_socket_session_sm_event(cluster_target_session_socket_t *tsso,
        cluster_target_socket_session_event_e event, void *extptr, void *tag);
static void
cluster_target_socket_port_watcher(cluster_target_port_socket_t *tpso);

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
//	ctso_assert(refcnt->tr_wait == CTSO_REF_NOWAIT);
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
	int sndbuf = 2 * 1024 * 1024, rcvbuf = 2 * 1024 *1024;
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

static int
cluster_target_socket_session_compare(cluster_target_session_t *cts, void *phy_head)
{
	cluster_status_t rval = B_FALSE;
	cluster_target_socket_param_t *param = phy_head;
	cluster_target_session_socket_t *tsso = cts->sess_target_private;

	if (strcmp(tsso->tsso_ipaddr, param->ipaddr) == 0)
		rval = B_TRUE;
	return rval;
}

static void
cluster_target_socket_session_get_info(cluster_target_session_t *cts, nvlist_t *nvl)
{
	cluster_target_session_socket_t *tsso = cts->sess_target_private;
	
	VERIFY(nvlist_add_string(nvl, CS_NVL_RPC_IP, tsso->tsso_ipaddr) == 0);
}

static cluster_status_t
cluster_target_socket_session_tran_fragment(void *src_port, void *dst_sess, 
	cluster_tran_data_origin_t *origin_data,
	cluster_target_tran_data_t **fragmentations, int *cnt)
{
	cluster_target_tran_data_t *frag;
	cluster_target_socket_tran_data_t *tdt;
	cluster_target_msg_header_t *ct_head;
	
	tdt = kmem_alloc(sizeof(cluster_target_socket_tran_data_t), KM_SLEEP);
	
	ct_head = &tdt->tdt_ct_head;
	ct_head->ex_len = origin_data->header_len;
	ct_head->index = origin_data->index;
	ct_head->len = origin_data->data_len;
	ct_head->msg_type = origin_data->msg_type;
	ct_head->need_reply = origin_data->need_reply;
	ct_head->offset = 0;
	ct_head->total_len = origin_data->data_len;

	tdt->tdt_iov[0].iov_base = ct_head;
	tdt->tdt_iov[0].iov_len = sizeof(cluster_target_msg_header_t);
	tdt->tdt_iov[1].iov_base = origin_data->header;
	tdt->tdt_iov[1].iov_len = origin_data->header_len;
	tdt->tdt_iov[2].iov_base = origin_data->data;
	tdt->tdt_iov[2].iov_len = origin_data->data_len;
	tdt->tdt_iovlen = 3;
	tdt->tdt_iovbuflen = sizeof(cluster_target_msg_header_t) +
		origin_data->header_len + origin_data->data_len;
	
	frag = kmem_alloc(sizeof(cluster_target_tran_data_t), KM_SLEEP);
	frag->fragmentation = tdt;
	
	*cnt = 1;
	*fragmentations = frag;
	return CLUSTER_STATUS_SUCCESS;
}

static int
cluster_target_socket_session_tx_process(cluster_target_socket_tran_data_t *tdt)
{
	cluster_target_session_socket_conn_t *tssp = tdt->tdt_tssp;
	
	mutex_enter(&tssp->tssp_tx_mtx);
	list_insert_tail(&tssp->tssp_tx_wait_list, tdt);
	cv_signal(&tssp->tssp_tx_cv);
	mutex_exit(&tssp->tssp_tx_mtx);
}

static int
cluster_target_socket_session_tran_start(
		cluster_target_session_t *cts, void *fragment)
{
	cluster_status_t rval;
	cluster_tran_data_origin_t *origin_data = fragment;
	cluster_target_session_socket_t *tsso = cts->sess_target_private;
	cluster_target_session_socket_conn_t *tssp;
	cluster_target_socket_tran_data_t tdt_struct;
	cluster_target_socket_tran_data_t *tdt = &tdt_struct;
	cluster_target_msg_header_t *ct_head;
	cluster_evt_header_t *evt_header;
	kmutex_t mtx;
	kcondvar_t cv;

	mutex_init(&mtx, NULL, MUTEX_DEFAULT, NULL);
	cv_init(&cv, NULL, CV_DRIVER, NULL);
	
//	tdt = kmem_cache_alloc(ctso->ctso_tx_data_cache, KM_NOSLEEP);
	bzero(&tdt->tdt_sohdr, sizeof(struct msghdr));
	tdt->tdt_tssp = tsso->tsso_tssp[origin_data->index % tsso->tsso_tssp_cnt];
	tdt->tdt_waiting = B_TRUE;
	tdt->tdt_mtx = &mtx;
	tdt->tdt_cv = &cv;

/*	cmn_err(CE_NOTE, "idx(%llu) msg_type(%02x) hlen(%04x) dlen(%x)",
		origin_data->index, origin_data->msg_type, 
		origin_data->header_len, origin_data->data_len);

	if (origin_data->msg_type == CLUSTER_SAN_MSGTYPE_CLUSTER) {
		evt_header = origin_data->header;
		cmn_err(CE_NOTE, "%s msg_type(%02x) data_len(%x)", __func__, 
			evt_header->msg_type, origin_data->data_len);
	} */

	ct_head = &tdt->tdt_ct_head;
	ct_head->ex_len = origin_data->header_len;
	ct_head->index = origin_data->index;
	ct_head->len = origin_data->data_len;
	ct_head->msg_type = origin_data->msg_type;
	ct_head->need_reply = origin_data->need_reply;
	ct_head->offset = 0;
	ct_head->total_len = origin_data->data_len;
	
	tdt->tdt_iov[0].iov_base = &tdt->tdt_ct_head;
	tdt->tdt_iov[0].iov_len = sizeof(cluster_target_msg_header_t);

	tdt->tdt_iov[1].iov_base = origin_data->header;
	tdt->tdt_iov[1].iov_len = origin_data->header_len;

	tdt->tdt_iov[2].iov_base = origin_data->data;
	tdt->tdt_iov[2].iov_len = origin_data->data_len;

/*	if (origin_data->header_len) {
		tdt->tdt_iov[1].iov_base = kmem_alloc(origin_data->header_len);
		bcopy(origin_data->header, tdt->tdt_iov[1].iov_base, 
			origin_data->header_len);
		tdt->tdt_iov[1].iov_len = origin_data->header_len;
	} else {
		tdt->tdt_iov[1].iov_base = NULL;
		tdt->tdt_iov[1].iov_len = 0;
	}

	if (origin_data->data_len) {
		tdt->tdt_iov[2].iov_base = vmalloc(origin_data->data_len);
		bcopy(origin_data->data, tdt->tdt_iov[2].iov_base, origin_data->data_len);
		tdt->tdt_iov[2].iov_len = origin_data->data_len;
	} else {
		tdt->tdt_iov[2].iov_base = NULL;
		tdt->tdt_iov[2].iov_len = 0;
	} */

	tdt->tdt_iovlen = 3;
	tdt->tdt_iovbuflen = sizeof(cluster_target_msg_header_t) + 
		origin_data->header_len + origin_data->data_len;

	mutex_enter(&tsso->tsso_sm_mtx);
	if (tsso->tsso_curr_state != SSS_S3_READY) {
		mutex_exit(&tsso->tsso_sm_mtx);
		return -EOFFLINE;
	}
	mutex_exit(&tsso->tsso_sm_mtx);
	
	cluster_target_socket_session_tx_process(tdt);

	mutex_enter(tdt->tdt_mtx);
	while (tdt->tdt_waiting)
		cv_wait(tdt->tdt_cv, tdt->tdt_mtx);
	mutex_exit(tdt->tdt_mtx);
/*	kmem_cache_free(ctso->ctso_tx_data_cache, tdt);
	mutex_enter(&tssp->tssp_sm_mtx);
	if (tssp->tssp_curr_state != SOS_S4_XPRT_UP) {
		mutex_exit(&tssp->tssp_sm_mtx);
		return -ECONNABORTED;
	}
	mutex_exit(&tssp->tssp_sm_mtx); */

	return CLUSTER_STATUS_SUCCESS;	
}

static void
cluster_target_socket_session_conn_new(cluster_target_session_socket_t *tsso,
	struct socket *so, char *ip, uint16_t port, 
	cluster_target_session_socket_conn_t **tsspp)
{
	cluster_target_session_socket_conn_t *tssp;

	tssp = kmem_zalloc(sizeof(cluster_target_session_socket_conn_t), KM_SLEEP);
	tssp->tssp_so = so;
	tssp->tssp_port = port;
	tssp->tssp_tsso = tsso;
	cluster_target_socket_session_hold(tsso, CTSO_FTAG);
	strncpy(tssp->tssp_ipaddr, ip, 16);
	tssp->tssp_conn_tmhdl = TIMEOUT_ID_INVALID;
	
	mutex_init(&tssp->tssp_rx_mtx, NULL, MUTEX_DEFAULT, NULL);
	cv_init(&tssp->tssp_rx_cv, NULL, CV_DRIVER, NULL);
	list_create(&tssp->tssp_rx_data_list, sizeof(cs_rx_data_t),
		offsetof(cs_rx_data_t, node));

	mutex_init(&tssp->tssp_tx_mtx, NULL, MUTEX_DEFAULT, NULL);
	cv_init(&tssp->tssp_tx_cv, NULL, CV_DRIVER, NULL);
	list_create(&tssp->tssp_tx_wait_list, sizeof(cluster_target_socket_tran_data_t),
		offsetof(cluster_target_socket_tran_data_t, tdt_node));
	list_create(&tssp->tssp_tx_live_list, sizeof(cluster_target_socket_tran_data_t),
		offsetof(cluster_target_socket_tran_data_t, tdt_node));
	
	cluster_target_socket_session_conn_sminit(tssp);
	
	*tsspp = tssp;
}


static void
cluster_target_socket_session_conn_free(cluster_target_session_socket_conn_t *tssp)
{
	if (tssp->tssp_so)
		cluster_target_socket_destroy(tssp->tssp_so);
	cluster_target_socket_session_conn_smfini(tssp);
	cluster_target_socket_session_rele(tssp->tssp_tsso, CTSO_FTAG);
	tssp->tssp_tsso = NULL;
	kmem_free(tssp, sizeof(cluster_target_session_socket_conn_t));
}

static void
cluster_target_socket_session_conn_rx(cluster_target_session_socket_conn_t *tssp)
{
	cluster_status_t rval;
	cs_rx_data_t *cs_data = NULL;
	cluster_target_msg_header_t ct_head;
	boolean_t conn_break = B_FALSE;
	cluster_target_session_socket_t *tsso = tssp->tssp_tsso;
	cluster_evt_header_t *evt_header;
	void *rcv_buff = kmalloc(1024 * 1024, GFP_KERNEL);
	void *rcv_hdr = kmalloc(1024 * 1024, GFP_KERNEL);
	
	cmn_err(CE_NOTE, "PORT SESSION(%p) RX THREAD IS RUNNING", tssp);
	
	mutex_enter(&tssp->tssp_rx_mtx);
	tssp->tssp_rx_running = B_TRUE;
	cv_signal(&tssp->tssp_rx_cv);

	while (tssp->tssp_rx_running) {
		mutex_exit(&tssp->tssp_rx_mtx);

		if ((rval = cluster_target_socket_session_rx_buf(tssp->tssp_so,
				&ct_head, sizeof(ct_head))) != CLUSTER_STATUS_SUCCESS) {
			mutex_enter(&tssp->tssp_rx_mtx);
			/*
			 * if rx_running is true at this time,
			 * it's broken pipe mostly.
			 */
			if (tssp->tssp_rx_running)
				conn_break = B_TRUE;
			tssp->tssp_rx_running = B_FALSE;
			continue;
		}

/*		cmn_err(CE_NOTE, "idx(%llu) msg_type(%02x) hlen(%04x) dlen(%x)",
			ct_head.index, ct_head.msg_type, 
			ct_head.ex_len, ct_head.total_len); */
		
		cluster_target_socket_session_rx_new_csdata(
			tssp->tssp_tsso, &ct_head, rcv_buff, rcv_hdr, &cs_data);
		if (((rval = cluster_target_socket_session_rx_buf(tssp->tssp_so,
				cs_data->ex_head, cs_data->ex_len)) != CLUSTER_STATUS_SUCCESS) ||
			((rval = cluster_target_socket_session_rx_buf(tssp->tssp_so,
				cs_data->data, cs_data->data_len)) != CLUSTER_STATUS_SUCCESS)) {
			csh_rx_data_free_ext(cs_data);
			mutex_enter(&tssp->tssp_rx_mtx);
			/*
			 * if rx_running is true at this time,
			 * it's broken pipe mostly.
			 */
			if (tssp->tssp_rx_running)
				conn_break = B_TRUE;
			tssp->tssp_rx_running = B_FALSE;
			continue;
		}

		/*
		 * test not to alloc mem when rx data.
		 */
/*		if (ct_head.total_len >= 4 * 1024) {
			cs_data->data = NULL;
			cs_data->data_len = 0;
			cs_data->ex_head = NULL;
			cs_data->ex_len = 0;
			cs_data->msg_type = CLUSTER_SAN_MSGTYPE_HB;
			
		}  */

/*		if (ct_head.msg_type == CLUSTER_SAN_MSGTYPE_CLUSTER) {
			evt_header = cs_data->ex_head;
			cmn_err(CE_NOTE, "%s msg_type(%02x) data_len(%x)", __func__, 
				evt_header->msg_type, cs_data->data_len);
		} */
		
		cluster_target_socket_session_rx_process(tssp->tssp_tsso, cs_data);
		mutex_enter(&tssp->tssp_rx_mtx);
	}

	if (conn_break)
		cluster_target_socket_session_conn_sm_event(tssp,
			SOE_TSSP_CONN_LOST, NULL, CTSO_FTAG);
	cluster_target_socket_session_rele(tsso, CTSO_FTAG);
	cmn_err(CE_NOTE, "PORT SESSION(%p) RX THREAD EXIT", tssp);
	tssp->tssp_rx_stoped = B_TRUE;
	cv_signal(&tssp->tssp_rx_cv);
	mutex_exit(&tssp->tssp_rx_mtx);
}

/*
 * not complete
 */
static void
cluster_target_socket_session_conn_tx(cluster_target_session_socket_conn_t *tssp)
{
	int rval;
	cluster_target_socket_tran_data_t *tdt;
	cluster_target_session_socket_t *tsso = tssp->tssp_tsso;

	mutex_enter(&tssp->tssp_tx_mtx);
	tssp->tssp_tx_running = B_TRUE;
	cv_signal(&tssp->tssp_tx_cv);

	while (tssp->tssp_tx_running) {
		if (list_is_empty(&tssp->tssp_tx_wait_list)) {
			cv_wait(&tssp->tssp_tx_cv, &tssp->tssp_tx_mtx);
		}

		list_move_tail(&tssp->tssp_tx_live_list, &tssp->tssp_tx_wait_list);
		mutex_exit(&tssp->tssp_tx_mtx);
		
		while ((tdt = list_remove_head(&tssp->tssp_tx_live_list)) != NULL) {
			rval = kernel_sendmsg(tssp->tssp_so, &tdt->tdt_sohdr, 
				tdt->tdt_iov, tdt->tdt_iovlen, tdt->tdt_iovbuflen);
			mutex_enter(tdt->tdt_mtx);
			tdt->tdt_waiting = B_FALSE;
			cv_signal(tdt->tdt_cv);
			mutex_exit(tdt->tdt_mtx);
		}

		mutex_enter(&tssp->tssp_tx_mtx);
	}

	cluster_target_socket_session_rele(tsso, CTSO_FTAG);
	cmn_err(CE_NOTE, "PORT SESSION(%p) TX THREAD EXIT", tssp);
	tssp->tssp_tx_stoped = B_TRUE;
	cv_signal(&tssp->tssp_tx_cv);
	mutex_exit(&tssp->tssp_tx_mtx);
}

static void
cluster_target_socket_session_conn_connect_timedout(cluster_target_session_socket_conn_t *tssp)
{
	/*
	 * wake up kernel_connect
	 */
	cluster_target_socket_shutdown(tssp->tssp_so);
}

static void
cluster_target_socket_session_conn_connect(cluster_target_session_socket_conn_t *tssp)
{
	/* tx and rx thread */
	cluster_target_socket_session_hold(tssp->tssp_tsso, CTSO_FTAG);
	tssp->tssp_rx = kthread_run(cluster_target_socket_session_conn_rx,
		tssp, "tssp_rx_%p", tssp);
	mutex_enter(&tssp->tssp_rx_mtx);
	while (!tssp->tssp_rx_running)
		cv_wait(&tssp->tssp_rx_cv, &tssp->tssp_rx_mtx);
	mutex_exit(&tssp->tssp_rx_mtx);

	cluster_target_socket_session_hold(tssp->tssp_tsso, CTSO_FTAG);
	tssp->tssp_tx = kthread_run(cluster_target_socket_session_conn_tx,
		tssp, "tssp_tx_%p", tssp);
	mutex_enter(&tssp->tssp_tx_mtx);
	while (!tssp->tssp_tx_running)
		cv_wait(&tssp->tssp_tx_cv, &tssp->tssp_tx_mtx);
	mutex_exit(&tssp->tssp_tx_mtx);
}

static void
cluster_target_socket_session_conn_disconnect(cluster_target_session_socket_conn_t *tssp)
{
	mutex_enter(&tssp->tssp_tx_mtx);
	if (tssp->tssp_tx_running) {
		tssp->tssp_tx_running = B_FALSE;
		cv_signal(&tssp->tssp_tx_cv);
		while (!tssp->tssp_tx_stoped)
			cv_wait(&tssp->tssp_tx_cv, &tssp->tssp_tx_mtx);
	}
	mutex_exit(&tssp->tssp_tx_mtx);

	mutex_enter(&tssp->tssp_rx_mtx);
	if (tssp->tssp_rx_running) {
		tssp->tssp_rx_running = B_FALSE;
		cluster_target_socket_shutdown(tssp->tssp_so);
		while (!tssp->tssp_rx_stoped)
			cv_wait(&tssp->tssp_rx_cv, &tssp->tssp_rx_mtx);
	}
	mutex_exit(&tssp->tssp_rx_mtx);
}

static void
cluster_target_socket_session_conn_complete(cluster_target_session_socket_conn_t *tssp)
{
	cluster_target_socket_session_sm_event(tssp->tssp_tsso,
		SSE_TSSP_COMPLETE, tssp, CTSO_FTAG);
}

static void
cluster_target_socket_session_conn_new_state(cluster_target_session_socket_conn_t *tssp,
	cluster_target_socket_session_conn_state_e new_state, 
	cluster_target_socket_session_conn_sm_ctx_t *smc)
{
	cluster_status_t rval;
	struct sockaddr_in addr_in4;
	cluster_target_session_socket_t *iter_tsso;

	cmn_err(CE_NOTE, "tssp state(%d --> %d --> %d) cause(%d)",
		tssp->tssp_last_state, tssp->tssp_curr_state, 
		new_state, smc->smc_event);

	mutex_enter(&tssp->tssp_sm_mtx);
	tssp->tssp_last_state = tssp->tssp_curr_state;
	tssp->tssp_curr_state = new_state;
	CTSO_SM_AUDIT(&tssp->tssp_sm_audit, smc->smc_event, smc->smc_tag,
		tssp->tssp_last_state, tssp->tssp_curr_state);
	mutex_exit(&tssp->tssp_sm_mtx);

	switch (tssp->tssp_curr_state) {
		case SOS_S1_FREE:
			break;
		case SOS_S2_XPRT_WAIT:
/*			rw_enter(&ctso->ctso_rw, RW_WRITER);
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
			rw_exit(&ctso->ctso_rw); */

			/*
			 * nothing to do now.
			 */
			break;
		case SOS_S3_CONN_SND:
			bzero(&addr_in4, sizeof(struct sockaddr_in));
			addr_in4.sin_family = AF_INET;
			addr_in4.sin_port = htons(0);
			addr_in4.sin_addr.s_addr = in_aton(tssp->tssp_local_ip);
			if ((rval = kernel_bind(tssp->tssp_so, &addr_in4,
					sizeof(struct sockaddr_in))) != 0) {
				cmn_err(CE_NOTE, "%s kernel bind ip(%s) error(%d)",
					__func__, tssp->tssp_local_ip, rval);

				mutex_enter(&tssp->tssp_sm_mtx);
				tssp->tssp_connect_status = rval;
				//maybe someone wait it in the future.who know?
				cv_signal(&tssp->tssp_sm_cv);	
				mutex_exit(&tssp->tssp_sm_mtx);
				cluster_target_socket_session_conn_sm_event(tssp,
					SOE_CN_BIND_ERROR, NULL, CTSO_FTAG);
				break;
			}

			bzero(&addr_in4, sizeof(struct sockaddr_in));
			addr_in4.sin_family = AF_INET;
			addr_in4.sin_port = htons(tssp->tssp_port);
			addr_in4.sin_addr.s_addr = in_aton(tssp->tssp_ipaddr);
			tssp->tssp_conn_tmhdl = timeout(
				cluster_target_socket_session_conn_connect_timedout,
				tssp, SEC_TO_TICK(CTSO_CONNECT_TIMEOUT));
			if ((rval = kernel_connect(tssp->tssp_so, &addr_in4,
					sizeof(struct sockaddr_in), 0)) == 0) {
				cluster_target_socket_session_conn_sm_event(tssp,
					SOE_CONNECT_SUCCESS, NULL, CTSO_FTAG);
			} else {
				/*
				 * peer port offline
				 */
				if (rval == -ECONNREFUSED) {
					cmn_err(CE_NOTE, "peer(%s) port offline, wait it now...",
						tssp->tssp_ipaddr);
					cluster_target_socket_session_conn_sm_event(tssp,
						SOE_PRPORT_OFFLINE, NULL, CTSO_FTAG);
				} else {
					cmn_err(CE_NOTE, "%s connect ip(%s) error(%d)",
						__func__, tssp->tssp_ipaddr, rval);
					
					mutex_enter(&tssp->tssp_sm_mtx);
					tssp->tssp_connect_status = rval;
					cv_signal(&tssp->tssp_sm_cv);
					mutex_exit(&tssp->tssp_sm_mtx);
					cluster_target_socket_session_conn_sm_event(tssp,
						SOE_CONNECT_ERROR, NULL, CTSO_FTAG);
				}
			}
			break;
		case SOS_S4_XPRT_UP:
			cv_signal(&tssp->tssp_sm_cv);
			cmn_err(CE_NOTE, "%s session ip(%s) port(%d) come into SOS_S4_XPRT_UP",
				__func__, tssp->tssp_ipaddr, tssp->tssp_port);
			cluster_target_socket_session_conn_connect(tssp);
			cluster_target_socket_session_sm_event(tssp->tssp_tsso,
				SSE_TSSP_READY, NULL, CTSO_FTAG);
			break;
		case SOS_S5_IN_EPIPE:
			cmn_err(CE_NOTE, "%s session ip(%s) come into SOS_S5_IN_EPIPE",
				__func__, tssp->tssp_ipaddr);
			/* FALLTHROUGH */
		case SOS_S6_IN_LOGOUT:
			if (smc->smc_event == SOE_TSSO_IN_CLEANUP)
				cmn_err(CE_NOTE, "%s session ip(%s) come into SOS_S6_IN_LOGOUT",
					__func__, tssp->tssp_ipaddr);
			cluster_target_socket_session_conn_disconnect(tssp);
			/* FALLTHROUGH */
		case SOS_S7_CLEANUP:
			/* 
			 * we can't abort cs_rx_data not completed.
			 * so nothing to do now.
			 */
			/* FALLTHROUGH */
		case SOS_S8_COMPLETE:
			cmn_err(CE_NOTE, "%s session ip(%s) come into SOS_S8_COMPLETE",
				__func__, tssp->tssp_ipaddr);
			taskq_dispatch(ctso->ctso_refasync_taskq,
				cluster_target_socket_session_conn_complete,
				tssp, TQ_SLEEP);
			break;
	}
}

static void
cluster_target_socket_session_conn_s1_free(cluster_target_session_socket_conn_t *tssp,
	cluster_target_socket_session_conn_sm_ctx_t *smc)
{
	switch (smc->smc_event) {
		case SOE_RX_CONNECT:
			cluster_target_socket_session_conn_new_state(tssp, 
				SOS_S2_XPRT_WAIT, smc);
			break;
		case SOE_USER_CONNECT:
			cluster_target_socket_session_conn_new_state(tssp,
				SOS_S3_CONN_SND, smc);
			break;
		case SOE_TSSO_IN_CLEANUP:
			cluster_target_socket_session_conn_new_state(tssp,
				SOS_S8_COMPLETE, smc);
			break;
	}
}

static void
cluster_target_socket_session_conn_s2_xprt_wait(cluster_target_session_socket_conn_t *tssp,
	cluster_target_socket_session_conn_sm_ctx_t *smc)
{
	switch (smc->smc_event) {
		case SOE_USER_CONNECT:
			cluster_target_socket_session_conn_new_state(tssp,
				SOS_S4_XPRT_UP, smc);
			break;
		case SOE_TSSO_IN_CLEANUP:
			cluster_target_socket_session_conn_new_state(tssp,
				SOS_S8_COMPLETE, smc);
			break;
	}
}

static void
cluster_target_socket_session_conn_s3_conn_send(cluster_target_session_socket_conn_t *tssp,
	cluster_target_socket_session_conn_sm_ctx_t *smc)
{
	switch (smc->smc_event) {
		case SOE_CONNECT_SUCCESS:
			untimeout(tssp->tssp_conn_tmhdl);
			cluster_target_socket_session_conn_new_state(tssp,
				SOS_S4_XPRT_UP, smc);
			break;
		case SOE_PRPORT_OFFLINE:
		case SOE_CONNECT_ERROR:
			untimeout(tssp->tssp_conn_tmhdl);
			/* @FALLTHROUGH */
		case SOE_CN_BIND_ERROR:
			cv_signal(&tssp->tssp_sm_cv);
			cluster_target_socket_session_conn_new_state(tssp,
				SOS_S7_CLEANUP, smc);
			break;
		case SOE_TSSO_IN_CLEANUP:
			cluster_target_socket_session_conn_new_state(tssp,
				SOS_S8_COMPLETE, smc);
			break;
	}
}

static void
cluster_target_socket_session_conn_s4_xprt_up(cluster_target_session_socket_conn_t *tssp,
	cluster_target_socket_session_conn_sm_ctx_t *smc)
{
	switch (smc->smc_event) {
		case SOE_TSSO_IN_CLEANUP:
			cluster_target_socket_session_conn_new_state(tssp,
				SOS_S6_IN_LOGOUT, smc);
			break;
		case SOE_TSSP_CONN_LOST:
			cluster_target_socket_session_conn_new_state(tssp,
				SOS_S5_IN_EPIPE, smc);
	}
}

static void
cluster_target_socket_session_conn_s5_in_pipe(cluster_target_session_socket_conn_t *tssp,
	cluster_target_socket_session_conn_sm_ctx_t *smc)
{
	switch (smc->smc_event) {
		case SOE_TSSO_IN_CLEANUP:
			cluster_target_socket_session_conn_new_state(tssp,
				SOS_S6_IN_LOGOUT, smc);
			break;
	}
}

static void
cluster_target_socket_session_conn_s6_in_logout(cluster_target_session_socket_conn_t *tssp,
	cluster_target_socket_session_conn_sm_ctx_t *smc)
{
	switch (smc->smc_event) {
		case SOE_TSSO_IN_CLEANUP:
			cluster_target_socket_session_conn_new_state(tssp,
				SOS_S6_IN_LOGOUT, smc);
			break;
	}
}


static void
cluster_target_socket_session_conn_event_handle(void *arg)
{
	cluster_target_socket_session_conn_sm_ctx_t *smc = arg;
	cluster_target_session_socket_conn_t *tssp = smc->smc_tssp;

	switch (tssp->tssp_curr_state) {
		case SOS_S1_FREE:
			cluster_target_socket_session_conn_s1_free(tssp, smc);
			break;
		case SOS_S2_XPRT_WAIT:
			cluster_target_socket_session_conn_s2_xprt_wait(tssp, smc);
			break;
		case SOS_S3_CONN_SND:
			cluster_target_socket_session_conn_s3_conn_send(tssp, smc);
			break;
		case SOS_S4_XPRT_UP:
			cluster_target_socket_session_conn_s4_xprt_up(tssp, smc);
			break;
	}

	cluster_target_socket_session_rele(tssp->tssp_tsso, CTSO_FTAG);
	kmem_free(smc, sizeof(cluster_target_socket_session_conn_sm_ctx_t));
}

static void
cluster_target_socket_session_conn_sm_event_locked(
	cluster_target_session_socket_conn_t *tssp,
	cluster_target_socket_session_conn_event_e event, 
	uintptr_t extinfo, void *tag)
{
	cluster_target_socket_session_conn_sm_ctx_t *smc;

	cluster_target_socket_session_hold(tssp->tssp_tsso, tag);
	smc = kmem_alloc(sizeof(cluster_target_socket_session_conn_sm_ctx_t), KM_SLEEP);
	smc->smc_tssp = tssp;
	smc->smc_event = event;
	smc->smc_extinfo = extinfo;
	strncpy(smc->smc_tag, tag, 128);

	(void) taskq_dispatch(tssp->tssp_sm_ctx, 
		cluster_target_socket_session_conn_event_handle, 
		smc, TQ_SLEEP);
}

static void
cluster_target_socket_session_conn_sm_event(cluster_target_session_socket_conn_t *tssp,
	cluster_target_socket_session_conn_event_e event, uintptr_t extinfo, void *tag)
{
	mutex_enter(&tssp->tssp_sm_mtx);
	cluster_target_socket_session_conn_sm_event_locked(tssp, event, extinfo, tag);
	mutex_exit(&tssp->tssp_sm_mtx);
}

static void
cluster_target_socket_session_conn_sminit(cluster_target_session_socket_conn_t *tssp)
{
	char sm_tq_name[64] = {0};
	cluster_target_socket_sm_audit_t *sm_audit = &tssp->tssp_sm_audit;
	
	mutex_init(&tssp->tssp_sm_mtx, NULL, MUTEX_DEFAULT, NULL);
	cv_init(&tssp->tssp_sm_cv, NULL, CV_DRIVER, NULL);
	snprintf(sm_tq_name, 64, "tssp_sm_%p", tssp);
	tssp->tssp_sm_ctx = taskq_create(sm_tq_name, 1, minclsyspri, 
			1, 1, TASKQ_PREPOPULATE);

	tssp->tssp_last_state = SOS_S1_FREE;
	tssp->tssp_curr_state = SOS_S1_FREE;
	sm_audit->sa_max_depth = TSSO_SM_AUDIT_DEPTH - 1;
}

static void
cluster_target_socket_session_conn_smfini(cluster_target_session_socket_conn_t *tssp)
{
	if (tssp->tssp_sm_ctx == NULL)
		return ;

	mutex_destroy(&tssp->tssp_sm_mtx);
	taskq_wait(tssp->tssp_sm_ctx);
	taskq_destroy(tssp->tssp_sm_ctx);
	tssp->tssp_sm_ctx = NULL;
}

static void
cluster_target_socket_session_sminit(cluster_target_session_socket_t *tsso)
{
	mutex_init(&tsso->tsso_sm_mtx, NULL, MUTEX_DEFAULT, NULL);
	cv_init(&tsso->tsso_sm_cv, NULL, CV_DRIVER, NULL);
	list_create(&tsso->tsso_smevt_list, sizeof(cluster_target_socket_session_sm_ctx_t),
		offsetof(cluster_target_socket_session_sm_ctx_t, smc_node));
	tsso->tsso_curr_state = SSS_S1_FREE;
	tsso->tsso_last_state = SSS_S1_FREE;
	tsso->tsso_sm_busy = B_FALSE;
	tsso->tsso_sm_audit.sa_max_depth = TSSO_SM_AUDIT_DEPTH - 1;
}

static void
cluster_target_socket_session_smfini(cluster_target_session_socket_t *tsso)
{
	mutex_destroy(&tsso->tsso_sm_mtx);
	cv_destroy(&tsso->tsso_sm_cv);
	list_destroy(&tsso->tsso_smevt_list);
}

static void
cluster_target_socket_session_clean_tssp(cluster_target_session_socket_t *tsso)
{
	int idx = 0;
	cluster_target_session_socket_conn_t *tssp;
	
	mutex_enter(&tsso->tsso_mtx);
	for (; idx < tsso->tsso_tssp_cnt; idx++) {
		if ((tssp = tsso->tsso_tssp[idx]) != NULL)
			cluster_target_socket_session_conn_sm_event(tssp,
				SOE_TSSO_IN_CLEANUP, NULL, CTSO_FTAG);
	}
	mutex_exit(&tsso->tsso_mtx);
}

static void
cluster_target_socket_session_new_state(cluster_target_session_socket_t *tsso,
	cluster_target_socket_session_state_e new_state, 
	cluster_target_socket_session_sm_ctx_t *smc)
{
	cluster_status_t rval, idx = 0;
	cluster_target_session_socket_conn_t *tssp;
	
	cmn_err(CE_NOTE, "tsso(%s) state(%d --> %d --> %d) cause(%d)",
		tsso->tsso_ipaddr, tsso->tsso_last_state, tsso->tsso_curr_state, 
		new_state, smc->smc_event);

	mutex_enter(&tsso->tsso_sm_mtx);
	tsso->tsso_last_state = tsso->tsso_curr_state;
	tsso->tsso_curr_state = new_state;
	CTSO_SM_AUDIT(&tsso->tsso_sm_audit, smc->smc_event, smc->smc_tag, 
		tsso->tsso_last_state, tsso->tsso_curr_state);
	mutex_exit(&tsso->tsso_sm_mtx);

	switch (tsso->tsso_curr_state) {
		case SSS_S1_FREE:
			break;
		case SSS_S2_READY_WAIT:
			for ( ; idx < tsso->tsso_tssp_cnt; idx++)
				cluster_target_socket_session_conn_sm_event(
					tsso->tsso_tssp[idx], SOE_USER_CONNECT, 
					NULL, CTSO_FTAG);
			break;
		case SSS_S3_READY:
			ctso_assert(MUTEX_HELD(&tsso->tsso_mtx));
			cluster_target_session_hold(tsso->tsso_cts, "down2up evt");
			if (tsso->tsso_cts->sess_linkstate == CTS_LINK_DOWN) {
				tsso->tsso_cts->sess_linkstate = CTS_LINK_UP;
				taskq_dispatch(clustersan->cs_async_taskq, 
					cts_link_down_to_up_handle, 
					tsso->tsso_cts, TQ_SLEEP);
			} else
				cluster_target_session_rele(tsso->tsso_cts, "down2up evt");
			/*
			 * wake up user connect process
			 */
			cv_signal(&tsso->tsso_sm_cv);
			break;
		case SSS_S4_IN_TSSP_CLEAN:
			tssp = smc->smc_extinfo;
			mutex_enter(&tsso->tsso_mtx);
			cmn_err(CE_NOTE, "tssp(%p) idx(%d) of tsso(%s) to free",
				tssp, tssp->tssp_tsso_idx, tsso->tsso_ipaddr);
			tsso->tsso_tssp_idx = 0;
			tsso->tsso_s4_cnt = 0;
			if (tsso->tsso_tssp[tssp->tssp_tsso_idx] != NULL) {
				tsso->tsso_tssp[tssp->tssp_tsso_idx] = NULL;
				tsso->tsso_tssp_realcnt--;
				cluster_target_socket_session_conn_free(tssp);
				if ((tsso->tsso_status == CLUSTER_STATUS_SUCCESS) &&
					(tsso->tsso_tssp_realcnt == 0)) {
					cluster_target_session_hold(tsso->tsso_cts, "up2down evt");
					if (tsso->tsso_cts->sess_linkstate == CTS_LINK_UP) {
						tsso->tsso_cts->sess_linkstate = CTS_LINK_DOWN;
						taskq_dispatch(clustersan->cs_async_taskq, 
							cts_link_up_to_down_handle, 
							tsso->tsso_cts, TQ_SLEEP);
					} else 
						cluster_target_session_rele(tsso->tsso_cts, "up2down evt");
				}
			}
			mutex_exit(&tsso->tsso_mtx);
			break;
		case SSS_S5_CLEANUP:
			/*
			 * wake up active connect fail, user process wait it.
			 */
			cv_signal(&tsso->tsso_sm_cv);
			cluster_target_socket_session_clean_tssp(tsso);
			/* FALLTHROUGH */
		case SSS_S6_COMPLETE:
			
			break;
	}
}

static void
cluster_target_socket_session_s1_free(cluster_target_session_socket_t *tsso,
	cluster_target_socket_session_sm_ctx_t *smc)
{
	switch (smc->smc_event) {
		case SSE_USER_CONNECT:
			cluster_target_socket_session_new_state(tsso, 
				SSS_S2_READY_WAIT, smc);
			break;
		case SSE_TPSO_COMPLETE:
			cluster_target_socket_session_new_state(tsso, 
				SSS_S5_CLEANUP, smc);
			break;
	}
}

static void
cluster_target_socket_session_s2_ready_wait(cluster_target_session_socket_t *tsso,
	cluster_target_socket_session_sm_ctx_t *smc)
{
	cluster_target_session_socket_conn_t *tssp = smc->smc_extinfo;

	switch (smc->smc_event) {
		case SSE_TSSP_READY:
			mutex_enter(&tsso->tsso_mtx);
			tsso->tsso_s4_cnt++;
			if (tsso->tsso_s4_cnt == tsso->tsso_tssp_cnt)
				cluster_target_socket_session_new_state(tsso, 
					SSS_S3_READY, smc);
			mutex_exit(&tsso->tsso_mtx);
			break;
		case SSE_TSSP_COMPLETE:	/* connect appear errors */
			mutex_enter(&tsso->tsso_mtx);
			if (tssp->tssp_connect_status != CLUSTER_STATUS_SUCCESS)
				tsso->tsso_status = tssp->tssp_connect_status;
			mutex_exit(&tsso->tsso_mtx);
			/* FALLTHROUGH */
		case SSE_TPSO_COMPLETE:
			cluster_target_socket_session_new_state(tsso, 
				SSS_S5_CLEANUP, smc);
			break;
	}
}

static void
cluster_target_socket_session_s3_ready(cluster_target_session_socket_t *tsso,
	cluster_target_socket_session_sm_ctx_t *smc)
{
	switch (smc->smc_event) {
		case SSE_TPSO_COMPLETE:
			cluster_target_socket_session_new_state(tsso, 
				SSS_S5_CLEANUP, smc);
			break;
			/* FALLTHROUGH */
		case SSE_TSSP_COMPLETE:	/* peer conn lost */
			cluster_target_socket_session_new_state(tsso, 
				SSS_S4_IN_TSSP_CLEAN, smc);
			break;
	}
}

static void
cluster_target_socket_session_s4_in_tssp_clean(cluster_target_session_socket_t *tsso,
	cluster_target_socket_session_sm_ctx_t *smc)
{
	cluster_target_session_socket_conn_t *tssp = smc->smc_extinfo;

	switch (smc->smc_event) {
		case SSE_TSSP_COMPLETE:	
			mutex_enter(&tsso->tsso_mtx);
			if (tssp->tssp_connect_status != CLUSTER_STATUS_SUCCESS)
				tsso->tsso_status = tssp->tssp_connect_status;
			mutex_exit(&tsso->tsso_mtx);
			cluster_target_socket_session_new_state(tsso,
				SSS_S4_IN_TSSP_CLEAN, smc);
			break;
		case SSE_TSSP_READY:
			mutex_enter(&tsso->tsso_mtx);
			tsso->tsso_s4_cnt++;
			if (tsso->tsso_s4_cnt == tsso->tsso_tssp_cnt)
				cluster_target_socket_session_new_state(tsso, 
					SSS_S3_READY, smc);
			mutex_exit(&tsso->tsso_mtx);
			break;
	}
}

static void
cluster_target_socket_session_s5_cleanup(cluster_target_session_socket_t *tsso,
	cluster_target_socket_session_sm_ctx_t *smc)
{
	cluster_target_session_socket_conn_t *tssp = smc->smc_extinfo;

	switch (smc->smc_event) {
		case SSE_TSSP_COMPLETE:	
			mutex_enter(&tsso->tsso_mtx);
			if (tssp->tssp_connect_status != CLUSTER_STATUS_SUCCESS)
				tsso->tsso_status = tssp->tssp_connect_status;
			mutex_exit(&tsso->tsso_mtx);
			cluster_target_socket_session_new_state(tsso,
				SSS_S4_IN_TSSP_CLEAN, smc);
			break;
	}
}

static void
cluster_target_socket_session_sm_dispatch(cluster_target_socket_session_sm_ctx_t *smc)
{
	cluster_target_session_socket_t *tsso = smc->smc_tsso;

	switch(tsso->tsso_curr_state) {
		case SSS_S1_FREE:
			cluster_target_socket_session_s1_free(tsso, smc);
			break;
		case SSS_S2_READY_WAIT:
			cluster_target_socket_session_s2_ready_wait(tsso, smc);
			break;
		case SSS_S3_READY:
			cluster_target_socket_session_s3_ready(tsso, smc);
			break;
		case SSS_S4_IN_TSSP_CLEAN:
			cluster_target_socket_session_s4_in_tssp_clean(tsso, smc);
			break;
		case SSS_S5_CLEANUP:
			cluster_target_socket_session_s5_cleanup(tsso, smc);
			break;
	}

	cluster_target_socket_session_rele(tsso, smc->smc_tag);
	kmem_free(smc, sizeof(cluster_target_socket_session_sm_ctx_t));
}

static void
cluster_target_socket_session_sm_event_locked(cluster_target_session_socket_t *tsso,
	cluster_target_socket_session_event_e event, void *extptr, void *tag)
{
	cluster_target_socket_session_sm_ctx_t *smc;
	
	ctso_assert(MUTEX_HELD(&tsso->tsso_sm_mtx));
	if ((tsso->tsso_curr_state == SSS_S6_COMPLETE) ||
		(tsso->tsso_curr_state == SSS_S7_DESTROYED))
		return ;

	cluster_target_socket_session_hold(tsso, tag);
	smc = kmem_zalloc(sizeof(cluster_target_socket_session_sm_ctx_t), KM_SLEEP);
	smc->smc_tsso = tsso;
	smc->smc_event = event;
	smc->smc_extinfo = extptr;
	strncpy(smc->smc_tag, tag, 128);
	list_insert_tail(&tsso->tsso_smevt_list, smc);
	
	if (!tsso->tsso_sm_busy) {
		tsso->tsso_sm_busy = B_TRUE;
		while ((smc = list_remove_head(&tsso->tsso_smevt_list)) != NULL) {
			mutex_exit(&tsso->tsso_sm_mtx);
			cluster_target_socket_session_sm_dispatch(smc);
			mutex_enter(&tsso->tsso_sm_mtx);
		}
	}

	tsso->tsso_sm_busy = B_FALSE;	
}

static void
cluster_target_socket_session_sm_event(cluster_target_session_socket_t *tsso,
	cluster_target_socket_session_event_e event, void *extptr, void *tag)
{
	mutex_enter(&tsso->tsso_sm_mtx);
	cluster_target_socket_session_sm_event_locked(tsso, event, extptr, tag);
	mutex_exit(&tsso->tsso_sm_mtx);
}

static void
cluster_target_socket_session_new(cluster_target_port_socket_t *tpso,
	cluster_target_session_t *cts, struct socket *so,
	char *sess_ip, uint16_t sess_port, cluster_target_session_socket_t **tssop)
{
	cluster_target_session_socket_t *tsso;
	cluster_target_session_socket_conn_t *tssp;
	
	ctso_assert(rw_owner(&ctso->ctso_rw) == curthread);
	
	for(tsso = list_head(&ctso->ctso_tsso_list); tsso;
		tsso = list_next(&ctso->ctso_tsso_list, tsso)) {
//		cmn_err(CE_NOTE, "%s %s", sess_ip, tsso->tsso_ipaddr);
		if (strcmp(sess_ip, tsso->tsso_ipaddr) == 0)
			break;
	}

	if (tsso == NULL) {
		tsso = kmem_zalloc(sizeof(cluster_target_session_socket_t), KM_SLEEP);
		cluster_target_socket_refcnt_init(&tsso->tsso_refcnt, tsso);
		mutex_init(&tsso->tsso_mtx, NULL, MUTEX_DEFAULT, NULL);
		cv_init(&tsso->tsso_cv, NULL, CV_DRIVER, NULL);
		cluster_target_socket_port_hold(tpso, CTSO_FTAG);
		tsso->tsso_tpso = tpso;
		tsso->tsso_cts = cts;
		tsso->tsso_tssp_cnt = CTSO_CONN_NUM;
		strncpy(tsso->tsso_ipaddr, sess_ip, 16);
		cluster_target_socket_session_sminit(tsso);
		cluster_target_socket_session_hold(tsso, CTSO_FTAG);
		list_insert_tail(&ctso->ctso_tsso_list, tsso);
		mutex_enter(&tpso->tpso_mtx);
		list_insert_tail(&tpso->tpso_sess_list, tsso);
		mutex_exit(&tpso->tpso_mtx);
	}

	cluster_target_socket_session_conn_new(tsso, so, 
		sess_ip, sess_port, &tssp);

	mutex_enter(&tsso->tsso_mtx);
	tssp->tssp_tsso_idx = tsso->tsso_tssp_idx;
	tsso->tsso_tssp[tsso->tsso_tssp_idx++] = tssp;
	tsso->tsso_tssp_realcnt++;
	mutex_exit(&tsso->tsso_mtx);
	
	*tssop = tsso;
}

static void
cluster_target_socket_session_free(cluster_target_session_socket_t *tsso)
{
	int idx = 0;
	cluster_target_port_socket_t *tpso = tsso->tsso_tpso;
	cluster_target_session_socket_conn_t *tssp;

	ctso_assert(rw_owner(&ctso->ctso_rw) == curthread);
	
	cluster_target_socket_session_sm_event(tsso,
		SSE_TPSO_COMPLETE, NULL, CTSO_FTAG);

	/*
	 * remove from ctso global list
	 */
	cluster_target_socket_session_rele(tsso, CTSO_FTAG);
	list_remove(&ctso->ctso_tsso_list, tsso);
	
	cmn_err(CE_NOTE, "tsso(%s) is going to wait ref", tsso->tsso_ipaddr);
	cluster_target_socket_refcnt_wait_ref(&tsso->tsso_refcnt, CTSO_FTAG);
	cmn_err(CE_NOTE, "tsso(%s) wait ref finished", tsso->tsso_ipaddr);
	cluster_target_socket_port_rele(tsso->tsso_tpso, CTSO_FTAG);
	cluster_target_socket_refcnt_destroy(&tsso->tsso_refcnt);
	kmem_free(tsso, sizeof(cluster_target_session_socket_t));
}

static void
cluster_target_socket_session_release(cluster_target_session_socket_t *tsso)
{
	cluster_target_socket_session_sm_event(tsso,
			SSE_TPSO_COMPLETE, NULL, CTSO_FTAG);
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
	void *rcv_data, void *rcv_hdr, cs_rx_data_t **cs_datap)
{
	cs_rx_data_t *cs_data;
	cluster_san_hostinfo_t *cshi = tsso->tsso_cts->sess_host_private;

	/*
	 * fix txg_sync write big mem data msg. 
	 * 8K lun block, 4K host device block.
	 */
	if (hdrp->total_len > 1024 * 1024) {
		cs_data = cts_rx_data_alloc(0);
		cs_data->data = vmalloc(hdrp->total_len);
		cs_data->data_len = hdrp->total_len;
	} else
		cs_data = cts_rx_data_alloc(hdrp->total_len); 
	cs_data->ex_head = kmem_alloc(hdrp->ex_len, KM_SLEEP); 

	/*
	 * test not to alloc mem when rx data.
	 */
/*	if (hdrp->total_len >= 4 * 1024) {
		cs_data = cts_rx_data_alloc(0);
		cs_data->data = rcv_data;
		cs_data->data_len = hdrp->total_len;
		cs_data->ex_head = rcv_hdr;
	} else {
		cs_data = cts_rx_data_alloc(hdrp->total_len);
		cs_data->ex_head = kmem_alloc(hdrp->ex_len, KM_SLEEP);
	} */
	cs_data->data_index = hdrp->index;
	cs_data->msg_type = hdrp->msg_type;
	cs_data->cs_private = cshi;
	cs_data->need_reply = B_FALSE;
	cs_data->ex_len = hdrp->ex_len;
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

//	cluster_target_socket_session_hold(tsso, CTSO_FTAG);

	worker_idx = cs_data->data_index & (CTSO_PORT_RX_PROCESS_NTHREAD - 1);
	worker = tpso->tpso_rx_process_ctx[worker_idx];
	mutex_enter(&worker->worker_mtx);
	if (!worker->worker_running) {
		mutex_exit(&worker->worker_mtx);
//		cluster_target_socket_session_rele(tsso, CTSO_FTAG);
		csh_rx_data_free_ext(cs_data);
		return ;
	}

	list_insert_tail(worker->worker_list_w, cs_data);
	cv_signal(&worker->worker_cv);
	mutex_exit(&worker->worker_mtx);
}

static cluster_status_t
cluster_target_socket_wait_conn_timeout(cluster_target_session_socket_t *tsso)
{
	mutex_enter(&tsso->tsso_mtx);
	tsso->tsso_status = -ETIMEDOUT;
	cv_signal(&tsso->tsso_cv);
	mutex_exit(&tsso->tsso_mtx);
}

static cluster_status_t
cluster_target_socket_session_init_nonexist(cluster_target_session_socket_t **tssop,
	cluster_target_port_socket_t *tpso, cluster_target_session_t *cts, 
	cluster_target_socket_param_t *param)
{
	cluster_status_t rval;
	struct socket *so;
	cluster_target_session_socket_t *tsso;
	uint16_t srv_port = CTSO_SRV_PORT;
	int idx = 0;

	for (idx = 0; idx < CTSO_CONN_NUM; idx++) {
		cluster_target_socket_create(AF_INET, SOCK_STREAM, 0, &so);
		cluster_target_socket_setsockopt(so);
		cluster_target_socket_session_new(tpso, cts, so, 
			param->ipaddr, srv_port, &tsso);
		strncpy(tsso->tsso_local_ip, param->local, 16);
	}

	cts->sess_target_private = tsso;
	
	*tssop = tsso;
	return CLUSTER_STATUS_SUCCESS;
}

static cluster_status_t
cluster_target_socket_session_init_exist(cluster_target_session_socket_t *tsso,
	cluster_target_session_t *cts, cluster_target_socket_param_t *param)
{
	uint32_t idx = 0;
	cluster_status_t rval = CLUSTER_STATUS_SUCCESS;
	
	/*
	 * wait for peer connect local all successfully
	 */
	mutex_enter(&tsso->tsso_mtx);
	tsso->tsso_wtconn_tmhdl = timeout(cluster_target_socket_wait_conn_timeout, 
		tsso, SEC_TO_TICK(CTSO_CONNECT_TIMEOUT));
	while ((tsso->tsso_tssp_idx != tsso->tsso_tssp_cnt) &&
		(tsso->tsso_status == CLUSTER_STATUS_SUCCESS)) {
		rw_exit(&ctso->ctso_rw);
		cv_wait(&tsso->tsso_cv, &tsso->tsso_mtx);
		rw_enter(&ctso->ctso_rw, RW_WRITER);
	}

	if (tsso->tsso_status != CLUSTER_STATUS_SUCCESS) {
		rval = tsso->tsso_status;
		goto out;
	}
	for ( ; idx < tsso->tsso_tssp_cnt; idx++)
		strncpy(tsso->tsso_tssp[idx]->tssp_local_ip, param->local, 16);
	
	tsso->tsso_cts = cts;
	strncpy(tsso->tsso_local_ip, param->local, 16);

	cts->sess_target_private = tsso;
	/*
	 * other member need be inited.
	 */
	
out:
	mutex_exit(&tsso->tsso_mtx);
	return rval;
}

static int
cluster_target_socket_session_init(cluster_target_session_t *cts, 
	cluster_target_socket_param_t *param)
{
	int rval, idx = 0;
	cluster_target_session_socket_t *tsso;
	cluster_target_session_socket_conn_t *tssp;
	cluster_target_port_socket_t *tpso;
	
	rw_enter(&ctso->ctso_rw, RW_WRITER);
	for (tpso = list_head(&ctso->ctso_tpso_list); tpso;
		tpso = list_next(&ctso->ctso_tpso_list, tpso)) {
		if (strcmp(tpso->tpso_ipaddr, param->local) == 0)
			break;
	}

	if (tpso == NULL) {
		cmn_err(CE_NOTE, "%s ip(%s) is not enabled yet",
			__func__, param->ipaddr);
		rval = -EINVAL;
		goto failed_out;
	}
	
	for (tsso = list_head(&ctso->ctso_tsso_list); tsso;
		tsso = list_next(&ctso->ctso_tsso_list, tsso)) {
		if (strcmp(tsso->tsso_ipaddr, param->ipaddr) == 0)
			break;
	}

	if (tsso != NULL)
		rval = cluster_target_socket_session_init_exist(tsso, cts, param);
	else
		rval = cluster_target_socket_session_init_nonexist(&tsso, tpso, cts, param);

	switch (rval) {
		case CLUSTER_STATUS_SUCCESS:
			cluster_target_socket_session_sm_event(tsso, 
				SSE_USER_CONNECT, NULL, CTSO_FTAG);
			mutex_enter(&tsso->tsso_sm_mtx);
			while (tsso->tsso_curr_state < SSS_S3_READY)
				cv_wait(&tsso->tsso_sm_cv, &tsso->tsso_sm_mtx);
			/*
			 * for exist, the user_connect operate must be successful
			 * once all connections are accepted.
			 * for nonexist, it can be failed because of binding or 
			 * connecting error or timeout.
			 */
			if (tsso->tsso_curr_state > SSS_S3_READY)
				rval = tsso->tsso_status;
			mutex_exit(&tsso->tsso_sm_mtx);
			break;
		default:
			if (tsso != NULL) {
				mutex_enter(&tpso->tpso_mtx);
				list_remove(&tpso->tpso_sess_list, tsso);
				mutex_exit(&tpso->tpso_mtx);
				cluster_target_socket_session_free(tsso);
			}
			break;
	}

failed_out:
	rw_exit(&ctso->ctso_rw);
	return rval;
}

static void
cluster_target_socket_session_destroy(cluster_target_session_t *cts)
{
	/* all sessions are destroyed in port_fini */
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
cluster_target_socket_port_get_info(cluster_target_port_t *ctp, nvlist_t *nvl)
{
	cluster_target_port_socket_t *tpso = ctp->target_private;
	
	VERIFY(nvlist_add_string(nvl, CS_NVL_RPC_IP, tpso->tpso_ipaddr) == 0);
}

static void
cluster_target_socket_port_tran_free(void *fragment)
{
	cluster_target_socket_tran_data_t *tdt = fragment;

	kmem_free(tdt, sizeof(cluster_target_socket_tran_data_t));
}

static void
cluster_target_socket_port_rxframe_to_fragment(cluster_target_port_t *ctp, void *rx_msg)
{
	cmn_err(CE_PANIC, "%s CLUSTER TARGET SOCKET NEVER"
		" COME HERE, NOW PANIC...", __func__);
}

static void
cluster_target_socket_port_fragment_free(void)
{
	cmn_err(CE_PANIC, "%s CLUSTER TARGET SOCKET NEVER"
		" COME HERE, NOW PANIC...", __func__);
}

static void
cluster_target_socket_port_rxmsg_free(void)
{
	cmn_err(CE_PANIC, "%s CLUSTER TARGET SOCKET NEVER"
		" COME HERE, NOW PANIC...", __func__);
}

static int
cluster_target_socket_port_send(void *ctp, void *fragment)
{
	cmn_err(CE_PANIC, "%s CLUSTER TARGET SOCKET NEVER"
		" COME HERE, NOW PANIC...", __func__);
}

static void
cluster_target_socket_port_start_accepter(cluster_target_port_socket_t *tpso)
{
	cluster_target_socket_port_hold(tpso, CTSO_FTAG);
	tpso->tpso_accepter = kthread_run(cluster_target_socket_port_watcher,
		tpso, "tpso_watcher_%d", tpso->tpso_port);
	mutex_enter(&tpso->tpso_mtx);
	while (!tpso->tpso_accepter_running)
		cv_wait(&tpso->tpso_cv, &tpso->tpso_mtx);
	mutex_exit(&tpso->tpso_mtx);
}

static void
cluster_target_socket_port_stop_accepter(cluster_target_port_socket_t *tpso)
{
	/*
	 * wake up kernel_accept
	 */
	mutex_enter(&tpso->tpso_mtx);
	cluster_target_socket_shutdown(tpso->tpso_so);
	tpso->tpso_accepter_running = B_FALSE;
	cv_wait(&tpso->tpso_cv, &tpso->tpso_mtx);
	mutex_exit(&tpso->tpso_mtx);
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

		while ((cs_data = list_remove_head(worker->worker_list_r)) != NULL)
			cluster_san_host_rx_handle(cs_data);

		mutex_enter(&worker->worker_mtx);
	}

	worker->worker_running = B_FALSE;
	worker->worker_stopped = B_TRUE;
	cv_signal(&worker->worker_cv);
	cluster_target_socket_port_rele(tpso, CTSO_FTAG);
	mutex_exit(&worker->worker_mtx);
	cmn_err(CE_NOTE, "TARGET SOCKET RX HANDLE THREAD EXIT");
}

static void
cluster_target_socket_port_new_rx_worker(cluster_target_port_socket_t *tpso)
{
	int idx = 0;
	cluster_target_socket_worker_t *worker;

	for (; idx < CTSO_PORT_RX_PROCESS_NTHREAD; idx++) {
		cluster_target_socket_port_hold(tpso, CTSO_FTAG);
		worker = kmem_zalloc(sizeof(cluster_target_socket_worker_t), KM_SLEEP);
		worker->worker_private = tpso;
		worker->worker_idx = idx;
		worker->worker_list_r = &worker->worker_list1;
		worker->worker_list_w = &worker->worker_list2;
		list_create(&worker->worker_list1, sizeof(cs_rx_data_t),
			offsetof(cs_rx_data_t, node));
		list_create(&worker->worker_list2, sizeof(cs_rx_data_t),
			offsetof(cs_rx_data_t, node));
		mutex_init(&worker->worker_mtx, NULL, MUTEX_DEFAULT, NULL);
		cv_init(&worker->worker_cv, NULL, CV_DRIVER, NULL);
		worker->worker_ctx = kthread_run(cluster_target_socket_port_rx_handle,
			worker, "tpso_rxproc_%p_%d", tpso, idx);
		mutex_enter(&worker->worker_mtx);
		while (!worker->worker_running)
			cv_wait(&worker->worker_cv, &worker->worker_mtx);
		mutex_exit(&worker->worker_mtx);

		tpso->tpso_rx_process_ctx[idx] = worker;
	}
}

static void
cluster_target_socket_port_free_rx_worker(cluster_target_port_socket_t *tpso)
{
	int idx = 0;
	cluster_target_socket_worker_t *worker;

	for (; idx < CTSO_PORT_RX_PROCESS_NTHREAD; idx++) {
		worker = tpso->tpso_rx_process_ctx[idx];
		mutex_enter(&worker->worker_mtx);
		if (worker->worker_running) {
			worker->worker_running = B_FALSE;
			cv_broadcast(&worker->worker_cv);
		}
		mutex_exit(&worker->worker_mtx);
		
		mutex_enter(&worker->worker_mtx);
		while (!worker->worker_stopped)
			cv_wait(&worker->worker_cv, &worker->worker_mtx);
		mutex_exit(&worker->worker_mtx);
		kmem_free(worker, sizeof(cluster_target_socket_worker_t));
		tpso->tpso_rx_process_ctx[idx] = NULL;
	}
}

/*static void
cluster_target_socket_port_new_port(cluster_target_port_socket_t *tpso, 
	uint16_t port, char *ip, cluster_target_port_socket_port_t **tpspp)
{
	cluster_target_port_socket_port_t *tpsp;

	tpsp = kmem_zalloc(sizeof(cluster_target_port_socket_port_t), KM_SLEEP);
	tpsp->tpsp_tpso = tpso;
	tpsp->tpsp_port = port;
	strncpy(tpsp->tpsp_ipaddr, ip, 16);
	mutex_init(&tpsp->tpsp_mtx, NULL, MUTEX_DEFAULT, NULL);
	cv_init(&tpsp->tpsp_cv, NULL, CV_DRIVER, NULL);

	*tpspp = tpsp;
} */

static void
cluster_target_socket_port_new(cluster_target_port_socket_t **tpsop,
	cluster_target_port_t *ctp, char *ip, uint16_t port)
{
	int idx = 0;
	cluster_target_port_socket_t *tpso;

	tpso = kmem_zalloc(sizeof(cluster_target_port_socket_t), KM_SLEEP);
	tpso->tpso_ctp = ctp;
	tpso->tpso_port = port;
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
	cmn_err(CE_NOTE, "tpso is going to wait ref");
	cluster_target_socket_refcnt_wait_ref(&tpso->tpso_refcnt, CTSO_FTAG);
	cmn_err(CE_NOTE, "tpso wait ref finished");
	cluster_target_socket_refcnt_destroy(&tpso->tpso_refcnt);
	kmem_free(tpso, sizeof(cluster_target_port_socket_t));
}

static void
cluster_target_socket_port_watcher(cluster_target_port_socket_t *tpso)
{
	cluster_status_t rval;
	struct socket *new_so;
	cluster_target_session_socket_t *tsso;
	struct sockaddr_in addr_in4;
	int socklen = 0;
	char sess_ip[16] = {0};
	uint16_t sess_port = 0;
	unsigned char *saddr_bep = &addr_in4.sin_addr.s_addr;
	
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

		bzero(&addr_in4, sizeof(struct sockaddr_in));
		kernel_getpeername(new_so, &addr_in4, &socklen);
		snprintf(sess_ip, 16, "%d.%d.%d.%d",
			saddr_bep[0], saddr_bep[1], 
			saddr_bep[2], saddr_bep[3]);
		sess_port = ntohs(addr_in4.sin_port);
		
		cmn_err(CE_NOTE, "new accepted sess_ip(%s) sess_port(%d)", 
			sess_ip, sess_port);

		rw_enter(&ctso->ctso_rw, RW_WRITER);
		cluster_target_socket_session_new(tpso, NULL, new_so, 
			sess_ip, sess_port, &tsso);
		/*
		 * wake up user connect when connections are not all established.
		 */
		cv_signal(&tsso->tsso_cv);
		cluster_target_socket_session_conn_sm_event(
			tsso->tsso_tssp[tsso->tsso_tssp_idx-1], 
			SOE_RX_CONNECT, NULL, CTSO_FTAG);
		rw_exit(&ctso->ctso_rw);
	}

	tpso->tpso_accepter_running = B_FALSE;
	cv_signal(&tpso->tpso_cv);
	mutex_exit(&tpso->tpso_mtx);
	cmn_err(CE_NOTE, "CLUSTER TARGET SOCKET PORT(%d) OFFLINE",
		tpso->tpso_port);
	cluster_target_socket_port_rele(tpso, CTSO_FTAG);
}

static cluster_status_t
cluster_target_socket_port_portal_up(cluster_target_port_socket_t *tpso)
{
	int idx = 0, j;
	cluster_status_t rval;
	struct socket *so;
	struct sockaddr_in addr_in4;

	if ((rval = cluster_target_socket_create(AF_INET,
			SOCK_STREAM, 0, &so)) != CLUSTER_STATUS_SUCCESS) {
		cmn_err(CE_NOTE, "%s socket_create error(%d)", 
			__func__, rval);
		return rval;
	}

	tpso->tpso_so = so;
	cluster_target_socket_setsockopt(tpso->tpso_so);

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

	cluster_target_socket_port_start_accepter(tpso);

	return CLUSTER_STATUS_SUCCESS;
}

static void
cluster_target_socket_port_portal_down(cluster_target_port_socket_t *tpso)
{
	cluster_target_socket_port_stop_accepter(tpso);
	cluster_target_socket_destroy(tpso->tpso_so);
	tpso->tpso_so = NULL;
}

int cluster_target_socket_port_init(cluster_target_port_t *ctp, 
		char *link_name, nvlist_t *nvl_conf)
{
	int rval, link_pri;
	uint16_t srv_port = CTSO_SRV_PORT;
	char *ipaddr;
	cluster_target_port_socket_t *tpso = NULL;

	if ((nvl_conf == NULL) || ((rval = nvlist_lookup_string(
			nvl_conf, "ipaddr", &ipaddr)) != 0)) {
		cmn_err(CE_NOTE, "cluster san socket need a ipaddr at least");
		return EINVAL;
	}

	cmn_err(CE_NOTE, "cluster san socket ip(%s)", ipaddr);

	cluster_target_socket_port_new(&tpso, ctp, ipaddr, srv_port);
	rval = cluster_target_socket_port_portal_up(tpso);
	if (rval != CLUSTER_STATUS_SUCCESS) {
		cluster_target_socket_port_free(tpso);
		return rval;
	}
	
	ctp->f_send_msg = cluster_target_socket_port_send;
	ctp->f_tran_free = cluster_target_socket_port_tran_free;
	ctp->f_tran_fragment = cluster_target_socket_session_tran_fragment;
	ctp->f_session_tran_start = cluster_target_socket_session_tran_start;
	ctp->f_session_init = cluster_target_socket_session_init;
	ctp->f_session_fini = cluster_target_socket_session_destroy;
	ctp->f_rxmsg_to_fragment = cluster_target_socket_port_rxframe_to_fragment;
	ctp->f_fragment_free = cluster_target_socket_port_fragment_free;
	ctp->f_rxmsg_free = cluster_target_socket_port_rxmsg_free;
	ctp->f_cts_compare = cluster_target_socket_session_compare;
	ctp->f_ctp_get_info = cluster_target_socket_port_get_info;
	ctp->f_cts_get_info = cluster_target_socket_session_get_info; 
	ctp->target_private = tpso;

	rw_enter(&ctso->ctso_rw, RW_WRITER);
	cluster_target_socket_port_hold(tpso, CTSO_FTAG);
	list_insert_tail(&ctso->ctso_tpso_list, tpso);
	rw_exit(&ctso->ctso_rw);
	return CLUSTER_STATUS_SUCCESS;
}

void
cluster_target_socket_port_fini(cluster_target_port_t *ctp)
{
	cluster_target_port_socket_t *tpso = ctp->target_private;
	cluster_target_session_socket_t *tsso = NULL;
	
	rw_enter(&ctso->ctso_rw, RW_WRITER);
	cluster_target_socket_port_rele(tpso, CTSO_FTAG);
	list_remove(&ctso->ctso_tpso_list, tpso);
	
	mutex_enter(&tpso->tpso_mtx);
	while ((tsso = list_remove_head(&tpso->tpso_sess_list)) != NULL) {
		mutex_exit(&tpso->tpso_mtx);
		cluster_target_socket_session_free(tsso);
		mutex_enter(&tpso->tpso_mtx);
	}
	mutex_exit(&tpso->tpso_mtx);
	
	cluster_target_socket_port_portal_down(tpso);
	cluster_target_socket_port_free(tpso);
	rw_exit(&ctso->ctso_rw);
}

static int
cluster_target_socket_tx_data_cons(void *hdl, void *arg, int flags)
{
	cluster_target_socket_tran_data_t *tdt = hdl;

	bzero(&tdt->tdt_sohdr, sizeof(struct msghdr));
	tdt->tdt_iovbuflen = sizeof(cluster_target_msg_header_t);
	tdt->tdt_iovlen = 1;
	tdt->tdt_iov[0].iov_base = &tdt->tdt_ct_head;
	tdt->tdt_iov[0].iov_len = tdt->tdt_iovbuflen;
	return 0;
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
	ctso->ctso_tx_data_cache = kmem_cache_create("ctso_tx_data_cache",
		sizeof(cluster_target_socket_tran_data_t), 8, 
		cluster_target_socket_tx_data_cons,
		NULL, NULL, NULL, NULL, 0);
	if (ctso->ctso_tx_data_cache == NULL)
		return -ENOMEM;
	ctso->ctso_refasync_taskq = taskq_create("ctso_refasync_taskq", 1,
		minclsyspri, 8, 16, TASKQ_PREPOPULATE);
	if (ctso->ctso_refasync_taskq == NULL) {
		kmem_cache_destroy(ctso->ctso_tx_data_cache);
		return -EFAULT;
	}
	return CLUSTER_STATUS_SUCCESS;
}
