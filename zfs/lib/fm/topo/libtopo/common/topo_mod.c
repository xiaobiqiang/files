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

/*
 * Topology Plugin Modules
 *
 * Topology plugin modules are shared libraries that are dlopen'd and
 * used to enumerate resources in the system and export per-node method
 * operations.
 *
 * They are loaded by our builtin scheme-specific plugins, other modules or
 * by processing a topo map XML file to enumerate and create nodes for
 * resources that are present in the system.  They may also export a set of
 * topology node specific methods that can be invoked directly via
 * topo_method_invoke() or indirectly via the
 * topo_prop_get* family of functions to access dynamic property data.
 *
 * Module Plugin API
 *
 * Enumerators must provide entry points for initialization and clean-up
 * (_topo_init() and _topo_fini()).  In their _topo_init() function, an
 * enumerator should register (topo_mod_register()) its enumeration callback
 * and allocate resources required for a subsequent call to the callback.
 * Optionally, methods may also be registered with topo_method_register().
 *
 * In its enumeration callback routine, the module should search for resources
 * within its realm of responsibility and create any node ranges,
 * topo_node_range_create() and nodes, topo_node_bind().  The Enumerator
 * module is handed a node to which it may begin attaching additional
 * topology nodes.  The enumerator may only access those nodes within its
 * current scope of operation: the node passed into its enumeration op and
 * any nodes it creates during enumeration.  If the enumerator requires walker-
 * style access to these nodes, it must use
 * topo_mod_walk_init()/topo_walk_step()/topo_walk_fini().
 *
 * If additional helper modules need to be loaded to complete the enumeration
 * the module may do so by calling topo_mod_load().  Enumeration may then
 * continue with the module handing off enumeration to its helper module
 * by calling topo_mod_enumerate().  Similarly, a module may call
 * topo_mod_enummap() to kick-off enumeration according to a given XML
 * topology map file.  A module *may* not cause re-entrance to itself
 * via either of these interfaces.  If re-entry is detected an error
 * will be returned (ETOPO_ENUM_RECURS).
 *
 * If the module registers a release callback, it will be called on a node
 * by node basis during topo_snap_rele().  Any private node data may be
 * deallocated or methods unregistered at that time.  Global module data
 * should be cleaned up before or at the time that the module _topo_fini
 * entry point is called.
 *
 * Module entry points and method invocations are guaranteed to be
 * single-threaded for a given snapshot handle.  Applications may have
 * more than one topology snapshot open at a time.  This means that the
 * module operations and methods may be called for different module handles
 * (topo_mod_t) asynchronously.  The enumerator should not use static or
 * global data structures that may become inconsistent in this situation.
 * Method operations may be re-entrant if the module invokes one of its own
 * methods directly or via dynamic property access.  Caution should be
 * exercised with method operations to insure that data remains consistent
 * within the module and that deadlocks can not occur.
 */

#include <pthread.h>
#include <assert.h>
#include <errno.h>
#include <dirent.h>
#include <limits.h>
#include <alloca.h>
#include <unistd.h>
#include <stdio.h>
#include <syslog.h>
#include <ctype.h>
#include <sys/param.h>
#include <sys/utsname.h>
//#include <sys/smbios.h>
#include <sys/fm/protocol.h>
#include <sys/types.h>
#include <fcntl.h>
#include <topo_protocol.h>
#include <topo_alloc.h>
#include <topo_error.h>
#include <topo_file.h>
#include <topo_fmri.h>
#include <topo_module.h>
#include <topo_method.h>
#include <topo_string.h>
#include <topo_subr.h>
#include <topo_tree.h>
#include <topo_fruhash.h>

#define	PLUGIN_PATH	"plugins"
#define	PLUGIN_PATH_LEN	MAXNAMELEN + 5

topo_mod_t *
topo_mod_load(topo_mod_t *pmod, const char *name,
    topo_version_t version)
{
	char *path = NULL;
	char file[PLUGIN_PATH_LEN];
	topo_mod_t *mod = NULL;
	topo_hdl_t *thp;
	thp = pmod->tm_hdl;
	/*
	 * Already loaded, topo_mod_lookup will bump the ref count
	 */
	if ((mod = topo_mod_lookup(thp, name, 1)) != NULL) {
		if (mod->tm_info->tmi_version != version) {
			topo_mod_rele(mod);
			(void) topo_mod_seterrno(pmod, ETOPO_MOD_VER);
			return (NULL);
		}
		return (mod);
	}

	(void) snprintf(file, PLUGIN_PATH_LEN, "%s/lib%s.so",
	    PLUGIN_PATH, name);
	path = topo_search_path(pmod, thp->th_rootdir, (const char *)file);
	if (path == NULL ||
	    (mod = topo_modhash_load(thp, name, path, &topo_rtld_ops, version))
	    == NULL) { /* returned with mod held */
			topo_mod_strfree(pmod, path);
			(void) topo_mod_seterrno(pmod, topo_hdl_errno(thp) ?
			    topo_hdl_errno(thp) : ETOPO_MOD_NOENT);
			return (NULL);
	}

	topo_mod_strfree(pmod, path);

	return (mod);
}

void
topo_mod_unload(topo_mod_t *mod)
{
	topo_mod_rele(mod);
}

static int
set_register_error(topo_mod_t *mod, int err)
{
	if (mod->tm_info != NULL)
		topo_mod_unregister(mod);

	topo_dprintf(mod->tm_hdl, TOPO_DBG_ERR,
	    "module registration failed for %s: %s\n",
	    mod->tm_name, topo_strerror(err));

	return (topo_mod_seterrno(mod, err));
}

int
topo_mod_register(topo_mod_t *mod, const topo_modinfo_t *mip,
    topo_version_t version)
{

	assert(!(mod->tm_flags & TOPO_MOD_FINI ||
	    mod->tm_flags & TOPO_MOD_REG));

	if (version != TOPO_VERSION)
		return (set_register_error(mod, EMOD_VER_ABI));

	if ((mod->tm_info = topo_mod_zalloc(mod, sizeof (topo_imodinfo_t)))
	    == NULL)
		return (set_register_error(mod, EMOD_NOMEM));
	if ((mod->tm_info->tmi_ops = topo_mod_alloc(mod,
	    sizeof (topo_modops_t))) == NULL)
		return (set_register_error(mod, EMOD_NOMEM));

	mod->tm_info->tmi_desc = topo_mod_strdup(mod, mip->tmi_desc);
	if (mod->tm_info->tmi_desc == NULL)
		return (set_register_error(mod, EMOD_NOMEM));

	mod->tm_info->tmi_scheme = topo_mod_strdup(mod, mip->tmi_scheme);
	if (mod->tm_info->tmi_scheme == NULL)
		return (set_register_error(mod, EMOD_NOMEM));


	mod->tm_info->tmi_version = (topo_version_t)mip->tmi_version;

	mod->tm_info->tmi_ops->tmo_enum = mip->tmi_ops->tmo_enum;

	mod->tm_info->tmi_ops->tmo_release = mip->tmi_ops->tmo_release;

	mod->tm_flags |= TOPO_MOD_REG;

	topo_dprintf(mod->tm_hdl, TOPO_DBG_MODSVC,
	    "registration succeeded for %s\n", mod->tm_name);

	return (0);
}

