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
 * Copyright (C) 2008-2010 Lawrence Livermore National Security, LLC.
 * Produced at Lawrence Livermore National Laboratory (cf, DISCLAIMER).
 * Rewritten for Linux by Brian Behlendorf <behlendorf1@llnl.gov>.
 * LLNL-CODE-403049.
 * Copyright (c) 2012, 2014 by Delphix. All rights reserved.
 */

#include <sys/zfs_context.h>
#include <sys/spa.h>
#include <sys/vdev_disk.h>
#include <sys/vdev_impl.h>
#include <sys/fs/zfs.h>
#include <sys/zio.h>
#include <sys/sunldi.h>

#include <linux/genhd.h>

char *zfs_vdev_scheduler = VDEV_SCHEDULER;
static void *zfs_vdev_holder = VDEV_HOLDER;

/* here is double linked list. if there are too many disks, we may need hash table. */
LIST_HEAD(vdev_ev_mgt_list);
DEFINE_SPINLOCK(vdev_ev_mgt_lock);

#define WWN_LEN   (16)
#define WWN_STRLEN (WWN_LEN + 1)
/*
 * Virtual device vector for disks.
 */
typedef struct dio_request {
	zio_t			*dr_zio;	/* Parent ZIO */
	atomic_t		dr_ref;		/* References */
	int			dr_error;	/* Bio error */
	int			dr_bio_count;	/* Count of bio's */
	struct bio		*dr_bio[0];	/* Attached bio's */
} dio_request_t;

typedef struct vdev_ev_mgt {
    struct list_head list;
    vdev_t      *vd;
    char		*vdev_path;
    char        wwn_str[WWN_STRLEN];
    u64         wwn;
}vdev_ev_mgt_t;

static int vdev_ev_mgt_register(vdev_t *vd)
{
    vdev_ev_mgt_t *vdev_ev = NULL;
    vdev_disk_t *vdisk = NULL;
    struct block_device *bdev = NULL;
    char *wwn_key = "scsi-3";
    char *wwn_pos = NULL;
    unsigned long flags;

    wwn_pos = strstr(vd->vdev_path, wwn_key);
    if(wwn_pos == NULL) {
        printk(KERN_ERR "wwn key word[scsi-3] not found in disk:%s\n", vd->vdev_path);
        return -1;
    }

    vdisk = vd->vdev_tsd;
    bdev = vdisk->vd_bdev;

    //vdev_ev = kzalloc(sizeof (vdev_ev_mgt_t), GFP_KERNEL);
    vdev_ev = kmem_zalloc(sizeof (vdev_ev_mgt_t), KM_SLEEP);
    vdev_ev->vd = vd;
    vdev_ev->vdev_path = spa_strdup(vd->vdev_path);

    wwn_pos += strlen(wwn_key);
    strncpy(vdev_ev->wwn_str, wwn_pos, WWN_LEN);
    if(sscanf(vdev_ev->wwn_str, "%llx", &vdev_ev->wwn) == 0) {
        printk(KERN_ERR "invalid vdev_path for event management:%s\n", vd->vdev_path);
        spa_strfree(vdev_ev->vdev_path);
        kmem_free(vdev_ev, sizeof (vdev_ev_mgt_t));
        return -1;
    }

    spin_lock_irqsave(&vdev_ev_mgt_lock, flags);
    list_add_tail(&vdev_ev->list, &vdev_ev_mgt_list);
    spin_unlock_irqrestore(&vdev_ev_mgt_lock, flags);

    zfs_dbgmsg( "reg vdev[%s]\n", vdev_ev->wwn_str);

    return 0;
}

static int vdev_ev_mgt_unregister(vdev_t *vd)
{
    vdev_ev_mgt_t *vdev_ev = NULL;
    int found = FALSE;
    unsigned long flags;

    spin_lock_irqsave(&vdev_ev_mgt_lock, flags);
    list_for_each_entry(vdev_ev, &vdev_ev_mgt_list, list) {
        if(vdev_ev->vd == vd) {
            list_del_init(&vdev_ev->list);
            found = TRUE;
            break;
        }
    }
    spin_unlock_irqrestore(&vdev_ev_mgt_lock, flags);

    if(found == FALSE)
        return -1;

    zfs_dbgmsg( "unreg vdev[%s]\n", vdev_ev->wwn_str, __func__);

    spa_strfree(vdev_ev->vdev_path);
    kmem_free(vdev_ev, sizeof (vdev_ev_mgt_t));

    return 0;
}

