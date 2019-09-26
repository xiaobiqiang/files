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
#include <sys/kmem.h>
#include <sys/vmem.h>
#include <sys/sunddi.h>
#include <sys/atomic.h>
#include <sys/ipmi_notify_if.h>

static int ipmi_notify_open(struct inode *, struct file *);
static int ipmi_notify_unlocked_ioctl(struct file *, uint32_t, uint64_t);
static void ipmi_notify_deactivate_modules(struct ipmi_notify_module **);
static void ipmi_notify_activate_modules(struct ipmi_notify_module **);
static int ipmi_notify_push(struct ipmi_notify_module *, uint32_t, void *, uint32_t, void *, uint32_t);
static int ipmi_notify_copyout_iocdata(intptr_t, int, ipmi_iocdata_t *, void *);
static int ipmi_notify_copyin_iocdata(intptr_t, int, ipmi_iocdata_t **, void **, void **);
	
static const struct file_operations ipmi_notify_fops = {
	.owner		= THIS_MODULE,
	.unlocked_ioctl	= ipmi_notify_unlocked_ioctl,
	.open		= ipmi_notify_open,
};

static struct miscdevice ipmi_notify_dev = {
	.minor	= MISC_DYNAMIC_MINOR,
	.name   = "ipmi_notify",
	.fops   = &ipmi_notify_fops,
};

extern struct ipmi_notify_module ipmi_psu;
static struct ipmi_notify_module *ipmi_notify_modules[] = {
	NULL,
	&ipmi_psu,
	NULL
};

static int 
ipmi_notify_open(struct inode *inodep, struct file *filep)
{
	return 0;
}

static int 
ipmi_notify_unlocked_ioctl(struct file *filep, 
			uint32_t cmd, uint64_t data)
{
	int iRet = -EINVAL;
	ipmi_iocdata_t *iocdata;
	void *inbuf, *outbuf;
	struct ipmi_notify_module *module;
	
	printk(KERN_INFO "1 cmd:%d", cmd);
	if (cmd != IPMI_IOC_CMD)
		return -EINVAL;

	printk(KERN_INFO "2");
	if (ipmi_notify_copyin_iocdata(data, 0, &iocdata,
				&inbuf, &outbuf) != 0)
		return -EFAULT;

	printk(KERN_INFO "3");
	if ((iocdata->module <= IPMI_MODULE_FIRST) ||
		(iocdata->module >= IPMI_MODULE_LAST))
		goto out;
	
	printk(KERN_INFO "4");
	module = ipmi_notify_modules[iocdata->module];
	iRet = ipmi_notify_push(module, iocdata->module_spec, inbuf, 
				iocdata->inlen, outbuf, iocdata->outlen);
	printk(KERN_INFO "5 iRet:%d", iRet);
	if (iRet == 0)
		iRet = ipmi_notify_copyout_iocdata(
					data, 0, iocdata, outbuf);
	printk(KERN_INFO "6 iRet:%d", iRet);
out:
	if (outbuf) {
		kmem_free(outbuf, iocdata->outlen);
		outbuf = NULL;
	}
	if (inbuf) {
		kmem_free(inbuf, iocdata->inlen);
		inbuf = NULL;
	}
	kmem_free(iocdata, sizeof (ipmi_iocdata_t));
	return (iRet);
}

static int
ipmi_notify_copyin_iocdata(intptr_t data, int mode, ipmi_iocdata_t **iocd,
						void **ibuf, void **obuf)
{
	int ret;

	*ibuf = NULL;
	*obuf = NULL;
	*iocd = kmem_zalloc(sizeof (ipmi_iocdata_t), KM_SLEEP);

	ret = ddi_copyin((void *)data, *iocd, sizeof (ipmi_iocdata_t), mode);
	if (ret)
		return (EFAULT);

	if ((*iocd)->inlen) {
		*ibuf = vmem_zalloc((*iocd)->inlen, KM_SLEEP);
		ret = ddi_copyin((void *)((intptr_t)(*iocd)->inbuf),
		    *ibuf, (*iocd)->inlen, mode);
	}
	if ((*iocd)->outlen)
		*obuf = vmem_zalloc((*iocd)->outlen, KM_SLEEP);

	if (ret == 0)
		return (0);
	ret = EFAULT;
copyin_iocdata_done:;
	if (*obuf) {
		vmem_free(*obuf, (*iocd)->outlen);
		*obuf = NULL;
	}
	if (*ibuf) {
		vmem_free(*ibuf, (*iocd)->inlen);
		*ibuf = NULL;
	}
	kmem_free(*iocd, sizeof (ipmi_iocdata_t));
	return (ret);
}