void
topo_mod_unregister(topo_mod_t *mod)
{
	if (mod->tm_info == NULL)
		return;

	assert(!(mod->tm_flags & TOPO_MOD_FINI));

	mod->tm_flags &= ~TOPO_MOD_REG;

	if (mod->tm_info == NULL)
		return;

	if (mod->tm_info->tmi_ops != NULL)
		topo_mod_free(mod, mod->tm_info->tmi_ops,
		    sizeof (topo_modops_t));
	if (mod->tm_info->tmi_desc != NULL)
		topo_mod_strfree(mod, mod->tm_info->tmi_desc);
	if (mod->tm_info->tmi_scheme != NULL)
		topo_mod_strfree(mod, mod->tm_info->tmi_scheme);

	topo_mod_free(mod, mod->tm_info, sizeof (topo_imodinfo_t));

	mod->tm_info = NULL;
}

int
topo_mod_enumerate(topo_mod_t *mod, tnode_t *node, const char *enum_name,
    const char *name, topo_instance_t min, topo_instance_t max, void *data)
{
	int err = 0;
	topo_mod_t *enum_mod;

	assert(mod->tm_flags & TOPO_MOD_REG);

	if ((enum_mod = topo_mod_lookup(mod->tm_hdl, enum_name, 0)) == NULL)
		return (topo_mod_seterrno(mod, EMOD_MOD_NOENT));

	topo_node_hold(node);

	topo_dprintf(mod->tm_hdl, TOPO_DBG_MODSVC, "module %s enumerating "
	    "node %s=%d\n", (char *)mod->tm_name, (char *)node->tn_name,
	    node->tn_instance);

	topo_mod_enter(enum_mod);
	err = enum_mod->tm_info->tmi_ops->tmo_enum(enum_mod, node, name, min,
	    max, enum_mod->tm_priv, data);
	topo_mod_exit(enum_mod);

	if (err != 0) {
		(void) topo_mod_seterrno(mod, EMOD_UKNOWN_ENUM);

		topo_dprintf(mod->tm_hdl, TOPO_DBG_ERR,
		    "module %s failed enumeration for "
		    " node %s=%d\n", (char *)mod->tm_name,
		    (char *)node->tn_name, node->tn_instance);

		topo_node_rele(node);
		return (-1);
	}

	topo_node_rele(node);

	return (0);
}

int
topo_mod_enummap(topo_mod_t *mod, tnode_t *node, const char *name,
    const char *scheme)
{
	return (topo_file_load(mod, node, (char *)name, (char *)scheme, 0));
}

static nvlist_t *
set_fmri_err(topo_mod_t *mod, int err)
{
	(void) topo_mod_seterrno(mod, err);
	return (NULL);
}

nvlist_t *
topo_mod_hcfmri(topo_mod_t *mod, tnode_t *pnode, int version, const char *name,
    topo_instance_t inst, nvlist_t *hc_specific, nvlist_t *auth,
    const char *part, const char *rev, const char *serial)
{
	int err;
	nvlist_t *pfmri = NULL, *fmri = NULL, *args = NULL;
	nvlist_t *nfp = NULL;
	char *lpart, *lrev, *lserial;

	if (version != FM_HC_SCHEME_VERSION)
		return (set_fmri_err(mod, EMOD_FMRI_VERSION));

	/*
	 * Do we have any args to pass?
	 */
	if (pnode != NULL || auth != NULL || part != NULL || rev != NULL ||
	    serial != NULL || hc_specific != NULL) {
		if (topo_mod_nvalloc(mod, &args, NV_UNIQUE_NAME) != 0)
			return (set_fmri_err(mod, EMOD_FMRI_NVL));
	}

	if (pnode != NULL) {
		if (topo_node_resource(pnode, &pfmri, &err) < 0) {
			nvlist_free(args);
			return (set_fmri_err(mod, EMOD_NVL_INVAL));
		}

		if (nvlist_add_nvlist(args, TOPO_METH_FMRI_ARG_PARENT,
		    pfmri) != 0) {
			nvlist_free(pfmri);
			nvlist_free(args);
			return (set_fmri_err(mod, EMOD_FMRI_NVL));
		}
		nvlist_free(pfmri);
	}

	/*
	 * Add optional payload
	 */
	if (auth != NULL)
		(void) nvlist_add_nvlist(args, TOPO_METH_FMRI_ARG_AUTH, auth);
	if (part != NULL) {
		lpart = topo_cleanup_auth_str(mod->tm_hdl, part);
		if (lpart != NULL) {
			(void) nvlist_add_string(args, TOPO_METH_FMRI_ARG_PART,
			    lpart);
			topo_hdl_free(mod->tm_hdl, lpart, strlen(lpart) + 1);
		} else {
			(void) nvlist_add_string(args, TOPO_METH_FMRI_ARG_PART,
			    "");
		}
	}
	if (rev != NULL) {
		lrev = topo_cleanup_auth_str(mod->tm_hdl, rev);
		if (lrev != NULL) {
			(void) nvlist_add_string(args, TOPO_METH_FMRI_ARG_REV,
			    lrev);
			topo_hdl_free(mod->tm_hdl, lrev, strlen(lrev) + 1);
		} else {
			(void) nvlist_add_string(args, TOPO_METH_FMRI_ARG_REV,
			    "");
		}
	}
	if (serial != NULL) {
		lserial = topo_cleanup_auth_str(mod->tm_hdl, serial);
		if (lserial != NULL) {
			(void) nvlist_add_string(args, TOPO_METH_FMRI_ARG_SER,
			    lserial);
			topo_hdl_free(mod->tm_hdl, lserial,
			    strlen(lserial) + 1);
		} else {
			(void) nvlist_add_string(args, TOPO_METH_FMRI_ARG_SER,
			    "");
		}
	}
	if (hc_specific != NULL)
		(void) nvlist_add_nvlist(args, TOPO_METH_FMRI_ARG_HCS,
		    hc_specific);

	if ((fmri = topo_fmri_create(mod->tm_hdl, FM_FMRI_SCHEME_HC, name, inst,
	    args, &err)) == NULL) {
		nvlist_free(args);
		return (set_fmri_err(mod, err));
	}

	nvlist_free(args);

	(void) topo_mod_nvdup(mod, fmri, &nfp);
	nvlist_free(fmri);

	return (nfp);
}

