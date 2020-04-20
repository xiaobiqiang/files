#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
//#include <umem.h>
#include <pthread.h>
#include <fmd_api.h>
#include <libtopo.h>
#include <topo_hc.h>
#include <topo_mod.h>
#include <limits.h>
#include <string.h>
#include <libnvpair.h>
#include <sys/fm/protocol.h>
#include <disklist.h>
#include <disk_enum.h>
#include <topo_fruhash.h>
#include <topo_list.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <scsi/sg.h>

#include "disk-transport.h"
#include "disk_db.h"

#define DT_XML
#ifdef DT_XML
static xmlNodePtr xml_root_node = NULL;
#endif

#define ADD_SNMP_TRAP
#define DISK_TRAP_LOSTED "losted"
#define TOPO_LINK_TYPE "type"
#define TOPO_LINK_NAME "name"
#define TOPO_LINK_STATE "state"
#define TOPO_LINK_STATE_DESC "state_desc"
#define TOPO_LINK_LASTED_STATE "lasted_state"
#define TOPO_LINK_PRESENT "present"


#define	TOPO_PROP_TARGET_PATH	"target-path"
#define	TOPO_PGROUP_AUTHORITY	"authority"
#define	TOPO_PROP_PRODUCT_ID	"product-id"

#define	FM_EREPORT_SCSI_PREDFAIL	"predictive-failure"
#define	FM_EREPORT_PAYLOAD_SCSI_ASC	"additional-sense-code"
#define	FM_EREPORT_PAYLOAD_SCSI_ASCQ	"additional-sense-code-qualifier"

#define	FM_EREPORT_SCSI_OVERTEMP		"over-temperature"
#define	FM_EREPORT_PAYLOAD_SCSI_CURTEMP		"current-temperature"
#define	FM_EREPORT_PAYLOAD_SCSI_THRESHTEMP	"threshold-temperature"

#define	FM_EREPORT_SCSI_TESTFAIL		"self-test-failure"
#define	FM_EREPORT_PAYLOAD_SCSI_RESULTCODE	"result-code"
#define	FM_EREPORT_PAYLOAD_SCSI_ADDRESS		"address"
#define	FM_EREPORT_PAYLOAD_SCSI_TIMESTAMP	"timestamp"

#define	SELFTESTFAIL	0x01
#define	OVERTEMPFAIL	0x02
#define	PREDICTIVEFAIL	0x04
#define	SLOWDISK	0x08
#define	DERRDISK	0x10
#define	MERRDISK	0x20

#define	DISK_SMARTCTL_TEST_CMD	"/usr/sbin/smartctl -d scsi -a %s"

#define	IGNORE_FLAG	255

#define DISK_PATH_LEN 128
#define MAX_LINE	1024

static struct dt_stat {
	fmd_stat_t dropped;
} dt_stats = {
	{ "dropped", FMD_TYPE_UINT64, "number of dropped ereports" }
};

typedef struct disk_monitor {
	fmd_hdl_t	*dm_hdl;
	fmd_xprt_t	*dm_xprt;
    disk_db_handle_t dm_dbhdl;
	id_t		dm_timer;
	hrtime_t	dm_interval;
	char		*dm_sim_search;
	char		*dm_sim_file;
	boolean_t	dm_timer_istopo;
	boolean_t	dm_update_topo;
} disk_monitor_t;

/*
 * disk node information.
 */
typedef struct dt_disk_node {
	topo_list_t	dt_list;	/* list of disks */

	/* the following two fields are always defined */
	char		*dt_devid;	/* devid of disk */
	char		*dt_dpath;
	char		*dt_lpath;	/* logical path (public /dev name) */
	char		*dt_nblocks;
	char		*dt_blksize;
	char		*dt_mfg;	/* misc information about device */
	char		*dt_model;
	char		*dt_serial;
	char		*dt_firm;
	uint32_t	dt_enc;
	uint32_t	dt_slot;
	int			dt_slice_count;
	uint32_t	dt_rpm;
	char 		dt_gsize[64];
	char		dt_dim[24];
	char		interface_type[12];
	int         	is_ssd;
	char 		*dt_busy;	
} dt_disk_node_t;

