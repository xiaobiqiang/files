#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdarg.h>
#include <fcntl.h>           /* Definition of AT_* constants */
#include <unistd.h>

#define CMD_imod(mod)		"/sbin/modprobe %s", \
							(mod)
#define CMD_rmod(mod)		"/sbin/rmmod %s",	\
							(mod)
#define CMD_lmod			"/sbin/lsmod"
#define CMD_emod(mod)		CMD_lmod" | cut -d ' ' -f 1 | grep -w %s | wc -l",	\
							(mod)

#define CMD_UNAME_M			"uname -m"
#define CMD_UNAME_R			"uname -r"

#define FC_QLF				"qlf"
#define FC_QLF_HENGWEI		"qlf_hengwei"
#define FC_DRV_ORIG			"qla2xxx"

#define maybe_sleep_on_cv(cond, mp, cvp, tsp)	\
{	\
	if (!(cond)) {		\
		pthread_cond_timedwait(cvp, mp, tsp);	\
	}	\
}

#define maybe_sleep(cond, count)	\
{	\
	struct timespec ts;			\
	pthread_mutex_t mtx;		\
	pthread_cond_t cv;		\
	int tries = 0;		\
				\
	pthread_mutex_init(&mtx, NULL);	\
	pthread_cond_init(&cv, NULL);	\
				\
	pthread_mutex_lock(&mtx);	\	
	while (!(cond) && (tries++ < count)) {		\
		clock_gettime(CLOCK_REALTIME, &ts);	\
		ts.tv_sec += 1;		\
		maybe_sleep_on_cv(cond, &mtx, &cv, &ts);	\
	}		\
	pthread_mutex_unlock(&mtx);	\
}

static char *fc_drv = NULL;

static os_fc_map_t os_fc_map[] = {
	{"NeoKylin", 				FC_DRV_SW},
	{"CentOS Linux", 			FC_DRV_X86},
	{"deepin GNU/Linux 15.0", 	FC_DRV_SW},
	{"Kylin 4.0.2",				FC_DRV_SW},
	{NULL, NULL}
};

static void init_drv(char **name_ptr);
static void loop_load_drv(char *drv);
	
static int exec_for_echo(char *buf, int len, char *fmt, ...)
{
	FILE *ebuf = NULL;
	char cmd[512] = {0}, *tbuf;
	va_list ap;
	int res = 0;
	
	va_start(ap, fmt);
	vsnprintf(cmd, 512, fmt, ap);
	va_end(ap);

	if (((ebuf = popen(cmd, "r")) == NULL) ||
		((tbuf = fgets(buf, len, ebuf)) == NULL)) 
		res = -1;

	if (ebuf)
		pclose(ebuf);
	return res;
}

static int exec_blank_echo(char *fmt, ...)
{
	char cmd[512] = {0};
	va_list ap;
	
	va_start(ap, fmt);
	vsnprintf(cmd, 512, fmt, ap);
	va_end(ap);

	return system(cmd);
}

static int drv_exist(char *drv)
{
	char echo[64] = "0";

	exec_for_echo(echo, 64, CMD_emod(drv));
	return atoi(echo);
}

int main(int argc, char **argv)
{
	init_drv(&fc_drv);
	fprintf(stdout, "fcboot load fc driver(%s)...\n", fc_drv);
	loop_load_drv(fc_drv);
	return (0);
}

static void init_drv(char **name_ptr)
{
	char uname_m[64] = {0};
	char uname_r[64] = {0};
	
	exec_for_echo(uname_m, sizeof(uname_m), CMD_UNAME_M);
	exec_for_echo(uname_r, sizeof(uname_r), CMD_UNAME_R);

	if (strncmp(uname_m, "x86_64", strlen(uname_m)) == 0 &&
		!strstr(uname_r, "3.10.0")) {
		*name_ptr = strdup(FC_QLF);
	} else {
		*name_ptr = strdup(FC_QLF_HENGWEI);
	}
}
static void loop_load_drv(char *drv)
{
	while (!drv_exist(drv)) {
		while (drv_exist(FC_DRV_ORIG)) {
			exec_blank_echo(CMD_rmod(FC_DRV_ORIG));
			maybe_sleep(drv_exist(FC_DRV_ORIG) == 0, 1);
		}

		/*
		 * sleep some time to load
		 */
		maybe_sleep(0 == 1, 1);
		exec_blank_echo(CMD_imod(drv));
	}
}
