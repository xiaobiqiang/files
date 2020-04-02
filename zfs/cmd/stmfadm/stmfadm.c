#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <wctype.h>
#include <strings.h>
#include <sys/types.h>
#include <unistd.h>
#include <wchar.h>
#include <libintl.h>
#include <errno.h>
#include <time.h>
#include <string.h>
#include <assert.h>
#include <getopt.h>
#include <libstmf.h>
#include <signal.h>
#include <pthread.h>
#include <locale.h>
#include <stdio.h>
#include <stdio_ext.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <syslog.h>
#include <libzfs.h>
#include "cmdparse.h"
#include "stmfadm.h"

typedef enum { false = 0, true = 1 } bool_t;

static int addHostGroupMemberFunc(int, char **, cmdOptions_t *, void *);
static int addTargetGroupMemberFunc(int, char **, cmdOptions_t *, void *);
static int addViewFunc(int, char **, cmdOptions_t *, void *);
static int createHostGroupFunc(int, char **, cmdOptions_t *, void *);
static int createLuFunc(int, char **, cmdOptions_t *, void *);
static int modifyLuFunc(int, char **, cmdOptions_t *, void *);
static int importLuFunc(int, char **, cmdOptions_t *, void *);
static int deleteLuFunc(int, char **, cmdOptions_t *, void *);
static int createTargetGroupFunc(int, char **, cmdOptions_t *, void *);
static int deleteHostGroupFunc(int, char **, cmdOptions_t *, void *);
static int deleteTargetGroupFunc(int, char **, cmdOptions_t *, void *);
static int listLuFunc(int, char **, cmdOptions_t *, void *);
static int listTargetFunc(int, char **, cmdOptions_t *, void *);
static int listViewFunc(int, char **, cmdOptions_t *, void *);
static int listHostGroupFunc(int, char **, cmdOptions_t *, void *);
static int listStateFunc(int, char **, cmdOptions_t *, void *);
static int listTargetGroupFunc(int, char **, cmdOptions_t *, void *);
static int offlineTargetFunc(int, char **, cmdOptions_t *, void *);
static int offlineLuFunc(int, char **, cmdOptions_t *, void *);
static int onlineTargetFunc(int, char **, cmdOptions_t *, void *);
static int onlineLuFunc(int, char **, cmdOptions_t *, void *);
static int onlineOfflineTarget(char *, int);
static int onlineOfflineLu(char *, int);
static int removeHostGroupMemberFunc(int, char **, cmdOptions_t *, void *);
static int removeTargetGroupMemberFunc(int, char **, cmdOptions_t *, void *);
static int callModify(char *, stmfGuid *, uint32_t, const char *, const char *);
static int removeViewFunc(int, char **, cmdOptions_t *, void *);
static char *getExecBasename(char *);
static int parseDevid(char *input, stmfDevid *devid);
static void printGroupProps(stmfGroupProperties *groupProps);
static int checkScsiNameString(wchar_t *, stmfDevid *);
static int checkHexUpper(char *);
static int checkIscsiName(wchar_t *);
static void printLuProps(stmfLogicalUnitProperties *luProps);
static int printExtLuProps(stmfGuid *guid);
static void printGuid(stmfGuid *guid, FILE *printWhere);
static void printTargetProps(stmfTargetProperties *);
static void printSessionProps(stmfSessionList *);
static int setLuPropFromInput(luResource, char *);
static int convertCharToPropId(char *, uint32_t *);
static int unmapLuFunc(int, char **, cmdOptions_t *, void *);
static int clearTraceFunc(int, char **, cmdOptions_t *, void *);

static int getTraceFunc(int, char **, cmdOptions_t *, void *);
static int stmfadm_send_cmd(char * cmd_name);
static void stmfadm_no_receive(int recv);

static int setTaskLimitFunc(int, char **, cmdOptions_t *, void *);
static int getTaskInfoFunc(int, char **, cmdOptions_t *, void *);
static int setIopsLimitFunc(int, char **, cmdOptions_t *, void *);
static int getIopsInfoFunc(int, char **, cmdOptions_t *, void *);
static int listAllLunsFunc(int, char **, cmdOptions_t *, void *);
static int setKbpsFunc(int, char **, cmdOptions_t *, void *);
static int getKbpsFunc(int, char **, cmdOptions_t *, void *);
static int bindDrbdFunc(int, char **, cmdOptions_t *, void *);
static int delDrbdFunc(int, char **, cmdOptions_t *, void *);

/*
 *  MAJOR - This should only change when there is an incompatible change made
 *  to the interfaces or the output.
 *
 *  MINOR - This should change whenever there is a new command or new feature
 *  with no incompatible change.
 */
#define	VERSION_STRING_MAJOR	    "1"
#define	VERSION_STRING_MINOR	    "0"
#define	MAX_DEVID_INPUT		    256
#define	GUID_INPUT		    32
#define	MAX_LU_NBR		    16383
#define	ONLINE_LU		    0
#define	OFFLINE_LU		    1
#define	ONLINE_TARGET		    2
#define	OFFLINE_TARGET		    3
#define	PROPS_FORMAT		    "    %-18s: "
#define	VIEW_FORMAT		    "    %-13s: "
#define	LVL3_FORMAT		    "        %s"
#define	LVL4_FORMAT		    "            %s"


/* SCSI Name String length definitions */
#define	SNS_EUI_16		    16
#define	SNS_EUI_24		    24
#define	SNS_EUI_32		    32
#define	SNS_NAA_16		    16
#define	SNS_NAA_32		    32
#define	SNS_WWN_16		    16
#define	SNS_IQN_223		    223

/* LU Property strings */
#define	GUID			    "GUID"
#define	ALIAS			    "ALIAS"
#define	VID			    "VID"
#define	PID			    "PID"
#define	META_FILE		    "META"
#define	WRITE_PROTECT		    "WP"
#define	WRITEBACK_CACHE_DISABLE	    "WCD"
#define	COMPANY_ID		    "OUI"
#define	BLOCK_SIZE		    "BLK"
#define	SERIAL_NUMBER		    "SERIAL"
#define	MGMT_URL		    "MGMT-URL"
#define	HOST_ID			    "HOST-ID"

#define KBPS_G	(1ULL << 30)
#define KBPS_M	(1ULL << 20)
#define KBPS_K	1024

#define	MODIFY_HELP "\n"\
"Description: Modify properties of a logical unit. \n" \
"Valid properties for -p, --lu-prop are: \n" \
"     alias    - alias for logical unit (up to 255 chars)\n" \
"     mgmt-url - Management URL address\n" \
"     wcd      - write cache disabled (true, false)\n" \
"     wp       - write protect (true, false)\n\n" \
"-f alters the meaning of the operand to be a file name\n" \
"rather than a LU name. This allows for modification\n" \
"of a logical unit that is not yet imported into stmf\n"

#define	CREATE_HELP "\n"\
"Description: Create a logical unit. \n" \
"Valid properties for -p, --lu-prop are: \n" \
"     alias    - alias for logical unit (up to 255 chars)\n" \
"     blk      - block size in bytes in 2^n\n" \
"     guid     - 32 ascii hex characters in NAA format \n" \
"     host-id  - host identifier to be used for GUID generation \n" \
"                8 ascii hex characters\n" \
"     meta     - separate meta data file name\n" \
"     mgmt-url - Management URL address\n" \
"     oui      - organizational unique identifier\n" \
"                6 ascii hex characters of valid format\n" \
"     pid      - product identifier (up to 16 chars)\n" \
"     serial   - serial number (up to 252 chars)\n" \
"     vid      - vendor identifier (up to 8 chars)\n" \
"     wcd      - write cache disabled (true, false)\n" \
"     wp       - write protect (true, false)\n"
#define	ADD_VIEW_HELP "\n"\
"Description: Add a view entry to a logical unit. \n" \
"A view entry is comprised of three elements; the \n" \
"logical unit number, the target group name and the\n" \
"host group name. These three elements combine together\n" \
"to form a view for a given COMSTAR logical unit.\n" \
"This view is realized by a client, a SCSI initiator,\n" \
"via a REPORT LUNS command. \n"



/* tables set up based on cmdparse instructions */

/* add new options here */
optionTbl_t longOptions[] = {
	{"all", no_arg, 'a', NULL},
	{"group-name", required_arg, 'g', "group-name"},
	{"keep-views", no_arg, 'k', NULL},
	{"lu-name", required_arg, 'l', "LU-Name"},
	{"lun", required_arg, 'n', "logical-unit-number"},
	{"lu-prop", required_arg, 'p', "logical-unit-property=value"},
	{"file", no_arg, 'f', "filename"},
	{"size", required_arg, 's', "size K/M/G/T/P"},
	{"target-group", required_arg, 't', "group-name"},
	{"host-group", required_arg, 'h', "group-name"},
	{"verbose", no_arg, 'v', NULL},
	{"cluster",no_arg,'c',NULL},
	{"drbd-bdev", required_arg, 'd', "drbd-block-device"},
	{NULL, 0, 0, 0}
};

/*
 * Add new subcommands here
 */
subCommandProps_t subcommands[] = {
	{"add-hg-member", addHostGroupMemberFunc, "gc", "g", NULL,
		OPERAND_MANDATORY_MULTIPLE, OPERANDSTRING_GROUP_MEMBER, NULL},
	{"add-tg-member", addTargetGroupMemberFunc, "gc", "g", NULL,
		OPERAND_MANDATORY_MULTIPLE, OPERANDSTRING_GROUP_MEMBER, NULL},
	{"add-view", addViewFunc, "nthc", NULL, NULL,
		OPERAND_MANDATORY_SINGLE, OPERANDSTRING_LU, ADD_VIEW_HELP},
	{"create-hg", createHostGroupFunc,"c", NULL, NULL,
		OPERAND_MANDATORY_SINGLE, OPERANDSTRING_GROUP_NAME, NULL},
	{"create-tg", createTargetGroupFunc,"c", NULL, NULL,
		OPERAND_MANDATORY_SINGLE, OPERANDSTRING_GROUP_NAME, NULL},
	{"create-lu", createLuFunc, "ps", NULL, NULL, OPERAND_MANDATORY_SINGLE,
		"lu file", CREATE_HELP},
	{"delete-hg", deleteHostGroupFunc, "c", NULL, NULL,
		OPERAND_MANDATORY_SINGLE, OPERANDSTRING_GROUP_NAME, NULL},
	{"modify-lu", modifyLuFunc, "psf", NULL, NULL, OPERAND_MANDATORY_SINGLE,
		OPERANDSTRING_LU, MODIFY_HELP},
	{"delete-lu", deleteLuFunc, "kc", NULL, NULL,
		OPERAND_MANDATORY_MULTIPLE, OPERANDSTRING_LU, NULL},
	{"delete-tg", deleteTargetGroupFunc, "c", NULL, NULL,
		OPERAND_MANDATORY_SINGLE, OPERANDSTRING_GROUP_NAME, NULL},
	{"import-lu", importLuFunc, NULL, NULL, NULL,
		OPERAND_MANDATORY_SINGLE, "file name", NULL},
	{"list-hg", listHostGroupFunc, "v", NULL, NULL,
		OPERAND_OPTIONAL_MULTIPLE, OPERANDSTRING_GROUP_NAME, NULL},
	{"list-lu", listLuFunc, "v", NULL, NULL, OPERAND_OPTIONAL_MULTIPLE,
		OPERANDSTRING_LU, NULL},
	{"list-state", listStateFunc, NULL, NULL, NULL, OPERAND_NONE, NULL},
	{"list-target", listTargetFunc, "v", NULL, NULL,
		OPERAND_OPTIONAL_MULTIPLE, OPERANDSTRING_TARGET, NULL},
	{"list-tg", listTargetGroupFunc, "v", NULL, NULL,
		OPERAND_OPTIONAL_MULTIPLE, OPERANDSTRING_GROUP_NAME, NULL},
	{"list-view", listViewFunc, "l", "l", NULL,
		OPERAND_OPTIONAL_MULTIPLE, OPERANDSTRING_VIEW_ENTRY, NULL},
	{"online-lu", onlineLuFunc, NULL, NULL, NULL,
		OPERAND_MANDATORY_SINGLE, OPERANDSTRING_LU, NULL},
	{"offline-lu", offlineLuFunc, NULL, NULL, NULL,
		OPERAND_MANDATORY_SINGLE, OPERANDSTRING_LU, NULL},
	{"online-target", onlineTargetFunc, "c", NULL, NULL,
		OPERAND_MANDATORY_SINGLE, OPERANDSTRING_TARGET, NULL},
	{"offline-target", offlineTargetFunc, "c", NULL, NULL,
		OPERAND_MANDATORY_SINGLE, OPERANDSTRING_TARGET, NULL},
	{"remove-hg-member", removeHostGroupMemberFunc, "gc", "g", NULL,
		OPERAND_MANDATORY_MULTIPLE, OPERANDSTRING_GROUP_MEMBER, NULL},
	{"remove-tg-member", removeTargetGroupMemberFunc, "gc", "g", NULL,
		OPERAND_MANDATORY_MULTIPLE, OPERANDSTRING_GROUP_MEMBER, NULL},
	{"remove-view", removeViewFunc, "lac", "l", NULL,
		OPERAND_OPTIONAL_MULTIPLE, OPERANDSTRING_VIEW_ENTRY, NULL},
	{"set-drbd", bindDrbdFunc, "d", "d", NULL, 
		OPERAND_MANDATORY_SINGLE, OPERANDSTRING_LU, NULL},
	{"del-drbd", delDrbdFunc, NULL, NULL, NULL, 
		OPERAND_MANDATORY_SINGLE, OPERANDSTRING_LU, NULL},
	{"unmap-lu", unmapLuFunc, NULL, NULL, NULL,
		OPERAND_MANDATORY_SINGLE, OPERANDSTRING_LU, NULL},
	{"clear-trace", clearTraceFunc, NULL, NULL, NULL, OPERAND_NONE, NULL},
	{"get-trace", getTraceFunc, NULL, NULL, NULL, OPERAND_NONE, NULL},
	{"set-task-limit", setTaskLimitFunc, "lnc", "ln", NULL,
		OPERAND_OPTIONAL_MULTIPLE, OPERANDSTRING_LU, NULL},	
	{"get-task-info", getTaskInfoFunc, NULL, NULL, NULL,
		OPERAND_MANDATORY_SINGLE, OPERANDSTRING_LU, NULL},
	{"set-iops-limit", setIopsLimitFunc, "lnc", "ln", NULL,
		OPERAND_OPTIONAL_MULTIPLE, OPERANDSTRING_LU, NULL},	
	{"get-iops-info", getIopsInfoFunc, NULL, NULL, NULL,
		OPERAND_MANDATORY_SINGLE, OPERANDSTRING_LU, NULL},
    {"list-all-luns", listAllLunsFunc, NULL, NULL, NULL,
        OPERAND_OPTIONAL_MULTIPLE, OPERANDSTRING_LU, NULL},    
	{"set-kbps", setKbpsFunc, "lnc", "ln", NULL,
		OPERAND_OPTIONAL_MULTIPLE, OPERANDSTRING_LU, NULL},	
	{"get-kbps", getKbpsFunc, NULL, NULL, NULL,
		OPERAND_MANDATORY_SINGLE, OPERANDSTRING_LU, NULL},
	{NULL, 0, NULL, NULL, 0, 0, 0, NULL}
};

/* globals */
char *cmdName;

int stmf_proxy_door_fd;
/*save input order*/
static char cmdfullName[512]={"\0"};


static int
bindDrbdFunc(int operandLen, char *operands[], cmdOptions_t *options, void *args)
{
	int i;
	int stmfRet;
	char drbd_path[64] = {0};
	char sGuid[33] = {0};
	unsigned char guid[sizeof(stmfGuid)];
	stmfGuid inGuid;
	
	for (; options->optval; options++) {
		switch (options->optval) {
			case 'd':
				strncpy(drbd_path, options->optarg, 64);
				break;
			default:
				(void) fprintf(stderr, "%s: %c: %s\n",
				    cmdName, options->optval,
				    gettext("unknown option"));
				return (1);
		}
	}

	for (i = 0; i < 32; i++)
		sGuid[i] = tolower(operands[0][i]);
	sGuid[i] = 0;
	(void) sscanf(sGuid,
	    "%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x",
	    &guid[0], &guid[1], &guid[2], &guid[3], &guid[4], &guid[5],
	    &guid[6], &guid[7], &guid[8], &guid[9], &guid[10],
	    &guid[11], &guid[12], &guid[13], &guid[14], &guid[15]);

	for (i = 0; i < sizeof (stmfGuid); i++) {
		inGuid.guid[i] = guid[i];
	}

	return stmfLuBindDrbd(&inGuid, &drbd_path[0]);
}

static int
delDrbdFunc(int operandLen, char *operands[], cmdOptions_t *options, void *args)
{
	int i;
	int stmfRet;
	char sGuid[33] = {0};
	unsigned char guid[sizeof(stmfGuid)];
	stmfGuid inGuid;

	for (i = 0; i < 32; i++)
		sGuid[i] = tolower(operands[0][i]);
	sGuid[i] = 0;
	(void) sscanf(sGuid,
	    "%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x",
	    &guid[0], &guid[1], &guid[2], &guid[3], &guid[4], &guid[5],
	    &guid[6], &guid[7], &guid[8], &guid[9], &guid[10],
	    &guid[11], &guid[12], &guid[13], &guid[14], &guid[15]);

	for (i = 0; i < sizeof (stmfGuid); i++) {
		inGuid.guid[i] = guid[i];
	}

	return stmfLuUnbindDrbd(&inGuid);
}


/*
 * addHostGroupMemberFunc
 *
 * Add members to a host group
 *
 */
/*ARGSUSED*/
static int
addHostGroupMemberFunc(int operandLen, char *operands[], cmdOptions_t *options,
    void *args)
{
	int i;
	int ret = 0;
	int stmfRet;
	stmfGroupName groupName = {0};
	wchar_t groupNamePrint[sizeof (stmfGroupName)] = {0};
	stmfDevid devid;
	stmf_proxy_device_t *proxy_device;
	
	bool_t isCluster = false;

	for (; options->optval; options++) {
		switch (options->optval) {
			/* host group name */
			case 'g':
				(void) mbstowcs(groupNamePrint, options->optarg,
				    sizeof (stmfGroupName) - 1);
				bcopy(options->optarg, groupName,
				    strlen(options->optarg));
				break;
			case 'c':
				isCluster = true;
				break;
			default:
				(void) fprintf(stderr, "%s: %c: %s\n",
				    cmdName, options->optval,
				    gettext("unknown option"));
				return (1);
		}
	}

	for (i = 0; i < operandLen; i++) {
		if (parseDevid(operands[i], &devid) != 0) {
			(void) fprintf(stderr, "%s: %s: %s\n",
			    cmdName, operands[i],
			    gettext("unrecognized device id"));
			ret++;
			continue;
		}
		stmfRet = stmfAddToHostGroup(&groupName, &devid);
		switch (stmfRet) {
			case STMF_STATUS_SUCCESS:
				proxy_device = malloc(sizeof(stmf_proxy_device_t));
				proxy_device->head.item_type = STMF_HG_MEMBER_OP;
				proxy_device->head.op_type = STMF_OP_ADD;
				bcopy(&groupName, &proxy_device->name, sizeof(stmfGroupName));
				bcopy(&devid, &proxy_device->device, sizeof(stmfDevid));
				free(proxy_device);
				
				if (true == isCluster)
					stmfadm_send_cmd(cmdfullName);
				break;
			case STMF_ERROR_EXISTS:
				(void) fprintf(stderr, "%s: %s: %s\n", cmdName,
				    operands[i], gettext("already exists"));
				ret++;
				break;
			case STMF_ERROR_GROUP_NOT_FOUND:
				(void) fprintf(stderr, "%s: %ls: %s\n", cmdName,
				    groupNamePrint, gettext("not found"));
				ret++;
				break;
			case STMF_ERROR_PERM:
				(void) fprintf(stderr, "%s: %s\n", cmdName,
				    gettext("permission denied"));
				break;
			case STMF_ERROR_BUSY:
				(void) fprintf(stderr, "%s: %s: %s\n", cmdName,
				    operands[i], gettext("resource busy"));
				ret++;
				break;
			case STMF_ERROR_SERVICE_NOT_FOUND:
				(void) fprintf(stderr, "%s: %s\n", cmdName,
				    gettext("STMF service not found"));
				ret++;
				break;
			case STMF_ERROR_SERVICE_DATA_VERSION:
				(void) fprintf(stderr, "%s: %s\n", cmdName,
				    gettext("STMF service version incorrect"));
				ret++;
				break;
			default:
				(void) fprintf(stderr, "%s: %s: %s\n", cmdName,
				    operands[i], gettext("unknown error"));
				ret++;
				break;
		}
	}

	return (ret);
}

