#include <sys/in.h>
#include <sys/callout.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <sys/cmn_err.h>

extern int callout_init(void);
extern int inet_init(void);
extern void callout_fini(void);
extern void inet_fini(void);

static int __init
utils_init(void)
{
	int rc;
	
	if ((rc = inet_init()) != 0) {
		cmn_err(CE_NOTE, 
			"inet_init error,rc is %d", rc);
		goto out;
	}

	if ((rc = callout_init()) != 0) {
		cmn_err(CE_NOTE, 
			"callout_init error,rc is %d", rc);
		goto failed_inet;
	}
failed_inet:
	inet_fini();
out:
	return rc;
}

static void __exit
utils_fini(void)
{
	callout_fini();
	inet_fini();
}

module_init(utils_init);
module_exit(utils_fini);
MODULE_LICENSE("GPL");