/* common callback information for di_walk_node() and di_devlink_walk */
typedef struct dt_cbdata {
	topo_list_t		*dcb_list;

	di_devlink_handle_t	dcb_devhdl;
	dt_disk_node_t		*dcb_dnode;	/* for di_devlink_walk only */
	topo_hdl_t *thp;
	fmd_hdl_t *hdl;
	int count;
} dt_cbdata_t;

static void
dt_topo_change(fmd_hdl_t *hdl, topo_hdl_t *thp);
static int dt_update_disk_info(fmd_hdl_t *hdl, topo_hdl_t *thp);

#ifdef ADD_SNMP_TRAP
static void dt_send_snmptrap(fmd_hdl_t *hdl, fmd_xprt_t *xprt, const char *protocol, const char *faultname,
	uint64_t ena, nvlist_t *detector, const char *devpath, const char *state){

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
		e |= nvlist_add_uint32(nvl, TOPO_LINK_STATE, strcmp(DISK_TRAP_LOSTED, state) ? 3 : 4);
		e |= nvlist_add_string(nvl, TOPO_LINK_STATE_DESC, state);

		if(e == 0)
			fmd_xprt_post(hdl, xprt, nvl, 0);
		else
			nvlist_free(nvl);
	}
}
#endif

static void
dt_post_ereport(fmd_hdl_t *hdl, fmd_xprt_t *xprt, const char *protocol,
    const char *faultname, uint64_t ena, nvlist_t *detector, nvlist_t *payload, nvlist_t *asru)
{
	nvlist_t *nvl;
	int e = 0;
	char fullclass[PATH_MAX];

	(void) snprintf(fullclass, sizeof (fullclass), "%s.io.%s.disk.%s",
	    FM_EREPORT_CLASS, protocol, faultname);

	if (nvlist_alloc(&nvl, NV_UNIQUE_NAME, 0) == 0) {
		e |= nvlist_add_string(nvl, FM_CLASS, fullclass);
		e |= nvlist_add_uint8(nvl, FM_VERSION, FM_EREPORT_VERSION);
		e |= nvlist_add_uint64(nvl, FM_EREPORT_ENA, ena);
		e |= nvlist_add_nvlist(nvl, FM_EREPORT_DETECTOR, detector);
		e |= nvlist_add_nvlist(nvl, FM_FAULT_ASRU, asru);
		e |= nvlist_merge(nvl, payload, 0);

		if (e == 0) {
			fmd_xprt_post(hdl, xprt, nvl, 0);
		} else {
			nvlist_free(nvl);
			dt_stats.dropped.fmds_value.ui64++;
		}
	} else {
		dt_stats.dropped.fmds_value.ui64++;
	}
}

