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

#include <errno.h>
#include <libgen.h>
#include <libintl.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "zpool_util.h"

/*
 * Utility function to guarantee malloc() success.
 */
void *
safe_malloc(size_t size)
{
	void *data;

	if ((data = calloc(1, size)) == NULL) {
		(void) fprintf(stderr, "internal error: out of memory\n");
		exit(1);
	}

	return (data);
}

/*
 * Display an out of memory error message and abort the current program.
 */
void
zpool_no_memory(void)
{
	assert(errno == ENOMEM);
	(void) fprintf(stderr,
	    gettext("internal error: out of memory\n"));
	exit(1);
}

/*
 * Return the number of logs in supplied nvlist
 */
uint_t
num_logs(nvlist_t *nv)
{
	uint_t nlogs = 0;
	uint_t c, children;
	nvlist_t **child;

	if (nvlist_lookup_nvlist_array(nv, ZPOOL_CONFIG_CHILDREN,
	    &child, &children) != 0)
		return (0);

	for (c = 0; c < children; c++) {
		uint64_t is_log = B_FALSE;

		(void) nvlist_lookup_uint64(child[c], ZPOOL_CONFIG_IS_LOG,
		    &is_log);
		if (is_log)
			nlogs++;
	}
	return (nlogs);
}
/*
 * Return the number of logs in supplied nvlist
 */
uint_t
num_metas(nvlist_t *nv)
{
	uint_t nmetas = 0;
	uint_t c, children;
	nvlist_t **child;

	if (nvlist_lookup_nvlist_array(nv, ZPOOL_CONFIG_CHILDREN,
	    &child, &children) != 0)
		return (0);

	for (c = 0; c < children; c++) {
		uint64_t is_meta = B_FALSE;

		(void) nvlist_lookup_uint64(child[c], ZPOOL_CONFIG_IS_META,
		    &is_meta);
		if (is_meta)
			nmetas++;
	}
	return (nmetas);
}

/*
 * Return the number of lows in supplied nvlist
 */
uint_t
num_lows(nvlist_t *nv)
{
	uint_t nlows = 0;
	uint_t c, children;
	nvlist_t **child;

	if (nvlist_lookup_nvlist_array(nv, ZPOOL_CONFIG_CHILDREN,
	    &child, &children) != 0)
		return (0);

	for (c = 0; c < children; c++) {
		uint64_t is_low = B_FALSE;

		(void) nvlist_lookup_uint64(child[c], ZPOOL_CONFIG_IS_LOW,
		    &is_low);
		if (is_low)
			nlows++;
	}
	return (nlows);
}

#define	ZPOOL_LOCK_DIR	"/var/run/zpool_lock"

void
cprint(const char *format, ...)
{
	va_list args;
	FILE *fp;
	char path[] = "/var/cluster/log/zpool.log";
	char buf[512], *cp;
	char timebuf[32];
	struct timeval now;
	struct tm ltime;

	if (gettimeofday(&now, NULL) != 0)
		return;

	(void) strftime(timebuf, sizeof (timebuf), "%b %e %T",
	    localtime_r(&now.tv_sec, &ltime));

	(void) snprintf(buf, sizeof (buf), "%s: ", timebuf);
	cp = strchr(buf, '\0');
	va_start(args, format);
	(void) vsnprintf(cp, sizeof (buf) - (cp - buf), format, args);
	va_end(args);
	(void) strcat(buf, "\n");

	fp = fopen(path, "a");
	if (fp == NULL)
		return;
	(void) fputs(buf, fp);
	fclose(fp);
}

static int
_file_lockw(int fd)
{
	struct flock lock;

	if (fd < 0) {
		errno = EBADF;
		return (-1);
	}

	lock.l_type = F_WRLCK;
	lock.l_whence = SEEK_SET;
	lock.l_start = 0;
	lock.l_len = 0;
	while (fcntl(fd, F_SETLKW, &lock) < 0) {
		if (errno == EINTR)
			continue;
		return (-1);
	}

	return (0);
}

static int
_file_unlock(int fd)
{
	struct flock lock;

	if (fd < 0) {
		errno = EBADF;
		return (-1);
	}

	lock.l_type = F_UNLCK;
	lock.l_whence = SEEK_SET;
	lock.l_start = 0;
	lock.l_len = 0;
	if (fcntl(fd, F_SETLK, &lock) < 0)
		return (-1);

	return (0);
}

static pid_t
_file_is_locked(int fd)
{
	struct flock lock;

	if (fd < 0) {
		errno = EBADF;
		return (-1);
	}

	lock.l_type = F_WRLCK;
	lock.l_whence = SEEK_SET;
	lock.l_start = 0;
	lock.l_len = 0;
	if (fcntl(fd, F_GETLK, &lock) < 0)
		return (-1);
	if (lock.l_type == F_UNLCK)
		return (0);

	return (lock.l_pid);
}

static int
_lock_open(const char *lockfile)
{
	const mode_t filemode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
	int flags = O_RDWR | O_CREAT;
	mode_t mask;
	int fd;

	mask = umask(0);
	umask(mask | 022);
	fd = open(lockfile, flags, filemode);
	umask(mask);
	if (fd < 0) {
		cprint("Failed to open file \"%s\": %s",
			lockfile, strerror(errno));
		return (-1);
	}

	return (fd);
}

int
zpool_lock(const char *poolname, const char *cmd)
{
	char buf[BUFSIZ];
	int fd;
	pid_t pid;
	const mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;

	snprintf(buf, sizeof(buf), "%s/%s", ZPOOL_LOCK_DIR, poolname);
	(void) mkdir(ZPOOL_LOCK_DIR, mode);
	fd = _lock_open(buf);
	if (fd < 0)
		return (-1);

	if ((pid = _file_is_locked(fd)) > 0) {
		if (read(fd, buf, sizeof(buf)) > 0)
			cprint("pool is locked by %d: %s", pid, buf);
	}

	if (_file_lockw(fd) < 0) {
		close(fd);
		return (-1);
	}

	if (ftruncate(fd, 0) == -1) {
		cprint("Failed to truncate file \"%s\": %s",
			buf, strerror(errno));
	}

	lseek(fd, 0, SEEK_SET);
	snprintf(buf, sizeof(buf), "%d %s", getpid(), cmd);
	if (write(fd, buf, strlen(buf)) == -1) {
		close(fd);
		return (-1);
	}

	return (fd);
}

int
zpool_unlock(int fd,const char * poolname)
{
	int rv;
	char buf[BUFSIZ];

	if (fd < 0)
		return (-1);

	rv = _file_unlock(fd);
	close(fd);

	snprintf(buf, sizeof(buf), "%s/%s", ZPOOL_LOCK_DIR, poolname);
	(void) unlink(buf);
	return (rv);
}
