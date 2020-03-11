#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdarg.h>
#include <fcntl.h>           /* Definition of AT_* constants */
#include <unistd.h>

#define CMD_imod(mod)		"/usr/sbin/modprobe %s", \
							(mod)
#define CMD_rmod(mod)		"/usr/sbin/rmmod %s",	\
							(mod)
#define CMD_lmod			"/usr/sbin/lsmod"
#define CMD_emod(mod)		CMD_lmod" | cut -d ' ' -f 1 | grep -w %s | wc -l",	\
							(mod)
#define OS_RELEASE			"/etc/redhat-release"
#define CMD_os_release		"cat "OS_RELEASE

#define FC_DRV_SW			"qlf_hengwei"
#define FC_DRV_X86			"qlf"
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

typedef struct os_fc_map {
	char *os;
	char *driver;
} os_fc_map_t;

static char *fc_drv = NULL;
static os_fc_map_t os_fc_map[] = {
	{"NeoKylin Server release 5.0 (Sunway)", FC_DRV_SW},
	{"CentOS Linux release 7.3.1611 (Core)", FC_DRV_X86},
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

/*
 * default is FC_DRV_X86
 */
static void search_fc_drv(const char *os, char **drv_p)
{
	os_fc_map_t *mapping = &os_fc_map;
	int idx = 0;
	char *iter_os = NULL;

	while ((iter_os = mapping[idx++].os) != NULL) {
		if (strncmp(iter_os, os, strlen(iter_os)) == 0)
			break;
	}

	*drv_p = iter_os ? strdup(mapping[idx - 1].driver) :
		strdup(FC_DRV_X86);
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
	loop_load_drv(fc_drv);
}

static void init_drv(char **name_ptr)
{
	char os_name[128] = {0};
	
	maybe_sleep(access(OS_RELEASE, F_OK) == 0, -1U);
	exec_for_echo(os_name, 128, CMD_os_release);
	search_fc_drv(os_name, name_ptr);
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