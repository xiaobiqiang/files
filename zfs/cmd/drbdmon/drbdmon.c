#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <linux/types.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include <syslog.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <sys/list.h>
#include <sys/thread_pool.h>
#include <pthread.h>
#include <linux/un.h>
#include <arpa/inet.h>
#include <sys/if_drbdsvc.h>

#ifndef	offsetof
#define	offsetof(s, m)  ((size_t)(&((s *)0)->m))
#endif

#define DRBDMON_PING(ip, result)		\
	{								\
		char *line = NULL;			\
		size_t size = 0;			\
		char cmd[256];				\
		FILE *outbuf;				\
									\
		memset(cmd, 0, 256);		\
		snprintf(cmd, 256, "ping %s -c 1 -W 1 | grep transmitted | cut -d ',' -f 3", (ip));	\
		outbuf = popen(cmd, "r");	\
		getline(&line, &size, outbuf);	\
		if (line && size) {			\
			result = (line[1] == '0');	\
			free(line);				\
			line = NULL;			\
		}							\
		pclose(outbuf);				\
	}

#define DRBDMON_BP_INQR(drbdX, status, primary)	\
	{	\
		char *line = NULL;	\
		size_t line_sz = 0;		\
		FILE *ostream;		\
		char cmd[128] = {0};	\
		char *primary_ds, *secondary_ds, *cs, *ro, *ds;	\
		size_t nprimary_ds, nsecondary_ds;	\
				\
		status = 0;	\
		snprintf(cmd, 128, "cat /proc/drbd | grep %d:", drbdX);	\
		ostream = popen(cmd, "r");		\
		getline(&line, &line_sz, ostream);		\
		if (line && line_sz) {		\
			line_sz = strlen(line);		\
			if (line[line_sz-1] == '\n')		\
				line[line_sz-1]= '\0';		\
			if ((cs = strstr(line, "cs:")) != NULL) 	\
				cs += 3;	\
			if ((ro = strstr(line, "ro:")) != NULL)	\ 
				ro += 3;	\
			if ((ds = strstr(line, "ds:")) != NULL) 	\
				ds += 3;	\
			if (cs && ro && ds) {	\
				printf("%s cs:%s ro:%s ds:%s", __func__, cs, ro, ds);	\
				if (!strncmp(cs, "StandAlone", strlen("StandAlone"))) {		\
					if (!strncmp(ro, "Primary", strlen("Primary")) && 	\
						(primary == 1) && 	\
						!strncmp(ds, "UpToDate", strlen("UpToDate")))	\
						status = 1;		\
					else if (!strncmp(ro, "Secondary", strlen("Secondary")) && !primary)	\
						status = 1;	\
				}	\
			}	\
			free(line);		\
		}	\
	}

struct drbdmon_req{
	list_node_t			entry;
	struct drbdmon_head head;
	void 				*ibufp;
	void				*obufp;
	struct sockaddr 	*pr_addr;
	uint32_t			pr_addrlen;
	uint32_t			pr_sockfd;
	uint32_t			ilen;
	uint32_t			olen;
	uint32_t			ousedlen;
	uint32_t			error;
};

struct drbdmon_global {
	uint32_t			local_sock;
	uint32_t			max_conn;
	char				*sock_path;

	pthread_mutex_t		rcv_mtx;
	pthread_cond_t		rcv_cv;
	pthread_t			rcv_ctx;
	list_t				rcv_rqlist1;
	list_t				rcv_rqlist2;
	list_t				*rcv_rqwait;
	list_t				*rcv_rqlive;

	tpool_t 			*hdl_ctx;
	uint32_t			nhdl_ctx;
	
	uint32_t			mon_exit:1,
						rcv_insleep:1,
						rsvd:30;
};

struct drbdmon_resume_bp_ctx {
	list_node_t		entry;
	char			peer_ip[16];
	pthread_mutex_t ctx_mtx;
	pthread_cond_t	ctx_cv;
	list_t			resume_list;
	tpool_t			*mon_ctx;
	tpool_t			*invalidate_ctx;
	uint32_t		ip_addr;
	uint32_t		exit:1,
					rsvd:31;
};

typedef void (*accept_fn)(struct drbdmon_global *, int, struct sockaddr *, socklen_t);