static int
ipmi_notify_copyout_iocdata(intptr_t data, int mode, ipmi_iocdata_t *iocd, void *obuf)
{
	int ret;

	if (iocd->outlen) {
		ret = ddi_copyout(obuf, (void *)(intptr_t)iocd->outbuf,
		    iocd->outlen, mode);
		if (ret)
			return (EFAULT);
	}
	ret = ddi_copyout(iocd, (void *)data, sizeof (ipmi_iocdata_t), mode);
	if (ret)
		return (EFAULT);
	return (0);
}

int
ipmi_notify_subscribe(struct ipmi_subscriber *suber)
{
	int iRet;
	struct ipmi_notify_module *module;
	
	if (!suber->msghdl ||
		(suber->module <= IPMI_MODULE_FIRST) ||
		(suber->module >= IPMI_MODULE_LAST))
		return -EINVAL;

	module = ipmi_notify_modules[suber->module];
	/* not support subscribe */
	if ((suber->flags & IPMI_SUBER_REALLY) ||
		!module->_subscribe)	
		return -ENOTSUP;

	iRet = module->_subscribe(suber);
	if (iRet)
		return iRet;
	atomic_or_32(&suber->flags, IPMI_SUBER_REALLY);
	return 0;
}
EXPORT_SYMBOL(ipmi_notify_subscribe);

int
ipmi_notify_unsubscribe(struct ipmi_subscriber *suber)
{
	int iRet;
	struct ipmi_notify_module *module;
	
	if (!suber->msghdl ||
		(suber->module <= IPMI_MODULE_FIRST) ||
		(suber->module >= IPMI_MODULE_LAST))
		return -EINVAL;

	module = ipmi_notify_modules[suber->module];
	/* not support subscribe */
	if (!(suber->flags & IPMI_SUBER_REALLY) ||
		!module->_unsubscribe)	
		return -ENOTSUP;

	iRet = module->_unsubscribe(suber);
	if (iRet)
		return iRet;
	atomic_and_32(&suber->flags, ~IPMI_SUBER_REALLY);
	return 0;
}
EXPORT_SYMBOL(ipmi_notify_unsubscribe);

static int
ipmi_notify_push(struct ipmi_notify_module *module, uint32_t module_cmd, 
			void *ibuf, uint32_t ilen, void *obuf, uint32_t olen)
{
	if (module->_filter(module_cmd, ibuf, ilen))
		return -ENOTSUP;

	module->_push(module_cmd, 
		ibuf, ilen, obuf, olen);
	return 0;
}

static void
ipmi_notify_activate_modules(struct ipmi_notify_module **modules)
{
	int active, rv;
	struct ipmi_notify_module *md;

	while ((md = *(++modules)) != NULL) {
		if (md->active)
			continue;
		
		active = 1;
		if (md->_init && ((rv = md->_init()) != 0)) {
			active = 0;
			printk(KERN_INFO "%s module[%s] activate failed, error[%d]",
				__func__, md->name, rv);
		}
		md->active = active;
	}
}

static void
ipmi_notify_deactivate_modules(struct ipmi_notify_module **modules)
{
	struct ipmi_notify_module *md;

	while ((md = *(++modules)) != NULL) {
		if (!md->active)
			continue;

		if (md->_exit)
			md->_exit();
		md->active = 0;
	}
}

static int __init
ipmi_notify_init(void)
{
	int iRet;
	
	ipmi_notify_activate_modules(ipmi_notify_modules);

	if ((iRet = misc_register(&ipmi_notify_dev)) != 0) {
		printk(KERN_WARNING "%s register ipmi_notify "
			"miscdevice failed, error:%d", __func__, iRet);
		goto failed_misc;
	}

	return 0;
failed_misc:
	ipmi_notify_deactivate_modules(ipmi_notify_modules);
failed_out:
	return iRet;
}

static void __exit
ipmi_notify_exit(void)
{
	printk(KERN_INFO "%s ipmi_notify exit", __func__);
	misc_deregister(&ipmi_notify_dev);
	ipmi_notify_deactivate_modules(ipmi_notify_modules);
}

module_init(ipmi_notify_init);
module_exit(ipmi_notify_exit);
MODULE_LICENSE("GPL");
