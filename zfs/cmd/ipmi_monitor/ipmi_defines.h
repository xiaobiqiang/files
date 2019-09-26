#ifndef __IPMI_DEFINES_H
#define __IPMI_DEFINES_H

#include <pthread.h>
#include <sys/ipmi_notify_if.h>

#define IPMI_CONF					"/etc/ipmi_checker.ini"
#define IPMI_IOC_DEV				"/dev/ipmi_notify"

#define IPMI_USER_LEN				32
#define IPMI_PASSWD_LEN				64
#define IPMI_IP_LEN					16

/************ global setting ******************/
#define IPMI_SECT_GLOBAL			"global"
#define IPMI_SECT_GLOBAL_USER		"user"
#define IPMI_SECT_GLOBAL_PASSWD		"password"
#define IPMI_SECT_GLOBAL_IP			"ipmi_ip"
#define IPMI_SECT_GLOBAL_INTERVAL	"interval"
#define IPMI_SECT_GLOBAL_DOWN		"link_down"
#define IPMI_SECT_GLOBAL_RETRIES	"retry_times"
#define IPMI_SECT_GLOBAL_UP_THRE	"up_threshold"

#define IPMI_USER					"user"
#define IPMI_PASSWD					"user"
#define IPMI_IP						"0.0.0.0"
#define IPMI_INTERVAL				1000
#define IPMI_DOWN_ABRT				"abort"
#define IPMI_DOWN_SUSP				"suspend"	/* only suspend, not post */
#define IPMI_DOWN_POST				"post"		/* once detected, post, then continue */
#define IPMI_DOWN_RETRIES			"retry"		/* retry some times, then post and suspend */
#define IPMI_RETRIES				3
#define IPMI_UP_THRESHOLD			1

#define IPMI_LINK_DOWN_ABRT			0
#define IPMI_LINK_DOWN_SUSP			1
#define IPMI_LINK_DOWN_POST			2
#define IPMI_LINK_DOWN_RETRIES		3

/************ power module ***************/
#define IPMI_SECT_POWER				"power"
#define IPMI_SECT_POWER_INTERVAL	"interval"
#define IPMI_SECT_POWER_DOWN		"link_down"
#define IPMI_SECT_POWER_ERR_HDL		"error_fence"
#define IPMI_SECT_POWER_RETRIES		"retry_times"

#define POWER_INTERVAL				1000
#define POWER_DOWN_ABORT			"abort"
#define POWER_DOWN_SUSP				"suspend"
#define POWER_DOWN_IGNORE			"ignore"
#define POWER_ERR_RETRY				"retry"			/* retry some times, then post and suspend */
#define POWER_ERR_NEXT_RETRY		"next_retry"	/* like ignore it */
#define POWER_ERR_SUSP				"suspend"		/* once error, post and suspend */
#define POWER_ERR_ABRT				"abort"
#define POWER_RETRIES				3

#define POWER_LINK_DOWN_ABORT		0
#define POWER_LINK_DOWN_SUSP		1
#define POWER_LINK_DOWN_IGNORE		2

#define POWER_ERRHDL_RETRY			0
#define POWER_ERRHDL_NEXT			1
#define POWER_ERRHDL_SUSP			2
#define POWER_ERRHDL_ABRT			3

#define PSU_STATE_UNKNOWN			"unknown"
#define PSU_STATE_ON				"on"
#define PSU_STATE_OFF				"off"

#define PSU_UNKNOWN					0
#define PSU_ON						1
#define PSU_OFF						2

#define IPMI_PING_ALIVE(ip, result)			\
	{								\
		char *line = NULL;			\
		size_t size = 0;			\
		char cmd[256];				\
		FILE *outbuf;				\
									\
		memset(cmd, 0, 256);		\
		snprintf(cmd, 256, "ping %s -c 1 -W 1 | grep transmitted | cut -d ',' -f 3", (ip));	\
		outbuf = popen(cmd, "r");	\
		getline(&line, &size, outbuf);	\
		if (line && size) {			\
			result = (line[1] == '0');	\
			free(line);				\
			line = NULL;			\
		}							\
		pclose(outbuf);				\
	}

#define ipmi_checker_find_opt(x, optstr, optnum)	\
	{	\
		int i = 0;		\
		for (i=0; i<x##_num; i++) {		\
			if (strcmp(x[i].opt_str, optstr))	\
				continue;	\
			optnum = x[i].opt_num;	\
			printf("%s optnum:%d\n", __func__, optnum);	\
			break;		\
		}		\
	}	

enum ipmi_event {
	IPMI_EVT_LINK_DOWN = 1,
	IPMI_EVT_LINK_UP,
};

struct ipmi_opt {
	unsigned int opt_num;
	const char *opt_str;
};

struct ipmi_module {
	char				*name;
	int 				(*__init)(const struct ipmi_conf *conf);
	void 				(*__exit)(const struct ipmi_conf *conf);
	void 				(*__post)(const struct ipmi_conf *conf, enum ipmi_event evt, void *extra);

	unsigned int 		inited:1,
						exited:1,
						rsvd:30;
};

struct ipmi_conf {
	char 				*ic_conf;
	char 				*ic_ioc_dev;
	char 				*ic_user;
	char 				*ic_passwd;
	char 				*ic_ip;
	int					ic_ioc_fd;

	unsigned int		ic_up_threshold;
	unsigned int		ic_interval;
	unsigned int		ic_retries;
	unsigned int		ic_retried;
	unsigned int		ic_conf_loaded:1, 
						ic_ioc_opened:1,
						ic_exit:1,
						ic_susp:1,	/* if susp, a thread to check */
						ic_link_down:2,
						ic_link_up:1,
						ic_last_link_alive:1,
						ic_in_retry:1,
						ic_misc_running:1,
						ic_rsvd:22;

	pthread_mutex_t		ic_mutex;
	pthread_cond_t		ic_cv;
	pthread_t			ic_misc;
};

extern void ipmi_delay_interval(pthread_mutex_t *, pthread_cond_t *, unsigned);
extern void ipmi_delay_interval_locked(pthread_mutex_t *, pthread_cond_t *, unsigned);
extern int ipmi_ioctl(struct ipmi_conf *, unsigned, void *, unsigned, void *, unsigned);

#endif	/* __IPMI_DEFINES_H */
