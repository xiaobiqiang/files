#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <errno.h>
#include <syslog.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys/vfs.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <dirent.h>
#include <locale.h>

#include <libintl.h>

#define MPT3FAULTSIMU   _IOWR(MPT3_MAGIC_NUMBER, 33, \
	struct mpt3_fault_simu_info)


#define SCSIHOST_DIR "/sys/class/scsi_host"
#define MPT3CTL_DEV  "/dev/mpt3ctl"

typedef enum mpt_type {
    MPT2SAS,
    MPT3SAS
}mpt_type_e;


typedef struct mpt_ioc {
    int ioc_id;
    uint32_t ioc_dev_num;
    mpt_type_e ioc_type;
    int ioc_enabled;
}mpt_ioc_t;

/**
 * struct mpt3_ioctl_header - main header structure
 * @ioc_number -  IOC unit number
 * @port_number - IOC port number
 * @max_data_size - maximum number bytes to transfer on read
 */
struct mpt3_ioctl_header {
	unsigned int ioc_number;
	unsigned int port_number;
	unsigned int max_data_size;
};
#define MPT3_MAGIC_NUMBER	'L'

struct mpt3_fault_simu_info {
    struct mpt3_ioctl_header hdr;
    unsigned int subcmd;
    unsigned int action;
    unsigned long long wwn;
    void *   priv_data;
};

typedef enum {
    MPT3_NORESP,
    MPT3_MERR,     
} mpt3_simu_subcmd_e;

typedef struct mpt3_simu_cmd {
	const char	*sub_name;
	int		    (*handle)(int, char **);
	mpt3_simu_subcmd_e   sub_code;
} mpt3_simu_cmd_t;

typedef enum {
    mpt3simu_on = 0,
    mpt3simu_off,
    mpt3simu_repair
} mpt3simu_action_e;

static int mpt3ctl_noresponse(int argc, char **argv);
static int mpt3ctl_merr(int argc, char **argv);


static mpt3_simu_cmd_t subcmd_table[] = {
	{ "noresponse",	mpt3ctl_noresponse,	MPT3_NORESP		},
	{ "merr",	mpt3ctl_merr,	MPT3_MERR		},
    {NULL},
};

#define	NSUBCMDS	(sizeof (subcmd_table) / sizeof (subcmd_table[0]))
int current_subcmd_idx;

static int get_subcmd_idx(char *subcmd, int *idx)
{
	int i;

	for (i = 0; i < NSUBCMDS; i++) {
		if (subcmd_table[i].sub_name == NULL)
			continue;

		if (strcmp(subcmd, subcmd_table[i].sub_name) == 0) {
			*idx = i;
			return (0);
		}
	}
	return -1;
}

static const char *usage_info(mpt3_simu_subcmd_e subcode) {
    switch (subcode) {
    case MPT3_NORESP:
    	return (gettext("\tnoresponse <on| off | repair> <disk-path>\n"));

    case MPT3_MERR:
    	return (gettext("\tmerr <on| off | repair> <disk-path>\n"));
    }

    abort();
}

static void usage(mpt3_simu_cmd_t *subcmd)
{
	FILE *fp = stderr;

	if (subcmd == NULL) {
		int i;

		(void) fprintf(fp, gettext("usage: mpt3ctl command args ...\n"));
		(void) fprintf(fp,
		    gettext("where 'command' is one of the following:\n\n"));

		for (i = 0; i < NSUBCMDS; i++) {
			if (subcmd_table[i].sub_name == NULL) {
				//(void) fprintf(fp, "\n");
				continue;
            }
			else {
				(void) fprintf(fp, "%s", usage_info(subcmd_table[i].sub_code)); 
            }
		}
	} else {
		(void) fprintf(fp, gettext("usage:\n"));
		(void) fprintf(fp, "%s", usage_info(subcmd->sub_code));
	}

}


int main(int argc, char **argv) 
{
    char *subcmd;
    int  i = 0;
    int ret = -1;
    mpt3_simu_cmd_t *cmd = NULL;
    
	(void) setlocale(LC_ALL, "");
	(void) textdomain(TEXT_DOMAIN);
    
    if (argc < 2) {
        (void) fprintf(stderr, gettext("missing command\n"));
        usage(NULL);
        return ret;
    }

    subcmd = argv[1];
    if(get_subcmd_idx(subcmd, &i) == 0) {
        current_subcmd_idx = i;
        cmd = &subcmd_table[i];
        ret = cmd->handle(argc - 1, argv + 1);
    } else {
        usage(NULL);
    }

    return ret;
}

