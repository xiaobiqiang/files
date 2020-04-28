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

#define Mlsas_MAJOR			299

#define Mlsas_Mxbio_Size	(1 << 20)
#define Mlsas_Mxbio_Segment	128
#define Mlsas_DMA_Alimask	0x03

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
#define Mlsas_Rst_Submit_Net		0x03
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
#define Mlsas_Devevt_Last			0x10

#define __Mlsas_Get_ldev_if_state(Mlb, __minSt)	\
	((Mlb)->Mlb_st >= (__minSt))
#define Mlsas_Get_ldev(Mlb)		\
	__Mlsas_Get_ldev_if_state((Mlb), Mlsas_Devst_Degraded)

typedef enum Mlsas_devst {
	Mlsas_Devst_Standalone,
	Mlsas_Devst_Attaching,
	Mlsas_Devst_Failed,
	Mlsas_Devst_Degraded,
	Mlsas_Devst_Healthy,
	Mlsas_Devst_Last
} Mlsas_devst_e;

typedef struct Mlsas_backdev_info {
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
} Mlsas_backdev_info_t;

typedef struct Mlsas_pr_device {
	list_node_t Mlpd_node;
} Mlsas_pr_device_t;

typedef struct Mlsas_blkdev {
	uint64_t Mlb_hashkey;	/* const */
	uint64_t Mlb_nopen;
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
	/*
	 * 
	 */
	uint32_t Mlb_npending;
} Mlsas_blkdev_t;

typedef struct Mlsas_request {
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
} Mlsas_request_t;

/*
 * bio source complete
 */
typedef struct Mlsas_bio_and_error {
	struct bio *Mlbi_bio;
	int Mlbi_error;
	uint32_t k_put;
} Mlsas_bio_and_error_t;

typedef struct Mlsas_retry {
	struct workqueue_struct *Mlt_workq;
	struct work_struct Mlt_work;
	spinlock_t Mlt_lock;
	list_t Mlt_writes;
} Mlsas_retry_t;

typedef struct Mlsas {
	uint32_t Ml_state;
	kmutex_t Ml_mtx;
	mod_hash_t *Ml_devices;
	struct kmem_cache *Ml_skc;
	mempool_t *Ml_request_mempool;
	Mlsas_retry_t Ml_retry;
	uint32_t Ml_minor;
} Mlsas_t;

#endif
