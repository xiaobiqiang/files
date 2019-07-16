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

/*
 * Copyright 2009 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

#include <sys/fm/protocol.h>
#include <fm/fmd_api.h>
#include <sys/fs/zfs.h>
#include <libzfs.h>
#include <umem.h>
#include <strings.h>
#include <libdevinfo.h>
#include <sys/modctl.h>
#include <sys/libdevid.h>
#include <fm/topo_hc.h>
#include <fm/topo_list.h>
#include <fm/topo_fruhash.h>
#include <fm/topo_mod.h>
#include <ses_led.h>
#include <unistd.h>

#define	SELFTESTFAIL	0x01
#define	OVERTEMPFAIL	0x02
#define	PREDICTIVEFAIL	0x04
#define	SLOWDISK	0x08
#define	DERRDISK	0x10
#define	MERRDISK	0x20
#define	NORESPDISK	0x40

#define TOPO_LINK_TYPE "type"
#define TOPO_LINK_STATE "state"
#define TOPO_LINK_STATE_DESC "state_desc"
#define TOPO_LINK_NAME "name"

#define	TOPO_PGROUP_SES		"ses"
#define	TOPO_PROP_TARGET_PATH	"target-path"
#define	TOPO_PGROUP_AUTHORITY	"authority"
#define	TOPO_PROP_PRODUCT_ID	"product-id"

static int	global_disable;

struct except_list {
	char			*el_fault;
	struct except_list	*el_next;
};

typedef struct io_retire {
	fmd_hdl_t	*ir_fhdl;
	fmd_xprt_t	*ir_xprt;
	libzfs_handle_t	*ir_zhdl;
	char *compare_path;
	int repair;
	int flag;
} io_retire_t;

typedef struct find_cbdata {
	uint64_t	cb_guid;
	const char	*cb_devid;
	const char	*cb_fru;
	zpool_handle_t	*cb_zhp;
	nvlist_t	*cb_vdev;
} find_cbdata_t;

static struct except_list *except_list;

static void 
rio_send_snmptrap(fmd_hdl_t *hdl, fmd_xprt_t *xprt,
	const char *protocol, const char *faultname,
	uint64_t ena, nvlist_t *detector,
	const char *devpath, const char *state){

	nvlist_t *nvl;
	int e = 0;
	char fullclass[PATH_MAX];

	snprintf(fullclass, sizeof (fullclass), "%s.%s.%s", FM_EREPORT_CLASS, protocol, faultname);

	if(nvlist_alloc(&nvl, NV_UNIQUE_NAME, 0) == 0){
		e |= nvlist_add_string(nvl, FM_CLASS, fullclass);
		e |= nvlist_add_uint8(nvl, FM_VERSION, FM_EREPORT_VERSION);
		e |= nvlist_add_uint64(nvl, FM_EREPORT_ENA, ena);
		e |= nvlist_add_nvlist(nvl, FM_EREPORT_DETECTOR, detector);
		e |= nvlist_add_string(nvl, TOPO_LINK_TYPE, "disk");
		e |= nvlist_add_string(nvl, TOPO_LINK_NAME, devpath);
		e |= nvlist_add_uint32(nvl, TOPO_LINK_STATE, 4);
		e |= nvlist_add_string(nvl, TOPO_LINK_STATE_DESC, state);

		if(e == 0)
			fmd_xprt_post(hdl, xprt, nvl, 0);
		else
			nvlist_free(nvl);
	}
}

static void
parse_exception_string(fmd_hdl_t *hdl, char *estr)
{
	char	*p;
	char	*next;
	size_t	len;
	struct except_list *elem;

	len = strlen(estr);

	p = estr;
	for (;;) {
		/* Remove leading ':' */
		while (*p == ':')
			p++;
		if (*p == '\0')
			break;

		next = strchr(p, ':');

		if (next)
			*next = '\0';

		elem = fmd_hdl_alloc(hdl,
		    sizeof (struct except_list), FMD_SLEEP);
		elem->el_fault = fmd_hdl_strdup(hdl, p, FMD_SLEEP);
		elem->el_next = except_list;
		except_list = elem;

		if (next) {
			*next = ':';
			p = next + 1;
		} else {
			break;
		}
	}

	if (len != strlen(estr)) {
		fmd_hdl_abort(hdl, "Error parsing exception list: %s\n", estr);
	}
}

/*
 * Returns
 *	1  if fault on exception list
 *	0  otherwise
 */
static int
fault_exception(fmd_hdl_t *hdl, nvlist_t *fault)
{
	struct except_list *elem;

	for (elem = except_list; elem; elem = elem->el_next) {
		if (fmd_nvl_class_match(hdl, fault, elem->el_fault)) {
			fmd_hdl_debug(hdl, "rio_recv: Skipping fault "
			    "on exception list (%s)\n", elem->el_fault);
			return (1);
		}
	}

	return (0);
}

static void
free_exception_list(fmd_hdl_t *hdl)
{
	struct except_list *elem;

	while (except_list) {
		elem = except_list;
		except_list = elem->el_next;
		fmd_hdl_strfree(hdl, elem->el_fault);
		fmd_hdl_free(hdl, elem, sizeof (*elem));
	}
}