static int find_mpt_host(mpt_ioc_t **ioc_ids, int *ioc_ids_nr)
{
    DIR *dir;
    struct dirent *dirent;
    int fd = -1;
    ssize_t ret = -1;
    mpt_ioc_t *ids = NULL;
    int ids_idx = 0;
    int ids_sz = 2;

    ids = malloc(sizeof(*ids) * ids_sz);
    if (!ids)
        return -1;

    memset(ids, 0, sizeof(*ids) * ids_sz);

    dir = opendir(SCSIHOST_DIR);
    if (!dir) {
        free(ids);
        return -1;
    }

    while ( (dirent = readdir(dir)) != NULL ) {
        char filename[512];
        char procname[8];
        char dev_num[8];

        snprintf(filename, sizeof(filename), "%s/%s/proc_name", SCSIHOST_DIR, dirent->d_name);

        fd = open(filename, O_RDONLY);
        if (fd < 0)
            continue;

        ret = read(fd, procname, 8);
        if (ret < 0)
            continue;

        close(fd);

        procname[7] = '\0';

        if (strncmp("mpt3sas", procname, 7) == 0) {
            ids[ids_idx].ioc_type = MPT3SAS;
        } else if (strncmp("mpt2sas", procname, 7) == 0) {
            ids[ids_idx].ioc_type = MPT2SAS;
        } else {
            continue;
        }

        /* fetch scsi host uniqeu id. */
        snprintf(filename, sizeof(filename), "%s/%s/unique_id", SCSIHOST_DIR, dirent->d_name);

        fd = open(filename, O_RDONLY);
        if (fd < 0) {
            syslog(LOG_ERR, "open file :%s failed", filename);
            continue;
        }

        ret = read(fd, procname, 8);
        if (ret < 0) {
            syslog(LOG_ERR, "read failed, %s:%d", __FILE__, __LINE__);
            continue;
        }

        close(fd);

        /* fetch scsi host sas device number. */
        snprintf(filename, sizeof(filename), "%s/%s/host_sas_dev_cnt", SCSIHOST_DIR, dirent->d_name);

        fd = open(filename, O_RDONLY);
        if (fd < 0) {
            syslog(LOG_ERR, "open file :%s failed", filename);
            continue;
        }

        ret = read(fd, dev_num, 8);
        if (ret < 0)
            continue;

        close(fd);

        if (ids_idx == ids_sz)
        {
            ids_sz *= 2;
            ids = realloc(ids, sizeof(*ids) * ids_sz);
            if (!ids)
                break;

            ids[ids_idx].ioc_id = atoi(procname);
            ids[ids_idx].ioc_dev_num = atoi(dev_num);
            syslog(LOG_INFO, "Found MPT ioc %d type %d",
                    ids[ids_idx].ioc_id, ids[ids_idx].ioc_type);
            ids_idx += 1;
        }
        else
        {
            ids[ids_idx].ioc_id = atoi(procname);
            ids[ids_idx].ioc_dev_num = atoi(dev_num);
            syslog(LOG_INFO, "Found MPT ioc %d type %d",
                    ids[ids_idx].ioc_id, ids[ids_idx].ioc_type);
            ids_idx += 1;
        }
    }

    closedir(dir);

    *ioc_ids = ids;
    *ioc_ids_nr = ids_idx;

    return 0;
}

