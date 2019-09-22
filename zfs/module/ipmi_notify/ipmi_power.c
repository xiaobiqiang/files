#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/errno.h>
#include <linux/blkdev.h>
#include <linux/sched.h>
#include <linux/workqueue.h>
#include <linux/delay.h>
#include <linux/pci.h>
#include <linux/interrupt.h>
#include <linux/aer.h>
#include <linux/kernel.h>
#include <linux/gfp.h>
#include <asm/device.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/miscdevice.h>
#include <asm/unaligned.h>
#include <sys/rwlock.h>
#include <sys/ipmi_notify_if.h>

#define IPMI_POWER_NAME		"power"

static void ipmi_power_exit(void);
static int ipmi_power_init(void);
static int ipmi_power_filter(uint32_t, void *, uint32_t);
static void ipmi_power_push(uint32_t, void *, uint32_t, void *, uint32_t);
static int ipmi_power_unsubscribe(struct ipmi_subscriber *);
static int ipmi_power_subscribe(struct ipmi_subscriber *);

struct ipmi_power_subscriber {
	struct ipmi_subscriber *ipmi_suber;
	struct list_head		entry;
};

struct ipmi_power_global {
	struct ipmi_notify_module 		*module;

	uint32_t						nsubers;
	struct list_head				subers;
	krwlock_t						subers_rw;
};

struct ipmi_notify_module ipmi_psu = {
	.name 			= IPMI_POWER_NAME,
	.module 		= IPMI_MODULE_POWER,
	.active 		= 0,

	._init 			= ipmi_power_init,
	._exit 			= ipmi_power_exit,
	._subscribe 	= ipmi_power_subscribe,
	._unsubscribe	= ipmi_power_unsubscribe,
	._filter		= ipmi_power_filter,
	._push			= ipmi_power_push,
};

static struct ipmi_power_global psu_global;

static struct ipmi_power_subscriber *
ipmi_power_lookup_subscriber_locked(struct ipmi_subscriber *ipmi_suber)
{
	int found = 0;
	struct ipmi_power_subscriber *power_suber;
	struct ipmi_subscriber *iter_suber;
	
	list_for_each_entry(power_suber, &psu_global.subers, entry) {
		iter_suber = power_suber->ipmi_suber;

		if (strcmp(ipmi_suber->name, iter_suber->name))
			continue;
		found = 1;
		break;
	}

	if (!found)
		power_suber = NULL;
	return power_suber;
}

static struct ipmi_power_subscriber *
ipmi_power_lookup_subscriber(struct ipmi_subscriber *ipmi_suber)
{
	struct ipmi_power_subscriber *power_suber;

	rw_enter(&psu_global.subers_rw, RW_READER);
	power_suber = ipmi_power_lookup_subscriber_locked(ipmi_suber);
	rw_exit(&psu_global.subers_rw);
	return power_suber;
}

static int
ipmi_power_subscribe(struct ipmi_subscriber *suber)
{
	int iRet = -EEXIST;
	struct ipmi_power_subscriber *power_suber;

	rw_enter(&psu_global.subers_rw, RW_WRITER);
	power_suber = ipmi_power_lookup_subscriber_locked(suber);
	if (power_suber) 
		goto out;
	
	power_suber = kzalloc(sizeof(*power_suber), GFP_KERNEL);
	INIT_LIST_HEAD(&power_suber->entry);
	power_suber->ipmi_suber = suber;
	suber->md_priv = power_suber;
	list_add_tail(&power_suber->entry, &psu_global.subers);
	psu_global.nsubers++;
	iRet = 0;
out:
	rw_exit(&psu_global.subers_rw);
	return iRet;
}

static int
ipmi_power_unsubscribe(struct ipmi_subscriber *suber)
{
	int iRet = -ENOENT;
	struct ipmi_power_subscriber *power_priv, *power_suber;

	if (!suber->md_priv)
		return -EINVAL;
	power_priv = suber->md_priv;
	
	rw_enter(&psu_global.subers_rw, RW_WRITER);
	power_suber = ipmi_power_lookup_subscriber_locked(suber);
	if (!power_suber || 
		(power_suber != power_priv) ||
		(power_suber->ipmi_suber != suber))
		goto out;

	list_del_init(&power_suber->entry);
	psu_global.nsubers--;
	iRet = 0;
out:
	rw_exit(&psu_global.subers_rw);
	if (!iRet && power_suber)
		kfree(power_suber);
	return iRet;	
}

static void
ipmi_power_push(uint32_t msg, void *ibuf, uint32_t ilen, 
			void *obuf, uint32_t olen)
{
	struct ipmi_power_subscriber *power_suber;
	struct ipmi_subscriber *iter_suber;

	printk(KERN_INFO "%s pushed msg is %d", __func__, msg);
	
	rw_enter(&psu_global.subers_rw, RW_WRITER);	
	list_for_each_entry(power_suber, &psu_global.subers, entry) {
		iter_suber = power_suber->ipmi_suber;
		if (iter_suber && iter_suber->msghdl)
			iter_suber->msghdl(msg, ibuf, ilen, obuf, olen);
	}
	rw_exit(&psu_global.subers_rw);
}

static int
ipmi_power_filter(uint32_t msg, void *ibuf, uint32_t ilen)
{
	if (!(msg & IPMI_IOC_PSU_MASK))
		return -ENOTSUP;
	return 0;
}

static int 
ipmi_power_init(void)
{
	bzero(&psu_global, sizeof(psu_global));
	psu_global.module = &ipmi_psu;
	INIT_LIST_HEAD(&psu_global.subers);
	rw_init(&psu_global.subers_rw, NULL, RW_DRIVER, NULL);
}

static void 
ipmi_power_exit(void)
{
	
}
