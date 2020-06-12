#ifndef __MLT_SAS_H
#define __MLT_SAS_H

#include <linux/errno.h>
#include <linux/blkdev.h>
#include <linux/sched.h>
#include <linux/workqueue.h>
#include <linux/delay.h>
#include <linux/gfp.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/miscdevice.h>
#include <linux/blk_types.h>
#include <linux/bio.h>
#include <linux/blkdev.h>
#include <sys/cluster_san.h>
#include <sys/modhash.h>
#include <sys/callout.h>
#include <sys/kstat.h>
#include <mltsas_comm.h>

#define Mlsas_MAJOR			299

#define Mlsas_Mxbio_Size	(1 << 20)
#define Mlsas_Mxbio_Segment	128
#define Mlsas_DMA_Alimask	0x03

#define Mlsas_Delayed_RQ	0x01
#define Mlsas_Delayed_PR_RQ	0x02

#define Mlsas_Module_Name	"Mlsas"

/*
 * use clustersan
 */
#define Mlsas_Noma_Session	CLUSTER_SAN_BROADCAST_SESS

#define Mlsas_St_Disabled	0x01
#define Mlsas_St_Enabling	0x02
#define Mlsas_St_Enabled	0x03
#define Mlsas_St_Busy		0x04

#define Mlsas_RQ_Write			0x00000001
#define Mlsas_RQ_ReadA			0x00000002
#define Mlsas_RQ_Local_Pending	0x00000004
#define Mlsas_RQ_Net_Pending	0x00000008
#define Mlsas_RQ_Net_Queued		0x00000010
#define Mlsas_RQ_Net_Sent		0x00000020
#define Mlsas_RQ_Local_Done		0x00000040
#define Mlsas_RQ_Local_OK		0x00000080
#define Mlsas_RQ_Local_Aborted	0x00000100
#define Mlsas_RQ_Net_Done		0x00000200
#define Mlsas_RQ_Net_OK			0x00000400
#define Mlsas_RQ_Net_Aborted	0x00000800
#define Mlsas_RQ_Delayed		0x80000000

#define Mlsas_Rst_New				0x00
#define Mlsas_Rst_Submit_Local		0x01
#define Mlsas_Rst_Submit_Net		0x02
#define Mlsas_Rst_Queue_Net_Write	0x03
#define Mlsas_Rst_Queue_Net_Read	0x04
#define Mlsas_Rst_Net_Send_Error	0x05
#define Mlsas_Rst_Net_Send_OK		0x06
#define Mlsas_Rst_Net_Submitted		0x07
#define Mlsas_Rst_Net_Inflight		0x08
#define Mlsas_Rst_Net_Ack			0x09
#define Mlsas_Rst_Complete_OK		0x0A
#define Mlsas_Rst_Read_Error		0x0B
#define Mlsas_Rst_ReadA_Error		0x0C
#define Mlsas_Rst_Write_Error		0x0D
#define Mlsas_Rst_Discard_Error		0x0E
#define Mlsas_Rst_PR_Write_Error	0x0F
#define Mlsas_Rst_PR_Read_Error		0x10
#define Mlsas_Rst_PR_ReadA_Error	0x11
#define Mlsas_Rst_PR_Complete_OK	0x12
#define Mlsas_Rst_Done				0x13
#define Mlsas_Rst_Abort_Diskio		0x14
#define Mlsas_Rst_Abort_Netio		0x15

#define Mlsas_PRRst_Submit_Local 	0x01
#define Mlsas_PRRst_Discard_Error	0x02
#define Mlsas_PRRst_Write_Error		0x03
#define Mlsas_PRRst_Read_Error		0x04
#define Mlsas_PRRst_ReadA_Error		0x05
#define Mlsas_PRRst_Complete_OK		0x06
#define Mlsas_PRRst_Subimit_Net		0x07
#define Mlsas_PRRst_Queue_Net_Wrsp	0x08
#define Mlsas_PRRst_Queue_Net_Rrsp	0x09
#define Mlsas_PRRst_Send_Error		0x0A
#define Mlsas_PRRst_Send_OK			0x0B
#define Mlsas_PRRst_Abort_Local		0x0C
#define Mlsas_PRRst_Abort_Net		0x0D