static int do_mpt3ctl_simu(mpt3_simu_subcmd_e subcmd, mpt3simu_action_e action, unsigned long long wwn, void *data) 
{
    int ret;
    mpt_ioc_t *ids = NULL;
    int ids_nr = 0;
    int idx = 0;

    int fd = open(MPT3CTL_DEV, O_RDWR);
	if (fd < 0) {
		syslog(LOG_ERR, "Failed to open mpt device %s: %d (%m)", MPT3CTL_DEV, errno);
		return -1;
	}
    
    ret = find_mpt_host(&ids, &ids_nr);
    if (ret < 0) {
		syslog(LOG_WARNING, "no mpt host found");
        return -1;
    }
    if (ids_nr == 0) {
        free(ids);
        syslog(LOG_WARNING, "Not found any supported MPT controller");
        return -1;
    }

    /* ids_nr usually equls 1.*/
	for (idx = 0; idx < ids_nr; idx++) 
    {
    	struct mpt3_fault_simu_info *cmd;
    	int ret;

        cmd = malloc(sizeof(struct mpt3_fault_simu_info));
        if(NULL == cmd) {
            syslog(LOG_ERR, "malloc failed for mpt device: %d, malloc size:%lu", 
                    ids[idx].ioc_id, sizeof(struct mpt3_fault_simu_info));
            continue;
        }

    	memset(cmd, 0, sizeof(struct mpt3_fault_simu_info));
    	cmd->hdr.ioc_number = ids[idx].ioc_id;   // we only support mpt3sas.
    	cmd->hdr.port_number = 0;
        cmd->subcmd = subcmd;
        cmd->action = action;
        cmd->wwn = wwn;
        cmd->priv_data = data;

    	ret = ioctl(fd, MPT3FAULTSIMU, cmd);
    	if (ret == 0) {
            syslog(LOG_ERR, "simulate for disk:%llx on mpt ioc:%d", wwn, cmd->hdr.ioc_number);
            break;
        } else {
            syslog(LOG_ERR, "disk:%llx NOT found on mpt ioc:%d", wwn, cmd->hdr.ioc_number); // NOT reached here.
        }

        free(cmd);
        cmd = NULL;
    }

    free(ids);
    return ret;
}

static int mpt3ctl_merr(int argc, char **argv)
{
    mpt3simu_action_e action;
    
    char *wwn_key = "scsi-3";
    char *wwn_pos = NULL;
    char *dev_path;
    unsigned long long wwn;

    if (argc < 3) {
        usage(&subcmd_table[current_subcmd_idx]);
    }

    if(strcmp(argv[1], "on") == 0) {
        action = mpt3simu_on;
    } else if(strcmp(argv[1], "off") == 0) {
        action = mpt3simu_off;
    } else if(strcmp(argv[1], "repair") == 0) {
        action = mpt3simu_repair;
    } else {
        goto help_and_exit;
    }

    dev_path = argv[2];

    wwn_pos = strstr(dev_path, wwn_key);
    if(wwn_pos == NULL) {
        (void) fprintf(stderr, gettext("dev path invalid:%s\n"), dev_path);
        goto help_and_exit;
    }

    wwn_pos += strlen(wwn_key);
    if(sscanf(wwn_pos, "%llx", &wwn) == 0) {        
        (void) fprintf(stderr, gettext("disk wwn invalid:%s\n"), wwn_pos);
        goto help_and_exit;
    }

    return do_mpt3ctl_simu(subcmd_table[current_subcmd_idx].sub_code, action, wwn, NULL);

help_and_exit:
    usage(&subcmd_table[current_subcmd_idx]);
    return -1;
}


static int mpt3ctl_noresponse(int argc, char **argv)
{
    mpt3simu_action_e action;
    
    char *wwn_key = "scsi-3";
    char *wwn_pos = NULL;
    char *dev_path;
    unsigned long long wwn;

    if (argc < 3) {
        usage(&subcmd_table[current_subcmd_idx]);
	}

    if(strcmp(argv[1], "on") == 0) {
        action = mpt3simu_on;
    } else if(strcmp(argv[1], "off") == 0) {
        action = mpt3simu_off;
    } else if(strcmp(argv[1], "repair") == 0) {
        action = mpt3simu_repair;
    } else {
        goto help_and_exit;
    }

    dev_path = argv[2];

    wwn_pos = strstr(dev_path, wwn_key);
    if(wwn_pos == NULL) {
        (void) fprintf(stderr, gettext("dev path invalid:%s\n"), dev_path);
        goto help_and_exit;
    }

    wwn_pos += strlen(wwn_key);
    if(sscanf(wwn_pos, "%llx", &wwn) == 0) {        
        (void) fprintf(stderr, gettext("disk wwn invalid:%s\n"), wwn_pos);
        goto help_and_exit;
    }

    return do_mpt3ctl_simu(subcmd_table[current_subcmd_idx].sub_code, action, wwn, NULL);

help_and_exit:
    usage(&subcmd_table[current_subcmd_idx]);
    return -1;
}
