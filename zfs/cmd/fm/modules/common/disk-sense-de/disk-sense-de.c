/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License").
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
 */

#include <fm/fmd_api.h>
#include <sys/note.h>
#include <fm/libtopo.h>
#include <sys/fm/protocol.h>
#include <sys/fm/io/disk.h>
#include <strings.h>
#include <fm/topo_hc.h>
#include <fm/topo_list.h>

typedef struct disk_sense_stat {
	fmd_stat_t bad_fmri;
	fmd_stat_t bad_scheme;
} disk_sense_stat_t;

typedef struct disk_derr {
	char *compare_path;
} disk_derr_t;

disk_sense_stat_t disk_sense_stats = {
	{ "bad_FMRI", FMD_TYPE_UINT64,
		"event FMRI is missing or invalid" },
	{ "bad_scheme", FMD_TYPE_UINT64,
		"event does not contain a valid detector"},
};

static const fmd_prop_t fmd_props [] = {
	{ "io_N", FMD_TYPE_INT32, "20" },
	{ "io_T", FMD_TYPE_TIME, "5min"},
	{ "ignore-illegal-request", FMD_TYPE_BOOL, "true"},
	{ NULL, 0, NULL }
};

typedef struct topo_node_info {
	const char *device;
	nvlist_t *fru;
	nvlist_t *resource;
} topo_node_info_t;

void
disk_sense_close(fmd_hdl_t *hdl, fmd_case_t *c)
{
	char *devid = fmd_case_getspecific(hdl, c);
	if (devid != NULL && (fmd_serd_exists(hdl, devid) == FMD_B_TRUE)) {
		fmd_hdl_debug(hdl, "Destroying serd: %s", devid);
		fmd_serd_destroy(hdl, devid);
	}
}

int
topo_walk_cb(topo_hdl_t *thp, tnode_t *tn, void *arg) {

	topo_node_info_t *node = (topo_node_info_t *)arg;
	char *cur_devid = NULL;
	nvlist_t *fru = NULL;
	nvlist_t *resource = NULL;
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

	topo_node_info_t *node = (topo_node_info_t *) fmd_hdl_zalloc(hdl,
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
		fmd_hdl_debug(hdl, "Could not find device with matching FRU");
		fmd_hdl_free(hdl, node, sizeof (topo_node_info_t));
		return (NULL);
	} else {
		fmd_hdl_debug(hdl, "Found FRU for device %s", device);
		return (node);
	}
}

/*
 * Function	:
 *	when find the  error node, then change is_derr to "yes"
 */
static int find_derr_error_disk(topo_hdl_t *thp, tnode_t *node, void *arg)
{
	int err;
	disk_derr_t *disk_derr = arg;
	tnode_t *child = NULL;
	char *devpath = NULL;

	if ((strcmp(topo_node_name(node), "bay") == 0) &&
		((child = topo_child_first(node)) != NULL) &&
		(strncmp(topo_node_name(child), "disk", 4) == 0) &&
		(topo_prop_get_string(child, TOPO_PGROUP_IO,
			TOPO_IO_DEVID, &devpath, &err) == 0)) {
		if (strcmp(disk_derr->compare_path, devpath) == 0) {

			/* set the disk is derr node */
			if (topo_prop_set_string(child, TOPO_PGROUP_IO,
				TOPO_IO_DERR, TOPO_PROP_MUTABLE, "yes", &err) != 0) {
				syslog(LOG_ERR, "change is_slow status fail");
			}
		}
		topo_hdl_strfree(thp, devpath);
	}
	return (TOPO_WALK_NEXT);
}

/*
 * Function	:scan_disk_node
 *	scan all node, and find the error node
 */
static void
scan_disk_node(fmd_hdl_t *hdl, id_t id, void *data)
{
	topo_hdl_t *thp;
	topo_walk_t *twp;
	int err, ret;
	disk_derr_t *disk_derr = fmd_hdl_getspecific(hdl);

	thp = fmd_hdl_topo_hold(hdl, TOPO_VERSION);
	if ((twp = topo_walk_init(thp, FM_FMRI_SCHEME_HC, find_derr_error_disk,
	    disk_derr, &err)) == NULL) {
		fmd_hdl_topo_rele(hdl, thp);
		fmd_hdl_error(hdl, "failed to get topology: %s\n",
		    topo_strerror(err));
		syslog(LOG_ERR, "disk-sense-de scan disk failed\n");

		return;
	}

	if (topo_walk_step(twp, TOPO_WALK_CHILD) == TOPO_WALK_ERR) {
		topo_walk_fini(twp);
		fmd_hdl_topo_rele(hdl, thp);
		fmd_hdl_error(hdl, "failed to walk topology\n");
		syslog(LOG_ERR, "disk-sense-de scan disk failed\n");

		return;
	}

	topo_walk_fini(twp);
	fmd_hdl_topo_rele(hdl, thp);
}

