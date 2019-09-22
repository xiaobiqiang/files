#ifndef __SYS_IPMI_NOTIFY_IF_H
#define __SYS_IPMI_NOTIFY_IF_H

#include <linux/types.h>
#include <sys/types.h>

/*
 * there can be 2^24 ioctl opts for every module.
 */
#define IPMI_IOC_CMD				0x0000deaf
#define IPMI_IOC_MODULE(x)			(x << 24)

#define IPMI_MODULE_FIRST 			0
#define IPMI_MODULE_POWER			1
#define IPMI_MODULE_LAST			2

#define IPMI_IOC_PSU				IPMI_IOC_MODULE(IPMI_MODULE_POWER)
#define IPMI_PSU_IOC(x)				(IPMI_IOC_PSU | (1 << x))
#define IPMI_IOC_PSU_ON				IPMI_PSU_IOC(0)
#define IPMI_IOC_PSU_OFF			IPMI_PSU_IOC(1)
#define IPMI_IOC_PSU_MASK			((IPMI_IOC_PSU_ON | IPMI_IOC_PSU_OFF) & 0x00ffffff)

typedef struct ipmi_iocdata {
	unsigned module;
	unsigned module_spec;
	unsigned long inbuf;
	unsigned long outbuf;
	unsigned inlen;
	unsigned outlen;
} ipmi_iocdata_t;

struct ipmi_subscriber {
	char 		*name;
	/* subscribe which module */
	uint32_t	module;
	uint32_t	flags;
	/* deal with messages which are pushed by module subscribed */
	void		(*msghdl)(uint32_t, void *, uint32_t, void *, uint32_t);

	void 		*md_priv;
#define	IPMI_SUBER_REALLY	0x00000001
};

struct ipmi_notify_module {
	char 		*name;
	uint32_t 	module;
	uint32_t	active:1,
				rsvd:31;
	
	int 		(*_init)(void);
	void 		(*_exit)(void);
	int			(*_subscribe)(struct ipmi_subscriber *);
	int			(*_unsubscribe)(struct ipmi_subscriber *);		
	int			(*_filter)(uint32_t, void *, uint32_t);
	void		(*_push)(uint32_t, void *, uint32_t, void *, uint32_t);
};

#endif /* __SYS_IPMI_NOTIFY_IF_H */