/*
 * addTargetGroupMemberFunc
 *
 * Add members to a target group
 *
 */
/*ARGSUSED*/
static int
addTargetGroupMemberFunc(int operandLen, char *operands[],
    cmdOptions_t *options, void *args)
{
	int i;
	int ret = 0;
	int stmfRet;
	stmfGroupName groupName = {0};
	wchar_t groupNamePrint[sizeof (stmfGroupName)] = {0};
	stmfDevid devid;
	stmf_proxy_device_t *proxy_device;

	bool_t isCluster = false;

	for (; options->optval; options++) {
		switch (options->optval) {
			/* target group name */
			case 'g':
				(void) mbstowcs(groupNamePrint, options->optarg,
				    sizeof (stmfGroupName) - 1);
				bcopy(options->optarg, groupName,
				    strlen(options->optarg));
				break;
			case 'c':
				isCluster = true;
				break;
			default:
				(void) fprintf(stderr, "%s: %c: %s\n",
				    cmdName, options->optval,
				    gettext("unknown option"));
				return (1);
		}
	}

	for (i = 0; i < operandLen; i++) {
		if (parseDevid(operands[i], &devid) != 0) {
			(void) fprintf(stderr, "%s: %s: %s\n",
			    cmdName, operands[i],
			    gettext("unrecognized device id"));
			ret++;
			continue;
		}
		stmfRet = stmfAddToTargetGroup(&groupName, &devid);
		switch (stmfRet) {
			case STMF_STATUS_SUCCESS:
				proxy_device = malloc(sizeof(stmf_proxy_device_t));
				proxy_device->head.item_type = STMF_TG_MEMBER_OP;
				proxy_device->head.op_type = STMF_OP_ADD;
				bcopy(&groupName, &proxy_device->name, sizeof(stmfGroupName));
				bcopy(&devid, &proxy_device->device, sizeof(stmfDevid));
				free(proxy_device);

				if (true == isCluster)
					stmfadm_send_cmd(cmdfullName);
				break;
			case STMF_ERROR_EXISTS:
				(void) fprintf(stderr, "%s: %s: %s\n", cmdName,
				    operands[i], gettext("already exists"));
				ret++;
				break;
			case STMF_ERROR_GROUP_NOT_FOUND:
				(void) fprintf(stderr, "%s: %ls: %s\n", cmdName,
				    groupNamePrint, gettext("not found"));				
				ret++;
				break;
			case STMF_ERROR_PERM:
				(void) fprintf(stderr, "%s: %s\n", cmdName,
				    gettext("permission denied"));
				break;
			case STMF_ERROR_BUSY:
				(void) fprintf(stderr, "%s: %s: %s\n", cmdName,
				    operands[i], gettext("resource busy"));
				ret++;
				break;
			case STMF_ERROR_SERVICE_NOT_FOUND:
				(void) fprintf(stderr, "%s: %s\n", cmdName,
				    gettext("STMF service not found"));
				ret++;
				break;
			case STMF_ERROR_SERVICE_ONLINE:
				(void) fprintf(stderr, "%s: %s\n", cmdName,
				    gettext("STMF service must be offline"));
				ret++;
				break;
			case STMF_ERROR_SERVICE_DATA_VERSION:
				(void) fprintf(stderr, "%s: %s\n", cmdName,
				    gettext("STMF service version incorrect"));
				ret++;
				break;
			case STMF_ERROR_TG_ONLINE:
				(void) fprintf(stderr, "%s: %s\n", cmdName,
				    gettext("STMF target must be offline"));
				ret++;
				break;
			default:
				(void) fprintf(stderr, "%s: %s: %s\n", cmdName,
				    operands[i], gettext("unknown error"));
				ret++;
				break;
		}
	}

	return (ret);
}

/*
 * parseDevid
 *
 * Converts char * input to a stmfDevid
 *
 * input - this should be in the following format with either a
 * wwn. iqn. or eui. representation.
 * A name string of the format:
 *	wwn.<WWN> (FC/SAS address)
 *	iqn.<iSCSI name> (iSCSI iqn)
 *	eui.<WWN> (iSCSI eui name)
 *
 * devid - pointer to stmfDevid structure allocated by the caller.
 *
 * Returns:
 *  0 on success
 *  non-zero on failure
 */
static int
parseDevid(char *input, stmfDevid *devid)
{
	wchar_t inputWc[MAX_DEVID_INPUT + 1] = {0};

	/* convert to wcs */
	(void) mbstowcs(inputWc, input, MAX_DEVID_INPUT);

	/*
	 * Check for known scsi name string formats
	 * If one is found, we're done
	 * If not, then it's a failure to parse
	 */
	if (checkScsiNameString(inputWc, devid) == 0) {
		return (0);
	}

	return (-1);
}

/*
 * checkScsiNameString
 *
 * Validates known SCSI name string formats and converts to stmfDevid
 * format
 *
 * input - input SCSI name string
 * devid - pointer to stmfDevid structure allocated by the caller
 *         on successful return, contains the devid based on input
 *
 * returns:
 *         0 on success
 *         -1 on failure
 */
static int
checkScsiNameString(wchar_t *input, stmfDevid *devid)
{
	char *mbString = NULL;
	int mbStringLen;
	int len;
	int i;

	/*
	 * Convert to multi-byte string
	 *
	 * This is used for either eui or naa formats
	 */
	mbString = calloc(1, (mbStringLen = wcstombs(mbString, input, 0)) + 1);
	if (mbString == NULL) {
		(void) fprintf(stderr, "%s: %s\n",
		    cmdName, "Insufficient memory\n");
		return (-1);
	}
	if (wcstombs(mbString, input, mbStringLen) == (size_t)-1) {
		return (-1);
	}

	/*
	 * check for iqn format
	 */
	if (strncmp(mbString, "iqn.", 4) == 0) {
		if ((len = strlen(mbString)) > (SNS_IQN_223)) {
			return (-1);
		}
		for (i = 0; i < len; i++) {
			mbString[i] = tolower(mbString[i]);
		}
		if (checkIscsiName(input + 4) != 0) {
			return (-1);
		}
	} else if (strncmp(mbString, "wwn.", 4) == 0) {
		if ((len = strlen(mbString + 4)) != SNS_WWN_16) {
			return (-1);
		} else if (checkHexUpper(mbString + 4) != 0) {
			return (-1);
		}
	} else if (strncmp(mbString, "eui.", 4) == 0) {
		if ((len = strlen(mbString + 4)) != SNS_EUI_16) {
			return (-1);
		} else if (checkHexUpper(mbString + 4) != 0) {
			return (-1);
		}
	} else {
		return (-1);
	}

	/*
	 * We have a validated name string.
	 * Go ahead and set the length and copy it.
	 */
	devid->identLength = strlen(mbString);
	bzero(devid->ident, STMF_IDENT_LENGTH);
	bcopy(mbString, devid->ident, devid->identLength);

	return (0);
}


/*
 * Checks whether the entire string is in hex and converts to upper
 */
static int
checkHexUpper(char *input)
{
	int i;

	for (i = 0; i < strlen(input); i++) {
		if (isxdigit(input[i])) {
			input[i] = toupper(input[i]);
			continue;
		}
		return (-1);
	}

	return (0);
}

/*
 * checkIscsiName
 *
 * Purpose: Basic string checking on name
 */
static int
checkIscsiName(wchar_t *input)
{
	int i;

	for (i = 0; input[i] != 0; i++) {
		if (!iswalnum(input[i]) && input[i] != '-' &&
		    input[i] != '.' && input[i] != ':') {
			return (-1);
		}
	}

	return (0);
}

/*
 * addViewFunc
 *
 * Adds a view entry to a logical unit
 *
 */
/*ARGSUSED*/
static int
addViewFunc(int operandLen, char *operands[], cmdOptions_t *options,
    void *args)
{
	stmfViewEntry viewEntry;
	stmf_add_proxy_view_t *proxy_view_entry;
	stmfGuid inGuid;
	unsigned int guid[sizeof (stmfGuid)];
	uint16_t inputLuNbr;
	int ret = 0;
	int stmfRet;
	int i;
	char sGuid[GUID_INPUT + 1];
	bool_t isCluster = false;

	bzero(&viewEntry, sizeof (viewEntry));
	/* init view entry structure */
	viewEntry.allHosts = B_TRUE;
	viewEntry.allTargets = B_TRUE;
	viewEntry.luNbrValid = B_FALSE;

	/* check input length */
	if (strlen(operands[0]) != GUID_INPUT) {
		(void) fprintf(stderr, "%s: %s: %s%d%s\n", cmdName, operands[0],
		    gettext("must be "), GUID_INPUT,
		    gettext(" hexadecimal digits"));
		return (1);
	}

	for (; options->optval; options++) {
		switch (options->optval) {
			/* logical unit number */
			case 'n':
				viewEntry.luNbrValid = B_TRUE;
				inputLuNbr = atoi(options->optarg);
				if (inputLuNbr > MAX_LU_NBR) {
					(void) fprintf(stderr, "%s: %d: %s\n",
					    cmdName, inputLuNbr,
					    gettext("Logical unit number"
					    " must be less than 16384"));
					return (1);
				}
				viewEntry.luNbr[0] = inputLuNbr >> 8;
				viewEntry.luNbr[1] = inputLuNbr & 0xff;
				break;
			/* host group */
			case 'h':
				viewEntry.allHosts = B_FALSE;
				bcopy(options->optarg, viewEntry.hostGroup,
				    strlen(options->optarg));
				break;
			/* target group */
			case 't':
				viewEntry.allTargets = B_FALSE;
				bcopy(options->optarg, viewEntry.targetGroup,
				    strlen(options->optarg));
				break;
			case 'c':
				isCluster = true;
				break;
			default:
				(void) fprintf(stderr, "%s: %c: %s\n",
				    cmdName, options->optval,
				    gettext("unknown option"));
				return (1);
		}
	}

	/* convert to lower case for scan */
	for (i = 0; i < 32; i++)
		sGuid[i] = tolower(operands[0][i]);
	sGuid[i] = 0;

	(void) sscanf(sGuid, "%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x",
	    &guid[0], &guid[1], &guid[2], &guid[3], &guid[4], &guid[5],
	    &guid[6], &guid[7], &guid[8], &guid[9], &guid[10], &guid[11],
	    &guid[12], &guid[13], &guid[14], &guid[15]);

	for (i = 0; i < sizeof (stmfGuid); i++) {
		inGuid.guid[i] = guid[i];
	}

	/* add the view entry */
	stmfRet = stmfAddViewEntry(&inGuid, &viewEntry);
	switch (stmfRet) {
		case STMF_STATUS_SUCCESS:
			proxy_view_entry = malloc(sizeof(stmf_add_proxy_view_t));
			bzero(proxy_view_entry, sizeof(stmf_add_proxy_view_t));
			proxy_view_entry->head.op_type = STMF_OP_ADD;
			proxy_view_entry->head.item_type = STMF_VIEW_OP;
			bcopy(&inGuid, &proxy_view_entry->lu_guid, sizeof(stmfGuid));
			bcopy(&viewEntry, &proxy_view_entry->view_entry, sizeof(stmfViewEntry));
			free(proxy_view_entry);

			if (true == isCluster)
				stmfadm_send_cmd(cmdfullName);
			break;
		case STMF_ERROR_EXISTS:
			(void) fprintf(stderr, "%s: %s: %s\n", cmdName,
			    operands[0], gettext("already exists"));
			ret++;
			break;
		case STMF_ERROR_BUSY:
			(void) fprintf(stderr, "%s: %s: %s\n", cmdName,
			    operands[0], gettext("resource busy"));
			ret++;
			break;
		case STMF_ERROR_SERVICE_NOT_FOUND:
			(void) fprintf(stderr, "%s: %s\n", cmdName,
			    gettext("STMF service not found"));
			ret++;
			break;
		case STMF_ERROR_PERM:
			(void) fprintf(stderr, "%s: %s\n", cmdName,
			    gettext("permission denied"));
			ret++;
			break;
		case STMF_ERROR_LUN_IN_USE:
			(void) fprintf(stderr, "%s: %s\n", cmdName,
			    gettext("LUN already in use"));
			ret++;
			break;
		case STMF_ERROR_VE_CONFLICT:
			(void) fprintf(stderr, "%s: %s\n", cmdName,
			    gettext("view entry exists"));
			ret++;
			break;
		case STMF_ERROR_CONFIG_NONE:
			(void) fprintf(stderr, "%s: %s\n", cmdName,
			    gettext("STMF service is not initialized"));
			ret++;
			break;
		case STMF_ERROR_SERVICE_DATA_VERSION:
			(void) fprintf(stderr, "%s: %s\n", cmdName,
			    gettext("STMF service version incorrect"));
			ret++;
			break;
		case STMF_ERROR_INVALID_HG:
			(void) fprintf(stderr, "%s: %s\n", cmdName,
			    gettext("invalid host group"));
			ret++;
			break;
		case STMF_ERROR_INVALID_TG:
			(void) fprintf(stderr, "%s: %s\n", cmdName,
			    gettext("invalid target group"));
			ret++;
			break;
		default:
			(void) fprintf(stderr, "%s: %s: %s\n", cmdName,
			    operands[0], gettext("unknown error"));
			ret++;
			break;
	}

	return (ret);
}

/*
 * createHostGroupFunc
 *
 * Create a host group
 *
 */
/*ARGSUSED*/
static int
createHostGroupFunc(int operandLen, char *operands[],
    cmdOptions_t *options, void *args)
{
	int ret = 0;
	int stmfRet;
	wchar_t groupNamePrint[sizeof (stmfGroupName)] = {0};
	stmfGroupName groupName = {0};
	stmf_proxy_device_t *proxy_device;
	
	bool_t isCluster = false;

	(void) strlcpy(groupName, operands[0], sizeof (groupName));
	(void) mbstowcs(groupNamePrint, (char *)groupName,
	    sizeof (stmfGroupName) - 1);

	for (; options->optval; options++) {
		switch (options->optval) {
			case 'c':
				isCluster = true;
				break;
			default:
				(void) fprintf(stderr, "%s: %c: %s\n",
				    cmdName, options->optval,
				    gettext("unknown option"));
				return (1);
		}
	}

	
	/* call create group */
	stmfRet = stmfCreateHostGroup(&groupName);
	switch (stmfRet) {
		case STMF_STATUS_SUCCESS:
			proxy_device = malloc(sizeof(stmf_proxy_device_t));
			proxy_device->head.item_type = STMF_HG_OP;
			proxy_device->head.op_type = STMF_OP_ADD;
			bcopy(&groupName, &proxy_device->name, sizeof(stmfGroupName));
			free(proxy_device);
			
			if (true == isCluster)
				stmfadm_send_cmd(cmdfullName);
			break;
		case STMF_ERROR_EXISTS:
			(void) fprintf(stderr, "%s: %s: %s\n", cmdName,
			    operands[0], gettext("already exists"));
			ret++;
			break;
		case STMF_ERROR_BUSY:
			(void) fprintf(stderr, "%s: %s: %s\n", cmdName,
			    operands[0], gettext("resource busy"));
			ret++;
			break;
		case STMF_ERROR_SERVICE_NOT_FOUND:
			(void) fprintf(stderr, "%s: %s\n", cmdName,
			    gettext("STMF service not found"));
			ret++;
			break;
		case STMF_ERROR_PERM:
			(void) fprintf(stderr, "%s: %s\n", cmdName,
			    gettext("permission denied"));
			ret++;
			break;
		case STMF_ERROR_SERVICE_DATA_VERSION:
			(void) fprintf(stderr, "%s: %s\n", cmdName,
			    gettext("STMF service version incorrect"));
			ret++;
			break;
		default:
			(void) fprintf(stderr, "%s: %s: %s\n", cmdName,
			    operands[0], gettext("unknown error"));
			ret++;
			break;
	}

	return (ret);
}

static void
stmfadm_no_receive(int recv)
{
	char fifo_name[512];
	long int current_id;
	current_id = getpid();
	
	bzero(fifo_name,512);
	sprintf(fifo_name, "/tmp/synckeyrebak%ld", current_id);
	unlink(fifo_name);
	printf("synckey fail\n");
	exit(EXIT_FAILURE);
}


/*
*
*stmfadm_host_send_cmd
*
*update cmd to send 
*
*/
#if 0
static int stmfadm_send_cmd(char * cmd_name)
{
	int ret = 0;
	int size = 0;
	char buffer[512];
	libzfs_handle_t *zfs_handle = NULL;
	long int current_id = 0;
	
	int fifo_fd = -1;
	int chkfifo = 0;
	char fifo_name[512];
	char fifo_buffer[16];

	char tmp_p = NULL;
	
	signal(SIGALRM,stmfadm_no_receive);
	alarm(5);
		
	current_id = getpid();
	bzero(fifo_name, 512);
	sprintf(fifo_name, "/tmp/synckeyrebak%d", current_id);
	chkfifo = mkfifo(fifo_name, 0700);
	if (chkfifo != 0){
		printf("could not create FIFO\n");
		return (-1);
	}

		
	bzero(buffer, 512);
	sprintf(buffer,"%ld",current_id);
	bcopy(cmd_name, buffer + MAX_ID_BYTE, strlen(cmd_name));
	buffer[strlen(cmd_name) + MAX_ID_BYTE] = 0;
	/*size = strlen(buffer+MAX_ID_BYTE) + MAX_ID_BYTE;*/


	if(cmd_name[0] == '\"')
		cmd_name[0] = ' ';
	if(cmd_name[strlen(cmd_name) - 1] == '\"')
		cmd_name[strlen(cmd_name) - 1] = 0;

	size = strlen(buffer) + 1;
	if ((zfs_handle = libzfs_init()) == NULL) {
		printf("key file get zfs handle failed\n");
		return (-1);
	}

	/* Calculate the length of the string */
	size = strlen(buffer + MAX_ID_BYTE) + MAX_ID_BYTE + 1;
	if ((zfs_handle = libzfs_init()) == NULL) {
		printf("key file get zfs handle failed\n");
		return (-1);
	}
	
	/*send rcmd*/
	zfs_do_hbx_process(zfs_handle, buffer, size, ZFS_HBX_RCMD_UPDATE);
	libzfs_fini(zfs_handle);

	
	fifo_fd = open(fifo_name,O_RDONLY);
	if (fifo_fd != -1){
		read(fifo_fd, fifo_buffer, sizeof(fifo_buffer));
		if (strcmp(fifo_buffer, "0") != 0){
			printf("opposite terminal execute fail\n");
		}
		close(fifo_fd);
	} else {
		printf("open FIFO fail\n");
	}
	unlink(fifo_name);
	
	return ret;
}
#endif
static int stmfadm_send_cmd(char *cmd_name)
{
	libzfs_handle_t *zfs_handle = NULL;
	long int current_id = 0;
	nvlist_t *result;
	nvpair_t *nvp_elem;
	char *hostnameid;
	int ret;
	int have_others = 0;
	
	current_id = getpid();

	if(cmd_name[0] == '\"') {
		cmd_name[0] = ' ';
	}
	if(cmd_name[strlen(cmd_name) - 1] == '\"') {
		cmd_name[strlen(cmd_name) - 1] = 0;
	}
	
	if ((zfs_handle = libzfs_init()) == NULL) {
		printf("get zfs handle failed\n");
		return (-1);
	}
	result = zfs_clustersan_sync_cmd(zfs_handle, current_id,
		cmd_name, 10, -1);
	libzfs_fini(zfs_handle);

	if (result == NULL) {
		return (-1);
	}

	nvp_elem = NULL;
	printf("execution results from other hosts\n");
	while ((nvp_elem = nvlist_next_nvpair(result, nvp_elem)) != NULL) {
		have_others = 1;
		hostnameid = nvpair_name(nvp_elem);
		verify(0 == nvpair_value_int32(nvp_elem, &ret));
		printf("  %s: ", hostnameid);
		if (ret == 0) {
			printf("successs\n");
		} else {
			printf("fail\n");
		}
	}

	if (have_others == 0) {
		printf("  no other hosts\n");
	}
	nvlist_free(result);

	return (0);
}

