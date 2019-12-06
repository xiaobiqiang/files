#ifndef	_SYS_CLUSTER_TARGET_SOCKET_H
#define	_SYS_CLUSTER_TARGET_SOCKET_H
#include <linux/in.h>  
#include <linux/inet.h>  
#include <linux/socket.h>  
#include <net/sock.h>
#include <asm/atomic.h>
#include <sys/callout.h>

#define CTSO_FTAG	__func__

#define TSSO_SM_AUDIT_DEPTH		64

#define CTSO_REFAUDIT_MAX_RECORD
#define ctso_assert(cond)	\
	do {	\
		if (!(cond))	\
			spl_panic(__FILE__, __FUNCTION__, __LINE__, \
				"%s", "ctso_assert(" #cond ") failed\n");	\
	} while(0);

#define CTSO_REFAUDIT(refcnt, ftag, type)	{	\
	cluster_target_socket_refaudit_record_t *adr;	\
	uint32_t adr_offset;	\
	adr_offset = (refcnt)->tr_audit.adt_idx & (refcnt)->tr_audit.adt_max_depth;	\
	adr = &(refcnt)->tr_audit.adt_record[0] + adr_offset;	\
	adr->adr_nref = (refcnt)->tr_nref;	\
	adr->adr_fn = strdup(ftag);		\
	adr->adr_type = type;	\
	(refcnt)->tr_audit.adt_idx++;	\
}

#define CTSO_SESS_SM_AUDIT(tsso, event, ftag)		\
{	\
	cluster_target_socket_sm_audit_t *sa = &(tsso)->tsso_sm_audit;	\
	int idx = sa->sa_idx & sa->sa_max_depth;	\
	cluster_target_socket_sm_audit_record_t *sar = &sa->sa_record[0] + idx;	\
	sar->sar_ostate = tsso->tsso_last_state;	\
	sar->sar_nstate = tsso->tsso_curr_state;	\
	sar->sar_event = event;	\
	sar->sar_ftag = strdup(ftag);	\
	sa->sa_idx++;	\
}

typedef enum cluster_target_socket_refwait {
	CTSO_REF_NOWAIT,
	CTSO_REF_WAIT_SYNC,
	CTSO_REF_WAIT_ASYNC
} cluster_target_socket_refwait_e;

typedef enum cluster_target_socket_session_event {
	SOE_RX_CONNECT,
	SOE_SESS_CONFLICT,
	SOE_USER_CONNECT,
	SOE_CN_BIND_ERROR,
	SOE_CONNECT_ERROR,
	SOE_CONNECT_SUCCESS,
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

typedef enum cluster_target_socket_session_state {
	SOS_S1_FREE,
	SOS_S2_XPRT_WAIT,
	SOS_S3_CONN_SND,
	SOS_S4_XPRT_UP,
	SOS_S5_CLEANUP,
} cluster_target_socket_session_state_e;

typedef struct cluster_target_socket_worker {
	list_t			worker_list_r;
	list_t			worker_list_w;
	kmutex_t		worker_mtx;
	kcondvar_t		worker_cv;
	boolean_t		worker_running;
	boolean_t 		worker_stopped;
	uint32_t		worker_ntasks;
	struct task_struct *worker_ctx;
	void 			*worker_private;
} cluster_target_socket_worker_t;

typedef struct cluster_target_port_socket {
	/*
	 * new design
	 */
	list_node_t tpso_node;
	cluster_target_socket_refcnt_t tpso_refcnt;
	list_t tpso_sess_list;
	cluster_target_port_t *tpso_ctp;
	char tpso_ipaddr[16];
    uint32_t tpso_port;
	struct socket *tpso_so;
	kmutex_t tpso_mtx;
	kcondvar_t tpso_cv;
	struct task_struct *tpso_accepter;
	boolean_t tpso_accepter_running;

	/*
	 * for session rx handle
	 */
	uint32_t tpso_rx_process_nthread;
	cluster_target_socket_worker_t *tpso_rx_process_ctx;
} cluster_target_port_socket_t;

typedef struct cluster_target_socket_param {
   char hostname[256];
   char ipaddr[16];
   char local[16];
   int port;
   int hostid;
   int priority;
} cluster_target_socket_param_t;

typedef struct cluster_target_session_socket {
	/*
	 * new design
	 */
	list_node_t tsso_node;
	list_node_t tsso_tpso_node;
	cluster_target_socket_refcnt_t tsso_refcnt;
	char tsso_local_ip[16];
	char tsso_ipaddr[16];
	uint32_t tsso_port;
	cluster_target_port_socket_t *tsso_tpso;
	cluster_target_session_t *tsso_cts;
	struct socket *tsso_so;
	kmutex_t tsso_rx_mtx;
	kcondvar_t tsso_rx_cv;
	struct task_struct *tsso_rx;
	list_t tsso_rx_data_list;
	boolean_t tsso_rx_running;

	/*
	 * socket session state machine
	 */
	kmutex_t tsso_sm_mtx;
	kcondvar_t tsso_sm_cv;
	cluster_target_socket_sm_audit_t tsso_sm_audit;
	taskq_t *tsso_sm_ctx;
	cluster_target_socket_session_state_e tsso_curr_state;
	cluster_target_socket_session_state_e tsso_last_state;

	cluster_status_t tsso_connect_status;
	timeout_id_t tsso_conn_tmhdl;
} cluster_target_session_socket_t;

int cluster_target_socket_port_init(
	cluster_target_port_t *ctp, char *link_name, nvlist_t *nvl_conf);
void cts_socket_hb_init(cluster_target_session_t *cts);
void cluster_target_socket_port_fini(cluster_target_port_t *ctp);
void cts_socket_init(void);
cluster_status_t cluster_target_socket_init(void);
#endif

