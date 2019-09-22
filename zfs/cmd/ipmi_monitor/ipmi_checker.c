#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include <syslog.h>
#include "ipmi_checker.h"
#include "ini_parse.h"

static struct ipmi_opt ipmi_down_opts[] = {
	{IPMI_LINK_DOWN_ABRT, 		IPMI_DOWN_ABRT},
	{IPMI_LINK_DOWN_SUSP, 		IPMI_DOWN_SUSP},
	{IPMI_LINK_DOWN_POST, 		IPMI_DOWN_POST},
	{IPMI_LINK_DOWN_RETRIES, 	IPMI_DOWN_RETRIES}
};
static unsigned ipmi_down_opts_num = sizeof(ipmi_down_opts)/sizeof(struct ipmi_opt);
static struct ipmi_conf *global_conf = NULL;
static struct ipmi_module *ipmi_modules[] = {
	&power_module,
	NULL
};

static int __ipmi_parse_ipmi_conf(struct ipmi_conf *);
static void ipmi_create_ipmi_conf(struct ipmi_conf **);
static void ipmi_broadcast_event(struct ipmi_conf *, enum ipmi_event, void *);
static void ipmi_check_link_alive(struct ipmi_conf *);
static void ipmi_deactivate_modules(struct ipmi_module **);
static void ipmi_activate_modules(struct ipmi_module **);
static void *ipmi_wait_link_up(struct ipmi_conf *);
	
int main(int argc, char **argv)
{
	int iRet;
	
	ipmi_create_ipmi_conf(&global_conf);
	if (!global_conf) 
		return -EINVAL;

	ipmi_activate_modules(ipmi_modules);

	pthread_mutex_lock(&global_conf->ic_mutex);
loop:
	while (!global_conf->ic_exit && !global_conf->ic_susp) {
		pthread_mutex_unlock(&global_conf->ic_mutex);
		
		ipmi_check_link_alive(global_conf);
		if (global_conf->ic_exit || global_conf->ic_susp)
			continue;
		
		pthread_mutex_lock(&global_conf->ic_mutex);
		ipmi_delay_interval_locked(&global_conf->ic_mutex,
			&global_conf->ic_cv, global_conf->ic_interval);
	}
	
	if (global_conf->ic_exit) {
		pthread_mutex_unlock(&global_conf->ic_mutex);
		ipmi_deactivate_modules(ipmi_modules);
		return -ECONNABORTED;
	}

	/* susp */
	while (global_conf->ic_susp) {
		if (!global_conf->ic_misc_running) {
			if (pthread_create(&global_conf->ic_misc, NULL, 
						ipmi_wait_link_up, global_conf)) {
				fprintf(stderr, "create global_conf->ic_misc thread failed,"
					"service is in susp, exit.");
				return 0;
			}
			global_conf->ic_misc_running = 1;
			pthread_detach(global_conf->ic_misc);
		}
		pthread_cond_wait(&global_conf->ic_cv, &global_conf->ic_mutex);	
	}
	goto loop;
}

static void *
ipmi_wait_link_up(struct ipmi_conf *conf)
{
	int alive = 0, times = 0;
	
	for ( ; ; ) {
		IPMI_PING_ALIVE(conf->ic_ip, alive);
		if (!alive) {
			times = 0;
			goto delay;
		}
		
		if (++times >= conf->ic_up_threshold) {
			conf->ic_susp = 0;
			pthread_cond_broadcast(&conf->ic_cv);
			break;
		}
delay:
		ipmi_delay_interval(&conf->ic_mutex,
			&conf->ic_cv, conf->ic_interval);
	}
}

static void
ipmi_activate_modules(struct ipmi_module **modulep)
{
	int inited;
	struct ipmi_module *module = *modulep;

	while (module) {
		inited = 1;
		if (module->__init && module->__init(global_conf))
			inited = 0;
		module->inited = inited;
		module = *(++modulep);
	}
}

static void
ipmi_deactivate_modules(struct ipmi_module **modulep)
{
	struct ipmi_module *module = *modulep;

	while (module) {
		if (module->__exit && !module->exited)
			module->__exit(global_conf);
		module->exited = 1;
		module = *(++modulep);
	}
}