nvlist_t *
topo_mod_devfmri(topo_mod_t *mod, int version, const char *dev_path,
    const char *devid)
{
	int err;
	nvlist_t *fmri, *args;
	nvlist_t *nfp = NULL;

	if (version != FM_DEV_SCHEME_VERSION)
		return (set_fmri_err(mod, EMOD_FMRI_VERSION));

	if (topo_mod_nvalloc(mod, &args, NV_UNIQUE_NAME) != 0)
		return (set_fmri_err(mod, EMOD_FMRI_NVL));

	if (nvlist_add_string(args, FM_FMRI_DEV_PATH, dev_path) != 0) {
		nvlist_free(args);
		return (set_fmri_err(mod, EMOD_FMRI_NVL));
	}

	(void) nvlist_add_string(args, FM_FMRI_DEV_ID, devid);

	if ((fmri = topo_fmri_create(mod->tm_hdl, FM_FMRI_SCHEME_DEV,
	    FM_FMRI_SCHEME_DEV, 0, args, &err)) == NULL) {
		nvlist_free(args);
		return (set_fmri_err(mod, err));
	}

	nvlist_free(args);

	(void) topo_mod_nvdup(mod, fmri, &nfp);
	nvlist_free(fmri);

	return (nfp);
}

nvlist_t *
topo_mod_cpufmri(topo_mod_t *mod, int version, uint32_t cpu_id, uint8_t cpumask,
    const char *serial)
{
	int err;
	nvlist_t *fmri = NULL, *args = NULL;
	nvlist_t *nfp = NULL;

	if (version != FM_CPU_SCHEME_VERSION)
		return (set_fmri_err(mod, EMOD_FMRI_VERSION));

	if (topo_mod_nvalloc(mod, &args, NV_UNIQUE_NAME) != 0)
		return (set_fmri_err(mod, EMOD_FMRI_NVL));

	if (nvlist_add_uint32(args, FM_FMRI_CPU_ID, cpu_id) != 0) {
		nvlist_free(args);
		return (set_fmri_err(mod, EMOD_FMRI_NVL));
	}

	/*
	 * Add optional payload
	 */
	(void) nvlist_add_uint8(args, FM_FMRI_CPU_MASK, cpumask);
	(void) nvlist_add_string(args, FM_FMRI_CPU_SERIAL_ID, serial);

	if ((fmri = topo_fmri_create(mod->tm_hdl, FM_FMRI_SCHEME_CPU,
	    FM_FMRI_SCHEME_CPU, 0, args, &err)) == NULL) {
		nvlist_free(args);
		return (set_fmri_err(mod, err));
	}

	nvlist_free(args);

	(void) topo_mod_nvdup(mod, fmri, &nfp);
	nvlist_free(fmri);

	return (nfp);
}

nvlist_t *
topo_mod_memfmri(topo_mod_t *mod, int version, uint64_t pa, uint64_t offset,
	const char *unum, int flags)
{
	int err;
	nvlist_t *args = NULL, *fmri = NULL;
	nvlist_t *nfp = NULL;

	if (version != FM_MEM_SCHEME_VERSION)
		return (set_fmri_err(mod, EMOD_FMRI_VERSION));

	if (topo_mod_nvalloc(mod, &args, NV_UNIQUE_NAME) != 0)
		return (set_fmri_err(mod, EMOD_FMRI_NVL));

	err = nvlist_add_string(args, FM_FMRI_MEM_UNUM, unum);
		nvlist_free(args);
	if (flags & TOPO_MEMFMRI_PA)
		err |= nvlist_add_uint64(args, FM_FMRI_MEM_PHYSADDR, pa);
	if (flags & TOPO_MEMFMRI_OFFSET)
		err |= nvlist_add_uint64(args, FM_FMRI_MEM_OFFSET, offset);

	if (err != 0) {
		nvlist_free(args);
		return (set_fmri_err(mod, EMOD_FMRI_NVL));
	}

	if ((fmri = topo_fmri_create(mod->tm_hdl, FM_FMRI_SCHEME_MEM,
	    FM_FMRI_SCHEME_MEM, 0, args, &err)) == NULL) {
		nvlist_free(args);
		return (set_fmri_err(mod, err));
	}

	nvlist_free(args);

	(void) topo_mod_nvdup(mod, fmri, &nfp);
	nvlist_free(fmri);

	return (nfp);

}

nvlist_t *
topo_mod_pkgfmri(topo_mod_t *mod, int version, const char *path)
{
	int err;
	nvlist_t *fmri = NULL, *args = NULL;
	nvlist_t *nfp = NULL;

	if (version != FM_PKG_SCHEME_VERSION)
		return (set_fmri_err(mod, EMOD_FMRI_VERSION));

	if (topo_mod_nvalloc(mod, &args, NV_UNIQUE_NAME) != 0)
		return (set_fmri_err(mod, EMOD_FMRI_NVL));

	if (nvlist_add_string(args, "path", path) != 0) {
		nvlist_free(args);
		return (set_fmri_err(mod, EMOD_FMRI_NVL));
	}

	if ((fmri = topo_fmri_create(mod->tm_hdl, FM_FMRI_SCHEME_PKG,
	    FM_FMRI_SCHEME_PKG, 0, args, &err)) == NULL) {
		nvlist_free(args);
		return (set_fmri_err(mod, err));
	}

	nvlist_free(args);

	(void) topo_mod_nvdup(mod, fmri, &nfp);
	nvlist_free(fmri);

	return (nfp);
}

nvlist_t *
topo_mod_modfmri(topo_mod_t *mod, int version, const char *driver)
{
	int err;
	nvlist_t *fmri = NULL, *args = NULL;
	nvlist_t *nfp = NULL;

	if (version != FM_MOD_SCHEME_VERSION)
		return (set_fmri_err(mod, EMOD_FMRI_VERSION));

	if (topo_mod_nvalloc(mod, &args, NV_UNIQUE_NAME) != 0)
		return (set_fmri_err(mod, EMOD_FMRI_NVL));

	if (nvlist_add_string(args, "DRIVER", driver) != 0) {
		nvlist_free(args);
		return (set_fmri_err(mod, EMOD_FMRI_NVL));
	}

	if ((fmri = topo_fmri_create(mod->tm_hdl, FM_FMRI_SCHEME_MOD,
	    FM_FMRI_SCHEME_MOD, 0, args, &err)) == NULL) {
		nvlist_free(args);
		return (set_fmri_err(mod, err));
	}

	nvlist_free(args);

	(void) topo_mod_nvdup(mod, fmri, &nfp);
	nvlist_free(fmri);

	return (nfp);
}