#ifdef DT_XML
static void
xyses_new_disk_xml_node( char *devpath, char *rpath,
	char *lpath, char *tpath, char *en_id, char *slot_id, int status)
{
	xmlNodePtr disk_node, prop_node, status_node;

	disk_node = xmlNewChild(xml_root_node, NULL, (xmlChar*)"disk", NULL);
	/*
	 * LHL ADD
	 * if there is a string NULL fmd will get a sig SEGV when store xmlDoc.
	 */
	prop_node = xmlNewChild(disk_node, NULL, (xmlChar*)"devpath", NULL);
	if(devpath)
		xmlNodeSetContent(prop_node, (xmlChar*)devpath);

	prop_node = xmlNewChild(disk_node, NULL, (xmlChar*)"routepath", NULL);
	if(rpath)
		xmlNodeSetContent(prop_node, (xmlChar*)rpath);

	prop_node = xmlNewChild(disk_node, NULL, (xmlChar*)"linkpath", NULL);
	if(lpath)
		xmlNodeSetContent(prop_node, (xmlChar*)lpath);

	prop_node = xmlNewChild(disk_node, NULL, (xmlChar*)"targetpath", NULL);
	if(tpath)
		xmlNodeSetContent(prop_node, (xmlChar*)tpath);

	prop_node = xmlNewChild(disk_node, NULL, (xmlChar*)"en_id", NULL);
	if(en_id)
		xmlNodeSetContent(prop_node, (xmlChar*)en_id);

	prop_node = xmlNewChild(disk_node, NULL, (xmlChar*)"slot_id", NULL);
	if(slot_id)
		xmlNodeSetContent(prop_node, (xmlChar*)slot_id);

	prop_node = xmlNewChild(disk_node, NULL, (xmlChar*)"status", NULL);
	
	status_node = xmlNewChild(prop_node, NULL, (xmlChar*)FM_EREPORT_SCSI_TESTFAIL, NULL);
	if(status & SELFTESTFAIL)
		xmlNodeSetContent(status_node, (xmlChar*)"fault");
	else
		xmlNodeSetContent(status_node, (xmlChar*)"ok");
	
	status_node = xmlNewChild(prop_node, NULL, (xmlChar*)FM_EREPORT_SCSI_OVERTEMP, NULL);
	if(status & OVERTEMPFAIL)
		xmlNodeSetContent(status_node, (xmlChar*)"fault");
	else
		xmlNodeSetContent(status_node, (xmlChar*)"ok");

	status_node = xmlNewChild(prop_node, NULL, (xmlChar*)FM_EREPORT_SCSI_PREDFAIL, NULL);
	if(status & PREDICTIVEFAIL)
		xmlNodeSetContent(status_node, (xmlChar*)"fault");
	else
		xmlNodeSetContent(status_node, (xmlChar*)"ok");
			

}
#endif

