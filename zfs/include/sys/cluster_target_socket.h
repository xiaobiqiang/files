#ifndef	_SYS_CLUSTER_TARGET_SOCKET_H
#define	_SYS_CLUSTER_TARGET_SOCKET_H
#include <linux/in.h>  
#include <linux/inet.h>  
#include <linux/socket.h>  
#include <net/sock.h>
#include <asm/atomic.h>
#include <sys/callout.h>

#define CTSO_FTAG	__func__
#define CTSO_PORT_RX_PROCESS_NTHREAD	16
#define CTSO_CONN_NUM	4
#define CTSO_SRV_PORT	1866

#define TSSO_SM_AUDIT_DEPTH		64
#define CTSO_REFAUDIT_MAX_RECORD	64

#define CTSO_CONNECT_TIMEOUT	2

#define ctso_assert(cond)	\
	do {	\
		if (!(cond))	\
			spl_panic(__FILE__, __FUNCTION__, __LINE__, \
				"%s", "ctso_assert(" #cond ") failed\n");	\
	} while(0);

#define CTSO_REFAUDIT(refcnt, ftag, type)	{	\
	cluster_target_socket_refaudit_record_t *adr;	\
			\
	adr = (refcnt)->tr_audit.adt_record;		\
	adr += (refcnt)->tr_audit.adt_idx;		\
	(refcnt)->tr_audit.adt_idx++;	\
	adr->adr_nref = (refcnt)->tr_nref;	\
	adr->adr_fn = strdup(ftag);		\
	adr->adr_type = type;	\
	(refcnt)->tr_audit.adt_idx &= (refcnt)->tr_audit.adt_max_depth;	\
}

#define CTSO_SM_AUDIT(sa, event, ftag, last, curr)		\
{	\
	cluster_target_socket_sm_audit_record_t *sar; \
		\
	sar = (sa)->sa_record + (sa)->sa_idx;	\
	(sa)->sa_idx++;		\
	sar->sar_ostate = last;	\
	sar->sar_nstate = curr;	\
	sar->sar_event = event;	\
	sar->sar_ftag = strdup(ftag);	\
	(sa)->sa_idx &= (sa)->sa_max_depth;	\
}

typedef struct cluster_target_port_socket cluster_target_port_socket_t;
typedef struct cluster_target_session_socket_conn cluster_target_session_socket_conn_t;

typedef enum cluster_target_socket_refwait {
	CTSO_REF_NOWAIT,
	CTSO_REF_WAIT_SYNC,
	CTSO_REF_WAIT_ASYNC
} cluster_target_socket_refwait_e;

typedef enum cluster_target_socket_session_conn_event {
	SOE_RX_CONNECT,
	SOE_SESS_CONFLICT,
	SOE_USER_CONNECT,
	SOE_CN_BIND_ERROR,
	SOE_PRPORT_OFFLINE,
	SOE_CONNECT_ERROR,
	SOE_CONNECT_SUCCESS,
	SOE_TSSO_IN_CLEANUP,
	SOE_TSSP_CONN_LOST,
} cluster_target_socket_session_conn_event_e;

typedef enum cluster_target_socket_session_event {
	SSE_USER_CONNECT,
	SSE_TSSP_READY,
	SSE_TSSP_COMPLETE,
	SSE_TPSO_COMPLETE,
} cluster_target_socket_session_event_e;

typedef void (*cluster_target_socket_refcnt_cb_t)(void *);
typedef int cluster_status_t;
#define CLUSTER_STATUS_SUCCESS 0

typedef struct cluster_target_socket_refaudit_record {
	uint32_t adr_nref;
	char *adr_fn;
	char *adr_type;
} cluster_target_socket_refaudit_record_t;

typedef struct cluster_target_socket_refaudit {
	uint32_t adt_idx;
	uint32_t adt_max_depth;
	cluster_target_socket_refaudit_record_t adt_record[CTSO_REFAUDIT_MAX_RECORD];
} cluster_target_socket_refaudit_t;

