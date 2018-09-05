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
 * Copyright 2009 CeresData Co., Ltd.  All rights reserved.
 * Use is subject to license terms.
 */

#ifndef _DEVFSADM_H
#define	_DEVFSADM_H

#include <sys/types.h>
#include <libclumgt.h>

#undef	DEBUG
#ifndef DEBUG
#define	NDEBUG 1
#else
#undef	NDEBUG
#endif

#include <assert.h>


#ifdef	__cplusplus
extern "C" {
#endif

#define CMDNUMBER 4096

#define	INFO_MID		NULL		/* always prints */
#define	VERBOSE_MID		"verbose"	/* prints with -v */
#define	CHATTY_MID		"chatty" 	/* prints with -V chatty */


void clumgt_errprint(char *message, ...);
void clumgt_print(char *mid, char *message, ...);
int clumgt_server(void);
void *clumgt_worker (void *arg);
int clumgt_parse_revcdata(void *req, clumgt_response_t **resp);
extern int
clumgt_get_hosturl(char *url);


#ifdef	__cplusplus
}
#endif

#endif	/* _DEVFSADM_H */