static int wwnstr_match(char *s1, char *s2)
{
    char c1, c2;

    if(strncmp(s1, s2, WWN_LEN - 1) == 0) {
        c1 = s1[WWN_LEN - 1];
        c2 = s2[WWN_LEN - 1];

        if(c1 <= 'f' && c1 >= 'a')
            c1 = c1 - 'a' + 10;
        else if (c1 <= '9' && c1 >= '0')
            c1 = c1 - '0';
        else 
            return FALSE;

        if(c2 <= 'f' && c2 >= 'a')
            c2 = c2 - 'a' + 10;
        else if (c2 <= '9' && c2 >= '0')
            c2 = c2 - '0';
        else 
            return FALSE;

        if (c1 > c2) {
           return ((c1 - c2) < 4);
        } else {
            return ((c2 - c1) < 4);
        }
    }

    return FALSE;
}

static int wwn_match(u64 mgt_wwn, u64 tgt_wwn)
{
    /*if(mgt_wwn > tgt_wwn)
        return (mgt_wwn - tgt_wwn) < 4;

    return (tgt_wwn - mgt_wwn) < 4;*/
    
    return (mgt_wwn > tgt_wwn) ? (mgt_wwn - tgt_wwn) < 4 : (tgt_wwn - mgt_wwn) < 4 ;
}


static vdev_t* find_vdev_by_sas_addr(u64 sas_addr)
{
    vdev_ev_mgt_t *vdev_ev = NULL;
    vdev_t  *ret = NULL;
    unsigned long flags;
    /*char sas_wwn_str[WWN_STRLEN] = {0};

    sprintf(sas_wwn_str, "%016llx", sas_addr);
    printk(KERN_ERR "find for sas addr:0x%s\n", sas_wwn_str);
    printk(KERN_ERR "find for sas addr:0x%llx\n", sas_addr);*/

    spin_lock_irqsave(&vdev_ev_mgt_lock, flags);
    list_for_each_entry(vdev_ev, &vdev_ev_mgt_list, list) {
        /*printk(KERN_ERR "iterate sas wwn:%s\n", vdev_ev->wwn_str);
        if(wwnstr_match(vdev_ev->wwn_str, sas_wwn_str)) {
        printk(KERN_ERR "iterate sas wwn:%llx\n", vdev_ev->wwn);*/
        if(wwn_match(vdev_ev->wwn, sas_addr)) {
            ret = vdev_ev->vd;
            break;
        }
    }
    spin_unlock_irqrestore(&vdev_ev_mgt_lock, flags);

    return ret;
}


#ifdef HAVE_OPEN_BDEV_EXCLUSIVE
static fmode_t
vdev_bdev_mode(int smode)
{
	fmode_t mode = 0;

	ASSERT3S(smode & (FREAD | FWRITE), !=, 0);

	if (smode & FREAD)
		mode |= FMODE_READ;

	if (smode & FWRITE)
		mode |= FMODE_WRITE;

	return (mode);
}
#else
static int
vdev_bdev_mode(int smode)
{
	int mode = 0;

	ASSERT3S(smode & (FREAD | FWRITE), !=, 0);

	if ((smode & FREAD) && !(smode & FWRITE))
		mode = MS_RDONLY;

	return (mode);
}
#endif /* HAVE_OPEN_BDEV_EXCLUSIVE */

static uint64_t
bdev_capacity(struct block_device *bdev)
{
	struct hd_struct *part = bdev->bd_part;

	/* The partition capacity referenced by the block device */
	if (part)
		return (part->nr_sects << 9);

	/* Otherwise assume the full device capacity */
	return (get_capacity(bdev->bd_disk) << 9);
}

static void
vdev_disk_error(zio_t *zio)
{
#ifdef ZFS_DEBUG
	printk("ZFS: zio error=%d type=%d offset=%llu size=%llu "
	    "flags=%x delay=%llu\n", zio->io_error, zio->io_type,
	    (u_longlong_t)zio->io_offset, (u_longlong_t)zio->io_size,
	    zio->io_flags, (u_longlong_t)zio->io_delay);
#endif
}

/*
 * Use the Linux 'noop' elevator for zfs managed block devices.  This
 * strikes the ideal balance by allowing the zfs elevator to do all
 * request ordering and prioritization.  While allowing the Linux
 * elevator to do the maximum front/back merging allowed by the
 * physical device.  This yields the largest possible requests for
 * the device with the lowest total overhead.
 */
