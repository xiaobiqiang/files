#include <sys/callout.h>
#include <linux/types.h>
#include <linux/timer.h>
#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/module.h>

static struct kmem_cache *callout_timer_cache = NULL;

static int 
timeout_generic(timeout_id_t tmid, 
	void (*func)(void *), void *arg, clock_t delta)
{
	int rc = 0;
	struct timer_list *tlist = *tmid;

	init_timer(tlist);
	tlist->function = (void (*)(unsigned long))func;
	tlist->data = (unsigned long)arg;
	tlist->expires = jiffies + delta;
	add_timer(tlist);
	return rc;
}

timeout_id_t
timeout(void (*func)(void *), void *arg, clock_t delta)
{
	int rc;
	void *timer = 
		kmem_cache_alloc(callout_timer_cache, GFP_KERNEL);
	timeout_id_t tmid = &timer;
	if (!timer)
		goto failed;

	rc = timeout_generic(tmid, func, arg, delta);
	if (!rc)
		goto failed;
	
	return tmid;

failed:
	if (timer)
		kmem_cache_free(callout_timer_cache, timer);
	return (timeout_id_t)NULL;
}
EXPORT_SYMBOL(timeout);

static long
untimeout_generic(timeout_id_t tmid)
{
	long rc;
	struct timer_list *tlist = *tmid;

	rc = (long)(tlist->expires - jiffies);
	del_timer_sync(tlist);
	
	kmem_cache_free(callout_timer_cache, tlist);
	*tmid = NULL;
	
	return rc;
}

clock_t
untimeout(timeout_id_t id_arg)
{
	long rc;
	if (!id_arg || !*id_arg)
		return 0;

	rc = untimeout_generic(id_arg);
	return (clock_t)(rc <= 0 ? 0 : rc);
}
EXPORT_SYMBOL(untimeout);

//TODO:
int callout_init(void)
{
	int rc = 0;;
	struct kmem_cache *cache = NULL;

	cache = kmem_cache_create("callout_timer_cache",
		sizeof(struct timer_list), 0, 0, NULL);
	if (!cache) {
		rc = -ENOMEM;
		goto out;
	}

	callout_timer_cache = cache;
	
out:
	return rc;
}

//TODO
void callout_fini(void)
{
	kmem_cache_destroy(callout_timer_cache);
}
