#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/nvpair.h>
#include <cmdparse.h>
#include <locale.h>
#include <assert.h>
#include <getopt.h>
#include <errno.h>
#include <sys/kmem.h>
#include <cmdparse.h>
#include <mltsas_comm.h>

static int Mlsas_enable(int, char **, cmdOptions_t *, void *);

#define Mlsas_OK					0

#define Mlsas_MAIN_FUNC				main
#define	VERSION_STRING_MAJOR	    "1"
#define	VERSION_STRING_MINOR	    "0"
#define	VERSION_STRING_MAX_LEN	    10

#define Mlsas_Hash_Bits				8
#define Mlsas_n_Slot				(1 << Mlsas_Hash_Bits)

#define Mltsas_Node			"/dev/Mlsas_dev"
#define Disk_directory		"/dev/disk/by-id/"
#define Disk_prefix			"scsi-3"

#define Mlsas_CMD_disk		"ls "Disk_directory" | grep "Disk_prefix" | grep -v part"
#define Mlsas_CMD_nDisk		Mlsas_CMD_disk" | wc -l"

#define	TEN_MS_NANOSLEEP  		10000000
#define	HUNDRED_MS_NANOSLEEP  	100000000

#define Mlsas_Maybe_sleep_on(cond, nano, try)	\
{	\
	unsigned long tried = 0;		\
	struct timespec ts;		\
		\
	bzero(&ts, sizeof(struct timespec));	\
	ts.tv_sec = (nano) / 1000000000;	\
	ts.tv_nsec = (nano) % 1000000000;	\
	while (!(cond) && (tried++ < try))		\
		nanosleep(&ts, NULL);	\
}

