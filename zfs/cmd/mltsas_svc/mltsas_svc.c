#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/nvpair.h>
#include <cmdparse.h>
#include <locale.h>
#include <assert.h>
#include <getopt.h>
#include <errno.h>
#include <mltsas_comm.h>

static int mltsas_enable(int, char **, cmdOptions_t *, void *);
static int mltsas_disable(int, char **, cmdOptions_t *, void *);

#define	VERSION_STRING_MAJOR	    "1"
#define	VERSION_STRING_MINOR	    "0"
#define	VERSION_STRING_MAX_LEN	    10

#define	TEN_MS_NANOSLEEP  10000000

optionTbl_t longOptions[] = {
	{NULL, 0, 0, 0}
};

subCommandProps_t subcommands[] = {
	{"start", mltsas_enable, NULL, NULL, NULL, OPERAND_NONE, NULL},
	{"stop", mltsas_disable, NULL, NULL, NULL, OPERAND_NONE, NULL},
	{NULL, 0, NULL, NULL, 0, NULL, 0, NULL}
};

/* globals */
char *cmdName;

static int mltsas_open(int *fd)
{
	int rval = 0;

	*fd = open(Mltsas_Node, O_RDONLY);
	if (*fd < 0) {
		rval = errno;
		fprintf(stdout, "Open Mltsas Fail,"
			" Error(%d)", errno);
	}
	return (rval);
}

static int mltsas_enable_service(int fd)
{
	
}

static int mltsas_enable(int operandLen, char *operands[], 
		cmdOptions_t *options, void *args)
{
	int	rval = 0;
	int	fd = -1;
	Mlsas_iocdt_t ioc;
	nvlist_t *nvl = NULL;
	const char *path = "/dev/disk/by-id/scsi-35000c500abf3a0c8";
	char *packed = NULL;
	size_t packed_len = 0;
	
	(void) fprintf(stdout, "%s: %s\n", cmdName,
	    gettext("Requesting to enable Mltsas Service"));

	if ((rval = mltsas_open(&fd)) != 0)
		return rval;

	(void) fprintf(stdout, "mltsas_enable [fd=%d]\n", fd);

	if ((rval = ioctl(fd, Mlsas_Ioc_Ensvc, &hostinfo)) != 0) {
		(void) fprintf(stdout, "Unable to issue ioctl: %d", errno);
		return (ret);
	}
	return (ITADM_SUCCESS);
}

static int
mltsas_disable(int operandLen, char *operands[], cmdOptions_t *options,
    void *args)
{
	int	ret;
	int	fd;

	(void) fprintf(stdout, "%s: %s\n", cmdName,
	    gettext("Requesting to disable iscsi target"));

	/* Open the iscsi target node */
	if ((ret = it_open(&fd)) != ITADM_SUCCESS) {
		return (ret);
	}

	/* disable the iSCSI target */
	if ((ret = ioctl(fd, ISCSIT_IOC_DISABLE_SVC, NULL)) != 0) {
		return (ret);
	}
	return (ITADM_SUCCESS);
}

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



int main(int argc, char **argv)
{
    int fd = -1, rval = 0;
    Mlsas_iocdt_t ioc;
	nvlist_t *nvl = NULL;
	const char *path = "/dev/disk/by-id/scsi-35000c500abf3a0c8";
	char *packed = NULL;
	size_t packed_len = 0;
	
	nvlist_alloc(&nvl, NV_UNIQUE_NAME, KM_SLEEP);
	nvlist_add_string(nvl, "path", path);
	nvlist_pack(nvl, &packed, &packed_len, NV_ENCODE_NATIVE, KM_SLEEP);

    fd = open("/dev/Mlsas_dev", O_RDWR);
/*
	memset(&ioc, 0, sizeof(Mlsas_iocdt_t));
	ioc.Mlioc_magic = 0xdeafdeaf;
	rval = ioctl(fd, Mlsas_Ioc_Ensvc, &ioc);
	printf("Mlsas_Ioc_Ensvc(%d)\n", rval);
*/
	memset(&ioc, 0, sizeof(Mlsas_iocdt_t));
	ioc.Mlioc_magic = 0xdeafdeaf;
	ioc.Mlioc_ibufptr = packed;
	ioc.Mlioc_nibuf = packed_len;
    rval = ioctl(fd, Mlsas_Ioc_Newminor, &ioc);
	printf("Mlsas_Ioc_Newminor(%d)\n", rval);
	
    close(fd);
    return rval;
}
