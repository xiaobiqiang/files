#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <linux/types.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include <syslog.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <sys/list.h>
#include <sys/thread_pool.h>
#include <pthread.h>
#include <linux/un.h>
#include <sys/if_drbdsvc.h>
#include <locale.h>
#include <cmdparse.h>

#define VERSION_STRING_MAX_LEN	10
#define VERSION_STRING_MAJOR	"1"
#define VERSION_STRING_MINOR	"0"

static int drbdctl_set_resume_bp(int, char **, cmdOptions_t *, void *);
static void drbdctl_close(void);
static int drbdctl_read(void **, uint32_t *, uint32_t *);
static int drbdctl_write(struct drbdmon_head *, void *, uint32_t);
static int drbdctl_open(void);

optionTbl_t longOptions[] = {
	{"peer-ip", required_arg, 'i', "peer-ip"},
	{"primary", required_arg, 'p', "is-primary"},
	{"resource-name", required_arg, 'r', "resource-name"},
	{"drbd-minor", required_arg, 'n', "drbd-minor"},
	{NULL, 0, 0, 0}
};

/*
 * Add new subcommands here
 */
subCommandProps_t subcommands[] = {
	{"set-resume-bp", drbdctl_set_resume_bp, "inp", "inp", NULL,
		OPERAND_MANDATORY_SINGLE, "resource-name", NULL},
	{NULL, 0, NULL, NULL, 0, 0, 0, NULL}
};

char 	*cmdName = NULL;
int		sockfd = -1;

static int
drbdctl_open(void)
{
	struct sockaddr_un addr;
	
	if ((sockfd = socket(AF_LOCAL, SOCK_STREAM, 0)) < 0) {
		fprintf(stderr, "%s: %s: create socket failed,error:%s",
			cmdName, __func__, strerror(errno));
		return -1;
	}

	memset(&addr, 0, sizeof(struct sockaddr_un));
	addr.sun_family = AF_LOCAL;
	strncpy(addr.sun_path, DRBDMON_SOCKET, sizeof(addr.sun_path) - 1);
	if (connect(sockfd, &addr, sizeof(addr)) < 0) {
		fprintf(stderr, "%s: %s: connect failed,error:%s\n",
				cmdName, __func__, strerror(errno));
		drbdctl_close();
		return -1;
	}

	return 0;
}

static int
drbdctl_write(struct drbdmon_head *hdp, void *ibuf, uint32_t ilen)
{
	int flags = MSG_NOSIGNAL;
	ssize_t	Sndsz;
	
	assert(sockfd > 0);

	if (ibuf && ilen)
		flags |= MSG_MORE;
	Sndsz = send(sockfd, hdp, sizeof(struct drbdmon_head), flags);
	flags &= ~MSG_MORE;
	if ((Sndsz < sizeof(struct drbdmon_head)) ||
		(send(sockfd, ibuf, ilen, flags) < ilen))
		return -1;

	return 0;
}

static int
drbdctl_read(void **obufp, uint32_t *olenp, uint32_t *error)
{
	struct drbdmon_head head;
	ssize_t Rcvsz = 0;
	int flags = MSG_WAITALL;
	void *obuf = NULL;
	
	assert(sockfd > 0);
	memset(&head, 0, sizeof(struct drbdmon_head));

	Rcvsz = recv(sockfd, &head, sizeof(head), flags);
	if (Rcvsz < sizeof(head))
		return -1;

	if ((head.magic != DRBDMON_MAGIC) ||
		(head.ilen && !(obuf = malloc(head.ilen))) ||
		(head.ilen && (recv(sockfd, obuf, head.ilen, flags) < head.ilen)))
		goto failed;

	if (obufp)
		*obufp = obuf;
	else if (obuf)
		free(obuf);
		
	if (olenp)
		*olenp = head.ilen;
	if (error)
		*error = head.error;
	return 0;
failed:
	if (obuf)
		free(obuf);
	return -1;
}

static void
drbdctl_close(void)
{
	if (sockfd > 0)
		close (sockfd);
}