/*
 * createLuFunc
 *
 * Create a logical unit
 *
 */
/*ARGSUSED*/
static int
createLuFunc(int operandLen, char *operands[], cmdOptions_t *options,
    void *args)
{
	luResource hdl = NULL;
	int ret = 0;
	int stmfRet = 0;
	char guidAsciiBuf[33];
	stmfGuid createdGuid;

	stmfRet = stmfCreateLuResource(STMF_DISK, &hdl);

	if (stmfRet != STMF_STATUS_SUCCESS) {
		(void) fprintf(stderr, "%s: %s\n",
		    cmdName, gettext("Failure to create lu resource\n"));
		return (1);
	}

	for (; options->optval; options++) {
		switch (options->optval) {
			case 'p':
				ret = setLuPropFromInput(hdl, options->optarg);
				if (ret != 0) {
					(void) stmfFreeLuResource(hdl);
					return (1);
				}
				break;
			case 's':
				stmfRet = stmfSetLuProp(hdl, STMF_LU_PROP_SIZE,
				    options->optarg);
				if (stmfRet != STMF_STATUS_SUCCESS) {
					(void) fprintf(stderr, "%s: %c: %s\n",
					    cmdName, options->optval,
					    gettext("size param invalid"));
					(void) stmfFreeLuResource(hdl);
					return (1);
				}
				break;
			default:
				(void) fprintf(stderr, "%s: %c: %s\n",
				    cmdName, options->optval,
				    gettext("unknown option"));
				return (1);
		}
	}

	stmfRet = stmfSetLuProp(hdl, STMF_LU_PROP_FILENAME, operands[0]);

	if (stmfRet != STMF_STATUS_SUCCESS) {
		(void) fprintf(stderr, "%s: %s\n",
		    cmdName, gettext("could not set filename"));
		return (1);
	}

	stmfRet = stmfCreateLu(hdl, &createdGuid);
	switch (stmfRet) {
		case STMF_STATUS_SUCCESS:
			break;
		case STMF_ERROR_BUSY:
		case STMF_ERROR_LU_BUSY:
			(void) fprintf(stderr, "%s: %s\n", cmdName,
			    gettext("resource busy"));
			ret++;
			break;
		case STMF_ERROR_PERM:
			(void) fprintf(stderr, "%s: %s\n", cmdName,
			    gettext("permission denied"));
			ret++;
			break;
		case STMF_ERROR_FILE_IN_USE:
			(void) fprintf(stderr, "%s: filename %s: %s\n", cmdName,
			    operands[0], gettext("in use"));
			ret++;
			break;
		case STMF_ERROR_INVALID_BLKSIZE:
			(void) fprintf(stderr, "%s: %s\n", cmdName,
			    gettext("invalid block size"));
			ret++;
			break;
		case STMF_ERROR_GUID_IN_USE:
			(void) fprintf(stderr, "%s: %s\n", cmdName,
			    gettext("guid in use"));
			ret++;
			break;
		case STMF_ERROR_META_FILE_NAME:
			(void) fprintf(stderr, "%s: %s\n", cmdName,
			    gettext("meta file error"));
			ret++;
			break;
		case STMF_ERROR_DATA_FILE_NAME:
			(void) fprintf(stderr, "%s: %s\n", cmdName,
			    gettext("data file error"));
			ret++;
			break;
		case STMF_ERROR_FILE_SIZE_INVALID:
			(void) fprintf(stderr, "%s: %s\n", cmdName,
			    gettext("file size invalid"));
			ret++;
			break;
		case STMF_ERROR_SIZE_OUT_OF_RANGE:
			(void) fprintf(stderr, "%s: %s\n", cmdName,
			    gettext("invalid size"));
			ret++;
			break;
		case STMF_ERROR_META_CREATION:
			(void) fprintf(stderr, "%s: %s\n", cmdName,
			    gettext("could not create meta file"));
			ret++;
			break;
		case STMF_ERROR_WRITE_CACHE_SET:
			(void) fprintf(stderr, "%s: %s\n", cmdName,
			    gettext("could not set write cache"));
			ret++;
			break;
		default:
			(void) fprintf(stderr, "%s: %s\n", cmdName,
			    gettext("unknown error"));
			ret++;
			break;
	}

	if (ret != 0) {
		goto done;
	}

	(void) snprintf(guidAsciiBuf, sizeof (guidAsciiBuf),
	    "%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X"
	    "%02X%02X%02X%02X%02X%02X",
	    createdGuid.guid[0], createdGuid.guid[1], createdGuid.guid[2],
	    createdGuid.guid[3], createdGuid.guid[4], createdGuid.guid[5],
	    createdGuid.guid[6], createdGuid.guid[7], createdGuid.guid[8],
	    createdGuid.guid[9], createdGuid.guid[10], createdGuid.guid[11],
	    createdGuid.guid[12], createdGuid.guid[13], createdGuid.guid[14],
	    createdGuid.guid[15]);
	(void) printf("Logical unit created: %s\n", guidAsciiBuf);

done:
	(void) stmfFreeLuResource(hdl);
	return (ret);
}

/*
 * createLuFunc
 *
 * Create a logical unit
 *
 */
/*ARGSUSED*/
static int
modifyLuFunc(int operandLen, char *operands[], cmdOptions_t *options,
    void *args)
{
	stmfGuid inGuid;
	unsigned int guid[sizeof (stmfGuid)];
	int ret = 0;
	int i;
	char *fname = NULL;
	char *lasts = NULL;
	char sGuid[GUID_INPUT + 1];
	char *prop = NULL;
	char *propVal = NULL;
	boolean_t fnameUsed = B_FALSE;
	uint32_t propId;
	cmdOptions_t *optionStart = options;


	for (; options->optval; options++) {
		switch (options->optval) {
			case 'f':
				fnameUsed = B_TRUE;
				fname = operands[0];
				break;
		}
	}
	options = optionStart;

	/* check input length */
	if (!fnameUsed && strlen(operands[0]) != GUID_INPUT) {
		(void) fprintf(stderr, "%s: %s: %s%d%s\n", cmdName, operands[0],
		    gettext("must be "), GUID_INPUT,
		    gettext(" hexadecimal digits"));
		return (1);
	}

	if (!fnameUsed) {
		/* convert to lower case for scan */
		for (i = 0; i < 32; i++)
			sGuid[i] = tolower(operands[0][i]);
		sGuid[i] = 0;
		(void) sscanf(sGuid,
		    "%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x",
		    &guid[0], &guid[1], &guid[2], &guid[3], &guid[4], &guid[5],
		    &guid[6], &guid[7], &guid[8], &guid[9], &guid[10],
		    &guid[11], &guid[12], &guid[13], &guid[14], &guid[15]);

		for (i = 0; i < sizeof (stmfGuid); i++) {
			inGuid.guid[i] = guid[i];
		}
	}

	for (; options->optval; options++) {
		switch (options->optval) {
			case 'p':
				prop = strtok_r(options->optarg, "=", &lasts);
				propVal = strtok_r(NULL, "=", &lasts);
				ret = convertCharToPropId(prop, &propId);
				if (ret != 0) {
					(void) fprintf(stderr, "%s: %s: %s\n",
					    cmdName,
					gettext("invalid property specified"),
					    prop);
					return (1);
				}
				if (propVal ==  NULL &&
				    propId != STMF_LU_PROP_MGMT_URL) {
					(void) fprintf(stderr, "%s: %s: %s\n",
					    cmdName, options->optarg,
					    gettext("invalid property specifier"
					    "- prop=val\n"));
					return (1);
				}
				if (propVal ==  NULL) {
					ret = callModify(fname, &inGuid, propId,
					    "", prop);
				} else {
					ret = callModify(fname, &inGuid, propId,
					    propVal, prop);
				}
				if (ret != 0) {
					return (1);
				}
				break;
			case 's':
				if (callModify(fname, &inGuid,
				    STMF_LU_PROP_SIZE, options->optarg,
				    "size") != 0) {
					return (1);
				}
				break;
			case 'f':
				break;
			default:
				(void) fprintf(stderr, "%s: %c: %s\n",
				    cmdName, options->optval,
				    gettext("unknown option"));
				return (1);
		}
	}
	return (ret);
}

static int
callModify(char *fname, stmfGuid *luGuid, uint32_t prop, const char *propVal,
    const char *propString)
{
	int ret = 0;
	int stmfRet = 0;

	if (!fname) {
		stmfRet = stmfModifyLu(luGuid, prop, propVal);
	} else {
		stmfRet = stmfModifyLuByFname(STMF_DISK, fname, prop,
		    propVal);
	}
	switch (stmfRet) {
		case STMF_STATUS_SUCCESS:
			break;
		case STMF_ERROR_BUSY:
		case STMF_ERROR_LU_BUSY:
			(void) fprintf(stderr, "%s: %s\n", cmdName,
			    gettext("resource busy"));
			ret++;
			break;
		case STMF_ERROR_PERM:
			(void) fprintf(stderr, "%s: %s\n", cmdName,
			    gettext("permission denied"));
			ret++;
			break;
		case STMF_ERROR_INVALID_BLKSIZE:
			(void) fprintf(stderr, "%s: %s\n", cmdName,
			    gettext("invalid block size"));
			ret++;
			break;
		case STMF_ERROR_GUID_IN_USE:
			(void) fprintf(stderr, "%s: %s\n", cmdName,
			    gettext("guid in use"));
			ret++;
			break;
		case STMF_ERROR_META_FILE_NAME:
			(void) fprintf(stderr, "%s: %s\n", cmdName,
			    gettext("meta file error"));
			ret++;
			break;
		case STMF_ERROR_DATA_FILE_NAME:
			(void) fprintf(stderr, "%s: %s\n", cmdName,
			    gettext("data file error"));
			ret++;
			break;
		case STMF_ERROR_FILE_SIZE_INVALID:
			(void) fprintf(stderr, "%s: %s\n", cmdName,
			    gettext("file size invalid"));
			ret++;
			break;
		case STMF_ERROR_SIZE_OUT_OF_RANGE:
			(void) fprintf(stderr, "%s: %s\n", cmdName,
			    gettext("invalid size"));
			ret++;
			break;
		case STMF_ERROR_META_CREATION:
			(void) fprintf(stderr, "%s: %s\n", cmdName,
			    gettext("could not create meta file"));
			ret++;
			break;
		case STMF_ERROR_INVALID_PROP:
			(void) fprintf(stderr, "%s: %s\n", cmdName,
			    gettext("invalid property for modify"));
			ret++;
			break;
		case STMF_ERROR_WRITE_CACHE_SET:
			(void) fprintf(stderr, "%s: %s\n", cmdName,
			    gettext("could not set write cache"));
			ret++;
			break;
		case STMF_ERROR_ACCESS_STATE_SET:
			(void) fprintf(stderr, "%s: %s\n", cmdName,
			    gettext("cannot modify while in standby mode"));
			ret++;
			break;
		default:
			(void) fprintf(stderr, "%s: %s: %s: %d\n", cmdName,
			    gettext("could not set property"), propString,
			    stmfRet);
			ret++;
			break;
	}

	return (ret);
}


/*
 * importLuFunc
 *
 * Create a logical unit
 *
 */
/*ARGSUSED*/
static int
importLuFunc(int operandLen, char *operands[], cmdOptions_t *options,
    void *args)
{
	int stmfRet = 0;
	int ret = 0;
	char guidAsciiBuf[33];
	stmfGuid createdGuid;

	stmfRet = stmfImportLu(STMF_DISK, operands[0], &createdGuid);
	switch (stmfRet) {
		case STMF_STATUS_SUCCESS:
			break;
		case STMF_ERROR_BUSY:
		case STMF_ERROR_LU_BUSY:
			(void) fprintf(stderr, "%s: %s\n", cmdName,
			    gettext("resource busy"));
			ret++;
			break;
		case STMF_ERROR_PERM:
			(void) fprintf(stderr, "%s: %s\n", cmdName,
			    gettext("permission denied"));
			ret++;
			break;
		case STMF_ERROR_FILE_IN_USE:
			(void) fprintf(stderr, "%s: filename %s: %s\n", cmdName,
			    operands[0], gettext("in use"));
			ret++;
			break;
		case STMF_ERROR_GUID_IN_USE:
			(void) fprintf(stderr, "%s: %s\n", cmdName,
			    gettext("guid in use"));
			ret++;
			break;
		case STMF_ERROR_META_FILE_NAME:
			(void) fprintf(stderr, "%s: %s\n", cmdName,
			    gettext("meta file error"));
			ret++;
			break;
		case STMF_ERROR_DATA_FILE_NAME:
			(void) fprintf(stderr, "%s: %s\n", cmdName,
			    gettext("data file error"));
			ret++;
			break;
		case STMF_ERROR_META_CREATION:
			(void) fprintf(stderr, "%s: %s\n", cmdName,
			    gettext("could not create meta file"));
			ret++;
			break;
		case STMF_ERROR_WRITE_CACHE_SET:
			(void) fprintf(stderr, "%s: %s\n", cmdName,
			    gettext("could not set write cache"));
			ret++;
			break;
		default:
			(void) fprintf(stderr, "%s: %s\n", cmdName,
			    gettext("unknown error"));
			ret++;
			break;
	}

	if (ret != STMF_STATUS_SUCCESS) {
		goto done;
	}

	(void) snprintf(guidAsciiBuf, sizeof (guidAsciiBuf),
	    "%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X"
	    "%02X%02X%02X%02X%02X%02X",
	    createdGuid.guid[0], createdGuid.guid[1], createdGuid.guid[2],
	    createdGuid.guid[3], createdGuid.guid[4], createdGuid.guid[5],
	    createdGuid.guid[6], createdGuid.guid[7], createdGuid.guid[8],
	    createdGuid.guid[9], createdGuid.guid[10], createdGuid.guid[11],
	    createdGuid.guid[12], createdGuid.guid[13], createdGuid.guid[14],
	    createdGuid.guid[15]);
	(void) printf("Logical unit imported: %s\n", guidAsciiBuf);

done:
	return (ret);
}

static int
setLuPropFromInput(luResource hdl, char *optarg)
{
	char *prop = NULL;
	char *propVal = NULL;
	char *lasts = NULL;
	uint32_t propId;
	int ret = 0;

	prop = strtok_r(optarg, "=", &lasts);
	if ((propVal = strtok_r(NULL, "=", &lasts)) == NULL) {
		(void) fprintf(stderr, "%s: %s: %s\n",
		    cmdName, optarg,
		    gettext("invalid property specifier - prop=val\n"));
		return (1);
	}

	ret = convertCharToPropId(prop, &propId);
	if (ret != 0) {
		(void) fprintf(stderr, "%s: %s: %s\n",
		    cmdName, gettext("invalid property specified"), prop);
		return (1);
	}

	ret = stmfSetLuProp(hdl, propId, propVal);
	if (ret != STMF_STATUS_SUCCESS) {
		(void) fprintf(stderr, "%s: %s %s: ",
		    cmdName, gettext("unable to set"), prop);
		switch (ret) {
			case STMF_ERROR_INVALID_PROPSIZE:
				(void) fprintf(stderr, "invalid length\n");
				break;
			case STMF_ERROR_INVALID_ARG:
				(void) fprintf(stderr, "bad format\n");
				break;
			default:
				(void) fprintf(stderr, "\n");
				break;
		}
		return (1);
	}

	return (0);
}

static int
convertCharToPropId(char *prop, uint32_t *propId)
{
	if (strcasecmp(prop, GUID) == 0) {
		*propId = STMF_LU_PROP_GUID;
	} else if (strcasecmp(prop, ALIAS) == 0) {
		*propId = STMF_LU_PROP_ALIAS;
	} else if (strcasecmp(prop, VID) == 0) {
		*propId = STMF_LU_PROP_VID;
	} else if (strcasecmp(prop, PID) == 0) {
		*propId = STMF_LU_PROP_PID;
	} else if (strcasecmp(prop, WRITE_PROTECT) == 0) {
		*propId = STMF_LU_PROP_WRITE_PROTECT;
	} else if (strcasecmp(prop, WRITEBACK_CACHE_DISABLE) == 0) {
		*propId = STMF_LU_PROP_WRITE_CACHE_DISABLE;
	} else if (strcasecmp(prop, BLOCK_SIZE) == 0) {
		*propId = STMF_LU_PROP_BLOCK_SIZE;
	} else if (strcasecmp(prop, SERIAL_NUMBER) == 0) {
		*propId = STMF_LU_PROP_SERIAL_NUM;
	} else if (strcasecmp(prop, COMPANY_ID) == 0) {
		*propId = STMF_LU_PROP_COMPANY_ID;
	} else if (strcasecmp(prop, META_FILE) == 0) {
		*propId = STMF_LU_PROP_META_FILENAME;
	} else if (strcasecmp(prop, MGMT_URL) == 0) {
		*propId = STMF_LU_PROP_MGMT_URL;
	} else if (strcasecmp(prop, HOST_ID) == 0) {
		*propId = STMF_LU_PROP_HOST_ID;
	} else {
		return (1);
	}
	return (0);
}

/*
 * deleteLuFunc
 *
 * Delete a logical unit
 *
 */