#define Mlsas_PRRfl_Addl_Kmem 		0x0001
#define Mlsas_PRRfl_Ee_Error 		0x0002
#define Mlsas_PRRfl_Zero_Out		0x0004
#define Mlsas_PRRfl_Write			0x0008
#define Mlsas_PRRfl_Local_Pending	0x0010
#define Mlsas_PRRfl_Net_Pending		0x0020
#define Mlsas_PRRfl_Net_Queued		0x0040
#define Mlsas_PRRfl_Net_Sent		0x0080
#define Mlsas_PRRfl_Local_Done		0x0100
#define Mlsas_PRRfl_Local_OK		0x0200
#define Mlsas_PRRfl_Net_OK			0x0400
#define Mlsas_PRRfl_Local_Aborted	0x0800
#define Mlsas_PRRfl_Capa_Abort		0x1000
#define Mlsas_PRRfl_Net_Aborted		0x2000
#define Mlsas_PRRfl_Continue		0x40000000
#define Mlsas_PRRfl_Delayed			0x80000000


#define Mlsas_Devevt_To_Attach		0x01
#define Mlsas_Devevt_Attach_Error	0x02
#define Mlsas_Devevt_Attach_OK		0x03
#define Mlsas_Devevt_Attach_Local	0x04
#define Mlsas_Devevt_PR_Attach		0x05
#define Mlsas_Devevt_Error_Switch	0x06
#define Mlsas_Devevt_PR_Error_Sw	0x07
#define Mlsas_Devevt_PR_Disconnect	0x08
#define Mlsas_Devevt_PR_Down2up		0x09
#define Mlsas_Devevt_Hard_Stchg		0x0A
#define Mlsas_Devevt_Last			0x10

#define Mlsas_PRevt_Attach_OK		0x01
#define Mlsas_PRevt_Error_Sw		0x02
#define	Mlsas_PRevt_Disconnect		0x03
#define Mlsas_PRevt_PR_Down2up		0x04
#define Mlsas_PRevt_Hardly_Update	0x05
#define Mlsas_PRevt_Last			0x10

#define __Mlsas_Get_ldev_if_state(Mlb, __minSt)	\
	((Mlb)->Mlb_st >= (__minSt))
#define __Mlsas_Get_ldev_if_state_between(Mlb, __minSt, __maxSt)	\
	(((Mlb)->Mlb_st >= (__minSt)) && ((Mlb)->Mlb_st <= (__maxSt)))
#define __Mlsas_Get_ldev(Mlb)		\
	__Mlsas_Get_ldev_if_state((Mlb), Mlsas_Devst_Degraded)

#define __Mlsas_Get_PR_if_state(pr, __minSt)	\
	((pr)->Mlpd_st >= (__minSt))
#define __Mlsas_Get_PR_if_not_state(pr, __maxSt)	\
		((pr)->Mlpd_st <= (__maxSt))

typedef enum Mlsas_tst Mlsas_tst_e;
typedef enum Mlsas_ttype Mlsas_ttype_e;
typedef struct Mlsas_backdev_info Mlsas_backdev_info_t;
typedef struct Mlsas_blkdev Mlsas_blkdev_t;
typedef struct Mlsas_pr_device Mlsas_pr_device_t;
typedef struct Mlsas_delayed_obj Mlsas_Delayed_obj_t;
typedef struct Mlsas_request Mlsas_request_t;
typedef struct Mlsas_pr_req Mlsas_pr_req_t;
typedef struct Mlsas_pr_req_free Mlsas_pr_req_free_t;
typedef struct Mlsas_bio_and_error Mlsas_bio_and_error_t;
typedef struct Mlsas_rh Mlsas_rh_t;
typedef struct Mlsas_retry Mlsas_retry_t;
typedef struct Mlsas_rtx_wk Mlsas_rtx_wk_t;
typedef struct Mlsas_rtx_wq Mlsas_rtx_wq_t;
typedef struct Mlsas_thread Mlsas_thread_t;
typedef struct Mlsas_cs_rx_data Mlsas_cs_rx_data_t;
typedef struct Mlsas Mlsas_t;