int
topo_mod_str2nvl(topo_mod_t *mod, const char *fmristr, nvlist_t **fmri)
{
	int err;
	nvlist_t *np = NULL;

	if (topo_fmri_str2nvl(mod->tm_hdl, fmristr, &np, &err) < 0)
		return (topo_mod_seterrno(mod, err));

	if (topo_mod_nvdup(mod, np, fmri) < 0) {
		nvlist_free(np);
		return (topo_mod_seterrno(mod, EMOD_FMRI_NVL));
	}

	nvlist_free(np);

	return (0);
}

int
topo_mod_nvl2str(topo_mod_t *mod, nvlist_t *fmri, char **fmristr)
{
	int err;
	char *sp;

	if (topo_fmri_nvl2str(mod->tm_hdl, fmri, &sp, &err) < 0)
		return (topo_mod_seterrno(mod, err));

	if ((*fmristr = topo_mod_strdup(mod, sp)) == NULL) {
		topo_hdl_strfree(mod->tm_hdl, sp);
		return (topo_mod_seterrno(mod, EMOD_NOMEM));
	}

	topo_hdl_strfree(mod->tm_hdl, sp);

	return (0);
}

void *
topo_mod_getspecific(topo_mod_t *mod)
{
	return (mod->tm_priv);
}

void
topo_mod_setspecific(topo_mod_t *mod, void *data)
{
	mod->tm_priv = data;
}

void
topo_mod_setdebug(topo_mod_t *mod)
{
	mod->tm_debug = 1;
}

static ipmi_handle_t *
ipmi_open(int *errp, char **msg)
{
	ipmi_handle_t *ihp = NULL;

	return (ihp);
}


ipmi_handle_t *
topo_mod_ipmi_hold(topo_mod_t *mod)
{
	topo_hdl_t *thp = mod->tm_hdl;
	int err;
	char *errmsg;

	(void) pthread_mutex_lock(&thp->th_ipmi_lock);
	if (thp->th_ipmi == NULL) {
		if ((thp->th_ipmi = ipmi_open(&err, &errmsg)) == NULL) {
			topo_dprintf(mod->tm_hdl, TOPO_DBG_ERR,
			    "ipmi_open() failed: %s (ipmi errno=%d)", errmsg,
			    err);
			(void) pthread_mutex_unlock(&thp->th_ipmi_lock);
		}
	}

	return (thp->th_ipmi);
}

void
topo_mod_ipmi_rele(topo_mod_t *mod)
{
	topo_hdl_t *thp = mod->tm_hdl;

	(void) pthread_mutex_unlock(&thp->th_ipmi_lock);
}

di_node_t
topo_mod_devinfo(topo_mod_t *mod)
{
	return (topo_hdl_devinfo(mod->tm_hdl));
}
#if 0
smbios_hdl_t *
topo_mod_smbios(topo_mod_t *mod)
{
	topo_hdl_t *thp = mod->tm_hdl;

	if (thp->th_smbios == NULL)
		thp->th_smbios = smbios_open(NULL, SMB_VERSION, 0, NULL);

	return (thp->th_smbios);
}
#endif
di_prom_handle_t
topo_mod_prominfo(topo_mod_t *mod)
{
	return (topo_hdl_prominfo(mod->tm_hdl));
}

void
topo_mod_clrdebug(topo_mod_t *mod)
{
	mod->tm_debug = 0;
}

/*PRINTFLIKE2*/
void
topo_mod_dprintf(topo_mod_t *mod, const char *format, ...)
{
	va_list alist;

	if (mod->tm_debug == 0)
		return;

	va_start(alist, format);
	topo_vdprintf(mod->tm_hdl, TOPO_DBG_MOD, (const char *)mod->tm_name,
	    format, alist);
	va_end(alist);
}

static char *
topo_mod_product(topo_mod_t *mod)
{
	return (topo_mod_strdup(mod, mod->tm_hdl->th_product));
}

static char *
topo_mod_server(topo_mod_t *mod)
{
	static struct utsname uts;

	(void) uname(&uts);
	return (topo_mod_strdup(mod, uts.nodename));
}
#if 0
static char *
topo_mod_psn(topo_mod_t *mod)
{
	smbios_hdl_t *shp;
	const char *psn;

	if ((shp = topo_mod_smbios(mod)) == NULL ||
	    (psn = smbios_psn(shp)) == NULL)
		return (NULL);

	return (topo_cleanup_auth_str(mod->tm_hdl, psn));
}

static char *
topo_mod_csn(topo_mod_t *mod)
{
	char csn[MAXNAMELEN];
	smbios_hdl_t *shp;
	di_prom_handle_t promh = DI_PROM_HANDLE_NIL;
	di_node_t rooth = DI_NODE_NIL;
	const char *bufp;

	if ((shp = topo_mod_smbios(mod)) != NULL) {
		bufp = smbios_csn(shp);
		if (bufp != NULL)
			(void) strlcpy(csn, bufp, MAXNAMELEN);
		else
			return (NULL);
	} else if ((rooth = topo_mod_devinfo(mod)) != DI_NODE_NIL &&
	    (promh = topo_mod_prominfo(mod)) != DI_PROM_HANDLE_NIL) {
		if (di_prom_prop_lookup_bytes(promh, rooth, "chassis-sn",
		    (unsigned char **)&bufp) != -1) {
			(void) strlcpy(csn, bufp, MAXNAMELEN);
		} else {
			return (NULL);
		}
	} else {
		return (NULL);
	}

	return (topo_cleanup_auth_str(mod->tm_hdl, csn));
}
#endif

nvlist_t *
topo_mod_auth(topo_mod_t *mod, tnode_t *pnode)
{
	int err;
	char *prod = NULL;
	char *csn = NULL;
	char *psn = NULL;
	char *server = NULL;
	nvlist_t *auth;

	if ((err = topo_mod_nvalloc(mod, &auth, NV_UNIQUE_NAME)) != 0) {
		(void) topo_mod_seterrno(mod, EMOD_FMRI_NVL);
		return (NULL);
	}

	(void) topo_prop_get_string(pnode, FM_FMRI_AUTHORITY,
	    FM_FMRI_AUTH_PRODUCT, &prod, &err);
	(void) topo_prop_get_string(pnode, FM_FMRI_AUTHORITY,
	    FM_FMRI_AUTH_PRODUCT_SN, &psn, &err);
	(void) topo_prop_get_string(pnode, FM_FMRI_AUTHORITY,
	    FM_FMRI_AUTH_CHASSIS, &csn, &err);
	(void) topo_prop_get_string(pnode, FM_FMRI_AUTHORITY,
	    FM_FMRI_AUTH_SERVER, &server, &err);

	/*
	 * Let's do this the hard way
	 */
	if (prod == NULL)
		prod = topo_mod_product(mod);
#if 0
	if (csn == NULL)
		csn = topo_mod_csn(mod);
	if (psn == NULL)
		psn = topo_mod_psn(mod);
#endif
	if (server == NULL) {
		server = topo_mod_server(mod);
	}

	/*
	 * No luck, return NULL
	 */
	if (!prod && !server && !csn && !psn) {
		nvlist_free(auth);
		return (NULL);
	}

	err = 0;
	if (prod != NULL) {
		err |= nvlist_add_string(auth, FM_FMRI_AUTH_PRODUCT, prod);
		topo_mod_strfree(mod, prod);
	}
	if (psn != NULL) {
		err |= nvlist_add_string(auth, FM_FMRI_AUTH_PRODUCT_SN, psn);
		topo_mod_strfree(mod, psn);
	}
	if (server != NULL) {
		err |= nvlist_add_string(auth, FM_FMRI_AUTH_SERVER, server);
		topo_mod_strfree(mod, server);
	}
	if (csn != NULL) {
		err |= nvlist_add_string(auth, FM_FMRI_AUTH_CHASSIS, csn);
		topo_mod_strfree(mod, csn);
	}

	if (err != 0) {
		nvlist_free(auth);
		(void) topo_mod_seterrno(mod, EMOD_NVL_INVAL);
		return (NULL);
	}

	return (auth);
}

