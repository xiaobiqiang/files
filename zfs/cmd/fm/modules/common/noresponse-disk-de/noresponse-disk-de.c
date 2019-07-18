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
#include <sys/fm/protocol.h>
#include <strings.h>
#include <topo_hc.h>
#include <topo_list.h>

typedef struct noresp_disk_stat {
	fmd_stat_t bad_fmri;
	fmd_stat_t bad_scheme;
} noresp_disk_stat_t;

typedef struct noresp_disk {
	char *compare_path;
} noresp_disk_t;

noresp_disk_stat_t noresp_disk_stats = {
	{ "bad_FMRI", FMD_TYPE_UINT64,
		"event FMRI is missing or invalid" },
	{ "bad_scheme", FMD_TYPE_UINT64,
		"event does not contain a valid detector"},
};

static const fmd_prop_t fmd_props [] = {
	{ "io_N", FMD_TYPE_INT32, "6" },
	{ "io_T", FMD_TYPE_TIME, "10min"},
	{ NULL, 0, NULL }
};

typedef struct topo_noresp_node_info {
	char *device;
	nvlist_t *fru;
	nvlist_t *resource;
	uint32_t enc;
	uint32_t slot;
} topo_noresp_node_info_t;

void
noresp_disk_close(fmd_hdl_t *hdl, fmd_case_t *c)
{
	char *devid = fmd_case_getspecific(hdl, c);
	if (devid != NULL) {
		fmd_hdl_debug(hdl, "Destroying serd: %s", devid);
		fmd_serd_destroy(hdl, devid);
	}
}

int
noresp_disk_topo_walk_cb(topo_hdl_t *thp, tnode_t *tn, void *arg) {

	topo_noresp_node_info_t *node = (topo_noresp_node_info_t *)arg;
	char *cur_devid;
	char *enc;
	char *slot;
	nvlist_t *fru;
	nvlist_t *resource;
	int encid, slotid;
	int err = 0;
	_NOTE(ARGUNUSED(thp));

	if (strcmp(topo_node_name(tn), "disk") != 0)
		return (TOPO_WALK_NEXT);

	if (topo_prop_get_string(tn, "io", "devid", &cur_devid, &err) != 0 ||
		topo_prop_get_string(tn, "io", "en_id", &enc, &err) != 0 ||
		topo_prop_get_string(tn, "io", "slot_id", &slot, &err) != 0)
		return (TOPO_WALK_NEXT);

	encid = atoi(enc);
	slotid = atoi(slot);
	if (encid == node->enc && slotid == node->slot) {
		(void) topo_node_fru(tn, &fru, NULL, &err);
		(void) topo_node_resource(tn, &resource, &err);
		strncpy(node->device, cur_devid, 128);

		if (err == 0) {
			if (nvlist_dup(fru, &node->fru, 0) != 0 ||
				nvlist_dup(resource, &node->resource, 0) != 0) {
				nvlist_free(resource);
				nvlist_free(fru);
				topo_hdl_strfree(thp, cur_devid);
				topo_hdl_strfree(thp, enc);
				topo_hdl_strfree(thp, slot);
				return (TOPO_WALK_ERR);
			}
			
			nvlist_free(resource);
			nvlist_free(fru);
			topo_hdl_strfree(thp, cur_devid);
			topo_hdl_strfree(thp, enc);
			topo_hdl_strfree(thp, slot);
			return (TOPO_WALK_TERMINATE);
		}
		nvlist_free(resource);
		nvlist_free(fru);
	}
	topo_hdl_strfree(thp, cur_devid);
	topo_hdl_strfree(thp, enc);
	topo_hdl_strfree(thp, slot);

	return (TOPO_WALK_NEXT);
}

