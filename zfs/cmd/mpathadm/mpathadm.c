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
#include <cmdparse.h>
#include <sys/zfs_context.h>
#include <sys/avl.h>
#include <mltsas_comm.h>

#define MPATH_ADM_OK			0

#define VERSION_STRING_MAX_LEN	32
#define VERSION_STRING_MAJOR	"1"
#define VERSION_STRING_MINOR	"0"

#define	OPTIONSTRING_LUNIT	"logical-unit"
#define OPTIONSTRING_ALL	"all"

static int __mpath_adm_list(int operandLen, char *operands[], 
		cmdOptions_t *options, void *args);

static optionTbl_t longOptions[] = {
	{"logical-unit", 	required_arg, 	'l', OPTIONSTRING_LUNIT},
	{"all", 			no_arg, 		'a', OPTIONSTRING_ALL},
	{NULL, 0, 0, 0}
};

static subCommandProps_t subcommands[] = {
	{"list", __mpath_adm_list, "la", NULL, NULL, OPERAND_OPTIONAL_SINGLE, OPTIONSTRING_LUNIT},
	{NULL, 0, NULL, NULL, 0, NULL, 0, NULL}
};

static char *cmdName;

static int __mpath_adm_open_node(void)
{
	int rval = MPATH_ADM_OK;

	if (((rval = open(Mltsas_Node, O_RDONLY)) < 0) 
			&& errno)
		rval = (errno > 0) ? -errno : errno;
	
	return (rval);
}

static void __mpath_adm_close_node(int fd)
{
	if (fd > 0)
		close(fd);
}

static void __mpath_adm_fill_mlsas_ioc_data(Mlsas_iocdt_t *xd,
		void *ibuf, uint32_t ilen, 
		void *obuf, uint32_t olen)
{
	xd->Mlioc_magic = Mlsas_Ioc_Magic;
	xd->Mlioc_nofill = 0;
	xd->Mlioc_ibufptr = ibuf;
	xd->Mlioc_nibuf = ilen;
	xd->Mlioc_obufptr = obuf;
	xd->Mlioc_nobuf = olen;
}

static int __mpath_adm_do_ioctl(int node_fd, Mlsas_ioc_e cmd, 
		Mlsas_iocdt_t *xd)
{
	int rval = MPATH_ADM_OK;
	struct timespec ts; 

	ts.tv_sec = 0;
	ts.tv_nsec = 10000000;

again:
	errno = 0;
	if ((rval = ioctl(node_fd, cmd, xd)) 
			!= MPATH_ADM_OK) {
		printf("errno(%d)\n", errno);
		if (errno == EAGAIN) {
			nanosleep(&ts, NULL);
			goto again;
		}
		rval = errno ?: rval;
	}

	return rval;
}

static int __mpath_adm_get_lu_list(int node_fd, const char *LU, 
		int *count, mpath_adm_lu_info_t **info_list)
{
	nvlist_t *nvl = NULL;
	Mlsas_iocdt_t xd;
	char *packed = NULL, *obuf = NULL;
	size_t packed_len = 0, obuflen = 2 << 15;
	int rval = MPATH_ADM_OK, nc = 0;
	mpath_adm_lu_info_t *li = NULL;

	VERIFY(nvlist_alloc(&nvl, NV_UNIQUE_NAME, KM_SLEEP) == 0);
	
	if (LU == NULL)
		LU = "ALL";

	if (strncmp(LU, "ALL", 3) != 0)
		obuflen = sizeof(mpath_adm_lu_info_t) + 8;

	if (((rval = nvlist_add_string(nvl, "path", 
			LU)) != MPATH_ADM_OK) ||
		((rval = nvlist_pack(nvl, &packed, &packed_len, 
			NV_ENCODE_NATIVE, 
			KM_SLEEP)) != MPATH_ADM_OK))
		goto out;

	obuf = kmem_zalloc(obuflen, KM_SLEEP);
	__mpath_adm_fill_mlsas_ioc_data(&xd, packed, packed_len, obuf, obuflen);

	if ((rval = __mpath_adm_do_ioctl(node_fd, 
			Mlsas_Ioc_LuInfo, &xd)) != MPATH_ADM_OK) 
		goto free_obuf;

	nc = *((uint32_t *)obuf);
	li = kmem_zalloc(sizeof(mpath_adm_lu_info_t) * nc, KM_SLEEP);
	*count = nc;
	bcopy(obuf + 8, li, sizeof(mpath_adm_lu_info_t) * nc);

	*info_list = li;

free_obuf:
	if (obuf && obuflen)
		kmem_free(obuf, obuflen);
out:
	if (packed && packed_len)
		kmem_free(packed, packed_len);
	if (nvl)
		nvlist_free(nvl);
	return rval;
}

static void __mpath_adm_print_lu_info_list(uint32_t nLu, 
		mpath_adm_lu_info_t *li)
{
	int i = 0;
	mpath_adm_lu_info_t *iter = li;
	
	for ( ; i < nLu; i++, li++) {
		printf("\t%s\n", li->li_name);
		printf("\t\tTotal Path Count: %u\n", li->li_path_count);
		printf("\t\tOperational Path Count: %u\n", li->li_opt_path_count);
		printf("\t\tPrimary Path: mpt_sas%u\n", li->li_active_path);
	}
}

static int __mpath_adm_list(int operandLen, char *operands[], 
		cmdOptions_t *options, void *args)
{
	boolean_t list_all = B_TRUE, has_all_opt = B_FALSE;
	char *LU = NULL;
	int node_fd, rval, nLU = 0;
	mpath_adm_lu_info_t *li = NULL;
	
	for ( ; options->optval; options++) {
		switch (options->optval) {
		case 'a': 
			has_all_opt = B_TRUE;
			list_all = B_TRUE; 
			break;
		case 'l':
			LU = options->optarg;
			break;
		default:
			(void) fprintf(stderr, "%s: %c: %s\n",
				    cmdName, options->optval,
				    gettext("unknown option"));
			return -EINVAL;
		}
	}

	if (LU && !has_all_opt)
		list_all = B_FALSE;

	if (list_all)
		LU = NULL;

	if ((node_fd = __mpath_adm_open_node()) < 0) {
		(void) fprintf(stderr, "%s: FAIL when open mltsas node,"
				"ERROR(%d)\n", cmdName, node_fd);
		rval = node_fd;
		goto out;
	}

	if ((rval = __mpath_adm_get_lu_list(node_fd, LU, &nLU, 
			&li)) != MPATH_ADM_OK) {
		(void) fprintf(stderr, "%s: FAIL when get LU info list,"
				"ERROR(%d)\n", cmdName, rval);
		goto close_node;
	}

	__mpath_adm_print_lu_info_list(nLU, li);

	if (nLU && li)
		kmem_free(li, sizeof(mpath_adm_lu_info_t) * nLU);
	
close_node:
	__mpath_adm_close_node(node_fd);
out:
	return rval;
}

static char *getExecBasename(char *execFullname)
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

int main(int argc, char **argv)
{
	int rval, funcRet = 0;
	char versionString[VERSION_STRING_MAX_LEN];
	synTables_t synTables;
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

	rval = cmdParse(argc, argv, synTables, subcommandArgs, &funcRet);
	if (funcRet != 0)
		rval = funcRet;

	return rval;
}