topo_walk_t *
topo_mod_walk_init(topo_mod_t *mod, tnode_t *node, topo_mod_walk_cb_t cb_f,
    void *pdata, int *errp)
{
	topo_walk_t *wp;
	topo_hdl_t *thp = mod->tm_hdl;

	if ((wp = topo_node_walk_init(thp, mod, node, (topo_walk_cb_t)cb_f, pdata,
	    errp)) == NULL)
		return (NULL);

	return (wp);
}

static struct topo_fruhash tp_fruhash;

topo_fruhash_t *
topo_get_fruhash()
{
	return &tp_fruhash;
}


char *
topo_fru_strdup(const char *s, int flags)
{
	char *p;

	if (s != NULL)
		p = topo_zalloc(strlen(s) + 1, flags);
	else
		p = NULL;

	if (p != NULL)
		(void) strcpy(p, s);

	return (p);
}

void
topo_fru_strfree(char *s)
{
	if (s != NULL)
		topo_free(s, strlen(s) + 1);
}


static void
topo_fru_hash_lock(void)
{
	(void) pthread_mutex_lock(&(tp_fruhash.fh_lock));
}

static void
topo_fru_hash_unlock(void)
{
	(void) pthread_mutex_unlock(&(tp_fruhash.fh_lock));
}

topo_fru_t *
topo_fru_hash_lookup(const char *name)
{
	topo_fru_t *fru = NULL;
	uint_t h;

	h = topo_strhash(name) % tp_fruhash.fh_hashlen;

	for (fru = tp_fruhash.fh_hash[h]; fru != NULL; fru = fru->tf_next) {
		if (strcmp(name, fru->tf_name) == 0)
			break;
	}

	return (fru);
}

topo_fru_t *
topo_fru_hash_clear_flag(const char *name)
{
    topo_fru_t *fru = NULL;
    uint_t h;

    for (h = 0; h < TOPO_FRUHASH_BUCKETS; h++) {
        for (fru = tp_fruhash.fh_hash[h]; fru != NULL; fru = fru->tf_next) {
            if (strstr(fru->tf_name, name) != NULL) {
                if (fru->flag == 0) {
                    fru->tf_time = 0;
                }
                fru->flag = 0;
            }
        }
    }

    return (fru);
}

topo_fru_t *
topo_fru_setime(const char *name, int status, char *diskname,
	char *slotid, char *encid, char *product)
{
    topo_fru_t *fru;
    uint_t h;

    if ((fru = topo_fru_hash_lookup(name)) != NULL) {
        if (fru->tf_time == 0) {
            fru->tf_time = time(NULL);
            fru->tf_status |= status;
            fru->nor_count = 0;
            fru->err_count++;
            fru->flag = status; 
            return fru;
        } else if (fru->err_count < 3) {
            fru->nor_count = 0;
            fru->err_count++;
            fru->tf_status |= status;
            fru->flag = status;
            return fru;
        } else {
            fru->tf_status |= status;
            fru->flag = status;
            return NULL;
        }
    }

	topo_fru_hash_lock();
		
	fru = topo_zalloc(sizeof (topo_fru_t), 1);
	fru->tf_name = topo_fru_strdup(name, 1);
	fru->tf_time = time(NULL);
	fru->tf_status |= status;
        fru->flag = status;
	fru->err_count = 1;
	fru->nor_count = 0;

	if (diskname != NULL && slotid !=NULL &&
		encid != NULL) {
		fru->diskname = topo_fru_strdup(diskname, 1);
		fru->slotid = topo_fru_strdup(slotid, 1);
		fru->encid = topo_fru_strdup(encid, 1);
	}
	if (product != NULL)
		fru->product = topo_fru_strdup(product, 1);

	h = topo_strhash(name) % tp_fruhash.fh_hashlen;
	fru->tf_next = tp_fruhash.fh_hash[h];
	tp_fruhash.fh_hash[h] = fru;
	tp_fruhash.fh_nelems++;
	topo_fru_hash_unlock();

	return (fru);
}

topo_fru_t *
topo_fru_cleartime(const char *name, int status)
{
	topo_fru_t *fru;

	if ((fru = topo_fru_hash_lookup(name)) != NULL) {
		if (fru->tf_time != 0) {
			fru->err_count = 0;
			fru->nor_count++;
			fru->tf_status &= (~status);
			if (fru->tf_status == 0)
				fru->tf_time = 0;
			return fru;
		} else if (fru->nor_count < 3) {
			fru->nor_count++;
			fru->err_count = 0;
			return fru;
		} else {
			return NULL;
		}
	}
	return NULL;
}

void
topo_fru_hash_create(void)
{
	tp_fruhash.fh_hashlen = TOPO_FRUHASH_BUCKETS;
	tp_fruhash.fh_nelems = 0;
	tp_fruhash.fh_hash = topo_zalloc(
		sizeof (void *) * TOPO_FRUHASH_BUCKETS, 1);
	(void) pthread_mutex_init(&(tp_fruhash.fh_lock), NULL);
}

void
topo_fru_hash_destroy(void)
{
	int i;
	topo_fruhash_t *fhp = &tp_fruhash;
	topo_fru_t *fp, **pp;
	
	topo_fru_hash_lock();
	for (i = 0; i < TOPO_FRUHASH_BUCKETS; ++i) {
		pp = &fhp->fh_hash[i];
		fp = *pp;
		while (fp != NULL) {

			*pp = fp->tf_next;
			if (fp->tf_name != NULL)
				topo_fru_strfree(fp->tf_name);
			if (fp->diskname != NULL)
				topo_fru_strfree(fp->diskname);
			if (fp->slotid != NULL)
				topo_fru_strfree(fp->slotid);
			if (fp->encid != NULL)
				topo_fru_strfree(fp->encid);
			if (fp->product != NULL)
				topo_fru_strfree(fp->product);
			topo_free(fp, sizeof (topo_fru_t));
			fp = *pp;

			--fhp->fh_nelems;
		}
	}
	topo_fru_hash_unlock();
	topo_free(tp_fruhash.fh_hash, 
		sizeof (void *) * TOPO_FRUHASH_BUCKETS);
}