static void drbdmon_resume_bp_invalidate_impl(list_t *);
static void drbdmon_resume_bp_invalidate(struct drbdmon_resume_bp_ctx *, list_t *);
static void drbdmon_resume_bp_mixed(struct drbdmon_resume_bp_ctx *, uint32_t);
static void drbdmon_resume_bp_fn(struct drbdmon_resume_bp_ctx *);
static struct drbdmon_resume_bp_ctx *drbdmon_create_resume_bp_ctx(struct drbdmon_param_resume_bp *);
static struct drbdmon_resume_bp_ctx *drbdmon_lookup_resume_bp_ctx(const char *);
static int drbdmon_resume_breakpoint(struct drbdmon_req *);
static int drbdmon_resume_bp_init(struct drbdmon_global *);
static void drbdmon_free_request(struct drbdmon_req *);
static void drbdmon_deal_request(struct drbdmon_req *);
static void drbdmon_submit_request(struct drbdmon_req *, struct drbdmon_global *);
static void *drbdmon_recv(struct drbdmon_global *);
static int drbdmon_send(int, void *, size_t, int);
static int drbdmon_send_resp_impl(struct drbdmon_req *);
static int drbdmon_send_resp(struct drbdmon_req *);
static void drbdmon_build_resp(struct drbdmon_req *);
static void drbdmon_handle_connect(struct drbdmon_global *, int, struct sockaddr *, socklen_t);
static void drbdmon_accept(struct drbdmon_global *, accept_fn);
static int drbdmon_create_unix_socket(struct drbdmon_global *);
static void drbdmon_init_global(struct drbdmon_global *);
static void drbdmon_genl_global(struct drbdmon_global **);
static void drbdmon_delay_locked(pthread_mutex_t *, pthread_cond_t *, uint32_t);

static struct {
	enum drbdmon_rpc_type rpc;
	uint32_t param_len;
	int (*rpc_hdlp)(struct drbdmon_req *);
} drbdmon_rpc[] = {
	{DRBDMON_RPC_FIRST, 			0, NULL},
	{DRBDMON_RPC_RESUME_BREADPOINT, 
		sizeof(struct drbdmon_param_resume_bp), drbdmon_resume_breakpoint},
	{DRBDMON_RPC_LAST, 				NULL}
};

static struct drbdmon_global *drbdmon_glp = NULL;

pthread_mutex_t	drbdmon_bp_ctx_list_mtx;
static list_t	drbdmon_bp_ctx_list;

static void
drbdmon_delay_locked(pthread_mutex_t *mtx, pthread_cond_t *cv, uint32_t time)
{
	long sec_inc, nsec_inc;
	struct timespec tmspec;

	sec_inc = time / 1000;
	nsec_inc = (time - sec_inc*1000)*1000000;
	clock_gettime(CLOCK_REALTIME, &tmspec);
	if (tmspec.tv_nsec >= (1000000000 - nsec_inc)) {
		sec_inc++;
		nsec_inc -= 1000000000;
	}
	tmspec.tv_sec += sec_inc;
	tmspec.tv_nsec += nsec_inc;
	
	pthread_cond_timedwait(cv, mtx, &tmspec);
}

static void
drbdmon_genl_global(struct drbdmon_global **glop)
{
	struct drbdmon_global *glb = malloc(
			sizeof(struct drbdmon_global));
	if (!glb) return ;

	memset(glb, 0, sizeof(*glb));
	drbdmon_init_global(glb);
	*glop = glb;
}

static void
drbdmon_init_global(struct drbdmon_global *glop)
{
	glop->max_conn = 25;
	glop->nhdl_ctx = 2;
	glop->sock_path = strdup(DRBDMON_SOCKET);
	glop->local_sock = drbdmon_create_unix_socket(glop);
	list_create(&glop->rcv_rqlist1, sizeof(struct drbdmon_req),
			offsetof(struct drbdmon_req, entry));
	list_create(&glop->rcv_rqlist2, sizeof(struct drbdmon_req),
			offsetof(struct drbdmon_req, entry));
	glop->rcv_rqwait = &glop->rcv_rqlist1;
	glop->rcv_rqlive = &glop->rcv_rqlist2;
	pthread_mutex_init(&glop->rcv_mtx, NULL);
	pthread_cond_init(&glop->rcv_cv, NULL);
	pthread_create(&glop->rcv_ctx, NULL, drbdmon_recv, glop);
	glop->hdl_ctx = tpool_create(1, glop->nhdl_ctx, 0, NULL);
}

