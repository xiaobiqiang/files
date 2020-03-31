#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/time.h>
#include <sys/types.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <libgen.h>
#include <libintl.h>
#include <libuutil.h>
#include <locale.h>
#include <malloc.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <strings.h>
#include <syslog.h>
#include <unistd.h>
#include <stdlib.h>

static FILE *logfile = NULL;
static char *log_name = NULL;
static pid_t log_pid;

#define	LOG_DATE_SIZE	32

#ifndef	PATH_MAX
#define	PATH_MAX	255
#endif

#define	CLUSTER_LOG_FILE	0x1
#define	CLUSTER_LOG_TERMINAL	0x2
#define	CLUSTER_LOG_SYSLOG	0x4

#define	CLUSTER_DEFAULT_LOG	"cluster.log"
#define	CLUSTER_DBGMSG_LOG	"dbgmsgs.log"
#define	DBGMSG_PATH	"/proc/spl/kstat/zfs/dbgmsg"

static int cluster_log_level_min = LOG_INFO;
static uint_t cluster_log_flags = CLUSTER_LOG_FILE|CLUSTER_LOG_SYSLOG;

static char cluster_log_prefix[] = "/var/cluster/log/";
static char trace_script[MAXPATHLEN] = "trace_dbgmsgs.py";

static void
vlog_prefix(int severity, const char *prefix, const char *format, va_list args)
{
	char buf[512], *cp;
	char timebuf[LOG_DATE_SIZE];
	struct timeval now;
	struct tm ltime;

	if (severity > cluster_log_level_min)
		return;

	if (gettimeofday(&now, NULL) != 0)
		(void) fprintf(stderr, "gettimeofday(3C) failed: %s\n",
		    strerror(errno));

	(void) strftime(timebuf, sizeof (timebuf), "%b %e %T",
	    localtime_r(&now.tv_sec, &ltime));

	(void) snprintf(buf, sizeof (buf), "%s %s[%d]%s: ", timebuf,
	    log_name, log_pid, prefix);
	cp = strchr(buf, '\0');
	(void) vsnprintf(cp, sizeof (buf) - (cp - buf), format, args);

	if (cluster_log_flags & CLUSTER_LOG_SYSLOG || !logfile)
		syslog(severity, "%s", cp);

	(void) strcat(buf, "\n");

	if (cluster_log_flags & CLUSTER_LOG_FILE && logfile) {
		(void) fputs(buf, logfile);
		(void) fflush(logfile);
	}
	if (cluster_log_flags & CLUSTER_LOG_TERMINAL)
		(void) fputs(buf, stdout);
}

void
cluster_log_error(int severity, const char *format, ...)
{
	va_list args;

	va_start(args, format);
	vlog_prefix(severity, " ERROR", format, args);
	va_end(args);
}

void
cluster_log_framework(int severity, const char *format, ...)
{
	va_list args;

	va_start(args, format);
	vlog_prefix(severity, "", format, args);
	va_end(args);
}

static int
log_dir_writeable(const char *path)
{
	int fd;
	struct statvfs svb;

	if ((fd = open(path, O_RDONLY, 0644)) == -1)
		return (-1);

	if (fstatvfs(fd, &svb) == -1)
		return (-1);

	if (svb.f_flag & ST_RDONLY) {
		(void) close(fd);

		fd = -1;
	}

	return (fd);
}

void
cluster_log_init(const char *execname, const char *logpath,
	int log_level, int log_flags)
{
	int dirfd, logfd;
	const char *dir;

	if (log_level > 0)
		cluster_log_level_min = log_level;
	if (log_flags > 0)
		cluster_log_flags = log_flags;

	if (logfile) {
		(void) fclose(logfile);
		logfile = NULL;
	}

	closelog();
	openlog(execname, LOG_PID | LOG_NDELAY, LOG_DAEMON);
	(void) setlogmask(LOG_UPTO(LOG_NOTICE));

	if ((dirfd = log_dir_writeable(cluster_log_prefix)) == -1) {
		mkdirp(cluster_log_prefix, 0644);
		if ((dirfd = log_dir_writeable(cluster_log_prefix)) == -1)
			return;
	}

	dir = logpath != NULL ? logpath : cluster_log_prefix;

	if ((logfd = openat(dirfd, CLUSTER_DEFAULT_LOG, O_CREAT | O_RDWR,
	    0644)) == -1) {
		(void) close(dirfd);
		return;
	}

	(void) close(dirfd);

	if ((logfile = fdopen(logfd, "a")) == NULL)
		if (errno != EROFS)
			cluster_log_error(LOG_WARNING, "can't open logfile %s/%s",
			    dir, CLUSTER_DEFAULT_LOG);

	if (logfile &&
	    fcntl(fileno(logfile), F_SETFD, FD_CLOEXEC) == -1)
		cluster_log_error(LOG_WARNING,
		    "couldn't mark logfile close-on-exec: %s\n",
		    strerror(errno));

	log_name = strdup(execname);
	log_pid = getpid();
}

static void
trace_dbgmsgs(void)
{
	char buf[512];
	sprintf(buf, "%s -p %s/%s",
		trace_script, cluster_log_prefix, CLUSTER_DBGMSG_LOG);
	system(buf);
}

static void *
trace_dbgmsgs_thread(void *arg)
{
	pthread_detach(pthread_self());
	while (1) {
		trace_dbgmsgs();
	}
	return (NULL);
}

void
trace_dbgmsgs_init(const char *script_path)
{
	pthread_t tid;
	int error;

	if (script_path)
		strcpy(trace_script, script_path);

	error = pthread_create(&tid, NULL, trace_dbgmsgs_thread, NULL);
	if (error) {
		cluster_log_error(LOG_WARNING,
			"couldn't create thread: %s\n",
			strerror(error));
	}
}

