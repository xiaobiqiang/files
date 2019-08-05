#include <sys/callout.h>
#include <linux/types.h>
#include <linux/timer.h>
#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <sys/atomic.h>
#include <sys/types.h>
#include <linux/list.h>
#include <linux/wait.h>
#include <linux/kthread.h>
#include <linux/atomic.h>

#define CALLOUT_N_CONSUMER		4
#define CALLOUT_HOLDER_DESTROY	0x0001

#define CALLOUT_WKFL_ACTIVE		0x0001
#define CALLOUT_WKFL_TERMINAL	0x0002

struct timeout_id {
	struct list_head 	ti_entry;
	struct timer_list 	ti_timer;
	void 				(*ti_func)(void *);
	void 				*ti_farg;
};

typedef struct {
	struct list_head 	cw_list1;
	struct list_head 	cw_list2;
	struct list_head 	*cw_todo;
	struct list_head 	*cw_doing;
	uint64_t			cw_task_num;
	uint64_t			cw_index;
	uint64_t			cw_flags;
	spinlock_t			cw_spin;
	struct task_struct	*cw_worker;
	wait_queue_head_t	cw_waiter;
} callout_worker_t;

typedef struct {
	struct kmem_cache 	*cc_cache;
	spinlock_t 			cc_spin;
	uint64_t			cc_flags;
	uint64_t			cc_index;
	uint64_t			cc_nworker;
	callout_worker_t	*cc_worker[CALLOUT_N_CONSUMER];
} callout_consumer_t;

static callout_consumer_t	*callout_holder = NULL;

static void 
timeout_call_fn(void *arg)
{
	uint64_t			cc_index;
	uint64_t			worker_idx;
	unsigned long 		flags;
	struct timeout_id 	*tid = arg;
	callout_worker_t	*wk;

	spin_lock_irqsave(&callout_holder->cc_spin, flags);
	if (callout_holder->cc_flags & CALLOUT_HOLDER_DESTROY) {
		spin_unlock_irqrestore(&callout_holder->cc_spin, flags);
		return ;
	}
	cc_index = ++callout_holder->cc_index;
	worker_idx = cc_index % callout_holder->cc_nworker;
	spin_unlock_irqrestore(&callout_holder->cc_spin, flags);

	wk = callout_holder->cc_worker[worker_idx];
	spin_lock_irqsave(&wk->cw_spin, flags);
	list_add_tail(&tid->ti_entry, wk->cw_todo);
	if( atomic_inc_64_nv(&wk->cw_task_num) == 1)
		wake_up(&wk->cw_waiter);
	spin_unlock_irqrestore(&wk->cw_spin, flags);
}

static int
timeout_enqueue(struct timeout_id *tid, clock_t delta)
{
	tid->ti_timer.function = timeout_call_fn;
	tid->ti_timer.data = (uintptr_t)tid;
	tid->ti_timer.expires = jiffies + delta;
	add_timer(&tid->ti_timer);
	return 0;
}


static int 
timeout_generic(timeout_id_t tmid, 
	void (*func)(void *), void *arg, clock_t delta)
{
	int rc = 0;
	uint64_t box_idx;
	struct timeout_id *tid = tmid;

	init_timer(&tid->ti_timer);
	INIT_LIST_HEAD(&tid->ti_entry);
	tid->ti_func = func;
	tid->ti_farg = arg;

	return timeout_enqueue(tid, delta);
}

timeout_id_t
timeout(void (*func)(void *), void *arg, clock_t delta)
{
	int rt;
	timeout_id_t rc = TIMEOUT_ID_INVALID;
	
	rc = kmem_cache_alloc(
		callout_holder->cc_cache,
		GFP_KERNEL);
	if (!rc)
		goto failed;
	
	if (delta <= 0)
		delta = 1;
	rt = timeout_generic(rc, func, arg, delta);
	if (rt)
		goto failed;
	
	return rc;

failed:
	if (rc != TIMEOUT_ID_INVALID)
		kmem_cache_free(
			callout_holder->cc_cache, 
			rc);
	return TIMEOUT_ID_INVALID;
}
EXPORT_SYMBOL(timeout);