/*ARGSUSED*/
static int
deleteLuFunc(int operandLen, char *operands[], cmdOptions_t *options,
    void *args)
{
	int i, j;
	int ret = 0;
	int stmfRet;
	unsigned int inGuid[sizeof (stmfGuid)];
	stmfGuid delGuid;
	boolean_t keepViews = B_FALSE;
	boolean_t viewEntriesRemoved = B_FALSE;
	boolean_t noLunFound = B_FALSE;
	boolean_t views = B_FALSE;
	boolean_t isCluster = B_FALSE;
	char sGuid[GUID_INPUT + 1];
	stmfViewEntryList *viewEntryList = NULL;

	for (; options->optval; options++) {
		switch (options->optval) {
			/* Keep views for logical unit */
			case 'k':
				keepViews = B_TRUE;
				break;
			case 'c':
				isCluster = B_TRUE;
				break;				
			default:
				(void) fprintf(stderr, "%s: %c: %s\n",
				    cmdName, options->optval,
				    gettext("unknown option"));
				return (1);
		}
	}


	for (i = 0; i < operandLen; i++) {
		for (j = 0; j < GUID_INPUT; j++) {
			if (!isxdigit(operands[i][j])) {
				break;
			}
			sGuid[j] = tolower(operands[i][j]);
		}
		if (j != GUID_INPUT) {
			(void) fprintf(stderr, "%s: %s: %s%d%s\n",
			    cmdName, operands[i], gettext("must be "),
			    GUID_INPUT,
			    gettext(" hexadecimal digits long"));
			continue;
		}

		sGuid[j] = 0;

		(void) sscanf(sGuid,
		    "%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x",
		    &inGuid[0], &inGuid[1], &inGuid[2], &inGuid[3],
		    &inGuid[4], &inGuid[5], &inGuid[6], &inGuid[7],
		    &inGuid[8], &inGuid[9], &inGuid[10], &inGuid[11],
		    &inGuid[12], &inGuid[13], &inGuid[14], &inGuid[15]);

		for (j = 0; j < sizeof (stmfGuid); j++) {
			delGuid.guid[j] = inGuid[j];
		}

		stmfRet = stmfDeleteLu(&delGuid);
		switch (stmfRet) {
			case STMF_STATUS_SUCCESS:
				if (B_TRUE == isCluster)
					stmfadm_send_cmd(cmdfullName);				
				break;
			case STMF_ERROR_NOT_FOUND:
				noLunFound = B_TRUE;
				break;
			case STMF_ERROR_BUSY:
				(void) fprintf(stderr, "%s: %s\n", cmdName,
				    gettext("resource busy"));
				ret++;
				break;
			case STMF_ERROR_PERM:
				(void) fprintf(stderr, "%s: %s\n", cmdName,
				    gettext("permission denied"));
				ret++;
				break;
			default:
				(void) fprintf(stderr, "%s: %s\n", cmdName,
				    gettext("unknown error"));
				ret++;
				break;
		}

		if (ret)
			return (ret);

		if (!keepViews) {
			stmfRet = stmfGetViewEntryList(&delGuid,
			    &viewEntryList);
			if (stmfRet == STMF_STATUS_SUCCESS) {
				for (j = 0; j < viewEntryList->cnt; j++) {
					(void) stmfRemoveViewEntry(&delGuid,
					    viewEntryList->ve[j].veIndex);
				}
				viewEntriesRemoved = B_TRUE;
				stmfFreeMemory(viewEntryList);
			} else if (stmfRet != STMF_ERROR_NOT_FOUND) {
				(void) fprintf(stderr, "%s: %s\n", cmdName,
				    gettext("unable to remove view entries\n"));
				ret++;
			} /* No view entries to remove */
		}
		if (keepViews) {
			stmfRet = stmfGetViewEntryList(&delGuid,
			    &viewEntryList);
			if (stmfRet == STMF_STATUS_SUCCESS) {
				views = B_TRUE;
				stmfFreeMemory(viewEntryList);
			}
		}

		if ((!viewEntriesRemoved && noLunFound && !views) ||
		    (!views && keepViews && noLunFound)) {
			(void) fprintf(stderr, "%s: %s: %s\n",
			    cmdName, sGuid,
			    gettext("not found"));
			ret++;
		}
		noLunFound = viewEntriesRemoved = views = B_FALSE;
	}
	return (ret);
}


/*
 * createTargetGroupFunc
 *
 * Create a target group
 *
 */
/*ARGSUSED*/
static int
createTargetGroupFunc(int operandLen, char *operands[], cmdOptions_t *options,
    void *args)
{
	int ret = 0;
	int stmfRet;
	wchar_t groupNamePrint[sizeof (stmfGroupName)] = {0};
	stmfGroupName groupName = {0};
	stmf_proxy_device_t *proxy_device;

	bool_t isCluster = false;

	(void) strlcpy(groupName, operands[0], sizeof (groupName));
	(void) mbstowcs(groupNamePrint, (char *)groupName,
	    sizeof (stmfGroupName) - 1);

	for (; options->optval; options++) {
		switch (options->optval) {
			case 'c':
				isCluster = true;
				break;
			default:
				(void) fprintf(stderr, "%s: %c: %s\n",
				    cmdName, options->optval,
				    gettext("unknown option"));
				return (1);
		}
	}
	
	/* call create group */
	stmfRet = stmfCreateTargetGroup(&groupName);
	switch (stmfRet) {
		case STMF_STATUS_SUCCESS:
			proxy_device = malloc(sizeof(stmf_proxy_device_t));
			proxy_device->head.item_type = STMF_TG_OP;
			proxy_device->head.op_type = STMF_OP_ADD;
			bcopy(&groupName, &proxy_device->name, sizeof(stmfGroupName));
			free(proxy_device);

			if (true == isCluster)
				stmfadm_send_cmd(cmdfullName);
			break;
		case STMF_ERROR_EXISTS:
			(void) fprintf(stderr, "%s: %ls: %s\n", cmdName,
			    groupNamePrint, gettext("already exists"));
			ret++;
			break;
		case STMF_ERROR_BUSY:
			(void) fprintf(stderr, "%s: %ls: %s\n", cmdName,
			    groupNamePrint, gettext("resource busy"));
			ret++;
			break;
		case STMF_ERROR_PERM:
			(void) fprintf(stderr, "%s: %s\n", cmdName,
			    gettext("permission denied"));
			ret++;
			break;
		case STMF_ERROR_SERVICE_NOT_FOUND:
			(void) fprintf(stderr, "%s: %s\n", cmdName,
			    gettext("STMF service not found"));
			ret++;
			break;
		case STMF_ERROR_SERVICE_DATA_VERSION:
			(void) fprintf(stderr, "%s: %s\n", cmdName,
			    gettext("STMF service version incorrect"));
			ret++;
			break;
		default:
			(void) fprintf(stderr, "%s: %ls: %s\n", cmdName,
			    groupNamePrint, gettext("unknown error"));
			ret++;
			break;
	}

	return (ret);
}

/*
 * deleteHostGroupFunc
 *
 * Delete a host group
 *
 */
/*ARGSUSED*/
static int
deleteHostGroupFunc(int operandLen, char *operands[],
    cmdOptions_t *options, void *args)
{
	int ret = 0;
	int stmfRet;
	wchar_t groupNamePrint[sizeof (stmfGroupName)] = {0};
	stmfGroupName groupName = {0};
	stmf_proxy_device_t *proxy_device;
	
	bool_t isCluster = false;
	
	(void) strlcpy(groupName, operands[0], sizeof (groupName));
	(void) mbstowcs(groupNamePrint, (char *)groupName,
	    sizeof (stmfGroupName) - 1);

	for (; options->optval; options++) {
		switch (options->optval) {
			case 'c':
				isCluster = true;
				break;
			default:
				(void) fprintf(stderr, "%s: %c: %s\n",
				    cmdName, options->optval,
				    gettext("unknown option"));
				return (1);
		}
	}

	
	/* call delete group */
	stmfRet = stmfDeleteHostGroup(&groupName);
	switch (stmfRet) {
		case STMF_STATUS_SUCCESS:
			proxy_device = malloc(sizeof(stmf_proxy_device_t));
			proxy_device->head.item_type = STMF_HG_OP;
			proxy_device->head.op_type = STMF_OP_DELETE;
			bcopy(&groupName, &proxy_device->name, sizeof(stmfGroupName));
			free(proxy_device);

			if (true == isCluster)
				stmfadm_send_cmd(cmdfullName);
			break;
		case STMF_ERROR_NOT_FOUND:
			(void) fprintf(stderr, "%s: %ls: %s\n", cmdName,
			    groupNamePrint, gettext("not found"));
			ret++;
			break;
		case STMF_ERROR_BUSY:
			(void) fprintf(stderr, "%s: %ls: %s\n", cmdName,
			    groupNamePrint, gettext("resource busy"));
			ret++;
			break;
		case STMF_ERROR_SERVICE_NOT_FOUND:
			(void) fprintf(stderr, "%s: %s\n", cmdName,
			    gettext("STMF service not found"));
			ret++;
			break;
		case STMF_ERROR_PERM:
			(void) fprintf(stderr, "%s: %s\n", cmdName,
			    gettext("permission denied"));
			ret++;
			break;
		case STMF_ERROR_GROUP_IN_USE:
			(void) fprintf(stderr, "%s: %ls: %s\n", cmdName,
			    groupNamePrint,
			    gettext("group is in use by existing view entry"));
			ret++;
			break;
		case STMF_ERROR_SERVICE_DATA_VERSION:
			(void) fprintf(stderr, "%s: %s\n", cmdName,
			    gettext("STMF service version incorrect"));
			ret++;
			break;
		default:
			(void) fprintf(stderr, "%s: %ls: %s\n", cmdName,
			    groupNamePrint, gettext("unknown error"));
			ret++;
			break;
	}

	return (ret);
}

/*
 * deleteTargetGroupFunc
 *
 * Delete a target group
 *
 */
/*ARGSUSED*/
static int
deleteTargetGroupFunc(int operandLen, char *operands[], cmdOptions_t *options,
    void *args)
{
	int ret = 0;
	int stmfRet;
	wchar_t groupNamePrint[sizeof (stmfGroupName)] = {0};
	stmfGroupName groupName = {0};
	stmf_proxy_device_t *proxy_device;
	
	bool_t isCluster = false;
		
	(void) strlcpy(groupName, operands[0], sizeof (groupName));
	(void) mbstowcs(groupNamePrint, (char *)groupName,
	    sizeof (stmfGroupName) - 1);

	for (; options->optval; options++) {
		switch (options->optval) {
			case 'c':
				isCluster = true;
				break;
			default:
				(void) fprintf(stderr, "%s: %c: %s\n",
				    cmdName, options->optval,
				    gettext("unknown option"));
				return (1);
		}
	}

	
	/* call delete group */
	stmfRet = stmfDeleteTargetGroup(&groupName);
	switch (stmfRet) {
		case STMF_STATUS_SUCCESS:
			proxy_device = malloc(sizeof(stmf_proxy_device_t));
			proxy_device->head.item_type = STMF_TG_OP;
			proxy_device->head.op_type = STMF_OP_DELETE;
			bcopy(&groupName, &proxy_device->name, sizeof(stmfGroupName));
			free(proxy_device);

			if (true == isCluster)
				stmfadm_send_cmd(cmdfullName);
			break;
		case STMF_ERROR_NOT_FOUND:
			(void) fprintf(stderr, "%s: %ls: %s\n", cmdName,
			    groupNamePrint, gettext("not found"));
			ret++;
			break;
		case STMF_ERROR_BUSY:
			(void) fprintf(stderr, "%s: %ls: %s\n", cmdName,
			    groupNamePrint, gettext("resource busy"));
			ret++;
			break;
		case STMF_ERROR_SERVICE_NOT_FOUND:
			(void) fprintf(stderr, "%s: %s\n", cmdName,
			    gettext("STMF service not found"));
			ret++;
			break;
		case STMF_ERROR_PERM:
			(void) fprintf(stderr, "%s: %s\n", cmdName,
			    gettext("permission denied"));
			ret++;
			break;
		case STMF_ERROR_GROUP_IN_USE:
			(void) fprintf(stderr, "%s: %ls: %s\n", cmdName,
			    groupNamePrint,
			    gettext("group is in use by existing view entry"));
			ret++;
			break;
		case STMF_ERROR_SERVICE_DATA_VERSION:
			(void) fprintf(stderr, "%s: %s\n", cmdName,
			    gettext("STMF service version incorrect"));
			ret++;
			break;
		default:
			(void) fprintf(stderr, "%s: %ls: %s\n", cmdName,
			    groupNamePrint, gettext("unknown error"));
			ret++;
			break;
	}

	return (ret);
}

/*
 * listHostGroupFunc
 *
 * Lists the specified host groups or all if none are specified
 *
 */
/*ARGSUSED*/
static int
listHostGroupFunc(int operandLen, char *operands[], cmdOptions_t *options,
    void *args)
{
	int ret = 0;
	int stmfRet;
	int i, j, outerLoop;
	boolean_t verbose = B_FALSE;
	boolean_t found = B_TRUE;
	boolean_t operandEntered;
	stmfGroupList *groupList;
	stmfGroupProperties *groupProps;
	wchar_t operandName[sizeof (stmfGroupName)];
	wchar_t groupNamePrint[sizeof (stmfGroupName)];

	for (; options->optval; options++) {
		switch (options->optval) {
			case 'v':
				verbose = B_TRUE;
				break;
			default:
				(void) fprintf(stderr, "%s: %c: %s\n",
				    cmdName, options->optval,
				    gettext("unknown option"));
				return (1);
		}
	}

	if (operandLen > 0) {
		outerLoop = operandLen;
		operandEntered = B_TRUE;
	} else {
		outerLoop = 1;
		operandEntered = B_FALSE;
	}

	stmfRet = stmfGetHostGroupList(&groupList);
	if (stmfRet != STMF_STATUS_SUCCESS) {
		switch (stmfRet) {
			case STMF_ERROR_BUSY:
				(void) fprintf(stderr, "%s: %s\n", cmdName,
				    gettext("resource busy"));
				break;
			case STMF_ERROR_SERVICE_NOT_FOUND:
				(void) fprintf(stderr, "%s: %s\n", cmdName,
				    gettext("STMF service not found"));
				break;
			case STMF_ERROR_PERM:
				(void) fprintf(stderr, "%s: %s\n", cmdName,
				    gettext("permission denied"));
				break;
			case STMF_ERROR_SERVICE_DATA_VERSION:
				(void) fprintf(stderr, "%s: %s\n", cmdName,
				    gettext("STMF service version incorrect"));
				break;
			default:
				(void) fprintf(stderr, "%s: %s\n", cmdName,
				    gettext("unknown error"));
				break;
		}
		return (1);
	}

	for (i = 0; i < outerLoop; i++) {
		for (found = B_FALSE, j = 0; j < groupList->cnt; j++) {
			(void) mbstowcs(groupNamePrint,
			    (char *)groupList->name[j],
			    sizeof (stmfGroupName) - 1);
			groupNamePrint[sizeof (stmfGroupName) - 1] = 0;
			if (operandEntered) {
				(void) mbstowcs(operandName, operands[i],
				    sizeof (stmfGroupName) - 1);
				operandName[sizeof (stmfGroupName) - 1] = 0;
				if (wcscmp(operandName, groupNamePrint)
				    == 0) {
					found = B_TRUE;
				}
			}
			if ((found && operandEntered) || !operandEntered) {
				(void) printf("Host Group: %ls\n",
				    groupNamePrint);
				if (verbose) {
					stmfRet = stmfGetHostGroupMembers(
					    &(groupList->name[j]), &groupProps);
					if (stmfRet != STMF_STATUS_SUCCESS) {
						return (1);
					}
					printGroupProps(groupProps);
				}
				if (found && operandEntered) {
					break;
				}
			}

		}
		if (operandEntered && !found) {
			(void) fprintf(stderr, "%s: %s: %s\n", cmdName,
			    operands[i], gettext("not found"));
			ret = 1;
		}
	}
	return (ret);
}

/*
 * printGroupProps
 *
 * Prints group members for target or host groups
 *
 */
static void
printGroupProps(stmfGroupProperties *groupProps)
{
	int i;
	wchar_t memberIdent[sizeof (groupProps->name[0].ident) + 1] = {0};


	for (i = 0; i < groupProps->cnt; i++) {
		(void) mbstowcs(memberIdent, (char *)groupProps->name[i].ident,
		    sizeof (groupProps->name[0].ident));
		(void) printf("\tMember: %ls\n", memberIdent);
	}
}

/*
 * listTargetGroupFunc
 *
 * Lists the specified target groups or all if none are specified
 *
 */
/*ARGSUSED*/
static int
listTargetGroupFunc(int operandLen, char *operands[], cmdOptions_t *options,
    void *args)
{
	int ret = 0;
	int stmfRet;
	int i, j, outerLoop;
	boolean_t verbose = B_FALSE;
	boolean_t found = B_TRUE;
	boolean_t operandEntered;
	stmfGroupList *groupList;
	stmfGroupProperties *groupProps;
	wchar_t operandName[sizeof (stmfGroupName)];
	wchar_t groupNamePrint[sizeof (stmfGroupName)];

	for (; options->optval; options++) {
		switch (options->optval) {
			case 'v':
				verbose = B_TRUE;
				break;
			default:
				(void) fprintf(stderr, "%s: %c: %s\n",
				    cmdName, options->optval,
				    gettext("unknown option"));
				return (1);
		}
	}

	if (operandLen > 0) {
		outerLoop = operandLen;
		operandEntered = B_TRUE;
	} else {
		outerLoop = 1;
		operandEntered = B_FALSE;
	}

	stmfRet = stmfGetTargetGroupList(&groupList);
	if (stmfRet != STMF_STATUS_SUCCESS) {
		switch (stmfRet) {
			case STMF_ERROR_BUSY:
				(void) fprintf(stderr, "%s: %s\n", cmdName,
				    gettext("resource busy"));
				break;
			case STMF_ERROR_SERVICE_NOT_FOUND:
				(void) fprintf(stderr, "%s: %s\n", cmdName,
				    gettext("STMF service not found"));
				break;
			case STMF_ERROR_SERVICE_DATA_VERSION:
				(void) fprintf(stderr, "%s: %s\n", cmdName,
				    gettext("STMF service version incorrect"));
				break;
			case STMF_ERROR_PERM:
				(void) fprintf(stderr, "%s: %s\n", cmdName,
				    gettext("permission denied"));
				break;
			default:
				(void) fprintf(stderr, "%s: %s\n", cmdName,
				    gettext("unknown error"));
				break;
		}
		return (1);
	}

	for (i = 0; i < outerLoop; i++) {
		for (found = B_FALSE, j = 0; j < groupList->cnt; j++) {
			(void) mbstowcs(groupNamePrint,
			    (char *)groupList->name[j],
			    sizeof (stmfGroupName) - 1);
			groupNamePrint[sizeof (stmfGroupName) - 1] = 0;
			if (operandEntered) {
				(void) mbstowcs(operandName, operands[i],
				    sizeof (stmfGroupName) - 1);
				operandName[sizeof (stmfGroupName) - 1] = 0;
				if (wcscmp(operandName, groupNamePrint)
				    == 0) {
					found = B_TRUE;
				}
			}
			if ((found && operandEntered) || !operandEntered) {
				(void) printf("Target Group: %ls\n",
				    groupNamePrint);
				if (verbose) {
					stmfRet = stmfGetTargetGroupMembers(
					    &(groupList->name[j]), &groupProps);
					if (stmfRet != STMF_STATUS_SUCCESS) {
						return (1);
					}
					printGroupProps(groupProps);
				}
				if (found && operandEntered) {
					break;
				}
			}

		}
		if (operandEntered && !found) {
			(void) fprintf(stderr, "%s: %s: %s\n", cmdName,
			    operands[i], gettext("not found"));
			ret = 1;
		}
	}
	return (ret);
}

/*
 * listLuFunc
 *
 * List the logical units and optionally the properties
 *
 */