static int
rio_get_node_info(tnode_t *node, char **lpath,
	char **slot_id, char **en_id, char **product_id)
{
	int err = 0;
	
	if (topo_prop_get_string(node, TOPO_PGROUP_AUTHORITY,
		    TOPO_PROP_PRODUCT_ID, product_id, &err) != 0 ||
		    err != 0 ||
		topo_prop_get_string(node, TOPO_PGROUP_IO,
		    TOPO_IO_EN_ID, en_id, &err) != 0 ||
		    err !=0 ||
		topo_prop_get_string(node, TOPO_PGROUP_IO,
		    TOPO_IO_SLOT_ID, slot_id, &err) != 0 ||
		    err != 0 ||
		topo_prop_get_string(node, TOPO_PGROUP_IO,
		    TOPO_IO_LINK_PATH, lpath, &err) != 0 ||
		    err != 0)
		return err;
	return 0;
}

static void
rio_free_node_info(char *tpath, char *lpath,char *en_id, char *slot_id,
	char *product_id)
{
	if(lpath != NULL) {
		umem_free(lpath, strlen(lpath)+1);
	}
	if(en_id != NULL) {
		umem_free(en_id, strlen(en_id)+1);
	}
	if(slot_id != NULL) {
		umem_free(slot_id, strlen(slot_id)+1);
	}
	if(product_id != NULL) {
		umem_free(product_id, strlen(product_id)+1);
	}
	if(tpath != NULL)
		umem_free(tpath, strlen(tpath)+1);
}

