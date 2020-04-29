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
 * Copyright (c) 2005, 2010, Oracle and/or its affiliates. All rights reserved.
 * Copyright (c) 2011, 2014 by Delphix. All rights reserved.
 * Copyright (c) 2012, Joyent, Inc. All rights reserved.
 * Copyright (c) 2013 Steven Hartland.  All rights reserved.
 * Copyright 2013 Nexenta Systems, Inc. All rights reserved.
 */

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <libgen.h>
#include <libintl.h>
#include <libuutil.h>
#include <libnvpair.h>
#include <locale.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/fs/zfs.h>
#include <sys/fm/fs/zfs.h>
#include <sys/zfs_ioctl.h>
#include <sys/types.h>

#include <libzfs.h>
#include <libzfs_core.h>

libzfs_handle_t *g_zfs;
#define ARRAYSIZE(X)  (sizeof(X) / sizeof(X[0]))

struct
{
	char *name;
	int code;
} fault_code[] = 
{
	{"merr",		FM_SIMULATE_DEVICE_MERR},
	{"removed",		FM_SIMULATE_DEV_REMOVED},
	{"noresponse",	FM_SIMULATE_DEV_NORESP},
	{"smartfail",	FM_SIMULATE_DEV_SMART_FAIL}
};

static void
ussage(void)
{
	(void) fprintf(stderr, gettext("ussage: \n"));
	(void) fprintf(stderr, gettext("fm_simulate disk merr|removed|noresponse|smartfail\n"));
}

int
main(int argc, char **argv)
{
	int ret = 0;
	int i = 0;
	int fault = -1;
	char diskname[128] = {0}; 

	(void) setlocale(LC_ALL, "");
	(void) textdomain(TEXT_DOMAIN);

	if (argc != 3) {
		ussage();
		return (1);
	}

	if (strstr(argv[1], "/dev/disk/by-id/") == 0) {
		snprintf(diskname, sizeof(diskname), "/dev/disk/by-id/%s", argv[1]);
	} else {
		strncpy(diskname, argv[1], strlen(argv[1]));
	}

	for (i = 0; i < ARRAYSIZE(fault_code); i++) {
		if (strcmp(fault_code[i].name, argv[2]) == 0) {
			fault = fault_code[i].code;
			break;
		}
	}

	if (fault < 0) {
		ussage();
		return (1);
	}
	
	if ((g_zfs = libzfs_init()) == NULL) {
		(void) fprintf(stderr, "%s", libzfs_error_init(errno));
		return (1);
	}

	ret = zfs_fm_simulate(g_zfs, diskname, fault);
	if (ret)
		(void) fprintf(stderr, gettext("simulate failed\n"));
	
	libzfs_fini(g_zfs);
	return (ret);
}