/*ARGSUSED*/
static int
listLuFunc(int operandLen, char *operands[], cmdOptions_t *options, void *args)
{
	cmdOptions_t *optionList = options;
	boolean_t operandEntered;
	int i, j;
	int ret = 0;
	int stmfRet;
	int outerLoop;
	unsigned int inGuid[sizeof (stmfGuid)];
	stmfGuid cmpGuid;
	boolean_t verbose = B_FALSE;
	boolean_t found;
	char sGuid[GUID_INPUT + 1];
	stmfGuidList *luList;
	stmfLogicalUnitProperties luProps;
	boolean_t invalidInput = B_FALSE;
	stmfViewEntryList *viewEntryList;

	for (; optionList->optval; optionList++) {
		switch (optionList->optval) {
			case 'v':
				verbose = B_TRUE;
				break;
		}
	}

	if ((stmfRet = stmfGetLogicalUnitList(&luList))
	    != STMF_STATUS_SUCCESS) {
		switch (stmfRet) {
			case STMF_ERROR_SERVICE_NOT_FOUND:
				(void) fprintf(stderr, "%s: %s\n", cmdName,
				    gettext("STMF service not found"));
				break;
			case STMF_ERROR_BUSY:
				(void) fprintf(stderr, "%s: %s\n", cmdName,
				    gettext("resource busy"));
				break;
			case STMF_ERROR_PERM:
				(void) fprintf(stderr, "%s: %s\n", cmdName,
				    gettext("permission denied"));
				break;
			case STMF_ERROR_SERVICE_DATA_VERSION:
				(void) fprintf(stderr, "%s: %s\n", cmdName,
				    gettext("STMF service version incorrect"));
				break;
			default:
				(void) fprintf(stderr, "%s: %s\n", cmdName,
				    gettext("list failed"));
				break;
		}
		return (1);
	}

	if (operandLen > 0) {
		operandEntered = B_TRUE;
		outerLoop = operandLen;
	} else {
		operandEntered = B_FALSE;
		outerLoop = 1;
	}


	for (invalidInput = B_FALSE, i = 0; i < outerLoop; i++) {
		if (operandEntered) {
			if (strlen(operands[i]) != GUID_INPUT) {
				invalidInput = B_TRUE;
			} else {
				for (j = 0; j < GUID_INPUT; j++) {
					if (!isxdigit(operands[i][j])) {
						invalidInput = B_TRUE;
						break;
					}
				}
			}
			if (invalidInput) {
				(void) fprintf(stderr, "%s: %s: %s%d%s\n",
				    cmdName, operands[i], gettext("must be "),
				    GUID_INPUT,
				    gettext(" hexadecimal digits long"));
				invalidInput = B_FALSE;
				continue;
			}

			for (j = 0; j < GUID_INPUT; j++) {
				sGuid[j] = tolower(operands[i][j]);
			}
			sGuid[j] = 0;

			(void) sscanf(sGuid,
			    "%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x",
			    &inGuid[0], &inGuid[1], &inGuid[2], &inGuid[3],
			    &inGuid[4], &inGuid[5], &inGuid[6], &inGuid[7],
			    &inGuid[8], &inGuid[9], &inGuid[10], &inGuid[11],
			    &inGuid[12], &inGuid[13], &inGuid[14], &inGuid[15]);

			for (j = 0; j < sizeof (stmfGuid); j++) {
				cmpGuid.guid[j] = inGuid[j];
			}
		}

		for (found = B_FALSE, j = 0; j < luList->cnt; j++) {
			if (operandEntered) {
				if (bcmp(luList->guid[j].guid, cmpGuid.guid,
				    sizeof (stmfGuid)) == 0) {
					found = B_TRUE;
				}
			}
			if ((found && operandEntered) || !operandEntered) {
				(void) printf("LU Name: ");
				printGuid(&luList->guid[j], stdout);
				(void) printf("\n");

				if (verbose) {
					stmfRet = stmfGetLogicalUnitProperties(
					    &(luList->guid[j]), &luProps);
					if (stmfRet == STMF_STATUS_SUCCESS) {
						printLuProps(&luProps);
					} else {
						(void) fprintf(stderr, "%s:",
						    cmdName);
						printGuid(&luList->guid[j],
						    stderr);
						(void) fprintf(stderr, "%s\n",
						    gettext(" get properties "
						    "failed"));
					}
					stmfRet = stmfGetViewEntryList(
					    &(luList->guid[j]),
					    &viewEntryList);
					(void) printf(PROPS_FORMAT,
					    "View Entry Count");
					if (stmfRet == STMF_STATUS_SUCCESS) {
						(void) printf("%d",
						    viewEntryList->cnt);
					} else if (stmfRet ==
					    STMF_ERROR_NOT_FOUND) {
						(void) printf("0");
					} else {
						(void) printf("unknown");
					}
					(void) printf("\n");
					ret = printExtLuProps(
					    &(luList->guid[j]));
				}
				if (found && operandEntered) {
					break;
				}
			}

		}
		if (operandEntered && !found) {
			(void) fprintf(stderr, "%s: %s: %s\n", cmdName,
			    operands[i], gettext("not found"));
			ret = 1;
		}
	}

	return (ret);
}

static void
printGuid(stmfGuid *guid, FILE *stream)
{
	int i;
	for (i = 0; i < 16; i++) {
		(void) fprintf(stream, "%02X", guid->guid[i]);
	}
}

static int
printExtLuProps(stmfGuid *guid)
{
	int stmfRet;
	luResource hdl = NULL;
	int ret = 0;
	char propVal[MAXNAMELEN];
	size_t propValSize = sizeof (propVal);

	if ((stmfRet = stmfGetLuResource(guid, &hdl))
	    != STMF_STATUS_SUCCESS) {
		switch (stmfRet) {
			case STMF_ERROR_BUSY:
				(void) fprintf(stderr, "%s: %s\n", cmdName,
				    gettext("resource busy"));
				break;
			case STMF_ERROR_PERM:
				(void) fprintf(stderr, "%s: %s\n", cmdName,
				    gettext("permission denied"));
				break;
			case STMF_ERROR_NOT_FOUND:
				/* No error here */
				return (0);
				break;
			default:
				(void) fprintf(stderr, "%s: %s\n", cmdName,
				    gettext("get extended properties failed"));
				break;
		}
		return (1);
	}

	stmfRet = stmfGetLuProp(hdl, STMF_LU_PROP_FILENAME, propVal,
	    &propValSize);
	(void) printf(PROPS_FORMAT, "Data File");
	if (stmfRet == STMF_STATUS_SUCCESS) {
		(void) printf("%s\n", propVal);
	} else if (stmfRet == STMF_ERROR_NO_PROP) {
		(void) printf("not set\n");
	} else if (stmfRet == STMF_ERROR_NO_PROP_STANDBY) {
		(void) printf("prop unavailable in standby\n");
	} else {
		(void) printf("<error retrieving property>\n");
		ret++;
	}

	stmfRet = stmfGetLuProp(hdl, STMF_LU_PROP_META_FILENAME, propVal,
	    &propValSize);
	(void) printf(PROPS_FORMAT, "Meta File");
	if (stmfRet == STMF_STATUS_SUCCESS) {
		(void) printf("%s\n", propVal);
	} else if (stmfRet == STMF_ERROR_NO_PROP) {
		(void) printf("not set\n");
	} else if (stmfRet == STMF_ERROR_NO_PROP_STANDBY) {
		(void) printf("prop unavailable in standby\n");
	} else {
		(void) printf("<error retrieving property>\n");
		ret++;
	}

	stmfRet = stmfGetLuProp(hdl, STMF_LU_PROP_SIZE, propVal,
	    &propValSize);
	(void) printf(PROPS_FORMAT, "Size");
	if (stmfRet == STMF_STATUS_SUCCESS) {
		(void) printf("%s\n", propVal);
	} else if (stmfRet == STMF_ERROR_NO_PROP) {
		(void) printf("not set\n");
	} else if (stmfRet == STMF_ERROR_NO_PROP_STANDBY) {
		(void) printf("prop unavailable in standby\n");
	} else {
		(void) printf("<error retrieving property>\n");
		ret++;
	}

	stmfRet = stmfGetLuProp(hdl, STMF_LU_PROP_BLOCK_SIZE, propVal,
	    &propValSize);
	(void) printf(PROPS_FORMAT, "Block Size");
	if (stmfRet == STMF_STATUS_SUCCESS) {
		(void) printf("%s\n", propVal);
	} else if (stmfRet == STMF_ERROR_NO_PROP) {
		(void) printf("not set\n");
	} else if (stmfRet == STMF_ERROR_NO_PROP_STANDBY) {
		(void) printf("prop unavailable in standby\n");
	} else {
		(void) printf("<error retrieving property>\n");
		ret++;
	}

	stmfRet = stmfGetLuProp(hdl, STMF_LU_PROP_MGMT_URL, propVal,
	    &propValSize);
	(void) printf(PROPS_FORMAT, "Management URL");
	if (stmfRet == STMF_STATUS_SUCCESS) {
		(void) printf("%s\n", propVal);
	} else if (stmfRet == STMF_ERROR_NO_PROP) {
		(void) printf("not set\n");
	} else if (stmfRet == STMF_ERROR_NO_PROP_STANDBY) {
		(void) printf("prop unavailable in standby\n");
	} else {
		(void) printf("<error retrieving property>\n");
		ret++;
	}

	stmfRet = stmfGetLuProp(hdl, STMF_LU_PROP_VID, propVal,
	    &propValSize);
	(void) printf(PROPS_FORMAT, "Vendor ID");
	if (stmfRet == STMF_STATUS_SUCCESS) {
		(void) printf("%s\n", propVal);
	} else if (stmfRet == STMF_ERROR_NO_PROP) {
		(void) printf("not set\n");
	} else if (stmfRet == STMF_ERROR_NO_PROP_STANDBY) {
		(void) printf("prop unavailable in standby\n");
	} else {
		(void) printf("<error retrieving property>\n");
		ret++;
	}

	stmfRet = stmfGetLuProp(hdl, STMF_LU_PROP_PID, propVal,
	    &propValSize);
	(void) printf(PROPS_FORMAT, "Product ID");
	if (stmfRet == STMF_STATUS_SUCCESS) {
		(void) printf("%s\n", propVal);
	} else if (stmfRet == STMF_ERROR_NO_PROP) {
		(void) printf("not set\n");
	} else if (stmfRet == STMF_ERROR_NO_PROP_STANDBY) {
		(void) printf("prop unavailable in standby\n");
	} else {
		(void) printf("<error retrieving property>\n");
		ret++;
	}

	stmfRet = stmfGetLuProp(hdl, STMF_LU_PROP_SERIAL_NUM, propVal,
	    &propValSize);
	(void) printf(PROPS_FORMAT, "Serial Num");
	if (stmfRet == STMF_STATUS_SUCCESS) {
		(void) printf("%s\n", propVal);
	} else if (stmfRet == STMF_ERROR_NO_PROP) {
		(void) printf("not set\n");
	} else if (stmfRet == STMF_ERROR_NO_PROP_STANDBY) {
		(void) printf("prop unavailable in standby\n");
	} else {
		(void) printf("<error retrieving property>\n");
		ret++;
	}

	stmfRet = stmfGetLuProp(hdl, STMF_LU_PROP_WRITE_PROTECT, propVal,
	    &propValSize);
	(void) printf(PROPS_FORMAT, "Write Protect");
	if (stmfRet == STMF_STATUS_SUCCESS) {
		(void) printf("%s\n",
		    strcasecmp(propVal, "true") ? "Disabled" : "Enabled");
	} else if (stmfRet == STMF_ERROR_NO_PROP) {
		(void) printf("not set\n");
	} else if (stmfRet == STMF_ERROR_NO_PROP_STANDBY) {
		(void) printf("prop unavailable in standby\n");
	} else {
		(void) printf("<error retrieving property>\n");
		ret++;
	}

	stmfRet = stmfGetLuProp(hdl, STMF_LU_PROP_WRITE_CACHE_DISABLE, propVal,
	    &propValSize);
	(void) printf(PROPS_FORMAT, "Writeback Cache");
	if (stmfRet == STMF_STATUS_SUCCESS) {
		(void) printf("%s\n",
		    strcasecmp(propVal, "true") ? "Enabled" : "Disabled");
	} else if (stmfRet == STMF_ERROR_NO_PROP) {
		(void) printf("not set\n");
	} else if (stmfRet == STMF_ERROR_NO_PROP_STANDBY) {
		(void) printf("prop unavailable in standby\n");
	} else {
		(void) printf("<error retrieving property>\n");
		ret++;
	}

	stmfRet = stmfGetLuProp(hdl, STMF_LU_PROP_ACCESS_STATE, propVal,
	    &propValSize);
	(void) printf(PROPS_FORMAT, "Access State");
	if (stmfRet == STMF_STATUS_SUCCESS) {
		if (strcmp(propVal, STMF_ACCESS_ACTIVE) == 0) {
			(void) printf("%s\n", "Active");
		} else if (strcmp(propVal,
		    STMF_ACCESS_ACTIVE_TO_STANDBY) == 0) {
			(void) printf("%s\n", "Active->Standby");
		} else if (strcmp(propVal, STMF_ACCESS_STANDBY) == 0) {
			(void) printf("%s\n", "Standby");
			stmfRet = stmfGetLuProp(hdl, STMF_LU_PROP_ACTIVE_HOST_ID, propVal,
				&propValSize);
			if (stmfRet == STMF_STATUS_SUCCESS) {
				(void) printf(PROPS_FORMAT, "Active Hostid");
				(void) printf("%s\n", propVal);
			}
		} else if (strcmp(propVal,
		    STMF_ACCESS_STANDBY_TO_ACTIVE) == 0) {
			(void) printf("%s\n", "Standby->Active");
		} else {
			(void) printf("%s\n", "Unknown");
		}
	} else if (stmfRet == STMF_ERROR_NO_PROP) {
		(void) printf("not set\n");
	} else {
		(void) printf("<error retrieving property>\n");
		ret++;
	}

done:
	(void) stmfFreeLuResource(hdl);
	return (ret);

}


/*
 * printLuProps
 *
 * Prints the properties for a logical unit
 *
 */
static void
printLuProps(stmfLogicalUnitProperties *luProps)
{
	(void) printf(PROPS_FORMAT, "Operational Status");
	switch (luProps->status) {
		case STMF_LOGICAL_UNIT_ONLINE:
			(void) printf("Online");
			break;
		case STMF_LOGICAL_UNIT_OFFLINE:
			(void) printf("Offline");
			break;
		case STMF_LOGICAL_UNIT_ONLINING:
			(void) printf("Onlining");
			break;
		case STMF_LOGICAL_UNIT_OFFLINING:
			(void) printf("Offlining");
			break;
		case STMF_LOGICAL_UNIT_UNREGISTERED:
			(void) printf("unregistered");
			(void) strncpy(luProps->providerName, "unregistered",
			    sizeof (luProps->providerName));
			break;
		default:
			(void) printf("unknown");
			break;
	}
	(void) printf("\n");
	(void) printf(PROPS_FORMAT, "Provider Name");
	if (luProps->providerName[0] != 0) {
		(void) printf("%s", luProps->providerName);
	} else {
		(void) printf("unknown");
	}
	(void) printf("\n");
	(void) printf(PROPS_FORMAT, "Alias");
	if (luProps->alias[0] != 0) {
		(void) printf("%s", luProps->alias);
	} else {
		(void) printf("-");
	}
	(void) printf("\n");
}

/*
 * printTargetProps
 *
 * Prints the properties for a target
 *
 */
static void
printTargetProps(stmfTargetProperties *targetProps)
{
	(void) printf(PROPS_FORMAT, "Operational Status");
	switch (targetProps->status) {
		case STMF_TARGET_PORT_ONLINE:
			(void) printf("Online");
			break;
		case STMF_TARGET_PORT_OFFLINE:
			(void) printf("Offline");
			break;
		case STMF_TARGET_PORT_ONLINING:
			(void) printf("Onlining");
			break;
		case STMF_TARGET_PORT_OFFLINING:
			(void) printf("Offlining");
			break;
		default:
			(void) printf("unknown");
			break;
	}
	(void) printf("\n");
	(void) printf(PROPS_FORMAT, "Provider Name");
	if (targetProps->providerName[0] != 0) {
		(void) printf("%s", targetProps->providerName);
	}
	(void) printf("\n");
	(void) printf(PROPS_FORMAT, "Alias");
	if (targetProps->alias[0] != 0) {
		(void) printf("%s", targetProps->alias);
	} else {
		(void) printf("-");
	}
	(void) printf("\n");
	(void) printf(PROPS_FORMAT, "Protocol");
	switch (targetProps->protocol) {
		case STMF_PROTOCOL_FIBRE_CHANNEL:
			(void) printf("%s", "Fibre Channel");
			break;
		case STMF_PROTOCOL_ISCSI:
			(void) printf("%s", "iSCSI");
			break;
		case STMF_PROTOCOL_SRP:
			(void) printf("%s", "SRP");
			break;
		case STMF_PROTOCOL_SAS:
			(void) printf("%s", "SAS");
			break;
		default:
			(void) printf("%s", "unknown");
			break;
	}

	(void) printf("\n");
}

/*
 * printSessionProps
 *
 * Prints the session data
 *
 */
static void
printSessionProps(stmfSessionList *sessionList)
{
	int i;
	char *cTime;
	wchar_t initiator[STMF_IDENT_LENGTH + 1];

	(void) printf(PROPS_FORMAT, "Sessions");
	(void) printf("%d\n", sessionList->cnt);
	for (i = 0; i < sessionList->cnt; i++) {
		(void) mbstowcs(initiator,
		    (char *)sessionList->session[i].initiator.ident,
		    STMF_IDENT_LENGTH);
		initiator[STMF_IDENT_LENGTH] = 0;
		(void) printf(LVL3_FORMAT, "Initiator: ");
		(void) printf("%ls\n", initiator);
		(void) printf(LVL4_FORMAT, "Alias: ");
		if (sessionList->session[i].alias[0] != 0) {
			(void) printf("%s", sessionList->session[i].alias);
		} else {
			(void) printf("-");
		}
		(void) printf("\n");
		(void) printf(LVL4_FORMAT, "Logged in since: ");
		cTime = ctime(&(sessionList->session[i].creationTime));
		if (cTime != NULL) {
			(void) printf("%s", cTime);
		} else {
			(void) printf("unknown\n");
		}
	}
}

static int
getStmfState(stmfState *state)
{
	int ret;

	ret = stmfGetState(state);
	switch (ret) {
		case STMF_STATUS_SUCCESS:
			break;
		case STMF_ERROR_PERM:
			(void) fprintf(stderr, "%s: %s\n", cmdName,
			    gettext("permission denied"));
			break;
		case STMF_ERROR_SERVICE_NOT_FOUND:
			(void) fprintf(stderr, "%s: %s\n", cmdName,
			    gettext("STMF service not found"));
			break;
		case STMF_ERROR_BUSY:
			(void) fprintf(stderr, "%s: %s\n", cmdName,
			    gettext("resource busy"));
			break;
		case STMF_ERROR_SERVICE_DATA_VERSION:
			(void) fprintf(stderr, "%s: %s\n", cmdName,
			    gettext("STMF service version incorrect"));
			break;
		default:
			(void) fprintf(stderr, "%s: %s: %d\n", cmdName,
			    gettext("unknown error"), ret);
			break;
	}
	return (ret);
}

/*
 * listStateFunc
 *
 * List the operational and config state of the stmf service
 *
 */