static uint32_t cm_cnm_disk_enid_exchange(long hostid,uint32_t localid)
{
    uint32_t sbbid = ((uint32_t)hostid + 1)>>1;
    
    if(localid <= 22)
    {
        localid = 0;
    }
    else
    {
        localid -= 22;
    }
    return (sbbid*1000 + localid);
}
static int led_fault_node_cb(topo_hdl_t *thp, tnode_t *node, void *arg)
{
	int err = 0;
	uint64_t ena;
	io_retire_t *irp = arg;
	tnode_t *child = NULL;
	char *devpath = NULL;
	char *tpath = NULL, *lpath = NULL, *productid = NULL;
	char *slotid = NULL, *encid;
	unsigned int slot;
	boolean_t is_repair = irp->repair;
	nvlist_t *fmri;
	char *fmristr;
	topo_fru_t *fru;
    long hostid = 0;
    uint32_t enid = 0;
	char buf[128];
	char cmd_buf[CM_ALARM_CMD_LEN];
	char hostname[64];

	if ((strcmp(topo_node_name(node), "bay") == 0) &&
		((child = topo_child_first(node)) != NULL) &&
		(strncmp(topo_node_name(child), "disk", 4) == 0) &&
		(topo_prop_get_string(child, TOPO_PGROUP_IO,
			TOPO_IO_DEVID, &devpath, &err) == 0)) {
			
		if (strcmp(irp->compare_path, devpath) == 0) {

			if (topo_node_resource(child, &fmri, &err) != 0 ||
				topo_fmri_nvl2str(thp, fmri, &fmristr, &err) != 0){
				nvlist_free(fmri);
				topo_hdl_strfree(thp, devpath);
				syslog(LOG_ERR, "failed to get fmri: %s\n", topo_strerror(err));
				return TOPO_WALK_ERR;
			}
			if (rio_get_node_info(child, &lpath, &slotid,
				&encid, &productid) != 0) {
				syslog(LOG_ERR, "get node info failed: %s\n", topo_strerror(err));
				return TOPO_WALK_ERR;
			}
            hostid = gethostid();
            enid = cm_cnm_disk_enid_exchange(hostid,atoi(slotid));
			ena = fmd_event_ena_create(irp->ir_fhdl);
			memset(buf, 128, 0);
			snprintf(buf, 128, "%s,enc=%s,slot=%s",lpath, encid, slotid);
			if (gethostname(hostname, sizeof(hostname)) < 0) {
				syslog(LOG_ERR, "get hostname failed\n");
			}
			switch (irp->flag) {
				case EN_TOPO_IO_SLOW:
					if (topo_prop_set_string(child, TOPO_PGROUP_IO,
						TOPO_IO_SLOW, TOPO_PROP_MUTABLE, (is_repair ? "no" : "yes"), &err) != 0) {
						syslog(LOG_ERR, "change is_slow status fail");
					}
					if (is_repair) {
						fru = topo_fru_cleartime(fmristr, SLOWDISK);
						cm_alarm_cmd(cmd_buf, "%s %d \"%s,%s,-,-,-\"",
							"recovery", AMARM_ID_SLOW_DISK, hostname, lpath);
					} else {
						fru = topo_fru_setime(fmristr, SLOWDISK, lpath, slotid,
							encid, productid);
						cm_alarm_cmd(cmd_buf, "%s %d \"%s,%s,%s,%u,%s\"", 
							"report", AMARM_ID_SLOW_DISK,
							hostname, lpath, slotid, enid, productid);
						rio_send_snmptrap(irp->ir_fhdl, irp->ir_xprt, "ceresdata", "trapinfo",
							ena, fmri, buf, "slow-disk");
					}
					break;
				case EN_TOPO_IO_DERR:
					if (topo_prop_set_string(child, TOPO_PGROUP_IO,
						TOPO_IO_DERR, TOPO_PROP_MUTABLE, (is_repair ? "no" : "yes"), &err) != 0) {
						syslog(LOG_ERR, "change is_derr status fail");
					}
					if (is_repair) {
						fru = topo_fru_cleartime(fmristr, DERRDISK);
						cm_alarm_cmd(cmd_buf, "%s %d \"%s,%s,-,-,-\"",
							"recovery", AMARM_ID_DERR_DISK, hostname, lpath);
					} else {
						fru = topo_fru_setime(fmristr, DERRDISK, lpath, slotid,
							encid, productid);
						cm_alarm_cmd(cmd_buf, "%s %d \"%s,%s,%s,%u,%s\"", 
							"report", AMARM_ID_DERR_DISK,
							hostname, lpath, slotid, enid, productid);
						rio_send_snmptrap(irp->ir_fhdl, irp->ir_xprt, "ceresdata", "trapinfo",
							ena, fmri, buf, "derr-disk");
					}
					break;
				case EN_TOPO_IO_MERR:
					if (topo_prop_set_string(child, TOPO_PGROUP_IO,
						TOPO_IO_MERR, TOPO_PROP_MUTABLE, (is_repair ? "no" : "yes"), &err) != 0) {
						syslog(LOG_ERR, "change is_merr status fail");
					}
					if (is_repair) {
						fru = topo_fru_cleartime(fmristr, MERRDISK);
						cm_alarm_cmd(cmd_buf, "%s %d \"%s,%s,-,-,-\"",
							"recovery", AMARM_ID_MERR_DISK, hostname, lpath);
					} else {
						fru = topo_fru_setime(fmristr, MERRDISK, lpath, slotid,
							encid, productid);
						cm_alarm_cmd(cmd_buf, "%s %d \"%s,%s,%s,%u,%s\"", 
							"report", AMARM_ID_MERR_DISK,
							hostname, lpath, slotid, enid, productid);
						rio_send_snmptrap(irp->ir_fhdl, irp->ir_xprt, "ceresdata", "trapinfo",
							ena, fmri, buf, "merr-disk");
					}
					break;
				case EN_TOPO_IO_NORESP:
					if (topo_prop_set_string(child, TOPO_PGROUP_IO,
						TOPO_IO_NORESP, TOPO_PROP_MUTABLE,  (is_repair ? "no" : "yes"), &err) != 0) {
						syslog(LOG_ERR, "change no_response status fail");
					}
					if (is_repair) {
						fru = topo_fru_cleartime(fmristr, NORESPDISK);
						cm_alarm_cmd(cmd_buf, "%s %d \"%s,%s,-,-,-\"",
							"recovery", AMARM_ID_NORESPONSE_DISK, hostname, lpath);
					} else {
						fru = topo_fru_setime(fmristr, NORESPDISK, lpath, slotid,
							encid, productid);
						cm_alarm_cmd(cmd_buf, "%s %d \"%s,%s,%s,%u,%s\"", 
							"report", AMARM_ID_NORESPONSE_DISK,
							hostname, lpath, slotid, enid, productid);
						rio_send_snmptrap(irp->ir_fhdl, irp->ir_xprt, "ceresdata", "trapinfo",
							ena, fmri, buf, "no-response-disk");
					}
					break;
				case EN_TOPO_IO_PREF:
					if (is_repair) {
						fru = topo_fru_cleartime(fmristr, PREDICTIVEFAIL);
						cm_alarm_cmd(cmd_buf, "%s %d \"%s,%s,-,-,-\"",
							"recovery", AMARM_ID_PREDICTURE_DISK, hostname, lpath);
					} else {
						fru = topo_fru_setime(fmristr, PREDICTIVEFAIL, lpath, slotid,
							encid, productid);
						cm_alarm_cmd(cmd_buf, "%s %d \"%s,%s,%s,%u,%s\"", 
							"report", AMARM_ID_PREDICTURE_DISK,
							hostname, lpath, slotid, enid, productid);
						rio_send_snmptrap(irp->ir_fhdl, irp->ir_xprt, "ceresdata", "trapinfo",
							ena, fmri, buf, "predictive-failure");
					}
					break;
				case EN_TOPO_IO_SELF:
					if (is_repair) {
						fru = topo_fru_cleartime(fmristr, SELFTESTFAIL);
						cm_alarm_cmd(cmd_buf, "%s %d \"%s,%s,-,-,-\"",
							"recovery", AMARM_ID_SELFTEST_FAIL_DISK, hostname, lpath);
					} else {
						fru = topo_fru_setime(fmristr, SELFTESTFAIL, lpath, slotid,
							encid, productid);
						cm_alarm_cmd(cmd_buf, "%s %d \"%s,%s,%s,%u,%s\"", 
							"report", AMARM_ID_SELFTEST_FAIL_DISK,
							hostname, lpath, slotid, enid, productid);
						rio_send_snmptrap(irp->ir_fhdl, irp->ir_xprt, "ceresdata", "trapinfo",
							ena, fmri, buf, "self-test-failure");
					}
					break;
				case EN_TOPO_IO_TEMP:
					if (is_repair) {
						fru = topo_fru_cleartime(fmristr, OVERTEMPFAIL);
						cm_alarm_cmd(cmd_buf, "%s %d \"%s,%s,-,-,-\"",
							"recovery", AMARM_ID_OVERTEMP_DISK, hostname, lpath);
					} else {
						fru = topo_fru_setime(fmristr, OVERTEMPFAIL, lpath, slotid,
							encid, productid);
						cm_alarm_cmd(cmd_buf, "%s %d \"%s,%s,%s,%u,%s\"", 
							"report", AMARM_ID_OVERTEMP_DISK,
							hostname, lpath, slotid, enid, productid);
						rio_send_snmptrap(irp->ir_fhdl, irp->ir_xprt, "ceresdata", "trapinfo",
							ena, fmri, buf, "over-temperature");
					}
					break;
			}

			if (fru != NULL && fru->tf_time == 0 &&
				topo_prop_set_uint32(node, TOPO_PGROUP_PROTOCOL, 
				"state", TOPO_PROP_MUTABLE, SXML_OK, &err)) {
				syslog(LOG_ERR, "set node failed\n");
			} else if (fru != NULL && fru->tf_time != 0 &&
				topo_prop_set_uint32(node, TOPO_PGROUP_PROTOCOL, 
				"state", TOPO_PROP_MUTABLE, SXML_CRITICAL, &err)) {
				syslog(LOG_ERR, "set node failed\n");
			}
			
			/* change led status */
			topo_prop_get_string(node, TOPO_PGROUP_SES,
				TOPO_PROP_TARGET_PATH, &tpath, &err);
			if (err == 0) {
				slot = strtoul((const char *)slotid, NULL, 10) - 1;
				if (ses_led_set_disk_led(tpath, slot, (is_repair ? 1 : 2)) != 0) {
					if (ses_led_set_disk_led(tpath, slot, 2) != 0)
						syslog(LOG_ERR, "io-retire: led failed, target-path:%s, slot:%d\n",
							tpath, slot);
				}
			} else {
				syslog(LOG_ERR, "io-retire: get target-path or slot failed\n");
			}
			
			rio_free_node_info(tpath, lpath, encid, slotid, productid);
			nvlist_free(fmri);
			topo_hdl_strfree(thp, fmristr);
		}
		topo_hdl_strfree(thp, devpath);
	}
	return (TOPO_WALK_NEXT);
}