void
cm_alarm_cmd(char *buf, int size,const char *format, ...)
{
	va_list arg;
	int n = 0;
    char* p = NULL;

	memset(buf, size, 0);
	n = snprintf(buf, CM_ALARM_CMD_LEN, CM_ALARM_CMD_T);
	
	va_start(arg, format);
	vsnprintf(buf + n, CM_ALARM_CMD_LEN, format, arg);
	va_end(arg);

    p = buf;
    while(*p != '\0'){
        if(*p == '\n'){
            *p = ' ';
        }
        p++;
    }
        
	if (system(buf) != 0)
		syslog(LOG_ERR, "system error: %s\n", buf);
}

/*topo alarm xml*/
#define TOPO_FRU_XML_PATH "/var/fm/topo_fru.xml"
static pthread_mutex_t g_topo_fru_xml_lock;
static xmlDocPtr topo_xml_doc;

#define ALARMED "alarm"
#define NON_ALARMED "non-alarm"
#define ISCLEAR 0x1 << 1
#define TISSET 	0x1	<< 2
#define SETCNT	0x1	<< 3

#define INFOMAXLEN 512
#define ISFINELEN(a)\
	(((a) > (INFOMAXLEN))||((a) < (0)))
typedef xmlNodePtr topo_xml_hd_t;

typedef struct xml_node_list{
	int cnt;
	int tocm;
	int alarmid;
	xmlNodePtr xmlNode;
	struct xml_node_list *nodeNext;
}xml_node_list_t;

typedef struct xml_nodes {
	xml_node_list_t * xmlHead;
	int 	   xmlLen;
}xml_nodes_t;

enum{
	TOPO_XML_SUCCESS,
	TOPO_XML_FAULT,
	TOPO_NODE_EXIST,
	TOPO_NODE_NON_EXIST,
	TOPO_SET_INFO_OK,
	TOPO_SET_INFO_FAULT
};

#define HOSTNAMEXML 64

static int topo_fru_xml_exist();
static void topo_fru_xml_node_list_instert(xml_nodes_t *,xmlNodePtr,int,int);


#define TOPO_FRU_XML_IS_EXIST topo_fru_xml_exist()

static xml_node_list_t * topo_fru_xml_alloc()
{
	return (xml_node_list_t *)topo_zalloc(sizeof(xml_node_list_t), 1);
}

static void topo_fru_xml_free(xml_node_list_t * ptr)
{
	topo_free(ptr,sizeof(xml_node_list_t));
}

static void
topo_fru_xml_lock(void)
{
	(void) pthread_mutex_lock(&g_topo_fru_xml_lock);
}

static void
topo_fru_xml_unlock(void)
{
	(void) pthread_mutex_unlock(&g_topo_fru_xml_lock);
}

static void topo_fru_zero_buff(char * info, int size)
{
	memset(info,0,size);
	return;
}

static int topo_fru_is_system_ready(char * hostname)
{
	if(!strncasecmp(hostname,"Prodigy",strlen("Prodigy"))){
		return TOPO_XML_FAULT;
	}

	return TOPO_XML_SUCCESS;
}

static int topo_fru_get_hostname(char * hostname,int len){
	if (gethostname(hostname, len) < 0) {
			return TOPO_XML_FAULT;
	}
	return topo_fru_is_system_ready(hostname);
}

static int
topo_fru_tocm_info_set(char * info, int size,va_list ap)
{
	int ret = TOPO_SET_INFO_OK;
	topo_xml_type_t type;
	char buff[INFOMAXLEN] = {0};
	int maxlen = size - 1;
	char * strp = NULL;
		
	if(ISFINELEN(size))
		return TOPO_XML_FAULT;

	topo_fru_zero_buff(info,size);
	
	while (ret == TOPO_SET_INFO_OK) {
		type = va_arg(ap,topo_xml_type_t);
		topo_fru_zero_buff(info,size);
		strncpy(info,buff,size);
		topo_fru_zero_buff(buff,sizeof(buff));
		switch (type) {
		case TOPO_XML_UINT32:
			snprintf(buff,maxlen,"%s,%u",info,va_arg(ap, uint32_t));
			break;
		case TOPO_XML_INT32:
			snprintf(buff,maxlen,"%s,%d",info,va_arg(ap, int32_t));
			break;
		case TOPO_XML_UINT64:
			snprintf(buff,maxlen,"%s,%llu",info,va_arg(ap, uint64_t));
			break;
		case TOPO_XML_INT64:
			snprintf(buff,maxlen,"%s,%lld",info,va_arg(ap, int64_t));
			break;
		case TOPO_XML_STRING:
			strp =  va_arg(ap, char *);
			if(strp)
				snprintf(buff,maxlen,"%s,%s",info,strp);
			else
				snprintf(buff,maxlen,"%s,-",info);
			break;
		case TOPO_XML_DOUBLE:
			snprintf(buff,maxlen,"%s,%llf",info,va_arg(ap, double));
			break;
		case TOPO_XML_LONG:
			snprintf(buff,maxlen,"%s,%ld",info,va_arg(ap, long));
			break;
		case TOPO_XML_DONE:
			ret = TOPO_XML_SUCCESS;
			break;
		default:
			ret = TOPO_XML_FAULT;
		}
	}
	return (ret);
}

static void topo_fru_node_list_free(xml_nodes_t * xml_nodesp){
	xml_node_list_t * xml_cnt_node = xml_nodesp->xmlHead;
	for(;(xml_cnt_node);xml_cnt_node = xml_cnt_node->nodeNext){
		topo_fru_xml_free(xml_cnt_node);
	}
}

void topo_fru_alarm_xml_init()
{
	if(!TOPO_FRU_XML_IS_EXIST){
		xmlDocPtr doc = xmlNewDoc((xmlChar *)"1.0");
		xmlNodePtr root_node = xmlNewNode(NULL, (xmlChar *)"root");
		xmlDocSetRootElement(doc, root_node);

		xmlSaveFormatFileEnc(TOPO_FRU_XML_PATH, doc, "UTF-8", 1);
		xmlFreeDoc(doc);
	}

	(void) pthread_mutex_init(&g_topo_fru_xml_lock, NULL);
}

void topo_fru_alarm_xml_fini()
{
	(void) pthread_mutex_destroy(&g_topo_fru_xml_lock);
}

