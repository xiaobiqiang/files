#include <sys/zfs_context.h>
#include <linux/seq_file.h>
#include <sys/kstat.h>
#include <sys/vmem.h>
#include <sys/spa.h>

static struct proc_dir_entry *proc_udb = NULL;
static struct proc_dir_entry *proc_spa_ref = NULL;

static kmutex_t zfs_udb_lock;
static char *zu_spa_ref_buf;
static ulong_t zu_spa_ref_buf_len = (128*1024);
static kmutex_t zu_spa_ref_lock;

static int
spa_ref_seq_show(struct seq_file *f, void *p)
{
	printk(KERN_NOTICE "spa_ref_seq_show p %p\n", p);
	seq_printf(f, "%s", zu_spa_ref_buf);
	return (0);
}

static void *
spa_ref_seq_start(struct seq_file *f, loff_t *pos)
{
	printk(KERN_NOTICE "spa_ref_seq_start pos %ld\n", (long int) *pos);
	if (*pos > 0)
		return (NULL);
	mutex_enter(&zu_spa_ref_lock);
	sprintf_all_spa_refcount(zu_spa_ref_buf, zu_spa_ref_buf_len);

	return ((void *) zu_spa_ref_buf);
}

static void *
spa_ref_seq_next(struct seq_file *f, void *p, loff_t *pos)
{
	printk(KERN_NOTICE "spa_ref_seq_next p %p pos %ld\n", p, (long int) *pos);
	return (NULL);
}

static void
spa_ref_seq_stop(struct seq_file *f, void *v)
{
	printk(KERN_NOTICE "spa_ref_seq_stop v %p\n", v);
	mutex_exit(&zu_spa_ref_lock);
}


static struct seq_operations spa_ref_seq_ops = {
        .show  = spa_ref_seq_show,
        .start = spa_ref_seq_start,
        .next  = spa_ref_seq_next,
        .stop  = spa_ref_seq_stop,
};

static int
proc_spa_ref_open(struct inode *inode, struct file *filp)
{
        return seq_open(filp, &spa_ref_seq_ops);
}

static struct file_operations proc_spa_ref_operations = {
        .open           = proc_spa_ref_open,
        .read           = seq_read,
        .llseek         = seq_lseek,
        .release        = seq_release,
};

static int
zfs_udb_proc_init(void)
{
	int rc = 0;

	proc_udb = proc_mkdir("zudb", NULL);
	if (proc_udb == NULL) {
		rc = -EUNATCH;
		goto out;
	}

	proc_spa_ref = proc_create_data("spa_ref", 0444,
		proc_udb, &proc_spa_ref_operations, NULL);
        if (proc_spa_ref == NULL) {
		rc = -EUNATCH;
		goto out;
	}

out:
	if (rc) {
		remove_proc_entry("spa_ref", proc_udb);
		remove_proc_entry("zudb", NULL);
	}
	return (rc);
}

static void
zfs_udb_proc_fini(void)
{
	remove_proc_entry("spa_ref", proc_udb);
	remove_proc_entry("zudb", NULL);
}

int
zfs_udb_init(void)
{
	mutex_init(&zfs_udb_lock, NULL, MUTEX_DEFAULT, NULL);
	mutex_init(&zu_spa_ref_lock, NULL, MUTEX_DEFAULT, NULL);
	zu_spa_ref_buf = vmem_alloc(zu_spa_ref_buf_len, KM_SLEEP);
	return (zfs_udb_proc_init());
}

void
zfs_udb_fini(void)
{
	zfs_udb_proc_fini();
	vmem_free(zu_spa_ref_buf, zu_spa_ref_buf_len);
	mutex_destroy(&zu_spa_ref_lock);
	mutex_destroy(&zfs_udb_lock);
}