static void
rio_scan_disk_node(fmd_hdl_t *hdl, id_t id, void *data)
{
	topo_hdl_t *thp;
	topo_walk_t *twp;
	int err, ret;
	io_retire_t *irp = fmd_hdl_getspecific(hdl);

	thp = fmd_hdl_topo_hold(hdl, TOPO_VERSION);
	if ((twp = topo_walk_init(thp, FM_FMRI_SCHEME_HC, led_fault_node_cb,
	    irp, &err)) == NULL) {
		fmd_hdl_topo_rele(hdl, thp);
		fmd_hdl_error(hdl, "failed to get topology: %s\n",
		    topo_strerror(err));
		syslog(LOG_ERR, "rio failed to get topo\n");

		return;
	}

	if (topo_walk_step(twp, TOPO_WALK_CHILD) == TOPO_WALK_ERR) {
		topo_walk_fini(twp);
		fmd_hdl_topo_rele(hdl, thp);
		fmd_hdl_error(hdl, "failed to walk topology\n");
		syslog(LOG_ERR, "rio failed to walk topo\n");

		return;
	}

	topo_walk_fini(twp);
	fmd_hdl_topo_rele(hdl, thp);
}

static void
rio_update_topo_node_info(fmd_hdl_t *hdl, nvlist_t *fault,
	char *devid, boolean_t is_repair)
{
	io_retire_t *irp = fmd_hdl_getspecific(hdl);

	if (devid == NULL)
		return;

	if (fmd_nvl_class_match(hdl, fault, "fault.io.disk.slow-io")) {
		irp->flag = EN_TOPO_IO_SLOW;
	} else if (fmd_nvl_class_match(hdl, fault, "fault.io.disk.device-errors-exceeded")) {
		irp->flag = EN_TOPO_IO_DERR;
	} else if (fmd_nvl_class_match(hdl, fault, "fault.io.disk.no-response")) {
		irp->flag = EN_TOPO_IO_NORESP;
	} else if (fmd_nvl_class_match(hdl, fault, "fault.io.scsi.cmd.disk.dev.rqs.derr")) {
		irp->flag = EN_TOPO_IO_DERR;
	} else if (fmd_nvl_class_match(hdl, fault, "fault.io.scsi.cmd.disk.dev.rqs.merr")) {
		irp->flag = EN_TOPO_IO_MERR;
	} else if (fmd_nvl_class_match(hdl, fault, "fault.io.disk.predictive-failure")) {
		irp->flag = EN_TOPO_IO_PREF;
	} else if (fmd_nvl_class_match(hdl, fault, "fault.io.disk.over-temperature")) {
		irp->flag = EN_TOPO_IO_TEMP;
	} else if (fmd_nvl_class_match(hdl, fault, "fault.io.disk.self-test-failure")) {
		irp->flag = EN_TOPO_IO_SELF;
	}
	
	irp->compare_path = devid;
	irp->repair = is_repair;
	rio_scan_disk_node(hdl, 0, NULL);
}