static topo_xml_hd_t topo_fru_open_xml()
{
	xmlNodePtr rootNode;
	xmlKeepBlanksDefault(0);
	topo_xml_doc = xmlReadFile(TOPO_FRU_XML_PATH,"UTF-8",XML_PARSE_RECOVER);
	if (NULL == topo_xml_doc) {
		return NULL;
	}
	
	rootNode = xmlDocGetRootElement(topo_xml_doc);
	if (NULL == rootNode) {
		xmlFreeDoc(topo_xml_doc);
		return NULL;
	}

	return rootNode;
}

static void topo_fru_close_xml()
{
	xmlSaveFormatFileEnc(TOPO_FRU_XML_PATH, topo_xml_doc, "UTF-8", 1);
	xmlFreeDoc(topo_xml_doc);
	xmlCleanupParser();
	xmlMemoryDump();
	return;
}

static int topo_fru_xml_exist(){
	topo_xml_hd_t xml_hd;
	
	xml_hd = topo_fru_open_xml();
	if(NULL == xml_hd){
		return 0;
	}else{
		topo_fru_close_xml();
	}
	return 1;
}

static void topo_set_alarm_status_node(xmlNodePtr xmlNode)
{
	xmlSetProp(xmlNode,BAD_CAST("status"),BAD_CAST(ALARMED));
}

static int topo_fru_add_node_xml(topo_xml_hd_t rootNode,const char * name,
	const int alarmid,const char * tocm){
	xmlNodePtr node;
	char num[64] = {0};
	node = xmlNewChild((xmlNodePtr)rootNode, NULL, BAD_CAST("node"), NULL);
	xmlSetProp(node,BAD_CAST("status"),BAD_CAST(ALARMED));
	xmlSetProp(node,BAD_CAST("cnt"),BAD_CAST("0"));
	xmlSetProp(node,BAD_CAST("tocm"),BAD_CAST(tocm));
	snprintf(num,sizeof(num),"%d",alarmid);
	xmlSetProp(node,BAD_CAST("alarmid"),BAD_CAST(num));
	xmlNodeSetContent(node, BAD_CAST(name));
	return 0;
}

static int topo_fru_delete_node_xml(xmlNodePtr xmlNode)
{
	xmlNodePtr nextNode = xmlNode->next;
	xmlNodePtr prevNode = xmlNode->prev;
	
	xmlUnlinkNode(xmlNode);
	xmlFreeNode(xmlNode);
	if(prevNode)
		prevNode->next = nextNode;
	return 0;
}

static int topo_fru_get_alarm_cnt(xmlNodePtr xmlNode){
	xmlChar * cnt;
	int icnt = 0;
	cnt = xmlGetProp(xmlNode,BAD_CAST("cnt"));
	icnt = (int)atoi((char *)cnt);
	xmlFree(cnt);
	return (icnt);
}

static int topo_fru_increase_alarm_cnt(xmlNodePtr xmlNode)
{
	char num[64] = {0};
	int cnt = topo_fru_get_alarm_cnt(xmlNode);
	cnt = cnt + 1;
	snprintf(num,sizeof(num),"%d",cnt);
	xmlSetProp(xmlNode,BAD_CAST("cnt"),BAD_CAST(num));
	return (cnt);
}

static int topo_fru_get_tocm_xml(xmlNodePtr xmlNode){
	xmlChar * tocm;
	int ret = TOPO_XML_SUCCESS;
	tocm = xmlGetProp(xmlNode,BAD_CAST("tocm"));
	if(xmlStrEqual(tocm,BAD_CAST("no"))){
		ret = TOPO_XML_FAULT;
	}
	xmlFree(tocm);
	return (ret);
}

static int topo_fru_get_alarmid_xml(xmlNodePtr xmlNode){
	char * alarmidStr = NULL;
	int ialarmidStr = 0;
	alarmidStr = (char *)xmlGetProp(xmlNode,BAD_CAST("alarmid"));
	ialarmidStr = (int)atoi(alarmidStr);
	xmlFree(ialarmidStr);
	return (ialarmidStr);
}

static void topo_fru_set_tocm_xml(xmlNodePtr xmlNode){
	xmlSetProp(xmlNode,BAD_CAST("tocm"),BAD_CAST("yes"));
	return;
}

static int same_node(xmlChar * content,const char *name,int flag){
	if(flag & ISCLEAR){
		return(!xmlStrncmp(content,BAD_CAST(name),strlen(name)));
	}
	else{
		return(!strcmp((const char*) content,name));
	}
}

static int topo_fru_search_fault_xml(
	topo_xml_hd_t xml_hd,const char *name,xml_nodes_t * xml_nodesp,int flag){
	int ret = TOPO_NODE_NON_EXIST;
	int cnt = 0;
	int tocm = TOPO_XML_SUCCESS;
	xmlChar * content = NULL;
	xmlChar * status = NULL;
	xmlNodePtr curNode = (xmlNodePtr)xml_hd->xmlChildrenNode;
	
	while(curNode){
		content = xmlNodeGetContent(curNode);
		if(content == NULL){
			continue;
		}
		if(same_node(content,name,flag)){
			status = xmlGetProp(curNode,BAD_CAST("status"));
			if(status == NULL){
				xmlFree(content);
				content = NULL;
				continue;
			}
			if(xmlStrEqual(status,BAD_CAST(NON_ALARMED))){
				ret = TOPO_NODE_EXIST;
				if(flag & TISSET)
					xmlSetProp(curNode,BAD_CAST("status"),BAD_CAST(ALARMED));
				if(flag & SETCNT){
					tocm = topo_fru_get_tocm_xml(curNode);
					if(TOPO_XML_FAULT == tocm)
						cnt = topo_fru_increase_alarm_cnt(curNode);
				}
				
				if(NULL == xml_nodesp){
					break;
				}else{
					topo_fru_xml_node_list_instert(xml_nodesp,curNode,cnt,tocm);
				}
			}else if(flag & ISCLEAR){
				xmlSetProp(curNode,BAD_CAST("status"),BAD_CAST(NON_ALARMED));
			}
		}
		xmlFree(content);
		content = NULL;
		xmlFree(status);
		status = NULL;
		curNode = curNode->next;
	}

	if(content){
		xmlFree(content);
		content = NULL;
	}

	if(status){
		xmlFree(status);
		status = NULL;
	}
	return (ret);
}

static xmlChar * topo_fru_get_name_xml(xmlNodePtr xmlNode)
{
	return xmlNodeGetContent(xmlNode);
}

