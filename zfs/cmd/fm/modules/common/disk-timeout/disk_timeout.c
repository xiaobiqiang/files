/*
 * This file and its contents are supplied under the terms of the
 * Common Development and Distribution License ("CDDL"), version 1.0.
 * You may only use this file in accordance with the terms of version
 * 1.0 of the CDDL.
 *
 * A full copy of the text of the CDDL should have accompanied this
 * source.  A copy of the CDDL is also available via the Internet at
 * http://www.illumos.org/license/CDDL.
 */

/*
 * Copyright 2014 Nexenta Systems, Inc.  All rights reserved.
 */

#include <fmd_api.h>
#include <sys/note.h>
#include <libtopo.h>
#include <sys/protocol.h>
#include <strings.h>
#include <topo_hc.h>
#include <topo_list.h>

typedef struct disk_io_stat {
	fmd_stat_t bad_fmri;
	fmd_stat_t bad_scheme;
} disk_io_stat_t;

typedef struct disk_timeout {
	char *compare_path;
} disk_timeout_t;

slow_io_stat_t disk_io_stats = {
	{ "bad_FMRI", FMD_TYPE_UINT64,
		"event FMRI is missing or invalid" },
	{ "bad_scheme", FMD_TYPE_UINT64,
		"event does not contain a valid detector"},
};

static const fmd_prop_t fmd_props [] = {
    { "busy_threshhold", FMD_TYPE_INT32, "60" },
    { "stat_sec", FMD_TYPE_INT32, "1" },
    { "stat_cnt", FMD_TYPE_INT32, "2" },
    { "io_N", FMD_TYPE_INT32, "16" },
	{ "io_T", FMD_TYPE_TIME, "5min" },
	{ "io_check_onoff", FMD_TYPE_BOOL, "true" },
	{ NULL, 0, NULL }
};

typedef struct topo_node_info {
	const char *device;
	nvlist_t *fru;
	nvlist_t *resource;
} topo_node_info_t;

void
timeout_io_close(fmd_hdl_t *hdl, fmd_case_t *c)
{
	char *devid = fmd_case_getspecific(hdl, c);
	if (devid != NULL) {
		fmd_hdl_debug(hdl, "Destroying serd: %s", devid);
		fmd_serd_destroy(hdl, devid);
	}
}

int
topo_walk_cb(topo_hdl_t *thp, tnode_t *tn, void *arg) {

	topo_node_info_t *node = (topo_node_info_t *)arg;
	char *cur_devid;
	nvlist_t *fru;
	nvlist_t *resource;
	int err = 0;
	_NOTE(ARGUNUSED(thp));

	if (strcmp(topo_node_name(tn), "disk") != 0)
		return (TOPO_WALK_NEXT);

	if (topo_prop_get_string(tn, "io", "devid", &cur_devid, &err) != 0)
		return (TOPO_WALK_NEXT);

	if (strcmp(cur_devid, node->device) == 0) {
		(void) topo_node_fru(tn, &fru, NULL, &err);
		(void) topo_node_resource(tn, &resource, &err);

		if (err == 0) {
			if (nvlist_dup(fru, &node->fru, 0) != 0 ||
				nvlist_dup(resource, &node->resource, 0) != 0) {
				nvlist_free(resource);
				nvlist_free(fru);
				topo_hdl_strfree(thp, cur_devid);
				return (TOPO_WALK_ERR);
			}
			
			nvlist_free(resource);
			nvlist_free(fru);
			topo_hdl_strfree(thp, cur_devid);
			return (TOPO_WALK_TERMINATE);
		}
		nvlist_free(resource);
		nvlist_free(fru);
	}
	topo_hdl_strfree(thp, cur_devid);

	return (TOPO_WALK_NEXT);
}

topo_node_info_t *
topo_node_lookup_by_devid(fmd_hdl_t *hdl, char *device) {

	int err = 0;
	topo_hdl_t *thp;
	topo_walk_t *twp;

	topo_node_info_t *node = (topo_node_info_t *)fmd_hdl_zalloc(hdl,
	    sizeof (topo_node_info_t), FMD_SLEEP);
	if (node == NULL)
		return (NULL);

	thp = fmd_hdl_topo_hold(hdl, TOPO_VERSION);

	node->device = device;

	if ((twp = topo_walk_init(thp, FM_FMRI_SCHEME_HC, topo_walk_cb,
	    node, &err)) == NULL) {
		fmd_hdl_error(hdl, "failed to get topology: %s",
		    topo_strerror(err));
		fmd_hdl_topo_rele(hdl, thp);
		fmd_hdl_free(hdl, node, sizeof (topo_node_info_t));
		return (NULL);
	}

	(void) topo_walk_step(twp, TOPO_WALK_CHILD);
	if (twp != NULL)
		topo_walk_fini(twp);
	if (thp != NULL)
		fmd_hdl_topo_rele(hdl, thp);

	if (node->fru == NULL || node->resource == NULL) {
		fmd_hdl_debug(hdl, "Could not find device and its FRU");
		fmd_hdl_free(hdl, node, sizeof (topo_node_info_t));
		return (NULL);
	} else {
		fmd_hdl_debug(hdl, "Found FRU for device %s", device);
		return (node);
	}
}