/*
 * Find a vdev within a tree with a matching devid.
 */
static nvlist_t *
find_vdev(libzfs_handle_t *zhdl, nvlist_t *nv, const char *search_devid,
    uint64_t search_guid)
{
	uint64_t guid;
	nvlist_t **child;
	uint_t c, children;
	nvlist_t *ret;
	char *devid;

	if (search_devid != NULL) {
		if (nvlist_lookup_string(nv, ZPOOL_CONFIG_DEVID, &devid) == 0 &&
			devid_str_compare(devid, (char *)search_devid) == 0)
			return (nv);
	} else {
		if (nvlist_lookup_uint64(nv, ZPOOL_CONFIG_GUID, &guid) == 0 &&
		    guid == search_guid)
			return (nv);
	}

	if (nvlist_lookup_nvlist_array(nv, ZPOOL_CONFIG_CHILDREN,
	    &child, &children) != 0)
		return (NULL);

	for (c = 0; c < children; c++) {
		if ((ret = find_vdev(zhdl, child[c], search_devid,
		    search_guid)) != NULL)
			return (ret);
	}

	if (nvlist_lookup_nvlist_array(nv, ZPOOL_CONFIG_L2CACHE,
	    &child, &children) != 0)
		return (NULL);

	for (c = 0; c < children; c++) {
		if ((ret = find_vdev(zhdl, child[c], search_devid,
		    search_guid)) != NULL)
			return (ret);
	}

	return (NULL);
}

static int
search_pool(zpool_handle_t *zhp, void *data)
{
	find_cbdata_t *cbp = data;
	nvlist_t *config;
	nvlist_t *nvroot;

	config = zpool_get_config(zhp, NULL);
	if (nvlist_lookup_nvlist(config, ZPOOL_CONFIG_VDEV_TREE,
	    &nvroot) != 0) {
		zpool_close(zhp);
		return (0);
	}

	if ((cbp->cb_vdev = find_vdev(zpool_get_handle(zhp), nvroot,
	    cbp->cb_devid, 0)) != NULL) {
		cbp->cb_zhp = zhp;
		return (1);
	}

	zpool_close(zhp);
	return (0);
}

/*
 * Given a devid, find the matching pool and vdev.
 */
static zpool_handle_t *
find_by_devid(libzfs_handle_t *zhdl, const char *devid, nvlist_t **vdevp)
{
	find_cbdata_t cb;

	cb.cb_devid = devid;
	cb.cb_zhp = NULL;
	if (zpool_iter(zhdl, search_pool, &cb) != 1)
		return (NULL);

	*vdevp = cb.cb_vdev;
	return (cb.cb_zhp);
}

static int
rio_iter_vdev_d(nvlist_t *nvl, nvlist_t *pnvl, int *count, char *devid)
{
	nvlist_t	**child;
	uint_t		c, children;
	char		*type;
	char		*search_devid;
	vdev_stat_t *vs;
	uint_t ct;
	int		ret = -1;

	if (nvlist_lookup_string(nvl, ZPOOL_CONFIG_TYPE, &type) == 0 &&
		strcmp(type, VDEV_TYPE_DISK) == 0 &&
		nvlist_lookup_string(nvl, ZPOOL_CONFIG_DEVID, &search_devid) == 0 &&
		devid_str_compare(devid, search_devid) != 0 &&
		nvlist_lookup_uint64_array(nvl, ZPOOL_CONFIG_VDEV_STATS,
			 (uint64_t **)&vs, &ct) == 0 &&
		vs->vs_state < VDEV_STATE_DEGRADED) {
		(*count)++;
		return (0);
	}
	if (nvlist_lookup_nvlist_array(nvl, ZPOOL_CONFIG_CHILDREN,
	    &child, &children) != 0)
		return (ret);
	
	for (c = 0; c < children; c++) {
		ret = rio_iter_vdev_d(child[c], nvl, count, devid);
	}
	return ret;
}