static int
vdev_elevator_switch(vdev_t *v, char *elevator)
{
	vdev_disk_t *vd = v->vdev_tsd;
	struct block_device *bdev = vd->vd_bdev;
	struct request_queue *q = bdev_get_queue(bdev);
	char *device = bdev->bd_disk->disk_name;
	int error;

	/*
	 * Skip devices which are not whole disks (partitions).
	 * Device-mapper devices are excepted since they may be whole
	 * disks despite the vdev_wholedisk flag, in which case we can
	 * and should switch the elevator. If the device-mapper device
	 * does not have an elevator (i.e. dm-raid, dm-crypt, etc.) the
	 * "Skip devices without schedulers" check below will fail.
	 */
	if (!v->vdev_wholedisk && strncmp(device, "dm-", 3) != 0)
		return (0);

	/* Skip devices without schedulers (loop, ram, dm, etc) */
	if (!q->elevator || !blk_queue_stackable(q))
		return (0);

	/* Leave existing scheduler when set to "none" */
	if ((strncmp(elevator, "none", 4) == 0) && (strlen(elevator) == 4))
		return (0);

#ifdef HAVE_ELEVATOR_CHANGE
	error = elevator_change(q, elevator);
#else
	/*
	 * For pre-2.6.36 kernels elevator_change() is not available.
	 * Therefore we fall back to using a usermodehelper to echo the
	 * elevator into sysfs;  This requires /bin/echo and sysfs to be
	 * mounted which may not be true early in the boot process.
	 */
#define	SET_SCHEDULER_CMD \
	"exec 0</dev/null " \
	"     1>/sys/block/%s/queue/scheduler " \
	"     2>/dev/null; " \
	"echo %s"

	{
		char *argv[] = { "/bin/sh", "-c", NULL, NULL };
		char *envp[] = { NULL };

		argv[2] = kmem_asprintf(SET_SCHEDULER_CMD, device, elevator);
		error = call_usermodehelper(argv[0], argv, envp, UMH_WAIT_PROC);
		strfree(argv[2]);
	}
#endif /* HAVE_ELEVATOR_CHANGE */
	if (error)
		printk("ZFS: Unable to set \"%s\" scheduler for %s (%s): %d\n",
		    elevator, v->vdev_path, device, error);

	return (error);
}

/*
 * Expanding a whole disk vdev involves invoking BLKRRPART on the
 * whole disk device. This poses a problem, because BLKRRPART will
 * return EBUSY if one of the disk's partitions is open. That's why
 * we have to do it here, just before opening the data partition.
 * Unfortunately, BLKRRPART works by dropping all partitions and
 * recreating them, which means that for a short time window, all
 * /dev/sdxN device files disappear (until udev recreates them).
 * This means two things:
 *  - When we open the data partition just after a BLKRRPART, we
 *    can't do it using the normal device file path because of the
 *    obvious race condition with udev. Instead, we use reliable
 *    kernel APIs to get a handle to the new partition device from
 *    the whole disk device.
 *  - Because vdev_disk_open() initially needs to find the device
 *    using its path, multiple vdev_disk_open() invocations in
 *    short succession on the same disk with BLKRRPARTs in the
 *    middle have a high probability of failure (because of the
 *    race condition with udev). A typical situation where this
 *    might happen is when the zpool userspace tool does a
 *    TRYIMPORT immediately followed by an IMPORT. For this
 *    reason, we only invoke BLKRRPART in the module when strictly
 *    necessary (zpool online -e case), and rely on userspace to
 *    do it when possible.
 */