enum Mlsas_tst {
	Mt_None,
	Mt_Run,
	Mt_Exit,
	Mt_Restart
};

enum Mlsas_ttype {
	Mtt_First,
	Mtt_Tx,
	Mtt_Rx,
	Mtt_Asender,
	Mtt_Sender,
	Mtt_Last
};

struct Mlsas_backdev_info {
	/*
	 * disk path to write
	 */
	char Mlbd_path[128];
	/*
	 * attached partion
	 */
	struct block_device *Mlbd_part_bdev;
	/*
	 * used for total disk submit_bio
	 */
	struct block_device *Mlbd_bdev;
	/*
	 * total disk
	 */
	struct gendisk *Mlbd_gdisk;
	/*
	 * requests submitted
	 */
	list_t Mldb_pending_rqs;
};

struct Mlsas_pr_device {
	list_node_t Mlpd_node;
	list_node_t Mlpd_rh_node;
	struct kref Mlpd_ref;

	list_t Mlpd_rqs;
	list_t Mlpd_pr_rqs;
	list_t Mlpd_net_pr_rqs;

	wait_queue_head_t Mlpd_wait;
	
	Mlsas_blkdev_t *Mlpd_mlb;
	Mlsas_rh_t *Mlpd_rh;
	uint32_t Mlpd_hostid;

	Mlsas_devst_e Mlpd_st;
	uint64_t Mlpd_pr;

	uint32_t Mlpd_switch;
	uint32_t Mlpd_error_now;
};

struct Mlsas_rtx_wk {
	list_node_t rtw_node;
	int (*rtw_fn)(Mlsas_rtx_wk_t *);
};

struct Mlsas_rtx_wq {
	spinlock_t rtq_lock;
	list_t rtq_list;
	wait_queue_head_t rtq_wake;
};

struct Mlsas_thread {
	spinlock_t Mt_lock;
	struct task_struct *Mt_w;
	struct completion Ml_stop;
	Mlsas_tst_e Mt_state;
	int (*Mt_fn) (Mlsas_thread_t *);
//	int reset_cpu_mask;
	const char *Mt_name;
	Mlsas_ttype_e Mt_type;
};

struct Mlsas_blkdev {
	uint64_t Mlb_hashkey;	/* const */
	uint64_t Mlb_nopen;
	char Mlb_scsi[32];
	struct kref Mlb_ref;
	spinlock_t Mlb_rq_spin;
	struct request_queue *Mlb_rq;
	struct block_device *Mlb_this;
	struct gendisk *Mlb_gdisk;
	Mlsas_backdev_info_t Mlb_bdi;
	Mlsas_devst_e	Mlb_st;
	list_t Mlb_pr_devices;
	list_t Mlb_local_rqs;
	list_t Mlb_peer_rqs;
	list_t Mlb_topr_rqs;
	list_t Mlb_net_pr_rqs;

	wait_queue_head_t Mlb_wait;

	struct sg_table *Mlb_w_sgl;
	void *Mlb_txbuf;
	uint32_t Mlb_txbuf_len;
	uint32_t Mlb_txbuf_used;

	void *Mlb_astx_buf;
	uint32_t Mlb_astxbuf_len;
	uint32_t Mlb_astxbuf_used;

	void *Mlb_stx_buf;
	uint32_t Mlb_stxbuf_len;
	uint32_t Mlb_stxbuf_used;
	
	Mlsas_thread_t Mlb_tx;
	Mlsas_rtx_wq_t Mlb_tx_wq;
	
	Mlsas_thread_t Mlb_rx;
	Mlsas_rtx_wq_t Mlb_rx_wq;

	Mlsas_thread_t Mlb_asender;
	Mlsas_rtx_wq_t Mlb_asender_wq;

	Mlsas_thread_t Mlb_sender;
	Mlsas_rtx_wq_t Mlb_sender_wq;
	
	uint32_t Mlb_attach_status;
	
	uint32_t Mlb_npending;

	uint32_t Mlb_switch;
	uint32_t Mlb_error_cnt;