/*
 * Function	:
 *	when find the repair node, then change is_slow to "no"
 */
static int find_repair_falty_disk(topo_hdl_t *thp, tnode_t *node, void *arg)
{
	int err;
	disk_timeout_t *disk_timeout = arg;
	tnode_t *child = NULL;
	char *devpath = NULL;

	if ((strcmp(topo_node_name(node), "bay") == 0) &&
		((child = topo_child_first(node)) != NULL) &&
		(strncmp(topo_node_name(child), "disk", 4) == 0) &&
		(topo_prop_get_string(child, TOPO_PGROUP_IO,
			TOPO_IO_DEVID, &devpath, &err) == 0)) {
		if (strcmp(disk_timeout->compare_path, devpath) == 0) {

			/* set the disk is  slow */
			if (topo_prop_set_string(child, TOPO_PGROUP_IO,
				TOPO_IO_TIMEOUT, TOPO_PROP_MUTABLE, "yes", &err) != 0) {
				syslog(LOG_ERR, "change is_timeout status fail");
			}
		}
		topo_hdl_strfree(thp, devpath);
	}
	return (TOPO_WALK_NEXT);
}

/*
 * Function	:scan_disk_node
 *	scan all node, and find the repair node
 *
 */
static void
scan_disk_node(fmd_hdl_t *hdl, id_t id, void *data)
{
	topo_hdl_t *thp;
	topo_walk_t *twp;
	int err, ret;
	disk_timeout_t *disk_timeout = fmd_hdl_getspecific(hdl);

	thp = fmd_hdl_topo_hold(hdl, TOPO_VERSION);
	if ((twp = topo_walk_init(thp, FM_FMRI_SCHEME_HC, find_repair_falty_disk,
	    disk_timeout, &err)) == NULL) {
		fmd_hdl_topo_rele(hdl, thp);
		fmd_hdl_error(hdl, "failed to get topology: %s\n",
		    topo_strerror(err));
		syslog(LOG_ERR, "dt timeout analyze disk failed to get topo");

		return;
	}

	if (topo_walk_step(twp, TOPO_WALK_CHILD) == TOPO_WALK_ERR) {
		topo_walk_fini(twp);
		fmd_hdl_topo_rele(hdl, thp);
		fmd_hdl_error(hdl, "failed to walk topology\n");
		syslog(LOG_ERR, "dt timeout analyze disk failed to walk topo");

		return;
	}

	topo_walk_fini(twp);
	fmd_hdl_topo_rele(hdl, thp);
}

#define DISK_IOSTAT_CMD  "iostat -dxzn %d %d | grep -i %s | awk \'NR>1{print 
$1, $2}\'"
#define DISK_PATH_LEN 128
#define MAX_LINE        1024
static int busy_threshhold = 0; /* MB/s */
static int stat_sec = 0;
static int stat_cnt = 0;
static int io_check_onoff = FMD_B_TRUE;
static int vdev_is_busy(char *dev_path)
{
    FILE *f;
    char cmd[DISK_PATH_LEN];
    char buf[MAX_LINE] = {0};
    float rd_bw = 0, wr_bw = 0; 
    char *filter = NULL;
    const char * delimit = "@n"; /* dev_path is like this: id1,sd@
n5000c50062c42ffb */

    filter = strstr(dev_path, delimit);
    if(NULL == filter) {
        syslog(LOG_ERR, "dev invalid: %s\n", dev_path);
        return B_FALSE;
    }
    filter += strlen(delimit);

    memset(cmd, 0, DISK_PATH_LEN);
    snprintf(cmd, DISK_PATH_LEN,
             DISK_IOSTAT_CMD, stat_sec, stat_cnt, filter);

    if ((f = popen(cmd, "r")) == NULL) {
        syslog(LOG_ERR, "popen error: %s\n", cmd);
        return B_FALSE;
    }

    while (fgets(buf, MAX_LINE, f) != NULL) {
        sscanf(buf, "%f %f", &rd_bw, &wr_bw);
        syslog(LOG_ERR, "rd_bw:%f, wr_bw:%f, busy_threshhold:%d(MB/s)\n", 
rd_bw, wr_bw, busy_threshhold);

        /*Once there is 1 record W/R bandwidth exceeds the threshhold, we 
think the device is busy.*/
        if(rd_bw + wr_bw > (float)busy_threshhold) {
            pclose(f);
            return B_TRUE;
        }
        memset(buf, 0, MAX_LINE);
    }

    pclose(f);
    return B_FALSE;
}