static struct block_device *
vdev_disk_rrpart(const char *path, int mode, vdev_disk_t *vd)
{
#if defined(HAVE_3ARG_BLKDEV_GET) && defined(HAVE_GET_GENDISK)
	struct block_device *bdev, *result = ERR_PTR(-ENXIO);
	struct gendisk *disk;
	int error, partno;

	bdev = vdev_bdev_open(path, vdev_bdev_mode(mode), zfs_vdev_holder);
	if (IS_ERR(bdev))
		return (bdev);

	disk = get_gendisk(bdev->bd_dev, &partno);
	vdev_bdev_close(bdev, vdev_bdev_mode(mode));

	if (disk) {
		bdev = bdget(disk_devt(disk));
		if (bdev) {
			error = blkdev_get(bdev, vdev_bdev_mode(mode), vd);
			if (error == 0)
				error = ioctl_by_bdev(bdev, BLKRRPART, 0);
			vdev_bdev_close(bdev, vdev_bdev_mode(mode));
		}

		bdev = bdget_disk(disk, partno);
		if (bdev) {
			error = blkdev_get(bdev,
			    vdev_bdev_mode(mode) | FMODE_EXCL, vd);
			if (error == 0)
				result = bdev;
		}
		put_disk(disk);
	}

	return (result);
#else
	return (ERR_PTR(-EOPNOTSUPP));
#endif /* defined(HAVE_3ARG_BLKDEV_GET) && defined(HAVE_GET_GENDISK) */
}

static int
vdev_disk_open(vdev_t *v, uint64_t *psize, uint64_t *max_psize,
    uint64_t *ashift)
{
	struct block_device *bdev = ERR_PTR(-ENXIO);
	vdev_disk_t *vd;
	int count = 0, mode, block_size;
    char bdev_name[BDEVNAME_SIZE];
    int bdev_retry_count = 50;
    int ms_shift = 0;

	/* Must have a pathname and it must be absolute. */
	if (v->vdev_path == NULL || v->vdev_path[0] != '/') {
		v->vdev_stat.vs_aux = VDEV_AUX_BAD_LABEL;
		return (SET_ERROR(EINVAL));
	}

	/*
	 * Reopen the device if it's not currently open. Otherwise,
	 * just update the physical size of the device.
	 */
	if (v->vdev_tsd != NULL) {
		ASSERT(v->vdev_reopening);
		vd = v->vdev_tsd;
		goto skip_open;
	}

	vd = kmem_zalloc(sizeof (vdev_disk_t), KM_SLEEP);
	if (vd == NULL)
		return (SET_ERROR(ENOMEM));

	/*
	 * Devices are always opened by the path provided at configuration
	 * time.  This means that if the provided path is a udev by-id path
	 * then drives may be recabled without an issue.  If the provided
	 * path is a udev by-path path, then the physical location information
	 * will be preserved.  This can be critical for more complicated
	 * configurations where drives are located in specific physical
	 * locations to maximize the systems tolerence to component failure.
	 * Alternatively, you can provide your own udev rule to flexibly map
	 * the drives as you see fit.  It is not advised that you use the
	 * /dev/[hd]d devices which may be reordered due to probing order.
	 * Devices in the wrong locations will be detected by the higher
	 * level vdev validation.
	 *
	 * The specified paths may be briefly removed and recreated in
	 * response to udev events.  This should be exceptionally unlikely
	 * because the zpool command makes every effort to verify these paths
	 * have already settled prior to reaching this point.  Therefore,
	 * a ENOENT failure at this point is highly likely to be transient
	 * and it is reasonable to sleep and retry before giving up.  In
	 * practice delays have been observed to be on the order of 100ms.
	 */
	mode = spa_mode(v->vdev_spa);
	if (v->vdev_wholedisk && v->vdev_expanding) {
		bdev = vdev_disk_rrpart(v->vdev_path, mode, vd);
        bdev_retry_count += 6;
    }

	while (IS_ERR(bdev) && count < bdev_retry_count) {
		bdev = vdev_bdev_open(v->vdev_path,
		    vdev_bdev_mode(mode), zfs_vdev_holder);
		if (unlikely(PTR_ERR(bdev) == -ENOENT)) {
            if(count + 1 > 50) {
                ms_shift = count + 1 - 50;
            }
			msleep(10 << ms_shift);
			count++;
		} else if (IS_ERR(bdev)) {
			break;
		}
	}

	if (IS_ERR(bdev)) {
		dprintf("failed open v->vdev_path=%s, error=%d count=%d\n",
		    v->vdev_path, -PTR_ERR(bdev), count);
		kmem_free(vd, sizeof (vdev_disk_t));
		return (SET_ERROR(-PTR_ERR(bdev)));
	}

	v->vdev_tsd = vd;
	vd->vd_bdev = bdev;

#if 0
    printk(KERN_ERR "%s: [vdev_path:%s]\n",   __func__, v->vdev_path);
    printk(KERN_ERR "%s: [bdev name:%s]\n", __func__, bdevname(bdev, bdev_name));
    printk(KERN_ERR "%s: [device name:%s]\n", __func__, dev_name(disk_to_dev(bdev->bd_disk)));
#endif

    vdev_ev_mgt_register(v);

skip_open:
	/*  Determine the physical block size */
	block_size = vdev_bdev_block_size(vd->vd_bdev);

	/* Clear the nowritecache bit, causes vdev_reopen() to try again. */
	v->vdev_nowritecache = B_FALSE;

	/* Inform the ZIO pipeline that we are non-rotational */
	v->vdev_nonrot = blk_queue_nonrot(bdev_get_queue(vd->vd_bdev));

	/* Physical volume size in bytes */
	*psize = bdev_capacity(vd->vd_bdev);

	/* TODO: report possible expansion size */
	*max_psize = *psize;

	/* Based on the minimum sector size set the block size */
	*ashift = highbit64(MAX(block_size, SPA_MINBLOCKSIZE)) - 1;

	/* Try to set the io scheduler elevator algorithm */
	(void) vdev_elevator_switch(v, zfs_vdev_scheduler);

	return (0);
}