	boolean_t Mlb_in_resume_virt;
};

struct Mlsas_delayed_obj {
	uint32_t dlo_magic;
	list_node_t dlo_node;
};

struct Mlsas_request {
	uint32_t Mlrq_delayed_magic;
	list_node_t Mlrq_delayed_node;
	list_node_t Mlrq_node;
	list_node_t Mlrq_pr_node;
	struct kref Mlrq_ref;
	Mlsas_blkdev_t *Mlrq_bdev;
	uint32_t Mlrq_flags;
	uint32_t Mlrq_state;
	uint64_t Mlrq_start_jif;
	uint64_t Mlrq_submit_jif;
	struct bio *Mlrq_master_bio;
	struct bio *Mlrq_back_bio;
	uint32_t Mlrq_completion_ref;

	Mlsas_pr_device_t *Mlrq_pr;
	Mlsas_rtx_wk_t Mlrq_wk;

	uint32_t Mlrq_switch;
	uint32_t Mlrq_error_now;
	
	/*
	 * locate
	 */
	uint32_t Mlrq_bsize;
	uint32_t Mlrq_bflags;
	sector_t Mlrq_sector;

	timeout_id_t Mlrq_tm;
};

struct Mlsas_pr_req {
	uint32_t prr_delayed_magic;
	list_node_t prr_delayed_node;
	list_node_t prr_node;
	list_node_t prr_mlb_node;
	list_node_t prr_net_node;
	list_node_t prr_mlb_net_node;
	struct kref prr_ref;
	Mlsas_rtx_wk_t prr_wk;
	Mlsas_pr_device_t *prr_pr;
	uint32_t prr_completion_ref;
	uint32_t prr_flags;
	uint32_t prr_bsize;
	uint64_t prr_bsector;
	uint64_t prr_pr_rq;

	void *prr_dt;
	uint32_t prr_dtlen;
	
	uint32_t prr_pending_bios;
	int prr_error;
};

struct Mlsas_cs_rx_data {
	/* MUST BE FIRST */
	union {
		cs_rx_data_t u_cs;
		struct {
			Mlsas_rtx_wk_t u_rtx;
			uint64_t		data_index;
			uint64_t		data_len;
			uint8_t			msg_type;
			uint8_t			need_reply;
			uint8_t			pad[6];
			uint64_t		ex_len;
			void			*ex_head;
			char			*data;
			void 			*cs_private;
		} u_ms;
	} u;
	/* EXT */
};

/*
 * bio source complete
 */
struct Mlsas_bio_and_error {
	struct bio *Mlbi_bio;
	int Mlbi_error;
	uint32_t k_put;
};

struct Mlsas_pr_req_free {
	uint32_t k_put;
	uint32_t can_free;
};

struct Mlsas_retry {
	struct workqueue_struct *Mlt_workq;
	struct work_struct Mlt_work;
	spinlock_t Mlt_lock;
	list_t Mlt_writes;
};

#define Mlsas_RHS_New			0x01
#define Mlsas_RHS_Corrupt		0x02		
struct Mlsas_rh {
	uint32_t Mh_hostid;
	uint32_t Mh_state;
	struct kref Mh_ref;
	kmutex_t Mh_mtx;
	list_t Mh_devices;
	void *Mh_session;
};

typedef struct Mlsas_stat {
	kstat_named_t Mlst_virt_alloc_m;
	kstat_named_t Mlst_rhost_alloc_m;
	kstat_named_t Mlst_pr_alloc_m;
	kstat_named_t Mlst_req_alloc_m;
	kstat_named_t Mlst_pr_rq_alloc_m;
	kstat_named_t Mlst_virt_kref_m;
	kstat_named_t Mlst_rhost_kref_m;
	kstat_named_t Mlst_pr_kref_m;
	kstat_named_t Mlst_req_kref_m;
	kstat_named_t Mlst_pr_rq_kref_m;
} Mlsas_stat_t;