static void
disk_sense_case_solve(fmd_hdl_t *hdl, const char *faultclass, fmd_case_t *c,
    char *devid, nvlist_t *detector)
{
	topo_node_info_t *node;
	char faultname[PATH_MAX];
	nvlist_t *fault = NULL;
	disk_derr_t *disk_derr = fmd_hdl_getspecific(hdl);

	(void) snprintf(faultname, sizeof (faultname),
	    "fault.io.disk.%s", faultclass);

	if ((node = topo_node_lookup_by_devid(hdl, devid)) == NULL) {
		fault = fmd_nvl_create_fault(hdl, faultname, 100,
		    detector, NULL, NULL);
	} else {
		fault = fmd_nvl_create_fault(hdl, faultname, 100,
		    detector, node->fru, node->resource);
		nvlist_free(node->fru);
		nvlist_free(node->resource);
		fmd_hdl_free(hdl, node, sizeof (topo_node_info_t));
	}

	fmd_case_add_suspect(hdl, c, fault);
	fmd_case_setspecific(hdl, c, devid);
	fmd_case_solve(hdl, c);

	/* record the path, then find the slow disk */
	disk_derr->compare_path = devid;
	scan_disk_node(hdl, 0, NULL);
}

void
disk_sense_recv(fmd_hdl_t *hdl, fmd_event_t *event, nvlist_t *nvl,
	const char *class)
{
	nvlist_t *detector = NULL;
	char *devid = NULL;
	uint8_t key = 0;
	uint8_t asc = 0;
	uint8_t ascq = 0;
	_NOTE(ARGUNUSED(class));

	if (nvlist_lookup_nvlist(nvl, "detector", &detector) != 0) {
		disk_sense_stats.bad_scheme.fmds_value.ui64++;
		return;
	}

	if (nvlist_lookup_string(detector, "devid", &devid) != 0) {
		disk_sense_stats.bad_fmri.fmds_value.ui64++;
		return;
	}

	if ((nvlist_lookup_uint8(nvl, "key", &key) != 0) &&
	    (nvlist_lookup_uint8(nvl, "asc", &asc) != 0) &&
	    (nvlist_lookup_uint8(nvl, "ascq", &ascq) != 0)) {
		disk_sense_stats.bad_fmri.fmds_value.ui64++;
		fmd_hdl_debug(hdl, "Invalid ereport payload");
		return;
	}

	if (key == 0x1 && asc == 0xb && ascq == 0x1) {
		/* over temp reported by drive sense data */
		fmd_case_t *c = fmd_case_open(hdl, NULL);
		fmd_case_add_ereport(hdl, c, event);
		disk_sense_case_solve(hdl, FM_FAULT_DISK_OVERTEMP, c,
		    devid, detector);
		return;
	}

	if (key == 0x5 && asc == 0x26 &&
	    (fmd_prop_get_int32(hdl, "ignore-illegal-request") == FMD_B_TRUE)) {
		fmd_hdl_debug(hdl, "Illegal request for device, ignoring");
		return;
	}

	if (fmd_serd_exists(hdl, devid) == 0) {
		fmd_serd_create(hdl, devid, fmd_prop_get_int32(hdl, "io_N"),
		    fmd_prop_get_int64(hdl, "io_T"));
		(void) fmd_serd_record_w(hdl, devid, event);
		return;
	}
	syslog(LOG_ERR, "errors-exceeded error\n");

	if (fmd_serd_record_w(hdl, devid, event) == FMD_B_TRUE) {
		fmd_case_t *c = fmd_case_open(hdl, NULL);
		fmd_case_add_serd(hdl, c, devid);
		syslog(LOG_ERR, "errors-exceeded fault\n");
		disk_sense_case_solve(hdl, "device-errors-exceeded", c,
		    devid, detector);
	}
}

static const fmd_hdl_ops_t fmd_ops = {
	disk_sense_recv,
	NULL,
	disk_sense_close,
	NULL,
	NULL,
};

static const fmd_hdl_info_t fmd_info = {
	"disk-sense-de", "0.2", &fmd_ops, fmd_props
};

void
_fmd_init(fmd_hdl_t *hdl)
{
	disk_derr_t *derr;

	if (fmd_hdl_register(hdl, FMD_API_VERSION, &fmd_info) != 0) {
		fmd_hdl_debug(hdl, "Internal error\n");
		return;
	}

	(void) fmd_stat_create(hdl, FMD_STAT_NOALLOC,
	    sizeof (disk_sense_stats) / sizeof (fmd_stat_t),
	    (fmd_stat_t *)&disk_sense_stats);

	derr = fmd_hdl_zalloc(hdl, sizeof (disk_derr_t), FMD_SLEEP);
	fmd_hdl_setspecific(hdl, derr);
}

void
_fmd_fini(fmd_hdl_t *hdl)
{
	disk_derr_t *derr;
	
	derr = fmd_hdl_getspecific(hdl);
	if (derr) {
		fmd_hdl_free(hdl, derr, sizeof (disk_derr_t));
	}
}