static void 
ipmi_check_link_alive(struct ipmi_conf *conf)
{
	int alive = 0;
	enum ipmi_event event = 0;
	
	IPMI_PING_ALIVE(conf->ic_ip, alive);
	/* link down or up */
	if (conf->ic_last_link_alive != alive) {
		if (!alive) {
			switch (conf->ic_link_down) {
				case IPMI_LINK_DOWN_ABRT:
					conf->ic_exit = 1;
					break;
				case IPMI_LINK_DOWN_SUSP:
					conf->ic_susp = 1;
					break;
				case IPMI_LINK_DOWN_POST:
					event = IPMI_EVT_LINK_DOWN;
					break;
				case IPMI_LINK_DOWN_RETRIES:
					conf->ic_in_retry = 1;
					break;
			}
		} else {
			switch (conf->ic_link_up) {
				case 0:		/* post */
					event = IPMI_EVT_LINK_UP;
					break;
				case 1:		/* ignore */
					break;
			}
			conf->ic_in_retry = 0;
			conf->ic_retried = 0;
		}
	}

	if (conf->ic_in_retry) {	/* down && IPMI_LINK_DOWN_RETRIES */
		assert (alive == 0);
		if (++conf->ic_retried < conf->ic_retries) 
			return ;
		event = IPMI_EVT_LINK_DOWN;
		conf->ic_susp = 1;
	}

	if (event)
		ipmi_broadcast_event(conf, event, NULL);
	conf->ic_last_link_alive = alive;
}

static void
ipmi_broadcast_event(struct ipmi_conf *conf, 
			enum ipmi_event event, void *extra_ptr)
{
	struct ipmi_module **modulep = ipmi_modules;
	struct ipmi_module *module = *modulep;

	while (module) {
		if (module->__post)
			module->__post(conf, event, extra_ptr);

		module = *(++modulep);
	}
}

static void
ipmi_create_ipmi_conf(struct ipmi_conf **confp)
{
	int iRet;
	struct ipmi_conf *conf;

	assert((conf = malloc(sizeof(*conf))) != NULL);
	memset(conf, 0, sizeof(*conf));

	pthread_mutex_init(&conf->ic_mutex, NULL);
	pthread_cond_init(&conf->ic_cv, NULL);
	conf->ic_ioc_dev = strdup(IPMI_IOC_DEV);
	conf->ic_conf = strdup(IPMI_CONF);
	assert(conf->ic_conf && conf->ic_ioc_dev);
	
	if (__ipmi_parse_ipmi_conf(conf) != 0)
		goto failed_parse;

	if ((conf->ic_ioc_fd = open(conf->ic_ioc_dev, O_RDWR)) < 0)
		goto failed_open;
	conf->ic_ioc_opened = 1;

	*confp = conf;
	return ;

failed_open:
	if (conf->ic_conf_loaded)
		iniFileFree();
	free(conf->ic_user);
	free(conf->ic_passwd);
	free(conf->ic_ip);
failed_parse:
	free(conf->ic_conf);
	free(conf->ic_ioc_dev);
	pthread_mutex_destroy(&conf->ic_mutex);
	pthread_cond_destroy(&conf->ic_cv);
	free(conf);
	*confp = NULL;
}