struct Mlsas {
	uint32_t Ml_state;
	uint32_t Ml_minor;
	kmutex_t Ml_mtx;
	mod_hash_t *Ml_devices;
	mod_hash_t *Ml_rhs;
	struct kmem_cache *Ml_skc;
	mempool_t *Ml_request_mempool;
	struct kmem_cache *Ml_prr_skc;
	mempool_t *Ml_prr_mempool;
	Mlsas_retry_t Ml_retry;
	taskq_t *Ml_async_tq;

	kstat_t *Ml_kstat;
};

extern Mlsas_stat_t Mlsas_stat;

#define __Mlsas_Bump(stat)	atomic_add_64(&Mlsas_stat.Mlst_##stat##_m.value.ui64, 1);
#define __Mlsas_Down(stat)	atomic_dec_64(&Mlsas_stat.Mlst_##stat##_m.value.ui64);
#define __Mlsas_Sub(stat, n)	atomic_sub_64(&Mlsas_stat.Mlst_##stat##_m.value.ui64, n);


/* =====================Define Message Type Struct=========================== */

#define Mlsas_Mms_Magic			0x2020

/* Message Type Define */
#define Mlsas_Mms_None				0x00
#define Mlsas_Mms_Attach			0x01
#define Mlsas_Mms_Down2Up_Attach	0x02
#define Mlsas_Mms_Bio_RW			0x03
#define Mlsas_Mms_Brw_Rsp			0x04
#define Mlsas_Mms_State_Change		0x05
#define Mlsas_Mms_Last				0x10

#define Mlsas_RXfl_Sync			0x01
#define Mlsas_RXfl_Fua			0x02
#define Mlsas_RXfl_Flush		0x04
#define Mlsas_RXfl_Disc			0x08
#define Mlsas_RXfl_Write		0x10

typedef struct Mlsas_Msh Mlsas_Msh_t;
typedef void (*Mlsas_RX_pfn_t)(Mlsas_Msh_t *, cs_rx_data_t *);

struct Mlsas_Msh {
	Mlsas_rtx_wk_t Mms_wk;
	Mlsas_rh_t	*Mms_rh;
	uint16_t 	Mms_ck;
	uint8_t 	Mms_type;
	uint8_t 	Mms_rsvd[1];
	/*
	 * total length contains header.
	 */
	uint32_t 	Mms_len;
	uint64_t 	Mms_hashkey;
	/*
	 * specified msg header.
	 */
};

typedef struct Mlsas_Attach_msg {
	uint32_t Atm_st;
	uint32_t Atm_rsp:1,
			 Atm_down2up_attach:1,
			 Atm_rsvd:30;
	uint64_t Atm_pr;
	uint64_t Atm_ext;
} Mlsas_Attach_msg_t;

typedef struct Mlsas_Aggr_Virt_msg {
	uint32_t Mv_st_error;
	uint32_t Mv_rsvd;
	uint64_t Mv_ext;
	/* to append */
} Mlsas_Aggr_Virt_msg_t;

typedef struct Mlsas_RW_msg {
	uint64_t rw_sector;
	uint64_t rw_rqid;
	uint64_t rw_mlbpr;
	uint32_t rw_flags;
} Mlsas_RW_msg_t;

typedef struct Mlsas_Disc_msg {
	uint64_t rw_sector;
	uint64_t rw_rqid;
	uint64_t rw_mlbpr;
	uint32_t rw_flags;
	uint32_t t_size;
} Mlsas_Disc_msg_t;

typedef struct Mlsas_RWrsp_msg {
	uint64_t rsp_rqid;
	int 	 rsp_error;
} Mlsas_RWrsp_msg_t;

typedef struct Mlsas_State_Change_msg {
	uint32_t scm_state;
	uint32_t scm_event;
	uint64_t scm_pr;
	uint64_t scm_ext;
} Mlsas_State_Change_msg_t;

extern int (*__Mlsas_clustersan_rx_hook_add)(uint32_t, cs_rx_cb_t, void *);
extern int (*__Mlsas_clustersan_link_evt_hook_add)(cs_link_evt_cb_t, void *);
extern int (*__Mlsas_clustersan_host_send)(cluster_san_hostinfo_t *, void *, uint64_t, 
	void *, uint64_t, uint8_t, int, boolean_t, int);
