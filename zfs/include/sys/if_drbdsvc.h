#ifndef __SYS_IF_DRBDSVC_H
#define __SYS_IF_DRBDSVC_H

#include <linux/types.h>
#include <sys/list.h>
#include <pthread.h>
#include <sys/thread_pool.h>

#define DRBDMON_SOCKET		"/usr/lib/systemd/system/drbdmon.socket"
#define DRBDMON_MAGIC		0x00deaf00
#define DRBDMON_ERR_RECV	0x00000001
#define DRBDMON_ERR_PARAM	0x00000002

enum drbdmon_rpc_type {
	DRBDMON_RPC_FIRST = 0,
	DRBDMON_RPC_RESUME_BREADPOINT,
	DRBDMON_RPC_LAST
};

struct drbdmon_head {
	uint32_t	magic;
	uint32_t	rpc;
	uint32_t	ilen;
	uint32_t	olen;
	uint32_t	error;
	uint32_t	rsvd;
};

struct drbdmon_resume_bp_ctx {
	list_node_t		entry;
	char			peer_ip[16];
	char			local_ip[16];
	pthread_mutex_t ctx_mtx;
	pthread_cond_t	ctx_cv;
	list_t			resume_list;
	tpool_t			*mon_ctx;
	tpool_t			*invalidate_ctx;
	uint32_t		ip_addr;
	uint32_t		exit:1,
					rsvd:31;
};

struct drbdmon_param_resume_bp {
	uint32_t	primary;
	uint32_t	drbdX;
	char		local_ip[16];
	char		peer_ip[16];
	char		resource[128];
	union {
		uint8_t			rsvd[32];
		struct {
			list_node_t entry;
			struct drbdmon_resume_bp_ctx *bp_ctx;
			uint8_t		handled;
			uint8_t		rsvd[7];
		} mon;
	} opt;
};

#endif /* __SYS_IF_DRBDSVC_H */
