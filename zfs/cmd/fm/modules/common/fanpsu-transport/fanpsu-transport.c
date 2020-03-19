#include <unistd.h>
#include <ctype.h>
//#include <umem.h>
#include <fmd_api.h>
#include <libtopo.h>
#include <topo_hc.h>
#include <topo_mod.h>
#include <limits.h>
#include <string.h>
#include <libnvpair.h>
#include <sys/fm/protocol.h>
#include <fanpsu_enum.h>
#include "fanpsu-transport.h"
#include "st_zfs.h"


#define		ZFS_SET_EACH_SYNC	"zfs set sync=%s %s "

#define		MAX_COMMAND_LENGTH	512


static void fpt_timeout(fmd_hdl_t *hdl, unsigned int id, void *data);

static const fmd_prop_t fmd_fanpsu_props[] = {
	{ "interval", FMD_TYPE_TIME, "40sec" },
	{ "min-interval", FMD_TYPE_TIME, "3min" },
	{ NULL, 0, NULL }
};

static const fmd_hdl_ops_t fmd_fanpsu_ops = {
	NULL,			/* fmdo_recv */
	fpt_timeout,		/* fmdo_timeout */
	NULL, 			/* fmdo_close */
	NULL,			/* fmdo_stats */
	NULL,			/* fmdo_gc */
	NULL,			/* fmdo_send */
	NULL,		/* fmdo_topo_change */
};

static const fmd_hdl_info_t fmd_info = {/*{{{*/
	"Fanpsu Transport Agent", "1.0", &fmd_fanpsu_ops, fmd_fanpsu_props
};

static struct fpt_stat {
	fmd_stat_t dropped;
} fpt_stats = {
	{ "dropped", FMD_TYPE_UINT64, "number of dropped ereports" }
};
	
static void fpt_post_ereport(fmd_hdl_t *hdl, fmd_xprt_t *xprt, const char *protocol, const char *faultname,
	uint64_t ena, nvlist_t *detector, nvlist_t *payload){

	nvlist_t *nvl;
	int e = 0;
	char fullclass[PATH_MAX];

	snprintf(fullclass, sizeof (fullclass), "%s.%s.%s", FM_EREPORT_CLASS, protocol, faultname);

	if(0 == nvlist_alloc(&nvl, NV_UNIQUE_NAME, 0)){
		e |= nvlist_add_string(nvl, FM_CLASS, fullclass);
		e |= nvlist_add_uint8(nvl, FM_VERSION, FM_EREPORT_VERSION);
		e |= nvlist_add_uint64(nvl, FM_EREPORT_ENA, ena);
		e |= nvlist_add_nvlist(nvl, FM_EREPORT_DETECTOR, detector);
		e |= nvlist_merge(nvl, payload, 0);

		if(e == 0){
			fmd_xprt_post(hdl, xprt, nvl, 0);
		}else{
			fpt_stats.dropped.fmds_value.ui64++;
		}
		nvlist_free(nvl);
	}else{
		fpt_stats.dropped.fmds_value.ui64++;
	}
}