static int
rio_iter_vdev(nvlist_t *nvl, nvlist_t *pnvl, int *count, char *devid)
{
	nvlist_t	**child;
	uint_t		c, children;
	uint64_t	retire;
	char		*type;
	char		*search_devid;
	int			tmp;
	vdev_stat_t *vs;
	uint_t ct;
	int			ret = -1;

	if (nvlist_lookup_string(nvl, ZPOOL_CONFIG_TYPE, &type) == 0 &&
		strcmp(type, VDEV_TYPE_DISK) == 0 &&
		nvlist_lookup_string(nvl, ZPOOL_CONFIG_DEVID, &search_devid) == 0 &&
		devid_str_compare(devid, search_devid) == 0 &&
		nvlist_lookup_uint64_array(nvl, ZPOOL_CONFIG_VDEV_STATS,
			 (uint64_t **)&vs, &ct) == 0 &&
		vs->vs_state < VDEV_STATE_DEGRADED) {
		*count = 1;
		return (0);
	}

	if (nvlist_lookup_nvlist_array(nvl, ZPOOL_CONFIG_CHILDREN,
	    &child, &children) != 0)
		return (ret);

	for (c = 0; c < children; c++) {
		ret = rio_iter_vdev(child[c], nvl, count, devid);
	}

	if (nvlist_lookup_nvlist_array(nvl, ZPOOL_CONFIG_L2CACHE,
	    &child, &children) != 0)
		return (ret);

	for (c = 0; c < children; c++) {
		ret = rio_iter_vdev(child[c], nvl, count, devid);
	}

	return (ret);
}

static void
rio_handle_ereport_event(fmd_hdl_t *hdl, nvlist_t *nvl)
{
	io_retire_t *irp;
	zpool_handle_t *zhp;
	di_retire_t	drt = {0};
	char		*devid;
	uint64_t	retire;
	nvlist_t	*asru;
	nvlist_t	*vdev;
	
	char		*path;
	int			error;

	irp = fmd_hdl_getspecific(hdl);

	drt.rt_abort = (void (*)(void *, const char *, ...))fmd_hdl_abort;
	drt.rt_debug = (void (*)(void *, const char *, ...))fmd_hdl_debug;
	drt.rt_hdl = hdl;

	if (nvlist_lookup_nvlist(nvl, FM_FAULT_ASRU, &asru) != 0 ||
		nvlist_lookup_string(asru, FM_FMRI_DEV_ID, &devid) != 0 ||
		nvlist_lookup_string(asru, FM_FMRI_DEV_PATH, &path) !=0)
		return;

	if ((zhp = find_by_devid(irp->ir_zhdl, devid, &vdev)) == NULL)
		return;
	
	if (nvlist_lookup_uint64(vdev, ZPOOL_CONFIG_RETIRE, &retire) != 0) {
		zpool_close(zhp);
		return;
	}

	if (retire == VDEV_EXTERNAL_FAULT_CANRETIRE &&
		fmd_nvl_fmri_has_fault(hdl, asru, FMD_HAS_FAULT_ASRU, NULL) == 1) {
		error = di_retire_device(path, &drt, 0);
		if (error != 0) {
			fmd_hdl_debug(hdl, "rio_recv:"
				" di_retire_device failed:"
				" error: %d %s", error, path);
		}
	} 	
	zpool_close(zhp);
}

static int
rio_check_whether_can_retire_disk(fmd_hdl_t *hdl, nvlist_t *nvl)
{
	io_retire_t *irp;
	zpool_handle_t *zhp;
	char		*devid;
	uint64_t	retire;
	nvlist_t	*asru;
	nvlist_t	*vdev;
	char		*path;
	int			count;
	int 		error;
	nvlist_t	 *config;
	nvlist_t	 *nvroot;
	int		 ret = -1;

	irp = fmd_hdl_getspecific(hdl);

	if (nvlist_lookup_nvlist(nvl, FM_FAULT_ASRU, &asru) != 0 ||
		nvlist_lookup_string(asru, FM_FMRI_DEV_ID, &devid) != 0 ||
		nvlist_lookup_string(asru, FM_FMRI_DEV_PATH, &path) !=0)
		return 0;

	if ((zhp = find_by_devid(irp->ir_zhdl, devid, &vdev)) == NULL)
		return 1;

	config = zpool_get_config(zhp, NULL);
	if (nvlist_lookup_nvlist(config, ZPOOL_CONFIG_VDEV_TREE,
		&nvroot) != 0) {
		zpool_close(zhp);
		return 0;
	}
 
	(void) rio_iter_vdev(nvroot, NULL, &count, devid);
	syslog(LOG_ERR, "fault vdevs: %d\n", count);
	if (count > 0) {
		zpool_close(zhp);
	 	return 1;
	}
	
	syslog(LOG_ERR, "Pool %s may be not health, can't retire %s.\n",
		zpool_get_name(zhp), devid);
	zpool_close(zhp);
	return 0;
}


