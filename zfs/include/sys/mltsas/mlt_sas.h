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
#include <sys/modhash.h>
#include <mltsas_comm.h>

#define Mlsas_MAJOR			299

#define Mlsas_Mxbio_Size	(1 << 20)
#define Mlsas_Mxbio_Segment	128
#define Mlsas_DMA_Alimask	0x03

/*
 * use clustersan
 */
#define Mlsas_Noma_Session	CLUSTER_SAN_BROADCAST_SESS

#define Mlsas_St_Disabled	0x01
#define Mlsas_St_Enabling	0x02
#define Mlsas_St_Enabled	0x03
#define Mlsas_St_Busy		0x04

#define Mlsas_RQ_Write				0x00000001
#define Mlsas_RQ_Local_Pending		0x00000002
#define Mlsas_RQ_Net_Pending		0x00000004
#define Mlsas_RQ_Net_Queued			0x00000008
#define Mlsas_RQ_Net_Sent			0x00000010
#define Mlsas_RQ_Local_Done			0x00000020
#define Mlsas_RQ_Local_OK			0x00000040
#define Mlsas_RQ_Local_Aborted		0x00000080
#define Mlsas_RQ_Net_Done			0x00000100
#define Mlsas_RQ_Net_OK				0x00000200
#define Mlsas_RQ_Delayed			0x80000000

#define Mlsas_Rst_New				0x00
#define Mlsas_Rst_Submit_Local		0x01
#define Mlsas_Rst_Submit_Net		0x02
#define Mlsas_Rst_Queue_Net			0x03
#define Mlsas_Rst_Net_Submitted		0x04
#define Mlsas_Rst_Net_Inflight		0x05
#define Mlsas_Rst_Net_Ack			0x06
#define Mlsas_Rst_Complete_OK		0x07
#define Mlsas_Rst_Read_Error		0x08
#define Mlsas_Rst_ReadA_Error		0x09
#define Mlsas_Rst_Write_Error		0x0A
#define Mlsas_Rst_Discard_Error		0x0B
#define Mlsas_Rst_Done				0x08

#define Mlsas_Devevt_To_Attach		0x01
#define Mlsas_Devevt_Attach_Error	0x02
#define Mlsas_Devevt_Attach_OK		0x03
#define Mlsas_Devevt_Attach_Local	0x04
#define Mlsas_Devevt_Attach_PR		0x05
#define Mlsas_Devevt_Last			0x10

#define __Mlsas_Get_ldev_if_state(Mlb, __minSt)	\
	((Mlb)->Mlb_st >= (__minSt))
#define __Mlsas_Get_ldev(Mlb)		\
	__Mlsas_Get_ldev_if_state((Mlb), Mlsas_Devst_Degraded)

#define __Mlsas_Get_PR_if_state(pr, __minSt)	\
	((pr)->Mlpd_st >= (__minSt))

typedef enum Mlsas_devst Mlsas_devst_e;
typedef enum Mlsas_tst Mlsas_tst_e;
typedef struct Mlsas_backdev_info Mlsas_backdev_info_t;
typedef struct Mlsas_blkdev Mlsas_blkdev_t;
typedef struct Mlsas_pr_device Mlsas_pr_device_t;
typedef struct Mlsas_request Mlsas_request_t;
typedef struct Mlsas_bio_and_error Mlsas_bio_and_error_t;
typedef struct Mlsas_rh Mlsas_rh_t;
typedef struct Mlsas_retry Mlsas_retry_t;
typedef struct Mlsas_thread Mlsas_thread_t;

typedef struct Mlsas Mlsas_t;

enum Mlsas_devst {
	Mlsas_Devst_Standalone,
	Mlsas_Devst_Failed,
	Mlsas_Devst_Degraded,
	Mlsas_Devst_Attached,
	Mlsas_Devst_Healthy,
	Mlsas_Devst_Last
};

enum Mlsas_tst {
	Mt_None,
	Mt_Run,
	Mt_Exit,
	Mt_Restart
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
	Mlsas_blkdev_t *Mlpd_mlb;
	Mlsas_rh_t *Mlpd_rh;
	uint32_t Mlpd_hostid;
	/*
	 * protected by Mlb_rq_spin
	 */
	spinlock_t *Mlpd_st_spin;
	Mlsas_devst_e Mlpd_st;
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
	uint32_t Mlb_attach_status;
	/*
	 * 
	 */
	uint32_t Mlb_npending;
};

struct Mlsas_request {
	list_node_t Mlrq_node;
	struct kref Mlrq_ref;
	Mlsas_blkdev_t *Mlrq_bdev;
	uint32_t Mlrq_flags;
	uint32_t Mlrq_state;
	uint64_t Mlrq_start_jif;
	uint64_t Mlrq_submit_jif;
	struct bio *Mlrq_master_bio;
	struct bio *Mlrq_back_bio;
	uint32_t Mlrq_completion_ref;
	
	/*
	 * locate
	 */
	uint32_t Mlrq_bsize;
	sector_t Mlrq_sector;
	Mlsas_pr_device_t *Mlrq_pr;
};

/*
 * bio source complete
 */
struct Mlsas_bio_and_error {
	struct bio *Mlbi_bio;
	int Mlbi_error;
	uint32_t k_put;
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
	Mlsas_retry_t Mh_w;
	kmutex_t Mh_mtx;
	list_t Mh_devices;
	void *Mh_session;
};

struct Mlsas_thread {
	spinlock_t Mt_lock;
	struct task_struct *Mt_w;
	struct completion Ml_stop;
	Mlsas_tst_e Mt_state;
	int (*Mt_fn) (Mlsas_thread_t *);
//	int reset_cpu_mask;
	const char *Mt_name;
};

struct Mlsas {
	uint32_t Ml_state;
	uint32_t Ml_minor;
	kmutex_t Ml_mtx;
	mod_hash_t *Ml_devices;
	mod_hash_t *Ml_rhs;
	struct kmem_cache *Ml_skc;
	mempool_t *Ml_request_mempool;
	Mlsas_retry_t Ml_retry;
	taskq_t *Ml_async_tq;
};

extern void __Mlsas_Thread_Init(Mlsas_thread_t *thi,
	int (*fn) (Mlsas_thread_t *), const char *name);
extern void __Mlsas_Thread_Start(Mlsas_thread_t *thi);
extern void __Mlsas_Thread_Stop(Mlsas_thread_t *thi);
extern void __Mlsas_Thread_Stop_nowait(Mlsas_thread_t *thi);


/* =====================Define Message Type Struct=========================== */

#define Mlsas_Mms_Magic			0x2020

/* Message Type Define */
#define Mlsas_Mms_None			0x00
#define Mlsas_Mms_Attach		0x01
#define Mlsas_Mms_Aggr_Virt 	0x02
#define Mlsas_Mms_Aggr_Rsp 		0x03
#define Mlsas_Mms_Last			0x10

typedef struct Mlsas_Msh Mlsas_Msh_t;
typedef void (*Mlsas_RX_pfn_t)(Mlsas_Msh_t *, cs_rx_data_t *);

struct Mlsas_Msh {
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
	uint32_t Atm_rsvd;
	uint64_t Atm_ext;
} Mlsas_Attach_msg_t;

typedef struct Mlsas_Aggr_Virt_msg {
	uint32_t Mv_st_error;
	uint32_t Mv_rsvd;
	uint64_t Mv_ext;
	/* to append */
} Mlsas_Aggr_Virt_msg_t;

#endif