void
timeout_io_recv(fmd_hdl_t *hdl, fmd_event_t *event, nvlist_t *nvl,
	const char *class)
{
	nvlist_t *detector = NULL;
	uint64_t sas_addr = 0;
	char devid[32] = {0};
	nvlist_t *fault;
	topo_node_info_t *node;
	disk_timeout_t *disk_timeout = fmd_hdl_getspecific(hdl);

	_NOTE(ARGUNUSED(class));

	if (nvlist_lookup_nvlist(nvl, "detector", &detector) != 0) {
		slow_io_stats.bad_scheme.fmds_value.ui64++;
		return;
	}
	if (nvlist_lookup_uint64(detector, "addr", &sas_addr) != 0) {
		disk_sense_stats.bad_fmri.fmds_value.ui64++;
		return;
	}
	U64toA(devid,sizeof(devid)/sizeof(devid[0]),sas_addr);
	if (nvlist_lookup_string(detector, "devid", &devid) != 0) {
		disk_io_stats.bad_fmri.fmds_value.ui64++;
		return;
	}

	if (fmd_serd_exists(hdl, devid) == 0) {
		fmd_serd_create(hdl, devid, fmd_prop_get_int32(hdl, "io_N"),
		    fmd_prop_get_int64(hdl, "io_T"));
		(void) fmd_serd_record_w(hdl, devid, event);
		return;
	}

    if ((fmd_serd_record_w(hdl, devid, event) == FMD_B_TRUE)
            && (slow_check_onoff == FMD_B_TRUE)) {
        fmd_case_t *c;

        if(vdev_is_busy(devid)) {
            fmd_serd_destroy(hdl, devid);
            syslog(LOG_ERR, "vdev:%s is busy, not need report slow disk", devid);
            return;
        }
	
        c = fmd_case_open(hdl, NULL);
		fmd_case_add_serd(hdl, c, devid);
		node = topo_node_lookup_by_devid(hdl, devid);

		/*
		 * If for some reason libtopo does not enumureate the device
		 * we still want to create a fault, we will see a "bad"
		 * FMA message however that lacks the FRU information.
		 */

		if (node == NULL) {
			fault = fmd_nvl_create_fault(hdl,
			    "fault.io.disk.timeout", 100,
			    detector, NULL, NULL);
		} else {
			fault = fmd_nvl_create_fault(hdl,
			    "fault.io.disk.timeout", 100,
			    detector, node->fru, node->resource);
			nvlist_free(node->fru);
			nvlist_free(node->resource);
			fmd_hdl_free(hdl, node, sizeof (topo_node_info_t));
		}

		fmd_case_add_suspect(hdl, c, fault);
		fmd_case_setspecific(hdl, c, devid);
		fmd_case_solve(hdl, c);

		/* record the path, then find the slow disk */
		disk_timeout->compare_path = devid;
		scan_disk_node(hdl, 0, NULL);
	}
}

static const fmd_hdl_ops_t fmd_ops = {
	timeout_io_recv,
	NULL,
	timeout_io_close,
	NULL,
	NULL,
};

static const fmd_hdl_info_t fmd_info = {
	"timeout-io-de", "0.2", &fmd_ops, fmd_props
};

void
_fmd_init(fmd_hdl_t *hdl)
{
	disk_timeout_t *timeout;
	if (fmd_hdl_register(hdl, FMD_API_VERSION, &fmd_info) != 0) {
		fmd_hdl_debug(hdl, "Internal error\n");
		return;
	}

    busy_threshhold = fmd_prop_get_int32(hdl, "busy_threshhold");
    stat_sec = fmd_prop_get_int32(hdl, "stat_sec");
    stat_cnt = fmd_prop_get_int32(hdl, "stat_cnt");
    if(busy_threshhold == 0 ||
       stat_sec == 0 || stat_cnt == 0) {
       fmd_hdl_error(hdl, "moudle props error");
       fmd_hdl_unregister(hdl);
       return;
    }
    io_check_onoff = fmd_prop_get_int32(hdl, "io_check_onoff");

    syslog(LOG_ERR, "busy_threshhold:%d(MB/s), stat_sec:%d, stat_cnt:%d, 
slow_check_onoff:%d", 
        busy_threshhold, stat_sec, stat_cnt, slow_check_onoff);

	(void) fmd_stat_create(hdl, FMD_STAT_NOALLOC, sizeof (disk_io_stats) /
	    sizeof (fmd_stat_t), (fmd_stat_t *)&disk_io_stats);

	timeout = fmd_hdl_zalloc(hdl, sizeof (disk_timeout_t), FMD_SLEEP);
	fmd_hdl_setspecific(hdl, timeout);
	fmd_hdl_subscribe(hdl, "ereport.io.device.link down");
}

void
_fmd_fini(fmd_hdl_t *hdl)
{
	disk_timeout_t *timeout;
	
	timeout = fmd_hdl_getspecific(hdl);
	if (timeout) {
		fmd_hdl_free(hdl, timeout, sizeof (disk_timeout_t));
	}

	_NOTE(ARGUNUSED(hdl));
}