/*ARGSUSED*/
static void
rio_recv(fmd_hdl_t *hdl, fmd_event_t *ep, nvlist_t *nvl, const char *class)
{
	nvlist_t	**faults = NULL;
	nvlist_t	*asru;
	uint_t		nfaults = 0;
	int		f;
	char		*path;
	char		*uuid;
	char		*scheme;
	di_retire_t	drt = {0};
	int		retire;
	int		rval = 0;
	int		valid_suspect = 0;
	int		error;
	char	*snglfault = FM_FAULT_CLASS"."FM_ERROR_IO".";
	char	*ereportclass1 = "ereport.io.scsi.disk";	/* self test, over-temp, pred-fail */
	char	*ereportclass2 = "ereport.io.scsi.cmd.disk";/* slow-io derr-io merr-io */
	char 	*ereportclass3 = "ereport.io.device";		/* no-response */
	boolean_t	rtr;
	char	*fclass;
	char	*devid;
	io_retire_t *irp;
	nvlist_t	*vdev;
	nvlist_t	*tmpasru;

	/*
	 * If disabled, we don't do retire. We still do unretires though
	 */
	if (global_disable && (strcmp(class, FM_LIST_SUSPECT_CLASS) == 0 ||
	    strcmp(class, FM_LIST_UPDATED_CLASS) == 0)) {
		fmd_hdl_debug(hdl, "rio_recv: retire disabled\n");
		return;
	}

	drt.rt_abort = (void (*)(void *, const char *, ...))fmd_hdl_abort;
	drt.rt_debug = (void (*)(void *, const char *, ...))fmd_hdl_debug;
	drt.rt_hdl = hdl;

	if (strncmp(class, ereportclass1, strlen(ereportclass1)) == 0 ||
		strncmp(class, ereportclass2, strlen(ereportclass2)) == 0 ||
		strncmp(class, ereportclass3, strlen(ereportclass3)) == 0) {
		rio_handle_ereport_event(hdl, nvl);
	} else if (strcmp(class, FM_LIST_SUSPECT_CLASS) == 0) {
		retire = 1;
	} else if (strcmp(class, FM_LIST_REPAIRED_CLASS) == 0) {
		retire = 0;
	} else if (strcmp(class, FM_LIST_UPDATED_CLASS) == 0) {
		retire = 0;
	} else if (strcmp(class, FM_LIST_RESOLVED_CLASS) == 0) {
		return;
	} else if (strncmp(class, snglfault, strlen(snglfault)) == 0) {
		retire = 1;
		faults = &nvl;
		nfaults = 1;
	} else {
		fmd_hdl_debug(hdl, "rio_recv: not list.* class: %s\n", class);
		return;
	}

	if (nfaults == 0 && nvlist_lookup_nvlist_array(nvl,
	    FM_SUSPECT_FAULT_LIST, &faults, &nfaults) != 0) {
		fmd_hdl_debug(hdl, "rio_recv: no fault list");
		return;
	}

	for (f = 0; f < nfaults; f++) {
		if (nvlist_lookup_boolean_value(faults[f], FM_SUSPECT_RETIRE,
		    &rtr) == 0 && !rtr) {
			fmd_hdl_debug(hdl, "rio_recv: retire suppressed");
			continue;
		}

		if (nvlist_lookup_nvlist(faults[f], FM_FAULT_ASRU,
		    &asru) != 0) {
			fmd_hdl_debug(hdl, "rio_recv: no asru in fault");
			continue;
		}

		if (nvlist_lookup_string(faults[f], FM_CLASS,
		    &fclass) != 0) {
			fmd_hdl_debug(hdl, "rio_recv: no fm_class in fault");
			continue;
		}

		scheme = NULL;
		if (nvlist_lookup_string(asru, FM_FMRI_SCHEME, &scheme) != 0 ||
		    strcmp(scheme, FM_FMRI_SCHEME_DEV) != 0) {
			fmd_hdl_debug(hdl, "rio_recv: not \"dev\" scheme: %s",
			    scheme ? scheme : "<NULL>");
			continue;
		}

		if (fault_exception(hdl, faults[f]))
			continue;

		if (nvlist_lookup_string(asru, FM_FMRI_DEV_PATH,
		    &path) != 0 || path[0] == '\0') {
			fmd_hdl_debug(hdl, "rio_recv: no dev path in asru");
			continue;
		}

		valid_suspect = 1;
		if (retire) {
			sleep(5);

			/*
			 when system reboot, led the fault disk.
			*/
			if (nvlist_lookup_string(asru, FM_FMRI_DEV_ID,
				&devid) != 0 || devid[0] == '\0')
				continue;
			rio_update_topo_node_info(hdl, faults[f], devid, 0);

			/*
			 retire the no response disk
			*/
			if (strcmp(fclass, "fault.io.disk.no-response") != 0 ||
				rio_check_whether_can_retire_disk(hdl, faults[f]) == 0)
				continue;
			
			if (fmd_nvl_fmri_has_fault(hdl, asru,
			    FMD_HAS_FAULT_ASRU, NULL) == 1) {
				error = di_retire_device(path, &drt, 0);
				if (error != 0) {
					fmd_hdl_debug(hdl, "rio_recv:"
					    " di_retire_device failed:"
					    " error: %d %s", error, path);
					rval = -1;
				}
			}
		} else {
			/*
			 * recover the fault disk led.
			*/
			if (nvlist_lookup_string(asru, FM_FMRI_DEV_ID,
				&devid) != 0 || devid[0] == '\0')
				continue;
			rio_update_topo_node_info(hdl, faults[f], devid, 1);
			
			if (fmd_nvl_fmri_has_fault(hdl, asru,
			    FMD_HAS_FAULT_ASRU, NULL) == 0) {
				error = di_unretire_device(path, &drt);
				if (error != 0) {
					fmd_hdl_debug(hdl, "rio_recv:"
					    " di_unretire_device failed:"
					    " error: %d %s", error, path);
					rval = -1;
				}
			}
		}
	}
	/*
	 * Run through again to handle new faults in a list.updated.
	 */
	for (f = 0; f < nfaults; f++) {
		if (nvlist_lookup_boolean_value(faults[f], FM_SUSPECT_RETIRE,
		    &rtr) == 0 && !rtr) {
			fmd_hdl_debug(hdl, "rio_recv: retire suppressed");
			continue;
		}

		if (nvlist_lookup_nvlist(faults[f], FM_FAULT_ASRU,
		    &asru) != 0) {
			fmd_hdl_debug(hdl, "rio_recv: no asru in fault");
			continue;
		}

		scheme = NULL;
		if (nvlist_lookup_string(asru, FM_FMRI_SCHEME, &scheme) != 0 ||
		    strcmp(scheme, FM_FMRI_SCHEME_DEV) != 0) {
			fmd_hdl_debug(hdl, "rio_recv: not \"dev\" scheme: %s",
			    scheme ? scheme : "<NULL>");
			continue;
		}

		if (fault_exception(hdl, faults[f]))
			continue;

		if (nvlist_lookup_string(asru, FM_FMRI_DEV_PATH,
		    &path) != 0 || path[0] == '\0') {
			fmd_hdl_debug(hdl, "rio_recv: no dev path in asru");
			continue;
		}

		if (strcmp(class, FM_LIST_UPDATED_CLASS) == 0) {
			if (fmd_nvl_fmri_has_fault(hdl, asru,
			    FMD_HAS_FAULT_ASRU, NULL) == 1) {
				error = di_retire_device(path, &drt, 0);
				if (error != 0) {
					fmd_hdl_debug(hdl, "rio_recv:"
					    " di_retire_device failed:"
					    " error: %d %s", error, path);
				}
			}
		}
	}

	/*
	 * Don't send uuclose or uuresolved unless at least one suspect
	 * was valid for this retire agent and no retires/unretires failed.
	 */
	if (valid_suspect == 0)
		return;

	/*
	 * The fmd framework takes care of moving a case to the repaired
	 * state. To move the case to the closed state however, we (the
	 * retire agent) need to call fmd_case_uuclose()
	 */
	if (strcmp(class, FM_LIST_SUSPECT_CLASS) == 0 && rval == 0) {
		if (nvlist_lookup_string(nvl, FM_SUSPECT_UUID, &uuid) == 0 &&
		    !fmd_case_uuclosed(hdl, uuid)) {
			fmd_case_uuclose(hdl, uuid);
		}
	}

	/*
	 * Similarly to move the case to the resolved state, we (the
	 * retire agent) need to call fmd_case_uuresolved()
	 */
	if (strcmp(class, FM_LIST_REPAIRED_CLASS) == 0 && rval == 0 &&
	    nvlist_lookup_string(nvl, FM_SUSPECT_UUID, &uuid) == 0) 
		fmd_case_uuresolved(hdl, uuid);
}