extern int (*__Mlsas_clustersan_host_send_bio)(cluster_san_hostinfo_t *, void *, uint64_t, 
	void *, uint64_t, uint8_t, int, boolean_t, int);
extern void (*__Mlsas_clustersan_broadcast_send)(void *, uint64_t, void *, uint64_t, uint8_t, int);
extern void (*__Mlsas_clustersan_hostinfo_hold)(cluster_san_hostinfo_t *);
extern void (*__Mlsas_clustersan_hostinfo_rele)(cluster_san_hostinfo_t *);
extern void (*__Mlsas_clustersan_rx_data_free)(cs_rx_data_t *, boolean_t );
extern void (*__Mlsas_clustersan_rx_data_free_ext)(cs_rx_data_t *);
extern void *(*__Mlsas_clustersan_kmem_alloc)(size_t);
extern void (*__Mlsas_clustersan_kmem_free)(void *, size_t);

extern void __Mlsas_Thread_Init(Mlsas_thread_t *thi,
		int (*fn) (Mlsas_thread_t *), 
		Mlsas_ttype_e tt, const char *name);
extern void __Mlsas_Thread_Start(Mlsas_thread_t *thi);
extern void __Mlsas_Thread_Stop(Mlsas_thread_t *thi);
extern void __Mlsas_Thread_Stop_nowait(Mlsas_thread_t *thi);
extern int __Mlsas_Tx_biow(Mlsas_rtx_wk_t *w);
extern int __Mlsas_Tx_bior(Mlsas_rtx_wk_t *w);
extern void __Mlsas_Queue_RTX(Mlsas_rtx_wq_t *wq, Mlsas_rtx_wk_t *wk);
extern void __Mlsas_RTx_Init_WQ(Mlsas_rtx_wq_t *wq);
extern int __Mlsas_RTx(Mlsas_thread_t *thi);
extern int __Mlsas_Tx_PR_RQ_rsp(Mlsas_rtx_wk_t *w);
extern int __Mlsas_Rx_bior(Mlsas_rtx_wk_t *w);
extern int __Mlsas_Rx_biow(Mlsas_rtx_wk_t *w);
extern Mlsas_RW_msg_t *__Mlsas_Setup_RWmsg(Mlsas_request_t *rq);

extern void __Mlsas_Submit_PR_request(Mlsas_blkdev_t *Mlb,
		unsigned long rw, Mlsas_pr_req_t *prr);
extern int Mlsas_TX(void *session, void *header, uint32_t hdlen,
                void *dt, uint32_t dtlen, boolean_t io);
extern void __Mlsas_Req_Stmt(Mlsas_request_t *rq, uint32_t what,
                Mlsas_bio_and_error_t *Mlbi);
extern void __Mlsas_Complete_Master_Bio(Mlsas_request_t *rq,       
                Mlsas_bio_and_error_t *Mlbi);
extern void __Mlsas_Free_PR_RQ(Mlsas_pr_req_t *prr);
extern Mlsas_pr_req_t *__Mlsas_Alloc_PR_RQ(Mlsas_pr_device_t *pr,
                uint64_t reqid, sector_t sec, size_t len);
extern void __Mlsas_PR_RQ_stmt(Mlsas_pr_req_t *prr, uint32_t what,
		Mlsas_pr_req_free_t *fr);
extern inline void __Mlsas_sub_PR_RQ(Mlsas_pr_req_t *prr, uint32_t put);
extern inline void __Mlsas_put_PR_RQ(Mlsas_pr_req_t *prr);
extern inline void __Mlsas_get_PR_RQ(Mlsas_pr_req_t *prr);
extern inline void __Mlsas_put_virt(Mlsas_blkdev_t *vt);
extern inline void __Mlsas_get_virt(Mlsas_blkdev_t *vt);
extern inline void __Mlsas_walk_virt(void *priv,
		uint_t (*cb)(mod_hash_key_t, mod_hash_val_t *, void *));
extern int __Mlsas_Virt_export_zfs_attach(const char *path, struct block_device *bdev,
		struct block_device **new_bdev);
extern void __Mlsas_Virt_export_zfs_detach(const char *partial, struct block_device *vt_partial);

#endif