static long
untimeout_generic(timeout_id_t tmid)
{
	long rc;
	struct timeout_id *tid = tmid;
	struct timer_list *tlist = &tid->ti_timer;
	rc = (long)(tlist->expires - jiffies);
	if (rc <= 0)
		return 0;

	del_timer_sync(tlist);
	kmem_cache_free(
		callout_holder->cc_cache, 
		tid);
	
	return rc;
}

clock_t
untimeout(timeout_id_t id_arg)
{
	long rc;
	if (!id_arg)
		return 0;

	rc = untimeout_generic(id_arg);
	return (clock_t)(rc <= 0 ? 0 : rc);
}
EXPORT_SYMBOL(untimeout);

static int 
callout_worker_fn(void *arg)
{
	boolean_t			terminal = B_FALSE;
	callout_worker_t 	*wk = arg;
	struct list_head	*temp;
	struct timeout_id	*task;
	
	while (!terminal) {
		spin_lock(&wk->cw_spin);
		if (list_empty(wk->cw_todo)) {
			spin_unlock(&wk->cw_spin);
			wait_event_timeout(wk->cw_waiter, 
				(terminal = (wk->cw_flags & CALLOUT_WKFL_TERMINAL)) ||
				atomic64_read((atomic64_t *)&wk->cw_task_num), 20*HZ);
			spin_lock(&wk->cw_spin);
		}
		
		temp = wk->cw_todo;
		wk->cw_todo = wk->cw_doing;
		wk->cw_doing = temp;
		spin_unlock(&wk->cw_spin);

		while (!list_empty(wk->cw_doing)) {
			atomic64_dec(&wk->cw_task_num);
			task = list_first_entry(wk->cw_doing, 
				struct timeout_id, ti_entry);
			list_del_init(&task->ti_entry);	
			if (task->ti_func)
				task->ti_func(task->ti_farg);
			kmem_cache_free(callout_holder->cc_cache, task);
		}
	}
out:
	printk(KERN_INFO "callout worker[index:%lu] terminate.", wk->cw_index);
	return 0;
}

static int
callout_init_single_work(callout_worker_t *wk, uint64_t index)
{
	INIT_LIST_HEAD(&wk->cw_list1);
	INIT_LIST_HEAD(&wk->cw_list2);
	wk->cw_todo = &wk->cw_list1;
	wk->cw_doing = &wk->cw_list2;
	wk->cw_index = index;
	wk->cw_flags |= CALLOUT_WKFL_ACTIVE;
	spin_lock_init(&wk->cw_spin);
	init_waitqueue_head(&wk->cw_waiter);
	wk->cw_worker = kthread_run(callout_worker_fn, wk, 
		"callout_worker_%llu", wk->cw_index);
	if (!wk->cw_worker || IS_ERR(wk->cw_worker))
		return -ENOMEM;
	return 0;
}

//TODO:
int callout_init(void)
{
	int i = 0, j = 0, rc = -ENOMEM;;
	struct kmem_cache *cache = NULL;
	callout_worker_t *wk;

	callout_holder = kzalloc(
		sizeof(callout_consumer_t), GFP_KERNEL);
	if (!callout_holder)
		goto failed;
	
	cache = kmem_cache_create("callout_cache",
		sizeof(struct timeout_id), 0, 0, NULL);
	if (!cache)
		goto failed;
	
	callout_holder->cc_cache = cache;
	spin_lock_init(&callout_holder->cc_spin);
	callout_holder->cc_nworker = CALLOUT_N_CONSUMER;

	for (i=0; i<CALLOUT_N_CONSUMER; i++) {
		wk = kzalloc(
			sizeof(callout_worker_t), GFP_KERNEL);
		if (!wk)
			goto failed;
		rc = callout_init_single_work(wk, i);
		if (rc) {
			kfree(wk);
			goto failed;
		}
		callout_holder->cc_worker[i] = wk;
	}
	return 0;
	
failed:
	for (j=0; j<i; j++)
		kfree(callout_holder->cc_worker[j]);
	if (cache)
		kmem_cache_destroy(cache);
	if (callout_holder)
		kfree(callout_holder);
	
	return rc;
}

//TODO
void callout_fini(void)
{
//	kmem_cache_destroy(callout_timer_cache);
}