typedef struct cluster_target_socket_refcnt {
	cluster_target_socket_refwait_e tr_wait;
	uint32_t tr_nref;
	void *tr_refcnt_obj;
	cluster_target_socket_refcnt_cb_t tr_fn;
	kmutex_t tr_mtx;
	kcondvar_t tr_cv;
	cluster_target_socket_refaudit_t tr_audit;
} cluster_target_socket_refcnt_t;

typedef struct cluster_target_socket_sm_audit_record {
	uint16_t sar_ostate;
	uint16_t sar_nstate;
	uint32_t sar_event;
	char *sar_ftag;
} cluster_target_socket_sm_audit_record_t;

typedef struct cluster_target_socket_sm_audit {
	uint32_t sa_idx;
	uint32_t sa_max_depth;
	cluster_target_socket_sm_audit_record_t sa_record[CTSO_REFAUDIT_MAX_RECORD];
} cluster_target_socket_sm_audit_t;

typedef enum cluster_target_socket_session_conn_state {
	SOS_S1_FREE,
	SOS_S2_XPRT_WAIT,
	SOS_S3_CONN_SND,
	SOS_S4_WAIT_PRPORT,
	SOS_S4_XPRT_UP,
	SOS_S5_IN_EPIPE,
	SOS_S6_IN_LOGOUT,
	SOS_S7_CLEANUP,
	SOS_S8_COMPLETE,
} cluster_target_socket_session_conn_state_e;

typedef enum cluster_target_socket_session_state {
	SSS_S1_FREE,
	SSS_S2_READY_WAIT,
	SSS_S3_READY,
	SSS_S4_IN_TSSP_CLEAN,
	SSS_S5_CLEANUP,
	SSS_S6_COMPLETE,	/* going to destroy */
	SSS_S7_DESTROYED	/* destroyed */
} cluster_target_socket_session_state_e;

typedef struct cluster_target_socket_worker {
	list_t			*worker_list_r;
	list_t			*worker_list_w;
	list_t 			worker_list1;
	list_t 			worker_list2;
	kmutex_t		worker_mtx;
	kcondvar_t		worker_cv;
	boolean_t		worker_running;
	boolean_t 		worker_stopped;
	struct task_struct *worker_ctx;
	void 			*worker_private;
	uint32_t 		worker_idx;
} cluster_target_socket_worker_t;

typedef struct cluster_target_port_socket_port {
	char tpsp_ipaddr[16];
    uint32_t tpsp_port;

	struct socket *tpsp_so;
	kmutex_t tpsp_mtx;
	kcondvar_t tpsp_cv;
	struct task_struct *tpsp_accepter;
	boolean_t tpsp_accepter_running;

	cluster_target_port_socket_t *tpsp_tpso;
} cluster_target_port_socket_port_t;

struct cluster_target_port_socket {
	/*
	 * new design
	 */
	list_node_t tpso_node;
	cluster_target_socket_refcnt_t tpso_refcnt;
	cluster_target_port_t *tpso_ctp;
	char tpso_ipaddr[16];
    uint32_t tpso_port;
	boolean_t tpso_accepter_running;
	struct socket *tpso_so;
	struct task_struct *tpso_accepter;
	kmutex_t tpso_mtx;
	kcondvar_t tpso_cv;
	list_t tpso_sess_list;
	cluster_target_socket_worker_t *tpso_rx_process_ctx[CTSO_PORT_RX_PROCESS_NTHREAD];
};

typedef struct cluster_target_socket_param {
   char hostname[256];
   char ipaddr[16];
   char local[16];
   int hostid;
   int priority;
} cluster_target_socket_param_t;

#define TSSO_STATUS_OK			CLUSTER_STATUS_SUCCESS
#define TSSO_STATUS_IN_CLEAN	0x1
#define TSSO_STATUS_CAN_FREE	0x2