/*ARGSUSED*/
static int
listStateFunc(int operandLen, char *operands[], cmdOptions_t *options,
    void *args)
{
	int ret;
	stmfState state;
	boolean_t aluaEnabled;
	uint32_t node;

	if ((ret = getStmfState(&state)) != STMF_STATUS_SUCCESS)
		return (ret);

	(void) printf("%-18s: ", "Operational Status");
	switch (state.operationalState) {
		case STMF_SERVICE_STATE_ONLINE:
			(void) printf("online");
			break;
		case STMF_SERVICE_STATE_OFFLINE:
			(void) printf("offline");
			break;
		case STMF_SERVICE_STATE_ONLINING:
			(void) printf("onlining");
			break;
		case STMF_SERVICE_STATE_OFFLINING:
			(void) printf("offlining");
			break;
		default:
			(void) printf("unknown");
			break;
	}
	(void) printf("\n");
	(void) printf("%-18s: ", "Config Status");
	switch (state.configState) {
		case STMF_CONFIG_STATE_NONE:
			(void) printf("uninitialized");
			break;
		case STMF_CONFIG_STATE_INIT:
			(void) printf("initializing");
			break;
		case STMF_CONFIG_STATE_INIT_DONE:
			(void) printf("initialized");
			break;
		default:
			(void) printf("unknown");
			break;
	}
	(void) printf("\n");
	ret = stmfGetAluaState(&aluaEnabled, &node);
	switch (ret) {
		case STMF_STATUS_SUCCESS:
			break;
		case STMF_ERROR_PERM:
			(void) fprintf(stderr, "%s: %s\n", cmdName,
			    gettext("permission denied"));
			break;
		case STMF_ERROR_BUSY:
			(void) fprintf(stderr, "%s: %s\n", cmdName,
			    gettext("resource busy"));
			break;
		default:
			(void) fprintf(stderr, "%s: %s: %d\n", cmdName,
			    gettext("unknown error"), ret);
			break;
	}
	(void) printf("%-18s: ", "ALUA Status");
	if (ret == STMF_STATUS_SUCCESS) {
		if (aluaEnabled == B_TRUE) {
			(void) printf("enabled");
		} else {
			(void) printf("disabled");
		}
	} else {
		(void) printf("unknown");
	}

	(void) printf("\n");
	(void) printf("%-18s: ", "ALUA Node");
	if (ret == STMF_STATUS_SUCCESS) {
		(void) printf("%d", node);
	} else {
		(void) printf("unknown");
	}
	(void) printf("\n");
	return (ret);
}

/*
 * listTargetFunc
 *
 * list the targets and optionally their properties
 *
 */
/*ARGSUSED*/
static int
listTargetFunc(int operandLen, char *operands[], cmdOptions_t *options,
    void *args)
{
	cmdOptions_t *optionList = options;
	int ret = 0;
	int stmfRet;
	int i, j;
	int outerLoop;
	stmfSessionList *sessionList;
	stmfDevid devid;
	boolean_t operandEntered, found, verbose = B_FALSE;
	stmfDevidList *targetList;
	wchar_t targetIdent[STMF_IDENT_LENGTH + 1];
	stmfTargetProperties targetProps;

	if ((stmfRet = stmfGetTargetList(&targetList)) != STMF_STATUS_SUCCESS) {
		switch (stmfRet) {
			case STMF_ERROR_NOT_FOUND:
				ret = 0;
				break;
			case STMF_ERROR_SERVICE_OFFLINE:
				(void) fprintf(stderr, "%s: %s\n", cmdName,
				    gettext("STMF service offline"));
				break;
			case STMF_ERROR_BUSY:
				(void) fprintf(stderr, "%s: %s\n", cmdName,
				    gettext("resource busy"));
				break;
			case STMF_ERROR_SERVICE_DATA_VERSION:
				(void) fprintf(stderr, "%s: %s\n", cmdName,
				    gettext("STMF service version incorrect"));
				break;
			case STMF_ERROR_PERM:
				(void) fprintf(stderr, "%s: %s\n", cmdName,
				    gettext("permission denied"));
				break;
			default:
				(void) fprintf(stderr, "%s: %s\n", cmdName,
				    gettext("unknown error"));
				break;
		}
		return (1);
	}

	for (; optionList->optval; optionList++) {
		switch (optionList->optval) {
			case 'v':
				verbose = B_TRUE;
				break;
		}
	}

	if (operandLen > 0) {
		outerLoop = operandLen;
		operandEntered = B_TRUE;
	} else {
		outerLoop = 1;
		operandEntered = B_FALSE;
	}

	for (i = 0; i < outerLoop; i++) {
		if (operandEntered) {
			bzero(&devid, sizeof (devid));
			(void) parseDevid(operands[i], &devid);
		}
		for (found = B_FALSE, j = 0; j < targetList->cnt; j++) {
			if (operandEntered) {
				if (bcmp(&devid, &(targetList->devid[j]),
				    sizeof (devid)) == 0) {
					found = B_TRUE;
				}
			}
			if ((found && operandEntered) || !operandEntered) {
				(void) mbstowcs(targetIdent,
				    (char *)targetList->devid[j].ident,
				    STMF_IDENT_LENGTH);
				targetIdent[STMF_IDENT_LENGTH] = 0;
				(void) printf("Target: %ls\n", targetIdent);
				if (verbose) {
					stmfRet = stmfGetTargetProperties(
					    &(targetList->devid[j]),
					    &targetProps);
					if (stmfRet == STMF_STATUS_SUCCESS) {
						printTargetProps(&targetProps);
					} else {
						(void) fprintf(stderr, "%s:",
						    cmdName);
						(void) fprintf(stderr, "%s\n",
						    gettext(" get properties"
						    " failed"));
					}
					stmfRet = stmfGetSessionList(
					    &(targetList->devid[j]),
					    &sessionList);
					if (stmfRet == STMF_STATUS_SUCCESS) {
						printSessionProps(sessionList);
					} else {
						(void) fprintf(stderr, "%s:",
						    cmdName);
						(void) fprintf(stderr, "%s\n",
						    gettext(" get session info"
						    " failed"));
					}
				}
				if (found && operandEntered) {
					break;
				}
			}

		}
		if (operandEntered && !found) {
			(void) fprintf(stderr, "%s: %s: %s\n", cmdName,
			    operands[i], "not found");
			ret = 1;
		}
	}
	return (ret);
}

/*
 * listViewFunc
 *
 * list the view entries for the specified logical unit
 *
 */
/*ARGSUSED*/
static int
listViewFunc(int operandLen, char *operands[], cmdOptions_t *options,
    void *args)
{
	stmfViewEntryList *viewEntryList;
	stmfGuid inGuid;
	unsigned int guid[sizeof (stmfGuid)];
	int ret = 0;
	int stmfRet;
	int i, j, outerLoop;
	boolean_t found = B_TRUE;
	boolean_t operandEntered;
	uint16_t outputLuNbr;
	wchar_t groupName[sizeof (stmfGroupName)];
	char sGuid[GUID_INPUT + 1];


	for (; options->optval; options++) {
		switch (options->optval) {
			case 'l':
				if (strlen(options->optarg) != GUID_INPUT) {
					(void) fprintf(stderr,
					    "%s: %s: %s%d%s\n",
					    cmdName, options->optarg,
					    gettext("must be "), GUID_INPUT,
					    gettext(" hexadecimal digits"
					    " long"));
					return (1);
				}
				bcopy(options->optarg, sGuid, GUID_INPUT);
				break;
			default:
				(void) fprintf(stderr, "%s: %c: %s\n",
				    cmdName, options->optval,
				    gettext("unknown option"));
				return (1);
		}
	}

	if (operandLen > 0) {
		outerLoop = operandLen;
		operandEntered = B_TRUE;
	} else {
		outerLoop = 1;
		operandEntered = B_FALSE;
	}

	for (i = 0; i < 32; i++)
		sGuid[i] = tolower(sGuid[i]);
	sGuid[i] = 0;

	(void) sscanf(sGuid, "%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x",
	    &guid[0], &guid[1], &guid[2], &guid[3], &guid[4], &guid[5],
	    &guid[6], &guid[7], &guid[8], &guid[9], &guid[10], &guid[11],
	    &guid[12], &guid[13], &guid[14], &guid[15]);

	for (i = 0; i < sizeof (stmfGuid); i++) {
		inGuid.guid[i] = guid[i];
	}

	if ((stmfRet = stmfGetViewEntryList(&inGuid, &viewEntryList))
	    != STMF_STATUS_SUCCESS) {

		switch (stmfRet) {
			case STMF_ERROR_BUSY:
				(void) fprintf(stderr, "%s: %s: %s\n", cmdName,
				    sGuid, gettext("resource busy"));
				break;
			case STMF_ERROR_NOT_FOUND:
				(void) fprintf(stderr, "%s: %s: %s\n", cmdName,
				    sGuid, gettext("no views found"));
				break;
			case STMF_ERROR_SERVICE_NOT_FOUND:
				(void) fprintf(stderr, "%s: %s\n", cmdName,
				    gettext("STMF service not found"));
				break;
			case STMF_ERROR_SERVICE_DATA_VERSION:
				(void) fprintf(stderr, "%s: %s\n", cmdName,
				    gettext("STMF service version incorrect"));
				break;
			case STMF_ERROR_PERM:
				(void) fprintf(stderr, "%s: %s\n", cmdName,
				    gettext("permission denied"));
				break;
			default:
				(void) fprintf(stderr, "%s: %s: %s\n", cmdName,
				    sGuid, gettext("unknown error"));
				break;
		}
		return (1);
	}

	for (i = 0; i < outerLoop; i++) {
		for (found = B_FALSE, j = 0; j < viewEntryList->cnt; j++) {
			if (operandEntered) {
				if (atoi(operands[i]) ==
				    viewEntryList->ve[j].veIndex) {
					found = B_TRUE;
				}
			}
			if ((found && operandEntered) || !operandEntered) {
				(void) printf("View Entry: %d\n",
				    viewEntryList->ve[j].veIndex);
				(void) printf(VIEW_FORMAT, "Host group");
				if (viewEntryList->ve[j].allHosts) {
					(void) printf("All\n");
				} else {
					(void) mbstowcs(groupName,
					    viewEntryList->ve[j].hostGroup,
					    sizeof (stmfGroupName) - 1);
					groupName[sizeof (stmfGroupName) - 1]
					    = 0;
					(void) printf("%ls\n", groupName);
				}
				(void) printf(VIEW_FORMAT, "Target group");
				if (viewEntryList->ve[j].allTargets) {
					(void) printf("All\n");
				} else {
					(void) mbstowcs(groupName,
					    viewEntryList->ve[j].targetGroup,
					    sizeof (stmfGroupName) - 1);
					groupName[sizeof (stmfGroupName) - 1]
					    = 0;
					(void) printf("%ls\n", groupName);
				}
				outputLuNbr = ((viewEntryList->ve[j].luNbr[0] &
				    0x3F) << 8) | viewEntryList->ve[j].luNbr[1];
				(void) printf(VIEW_FORMAT, "LUN");
				(void) printf("%d\n", outputLuNbr);
				if (found && operandEntered) {
					break;
				}
			}
		}
		if (operandEntered && !found) {
			(void) fprintf(stderr, "%s: %s, %s: %s\n", cmdName,
			    sGuid, operands[i], gettext("not found"));
			ret = 1;
		}
	}

	return (ret);
}


/*
 * onlineOfflineLu
 *
 * Purpose: Online or offline a logical unit
 *
 * lu - logical unit to online or offline
 *
 * state - ONLINE_LU
 *         OFFLINE_LU
 */
static int
onlineOfflineLu(char *lu, int state)
{
	char sGuid[GUID_INPUT + 1];
	stmfGuid inGuid;
	unsigned int guid[sizeof (stmfGuid)];
	int i;
	int ret = 0;

	if (strlen(lu) != GUID_INPUT) {
		(void) fprintf(stderr, "%s: %s: %s %d %s\n", cmdName, lu,
		    gettext("must be"), GUID_INPUT,
		    gettext("hexadecimal digits long"));
		return (1);
	}

	bcopy(lu, sGuid, GUID_INPUT);

	for (i = 0; i < 32; i++)
		sGuid[i] = tolower(sGuid[i]);
	sGuid[i] = 0;

	(void) sscanf(sGuid, "%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x",
	    &guid[0], &guid[1], &guid[2], &guid[3], &guid[4], &guid[5],
	    &guid[6], &guid[7], &guid[8], &guid[9], &guid[10], &guid[11],
	    &guid[12], &guid[13], &guid[14], &guid[15]);

	for (i = 0; i < sizeof (stmfGuid); i++) {
		inGuid.guid[i] = guid[i];
	}

	if (state == ONLINE_LU) {
		ret = stmfOnlineLogicalUnit(&inGuid);
	} else if (state == OFFLINE_LU) {
		ret = stmfOfflineLogicalUnit(&inGuid);
	}
	if (ret != STMF_STATUS_SUCCESS) {
		switch (ret) {
			case STMF_ERROR_PERM:
				(void) fprintf(stderr, "%s: %s\n", cmdName,
				    gettext("permission denied"));
				break;
			case STMF_ERROR_SERVICE_NOT_FOUND:
				(void) fprintf(stderr, "%s: %s\n", cmdName,
				    gettext("STMF service not found"));
				break;
			case STMF_ERROR_NOT_FOUND:
				(void) fprintf(stderr, "%s: %s: %s\n", cmdName,
				    lu, gettext("not found"));
				break;
			case STMF_ERROR_SERVICE_DATA_VERSION:
				(void) fprintf(stderr, "%s: %s\n", cmdName,
				    gettext("STMF service version incorrect"));
				break;
			default:
				(void) fprintf(stderr, "%s: %s\n", cmdName,
				    gettext("unknown error"));
				break;
		}
	}
	return (ret);
}

/*
 * onlineLuFunc
 *
 * Purpose: Online a logical unit
 *
 */
/*ARGSUSED*/
static int
onlineLuFunc(int operandLen, char *operands[], cmdOptions_t *options,
    void *args)
{
	int ret;
	stmfState state;

	ret = getStmfState(&state);
	if (ret != STMF_STATUS_SUCCESS)
		return (ret);
	if (state.operationalState == STMF_SERVICE_STATE_OFFLINE ||
	    state.operationalState == STMF_SERVICE_STATE_OFFLINING) {
		(void) fprintf(stderr, "%s: %s\n", cmdName,
		    gettext("STMF service is offline"));
		return (1);
	}
	return (onlineOfflineLu(operands[0], ONLINE_LU));
}

/*
 * offlineLuFunc
 *
 * Purpose: Offline a logical unit
 *
 */
/*ARGSUSED*/
static int
offlineLuFunc(int operandLen, char *operands[], cmdOptions_t *options,
    void *args)
{
	return (onlineOfflineLu(operands[0], OFFLINE_LU));
}

static int
unmapLu(char *lu)
{
	char sGuid[GUID_INPUT + 1];
	stmfGuid inGuid;
	unsigned int guid[sizeof (stmfGuid)];
	int i;
	int ret = 0;

	if (strlen(lu) != GUID_INPUT) {
		(void) fprintf(stderr, "%s: %s: %s %d %s\n", cmdName, lu,
		    gettext("must be"), GUID_INPUT,
		    gettext("hexadecimal digits long"));
		return (1);
	}

	bcopy(lu, sGuid, GUID_INPUT);

	for (i = 0; i < 32; i++)
		sGuid[i] = tolower(sGuid[i]);
	sGuid[i] = 0;

	(void) sscanf(sGuid, "%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x",
	    &guid[0], &guid[1], &guid[2], &guid[3], &guid[4], &guid[5],
	    &guid[6], &guid[7], &guid[8], &guid[9], &guid[10], &guid[11],
	    &guid[12], &guid[13], &guid[14], &guid[15]);

	for (i = 0; i < sizeof (stmfGuid); i++) {
		inGuid.guid[i] = guid[i];
	}

	ret = stmfUnmapLu(&inGuid, 0, 0);

	if (ret)
		fprintf(stderr, "stmf unmap lu failed, ret=%d", ret);
	return (ret);
}

/*
 * unmapLuFunc
 */

static int
unmapLuFunc(int operandLen, char *operands[], cmdOptions_t *options,
    void *args)
{
	return (unmapLu(operands[0]));
}

/*
 * online or offline all pppt target for clusterd
 */
static int
onlineOfflinePpptTarget(int state)
{
	int ret = 0, i, err = 0;
	stmfDevidList *targetList;
	stmfTargetProperties targetProps;
	wchar_t targetIdent[STMF_IDENT_LENGTH + 1];
	
	if ((ret = stmfGetTargetList(&targetList)) != STMF_STATUS_SUCCESS) {
		syslog(LOG_ERR, "online offline pppt get target list failed, ret:0x%x", ret);
		return (1);
	} else {
		for (i = 0; i < targetList->cnt; i++) {
			ret = stmfGetTargetProperties(&(targetList->devid[i]),
				&targetProps);
			if (ret == STMF_STATUS_SUCCESS) {
				mbstowcs(targetIdent, (char *)targetList->devid[i].ident,
					STMF_IDENT_LENGTH);
				targetIdent[STMF_IDENT_LENGTH] = 0;
				
				if (strcmp(targetProps.providerName, "pppt") == 0) {
					if (state == ONLINE_TARGET) {
						ret = stmfOnlineTarget(&(targetList->devid[i]));
					} else if (state == OFFLINE_TARGET) {
						ret = stmfOfflineTarget(&(targetList->devid[i]));
					}
					if (ret != STMF_STATUS_SUCCESS) {
						char *target;
						int len = 2 * wcslen(targetIdent) + 1;
						target = (char *)malloc(len);
						wcstombs(target, targetIdent, len);
						syslog(LOG_ERR, "online offline pppt target failed,"
							" %s, state:%d", target, state);
						free(target);
						err = 1;
					} else {
						printf("target:%ls, provider:%s\n",
							targetIdent, targetProps.providerName);
					}
				}
			} else {
				syslog(LOG_ERR, "offline ppt get target props failed");
				err = 1;
			}
		}
	}

	return (err);
	
}

/*
 * onlineOfflineTarget
 *
 * Purpose: Online or offline a target
 *
 * target - target to online or offline
 *
 * state - ONLINE_TARGET
 *         OFFLINE_TARGET
 */
static int
onlineOfflineTarget(char *target, int state)
{
	int ret = 0;
	stmfDevid devid;

	if (parseDevid(target, &devid) != 0) {
		(void) fprintf(stderr, "%s: %s: %s\n",
		    cmdName, target, gettext("unrecognized device id"));
		return (1);
	}
	if (state == ONLINE_TARGET) {
		ret = stmfOnlineTarget(&devid);
	} else if (state == OFFLINE_TARGET) {
		ret = stmfOfflineTarget(&devid);
	}
	if (ret != STMF_STATUS_SUCCESS) {
		switch (ret) {
			case STMF_ERROR_PERM:
				(void) fprintf(stderr, "%s: %s\n", cmdName,
				    gettext("permission denied"));
				break;
			case STMF_ERROR_SERVICE_NOT_FOUND:
				(void) fprintf(stderr, "%s: %s\n", cmdName,
				    gettext("STMF service not found"));
				break;
			case STMF_ERROR_NOT_FOUND:
				(void) fprintf(stderr, "%s: %s: %s\n", cmdName,
				    target, gettext("not found"));
				break;
			case STMF_ERROR_SERVICE_DATA_VERSION:
				(void) fprintf(stderr, "%s: %s\n", cmdName,
				    gettext("STMF service version incorrect"));
				break;
			default:
				(void) fprintf(stderr, "%s: %s\n", cmdName,
				    gettext("unknown error"));
				break;
		}
	}
	return (ret);
}

/*
 * onlineTargetFunc
 *
 * Purpose: Online a target
 *
 */
