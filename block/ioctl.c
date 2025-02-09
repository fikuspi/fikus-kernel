#include <fikus/capability.h>
#include <fikus/blkdev.h>
#include <fikus/export.h>
#include <fikus/gfp.h>
#include <fikus/blkpg.h>
#include <fikus/hdreg.h>
#include <fikus/backing-dev.h>
#include <fikus/fs.h>
#include <fikus/blktrace_api.h>
#include <asm/uaccess.h>

static int blkpg_ioctl(struct block_device *bdev, struct blkpg_ioctl_arg __user *arg)
{
	struct block_device *bdevp;
	struct gendisk *disk;
	struct hd_struct *part, *lpart;
	struct blkpg_ioctl_arg a;
	struct blkpg_partition p;
	struct disk_part_iter piter;
	long long start, length;
	int partno;

	if (!capable(CAP_SYS_ADMIN))
		return -EACCES;
	if (copy_from_user(&a, arg, sizeof(struct blkpg_ioctl_arg)))
		return -EFAULT;
	if (copy_from_user(&p, a.data, sizeof(struct blkpg_partition)))
		return -EFAULT;
	disk = bdev->bd_disk;
	if (bdev != bdev->bd_contains)
		return -EINVAL;
	partno = p.pno;
	if (partno <= 0)
		return -EINVAL;
	switch (a.op) {
		case BLKPG_ADD_PARTITION:
			start = p.start >> 9;
			length = p.length >> 9;
			/* check for fit in a hd_struct */
			if (sizeof(sector_t) == sizeof(long) &&
			    sizeof(long long) > sizeof(long)) {
				long pstart = start, plength = length;
				if (pstart != start || plength != length
				    || pstart < 0 || plength < 0 || partno > 65535)
					return -EINVAL;
			}

			mutex_lock(&bdev->bd_mutex);

			/* overlap? */
			disk_part_iter_init(&piter, disk,
					    DISK_PITER_INCL_EMPTY);
			while ((part = disk_part_iter_next(&piter))) {
				if (!(start + length <= part->start_sect ||
				      start >= part->start_sect + part->nr_sects)) {
					disk_part_iter_exit(&piter);
					mutex_unlock(&bdev->bd_mutex);
					return -EBUSY;
				}
			}
			disk_part_iter_exit(&piter);

			/* all seems OK */
			part = add_partition(disk, partno, start, length,
					     ADDPART_FLAG_NONE, NULL);
			mutex_unlock(&bdev->bd_mutex);
			return IS_ERR(part) ? PTR_ERR(part) : 0;
		case BLKPG_DEL_PARTITION:
			part = disk_get_part(disk, partno);
			if (!part)
				return -ENXIO;

			bdevp = bdget(part_devt(part));
			disk_put_part(part);
			if (!bdevp)
				return -ENOMEM;

			mutex_lock(&bdevp->bd_mutex);
			if (bdevp->bd_openers) {
				mutex_unlock(&bdevp->bd_mutex);
				bdput(bdevp);
				return -EBUSY;
			}
			/* all seems OK */
			fsync_bdev(bdevp);
			invalidate_bdev(bdevp);

			mutex_lock_nested(&bdev->bd_mutex, 1);
			delete_partition(disk, partno);
			mutex_unlock(&bdev->bd_mutex);
			mutex_unlock(&bdevp->bd_mutex);
			bdput(bdevp);

			return 0;
		case BLKPG_RESIZE_PARTITION:
			start = p.start >> 9;
			/* new length of partition in bytes */
			length = p.length >> 9;
			/* check for fit in a hd_struct */
			if (sizeof(sector_t) == sizeof(long) &&
			    sizeof(long long) > sizeof(long)) {
				long pstart = start, plength = length;
				if (pstart != start || plength != length
				    || pstart < 0 || plength < 0)
					return -EINVAL;
			}
			part = disk_get_part(disk, partno);
			if (!part)
				return -ENXIO;
			bdevp = bdget(part_devt(part));
			if (!bdevp) {
				disk_put_part(part);
				return -ENOMEM;
			}
			mutex_lock(&bdevp->bd_mutex);
			mutex_lock_nested(&bdev->bd_mutex, 1);
			if (start != part->start_sect) {
				mutex_unlock(&bdevp->bd_mutex);
				mutex_unlock(&bdev->bd_mutex);
				bdput(bdevp);
				disk_put_part(part);
				return -EINVAL;
			}
			/* overlap? */
			disk_part_iter_init(&piter, disk,
					    DISK_PITER_INCL_EMPTY);
			while ((lpart = disk_part_iter_next(&piter))) {
				if (lpart->partno != partno &&
				   !(start + length <= lpart->start_sect ||
				   start >= lpart->start_sect + lpart->nr_sects)
				   ) {
					disk_part_iter_exit(&piter);
					mutex_unlock(&bdevp->bd_mutex);
					mutex_unlock(&bdev->bd_mutex);
					bdput(bdevp);
					disk_put_part(part);
					return -EBUSY;
				}
			}
			disk_part_iter_exit(&piter);
			part_nr_sects_write(part, (sector_t)length);
			i_size_write(bdevp->bd_inode, p.length);
			mutex_unlock(&bdevp->bd_mutex);
			mutex_unlock(&bdev->bd_mutex);
			bdput(bdevp);
			disk_put_part(part);
			return 0;
		default:
			return -EINVAL;
	}
}