static void
vdev_disk_close(vdev_t *v)
{
	vdev_disk_t *vd = v->vdev_tsd;

	if (v->vdev_reopening || vd == NULL)
		return;

	if (vd->vd_bdev != NULL)
		vdev_bdev_close(vd->vd_bdev,
		    vdev_bdev_mode(spa_mode(v->vdev_spa)));

	kmem_free(vd, sizeof (vdev_disk_t));
	v->vdev_tsd = NULL;
    vdev_ev_mgt_unregister(v);  /* NOTICE: We need to check whether it should be unregistered. */
}

static dio_request_t *
vdev_disk_dio_alloc(int bio_count)
{
	dio_request_t *dr;
	int i;

	dr = kmem_zalloc(sizeof (dio_request_t) +
	    sizeof (struct bio *) * bio_count, KM_SLEEP);
	if (dr) {
		atomic_set(&dr->dr_ref, 0);
		dr->dr_bio_count = bio_count;
		dr->dr_error = 0;

		for (i = 0; i < dr->dr_bio_count; i++)
			dr->dr_bio[i] = NULL;
	}

	return (dr);
}

static void
vdev_disk_dio_free(dio_request_t *dr)
{
	int i;

	for (i = 0; i < dr->dr_bio_count; i++)
		if (dr->dr_bio[i])
			bio_put(dr->dr_bio[i]);

	kmem_free(dr, sizeof (dio_request_t) +
	    sizeof (struct bio *) * dr->dr_bio_count);
}

static void
vdev_disk_dio_get(dio_request_t *dr)
{
	atomic_inc(&dr->dr_ref);
}

static int
vdev_disk_dio_put(dio_request_t *dr)
{
	int rc = atomic_dec_return(&dr->dr_ref);

	/*
	 * Free the dio_request when the last reference is dropped and
	 * ensure zio_interpret is called only once with the correct zio
	 */
	if (rc == 0) {
		zio_t *zio = dr->dr_zio;
		int error = dr->dr_error;

		vdev_disk_dio_free(dr);

		if (zio) {
			zio->io_delay = jiffies_64 - zio->io_delay;
			zio->io_error = error;
			ASSERT3S(zio->io_error, >=, 0);
			if (zio->io_error)
				vdev_disk_error(zio);
			zio_interrupt(zio);
		}
	}

	return (rc);
}

BIO_END_IO_PROTO(vdev_disk_physio_completion, bio, error)
{
	dio_request_t *dr = bio->bi_private;
	int rc;

	if (dr->dr_error == 0) {
#ifdef HAVE_1ARG_BIO_END_IO_T
		dr->dr_error = -(bio->bi_error);
#else
		if (error)
			dr->dr_error = -(error);
		else if (!test_bit(BIO_UPTODATE, &bio->bi_flags))
			dr->dr_error = EIO;
#endif
	}

	/* Drop reference aquired by __vdev_disk_physio */
	rc = vdev_disk_dio_put(dr);
}

static inline unsigned long
bio_nr_pages(void *bio_ptr, unsigned int bio_size)
{
	return ((((unsigned long)bio_ptr + bio_size + PAGE_SIZE - 1) >>
	    PAGE_SHIFT) - ((unsigned long)bio_ptr >> PAGE_SHIFT));
}