/*ARGSUSED*/
static int
onlineTargetFunc(int operandLen, char *operands[], cmdOptions_t *options,
    void *args)
{
	int ret;
	stmfState state;

	bool_t isCluster = false;
	
	for (; options->optval; options++) {
		switch (options->optval) {
			case 'c':
				isCluster = true;
				break;
			default:
				(void) fprintf(stderr, "%s: %c: %s\n",
				    cmdName, options->optval,
				    gettext("unknown option"));
				return (1);
		}
	}	

	ret = getStmfState(&state);
	if (ret != STMF_STATUS_SUCCESS)
		return (ret);
	if (state.operationalState == STMF_SERVICE_STATE_OFFLINE ||
	    state.operationalState == STMF_SERVICE_STATE_OFFLINING) {
		(void) fprintf(stderr, "%s: %s\n", cmdName,
		    gettext("STMF service is offline"));
		return (1);
	}

	if (operands[0] != NULL && strcmp(operands[0], "pppt") == 0) {
		ret = onlineOfflinePpptTarget(ONLINE_TARGET);
		/*return (onlineOfflinePpptTarget(ONLINE_TARGET));*/		
	} else {
		ret = onlineOfflineTarget(operands[0], ONLINE_TARGET);
		/*return (onlineOfflineTarget(operands[0], ONLINE_TARGET));*/
	}
		
	if (ret == STMF_STATUS_SUCCESS && true == isCluster)	
		stmfadm_send_cmd(cmdfullName);
	return ret;
}


/*
 * offlineTargetFunc
 *
 * Purpose: Offline a target
 *
 */
/*ARGSUSED*/
static int
offlineTargetFunc(int operandLen, char *operands[], cmdOptions_t *options,
    void *args)
{	
	int ret = 0;
	bool_t isCluster = false;
	
	for (; options->optval; options++) {
		switch (options->optval) {
			case 'c':
				isCluster = true;
				break;
			default:
				(void) fprintf(stderr, "%s: %c: %s\n",
				    cmdName, options->optval,
				    gettext("unknown option"));
				return (1);
		}
	}	
	
	if (operands[0] != NULL && strcmp(operands[0], "pppt") == 0) {
		ret = onlineOfflinePpptTarget(OFFLINE_TARGET);
		/*return (onlineOfflinePpptTarget(OFFLINE_TARGET));*/		
	} else {
		ret = onlineOfflineTarget(operands[0], OFFLINE_TARGET);
		/*return (onlineOfflineTarget(operands[0], OFFLINE_TARGET));*/
	}
		
	if (ret == STMF_STATUS_SUCCESS && true == isCluster)	
			stmfadm_send_cmd(cmdfullName);
	return ret;
}


/*ARGSUSED*/
static int
removeHostGroupMemberFunc(int operandLen, char *operands[],
    cmdOptions_t *options, void *args)
{
	int i;
	int ret = 0;
	int stmfRet;
	stmfGroupName groupName = {0};
	stmfDevid devid;
	wchar_t groupNamePrint[sizeof (stmfGroupName)] = {0};
	stmf_proxy_device_t *proxy_device;
	
	bool_t isCluster = false;

	for (; options->optval; options++) {
		switch (options->optval) {
			case 'g':
				(void) mbstowcs(groupNamePrint, options->optarg,
				    sizeof (stmfGroupName) - 1);
				bcopy(options->optarg, groupName,
				    strlen(options->optarg));
				break;
			case 'c':
				isCluster = true;
				break;
			default:
				(void) fprintf(stderr, "%s: %c: %s\n",
				    cmdName, options->optval,
				    gettext("unknown option"));
				return (1);
		}
	}

	for (i = 0; i < operandLen; i++) {
		if (parseDevid(operands[i], &devid) != 0) {
			(void) fprintf(stderr, "%s: %s: %s\n",
			    cmdName, operands[i],
			    gettext("unrecognized device id"));
			ret++;
			continue;
		}
		stmfRet = stmfRemoveFromHostGroup(&groupName, &devid);
		switch (stmfRet) {
			case STMF_STATUS_SUCCESS:
				proxy_device = malloc(sizeof(stmf_proxy_device_t));
				proxy_device->head.item_type = STMF_HG_MEMBER_OP;
				proxy_device->head.op_type = STMF_OP_DELETE;
				bcopy(&groupName, &proxy_device->name, sizeof(stmfGroupName));
				bcopy(&devid, &proxy_device->device, sizeof(stmfDevid));
				free(proxy_device);

				if (true == isCluster)
					stmfadm_send_cmd(cmdfullName);
				break;
			case STMF_ERROR_MEMBER_NOT_FOUND:
				(void) fprintf(stderr, "%s: %s: %s\n", cmdName,
				    operands[i], gettext("not found"));
				ret++;
				break;
			case STMF_ERROR_GROUP_NOT_FOUND:
				(void) fprintf(stderr, "%s: %ls: %s\n", cmdName,
				    groupNamePrint, gettext("not found"));
				ret++;
				break;
			case STMF_ERROR_BUSY:
				(void) fprintf(stderr, "%s: %s: %s\n", cmdName,
				    operands[i], "resource busy");
				ret++;
				break;
			case STMF_ERROR_SERVICE_NOT_FOUND:
				(void) fprintf(stderr, "%s: %s\n", cmdName,
				    gettext("STMF service not found"));
				ret++;
				break;
			case STMF_ERROR_SERVICE_DATA_VERSION:
				(void) fprintf(stderr, "%s: %s\n", cmdName,
				    gettext("STMF service version incorrect"));
				ret++;
				break;
			case STMF_ERROR_PERM:
				(void) fprintf(stderr, "%s: %s\n", cmdName,
				    gettext("permission denied"));
				ret++;
				break;
			default:
				(void) fprintf(stderr, "%s: %s: %s\n", cmdName,
				    operands[i], gettext("unknown error"));
				ret++;
				break;
		}
	}

	return (ret);
}

/*
 * removeTargetGroupMemberFunc
 *
 * Removes one or more members from a target group
 *
 */
/*ARGSUSED*/
static int
removeTargetGroupMemberFunc(int operandLen, char *operands[],
    cmdOptions_t *options, void *args)
{
	int i;
	int ret = 0;
	int stmfRet;
	stmfGroupName groupName = {0};
	stmfDevid devid;
	wchar_t groupNamePrint[sizeof (stmfGroupName)] = {0};
	stmf_proxy_device_t *proxy_device;
	
	bool_t isCluster = false;

	for (; options->optval; options++) {
		switch (options->optval) {
			case 'g':
				(void) mbstowcs(groupNamePrint, options->optarg,
				    sizeof (stmfGroupName) - 1);
				bcopy(options->optarg, groupName,
				    strlen(options->optarg));
				break;
			case 'c':
				isCluster = true;
				break;
			default:
				(void) fprintf(stderr, "%s: %c: %s\n",
				    cmdName, options->optval,
				    gettext("unknown option"));
				return (1);
		}
	}

	for (i = 0; i < operandLen; i++) {
		if (parseDevid(operands[i], &devid) != 0) {
			(void) fprintf(stderr, "%s: %s: %s\n",
			    cmdName, operands[i],
			    gettext("unrecognized device id"));
			ret++;
			continue;
		}
		stmfRet = stmfRemoveFromTargetGroup(&groupName, &devid);
		switch (stmfRet) {
			case STMF_STATUS_SUCCESS:
				proxy_device = malloc(sizeof(stmf_proxy_device_t));
				proxy_device->head.item_type = STMF_TG_MEMBER_OP;
				proxy_device->head.op_type = STMF_OP_DELETE;
				bcopy(&groupName, &proxy_device->name, sizeof(stmfGroupName));
				bcopy(&devid, &proxy_device->device, sizeof(stmfDevid));
				free(proxy_device);

				if (true == isCluster)
					stmfadm_send_cmd(cmdfullName);
				break;
			case STMF_ERROR_MEMBER_NOT_FOUND:
				(void) fprintf(stderr, "%s: %s: %s\n", cmdName,
				    operands[i], gettext("not found"));
				ret++;
				break;
			case STMF_ERROR_GROUP_NOT_FOUND:
				(void) fprintf(stderr, "%s: %ls: %s\n", cmdName,
				    groupNamePrint, gettext("not found"));
				ret++;
				break;
			case STMF_ERROR_BUSY:
				(void) fprintf(stderr, "%s: %s: %s\n", cmdName,
				    operands[i], gettext("resource busy"));
				ret++;
				break;
			case STMF_ERROR_SERVICE_NOT_FOUND:
				(void) fprintf(stderr, "%s: %s\n", cmdName,
				    gettext("STMF service not found"));
				ret++;
				break;
			case STMF_ERROR_PERM:
				(void) fprintf(stderr, "%s: %s\n", cmdName,
				    gettext("permission denied"));
				ret++;
				break;
			case STMF_ERROR_SERVICE_DATA_VERSION:
				(void) fprintf(stderr, "%s: %s\n", cmdName,
				    gettext("STMF service version incorrect"));
				ret++;
				break;
			case STMF_ERROR_TG_ONLINE:
				(void) fprintf(stderr, "%s: %s\n", cmdName,
				    gettext("STMF target must be offline"));
				ret++;
				break;
			default:
				(void) fprintf(stderr, "%s: %s: %s\n", cmdName,
				    operands[i], gettext("unknown error"));
				ret++;
				break;
		}
	}

	return (ret);
}

/*
 * removeViewFunc
 *
 * Removes one or more view entries from a logical unit
 *
 */
/*ARGSUSED*/
static int
removeViewFunc(int operandLen, char *operands[], cmdOptions_t *options,
    void *args)
{
	char sGuid[GUID_INPUT + 1];
	stmfViewEntryList *viewEntryList;
	stmfGuid inGuid;
	uint32_t count;
	unsigned int guid[sizeof (stmfGuid)];
	char *endPtr;
	uint32_t veNbr;
	int i;
	boolean_t all = B_FALSE;
	boolean_t luInput = B_FALSE;
	int ret = 0;
	int stmfRet;
	stmf_remove_proxy_view_t *proxy_remove_view_entry;
	
	bool_t isCluster = false;

	/* Note: 'l' is required */
	for (; options->optval; options++) {
		switch (options->optval) {
			case 'l':
				if (strlen(options->optarg) != GUID_INPUT) {
					(void) fprintf(stderr,
					    "%s: %s: %s %d %s\n",
					    cmdName, options->optarg,
					    gettext("must be"), GUID_INPUT,
					    gettext("hexadecimal digits long"));
					return (1);
				}
				bcopy(options->optarg, sGuid, GUID_INPUT);
				luInput = B_TRUE;
				break;
			case 'a':
				/* removing all view entries for this GUID */
				all = B_TRUE;
				break;
			case 'c':
				isCluster = true;
				break;
			default:
				(void) fprintf(stderr, "%s: %c: %s\n",
				    cmdName, options->optval,
				    "unknown option");
				return (1);
		}
	}

	if (!all && operandLen == 0) {
		(void) fprintf(stderr, "%s: %s\n", cmdName,
		    gettext("no view entries specified"));
		return (1);
	}

	if (!luInput) {
		(void) fprintf(stderr, "%s: %s\n", cmdName,
		    gettext("logical unit (-l) not specified"));
		return (1);
	}

	for (i = 0; i < 32; i++)
		sGuid[i] = tolower(sGuid[i]);
	sGuid[i] = 0;

	(void) sscanf(sGuid, "%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x",
	    &guid[0], &guid[1], &guid[2], &guid[3], &guid[4], &guid[5],
	    &guid[6], &guid[7], &guid[8], &guid[9], &guid[10], &guid[11],
	    &guid[12], &guid[13], &guid[14], &guid[15]);

	for (i = 0; i < sizeof (stmfGuid); i++) {
		inGuid.guid[i] = guid[i];
	}

	if ((stmfRet = stmfGetViewEntryList(&inGuid, &viewEntryList))
	    != STMF_STATUS_SUCCESS) {

		switch (stmfRet) {
			case STMF_ERROR_BUSY:
				(void) fprintf(stderr, "%s: %s: %s\n", cmdName,
				    sGuid, gettext("resource busy"));
				break;
			case STMF_ERROR_NOT_FOUND:
				(void) fprintf(stderr, "%s: %s: %s\n", cmdName,
				    sGuid, gettext("no views found"));
				break;
			case STMF_ERROR_SERVICE_NOT_FOUND:
				(void) fprintf(stderr, "%s: %s\n", cmdName,
				    gettext("STMF service not found"));
				break;
			case STMF_ERROR_SERVICE_DATA_VERSION:
				(void) fprintf(stderr, "%s: %s\n", cmdName,
				    gettext("STMF service version incorrect"));
				break;
			case STMF_ERROR_PERM:
				(void) fprintf(stderr, "%s: %s\n", cmdName,
				    gettext("permission denied"));
				break;
			default:
				(void) fprintf(stderr, "%s: %s: %s\n", cmdName,
				    sGuid, gettext("unknown error"));
				break;
		}
		return (1);
	}

	if (all) {
		count = viewEntryList->cnt;
	} else {
		count = operandLen;
	}

	for (i = 0; i < count; i++) {
		if (all) {
			veNbr = viewEntryList->ve[i].veIndex;
		} else {
			endPtr = NULL;
			veNbr = strtol(operands[i], &endPtr, 10);
			if (endPtr && *endPtr != 0) {
				(void) fprintf(stderr, "%s: %s: %s\n", cmdName,
				    operands[i], gettext("invalid input"));
				continue;
			}
		}
		stmfRet = stmfRemoveViewEntry(&inGuid, veNbr);
		switch (stmfRet) {
			case STMF_STATUS_SUCCESS:
			proxy_remove_view_entry = malloc(sizeof(stmf_remove_proxy_view_t));
			bzero(proxy_remove_view_entry, sizeof(stmf_remove_proxy_view_t));
			proxy_remove_view_entry->head.op_type = STMF_OP_DELETE;
			proxy_remove_view_entry->head.item_type = STMF_VIEW_OP;
			bcopy(&inGuid, &proxy_remove_view_entry->lu_guid, sizeof(stmfGuid));
			proxy_remove_view_entry->view_index = veNbr;
			free(proxy_remove_view_entry);
			
			if (true == isCluster)
				stmfadm_send_cmd(cmdfullName);
				break;
			case STMF_ERROR_NOT_FOUND:
				(void) fprintf(stderr, "%s: %s: %d: %s\n",
				    cmdName, sGuid, veNbr,
				    gettext("not found"));
				ret++;
				break;
			case STMF_ERROR_BUSY:
				(void) fprintf(stderr, "%s: %s: %s\n", cmdName,
				    sGuid, gettext("resource busy"));
				ret++;
				break;
			case STMF_ERROR_SERVICE_NOT_FOUND:
				(void) fprintf(stderr, "%s: %s\n", cmdName,
				    gettext("STMF service not found"));
				ret++;
				break;
			case STMF_ERROR_CONFIG_NONE:
				(void) fprintf(stderr, "%s: %s\n", cmdName,
				    gettext("STMF service is not initialized"));
				ret++;
				break;
			case STMF_ERROR_SERVICE_DATA_VERSION:
				(void) fprintf(stderr, "%s: %s\n", cmdName,
				    gettext("STMF service version incorrect"));
				ret++;
				break;
			default:
				(void) fprintf(stderr, "%s: %s, %d: %s",
				    cmdName, sGuid, veNbr,
				    gettext("unknown error"));
				ret++;
				break;
		}
	}

	return (ret);
}

static int
clearTraceFunc(int operandLen, char *operands[], cmdOptions_t *options,
    void *args)
{
	return stmfClearTrace();
}

static int
getTraceFunc(int operandLen, char *operands[], cmdOptions_t *options,
    void *args)
{
	return stmfGetTrace();
}

static int
setTaskLimitFunc(int operandLen, char *operands[], cmdOptions_t *options,
    void *args)
{
	int ret, i;
	stmfState state;
	char sGuid[GUID_INPUT + 1] = {0};
	uint32_t limit = 0;
	boolean_t luInput = B_FALSE;
	boolean_t limitInput = B_FALSE;
	boolean_t isCluster = B_FALSE;
	stmfGuid inGuid;
	unsigned int guid[sizeof (stmfGuid)];
	uint32_t stmf_max_cur_task;

	ret = getStmfState(&state);
	if (ret != STMF_STATUS_SUCCESS)
		return (ret);
	if (state.operationalState == STMF_SERVICE_STATE_OFFLINE ||
	    state.operationalState == STMF_SERVICE_STATE_OFFLINING) {
		(void) fprintf(stderr, "%s: %s\n", cmdName,
		    gettext("STMF service is offline"));
		return (1);
	}

	for (; options->optval; options++) {
		switch (options->optval) {
			case 'l':
				if (strlen(options->optarg) != GUID_INPUT) {
					(void) fprintf(stderr,
					    "%s: %s: %s %d %s\n",
					    cmdName, options->optarg,
					    gettext("must be"), GUID_INPUT,
					    gettext("hexadecimal digits long"));
					return (1);
				}
				bcopy(options->optarg, sGuid, GUID_INPUT);
				luInput = B_TRUE;
				break;
			case 'n':
				limit = atoi(options->optarg);
				limitInput = B_TRUE;
				break;
			case 'c':
				isCluster = B_TRUE;
				break;
			default:
				(void) fprintf(stderr, "%s: %c: %s\n",
				    cmdName, options->optval,
				    "unknown option");
				return (1);
		}
	}

	if (!luInput) {
		(void) fprintf(stderr, "%s: %s\n", cmdName,
		    gettext("no lu specified"));
		return (1);
	}

	if (!limitInput) {
		(void) fprintf(stderr, "%s: %s\n", cmdName,
		    gettext("no task limit specified"));
		return (1);
	}

	for (i = 0; i < GUID_INPUT; i++)
		sGuid[i] = tolower(sGuid[i]);
	sGuid[i] = 0;

	(void) sscanf(sGuid, "%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x",
	    &guid[0], &guid[1], &guid[2], &guid[3], &guid[4], &guid[5],
	    &guid[6], &guid[7], &guid[8], &guid[9], &guid[10], &guid[11],
	    &guid[12], &guid[13], &guid[14], &guid[15]);

	for (i = 0; i < sizeof (stmfGuid); i++) {
		inGuid.guid[i] = guid[i];
	}

	ret = stmfSetLuTaskLimit(&inGuid, limit, &stmf_max_cur_task);

	if (ret == STMF_STATUS_SUCCESS) {
		if (isCluster == B_TRUE)
			stmfadm_send_cmd(cmdfullName);
	} else {
		switch (ret) {
		case STMF_ERROR_BUSY:
			(void) fprintf(stderr, "%s: %s\n", cmdName,
			    gettext("STMF is busy"));
			break;
		case STMF_ERROR_PERM:
			(void) fprintf(stderr, "%s: %s\n", cmdName,
			    gettext("permission denied"));
			break;
		case STMF_ERROR_NOT_FOUND:
			(void) fprintf(stderr, "%s: %s: %s\n", cmdName,
			    sGuid, gettext("not found"));
			break;
		case STMF_ERROR_INVALID_TASK_LIMIT:
			(void) fprintf(stderr, "%s: limit %d is invalid, "
				"should in [1, %d]\n",
				cmdName, limit, stmf_max_cur_task);
			break;
		default:
			(void) fprintf(stderr, "%s: %s\n", cmdName,
			    gettext("unknown error"));
			break;
		}
	}

	return (ret);
}