static int 
drbdmon_create_unix_socket(struct drbdmon_global *glop)
{
	int fd;
	struct sockaddr_un my_addr;
	
	fd = socket(AF_LOCAL, SOCK_STREAM | SOCK_CLOEXEC, 0);
	if (fd < 0)
		return fd;

	memset(&my_addr, 0, sizeof(struct sockaddr_un));
	my_addr.sun_family = AF_LOCAL;
	strncpy(my_addr.sun_path, glop->sock_path, sizeof(my_addr.sun_path)-1);
	if (bind(fd, (struct sockaddr *)&my_addr, sizeof(my_addr)) ||
		listen(fd, glop->max_conn))
		goto failed_listen;

	return fd;
failed_listen:
	close(fd);
	return -1;
}

static void
drbdmon_accept(struct drbdmon_global *glop, accept_fn callback_fnp)
{
	int new_fd;
	struct sockaddr_un pr_addr;
	socklen_t addr_len = 0;
	
	memset(&pr_addr, 0, sizeof(struct sockaddr_un));
	while (!glop->mon_exit) {
		printf("%s start to accept.\n", __func__);
		new_fd = accept(glop->local_sock, &pr_addr, &addr_len);
		printf("%s accepted, new:%d\n", __func__, new_fd);
		if (new_fd < 0)
			continue;

		if (callback_fnp)
			callback_fnp(glop, new_fd, &pr_addr, addr_len);
	}
}

static void
drbdmon_handle_connect(struct drbdmon_global *glop, 
		int prfd, struct sockaddr *pr_addr, socklen_t addr_len)
{
	struct drbdmon_req *req;
	uint32_t	need_to_wake = 0;
	struct sockaddr *addr = NULL;
	
	printf("%s fd:%d addr_len:%02x.\n", __func__, prfd, addr_len);
	if (!(req = malloc(sizeof(struct drbdmon_req))) ||
		(addr_len && !(addr = malloc(addr_len))))
		goto failed;

	memset(req, 0, sizeof(*req));
	req->pr_addr = addr;
	memcpy(req->pr_addr, pr_addr, addr_len);
	req->pr_addrlen = addr_len;
	req->pr_sockfd = prfd;
	pthread_mutex_lock(&glop->rcv_mtx);
	list_insert_tail(glop->rcv_rqwait, req);
	if (glop->rcv_insleep) {
		need_to_wake = 1;
		glop->rcv_insleep = 0;
	}
	pthread_mutex_unlock(&glop->rcv_mtx);
	if (need_to_wake)
		pthread_cond_signal(&glop->rcv_cv);
	
	printf("%s exit success.\n", __func__);
	return ;	
failed:
	if (addr && addr_len)
		free(addr);
	if (req)
		free(req);
	close(prfd);
}

static void 
drbdmon_build_resp(struct drbdmon_req *req)
{
	struct drbdmon_head *hdp = &req->head;

	hdp->magic = DRBDMON_MAGIC;
	hdp->ilen = req->ousedlen;
	hdp->olen = 0;
	hdp->error = req->error;
}

static int
drbdmon_send_resp(struct drbdmon_req *req)
{
	struct drbdmon_head *hdp = &req->head;

	drbdmon_build_resp(req);
	return drbdmon_send_resp_impl(req);
}

static int
drbdmon_send_resp_impl(struct drbdmon_req *req)
{
	int iRet;
	struct drbdmon_head *hdp = &req->head;
	int flags = MSG_NOSIGNAL;
	
	assert (req->pr_sockfd > 0);

	if (hdp->ilen)
		flags |= MSG_MORE;
	iRet = drbdmon_send(req->pr_sockfd, 
			hdp, sizeof(struct drbdmon_head), flags);
	if (iRet < 0)
		return iRet;
	
	flags &= ~MSG_MORE;
	return drbdmon_send(req->pr_sockfd, 
				req->obufp, req->ousedlen, flags);
}

static int
drbdmon_send(int sock, void *data, size_t len, int flags)
{
	ssize_t origin = len, lenSnd = 0, lenSndonce;

	while (len && ((lenSndonce = send(
				sock, data, len, flags)) > 0)) {
		len -= lenSndonce;
		data += lenSndonce;
		lenSnd += lenSndonce;
	}

	if (lenSnd < origin)
		return -1;
	return 0;
}