static unsigned int
bio_map(struct bio *bio, void *bio_ptr, unsigned int bio_size)
{
	unsigned int offset, size, i;
	struct page *page;

	offset = offset_in_page(bio_ptr);
	for (i = 0; i < bio->bi_max_vecs; i++) {
		size = PAGE_SIZE - offset;

		if (bio_size <= 0)
			break;

		if (size > bio_size)
			size = bio_size;

		if (is_vmalloc_addr(bio_ptr))
			page = vmalloc_to_page(bio_ptr);
		else
			page = virt_to_page(bio_ptr);

		/*
		 * Some network related block device uses tcp_sendpage, which
		 * doesn't behave well when using 0-count page, this is a
		 * safety net to catch them.
		 */
		ASSERT3S(page_count(page), >, 0);

		if (bio_add_page(bio, page, size, offset) != size)
			break;

		bio_ptr  += size;
		bio_size -= size;
		offset = 0;
	}

	return (bio_size);
}

static inline void
vdev_submit_bio_impl(struct bio *bio)
{
#ifdef HAVE_1ARG_SUBMIT_BIO
	submit_bio(bio);
#else
	submit_bio(0, bio);
#endif
}

static inline void
vdev_submit_bio(struct bio *bio)
{
#ifdef HAVE_CURRENT_BIO_TAIL
	struct bio **bio_tail = current->bio_tail;
	current->bio_tail = NULL;
	vdev_submit_bio_impl(bio);
	current->bio_tail = bio_tail;
#else
	struct bio_list *bio_list = current->bio_list;
	current->bio_list = NULL;
	vdev_submit_bio_impl(bio);
	current->bio_list = bio_list;
#endif
}

static int
__vdev_disk_physio(struct block_device *bdev, zio_t *zio, caddr_t kbuf_ptr,
    size_t kbuf_size, uint64_t kbuf_offset, int rw, int flags)
{
	dio_request_t *dr;
	caddr_t bio_ptr;
	uint64_t bio_offset;
	int bio_size, bio_count = 16;
	int i = 0, error = 0;
#if defined(HAVE_BLK_QUEUE_HAVE_BLK_PLUG)
	struct blk_plug plug;
#endif

	ASSERT3U(kbuf_offset + kbuf_size, <=, bdev->bd_inode->i_size);

retry:
	dr = vdev_disk_dio_alloc(bio_count);
	if (dr == NULL)
		return (ENOMEM);

	if (zio && !(zio->io_flags & (ZIO_FLAG_IO_RETRY | ZIO_FLAG_TRYHARD)))
		bio_set_flags_failfast(bdev, &flags);

	dr->dr_zio = zio;

	/*
	 * When the IO size exceeds the maximum bio size for the request
	 * queue we are forced to break the IO in multiple bio's and wait
	 * for them all to complete.  Ideally, all pool users will set
	 * their volume block size to match the maximum request size and
	 * the common case will be one bio per vdev IO request.
	 */
	bio_ptr    = kbuf_ptr;
	bio_offset = kbuf_offset;
	bio_size   = kbuf_size;
	for (i = 0; i <= dr->dr_bio_count; i++) {

		/* Finished constructing bio's for given buffer */
		if (bio_size <= 0)
			break;

		/*
		 * By default only 'bio_count' bio's per dio are allowed.
		 * However, if we find ourselves in a situation where more
		 * are needed we allocate a larger dio and warn the user.
		 */
		if (dr->dr_bio_count == i) {
			vdev_disk_dio_free(dr);
			bio_count *= 2;
			goto retry;
		}

		/* bio_alloc() with __GFP_WAIT never returns NULL */
		dr->dr_bio[i] = bio_alloc(GFP_NOIO,
		    MIN(bio_nr_pages(bio_ptr, bio_size), BIO_MAX_PAGES));
		if (unlikely(dr->dr_bio[i] == NULL)) {
			vdev_disk_dio_free(dr);
			return (ENOMEM);
		}

		/* Matching put called by vdev_disk_physio_completion */
		vdev_disk_dio_get(dr);

		dr->dr_bio[i]->bi_bdev = bdev;
		BIO_BI_SECTOR(dr->dr_bio[i]) = bio_offset >> 9;
		dr->dr_bio[i]->bi_end_io = vdev_disk_physio_completion;
		dr->dr_bio[i]->bi_private = dr;
		bio_set_op_attrs(dr->dr_bio[i], rw, flags);

		/* Remaining size is returned to become the new size */
		bio_size = bio_map(dr->dr_bio[i], bio_ptr, bio_size);

		/* Advance in buffer and construct another bio if needed */
		bio_ptr    += BIO_BI_SIZE(dr->dr_bio[i]);
		bio_offset += BIO_BI_SIZE(dr->dr_bio[i]);
	}

	/* Extra reference to protect dio_request during vdev_submit_bio */
	vdev_disk_dio_get(dr);
	if (zio)
		zio->io_delay = jiffies_64;

#if defined(HAVE_BLK_QUEUE_HAVE_BLK_PLUG)
	if (dr->dr_bio_count > 1)
		blk_start_plug(&plug);
#endif

	/* Submit all bio's associated with this dio */
	for (i = 0; i < dr->dr_bio_count; i++)
		if (dr->dr_bio[i])
			vdev_submit_bio(dr->dr_bio[i]);

#if defined(HAVE_BLK_QUEUE_HAVE_BLK_PLUG)
	if (dr->dr_bio_count > 1)
		blk_finish_plug(&plug);
#endif

	(void) vdev_disk_dio_put(dr);

	return (error);
}