topo_noresp_node_info_t *
topo_node_lookup_by_enc_slot(fmd_hdl_t *hdl,
	char *device, uint32_t enc, uint32_t slot)
{

	int err = 0;
	topo_hdl_t *thp;
	topo_walk_t *twp;
	topo_noresp_node_info_t *node = NULL;

	if ((node = (topo_noresp_node_info_t *)fmd_hdl_zalloc(hdl,
	    sizeof (topo_noresp_node_info_t), FMD_SLEEP)) == NULL)
		return (NULL);
	
	node->enc = enc;
	node->slot = slot;
	node->device = device;

	thp = fmd_hdl_topo_hold(hdl, TOPO_VERSION);
	if ((twp = topo_walk_init(thp, FM_FMRI_SCHEME_HC, noresp_disk_topo_walk_cb,
	    node, &err)) == NULL) {
		fmd_hdl_error(hdl, "failed to get topology: %s",
		    topo_strerror(err));
		fmd_hdl_topo_rele(hdl, thp);
		fmd_hdl_free(hdl, node, sizeof (topo_noresp_node_info_t));
		return (NULL);
	}

	(void) topo_walk_step(twp, TOPO_WALK_CHILD);
	if (twp != NULL)
		topo_walk_fini(twp);
	if (thp != NULL)
		fmd_hdl_topo_rele(hdl, thp);

	if (node->fru == NULL || node->resource == NULL) {
		fmd_hdl_debug(hdl, "Could not find device and its FRU");
		fmd_hdl_free(hdl, node, sizeof (topo_noresp_node_info_t));
		return (NULL);
	} else {
		fmd_hdl_debug(hdl, "Found FRU for device %s", node->device);
		return (node);
	}
}

/*
 * Function	:
 *	when find the repair node, then change no_resp to "yes"
 */