static void
xyses_free_node_info(char *devpath, char *rpath, char *lpath,
	char *tpath, char *en_id, char *slot_id, char *product_id)
{
	if(devpath != NULL) {
		umem_free(devpath, strlen(devpath)+1);
	}
	if(rpath != NULL) {
		umem_free(rpath, strlen(rpath)+1);
	}
	if(lpath != NULL) {
		umem_free(lpath, strlen(lpath)+1);
	}
	if(tpath != NULL) {
		umem_free(tpath, strlen(tpath)+1);
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
}

static void
xyses_get_node_info(tnode_t *node, char **devpath, char **rpath,
	char **lpath, char **en_id, char **slot_id)
{
	int err;
	
	topo_prop_get_string(node, TOPO_PGROUP_IO,
		    TOPO_IO_DEV_PATH, devpath, &err);
	topo_prop_get_string(node, TOPO_PGROUP_IO,
		    TOPO_IO_EN_ID, en_id, &err);
	topo_prop_get_string(node, TOPO_PGROUP_IO,
		    TOPO_IO_SLOT_ID, slot_id, &err);
	topo_prop_get_string(node, TOPO_PGROUP_IO,
		    TOPO_IO_ROUTE_PATH, rpath, &err);
	topo_prop_get_string(node, TOPO_PGROUP_IO,
		    TOPO_IO_LINK_PATH, lpath, &err);
}

static int dt_analyze_disk(topo_hdl_t *thp, tnode_t *node, void *arg){

	nvlist_t *result;
	nvlist_t *fmri, *tfmri, *faults;
	char *protocol;
	int err;
	disk_monitor_t *dmp = arg;
	uint64_t ena;
	nvpair_t *elem;
	boolean_t fault;
	nvlist_t *details;
	char *fmristr, *tfmristr;
	nvlist_t *in = NULL;
	tnode_t *child = NULL;
	topo_fru_t *fru = NULL;
	int 	fault_occur = 0, ret = 0;
	char *devpath = NULL, *rpath = NULL, *lpath = NULL, *tpath = NULL;
	char *en_id = NULL, *slot_id = NULL, *product_id = NULL;
	nvlist_t *asru = NULL;

	if (strcmp(topo_node_name(node), "disk") &&
		strcmp(topo_node_name(node), "bay"))
		return TOPO_WALK_NEXT;

	if(topo_node_resource(node, &fmri, &err) != 0 ||
		topo_fmri_nvl2str(thp, fmri, &fmristr, &err) != 0){
		nvlist_free(fmri);
		fmd_hdl_error(dmp->dm_hdl, "failed to get fmri: %s\n", topo_strerror(err));
		return TOPO_WALK_ERR;
	}

	if (strcmp(topo_node_name(node), "bay") == 0) {
		if ((child = topo_child_first(node)) != NULL &&
			strncmp(topo_node_name(child), "disk", 4) != 0) {
			/*fru = topo_fru_setime(fmristr);*/
		}
		nvlist_free(fmri);
		topo_hdl_strfree(thp, fmristr);
		return TOPO_WALK_NEXT;
	}

	tfmristr = NULL;
	tfmri = NULL;
	if((child = topo_node_parent(node)) == NULL ||
		topo_node_resource(child, &tfmri, &err) != 0 ||
		topo_fmri_nvl2str(thp, tfmri, &tfmristr, &err) != 0) {
		nvlist_free(fmri);
		nvlist_free(tfmri);
		topo_hdl_strfree(thp, fmristr);
		topo_hdl_strfree(thp, tfmristr);
		return TOPO_WALK_ERR;
	}

	if(topo_hdl_nvalloc(thp, &in, NV_UNIQUE_NAME) != 0){
		nvlist_free(fmri);
		nvlist_free(tfmri);
		topo_hdl_strfree(thp, fmristr);
		topo_hdl_strfree(thp, tfmristr);
		return TOPO_WALK_ERR;
	}

	xyses_get_node_info(node, &devpath, &rpath, &lpath, &en_id, &slot_id);
	topo_prop_get_string(child, TOPO_PGROUP_SES, TOPO_PROP_TARGET_PATH, &tpath, &err);
	topo_prop_get_string(child, TOPO_PGROUP_AUTHORITY, TOPO_PROP_PRODUCT_ID, &product_id, &err);

	(void) nvlist_add_string(in, "path", lpath);
	/*
	 * Try to invoke the method.  If this fails (most likely because the
	 * method is not supported), then ignore this node.
	 */
	ena = fmd_event_ena_create(dmp->dm_hdl);

	ret = topo_method_invoke(node, TOPO_METH_DISK_STATUS,
		TOPO_METH_DISK_STATUS_VERSION, in, &result, &err);
	
	if(ret == -2){
		if((fru = topo_fru_setime(tfmristr, SXML_UNAVAI,
			lpath, slot_id, en_id, product_id)) != NULL) {
			dmp->dm_update_topo = B_TRUE;
			if (fru->err_count > 0) {
				/*disk_fault_execute_action(rpath, lpath, tpath, slot_id);*/
				dt_send_snmptrap(dmp->dm_hdl, dmp->dm_xprt, "ceresdata", "trapinfo",
					ena, fmri, devpath, DISK_TRAP_LOSTED);
				syslog(LOG_ERR, "open failed, set node failed, lpath:<%s>", lpath);
			}
		} else {
			/*disk_fault_execute_action(rpath, lpath, tpath, slot_id);*/
		}
		
#ifdef DT_XML
		xyses_new_disk_xml_node(devpath, rpath, lpath, tpath,
			en_id, slot_id, (SELFTESTFAIL | OVERTEMPFAIL | PREDICTIVEFAIL));
#endif
	
		nvlist_free(in);
		nvlist_free(fmri);
		nvlist_free(tfmri);
		topo_hdl_strfree(thp, fmristr);
		topo_hdl_strfree(thp, tfmristr);
		xyses_free_node_info(devpath, rpath, lpath, tpath,
			en_id, slot_id, product_id);
		
		return TOPO_WALK_NEXT;
	}else if(ret == -1){
		if((fru = topo_fru_setime(tfmristr, SXML_UNAVAI,
			lpath, slot_id, en_id, product_id)) != NULL) {
			dmp->dm_update_topo = B_TRUE;
			if (fru->err_count > 0) {
				/*disk_fault_execute_action(rpath, lpath, tpath, slot_id);*/
				dt_send_snmptrap(dmp->dm_hdl, dmp->dm_xprt, "ceresdata", "trapinfo",
					ena, fmri, devpath, DISK_TRAP_LOSTED);
				syslog(LOG_ERR, "open fail, set node failed, lpath:<%s>", lpath);
			}
		} else {
			/*disk_fault_execute_action(rpath, lpath, tpath, slot_id);*/
		}

#ifdef DT_XML
		xyses_new_disk_xml_node(devpath, rpath, lpath, tpath,
			en_id, slot_id, (SELFTESTFAIL | OVERTEMPFAIL | PREDICTIVEFAIL));
#endif
		
		nvlist_free(in);
		nvlist_free(fmri);
		nvlist_free(tfmri);
		topo_hdl_strfree(thp, fmristr);
		topo_hdl_strfree(thp, tfmristr);
		xyses_free_node_info(devpath, rpath, lpath, tpath, 
			en_id, slot_id, product_id);
		
		return TOPO_WALK_NEXT;
	}
	nvlist_free(in);
	nvlist_free(tfmri);
	if ((fru = topo_fru_cleartime(tfmristr, SXML_UNAVAI)) != NULL) {
		dmp->dm_update_topo = B_TRUE;
	}
	topo_hdl_strfree(thp, tfmristr);

	if(nvlist_lookup_nvlist(result, "faults", &faults) == 0 &&
		nvlist_lookup_string(result, "protocol", &protocol) == 0){

		elem = NULL;
		while((elem = nvlist_next_nvpair(faults, elem)) != NULL){

			if (nvpair_type(elem) != DATA_TYPE_BOOLEAN_VALUE)
				continue;

			nvpair_value_boolean_value(elem, &fault);
			if(!fault || nvlist_lookup_nvlist(result, nvpair_name(elem), &details) != 0)
			    continue;

			if (topo_node_asru(node, &asru, NULL, &err) != 0) {
				syslog(LOG_ERR, "failed to get asru: %s\n", topo_strerror(err));
				continue;
			}

			if(strcmp(FM_EREPORT_SCSI_TESTFAIL, nvpair_name(elem)) == 0)
				fault_occur = fault_occur | SELFTESTFAIL;
			else if(strcmp(FM_EREPORT_SCSI_OVERTEMP, nvpair_name(elem)) == 0)
				fault_occur = fault_occur | OVERTEMPFAIL;
			else
				fault_occur = fault_occur | PREDICTIVEFAIL;

#ifdef ADD_SNMP_TRAP
			dt_send_snmptrap(dmp->dm_hdl, dmp->dm_xprt, "ceresdata", "trapinfo", ena, fmri, devpath, nvpair_name(elem));
#endif
			/*disk_fault_execute_action(rpath, lpath, tpath, slot_id);*/
			dt_post_ereport(dmp->dm_hdl, dmp->dm_xprt, protocol, nvpair_name(elem), ena, fmri, details, asru);
			nvlist_free(asru);
		}
	}

#ifdef DT_XML
	xyses_new_disk_xml_node(devpath, rpath, lpath, tpath,
		en_id, slot_id, fault_occur);
#endif

	nvlist_free(result);
	nvlist_free(fmri);
	topo_hdl_strfree(thp, fmristr);
	xyses_free_node_info(devpath, rpath, lpath, tpath,
		en_id, slot_id, product_id);
	return TOPO_WALK_NEXT;
}

static void
dt_timeout(fmd_hdl_t *hdl, id_t id, void *data)
{
	topo_hdl_t *thp;
	topo_walk_t *twp;
	int ret, err;
	disk_monitor_t *dmp = fmd_hdl_getspecific(hdl);
#ifdef DT_XML
	xmlDocPtr doc = xmlNewDoc(BAD_CAST"1.0");

	/* configure root node */
	xml_root_node = xmlNewNode(NULL, BAD_CAST"root");
	xmlDocSetRootElement(doc, xml_root_node);
#endif
	dmp->dm_hdl = hdl;
	dmp->dm_update_topo = B_FALSE;

	thp = fmd_hdl_topo_hold(hdl, TOPO_VERSION);
	syslog(LOG_ERR, "dt_update_disk_info");
	dt_update_disk_info(hdl, thp);
	if ((twp = topo_walk_init(thp, FM_FMRI_SCHEME_HC, dt_analyze_disk,
	    dmp, &err)) == NULL) {
		fmd_hdl_topo_rele(hdl, thp);
		fmd_hdl_error(hdl, "failed to get topology: %s\n",
		    topo_strerror(err));
		syslog(LOG_ERR, "dt timeout analyze disk failed to get topo");
#ifdef DT_XML
		xmlFreeDoc(doc);
#endif
		return;
	}

	if (topo_walk_step(twp, TOPO_WALK_CHILD) == TOPO_WALK_ERR) {
		topo_walk_fini(twp);
		fmd_hdl_topo_rele(hdl, thp);
		fmd_hdl_error(hdl, "failed to walk topology\n");
		syslog(LOG_ERR, "dt timeout analyze disk failed to walk topo");
#ifdef DT_XML
		xmlFreeDoc(doc);
#endif
		return;
	}

	topo_walk_fini(twp);

	/* save disk status xml */
#ifdef DT_XML
	ret = xmlSaveFormatFileEnc("/tmp/fmd_disk.xml", doc, "UTF-8", 1);
	if(ret == -1)
		syslog(LOG_ERR, " disk trans save xml file failed");
	xmlFreeDoc(doc);
#endif
	
	dmp->dm_timer = fmd_timer_install(hdl, NULL, NULL, dmp->dm_interval);
	dmp->dm_timer_istopo = B_FALSE;
	if (dmp->dm_update_topo == B_TRUE) {
		dt_topo_change(hdl, thp);
		syslog(LOG_ERR, "dt_topo_change\n");
	}
	
	fmd_hdl_topo_rele(hdl, thp);
}

static void
dt_topo_change(fmd_hdl_t *hdl, topo_hdl_t *thp)
{
	static hrtime_t time = 0;
	hrtime_t time1;
	uint64_t deleta;
	disk_monitor_t *dmp = fmd_hdl_getspecific(hdl);

	time1 = gethrtime();
	deleta = time1 - time;

	/*4s*/
	if (deleta > 4000000000ull) {
		time = time1;
		dt_update_disk_info(hdl, thp);
	}

	if (dmp->dm_timer_istopo)
		return;

	fmd_timer_remove(hdl, dmp->dm_timer);
	dmp->dm_timer = fmd_timer_install(hdl, NULL, NULL,
	    fmd_prop_get_int64(hdl, "min-interval"));
	dmp->dm_timer_istopo = B_TRUE;
}

static void
dt_insert_disk(fmd_hdl_t *hdl, dt_disk_node_t *dnode)
{
    disk_monitor_t *dmp = fmd_hdl_getspecific(hdl);

    if (dmp->dm_dbhdl == NULL)
        return;

	(void) disk_db_exec_ext(dmp->dm_dbhdl,
                            "INSERT INTO record_t (id,sn,vendor,nblocks,blksize,gsize,dim,enid,slotid,rpm,status)"
                            " VALUES ('%s','%s','%s','%s','%s',%s,'%s',%d,%d,%d,'%s')",
                            dnode->dt_lpath, dnode->dt_serial, dnode->dt_mfg,
                            "-", "-",
                            dnode->dt_gsize, dnode->dt_dim,
                            dnode->dt_enc, dnode->dt_slot,
                            dnode->dt_rpm,dnode->dt_busy);
}

static int
dt_get_scsi_rpm( disk_info_t *disk ) {
	char *path = disk->dk_name ;
	uint8_t output[ INQ_REPLY_LEN ] ;
	uint8_t cmd[] = {0x12, 1, 0xB1, 0, INQ_REPLY_LEN, 0 } ;
	int fd ;
	sg_io_hdr_t io_hdr ;

	if( (fd = open( path, O_RDONLY ) ) == -1 ) {
		fprintf( stderr, "in %s[%d]: cann't open dev<%s> for reading, error( %s )\n", __func__, __LINE__, path, strerror( errno ) ) ;
		return (-1);
	}


	memset( &io_hdr, 0, sizeof( sg_io_hdr_t ) ) ;
	io_hdr.interface_id = 'S' ;
	io_hdr.dxfer_direction = SG_DXFER_FROM_DEV ;
	io_hdr.dxfer_len = INQ_REPLY_LEN ;
	io_hdr.dxferp = output ;
	io_hdr.cmdp = cmd ;
	io_hdr.cmd_len = 6 ;
	io_hdr.timeout = 1000 ;

	if( ioctl( fd, SG_IO, &io_hdr ) == -1 ) {
		close(fd);
		return (-1);
	}

	if( ( io_hdr.info & SG_INFO_OK_MASK ) != SG_INFO_OK ) {
		close(fd);
		return (-1);
	}

	close(fd);
	disk->dk_rpm = output[4] * 256 + output[5] ;

	return (0);
}


static int
dt_update_disk_info(fmd_hdl_t *hdl, topo_hdl_t *thp)
{
	dt_cbdata_t		dcb;
	topo_list_t listp = {NULL, NULL};
	dt_disk_node_t	dnode;
    disk_monitor_t *dmp = fmd_hdl_getspecific(hdl);
	disk_table_t dt = {0};
	disk_info_t *current = NULL;
	char * scsid = NULL;
	int len = 0;

	dcb.dcb_list = &listp;
	dcb.hdl = hdl;
	dcb.thp = thp;
	dcb.count = 0;

	disk_get_info(&dt);
	
    if(disk_db_init() == 0)
    {
        dmp->dm_dbhdl = disk_db_get();
    }
	else
    {
		return(0);
	}

	for (current = dt.next; current != NULL; current = current->next) {
		dt_get_scsi_rpm( current ) ;
		//dnode->dt_blksize = current->dk_blocks;
		dnode.dt_slot = current->dk_slot;
		dnode.dt_enc = current->dk_enclosure;
		scsid = strstr(current->dk_scsid,"scsi-");
		if(scsid != NULL)
			dnode.dt_lpath = scsid;
		dnode.dt_busy = current->dk_busy;
		len = strlen(current->dk_gsize);
		if(len != 0){
			strncpy(dnode.dt_dim,(char *)((long)current->dk_gsize + len - 1),sizeof(dnode.dt_dim));
			current->dk_gsize[len - 1] = 0;
			strncpy(dnode.dt_gsize,current->dk_gsize,sizeof(dnode.dt_gsize));
		}
		dnode.dt_rpm = current->dk_rpm;
		dnode.dt_serial = current->dk_serial;
		dnode.dt_mfg = current->dk_vendor;
		dt_insert_disk(hdl, &dnode);
	}
	
    disk_db_close(dmp->dm_dbhdl);
    dmp->dm_dbhdl = NULL;

    return (0);
}

static void
dt_recv(fmd_hdl_t *hdl, fmd_event_t *ep, nvlist_t *nvl, const char *class)
{
	char vdev_type[32] = {0};
	char vdev_path[128] = {0};
	char buff[512] = {0};
	char hostname[128] = {0};
	if (gethostname(hostname, sizeof(hostname)) < 0) {
		syslog(LOG_ERR, "get hostname failed\n");
	}
	
	if(strcmp(class,"ereport.fs.zfs.vdev.removed") == 0){
		nvlist_lookup_string(nvl,"vdev_type",&vdev_type);
		if(strcmp(vdev_type,"disk") == 0){
			nvlist_lookup_string(nvl,"vdev_path",&vdev_path);

			cm_alarm_cmd(buff, "%s %d \"%s,%s,-,-,-\"",
                             "report", AMARM_ID_DISK_LOST,
                             hostname, vdev_path);
		}
	}
	return;
}


static const fmd_prop_t fmd_props[] = {
	{ "interval", FMD_TYPE_TIME, "10min" },
	{ "min-interval", FMD_TYPE_TIME, "1min" },
	{ "simulate", FMD_TYPE_STRING, "" },
	{ NULL, 0, NULL }
};

static const fmd_hdl_ops_t fmd_ops = {
	dt_recv,			/* fmdo_recv */
	dt_timeout,		/* fmdo_timeout */
	NULL, 			/* fmdo_close */
	NULL,			/* fmdo_stats */
	NULL,			/* fmdo_gc */
	NULL,			/* fmdo_send */
	dt_topo_change,		/* fmdo_topo_change */
};

static const fmd_hdl_info_t fmd_info = {
	"Disk Transport Agent", "1.0", &fmd_ops, fmd_props
};

void _fmd_init(fmd_hdl_t *hdl)
{
	disk_monitor_t *dmp;
	char *simulate;


	if (fmd_hdl_register(hdl, FMD_API_VERSION, &fmd_info) != 0)
		return;

	(void) fmd_stat_create(hdl, FMD_STAT_NOALLOC,
		sizeof (dt_stats) / sizeof (fmd_stat_t),
		(fmd_stat_t *)&dt_stats);

	dmp = fmd_hdl_zalloc(hdl, sizeof (disk_monitor_t), FMD_SLEEP);
	fmd_hdl_setspecific(hdl, dmp);

	dmp->dm_xprt = fmd_xprt_open(hdl, FMD_XPRT_RDONLY, NULL, NULL);
	dmp->dm_interval = fmd_prop_get_int64(hdl, "interval");

	/*
	 * Determine if we have the simulate property set.	This property allows
	 * the developer to substitute a faulty device based off all or part of
	 * an FMRI string.	For example, one could do:
	 *
	 *	setprop simulate "bay=4/disk=4	/path/to/sim.so"
	 *
	 * When the transport module encounters an FMRI containing the given
	 * string, then it will open the simulator file instead of the
	 * corresponding device.  This can be any file, but is intended to be a
	 * libdiskstatus simulator shared object, capable of faking up SCSI
	 * responses.
	 *
	 * The property consists of two strings, an FMRI fragment and an
	 * absolute path, separated by whitespace.
	 */
	simulate = fmd_prop_get_string(hdl, "simulate");
	if (simulate[0] != '\0') {
		const char *sep;
		size_t len;

		for (sep = simulate; *sep != '\0'; sep++) {
			if (isspace(*sep))
				break;
		}

		if (*sep != '\0') {
			len = sep - simulate;

			dmp->dm_sim_search = fmd_hdl_alloc(hdl,
				len + 1, FMD_SLEEP);
			(void) memcpy(dmp->dm_sim_search, simulate, len);
			dmp->dm_sim_search[len] = '\0';
		}

		for (; *sep != '\0'; sep++) {
			if (!isspace(*sep))
				break;
		}

		if (*sep != '\0') {
			dmp->dm_sim_file = fmd_hdl_strdup(hdl, sep, FMD_SLEEP);
		} else if (dmp->dm_sim_search) {
			fmd_hdl_strfree(hdl, dmp->dm_sim_search);
			dmp->dm_sim_search = NULL;
		}
	}
	fmd_prop_free_string(hdl, simulate);

	/*
	 * Call our initial timer routine.	This will do an initial check of all
	 * the disks, and then start the periodic timeout.
	 */
	dmp->dm_timer = fmd_timer_install(hdl, NULL, NULL, 0);

}

void
_fmd_fini(fmd_hdl_t *hdl)
{
	disk_monitor_t *dmp;

#ifdef LHL_DBG
	if(log_file)
		fclose(log_file);
#endif

	dmp = fmd_hdl_getspecific(hdl);
	if (dmp) {
		fmd_xprt_close(hdl, dmp->dm_xprt);
		fmd_hdl_strfree(hdl, dmp->dm_sim_search);
		fmd_hdl_strfree(hdl, dmp->dm_sim_file);
		fmd_hdl_free(hdl, dmp, sizeof (*dmp));
	}
}