static int
st_set_callback(zfs_handle_t *zhp, int depth, void *data, boolean_t flag)
{
	char buf[ZFS_MAXPROPLEN];
	char rbuf[ZFS_MAXPROPLEN];
	char zfs_set_sync_cmd[MAX_COMMAND_LENGTH];
	zprop_source_t sourcetype;
	char source[ZFS_MAXNAMELEN];
	zprop_get_cbdata_t *cbp = data;
	nvlist_t *user_props = zfs_get_user_props(zhp);
	zprop_list_t *pl = cbp->cb_proplist;
	nvlist_t *propval;
	char *strval;
	char *sourceval;
	boolean_t is_fs = B_FALSE;
	boolean_t received = is_recvd_column(cbp);

	LOG("setdataset:%s,%d\n",zfs_get_name(zhp), flag);
	for (; pl != NULL; pl = pl->pl_next) {
		char *recvdval = NULL;
		/*
		 * Skip the special fake placeholder.  This will also skip over
		 * the name property when 'all' is specified.
		 */
		if (pl->pl_prop == ZFS_PROP_NAME &&
		    pl == cbp->cb_proplist)
			continue;

		if (pl->pl_prop != ZPROP_INVAL) 	{
			if (zfs_prop_get(zhp, pl->pl_prop, buf,
			    sizeof (buf), &sourcetype, source,
			    sizeof (source),
			    cbp->cb_literal) != 0) {
				if (pl->pl_all)
					continue;
				if (!zfs_prop_valid_for_type(pl->pl_prop,
				    ZFS_TYPE_DATASET,B_FALSE)) {
					(void) syslog(LOG_ERR, "No such property '%s'\n",
					    zfs_prop_to_name(pl->pl_prop));
					continue;
				}
				sourcetype = ZPROP_SRC_NONE;
				(void) strlcpy(buf, "-", sizeof (buf));
			}

			if (received && (zfs_prop_get_recvd(zhp,
			    zfs_prop_to_name(pl->pl_prop), rbuf, sizeof (rbuf),
			    cbp->cb_literal) == 0)) {
				recvdval = rbuf;
			}
		} else {
			if (nvlist_lookup_nvlist(user_props,
			    pl->pl_user_prop, &propval) != 0) {
				if (pl->pl_all)
					continue;
				sourcetype = ZPROP_SRC_NONE;
				strval = "-";
			} else {
				verify(nvlist_lookup_string(propval,
				    ZPROP_VALUE, &strval) == 0);
				verify(nvlist_lookup_string(propval,
				    ZPROP_SOURCE, &sourceval) == 0);

				if (strcmp(sourceval,
				    zfs_get_name(zhp)) == 0) {
					sourcetype = ZPROP_SRC_LOCAL;
				} else if (strcmp(sourceval,
				    ZPROP_SOURCE_VAL_RECVD) == 0) {
					sourcetype = ZPROP_SRC_RECEIVED;
				} else {
					sourcetype = ZPROP_SRC_INHERITED;
					(void) strlcpy(source,
					    sourceval, sizeof (source));
				}
			}

			if (received && (zfs_prop_get_recvd(zhp,
			    pl->pl_user_prop, rbuf, sizeof (rbuf),
			    cbp->cb_literal) == 0))
				recvdval = rbuf;
			
			LOG("  %10s %10s type:%d,%d\n", pl->pl_user_prop, strval, sourcetype, 
received);
			if ((strcmp(pl->pl_user_prop, "origin:sync") == 0)
				&& (strcmp(strval, "-") != 0)) {
				bzero(zfs_set_sync_cmd, MAX_COMMAND_LENGTH);
				if (flag && is_fs)
					sprintf(zfs_set_sync_cmd, ZFS_SET_EACH_SYNC, "always", zfs_get_name(zhp));
				else if (flag && !is_fs)
					sprintf(zfs_set_sync_cmd, ZFS_SET_EACH_SYNC, "disk", zfs_get_name(zhp));
				else
					sprintf(zfs_set_sync_cmd, ZFS_SET_EACH_SYNC, strval, zfs_get_name(zhp));
				system(zfs_set_sync_cmd);
				LOG("  st_set_callback: %s\n", zfs_set_sync_cmd);
			}
		}
		if (strcmp(zfs_prop_to_name(pl->pl_prop), "type") == 0) {
			if (strcmp("filesystem", buf) == 0) {
				is_fs = B_TRUE;
			} else {
				is_fs = B_FALSE;
			}
		}
	}
	
	return (0);
}

static int
st_walk_zfs_dataset(st_zfs_iter_cb callback, boolean_t flag)
{
	zprop_get_cbdata_t cb = { 0 };
	int i, c, flags = 0;
	char *value;
	char fields[22];	
	int ret = 0;
	int limit = 0;
	zprop_list_t fake_name = { 0 };

	cb.cb_sources = ZPROP_SRC_ALL;
	cb.cb_columns[0] = GET_COL_NAME;
	cb.cb_columns[1] = GET_COL_PROPERTY;
	cb.cb_columns[2] = GET_COL_VALUE;
	cb.cb_columns[3] = GET_COL_SOURCE;
	cb.cb_type = ZFS_TYPE_DATASET;

	bzero(fields, 22);
	bcopy("type,sync,origin:sync", fields, 22);

	if ((g_zfs = libzfs_init()) == NULL) {
		(void) syslog(LOG_ERR, "fmd sensor transport error: failed to "
		    "initialize ZFS library");
		return (-1);
	}
	
	if (zprop_get_list(g_zfs, fields, &cb.cb_proplist, ZFS_TYPE_DATASET)
	    != 0) {
		syslog (LOG_ERR, "zfs get list failed");
		return (-1);
	}

	if (cb.cb_proplist != NULL) {
		fake_name.pl_prop = ZFS_PROP_NAME;
		fake_name.pl_width = strlen("NAME");
		fake_name.pl_next = cb.cb_proplist;
		cb.cb_proplist = &fake_name;
	}

	cb.cb_first = B_TRUE;
	
	/* run for each object */
	ret = st_zfs_for_each(flags, ZFS_TYPE_DATASET, NULL,
	    &cb.cb_proplist, limit, callback, &cb, flag);

	if (cb.cb_proplist == &fake_name)
		zprop_free_list(fake_name.pl_next);
	else
		zprop_free_list(cb.cb_proplist);

	
	libzfs_fini(g_zfs);
	return ret;
}