static int find_set_noresp_disk(topo_hdl_t *thp, tnode_t *node, void *arg)
{
	int err;
	noresp_disk_t *noresp = arg;
	tnode_t *child = NULL;
	char *devpath = NULL;

	if ((strcmp(topo_node_name(node), "bay") == 0) &&
		((child = topo_child_first(node)) != NULL) &&
		(strncmp(topo_node_name(child), "disk", 4) == 0) &&
		(topo_prop_get_string(child, TOPO_PGROUP_IO,
			TOPO_IO_DEVID, &devpath, &err) == 0)) {
		if (strcmp(noresp->compare_path, devpath) == 0) {

			/* set the disk is  slow */
			if (topo_prop_set_string(child, TOPO_PGROUP_IO,
				TOPO_IO_NORESP, TOPO_PROP_MUTABLE, "yes", &err) != 0) {
				syslog(LOG_ERR, "change no_resp status fail");
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
	noresp_disk_t *noresp = fmd_hdl_getspecific(hdl);

	thp = fmd_hdl_topo_hold(hdl, TOPO_VERSION);
	if ((twp = topo_walk_init(thp, FM_FMRI_SCHEME_HC, find_set_noresp_disk,
	    noresp, &err)) == NULL) {
		fmd_hdl_topo_rele(hdl, thp);
		fmd_hdl_error(hdl, "failed to get topology: %s\n",
		    topo_strerror(err));
		syslog(LOG_ERR, "no resp disk failed to get topo");

		return;
	}

	if (topo_walk_step(twp, TOPO_WALK_CHILD) == TOPO_WALK_ERR) {
		topo_walk_fini(twp);
		fmd_hdl_topo_rele(hdl, thp);
		fmd_hdl_error(hdl, "failed to walk topology\n");
		syslog(LOG_ERR, "no resp disk failed to walk topo");

		return;
	}

	topo_walk_fini(twp);
	fmd_hdl_topo_rele(hdl, thp);
}

void
noresp_disk_recv(fmd_hdl_t *hdl, fmd_event_t *event, nvlist_t *nvl,
	const char *class)
{
	uint32_t enc;
	uint32_t slot;
	uint64_t wwn;
	char buf[128];
	char *devid;
	nvlist_t *fault;
	nvlist_t *asru;
	topo_noresp_node_info_t *node = NULL;
	noresp_disk_t *noresp = fmd_hdl_getspecific(hdl);

	_NOTE(ARGUNUSED(class));

	if (nvlist_lookup_uint64(nvl, "wwn", &wwn) != 0) {
		noresp_disk_stats.bad_scheme.fmds_value.ui64++;
		return;
	}

	if (nvlist_lookup_uint32(nvl, "enc", &enc) != 0) {
		noresp_disk_stats.bad_fmri.fmds_value.ui64++;
		return;
	}

	if (nvlist_lookup_uint32(nvl, "slot", &slot) != 0) {
		noresp_disk_stats.bad_fmri.fmds_value.ui64++;
		return;
	}

	memset(buf, 0, 128);
	if ((node = topo_node_lookup_by_enc_slot(hdl,
		buf, enc, slot)) == NULL) {
		snprintf(buf, 128, "id1,sd@n%x", wwn);
	}

	devid = fmd_hdl_zalloc(hdl, strlen(buf)+1, FMD_SLEEP);
	memcpy(devid, buf, strlen(buf)+1);

	if (fmd_serd_exists(hdl, devid) == 0) {
		fmd_serd_create(hdl, devid, fmd_prop_get_int32(hdl, "io_N"),
		    fmd_prop_get_int64(hdl, "io_T"));
		(void) fmd_serd_record(hdl, devid, event);
		if (node != NULL) {
			nvlist_free(node->fru);
			nvlist_free(node->resource);
			fmd_hdl_free(hdl, node, sizeof (topo_noresp_node_info_t));
		}
		fmd_hdl_free(hdl, devid, strlen(devid)+1);
		return;
	}

	if (fmd_serd_record(hdl, devid, event) == FMD_B_TRUE) {
		fmd_case_t *c = fmd_case_open(hdl, NULL);
		fmd_case_add_serd(hdl, c, devid);

		if ((asru = fmd_nvl_alloc(hdl, FMD_SLEEP)) == NULL ||
			nvlist_add_string(asru, "devid", devid) != 0) {
			noresp_disk_stats.bad_fmri.fmds_value.ui64++;
			nvlist_free(asru);
			return;
		}
		
		/*
		 * If for some reason libtopo does not enumureate the device
		 * we still want to create a fault, we will see a "bad"
		 * FMA message however that lacks the FRU information.
		 */

		if (node == NULL) {
			fault = fmd_nvl_create_fault(hdl,
			    "fault.io.disk.no-response", 100,
			    asru, NULL, NULL);
		} else {
			fault = fmd_nvl_create_fault(hdl,
			    "fault.io.disk.no-response", 100,
			    asru, node->fru, node->resource);
			nvlist_free(node->fru);
			nvlist_free(node->resource);
			fmd_hdl_free(hdl, node, sizeof (topo_noresp_node_info_t));
		}
		
		nvlist_free(asru);
		fmd_case_add_suspect(hdl, c, fault);
		fmd_case_setspecific(hdl, c, devid);
		fmd_case_solve(hdl, c);

		/* record the path, then find the no resp disk */
		noresp->compare_path = devid;
		scan_disk_node(hdl, 0, NULL);
	} else if (node != NULL) {
		nvlist_free(node->fru);
		nvlist_free(node->resource);
		fmd_hdl_free(hdl, node, sizeof (topo_noresp_node_info_t));
		fmd_hdl_free(hdl, devid, strlen(devid)+1);
	}
}

static const fmd_hdl_ops_t fmd_ops = {
	noresp_disk_recv,
	NULL,
	noresp_disk_close,
	NULL,
	NULL,
};

static const fmd_hdl_info_t fmd_info = {
	"noresponse-disk-de", "0.1", &fmd_ops, fmd_props
};

void
_fmd_init(fmd_hdl_t *hdl)
{
	noresp_disk_t *noresp;

	if (fmd_hdl_register(hdl, FMD_API_VERSION, &fmd_info) != 0) {
		fmd_hdl_debug(hdl, "Internal error\n");
		return;
	}

	(void) fmd_stat_create(hdl, FMD_STAT_NOALLOC, sizeof (noresp_disk_stats) /
	    sizeof (fmd_stat_t), (fmd_stat_t *)&noresp_disk_stats);

	noresp = fmd_hdl_zalloc(hdl, sizeof (noresp_disk_t), FMD_SLEEP);
	fmd_hdl_setspecific(hdl, noresp);
}

void
_fmd_fini(fmd_hdl_t *hdl)
{
	noresp_disk_t *noresp;
	
	noresp = fmd_hdl_getspecific(hdl);
	if (noresp) {
		fmd_hdl_free(hdl, noresp, sizeof (noresp_disk_t));
	}

	_NOTE(ARGUNUSED(hdl));
}