BIO_END_IO_PROTO(vdev_disk_io_flush_completion, bio, rc)
{
	zio_t *zio = bio->bi_private;
#ifdef HAVE_1ARG_BIO_END_IO_T
	int rc = bio->bi_error;
#endif

	zio->io_delay = jiffies_64 - zio->io_delay;
	zio->io_error = -rc;
	if (rc && (rc == -EOPNOTSUPP))
		zio->io_vd->vdev_nowritecache = B_TRUE;

	bio_put(bio);
	ASSERT3S(zio->io_error, >=, 0);
	if (zio->io_error)
		vdev_disk_error(zio);
	zio_interrupt(zio);
}

static int
vdev_disk_io_flush(struct block_device *bdev, zio_t *zio)
{
	struct request_queue *q;
	struct bio *bio;

	q = bdev_get_queue(bdev);
	if (!q)
		return (ENXIO);

	bio = bio_alloc(GFP_NOIO, 0);
	/* bio_alloc() with __GFP_WAIT never returns NULL */
	if (unlikely(bio == NULL))
		return (ENOMEM);

	bio->bi_end_io = vdev_disk_io_flush_completion;
	bio->bi_private = zio;
	bio->bi_bdev = bdev;
	zio->io_delay = jiffies_64;
	bio_set_flush(bio);
	vdev_submit_bio(bio);
	invalidate_bdev(bdev);

	return (0);
}

static void
vdev_disk_io_start(zio_t *zio)
{
	vdev_t *v = zio->io_vd;
	vdev_disk_t *vd = v->vdev_tsd;
	int rw, flags, error;

	switch (zio->io_type) {
	case ZIO_TYPE_IOCTL:

		if (!vdev_readable(v)) {
			zio->io_error = SET_ERROR(ENXIO);
			zio_interrupt(zio);
			return;
		}

		switch (zio->io_cmd) {
		case DKIOCFLUSHWRITECACHE:

			if (zfs_nocacheflush)
				break;

			if (v->vdev_nowritecache) {
				zio->io_error = SET_ERROR(ENOTSUP);
				break;
			}

			error = vdev_disk_io_flush(vd->vd_bdev, zio);
			if (error == 0)
				return;

			zio->io_error = error;
			if (error == ENOTSUP)
				v->vdev_nowritecache = B_TRUE;

			break;

		default:
			zio->io_error = SET_ERROR(ENOTSUP);
		}

		zio_execute(zio);
		return;
	case ZIO_TYPE_WRITE:
		rw = WRITE;
#if defined(HAVE_BLK_QUEUE_HAVE_BIO_RW_UNPLUG)
		flags = (1 << BIO_RW_UNPLUG);
#elif defined(REQ_UNPLUG)
		flags = REQ_UNPLUG;
#else
		flags = 0;
#endif
		break;

	case ZIO_TYPE_READ:
		rw = READ;
#if defined(HAVE_BLK_QUEUE_HAVE_BIO_RW_UNPLUG)
		flags = (1 << BIO_RW_UNPLUG);
#elif defined(REQ_UNPLUG)
		flags = REQ_UNPLUG;
#else
		flags = 0;
#endif
		break;

	default:
		zio->io_error = SET_ERROR(ENOTSUP);
		zio_interrupt(zio);
		return;
	}

	error = __vdev_disk_physio(vd->vd_bdev, zio, zio->io_data,
	    zio->io_size, zio->io_offset, rw, flags);
	if (error) {
		zio->io_error = error;
		zio_interrupt(zio);
		return;
	}
}

