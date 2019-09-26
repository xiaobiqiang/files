#include <syslog.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include "ini_parse.h"
#include "power_checker.h"

#define POWER_CMD_FORMAT		"ipmitool -N 1 -R 1 -H %s -U %s -P %s power status"	\
								" | cut -d ' ' -f 4"

struct power_global {
	struct ipmi_module 		*module;
	const struct ipmi_conf	*ipmi_conf;

	unsigned int			buflen;
	char 					*check_command;

	pthread_mutex_t			ck_mtx;
	pthread_cond_t			ck_cv;
	pthread_t				checker_wk;

	/* configure */
	unsigned int			check_tm;		/* ms */
	unsigned int			retry_times;	/* for error_hdl:retry */
	unsigned int 			retried;
	unsigned int			link_down_abort:2,	//0:abort  1:suspend 	2:continue
							check_err_hdl:2,	//0:retry, post, suspend, 1:next_retry 2:suspend 3:abort
							
							start_check:1,
							checker_exit:1,
							cmd_set:1,
							check_susp:1,
							check_resume:1,
							in_retry:1,
							last_psu_status:2,
							curr_psu_status:2,
							rsvd:18;
};

static struct ipmi_opt down_abrt_opts[] = {
	{POWER_LINK_DOWN_ABORT,	POWER_DOWN_ABORT},
	{POWER_LINK_DOWN_SUSP, 	POWER_DOWN_SUSP},
	{POWER_LINK_DOWN_IGNORE,POWER_DOWN_IGNORE},
}, err_hdl_opts[] = {
	{POWER_ERRHDL_RETRY, 	POWER_ERR_RETRY},
	{POWER_ERRHDL_NEXT, 	POWER_ERR_NEXT_RETRY},
	{POWER_ERRHDL_SUSP, 	POWER_DOWN_SUSP},
	{POWER_ERRHDL_ABRT, 	POWER_ERR_ABRT}
}, power_status_opts[] = {
	{PSU_UNKNOWN, 			PSU_STATE_UNKNOWN},
	{PSU_ON,				PSU_STATE_ON},
	{PSU_OFF, 				PSU_STATE_OFF}
};
static unsigned int down_abrt_opts_num = sizeof(down_abrt_opts)/sizeof(down_abrt_opts[0]);
static unsigned int err_hdl_opts_num = sizeof(err_hdl_opts)/sizeof(err_hdl_opts[0]);
static unsigned int power_status_opts_num = sizeof(power_status_opts)/sizeof(power_status_opts[0]);
static struct power_global power_conf;

static void power_checker_notify(const struct ipmi_conf *, enum ipmi_event, void *);
static void power_checker_exit(const struct ipmi_conf *);
static void power_detect_status(struct power_global *);
static void power_checker_once(struct power_global *);
static void *power_checker(struct power_global *);
static void power_checker_parse_conf(struct power_global *);
static int power_checker_init(const struct ipmi_conf *conf);

struct ipmi_module power_module = {
        .name           = "power_checker",
        .__init         = power_checker_init,
        .__exit         = power_checker_exit,
        .__post	        = power_checker_notify,
        .inited         = 0,
        .exited         = 0,
};


static int
power_checker_init(const struct ipmi_conf *conf)
{
	int iRet;

	memset(&power_conf, 0, sizeof(power_conf));
	power_conf.module = &power_module;
	power_conf.ipmi_conf = conf;

	power_checker_parse_conf(&power_conf);
	
	pthread_mutex_init(&power_conf.ck_mtx, NULL);
	pthread_cond_init(&power_conf.ck_cv, NULL);
	if (pthread_create(&power_conf.checker_wk, NULL, 
				power_checker, &power_conf) != 0)
		goto failed;
	pthread_detach(power_conf.checker_wk);
	return 0;
	
failed:
	return -1;
}

static void
power_checker_parse_conf(struct power_global *conf)
{
	char link_down_abort[128] = {0};
	char err_hdl[128]= {0};
	struct ipmi_conf *ipmi_conf = conf->ipmi_conf;

	assert(ipmi_conf->ic_conf_loaded);
	
	conf->buflen = strlen(POWER_CMD_FORMAT) + 1 - 6 +
				IPMI_USER_LEN + IPMI_PASSWD_LEN + IPMI_IP_LEN;
	conf->check_command = malloc(conf->buflen);
	snprintf(conf->check_command, conf->buflen, POWER_CMD_FORMAT, 
			ipmi_conf->ic_ip, ipmi_conf->ic_user, ipmi_conf->ic_passwd);
	conf->cmd_set = 1;
	
	conf->check_tm = iniGetInt(IPMI_SECT_POWER, 
			IPMI_SECT_POWER_INTERVAL, POWER_INTERVAL);
	conf->retry_times = iniGetInt(IPMI_SECT_POWER, 
			IPMI_SECT_POWER_RETRIES, POWER_RETRIES);
	(void) iniGetString(IPMI_SECT_POWER, IPMI_SECT_POWER_DOWN, 
			link_down_abort, 128, POWER_DOWN_SUSP);
	(void) iniGetString(IPMI_SECT_POWER, IPMI_SECT_POWER_ERR_HDL, 
			err_hdl, 128, POWER_ERR_NEXT_RETRY);

	ipmi_checker_find_opt(down_abrt_opts, link_down_abort, conf->link_down_abort);
	ipmi_checker_find_opt(err_hdl_opts, err_hdl, conf->check_err_hdl);
	syslog(LOG_ERR, "%s command:%s check_tm:%u retry_times:%u link_down:%02x err_hdl:%02x",
		__func__, conf->check_command, conf->check_tm, conf->retry_times, 
		conf->link_down_abort, conf->check_err_hdl);
}