static const fmd_hdl_ops_t fmd_ops = {
	rio_recv,	/* fmdo_recv */
	NULL,		/* fmdo_timeout */
	NULL,		/* fmdo_close */
	NULL,		/* fmdo_stats */
	NULL,		/* fmdo_gc */
};

static const fmd_prop_t rio_props[] = {
	{ "global-disable", FMD_TYPE_BOOL, "false" },
	{ "fault-exceptions", FMD_TYPE_STRING, NULL },
	{ NULL, 0, NULL }
};

static const fmd_hdl_info_t fmd_info = {
	"I/O Retire Agent", "2.1", &fmd_ops, rio_props
};

void
_fmd_init(fmd_hdl_t *hdl)
{
	char	*estr;
	char	*estrdup;
	io_retire_t *irp;

	if (fmd_hdl_register(hdl, FMD_API_VERSION, &fmd_info) != 0) {
		fmd_hdl_debug(hdl, "failed to register handle\n");
		return;
	}

	global_disable = fmd_prop_get_int32(hdl, "global-disable");

	estrdup = NULL;
	if (estr = fmd_prop_get_string(hdl, "fault-exceptions")) {
		estrdup = fmd_hdl_strdup(hdl, estr, FMD_SLEEP);
		fmd_prop_free_string(hdl, estr);
		parse_exception_string(hdl, estrdup);
		fmd_hdl_strfree(hdl, estrdup);
	}

	irp = fmd_hdl_zalloc(hdl, sizeof(io_retire_t), FMD_SLEEP);
	irp->ir_fhdl = hdl;
	irp->ir_xprt = fmd_xprt_open(hdl, FMD_XPRT_RDONLY, NULL, NULL);
	if ((irp->ir_zhdl = libzfs_init()) == NULL) {
		fmd_hdl_free(hdl, irp, sizeof (io_retire_t));
		return;
	}

	fmd_hdl_setspecific(hdl, irp);
}

void
_fmd_fini(fmd_hdl_t *hdl)
{
	io_retire_t *irp = fmd_hdl_getspecific(hdl);
	
	free_exception_list(hdl);
	libzfs_fini(irp->ir_zhdl);
	fmd_xprt_close(hdl, irp->ir_xprt);
	fmd_hdl_free(hdl, irp, sizeof (io_retire_t));
}