static void topo_fru_free_cm_alarm_node(xmlNodePtr xmlNode,const int alarmid,const char *hostname)
{
	char cmd_buf[INFOMAXLEN] = {0};
	xmlChar * name = topo_fru_get_name_xml(xmlNode);
	switch(alarmid)
	{
		case AMARM_ID_ZFS_THINLUN:
		case AMARM_ID_USER_QUOTA:
			cm_alarm_cmd(cmd_buf,sizeof(cmd_buf),"recovery %d \"%s,%s,-,-,-\"", alarmid, hostname, (char *)name);
			break;
		case AMARM_ID_QUOTA:
		case AMARM_ID_DIR_QUOTA:
		case AMARM_ID_POOL_STATE:
			cm_alarm_cmd(cmd_buf,sizeof(cmd_buf),"recovery %d \"%s,%s,-,-\"", alarmid, hostname, (char *)name);
			break;
		case AMARM_ID_FC_LINK:	
		case AMARM_ID_ETH_LINK:		
		case AMARM_ID_HEART_LINK:
			cm_alarm_cmd(cmd_buf,sizeof(cmd_buf),"recovery %d \"%s,%s,-\"", alarmid, hostname, (char *)name);
			break;
		case AMARM_ID_SYS_TEMP:
		case AMARM_ID_P_DIMM:
		case AMARM_ID_CPU_DIMM:
		case AMARM_ID_CPU_CORE:
		case AMARM_ID_CPU_TEMP:
			cm_alarm_cmd(cmd_buf,sizeof(cmd_buf),"recovery %d \"%s,%s\"", alarmid, hostname, (char *)name);
			break;
	}
	xmlFree(name);
	return;
}

static void topo_fru_set_cm_alarm_node_z(
	const int alarmid,const char *hostname,const char *name,va_list ap)
{
	char cmd_buf[INFOMAXLEN] = {0};
	char	info[INFOMAXLEN] = {0};
	topo_fru_tocm_info_set(info,sizeof(info),ap);
	cm_alarm_cmd(cmd_buf,sizeof(cmd_buf),"report %d \"%s,%s%s\"", alarmid, hostname,name,info);
	return;
}


static void topo_fru_set_cm_alarm_node(
	xml_nodes_t * xml_nodesp,const int alarmid,const char *hostname,const char *name,va_list ap)
{
	char cmd_buf[INFOMAXLEN] = {0};
	char	info[INFOMAXLEN] = {0};
	topo_fru_tocm_info_set(info,sizeof(info),ap);
	cm_alarm_cmd(cmd_buf,sizeof(cmd_buf),"report %d \"%s,%s%s\"", alarmid, hostname,name,info);
	topo_fru_set_tocm_xml(xml_nodesp->xmlHead->xmlNode);
	return;
}

static void topo_fru_xml_node_list_instert(xml_nodes_t * xml_hd,
	xmlNodePtr xmlNode,const int cnt,const int tocm)
{
	xml_node_list_t * xml_head_node = NULL;
	xml_node_list_t * xml_cnt_node =NULL;
	xml_node_list_t * xml_list_node = topo_fru_xml_alloc();
	
	xml_list_node->xmlNode = xmlNode;
	xml_list_node->nodeNext= NULL;
	xml_list_node->cnt = cnt;
	xml_list_node->tocm = tocm;
	xml_list_node->alarmid = topo_fru_get_alarmid_xml(xmlNode);
	if(NULL == xml_hd->xmlHead){
		xml_hd->xmlHead = xml_list_node;
	}else{
		for(xml_cnt_node = xml_hd->xmlHead;
			xml_cnt_node->nodeNext;
			xml_cnt_node = xml_cnt_node->nodeNext);

		xml_cnt_node->nodeNext = xml_list_node;
	}
	xml_hd->xmlLen++;
}

static void topo_fru_xml_nodes_list_delete(
	xml_nodes_t * xml_nodesp,const char *hostname){
	int indx = 0;
	xml_node_list_t * xml_cnt_node = xml_nodesp->xmlHead;
	
	for(indx = 0;(xml_cnt_node && indx < xml_nodesp->xmlLen);
		++indx,xml_cnt_node = xml_cnt_node->nodeNext){
		topo_fru_free_cm_alarm_node(xml_cnt_node->xmlNode,xml_cnt_node->alarmid,hostname);
		topo_fru_delete_node_xml(xml_cnt_node->xmlNode);
		topo_fru_xml_free(xml_cnt_node);
	}
}

int topo_fru_set_fault_xml(
	const int alarmid,const char *name,const int alarm_cnt,...)
{
	topo_xml_hd_t	xml_hd;
	int	fault_is_exist = 1;
	xml_nodes_t	xml_nodes = {0};
	va_list	ap;
	char hostname[HOSTNAMEXML];
	
	topo_fru_xml_lock();
	xml_hd = topo_fru_open_xml();
	if(NULL == xml_hd){
		topo_fru_xml_unlock();
		return 0;
	}

	if(TOPO_XML_FAULT == topo_fru_get_hostname(hostname,sizeof(hostname))){
		topo_fru_close_xml();
		topo_fru_xml_unlock();
		return 0;
	}
	
	/*search fault*/
	if(TOPO_NODE_NON_EXIST == 
		topo_fru_search_fault_xml(xml_hd,name,&xml_nodes,TISSET|SETCNT)){
		/*insert fault*/
		if(0 == alarm_cnt){
			va_start(ap, alarm_cnt);
			topo_fru_set_cm_alarm_node_z(alarmid,hostname,name,ap);
			va_end(ap);
			topo_fru_add_node_xml(xml_hd,name,alarmid,"yes");
		}else{
			topo_fru_add_node_xml(xml_hd,name,alarmid,"no");
		}
		fault_is_exist = 0;
	}else {
		topo_set_alarm_status_node(xml_nodes.xmlHead->xmlNode);
		if(((xml_nodes.xmlHead->cnt) >= alarm_cnt) 
			&& ((xml_nodes.xmlHead->tocm) == TOPO_XML_FAULT)){
			va_start(ap, alarm_cnt);
			topo_fru_set_cm_alarm_node(&xml_nodes,alarmid,hostname,name,ap);
			va_end(ap);
		}
	}
	
	topo_fru_node_list_free(&xml_nodes);
	topo_fru_close_xml();
	topo_fru_xml_unlock();

	return (fault_is_exist);
}

void topo_fru_clear_fault_xml(const char* name)
{
	topo_xml_hd_t xml_hd;
	xml_nodes_t xml_nodes = {0};
	char hostname[HOSTNAMEXML];
	
	topo_fru_xml_lock();
	xml_hd = topo_fru_open_xml();
	if(NULL == xml_hd){
		topo_fru_xml_unlock();
		return;
	}
	
	if(TOPO_XML_FAULT == topo_fru_get_hostname(hostname,sizeof(hostname))){
		topo_fru_close_xml();
		topo_fru_xml_unlock();
		return;
	}
	
	/*search fault*/
	if(TOPO_NODE_EXIST == topo_fru_search_fault_xml(xml_hd,name,&xml_nodes,ISCLEAR)){
		topo_fru_xml_nodes_list_delete(&xml_nodes,hostname);
	}

	topo_fru_close_xml();
	topo_fru_xml_unlock();
}