typedef struct cluster_target_session_socket {
	/*
	 * new design
	 */
	list_node_t tsso_node;
	list_node_t tsso_tpso_node;
	cluster_target_socket_refcnt_t tsso_refcnt;
	cluster_target_port_socket_t *tsso_tpso;
	cluster_target_session_t *tsso_cts;
	char tsso_local_ip[16];
	char tsso_ipaddr[16];

	kmutex_t tsso_mtx;
	kcondvar_t tsso_cv;
	uint32_t tsso_status;
	uint32_t tsso_s4_cnt;
	uint32_t tsso_tssp_cnt;
	uint32_t tsso_tssp_idx;
	uint32_t tsso_tssp_realcnt;
	uint32_t tsso_align1;
	cluster_target_session_socket_conn_t *tsso_tssp[CTSO_CONN_NUM];
	timeout_id_t tsso_wtconn_tmhdl;

	/*
     * socket session state machine
     */
    kmutex_t tsso_sm_mtx;
    kcondvar_t tsso_sm_cv;
	list_t tsso_smevt_list;
    cluster_target_socket_sm_audit_t tsso_sm_audit;
    cluster_target_socket_session_state_e tsso_curr_state;
    cluster_target_socket_session_state_e tsso_last_state;
	boolean_t tsso_sm_busy;
	uint32_t tsso_align;
} cluster_target_session_socket_t;

struct cluster_target_session_socket_conn {
    cluster_target_session_socket_t *tssp_tsso;
    struct socket *tssp_so;
    char tssp_local_ip[16];
    char tssp_ipaddr[16];
    uint32_t tssp_port;
	cluster_status_t tssp_connect_status;
    timeout_id_t tssp_conn_tmhdl;
	
    boolean_t tssp_rx_running;
	boolean_t tssp_rx_stoped;
    kmutex_t tssp_rx_mtx;
    kcondvar_t tssp_rx_cv;
    struct task_struct *tssp_rx;
    list_t tssp_rx_data_list;

	boolean_t tssp_tx_running;
	boolean_t tssp_tx_stoped;
	kmutex_t tssp_tx_mtx;
	kcondvar_t tssp_tx_cv;
	struct task_struct *tssp_tx;
	list_t tssp_tx_wait_list;
	list_t tssp_tx_live_list;

    /*
     * socket session connection state machine
     */
    kmutex_t tssp_sm_mtx;
    kcondvar_t tssp_sm_cv;
    cluster_target_socket_sm_audit_t tssp_sm_audit;
    taskq_t *tssp_sm_ctx;
    cluster_target_socket_session_conn_state_e tssp_curr_state;
    cluster_target_socket_session_conn_state_e tssp_last_state;

	uint32_t tssp_tsso_idx;
};

typedef struct cluster_target_socket_tran_data {
	list_node_t tdt_node;
	cluster_target_session_socket_conn_t *tdt_tssp;
	/*
	 * iov[0] = ct_head
	 * iov[1] = ex_head
	 * iov[2] = data
	 */
	cluster_target_msg_header_t tdt_ct_head;
	struct msghdr tdt_sohdr;
	struct kvec tdt_iov[3];
	uint32_t tdt_iovlen;
	uint32_t tdt_iovbuflen;
	
	kmutex_t *tdt_mtx;
	kcondvar_t *tdt_cv;
	boolean_t tdt_waiting;

	uint32_t tdt_tran_idx;
} cluster_target_socket_tran_data_t;

int cluster_target_socket_port_init(
	cluster_target_port_t *ctp, char *link_name, nvlist_t *nvl_conf);
void cts_socket_hb_init(cluster_target_session_t *cts);
void cluster_target_socket_port_fini(cluster_target_port_t *ctp);
void cts_socket_init(void);
cluster_status_t cluster_target_socket_init(void);
#endif