static void
vdev_disk_io_done(zio_t *zio)
{
	/*
	 * If the device returned EIO, we revalidate the media.  If it is
	 * determined the media has changed this triggers the asynchronous
	 * removal of the device from the configuration.
	 */
	if (zio->io_error == EIO) {
		vdev_t *v = zio->io_vd;
		vdev_disk_t *vd = v->vdev_tsd;

		if (check_disk_change(vd->vd_bdev)) {
			vdev_bdev_invalidate(vd->vd_bdev);
			v->vdev_remove_wanted = B_TRUE;
			spa_async_request(zio->io_spa, SPA_ASYNC_REMOVE);
		}
	}
}

static void
vdev_disk_hold(vdev_t *vd)
{
	ASSERT(spa_config_held(vd->vdev_spa, SCL_STATE, RW_WRITER));

	/* We must have a pathname, and it must be absolute. */
	if (vd->vdev_path == NULL || vd->vdev_path[0] != '/')
		return;

	/*
	 * Only prefetch path and devid info if the device has
	 * never been opened.
	 */
	if (vd->vdev_tsd != NULL)
		return;

	/* XXX: Implement me as a vnode lookup for the device */
	vd->vdev_name_vp = NULL;
	vd->vdev_devid_vp = NULL;
}

static void
vdev_disk_rele(vdev_t *vd)
{
	ASSERT(spa_config_held(vd->vdev_spa, SCL_STATE, RW_WRITER));

	/* XXX: Implement me as a vnode rele for the device */
}

vdev_ops_t vdev_disk_ops = {
	vdev_disk_open,
	vdev_disk_close,
	vdev_default_asize,
	vdev_disk_io_start,
	vdev_disk_io_done,
	NULL,
	vdev_disk_hold,
	vdev_disk_rele,
	VDEV_TYPE_DISK,		/* name of this vdev type */
	B_TRUE			/* leaf vdev */
};

#include <linux/notifier.h>

static int vdev_disk_event(struct notifier_block *nb, unsigned long val,
			      void *args)
{
	unsigned long evt_type = val;
    u64	sas_address = *(u64 *)args;
    vdev_t *vd= NULL;

    vd = find_vdev_by_sas_addr(sas_address);
    if(vd == NULL) {
        zfs_dbgmsg( "event:%lu for sas address:0x%llx vdev not found[%s:%d] \n",
                    evt_type, sas_address, __FILE__, __LINE__);
        return NOTIFY_DONE;     /* Don't care */
    }

    zfs_dbgmsg( "event:%lu for sas address:0x%llx vdev found for vdev:%s[%s:%d]\n",
                evt_type, sas_address, vd->vdev_path, __FILE__, __LINE__);

	switch(val) {
    case SAS_EVT_DEV_REMOVE:
        zfs_ereport_post(FM_EREPORT_ZFS_DEV_REMOVED, vd->vdev_spa, vd, NULL, 0, 0);
        zfs_post_remove(vd->vdev_spa, vd);
        vd->vdev_remove_wanted = B_TRUE;
        spa_async_request(vd->vdev_spa, SPA_ASYNC_REMOVE);
        break;

    case SAS_EVT_DEV_MERR:
        zfs_ereport_post(FM_EREPORT_ZFS_DEVICE_MERR, vd->vdev_spa, vd, NULL, 0, 0);
        break;

    case SAS_EVT_DEV_NORESP:
        zfs_ereport_post(FM_EREPORT_ZFS_DEVICE_NORESP, vd->vdev_spa, vd, NULL, 0, 0);
        break;

    case SAS_EVT_DEV_SMART_FAIL:
        zfs_ereport_post(FM_EREPORT_ZFS_DEVICE_SMART_FAIL, vd->vdev_spa, vd, NULL, 0, 0);
        break;

    default :
        break;
    }

	return NOTIFY_OK;       /* Suits me */
}

struct notifier_block vdev_disk_notifier = {
	.notifier_call = vdev_disk_event,
};


module_param(zfs_vdev_scheduler, charp, 0644);
MODULE_PARM_DESC(zfs_vdev_scheduler, "I/O scheduler");