static int blkdev_reread_part(struct block_device *bdev)
{
	struct gendisk *disk = bdev->bd_disk;
	int res;

	if (!disk_part_scan_enabled(disk) || bdev != bdev->bd_contains)
		return -EINVAL;
	if (!capable(CAP_SYS_ADMIN))
		return -EACCES;
	if (!mutex_trylock(&bdev->bd_mutex))
		return -EBUSY;
	res = rescan_partitions(disk, bdev);
	mutex_unlock(&bdev->bd_mutex);
	return res;
}

static int blk_ioctl_discard(struct block_device *bdev, uint64_t start,
			     uint64_t len, int secure)
{
	unsigned long flags = 0;

	if (start & 511)
		return -EINVAL;
	if (len & 511)
		return -EINVAL;
	start >>= 9;
	len >>= 9;

	if (start + len > (i_size_read(bdev->bd_inode) >> 9))
		return -EINVAL;
	if (secure)
		flags |= BLKDEV_DISCARD_SECURE;
	return blkdev_issue_discard(bdev, start, len, GFP_KERNEL, flags);
}

static int blk_ioctl_zeroout(struct block_device *bdev, uint64_t start,
			     uint64_t len)
{
	if (start & 511)
		return -EINVAL;
	if (len & 511)
		return -EINVAL;
	start >>= 9;
	len >>= 9;

	if (start + len > (i_size_read(bdev->bd_inode) >> 9))
		return -EINVAL;

	return blkdev_issue_zeroout(bdev, start, len, GFP_KERNEL);
}

static int put_ushort(unsigned long arg, unsigned short val)
{
	return put_user(val, (unsigned short __user *)arg);
}

static int put_int(unsigned long arg, int val)
{
	return put_user(val, (int __user *)arg);
}

static int put_uint(unsigned long arg, unsigned int val)
{
	return put_user(val, (unsigned int __user *)arg);
}

static int put_long(unsigned long arg, long val)
{
	return put_user(val, (long __user *)arg);
}

static int put_ulong(unsigned long arg, unsigned long val)
{
	return put_user(val, (unsigned long __user *)arg);
}

static int put_u64(unsigned long arg, u64 val)
{
	return put_user(val, (u64 __user *)arg);
}

int __blkdev_driver_ioctl(struct block_device *bdev, fmode_t mode,
			unsigned cmd, unsigned long arg)
{
	struct gendisk *disk = bdev->bd_disk;

	if (disk->fops->ioctl)
		return disk->fops->ioctl(bdev, mode, cmd, arg);

	return -ENOTTY;
}
/*
 * For the record: _GPL here is only because somebody decided to slap it
 * on the previous export.  Sheer idiocy, since it wasn't copyrightable
 * at all and could be open-coded without any exports by anybody who cares.
 */
EXPORT_SYMBOL_GPL(__blkdev_driver_ioctl);

/*
 * Is it an unrecognized ioctl? The correct returns are either
 * ENOTTY (final) or ENOIOCTLCMD ("I don't know this one, try a
 * fallback"). ENOIOCTLCMD gets turned into ENOTTY by the ioctl
 * code before returning.
 *
 * Confused drivers sometimes return EINVAL, which is wrong. It
 * means "I understood the ioctl command, but the parameters to
 * it were wrong".
 *
 * We should aim to just fix the broken drivers, the EINVAL case
 * should go away.
 */
static inline int is_unrecognized_ioctl(int ret)
{
	return	ret == -EINVAL ||
		ret == -ENOTTY ||
		ret == -ENOIOCTLCMD;
}

/*
 * always keep this in sync with compat_blkdev_ioctl()
 */