static void *
drbdmon_recv(struct drbdmon_global *glop)
{
	list_t *exchange;
	struct drbdmon_req *req;
	ssize_t rcvsz;
	struct drbdmon_head *hdp;
	
	while (!glop->mon_exit) {
		pthread_mutex_lock(&glop->rcv_mtx);
		if (list_is_empty(glop->rcv_rqwait)) {
			glop->rcv_insleep = 1;
			pthread_cond_wait(&glop->rcv_cv, &glop->rcv_mtx);
		}

		exchange = glop->rcv_rqwait;
		glop->rcv_rqwait = glop->rcv_rqlive;
		glop->rcv_rqlive = exchange;
		pthread_mutex_unlock(&glop->rcv_mtx);

		while (!list_is_empty(glop->rcv_rqlive)) {
			req = list_remove_head(glop->rcv_rqlive);
			assert(req->pr_sockfd > 0);
			
			hdp = &req->head;
			rcvsz = recv(req->pr_sockfd, &req->head, 
					sizeof(struct drbdmon_head), MSG_WAITALL);
			if ((rcvsz < sizeof(struct drbdmon_head)) ||
				(req->head.magic != DRBDMON_MAGIC) ||
				(hdp->ilen && !(req->ibufp = malloc(hdp->ilen))) ||
				(hdp->olen && !(req->obufp = malloc(hdp->olen))))
				goto failed_recv;

			req->ilen = hdp->ilen;
			req->olen = hdp->olen;
			rcvsz = recv(req->pr_sockfd, req->ibufp, 
						req->ilen, MSG_WAITALL);
			if (rcvsz < req->ilen)
				goto failed_recv;
			printf("%s recved sock:%d, to drbdmon_submit_request\n",
				__func__, req->pr_sockfd);
			drbdmon_submit_request(req, glop);
			continue;
failed_recv:
			req->error = DRBDMON_ERR_RECV;
			(void) drbdmon_send_resp(req);
			drbdmon_free_request(req);
		}
	}
}

static void 
drbdmon_submit_request(struct drbdmon_req *req, 
				struct drbdmon_global *glop)
{
	printf("%s hdl_ctx:%p\n", __func__, glop->hdl_ctx);
	(void) tpool_dispatch(glop->hdl_ctx, 
				drbdmon_deal_request, req);
}

static void
drbdmon_deal_request(struct drbdmon_req *req)
{
	uint32_t obuflen;
	int error;
	struct drbdmon_head *hdp = &req->head;
	
	printf("%s handle req, rpc:%d, param_len:%d ilen:%d\n",
		__func__, hdp->rpc, drbdmon_rpc[hdp->rpc].param_len, req->ilen);
	if ((hdp->rpc <= DRBDMON_RPC_FIRST) ||
		(hdp->rpc >= DRBDMON_RPC_LAST) ||
		(drbdmon_rpc[hdp->rpc].param_len != req->ilen)) {
		error = DRBDMON_ERR_PARAM;
		goto failed;
	}
	
	req->error = drbdmon_rpc[hdp->rpc].rpc_hdlp(req);
	printf("%s handled, error:%d, send resp\n", __func__, req->error);
failed:
	(void) drbdmon_send_resp(req);
	drbdmon_free_request(req);
}

static void
drbdmon_free_request(struct drbdmon_req *req)
{
	struct drbdmon_head *hdp = &req->head;
	
	if (hdp->olen && req->obufp)
		free(req->obufp);
	if (hdp->ilen && req->ibufp)
		free(req->ibufp);
	if (req->pr_addr && req->pr_addrlen)
		free(req->pr_addr);
	close(req->pr_sockfd);
	free(req);
}

static int
drbdmon_resume_bp_init(struct drbdmon_global *glop)
{
	pthread_mutex_init(&drbdmon_bp_ctx_list_mtx, NULL);
	list_create(&drbdmon_bp_ctx_list, 
			sizeof(struct drbdmon_resume_bp_ctx),
			offsetof(struct drbdmon_resume_bp_ctx, entry));
}