static int
getTaskInfoFunc(int operandLen, char *operands[], cmdOptions_t *options,
    void *args)
{
	int ret, i;
	stmfState state;
	char sGuid[GUID_INPUT + 1] = {0};
	uint32_t cur_task, task_limit;
	stmfGuid inGuid;
	unsigned int guid[sizeof (stmfGuid)];

	ret = getStmfState(&state);
	if (ret != STMF_STATUS_SUCCESS)
		return (ret);
	if (state.operationalState == STMF_SERVICE_STATE_OFFLINE ||
	    state.operationalState == STMF_SERVICE_STATE_OFFLINING) {
		(void) fprintf(stderr, "%s: %s\n", cmdName,
		    gettext("STMF service is offline"));
		return (1);
	}

	if (strlen(operands[0]) != GUID_INPUT) {
		(void) fprintf(stderr,
			"%s: %s: %s %d %s\n",
			cmdName, operands[0],
			gettext("must be"), GUID_INPUT,
			gettext("hexadecimal digits long"));
		return (1);
	}

	bcopy(operands[0], sGuid, GUID_INPUT);

	for (i = 0; i < GUID_INPUT; i++)
		sGuid[i] = tolower(sGuid[i]);
	sGuid[i] = 0;

	(void) sscanf(sGuid, "%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x",
	    &guid[0], &guid[1], &guid[2], &guid[3], &guid[4], &guid[5],
	    &guid[6], &guid[7], &guid[8], &guid[9], &guid[10], &guid[11],
	    &guid[12], &guid[13], &guid[14], &guid[15]);

	for (i = 0; i < sizeof (stmfGuid); i++) {
		inGuid.guid[i] = guid[i];
	}

	ret = stmfGetLuTaskInfo(&inGuid, &cur_task, &task_limit);

	if (ret == STMF_STATUS_SUCCESS) {
		printf("cur task: %d, task limit: %d\n", cur_task, task_limit);
	} else {
		switch (ret) {
		case STMF_ERROR_BUSY:
			(void) fprintf(stderr, "%s: %s\n", cmdName,
			    gettext("STMF is busy"));
			break;
		case STMF_ERROR_PERM:
			(void) fprintf(stderr, "%s: %s\n", cmdName,
			    gettext("permission denied"));
			break;
		case STMF_ERROR_NOT_FOUND:
			(void) fprintf(stderr, "%s: %s: %s\n", cmdName,
			    sGuid, gettext("not found"));
			break;
		default:
			(void) fprintf(stderr, "%s: %s\n", cmdName,
			    gettext("unknown error"));
			break;
		}
	}

	return (ret);
}

static int
setIopsLimitFunc(int operandLen, char *operands[], cmdOptions_t *options,
    void *args)
{
	int ret, i;
	stmfState state;
	char sGuid[GUID_INPUT + 1] = {0};
	uint32_t iopsLimit = 0;
	boolean_t luInput = B_FALSE;
	boolean_t iopsInput = B_FALSE;
	boolean_t isCluster = B_FALSE;
	stmfGuid inGuid;
	unsigned int guid[sizeof (stmfGuid)];

	ret = getStmfState(&state);
	if (ret != STMF_STATUS_SUCCESS)
		return (ret);
	if (state.operationalState == STMF_SERVICE_STATE_OFFLINE ||
	    state.operationalState == STMF_SERVICE_STATE_OFFLINING) {
		(void) fprintf(stderr, "%s: %s\n", cmdName,
		    gettext("STMF service is offline"));
		return (1);
	}

	for (; options->optval; options++) {
		switch (options->optval) {
			case 'l':
				if (strlen(options->optarg) != GUID_INPUT) {
					(void) fprintf(stderr,
					    "%s: %s: %s %d %s\n",
					    cmdName, options->optarg,
					    gettext("must be"), GUID_INPUT,
					    gettext("hexadecimal digits long"));
					return (1);
				}
				bcopy(options->optarg, sGuid, GUID_INPUT);
				luInput = B_TRUE;
				break;
			case 'n':
				iopsLimit = atoi(options->optarg);
				iopsInput = B_TRUE;
				break;
			case 'c':
				isCluster = B_TRUE;
				break;
			default:
				(void) fprintf(stderr, "%s: %c: %s\n",
				    cmdName, options->optval,
				    "unknown option");
				return (1);
		}
	}

	if (!luInput) {
		(void) fprintf(stderr, "%s: %s\n", cmdName,
		    gettext("no lu specified"));
		return (1);
	}

	if (!iopsInput) {
		(void) fprintf(stderr, "%s: %s\n", cmdName,
		    gettext("no iops limit specified"));
		return (1);
	}

	for (i = 0; i < GUID_INPUT; i++)
		sGuid[i] = tolower(sGuid[i]);
	sGuid[i] = 0;

	(void) sscanf(sGuid, "%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x",
	    &guid[0], &guid[1], &guid[2], &guid[3], &guid[4], &guid[5],
	    &guid[6], &guid[7], &guid[8], &guid[9], &guid[10], &guid[11],
	    &guid[12], &guid[13], &guid[14], &guid[15]);

	for (i = 0; i < sizeof (stmfGuid); i++) {
		inGuid.guid[i] = guid[i];
	}

	ret = stmfSetIopsLimit(&inGuid, iopsLimit);

	if (ret == STMF_STATUS_SUCCESS) {
		if (isCluster == B_TRUE)
			stmfadm_send_cmd(cmdfullName);
	} else {
		switch (ret) {
		case STMF_ERROR_BUSY:
			(void) fprintf(stderr, "%s: %s\n", cmdName,
			    gettext("STMF is busy"));
			break;
		case STMF_ERROR_PERM:
			(void) fprintf(stderr, "%s: %s\n", cmdName,
			    gettext("permission denied"));
			break;
		case STMF_ERROR_NOT_FOUND:
			(void) fprintf(stderr, "%s: %s: %s\n", cmdName,
			    sGuid, gettext("not found"));
			break;
		case STMF_ERROR_INVALID_IOPS_LIMIT:
			(void) fprintf(stderr, "%s: iops limit %d is invalid",
				cmdName, iopsLimit);
			break;
		default:
			(void) fprintf(stderr, "%s: %s\n", cmdName,
			    gettext("unknown error"));
			break;
		}
	}

	return (ret);
}

static int
getIopsInfoFunc(int operandLen, char *operands[], cmdOptions_t *options,
    void *args)
{
	int ret, i;
	stmfState state;
	char sGuid[GUID_INPUT + 1] = {0};
	uint32_t cur_iops, iops_limit;
	stmfGuid inGuid;
	unsigned int guid[sizeof (stmfGuid)];

	ret = getStmfState(&state);
	if (ret != STMF_STATUS_SUCCESS)
		return (ret);
	if (state.operationalState == STMF_SERVICE_STATE_OFFLINE ||
	    state.operationalState == STMF_SERVICE_STATE_OFFLINING) {
		(void) fprintf(stderr, "%s: %s\n", cmdName,
		    gettext("STMF service is offline"));
		return (1);
	}

	if (strlen(operands[0]) != GUID_INPUT) {
		(void) fprintf(stderr,
			"%s: %s: %s %d %s\n",
			cmdName, operands[0],
			gettext("must be"), GUID_INPUT,
			gettext("hexadecimal digits long"));
		return (1);
	}

	bcopy(operands[0], sGuid, GUID_INPUT);

	for (i = 0; i < GUID_INPUT; i++)
		sGuid[i] = tolower(sGuid[i]);
	sGuid[i] = 0;

	(void) sscanf(sGuid, "%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x",
	    &guid[0], &guid[1], &guid[2], &guid[3], &guid[4], &guid[5],
	    &guid[6], &guid[7], &guid[8], &guid[9], &guid[10], &guid[11],
	    &guid[12], &guid[13], &guid[14], &guid[15]);

	for (i = 0; i < sizeof (stmfGuid); i++) {
		inGuid.guid[i] = guid[i];
	}

	ret = stmfGetIopsInfo(&inGuid, &cur_iops, &iops_limit);

	if (ret == STMF_STATUS_SUCCESS) {
		printf("cur iops: %d, iops limit: %d\n", cur_iops, iops_limit);
	} else {
		switch (ret) {
		case STMF_ERROR_BUSY:
			(void) fprintf(stderr, "%s: %s\n", cmdName,
			    gettext("STMF is busy"));
			break;
		case STMF_ERROR_PERM:
			(void) fprintf(stderr, "%s: %s\n", cmdName,
			    gettext("permission denied"));
			break;
		case STMF_ERROR_NOT_FOUND:
			(void) fprintf(stderr, "%s: %s: %s\n", cmdName,
			    sGuid, gettext("not found"));
			break;
		default:
			(void) fprintf(stderr, "%s: %s\n", cmdName,
			    gettext("unknown error"));
			break;
		}
	}

	return (ret);
}

/*
 * listAllLunsFunc
 *
 * List name and guid of the logical units
 *
 */
/*ARGSUSED*/
static int
listAllLunsFunc(int operandLen, char *operands[], cmdOptions_t *options, void *args)
{
    int i;
    int ret = 0;
    stmfLunsList *luList;
    
    if ((ret = stmfListAllLuns(&luList))
        != STMF_STATUS_SUCCESS) {
        switch (ret) {
            case STMF_ERROR_SERVICE_NOT_FOUND:
                (void) fprintf(stderr, "%s: %s\n", cmdName,
                    gettext("STMF service not found"));
                break;
            case STMF_ERROR_BUSY:
                (void) fprintf(stderr, "%s: %s\n", cmdName,
                    gettext("resource busy"));
                break;
            case STMF_ERROR_PERM:
                (void) fprintf(stderr, "%s: %s\n", cmdName,
                    gettext("permission denied"));
                break;
            case STMF_ERROR_SERVICE_DATA_VERSION:
                (void) fprintf(stderr, "%s: %s\n", cmdName,
                    gettext("STMF service version incorrect"));
                break;
            default:
                (void) fprintf(stderr, "%s: %s\n", cmdName,
                    gettext("list failed"));
                break;
        }
        return (1);
    }

    for (i = 0; i < luList->cnt; i++) {
        printGuid(&luList->luns[i].guid, stdout);
        (void) printf("\t");
        (void) printf("%s", luList->luns[i].name);
        (void) printf("\n");
    }

    return (ret);
}

static int
setKbpsFunc(int operandLen, char *operands[], cmdOptions_t *options,
    void *args)
{
	int ret, i;
	char *ptr = NULL;
	stmfState state;
	unsigned long convert = 0;
	char sGuid[GUID_INPUT + 1] = {0};
	uint64_t kbps_limit = 0;
	boolean_t luInput = B_FALSE;
	boolean_t kbpsInput = B_FALSE;
	boolean_t isCluster = B_FALSE;
	stmfGuid inGuid;
	unsigned int guid[sizeof (stmfGuid)];

	ret = getStmfState(&state);
	if (ret != STMF_STATUS_SUCCESS)
		return (ret);
	if (state.operationalState == STMF_SERVICE_STATE_OFFLINE ||
	    state.operationalState == STMF_SERVICE_STATE_OFFLINING) {
		(void) fprintf(stderr, "%s: %s\n", cmdName,
		    gettext("STMF service is offline"));
		return (1);
	}

	for (; options->optval; options++) {
		switch (options->optval) {
			case 'l':
				if (strlen(options->optarg) != GUID_INPUT) {
					(void) fprintf(stderr,
					    "%s: %s: %s %d %s\n",
					    cmdName, options->optarg,
					    gettext("must be"), GUID_INPUT,
					    gettext("hexadecimal digits long"));
					return (1);
				}
				bcopy(options->optarg, sGuid, GUID_INPUT);
				luInput = B_TRUE;
				break;
			case 'n':
				if (strchr(options->optarg, 'g') != NULL ||
						strchr(options->optarg, 'G') != NULL) {
					convert = strtoul(options->optarg, &ptr, 10);
					kbps_limit = convert * KBPS_G;
				} else if (strchr(options->optarg, 'm') != NULL ||
						strchr(options->optarg, 'M') != NULL) {
					convert = strtoul(options->optarg, &ptr, 10);
					kbps_limit = convert * KBPS_M;
				} else {
					convert = strtoul(options->optarg, &ptr, 10);
					kbps_limit = convert * KBPS_K;
				}
				kbpsInput = B_TRUE;
				break;
			case 'c':
				isCluster = B_TRUE;
				break;
			default:
				(void) fprintf(stderr, "%s: %c: %s\n",
				    cmdName, options->optval,
				    "unknown option");
				return (1);
		}
	}

	if (!luInput) {
		(void) fprintf(stderr, "%s: %s\n", cmdName,
		    gettext("no lu specified"));
		return (1);
	}

	if (!kbpsInput) {
		(void) fprintf(stderr, "%s: %s\n", cmdName,
		    gettext("no iops limit specified"));
		return (1);
	}

	for (i = 0; i < GUID_INPUT; i++)
		sGuid[i] = tolower(sGuid[i]);
	sGuid[i] = 0;

	(void) sscanf(sGuid, "%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x",
	    &guid[0], &guid[1], &guid[2], &guid[3], &guid[4], &guid[5],
	    &guid[6], &guid[7], &guid[8], &guid[9], &guid[10], &guid[11],
	    &guid[12], &guid[13], &guid[14], &guid[15]);

	for (i = 0; i < sizeof (stmfGuid); i++) {
		inGuid.guid[i] = guid[i];
	}

	ret = stmf_set_kbps(&inGuid, kbps_limit);

	if (ret == STMF_STATUS_SUCCESS) {
		if (isCluster == B_TRUE)
			stmfadm_send_cmd(cmdfullName);
	} else {
		switch (ret) {
		case STMF_ERROR_BUSY:
			(void) fprintf(stderr, "%s: %s\n", cmdName,
			    gettext("STMF is busy"));
			break;
		case STMF_ERROR_PERM:
			(void) fprintf(stderr, "%s: %s\n", cmdName,
			    gettext("permission denied"));
			break;
		case STMF_ERROR_NOT_FOUND:
			(void) fprintf(stderr, "%s: %s: %s\n", cmdName,
			    sGuid, gettext("not found"));
			break;
		case STMF_ERROR_INVALID_IOPS_LIMIT:
			(void) fprintf(stderr, "%s: kbps limit %ld is invalid",
				cmdName, kbps_limit);
			break;
		default:
			(void) fprintf(stderr, "%s: %s\n", cmdName,
			    gettext("unknown error"));
			break;
		}
	}

	return (ret);
}

static int
getKbpsFunc(int operandLen, char *operands[], cmdOptions_t *options,
    void *args)
{
	int ret, i;
	stmfGuid inGuid;
	stmfState state;
	char skbps[16] = {0};
	char skbps_limit[16] = {0};
	char sGuid[GUID_INPUT + 1] = {0};
	uint64_t kbps_limit = 0, cur_kbps = 0;
	unsigned int guid[sizeof (stmfGuid)];

	ret = getStmfState(&state);
	if (ret != STMF_STATUS_SUCCESS)
		return (ret);
	if (state.operationalState == STMF_SERVICE_STATE_OFFLINE ||
	    state.operationalState == STMF_SERVICE_STATE_OFFLINING) {
		(void) fprintf(stderr, "%s: %s\n", cmdName,
		    gettext("STMF service is offline"));
		return (1);
	}

	if (strlen(operands[0]) != GUID_INPUT) {
		(void) fprintf(stderr,
			"%s: %s: %s %d %s\n",
			cmdName, operands[0],
			gettext("must be"), GUID_INPUT,
			gettext("hexadecimal digits long"));
		return (1);
	}

	bcopy(operands[0], sGuid, GUID_INPUT);

	for (i = 0; i < GUID_INPUT; i++)
		sGuid[i] = tolower(sGuid[i]);
	sGuid[i] = 0;

	(void) sscanf(sGuid, "%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x",
	    &guid[0], &guid[1], &guid[2], &guid[3], &guid[4], &guid[5],
	    &guid[6], &guid[7], &guid[8], &guid[9], &guid[10], &guid[11],
	    &guid[12], &guid[13], &guid[14], &guid[15]);

	for (i = 0; i < sizeof (stmfGuid); i++) {
		inGuid.guid[i] = guid[i];
	}

	ret = stmf_get_kbps(&inGuid, &cur_kbps, &kbps_limit);

	if (ret == STMF_STATUS_SUCCESS) {
		if (cur_kbps / KBPS_G) {
			snprintf(skbps, sizeof(skbps), "%lluG", 
                (unsigned long long)cur_kbps / KBPS_G);
		} else if (cur_kbps / KBPS_M) {
			snprintf(skbps, sizeof(skbps), "%lluM", 
                (unsigned long long)cur_kbps / KBPS_M);
		} else {
			snprintf(skbps, sizeof(skbps), "%lluK", 
                (unsigned long long)cur_kbps / KBPS_K);
		}

		if (kbps_limit / KBPS_G) {
			snprintf(skbps_limit, sizeof(skbps_limit), "%lluG", 
                (unsigned long long)kbps_limit / KBPS_G);
		} else if (kbps_limit / KBPS_M) {
			snprintf(skbps_limit, sizeof(skbps_limit), "%lluM", 
                (unsigned long long)kbps_limit / KBPS_M);
		} else {
			snprintf(skbps_limit, sizeof(skbps_limit), "%lluK", 
                (unsigned long long)kbps_limit / KBPS_K);
		}
		printf("cur kbps: %s, kbps limit: %s\n", skbps, skbps_limit);
	} else {
		switch (ret) {
		case STMF_ERROR_BUSY:
			(void) fprintf(stderr, "%s: %s\n", cmdName,
			    gettext("STMF is busy"));
			break;
		case STMF_ERROR_PERM:
			(void) fprintf(stderr, "%s: %s\n", cmdName,
			    gettext("permission denied"));
			break;
		case STMF_ERROR_NOT_FOUND:
			(void) fprintf(stderr, "%s: %s: %s\n", cmdName,
			    sGuid, gettext("not found"));
			break;
		default:
			(void) fprintf(stderr, "%s: %s\n", cmdName,
			    gettext("unknown error"));
			break;
		}
	}

	return (ret);
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
	int i = 0;
	char *tmp_p = NULL;
	char *tmp_str = NULL;
	char tmp_argv[512] = "\0";
	

	(void) setlocale(LC_ALL, "");
	(void) textdomain(TEXT_DOMAIN);

	bzero(cmdfullName,512);
	for (i = 0; i <argc;i++)
	{
		strcpy(tmp_argv,argv[i]);
		if ('-' == tmp_argv[0])
		{
			if (NULL != strstr(tmp_argv,"c"))
			{
				/*delete 'C' */
				tmp_str=tmp_p=tmp_argv;
				while (*tmp_p)
				{
					if(*tmp_p != 'c')
						*tmp_str++ = *tmp_p;
					tmp_p++;
				}
				*tmp_str = '\0';
				
				/*delete "-"*/
				if (strcmp("-",tmp_argv) == 0)
				{
					bzero(tmp_argv,512);
				}						
			}
		}	
		//sprintf(cmdfullName,"%s %s",cmdfullName,tmp_argv);
		snprintf((cmdfullName)+strlen(cmdfullName),sizeof(cmdfullName)-strlen(cmdfullName)," %s",tmp_argv);
	}
	
	/* set global command name */
	cmdName = getExecBasename(argv[0]);

	(void) snprintf(versionString, VERSION_STRING_MAX_LEN, "%s.%s",
	    VERSION_STRING_MAJOR, VERSION_STRING_MINOR);
	synTables.versionString = versionString;
	synTables.longOptionTbl = &longOptions[0];
	synTables.subCommandPropsTbl = &subcommands[0];
#if 0
	stmf_proxy_door_fd = open(STMF_PROXY_DOOR_SVC, O_RDONLY);
	if (stmf_proxy_door_fd  <= 0) {
		syslog(LOG_ERR, "Open stmf proxy door service fails error =%d", errno);
		exit(1);
	}
	syslog(LOG_ERR, "door_fd1 = %d", stmf_proxy_door_fd);
#endif

	ret = cmdParse(argc, argv, synTables, subcommandArgs, &funcRet);
	if (ret != 0) {
		return (ret);
	}

#if 0
	close(stmf_proxy_door_fd);
#endif
	return (funcRet);
} /* end main */