int blkdev_ioctl(struct block_device *bdev, fmode_t mode, unsigned cmd,
			unsigned long arg)
{
	struct gendisk *disk = bdev->bd_disk;
	struct backing_dev_info *bdi;
	loff_t size;
	int ret, n;

	switch(cmd) {
	case BLKFLSBUF:
		if (!capable(CAP_SYS_ADMIN))
			return -EACCES;

		ret = __blkdev_driver_ioctl(bdev, mode, cmd, arg);
		if (!is_unrecognized_ioctl(ret))
			return ret;

		fsync_bdev(bdev);
		invalidate_bdev(bdev);
		return 0;

	case BLKROSET:
		ret = __blkdev_driver_ioctl(bdev, mode, cmd, arg);
		if (!is_unrecognized_ioctl(ret))
			return ret;
		if (!capable(CAP_SYS_ADMIN))
			return -EACCES;
		if (get_user(n, (int __user *)(arg)))
			return -EFAULT;
		set_device_ro(bdev, n);
		return 0;

	case BLKDISCARD:
	case BLKSECDISCARD: {
		uint64_t range[2];

		if (!(mode & FMODE_WRITE))
			return -EBADF;

		if (copy_from_user(range, (void __user *)arg, sizeof(range)))
			return -EFAULT;

		return blk_ioctl_discard(bdev, range[0], range[1],
					 cmd == BLKSECDISCARD);
	}
	case BLKZEROOUT: {
		uint64_t range[2];

		if (!(mode & FMODE_WRITE))
			return -EBADF;

		if (copy_from_user(range, (void __user *)arg, sizeof(range)))
			return -EFAULT;

		return blk_ioctl_zeroout(bdev, range[0], range[1]);
	}

	case HDIO_GETGEO: {
		struct hd_geometry geo;

		if (!arg)
			return -EINVAL;
		if (!disk->fops->getgeo)
			return -ENOTTY;

		/*
		 * We need to set the startsect first, the driver may
		 * want to override it.
		 */
		memset(&geo, 0, sizeof(geo));
		geo.start = get_start_sect(bdev);
		ret = disk->fops->getgeo(bdev, &geo);
		if (ret)
			return ret;
		if (copy_to_user((struct hd_geometry __user *)arg, &geo,
					sizeof(geo)))
			return -EFAULT;
		return 0;
	}
	case BLKRAGET:
	case BLKFRAGET:
		if (!arg)
			return -EINVAL;
		bdi = blk_get_backing_dev_info(bdev);
		if (bdi == NULL)
			return -ENOTTY;
		return put_long(arg, (bdi->ra_pages * PAGE_CACHE_SIZE) / 512);
	case BLKROGET:
		return put_int(arg, bdev_read_only(bdev) != 0);
	case BLKBSZGET: /* get block device soft block size (cf. BLKSSZGET) */
		return put_int(arg, block_size(bdev));
	case BLKSSZGET: /* get block device logical block size */
		return put_int(arg, bdev_logical_block_size(bdev));
	case BLKPBSZGET: /* get block device physical block size */
		return put_uint(arg, bdev_physical_block_size(bdev));
	case BLKIOMIN:
		return put_uint(arg, bdev_io_min(bdev));
	case BLKIOOPT:
		return put_uint(arg, bdev_io_opt(bdev));
	case BLKALIGNOFF:
		return put_int(arg, bdev_alignment_offset(bdev));
	case BLKDISCARDZEROES:
		return put_uint(arg, bdev_discard_zeroes_data(bdev));
	case BLKSECTGET:
		return put_ushort(arg, queue_max_sectors(bdev_get_queue(bdev)));
	case BLKROTATIONAL:
		return put_ushort(arg, !blk_queue_nonrot(bdev_get_queue(bdev)));
	case BLKRASET:
	case BLKFRASET:
		if(!capable(CAP_SYS_ADMIN))
			return -EACCES;
		bdi = blk_get_backing_dev_info(bdev);
		if (bdi == NULL)
			return -ENOTTY;
		bdi->ra_pages = (arg * 512) / PAGE_CACHE_SIZE;
		return 0;
	case BLKBSZSET:
		/* set the logical block size */
		if (!capable(CAP_SYS_ADMIN))
			return -EACCES;
		if (!arg)
			return -EINVAL;
		if (get_user(n, (int __user *) arg))
			return -EFAULT;
		if (!(mode & FMODE_EXCL)) {
			bdgrab(bdev);
			if (blkdev_get(bdev, mode | FMODE_EXCL, &bdev) < 0)
				return -EBUSY;
		}
		ret = set_blocksize(bdev, n);
		if (!(mode & FMODE_EXCL))
			blkdev_put(bdev, mode | FMODE_EXCL);
		return ret;
	case BLKPG:
		ret = blkpg_ioctl(bdev, (struct blkpg_ioctl_arg __user *) arg);
		break;
	case BLKRRPART:
		ret = blkdev_reread_part(bdev);
		break;
	case BLKGETSIZE:
		size = i_size_read(bdev->bd_inode);
		if ((size >> 9) > ~0UL)
			return -EFBIG;
		return put_ulong(arg, size >> 9);
	case BLKGETSIZE64:
		return put_u64(arg, i_size_read(bdev->bd_inode));
	case BLKTRACESTART:
	case BLKTRACESTOP:
	case BLKTRACESETUP:
	case BLKTRACETEARDOWN:
		ret = blk_trace_ioctl(bdev, cmd, (char __user *) arg);
		break;
	default:
		ret = __blkdev_driver_ioctl(bdev, mode, cmd, arg);
	}
	return ret;
}
EXPORT_SYMBOL_GPL(blkdev_ioctl);