static int fpt_check(topo_hdl_t *thp, tnode_t *node, void *arg){
	nvlist_t *fmri;
	nvlist_t *resault = NULL;
	int err;
	fanpsu_monitor_t *fpmp = arg;
	uint64_t ena;
	char *name = topo_node_name(node);

	if(strcmp(name, FAN) && strcmp(name, PSU)) 
		return TOPO_WALK_NEXT;

#if 0
	printf(" ## name ## %s ##\n", topo_node_name(node));
#endif

	if(0 != topo_node_resource(node, &fmri, &err)){
		fmd_hdl_error(fpmp->fpm_hdl, "failed to get fmri: %s\n", topo_strerror(err));
		return TOPO_WALK_ERR;
	}

	ena = fmd_event_ena_create(fpmp->fpm_hdl);

	if(-1 == topo_method_invoke(node, TOPO_METH_STATUS, TOPO_METH_FANPSU_VERSION,
		NULL, &resault, &err)){
		syslog(LOG_ERR, "failed to run topo_method_invoke TOPO_METH_FANPSU_UPDATE_STATE in fpt_check\n");
		return TOPO_WALK_NEXT;
	}

	if(resault){
		if(strcasestr(name,"PSU") != NULL){
			//(void) st_walk_zfs_dataset(st_set_callback, B_TRUE);
		}
		
		fpt_post_ereport(fpmp->fpm_hdl, fpmp->fpm_xprt, "sensor", "failure", ena, fmri, resault);
		nvlist_free(resault);
	}else{
		syslog(LOG_ERR, "There is no warning from fan and psu.\n");
	}
	nvlist_free(fmri);

	return TOPO_WALK_NEXT;
}

/*
 * Periodic timeout.  Iterates over all hc:// topo nodes, calling
 * lt_check_links() for each one.
 */
/*ARGSUSED*/
static void fpt_timeout(fmd_hdl_t *hdl, unsigned int id, void *data){

	topo_hdl_t *thp;
	topo_walk_t *twp;
	int err;
	fanpsu_monitor_t *fpmp = fmd_hdl_getspecific(hdl);
#if 0
	static int count = 0;
	printf("      ########    monitor count    %-5d    #########\n\n", count++);
#endif
	fpmp->fpm_hdl = hdl;

	thp = fmd_hdl_topo_hold(hdl, TOPO_VERSION);
	if(NULL == (twp = topo_walk_init(thp, FM_FMRI_SCHEME_HC, fpt_check, fpmp, &err))){
		fmd_hdl_topo_rele(hdl, thp);
		fmd_hdl_error(hdl, "failed to get topology: %s\n",
		    topo_strerror(err));
		return;
	}

	if(TOPO_WALK_ERR == topo_walk_step(twp, TOPO_WALK_CHILD)){
		topo_walk_fini(twp);
		fmd_hdl_topo_rele(hdl, thp);
		fmd_hdl_error(hdl, "failed to walk topology\n");
		return;
	}

	topo_walk_fini(twp);

	fmd_hdl_topo_rele(hdl, thp);

	fpmp->fpm_timer = fmd_timer_install(hdl, NULL, NULL, fpmp->fpm_interval);
}

void _fmd_init(fmd_hdl_t *hdl){
	fanpsu_monitor_t *fpmp;

	syslog(LOG_ERR,"fanpsu transport start.\n");

	if(0 != fmd_hdl_register(hdl, FMD_API_VERSION, &fmd_info)){
		syslog(LOG_ERR,"failed to run fmd_hdl_register in Link Transport Agent\n");
		return;
	}

	if(NULL == fmd_stat_create(hdl, FMD_STAT_NOALLOC, sizeof (fpt_stats) / sizeof (fmd_stat_t), (fmd_stat_t *)&fpt_stats)){
		syslog(LOG_ERR,"failed to run fmd_stat_create in Fanpsu Transport Agent\n");
		return;

	}

	fpmp = fmd_hdl_zalloc(hdl, sizeof (fanpsu_monitor_t), FMD_SLEEP);
	fmd_hdl_setspecific(hdl, fpmp);

	fpmp->fpm_xprt = fmd_xprt_open(hdl, FMD_XPRT_RDONLY, NULL, NULL);
	fpmp->fpm_interval = fmd_prop_get_int64(hdl, "interval");
	fpmp->fpm_timer = fmd_timer_install(hdl, NULL, NULL, 0);

	return;
}

void _fmd_fini(fmd_hdl_t *hdl){
	fanpsu_monitor_t *fpmp;

	fpmp = fmd_hdl_getspecific(hdl);
	if (fpmp) {
		fmd_xprt_close(hdl, fpmp->fpm_xprt);
		fmd_hdl_free(hdl, fpmp, sizeof (*fpmp));
	}

}