#define Mlsas_WAKE_locked(type)		\
{	\
	VERIFY(MUTEX_HELD(&gMlsas_Srv->MS_##type##_mtx));	\
		\
	if (gMlsas_Srv->MS_##type##_wait) {	\
		gMlsas_Srv->MS_##type##_wait = 0;	\
		cv_broadcast(&gMlsas_Srv->MS_##type##_cv);	\
	}	\
}
#define Mlsas_WAKE_event_locked		Mlsas_WAKE_locked(event)
#define Mlsas_WAKE_lpc_locked		Mlsas_WAKE_locked(lpc)

#define Mlsas_DAEMON_is_exit_locked(type) (gMlsas_Srv->MS_##type##_exit == 1)
#define Mlsas_EVENT_is_exit			Mlsas_DAEMON_is_exit_locked(event)
#define Mlsas_LPC_is_exit			Mlsas_DAEMON_is_exit_locked(lpc)

typedef enum Mlsas_async_event {
	Mlsas_EV_First,
	Mlsas_EV_New_Virt,
	Mlsas_EV_Last
} Mlsas_async_event_e;

typedef struct Mlsas_virt {
	list_node_t vt_one_phase_node;
	list_node_t vt_node;
	uint8_t vt_hash;
	uint8_t vt_pad[7];
	char 	vt_path[64];
} Mlsas_virt_t;

typedef struct Mlsas_virt_slot {
	uint8_t 	slt_hash;
	uint8_t 	slt_nele;
	uint8_t 	slt_pad[6];
	kmutex_t 	slt_mtx;
	list_t		slt_virt_list;
} Mlsas_virt_slot_t;

typedef struct Mlsas_async_evt {
	list_node_t ev_node;
	char ev_path[64];
	kmutex_t *ev_mtx;
	kcondvar_t *ev_cv;
	uint32_t ev_waiting;
	uint32_t ev_error;
	Mlsas_virt_t *ev_virt;
	Mlsas_async_event_e ev_type;
} Mlsas_async_evt_t;

typedef struct Mlsas_Srv {
	int MS_fd;
	kmutex_t MS_mtx;
	list_t MS_virt_one_phase_list;
	/* one phase */
	Mlsas_virt_slot_t MS_one_phase_vslt[Mlsas_n_Slot];
	/* two phase */
	Mlsas_virt_slot_t MS_vslt[Mlsas_n_Slot];

	kmutex_t MS_lpc_mtx;
	kcondvar_t MS_lpc_cv;
	tpool_t *MS_lpc_tp;

	tpool_t *MS_event_tp;
	kmutex_t MS_event_mtx;
	kcondvar_t MS_event_cv;
	list_t MS_event_list;
	uint32_t MS_n_event;
	
	uint32_t 	MS_event_wait:1,
				MS_event_exit:1,
				MS_lpc_wait:1,
				MS_lpc_exit:1,
				MS_bit_pad:28;
} Mlsas_Srv_t;

optionTbl_t longOptions[] = {
	{NULL, 0, 0, 0}
};

subCommandProps_t subcommands[] = {
	{"start", Mlsas_enable, NULL, NULL, NULL, OPERAND_NONE, NULL},
	{NULL, 0, NULL, NULL, 0, NULL, 0, NULL}
};

/* globals */
char *cmdName;
Mlsas_Srv_t Mlsas_Srv;
Mlsas_Srv_t *gMlsas_Srv = &Mlsas_Srv;

static char * Mlsas_Exec_for_newline(char *buf, int len, char *fmt, ...)
{
	FILE *ebuf = NULL;
	char cmd[512] = {0}, *tbuf;
	va_list ap;
	char *res = buf;
	
	va_start(ap, fmt);
	vsnprintf(cmd, 512, fmt, ap);
	va_end(ap);

	if (((ebuf = popen(cmd, "r")) == NULL) ||
		((tbuf = fgets(buf, len, ebuf)) == NULL)) 
		res = NULL;

	if (ebuf)
		pclose(ebuf);
	return res;
}

static int Mlsas_Exec_for_blank(char *fmt, ...)
{
	char cmd[512] = {0};
	va_list ap;
	
	va_start(ap, fmt);
	vsnprintf(cmd, 512, fmt, ap);
	va_end(ap);

	return system(cmd);
}

/*
 * llen is line buffer length, instead of strlen.
 */
static int Mlsas_Exec_for_iter_line(
		int (*fn)(void *priv, char *line, int llen),
		void *priv, const char *fmt, ...)
{
	char cmd[512] = {0};
	va_list ap;
	FILE *ebuf = NULL;
	char *line = NULL;
	int llen = 0, rval = 0;

	VERIFY(fn != NULL);
	
	va_start(ap, fmt);
	vsnprintf(cmd, 512, fmt, ap);
	va_end(ap);

	if ((ebuf = popen(cmd, "r")) == NULL) {
		fprintf(stdout, "popen command(%s) Fail\n", cmd);
		return -1;
	}

	while ((rval = getline(&line, &llen, ebuf)) != -1) {
		/* getline contains a newline */
		if (line[strlen(line) - 1] == '\n')
			line[strlen(line) - 1] = '\0';
		if ((rval = fn(priv, line, llen)) != Mlsas_OK) {
			free(line);
			break;
		}
		free(line);
		line = NULL;
		llen = 0;
	}

	return rval;
}

static int Mlsas_open(int *fd)
{
	int rval = 0;

	*fd = open(Mltsas_Node, O_RDONLY);
	if (*fd < 0) {
		rval = errno;
		fprintf(stdout, "Open Mltsas Fail,"
			" Error(%d)\n", errno);
	}
	return (rval);
}

static void Mlsas_close(int fd)
{
	if (fd > 0)
		close(fd);
}

static uint8_t Mlsas_Virt_id(const char *path)
{
	VERIFY(strlen(path) == 22 && 
		strncmp(path, "scsi-3", 6) == 0);
	
	return (path[20] << 8 | path[21]);
}

static Mlsas_virt_t *Mlsas_Lookup_virt_locked(Mlsas_virt_slot_t *slot,
		const char *path)
{
	Mlsas_virt_t *tmp, *next;

	VERIFY(MUTEX_HELD(&slot->slt_mtx));

	for (tmp = list_head(&slot->slt_virt_list);
		tmp != NULL; 
		next = list_next(&slot->slt_virt_list, tmp), 
			tmp = next) {
		if (!strncmp(tmp->vt_path, path, strlen(path))) {
			mutex_exit(&slot->slt_mtx);
			return tmp;
		}
	}

	return NULL;
}

static Mlsas_virt_t *Mlsas_Iter_virt_locked(Mlsas_virt_slot_t *slot,
		const char *path)
{
	Mlsas_virt_t *tmp, *next;

	VERIFY(MUTEX_HELD(&slot->slt_mtx));

	for (tmp = list_head(&slot->slt_virt_list);
		tmp != NULL; 
		next = list_next(&slot->slt_virt_list, tmp), 
			tmp = next) {
		if (!strncmp(tmp->vt_path, path, strlen(path))) {
			mutex_exit(&slot->slt_mtx);
			return tmp;
		}
	}

	return NULL;
}

static Mlsas_virt_t *Mlsas_New_virt(const char *path)
{
	Mlsas_virt_t *vt = NULL;
	uint8_t hash = Mlsas_Virt_id(path);

	VERIFY((vt = malloc(sizeof(Mlsas_virt_t))) != NULL);
	list_link_init(&vt->vt_one_phase_node);
	list_link_init(&vt->vt_node);
	strncpy(vt->vt_path, path, sizeof(vt->vt_path));
	vt->vt_hash = hash;

	return vt;
}

static void Mlsas_Free_virt(Mlsas_virt_t *vt)
{
	VERIFY(!list_link_active(&vt->vt_node));
	free(vt);
}

static int Mlsas_Ins_new_virt(Mlsas_virt_slot_t *slot, Mlsas_virt_t *vt)
{
	int rval = Mlsas_OK;

	mutex_enter(&slot->slt_mtx);
	if ((vt = Mlsas_Lookup_virt_locked(slot, 
			vt->vt_path)) != NULL)
		rval = -EEXIST;

	if (rval == Mlsas_OK) {
		list_insert_tail(&slot->slt_virt_list, vt);
		slot->slt_nele++;
	}

out:
	mutex_exit(&slot->slt_mtx);
	return rval;
}

static void Mlsas_Remove_virt_locked(Mlsas_virt_slot_t *slot, Mlsas_virt_t *vt)
{
	VERIFY(Mlsas_Lookup_virt_locked(slot, vt->vt_path) == vt);

	list_remove(&slot->slt_virt_list, vt);
	slot->slt_nele--;
}

static int Mlsas_Ena_ioctl(int fd)
{
	int rval = 0;
    Mlsas_iocdt_t ioc;
	nvlist_t *nvl = NULL;
	struct timespec ts;
	
	bzero(&ioc, sizeof(Mlsas_iocdt_t));
	ioc.Mlioc_magic = 0xdeafdeaf;

again:
	errno = Mlsas_OK;
	if((rval = ioctl(fd, Mlsas_Ioc_Ensvc, &ioc)) != 0) {
		switch (errno) {
		case EALREADY:
			rval = Mlsas_OK;
			break;
		case EAGAIN:
			ts.tv_sec = 0;
			ts.tv_nsec = TEN_MS_NANOSLEEP;
			nanosleep(&ts, NULL);
			goto again;
		default:
			break;
		}
	}

	if (rval != Mlsas_OK)
		fprintf(stdout, "Ena_ioctl Fail, rval(%d), Errno(%d)\n", 
			rval, errno);

    return rval;
}

static int Mlsas_Reg_phys_virt(void *priv, char *line, int llen, void *tag)
{
	nvlist_t *nvl = NULL;
	char path[64] = Disk_directory;
	Mlsas_iocdt_t xd;
	char *packed = NULL;
	int packed_len = 0, rval = 0;
	
	fprintf(stdout, "%s Reg_phys_virt[disk=%s]\n", 
		(char *)tag, line);

	VERIFY(strncmp(line, Disk_prefix, strlen(Disk_prefix)) == 0);

	(void) strncat(path, line, 64 - strlen(path));
	
	VERIFY(nvlist_alloc(&nvl, NV_UNIQUE_NAME, KM_SLEEP) == 0);
	VERIFY(nvlist_add_string(nvl, "path", path) == 0);
	VERIFY(nvlist_pack(nvl, &packed, &packed_len, 
		NV_ENCODE_NATIVE, KM_SLEEP) == 0);
	
	bzero(&xd, sizeof(Mlsas_iocdt_t));
	xd.Mlioc_magic = Mlsas_Ioc_Magic;
	xd.Mlioc_ibufptr = packed;
	xd.Mlioc_nibuf = packed_len;

	errno = 0;
	if ((rval = ioctl(gMlsas_Srv->MS_fd, 
			Mlsas_Ioc_Newminor, &xd)) != 0)
		rval = errno;
		fprintf(stdout, 
			"Reg_phys_virt[path=%s] ioctl[errno=%d]\n", 
			path, errno);
	else
		fprintf(stdout, 
			"Reg_phys_virt[path=%s] succeed\n", path);

	if (packed && packed_len)
		kmem_free(packed, packed_len);
	if (nvl)
		nvlist_free(nvl);
	
	return rval;	
}

static int Mlsas_Ena_reg_virt(void *priv, char *line, int llen)
{
	int rval = Mlsas_OK;
	Mlsas_virt_t *vt = NULL;
	
	if ((rval = Mlsas_Reg_phys_virt(priv, line, llen,
			__func__)) != Mlsas_OK)
		goto out;

	vt = Mlsas_New_virt(line);
	
	VERIFY(Mlsas_Ins_new_virt(
		&gMlsas_Srv->MS_vslt[vt->vt_hash], 
		vt) == Mlsas_OK);
	
	VERIFY(Mlsas_Ins_new_virt(
		&gMlsas_Srv->MS_one_phase_vslt[vt->vt_hash], 
		vt) == Mlsas_OK);

out:
	return Mlsas_OK;
}

static void Mlsas_Loop_Reg_phys(void)
{
	int ndisk = 0, odisk = 0, tmp;
	char buf[16] = {0};

	/* disks come into stable */
	Mlsas_Maybe_sleep_on(
		(ndisk = atoi(Mlsas_Exec_for_newline(buf, 16, 
				Mlsas_CMD_nDisk)),
			buf[0] = 0,
			tmp = odisk,
			odisk = ndisk,
			ndisk == tmp),
		HUNDRED_MS_NANOSLEEP,
		-1UL
	);

	Mlsas_Exec_for_iter_line(Mlsas_Ena_reg_virt, 
		NULL, Mlsas_CMD_disk);
}

/* come into second phase. */
static void Mlsas_HDL_async_event_NEW_VIRT(Mlsas_async_evt_t *ev)
{
	int rval = Mlsas_OK;
	Mlsas_virt_t *vt = ev->ev_virt;
	Mlsas_virt_slot_t *slot;
	
	VERIFY(ev->ev_type == Mlsas_EV_New_Virt);

	if ((rval = Mlsas_Reg_phys_virt(NULL, ev->ev_path, 0,
			__func__)) != Mlsas_OK) {
		ev->ev_error = rval;
		slot = &gMlsas_Srv->MS_one_phase_vslt[vt->vt_hash];
		mutex_enter(&slot->slt_mtx);
		if (!ev->ev_waiting && 
			list_link_active(&vt->vt_one_phase_node)) 
			Mlsas_Remove_virt_locked(slot, vt);
		mutex_exit(&slot->slt_mtx);
	}

	if (!ev->ev_error && !rval)
		VERIFY(Mlsas_Ins_new_virt(
			&gMlsas_Srv->MS_vslt[vt->vt_hash], 
			vt) == Mlsas_OK);
}

static void Mlsas_HDL_async_event(Mlsas_async_evt_t *ev)
{
	ev->ev_error = Mlsas_OK;

	VERIFY(!ev->ev_waiting || (ev->ev_mtx && ev->ev_cv));

	switch (ev->ev_type) {
	case Mlsas_EV_New_Virt:
		Mlsas_HDL_async_event_NEW_VIRT(ev);
		break;
	default: 
		fprintf(stdout, "WRONG Async Event(%u)\n", ev->ev_type);
		VERIFY(0);
	}

	if (!ev->ev_waiting)
		free(ev)
	else {
		mutex_enter(ev->ev_mtx);
		ev->ev_waiting = 0;
		cv_broadcast(ev->ev_cv);
		mutex_exit(ev->ev_mtx);
	}
}

static void Mlsas_Loop_event(void *priv)
{
	Mlsas_async_evt_t *ev;

	mutex_enter(&gMlsas_Srv->MS_event_mtx);
	Mlsas_WAKE_event_locked;
	gMlsas_Srv->MS_event_exit = 0;

loop:
	while (!Mlsas_EVENT_is_exit) {
		if (list_is_empty(&gMlsas_Srv->MS_event_list)) {
			gMlsas_Srv->MS_event_wait = 1;
			cv_wait(&gMlsas_Srv->MS_event_cv, 
				&gMlsas_Srv->MS_event_mtx);
			goto loop;
		}

		VERIFY(!list_is_empty(&gMlsas_Srv->MS_event_list));
		ev = list_remove_head(&gMlsas_Srv->MS_event_list);
		mutex_exit(&gMlsas_Srv->MS_event_mtx);

		Mlsas_HDL_async_event(ev);
		
		mutex_enter(&gMlsas_Srv->MS_event_mtx);
	}

	while (!list_is_empty(&gMlsas_Srv->MS_event_list)) {
		ev = list_remove_head(&gMlsas_Srv->MS_event_list);
		mutex_exit(&gMlsas_Srv->MS_event_mtx);
		
		Mlsas_HDL_async_event(ev);

		mutex_enter(&gMlsas_Srv->MS_event_mtx);
	}

	Mlsas_WAKE_event_locked;
	mutex_exit(&gMlsas_Srv->MS_event_mtx);
}

static int Mlsas_Link_every_virt(void *priv, char *line, int llen)
{
	list_t *vtlist = priv;

	list_insert_tail(vtlist, Mlsas_New_virt(line));
}

static void Mlsas_Itemize_lpc_virt(list_t *nwall, list_t *toins,
		list_t *todel, list_t *toreclaim)
{
	Mlsas_virt_t *vt, *next, *tmp;
	Mlsas_virt_slot_t *slot = NULL;
	int i = 0;

	VERIFY(list_is_empty(toins) && list_is_empty(todel) &&
		list_is_empty(toreclaim));
	
	/* merge to insert or reclaim */
	while (vt = list_remove_head(nwall)) {
		slot = &gMlsas_Srv->MS_one_phase_vslt[vt->vt_hash];
		mutex_enter(&slot->slt_mtx);
		if ((tmp = Mlsas_Lookup_virt_locked(slot, 
				vt->vt_path)) != NULL)
			list_insert_tail(toreclaim, vt);
		else
			list_insert_tail(toins, vt);
		mutex_exit(&slot->slt_mtx);
	}

	/* merge to delete */
	for ( ; i < Mlsas_n_Slot; i++) {
		slot = &gMlsas_Srv->MS_one_phase_vslt[i];
		mutex_enter(&slot->slt_mtx);
		if ()
	}
}

static void Mlsas_Loop_lpc_impl(void)
{
	list_t now_virt_list;
	list_t toins_virt_list;
	list_t todel_virt_list;
	list_t toreclaim_virt_list;
	
	list_create(&now_virt_list, sizeof(Mlsas_virt_t),
		offsetof(Mlsas_virt_t, vt_node));
	list_create(&toins_virt_list, sizeof(Mlsas_virt_t),
		offsetof(Mlsas_virt_t, vt_one_phase_node));
	list_create(&todel_virt_list, sizeof(Mlsas_virt_t),
		offsetof(Mlsas_virt_t, vt_one_phase_node));
	list_create(&toreclaim_virt_list, sizeof(Mlsas_virt_t),
		offsetof(Mlsas_virt_t, vt_one_phase_node));
	
	Mlsas_Exec_for_iter_line(Mlsas_Link_every_virt, 
		&now_virt_list, Mlsas_CMD_disk);

	mutex_enter(&gMlsas_Srv->MS_mtx);
	Mlsas_Itemize_lpc_virt(&now_virt_list, 
		&toins_virt_list, 
		&todel_virt_list,
		&toreclaim_virt_list);


	mutex_exit(&gMlsas_Srv->MS_mtx);
}

static void Mlsas_Loop_lpc(void *priv)
{
	mutex_enter(&gMlsas_Srv->MS_lpc_mtx);
	Mlsas_WAKE_lpc_locked;
	gMlsas_Srv->MS_lpc_exit = 0;

	while (!Mlsas_LPC_is_exit) {
		mutex_exit(&gMlsas_Srv->MS_lpc_mtx);

		Mlsas_Loop_lpc_impl();

		mutex_enter(&gMlsas_Srv->MS_lpc_mtx);
		if (!Mlsas_LPC_is_exit) {
			gMlsas_Srv->MS_lpc_wait = 1;
			cv_timedwait(&gMlsas_Srv->MS_lpc_cv, &gMlsas_Srv->MS_lpc_mtx,
				ddi_get_lbolt() + NSEC_TO_TICK(HUNDRED_MS_NANOSLEEP))
		}
	}

	Mlsas_WAKE_lpc_locked;
	mutex_exit(&gMlsas_Srv->MS_lpc_mtx);
}

static void Mlsas_Start_event_daemon(void)
{
	(void) fprintf(stdout, "STARTING Event Daemon\n");
	
	mutex_init(&gMlsas_Srv->MS_event_mtx, NULL, MUTEX_DEFAULT, NULL);
	cv_init(&gMlsas_Srv->MS_event_cv, NULL, CV_DRIVER, NULL);
	list_create(&gMlsas_Srv->MS_event_list, sizeof(Mlsas_async_evt_t),
		offsetof(Mlsas_async_evt_t, ev_node));
	gMlsas_Srv->MS_n_event = 0;
	gMlsas_Srv->MS_event_wait = 1;
	gMlsas_Srv->MS_event_exit = 1;
	
	VERIFY((gMlsas_Srv->MS_event_tp = tpool_create(
		1, 1, 0, NULL)) != NULL);

	mutex_enter(&gMlsas_Srv->MS_event_mtx);
	VERIFY(tpool_dispatch(gMlsas_Srv->MS_event_tp, 
		Mlsas_Loop_event, NULL) == 0);
	while (gMlsas_Srv->MS_event_exit)
		cv_wait(&gMlsas_Srv->MS_event_cv, &gMlsas_Srv->MS_event_mtx);
	mutex_exit(&gMlsas_Srv->MS_event_mtx);

	(void) fprintf(stdout, "START Event Daemon Complete\n");
}

static void Mlsas_Start_lpc_daemon(void)
{
	(void) fprintf(stdout, "STARTING LPC Daemon\n");
	
	mutex_init(&gMlsas_Srv->MS_lpc_mtx, NULL, MUTEX_DEFAULT, NULL);
	cv_init(&gMlsas_Srv->MS_lpc_cv, NULL, CV_DRIVER, NULL);
	gMlsas_Srv->MS_lpc_wait = 1;
	gMlsas_Srv->MS_lpc_exit = 1;
	
	VERIFY((gMlsas_Srv->MS_lpc_tp = tpool_create(
		1, 1, 0, NULL)) != NULL);

	mutex_enter(&gMlsas_Srv->MS_lpc_mtx);
	VERIFY(tpool_dispatch(gMlsas_Srv->MS_lpc_tp, 
		Mlsas_Loop_lpc, NULL) == 0);
	while (gMlsas_Srv->MS_lpc_exit)
		cv_wait(&gMlsas_Srv->MS_lpc_cv, &gMlsas_Srv->MS_lpc_mtx);
	mutex_exit(&gMlsas_Srv->MS_lpc_mtx);

	(void) fprintf(stdout, "START LPC Daemon Complete\n");
}

static void Mlsas_init_globals(Mlsas_Srv_t *gs)
{
	int i = 0;
	Mlsas_virt_slot_t *slot = NULL;
	
	bzero(gs, sizeof(Mlsas_Srv_t));
	mutex_init(&gs->MS_mtx, NULL, MUTEX_DEFAULT, NULL);
	list_create(&gs->MS_virt_one_phase_list, sizeof(Mlsas_virt_t),
		offsetof(Mlsas_virt_t, vt_one_phase_node));

	for ( ; i < Mlsas_n_Slot; i++) {
		slot = &gs->MS_vslt[i];
		slot->slt_hash = i;
		mutex_init(&slot->slt_mtx, NULL, MUTEX_DEFAULT, NULL);
		list_create(&slot->slt_virt_list, sizeof(Mlsas_virt_t),
			offsetof(Mlsas_virt_t, vt_node));
	}

	for ( ; i < Mlsas_n_Slot; i++) {
		slot = &gs->MS_one_phase_vslt[i];
		slot->slt_hash = i;
		mutex_init(&slot->slt_mtx, NULL, MUTEX_DEFAULT, NULL);
		list_create(&slot->slt_virt_list, sizeof(Mlsas_virt_t),
			offsetof(Mlsas_virt_t, vt_one_phase_node));
	}
}	

static int Mlsas_enable(int operandLen, char *operands[], 
		cmdOptions_t *options, void *args)
{
	int	rval = 0;
	
	(void) fprintf(stdout, "%s: %s\n", cmdName,
	    gettext("Requesting to enable Mltsas Service"));

	Mlsas_init_globals(gMlsas_Srv);

	if ((rval = Mlsas_open(&gMlsas_Srv->MS_fd)) != 0)
		return rval;
	(void) fprintf(stdout, "Mlsas_enable [fd=%d]\n", 
		gMlsas_Srv->MS_fd);

	if ((rval = Mlsas_Ena_ioctl(gMlsas_Srv->MS_fd)) != 0) 
		goto close_fd;
	(void) fprintf(stdout, "Mlsas_Ena_ioctl succeed\n");

	Mlsas_Loop_Reg_phys();
	Mlsas_Start_event_daemon();
	Mlsas_Start_lpc_daemon();
	
	return Mlsas_OK;
close_fd:
	Mlsas_close(gMlsas_Srv->MS_fd);
	return rval;
}

static char *
getExecBasename(char *execFullname)
{
	char *lastSlash, *execBasename;

	/* guard against '/' at end of command invocation */
	for (;;) {
		lastSlash = strrchr(execFullname, '/');
		if (lastSlash == NULL) {
			execBasename = execFullname;
			break;
		} else {
			execBasename = lastSlash + 1;
			if (*execBasename == '\0') {
				*lastSlash = '\0';
				continue;
			}
			break;
		}
	}
	return (execBasename);
}

int
Mlsas_MAIN_FUNC(int argc, char *argv[])
{
	synTables_t synTables;
	char versionString[VERSION_STRING_MAX_LEN];
	int ret;
	int funcRet;
	void *subcommandArgs = NULL;

	(void) setlocale(LC_ALL, "");
	/* set global command name */
	cmdName = getExecBasename(argv[0]);

	(void) snprintf(versionString, VERSION_STRING_MAX_LEN, "%s.%s",
	    VERSION_STRING_MAJOR, VERSION_STRING_MINOR);
	synTables.versionString = versionString;
	synTables.longOptionTbl = &longOptions[0];
	synTables.subCommandPropsTbl = &subcommands[0];

	ret = cmdParse(argc, argv, synTables, subcommandArgs, &funcRet);
	if (ret != 0) {
		return (ret);
	}

	return (funcRet);
} /* end main */
