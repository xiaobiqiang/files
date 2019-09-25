#ifndef __SYS_IF_DRBDSVC_H
#define __SYS_IF_DRBDSVC_H

#include <linux/types.h>
#include <sys/list.h>

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

struct drbdmon_param_resume_bp {
	uint32_t	primary;
	uint32_t	drbdX;
	char		peer_ip[16];
	char		resource[128];
	union {
		uint8_t			rsvd[24];
		struct {
			list_node_t entry;
			uint8_t		handled;
			uint8_t		rsvd[7];
		} mon;
	} opt;
};

#endif /* __SYS_IF_DRBDSVC_H */