static int 
drbdmon_resume_breakpoint(struct drbdmon_req *req)
{
	int new_create = 0;
	struct drbdmon_param_resume_bp *param_bp = 
			(struct drbdmon_param_resume_bp *)req->ibufp;
	struct drbdmon_resume_bp_ctx *bp_ctx;
	
	printf("coming into %s, handle req\n", __func__);
	bp_ctx = drbdmon_lookup_resume_bp_ctx(param_bp->peer_ip);
	printf("%s find bp_ctx:%p\n", __func__, bp_ctx);
	if (bp_ctx == NULL) {
		bp_ctx = drbdmon_create_resume_bp_ctx(param_bp);
		new_create = 1;
	}	
	/* It's a invalid ip address most likely */
	if (bp_ctx == NULL)
		return DRBDMON_ERR_PARAM;
	
	printf("%s find bp_ctx:%p, new_create:%d\n", 
		__func__, bp_ctx, new_create);

	pthread_mutex_lock(&bp_ctx->ctx_mtx);
	list_insert_tail(&bp_ctx->resume_list, param_bp);
	pthread_mutex_unlock(&bp_ctx->ctx_mtx);

	if (new_create)
		(void) tpool_dispatch(bp_ctx->mon_ctx, 
					drbdmon_resume_bp_fn, bp_ctx);
	req->ibufp = NULL;
	req->ilen = 0;
	return 0;
}

static struct drbdmon_resume_bp_ctx *
drbdmon_lookup_resume_bp_ctx(const char *ip)
{
	uint32_t ip_bin = 0;
	struct drbdmon_resume_bp_ctx *ctx = NULL;

	/* It's a invalid ip address most likely */
	if (inet_pton(AF_INET, ip, &ip_bin) != 1)
		return NULL;

	pthread_mutex_lock(&drbdmon_bp_ctx_list_mtx);
	for (ctx = list_head(&drbdmon_bp_ctx_list); ctx;
	    	ctx = list_next(&drbdmon_bp_ctx_list, ctx)) {
		if (ctx->ip_addr == ip_bin)
			break;
	}
	pthread_mutex_unlock(&drbdmon_bp_ctx_list_mtx);
	return ctx;
}

static struct drbdmon_resume_bp_ctx *
drbdmon_create_resume_bp_ctx(struct drbdmon_param_resume_bp *param_bp)
{
	uint32_t ip_bin = 0;
	struct drbdmon_resume_bp_ctx *bp_ctx = NULL;
	tpool_t *tp = NULL, *tp_invalidate = NULL;
	
	bp_ctx = malloc(sizeof(struct drbdmon_resume_bp_ctx));
	if (!bp_ctx || 
		(inet_pton(AF_INET, param_bp->peer_ip, &ip_bin) != 1) ||
		!(tp = tpool_create(1, 1, 0, NULL)) ||
		!(tp_invalidate = tpool_create(1, 1, 0, NULL)))
		goto failed;

	printf("%s malloc and tpool_create succeed\n", __func__);
	memset(bp_ctx, 0, sizeof(*bp_ctx));
	bp_ctx->ip_addr = ip_bin;
	bp_ctx->mon_ctx = tp;
	bp_ctx->invalidate_ctx = tp_invalidate;
	strncpy(bp_ctx->peer_ip, param_bp->peer_ip, 16);
	list_link_init(&bp_ctx->entry);
	pthread_mutex_init(&bp_ctx->ctx_mtx, NULL);
	pthread_cond_init(&bp_ctx->ctx_cv, NULL);
	list_create(&bp_ctx->resume_list, sizeof(struct drbdmon_param_resume_bp),
				offsetof(struct drbdmon_param_resume_bp, opt));
	
	pthread_mutex_lock(&drbdmon_bp_ctx_list_mtx);
	list_insert_tail(&drbdmon_bp_ctx_list, bp_ctx);
	pthread_mutex_unlock(&drbdmon_bp_ctx_list_mtx);
	
	return bp_ctx;
failed:
	if (tp)
		tpool_destroy(tp);
	if (tp_invalidate)
		tpool_destroy(tp_invalidate);
	if (bp_ctx)
		free(bp_ctx);
	return NULL;
}