static void *
power_checker(struct power_global *conf)
{
	pthread_mutex_lock(&conf->ck_mtx);
	while (!conf->start_check)
		pthread_cond_wait(&conf->ck_cv, &conf->ck_mtx);

	syslog(LOG_ERR, "%s start to check power status", __func__);
resume:
	for ( ; !conf->checker_exit && !conf->check_susp; ) {
		pthread_mutex_unlock(&conf->ck_mtx);

		power_checker_once(conf);
		pthread_mutex_lock(&conf->ck_mtx);
		if (conf->checker_exit || conf->check_susp)
			continue;
		
		ipmi_delay_interval_locked(&conf->ck_mtx, 
				&conf->ck_cv, conf->check_tm);
	}

	if (conf->checker_exit && !conf->module->exited) {
		conf->module->exited = 1;
		pthread_mutex_unlock(&conf->ck_mtx);
		power_checker_exit(conf->ipmi_conf);
		return NULL;
	}
	
	while (conf->check_susp)
		pthread_cond_wait(&conf->ck_cv, &conf->ck_mtx);
	goto resume;
}

static void
power_checker_once(struct power_global *conf)
{
	unsigned ioc = 0;
	unsigned need_to_ioc = 0;
	
	assert(conf->cmd_set);

	power_detect_status(conf);
	syslog(LOG_ERR, "%s curr_psu_status:%02x", __func__, conf->curr_psu_status);
	switch (conf->curr_psu_status) {
		case PSU_UNKNOWN:
			switch (conf->check_err_hdl) {
				case POWER_ERRHDL_RETRY:
					if (!conf->in_retry)
						conf->in_retry = 1;
					if (++conf->retried < conf->retry_times)
						goto break_retry;

					conf->in_retry = 0;
					conf->retried = 0;
					ioc = IPMI_IOC_PSU_OFF;
					need_to_ioc = 1;
					conf->check_resume = 0;
					conf->check_susp = 1;
break_retry:
					break;
				case POWER_ERRHDL_NEXT:
					break;
				case POWER_ERRHDL_SUSP:
					ioc = IPMI_IOC_PSU_OFF;
					need_to_ioc = 1;
					conf->check_resume = 0;
					conf->check_susp = 1;
					break;
				case POWER_ERRHDL_ABRT:
					conf->checker_exit = 1;
					break;
			}
break_unknown:
			break;
		case PSU_ON:
			ioc = IPMI_IOC_PSU_ON;
			/* @FULLTROUGHT */
		case PSU_OFF:
			if (!ioc)
				ioc = IPMI_IOC_PSU_OFF;
			if (conf->last_psu_status != conf->curr_psu_status)
				need_to_ioc = 1;
			if (conf->in_retry) {
				conf->in_retry = 0;
				conf->retried = 0;
			}
			break;
	}
	if (need_to_ioc)
		(void) ipmi_ioctl(conf->ipmi_conf, 
					ioc, NULL, 0, NULL, 0);
}

static void 
power_detect_status(struct power_global *conf)
{
	char *line = NULL;
	size_t line_sz = 0;
	FILE *fbuf;
	int detected = 0;
	int line_len = 0;

	conf->last_psu_status = conf->curr_psu_status;
	fbuf = popen(conf->check_command, "r");
	if (fbuf) {
		(void) getline(&line, &line_sz, fbuf);
		if (line && line_sz) {
			line_len = strlen(line);
			if (line[line_len-1] = '\n')
				line[line_len-1] = '\0';
			printf("power status: %s line_sz:%d\n", line, line_sz);
			conf->curr_psu_status = PSU_UNKNOWN;
			ipmi_checker_find_opt(power_status_opts, line, conf->curr_psu_status);
			detected = 1;
		}
	}

	if (!detected)
		conf->curr_psu_status = PSU_UNKNOWN;
	syslog(LOG_ERR, "power status: %02x-->%02x", 
		conf->last_psu_status, conf->curr_psu_status);
	if (fbuf)
		pclose (fbuf);
}

static void
power_checker_exit(const struct ipmi_conf *conf)
{

}

static void
power_checker_notify(const struct ipmi_conf *ipmi_conf, 
			enum ipmi_event evt, void *extra)
{
	struct power_global *conf = &power_conf;
	switch (evt) {
		case IPMI_EVT_LINK_DOWN:
//			conf->check_resume = 0;
//			conf->check_susp = 1;
			break;
		case IPMI_EVT_LINK_UP:
			if (!conf->start_check)
				conf->start_check = 1;
			if (conf->check_susp) {
				conf->check_resume = 1;
				conf->check_susp = 0;
			}
//			conf->check_resume = 1;
//			conf->check_susp = 0;
			break;
	}
	pthread_cond_broadcast(&conf->ck_cv);
}