static int
__ipmi_parse_ipmi_conf(struct ipmi_conf *confp)
{
	int iRet;
	char user[IPMI_USER_LEN];
	char passwd[IPMI_PASSWD_LEN];
	char ipmi_ip[IPMI_IP_LEN];
	char link_down[32];
	
	assert(confp->ic_conf && !confp->ic_conf_loaded);

	memset(user, 0, IPMI_USER_LEN);
	memset(passwd, 0, IPMI_PASSWD_LEN);
	memset(ipmi_ip, 0, IPMI_IP_LEN);
	memset(link_down, 0, 32);
	
	if (iniFileLoad(confp->ic_conf) == 0)
		return -ENOMEM;
	confp->ic_conf_loaded = 1;
	
	(void) iniGetString(IPMI_SECT_GLOBAL, IPMI_SECT_GLOBAL_USER, 
				user, IPMI_USER_LEN, IPMI_USER);
	(void) iniGetString(IPMI_SECT_GLOBAL, IPMI_SECT_GLOBAL_PASSWD, 
				passwd, IPMI_PASSWD_LEN, IPMI_PASSWD);
	(void) iniGetString(IPMI_SECT_GLOBAL, IPMI_SECT_GLOBAL_IP, 
				ipmi_ip, IPMI_IP_LEN, IPMI_IP);
	(void) iniGetString(IPMI_SECT_GLOBAL, IPMI_SECT_GLOBAL_DOWN, 
				link_down, 32, IPMI_DOWN_ABRT);
	confp->ic_interval = iniGetInt(IPMI_SECT_GLOBAL, 
				IPMI_SECT_GLOBAL_INTERVAL, IPMI_INTERVAL);
	confp->ic_retries = iniGetInt(IPMI_SECT_GLOBAL, 
				IPMI_SECT_GLOBAL_RETRIES, IPMI_RETRIES);
	confp->ic_up_threshold = iniGetInt(IPMI_SECT_GLOBAL,
				IPMI_SECT_GLOBAL_UP_THRE, IPMI_UP_THRESHOLD);
	if ((user[0] == '\0') || (passwd[0] == '\0') || (ipmi_ip[0] == '\0')) {
		iniFileFree();
		confp->ic_conf_loaded = 0;
		return -EINVAL;
	}

	confp->ic_user = strdup(user);
	confp->ic_passwd = strdup(passwd);
	confp->ic_ip = strdup(ipmi_ip);
	ipmi_checker_find_opt(ipmi_down_opts, link_down, confp->ic_link_down);
	
	assert(confp->ic_user && confp->ic_passwd && confp->ic_ip);
	return 0;
}

int 
ipmi_ioctl(struct ipmi_conf *conf, unsigned ioc, 
		void *inbuf, unsigned inlen, void *outbuf, unsigned outlen)
{
	int iRet;
	ipmi_iocdata_t iocdata = {0};

	memset(&iocdata, 0, sizeof(ipmi_iocdata_t));
	iocdata.module = ioc >> 10;
	iocdata.module_spec = ioc & 0x3ff;
	iocdata.inbuf = (unsigned long)inbuf;
	iocdata.inlen = inlen;
	iocdata.outbuf = (unsigned long)outbuf;
	iocdata.outlen = outlen;

	syslog(LOG_ERR, "%s module:%u module_cmd:%u", __func__, 
		iocdata.module, iocdata.module_spec);
	assert (conf->ic_ioc_opened && (conf->ic_ioc_fd > 0));

	iRet = ioctl(conf->ic_ioc_fd, IPMI_IOC_CMD, &iocdata);
	if (iRet != 0) {
		syslog(LOG_ERR, "%s module:%u module_cmd:%u ioctl failed", 
			__func__, iocdata.module, iocdata.module_spec);
	}
	return iRet;
}

/* must hold mtx before */
void
ipmi_delay_interval_locked(pthread_mutex_t *mtx, pthread_cond_t *cv, unsigned gap)
{
	long sec_inc, nsec_inc;
	struct timespec tmspec;

	sec_inc = gap / 1000;
	nsec_inc = (gap - sec_inc*1000)*1000000;
	clock_gettime(CLOCK_REALTIME, &tmspec);
	if (tmspec.tv_nsec >= (1000000000 - nsec_inc)) {
		sec_inc++;
		nsec_inc -= 1000000000;
	}
	tmspec.tv_sec += sec_inc;
	tmspec.tv_nsec += nsec_inc;
	
	pthread_cond_timedwait(cv, mtx, &tmspec);
}

void
ipmi_delay_interval(pthread_mutex_t *mtx, pthread_cond_t *cv, unsigned gap)
{
	pthread_mutex_lock(mtx);
	ipmi_delay_interval_locked(mtx, cv, gap);
	pthread_mutex_unlock(mtx);
}