/* we consider that it'a resume event once link is from up to down to up */
static void
drbdmon_resume_bp_fn(struct drbdmon_resume_bp_ctx *bp_ctx)
{
	int hasdown = 0, hasup = 0, prev = 0, curr = 0;

	printf("coming into %s, checking ip:%s\n", 
		__func__, bp_ctx->peer_ip);
	pthread_mutex_lock(&bp_ctx->ctx_mtx);
	while (!bp_ctx->exit) {
		curr = 0;
		pthread_mutex_unlock(&bp_ctx->ctx_mtx);
		
		printf("%s DRBDMON_PING ip %s\n", __func__, bp_ctx->peer_ip);
		DRBDMON_PING(bp_ctx->peer_ip, curr);
		printf("%s prev:%d curr:%d hasdown:%d hasup:%d\n\n",
			__func__, prev, curr, hasdown, hasup);
		if (curr && hasup) {
			drbdmon_resume_bp_mixed(bp_ctx, 1);
			goto cont;
		}
		if (curr && (prev != curr) && hasdown) {
			hasdown = 0;
			hasup = 1;
			drbdmon_resume_bp_mixed(bp_ctx, 0);
			goto cont;
		}
		if (!curr && (prev != curr)) {
			hasdown = 1;
			hasup = 0;
		}
cont:
		prev = curr;
		pthread_mutex_lock(&bp_ctx->ctx_mtx);
		drbdmon_delay_locked(&bp_ctx->ctx_mtx, &bp_ctx->ctx_cv, 2000);
	}

	if (bp_ctx->exit) {
		pthread_mutex_unlock(&bp_ctx->ctx_mtx);
		syslog(LOG_ERR, "drbdmon thread[%s] exit", bp_ctx->peer_ip);
		return ;
	}
}

static void
drbdmon_resume_bp_mixed(struct drbdmon_resume_bp_ctx *bp_ctx, uint32_t mixed)
{
	list_t retryList, *invalidateList, *exchange;
	uint32_t status = 0;
	struct drbdmon_param_resume_bp *param_bp;
	
	list_create(&retryList, sizeof(struct drbdmon_param_resume_bp),
				offsetof(struct drbdmon_param_resume_bp, opt));

	invalidateList = malloc(sizeof(list_t));
	list_create(invalidateList, sizeof(struct drbdmon_param_resume_bp),
				offsetof(struct drbdmon_param_resume_bp, opt));

	pthread_mutex_lock(&bp_ctx->ctx_mtx);
	while (!list_is_empty(&bp_ctx->resume_list)) {
		param_bp = list_remove_head(&bp_ctx->resume_list);
		if (mixed && !param_bp->opt.mon.handled) { //add after link up.
			list_insert_head(&retryList, param_bp);
			continue;
		}
		DRBDMON_BP_INQR(param_bp->drbdX, status, param_bp->primary);
		printf("%s status:%d, resource:%s\n", __func__, 
				status, param_bp->resource);
		param_bp->opt.mon.handled = 1;
		exchange = status ? invalidateList : &retryList;
		list_insert_tail(exchange, param_bp);
	}

	if (!list_is_empty(&retryList))
		list_move_tail(&bp_ctx->resume_list, &retryList);
	pthread_mutex_unlock(&bp_ctx->ctx_mtx);
	
	if (!list_is_empty(invalidateList))
		drbdmon_resume_bp_invalidate(bp_ctx, invalidateList);
}

static void
drbdmon_resume_bp_invalidate(struct drbdmon_resume_bp_ctx *bp_ctx, 
			list_t *invalidateList)
{
	(void) tpool_dispatch(bp_ctx->invalidate_ctx, 
				drbdmon_resume_bp_invalidate_impl, invalidateList);
}

static void
drbdmon_resume_bp_invalidate_impl(list_t *invalidateList)
{
	struct drbdmon_param_resume_bp *param_bp;
	char validate_cmd[256] = "drbdadm connect ";
	uint32_t cmdlen = strlen(validate_cmd);
	
	while (!list_is_empty(invalidateList)) {
		param_bp = list_remove_head(invalidateList);

		memcpy(&validate_cmd[cmdlen], param_bp->resource, 
				strlen(param_bp->resource) + 1);
		printf("%s connect[%s]", __func__, validate_cmd);
		(void) system(validate_cmd);
		free(param_bp);
	}
	list_destroy(invalidateList);
	free(invalidateList);
}

int main(int argc, char **argv)
{
	unlink(DRBDMON_SOCKET);
	drbdmon_genl_global(&drbdmon_glp);
	drbdmon_resume_bp_init(drbdmon_glp);
	drbdmon_accept(drbdmon_glp, drbdmon_handle_connect);
}