static int 
drbdctl_set_resume_bp(int operandLen, char *operands[],
			cmdOptions_t *options, void *args)
{
	char peer_ip[16] = {0};
	char resource[128] = {0};
	int minor = -1, primary = -1;
	struct drbdmon_head head;
	struct drbdmon_param_resume_bp param_bp;
	uint32_t error = 0;
	
	for (; options->optval; options++) {
		switch (options->optval) {
			case 'i':
				strncpy(peer_ip, options->optarg, 16);
				break;
			case 'n':
				errno = 0;
				minor = (uint32_t)strtol(options->optarg, NULL, 10);
				if (errno != 0) {
					fprintf(stderr, "%s: %s: %s\n",
				    	cmdName, options->optarg, gettext("invalid minor"));
					return 1;
				}
				break;
			case 'p':
				errno = 0;
				primary = (int)strtol(options->optarg, NULL, 10);
				if (errno != 0) {
					fprintf(stderr, "%s: %s: %s\n",
				    	cmdName, options->optarg, gettext("invalid primary set"));
					return 1;
				}
				break;
			default:
				(void) fprintf(stderr, "%s: %c: %s\n",
				    cmdName, options->optval,
				    gettext("unknown option"));
				return (1);
		}
	}

	strncpy(resource, operands[0], 128);

	if (!strlen(peer_ip) || !strlen(resource) || 
		(minor < 0) || (primary < 0)) {
		fprintf(stderr, "%s: %s: %s\n",
			cmdName, __func__, gettext("invalid parameters"));
		return 1;
	}
	memset(&head, 0, sizeof(head));
	memset(&param_bp, 0, sizeof(param_bp));

	head.ilen = sizeof(struct drbdmon_param_resume_bp);
	head.magic = DRBDMON_MAGIC;
	head.rpc = DRBDMON_RPC_RESUME_BREADPOINT;

	param_bp.primary = (primary ? 1 : 0);
	param_bp.drbdX = minor;
	strncpy(param_bp.peer_ip, peer_ip, 16);
	strncpy(param_bp.resource, resource, 128);

	if (drbdctl_open()) {
		fprintf(stderr, "%s: %s: drbdctl_open failed\n",
				cmdName, __func__);
		return 1;
	}
	if (drbdctl_write(&head, &param_bp, sizeof(param_bp))) {
		fprintf(stderr, "%s: %s: drbdctl_write failed\n",
				cmdName, __func__);
		error = 1;
		goto out;
	}

	if (drbdctl_read(NULL, NULL, &error)) {
		fprintf(stderr, "%s: %s: drbdctl_read failed\n",
				cmdName, __func__);
		error = 1;
		goto out;
	}

out:
	drbdctl_close();
	return error;
}


/*
 * input:
 *  execFullName - exec name of program (argv[0])
 *
 *  copied from usr/src/cmd/zoneadm/zoneadm.c in OS/Net
 *  (changed name to lowerCamelCase to keep consistent with this file)
 *
 * Returns:
 *  command name portion of execFullName
 */
static char *
getExecBasename(char *execFullname)
{
	char *lastSlash, *execBasename;

	/* guard against '/' at end of command invocation */
	for (;;) {
		lastSlash = strrchr(execFullname, '/');
		if (lastSlash == NULL) {
			execBasename = execFullname;
			break;
		} else {
			execBasename = lastSlash + 1;
			if (*execBasename == '\0') {
				*lastSlash = '\0';
				continue;
			}
			break;
		}
	}
	return (execBasename);
}

int
main(int argc, char *argv[])
{
	synTables_t synTables;
	char versionString[VERSION_STRING_MAX_LEN];
	int ret;
	int funcRet;
	void *subcommandArgs = NULL;

	(void) setlocale(LC_ALL, "");
	(void) textdomain(TEXT_DOMAIN);
	
	/* set global command name */
	cmdName = getExecBasename(argv[0]);

	(void) snprintf(versionString, VERSION_STRING_MAX_LEN, "%s.%s",
	    VERSION_STRING_MAJOR, VERSION_STRING_MINOR);
	synTables.versionString = versionString;
	synTables.longOptionTbl = &longOptions[0];
	synTables.subCommandPropsTbl = &subcommands[0];

	ret = cmdParse(argc, argv, synTables, subcommandArgs, &funcRet);
	if (ret != 0) {
		return (ret);
	}

	return (funcRet);
} /* end main */

