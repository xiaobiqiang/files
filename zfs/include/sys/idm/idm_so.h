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
 * Copyright (c) 2008, 2010, Oracle and/or its affiliates. All rights reserved.
 */

#ifndef _IDM_SO_H
#define	_IDM_SO_H

#ifdef	__cplusplus
extern "C" {
#endif

#include <sys/idm/idm_transport.h>
#include <linux/socket.h>
#include <linux/inet.h>

/*
 * Define TCP window size (send and receive buffer sizes)
 */

#define	IDM_RCVBUF_SIZE		(256 * 1024)
#define	IDM_SNDBUF_SIZE		(256 * 1024)

/* sockets-specific portion of idm_svc_t */
typedef void *kt_did_t;
typedef struct idm_so_svc_s {
	struct socket	*is_so;
	kthread_t		*is_thread;
	kt_did_t		is_thread_did;
	boolean_t		is_thread_running;
} idm_so_svc_t;

/* sockets-specific portion of idm_conn_t */
typedef struct idm_so_conn_s {
	struct socket	*ic_so;

	kthread_t		*ic_tx_thread;		/* idm_sotx_thread */
	kt_did_t		ic_tx_thread_did;
	boolean_t		ic_tx_thread_running;
	kmutex_t		ic_tx_mutex;
	kcondvar_t		ic_tx_cv;
	list_t			ic_tx_list;	/* idm_pdu_t list for transmit */

	kthread_t		*ic_rx_thread;		/* idm_sorx_thread */
	kt_did_t		ic_rx_thread_did;
	boolean_t		ic_rx_thread_running;
} idm_so_conn_t;

void idm_so_init(idm_transport_t *it);
void idm_so_fini(void);

/* used by idm_so_timed_socket_connect */
typedef struct idm_so_timed_socket_s {
	kcondvar_t	it_cv;
	boolean_t	it_callback_called;
	int		it_socket_error_code;
} idm_so_timed_socket_t;

/* used to track sgl format data buffers */
struct stmf_sglist_ent;
typedef struct idm_so_sgl_xfer_s {
	size_t			ix_size;	/* size of this object */
	struct stmf_sglist_ent	*ix_sgl_base;	/* the sgl format buffer */
	struct stmf_sglist_ent	*ix_seg;	/* the current seg to xfer */
	uint32_t		ix_seg_resid;	/* remaining data in cur seg */
	int			ix_numsegs;	/* total segments */
	int			ix_iovlen;	/* active iov entries */
	struct iovec		ix_iov[1];	/* space for iovec creation */
} idm_so_sgl_xfer_t;

/* Socket functions */

struct socket *
idm_socreate(int domain, int type, int protocol);

void idm_soshutdown(struct socket *so);

void idm_sodestroy(struct socket *so);

int idm_ss_compare(const struct sockaddr_storage *cmp_ss1,
    const struct sockaddr_storage *cmp_ss2,
    boolean_t v4_mapped_as_v4,
    boolean_t compare_ports);

int idm_get_ipaddr(idm_addr_list_t **);

void idm_addr_to_sa(idm_addr_t *dportal,
    struct sockaddr_storage *sa);

#define	IDM_SA_NTOP_BUFSIZ (INET6_ADDRSTRLEN + sizeof ("[].65535") + 1)

const char *idm_sa_ntop(const struct sockaddr_storage *sa,
    char *buf, size_t size);

int idm_sorecv(struct socket *so, void *msg, size_t len);

int idm_sosendto(struct socket *so, void *buff, size_t len,
    struct sockaddr *name, int namelen);

int idm_iov_sosend(struct socket *so, iovec_t *iop, int iovlen,
    size_t total_len);

int idm_iov_sorecv(struct socket *so, iovec_t *iop, int iovlen,
    size_t total_len);

void idm_sotx_thread(void *arg);
void idm_sorx_thread(void *arg);


int idm_sotx_pdu_constructor(void *hdl, void *arg, int flags);

void idm_sotx_pdu_destructor(void *pdu_void, void *arg);

int idm_sorx_pdu_constructor(void *hdl, void *arg, int flags);

void idm_sorx_pdu_destructor(void *pdu_void, void *arg);

void idm_so_svc_port_watcher(void *arg);

int idm_so_timed_socket_connect(struct socket *ks,
    struct sockaddr_storage *sa, int sa_sz, int login_max_usec);

#ifdef	__cplusplus
}
#endif

#endif /* _IDM_SO_H */
