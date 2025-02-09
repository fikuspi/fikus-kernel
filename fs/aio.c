/*
 *	An async IO implementation for Fikus
 *	Written by Benjamin LaHaise <bcrl@kvack.org>
 *
 *	Implements an efficient asynchronous io interface.
 *
 *	Copyright 2000, 2001, 2002 Red Hat, Inc.  All Rights Reserved.
 *
 *	See ../COPYING for licensing terms.
 */
#define pr_fmt(fmt) "%s: " fmt, __func__

#include <fikus/kernel.h>
#include <fikus/init.h>
#include <fikus/errno.h>
#include <fikus/time.h>
#include <fikus/aio_abi.h>
#include <fikus/export.h>
#include <fikus/syscalls.h>
#include <fikus/backing-dev.h>
#include <fikus/uio.h>

#include <fikus/sched.h>
#include <fikus/fs.h>
#include <fikus/file.h>
#include <fikus/mm.h>
#include <fikus/mman.h>
#include <fikus/mmu_context.h>
#include <fikus/percpu.h>
#include <fikus/slab.h>
#include <fikus/timer.h>
#include <fikus/aio.h>
#include <fikus/highmem.h>
#include <fikus/workqueue.h>
#include <fikus/security.h>
#include <fikus/eventfd.h>
#include <fikus/blkdev.h>
#include <fikus/compat.h>
#include <fikus/migrate.h>
#include <fikus/ramfs.h>
#include <fikus/percpu-refcount.h>
#include <fikus/mount.h>

#include <asm/kmap_types.h>
#include <asm/uaccess.h>

#include "internal.h"

#define AIO_RING_MAGIC			0xa10a10a1
#define AIO_RING_COMPAT_FEATURES	1
#define AIO_RING_INCOMPAT_FEATURES	0
struct aio_ring {
	unsigned	id;	/* kernel internal index number */
	unsigned	nr;	/* number of io_events */
	unsigned	head;
	unsigned	tail;

	unsigned	magic;
	unsigned	compat_features;
	unsigned	incompat_features;
	unsigned	header_length;	/* size of aio_ring */


	struct io_event		io_events[0];
}; /* 128 bytes + ring size */

#define AIO_RING_PAGES	8

struct kioctx_table {
	struct rcu_head	rcu;
	unsigned	nr;
	struct kioctx	*table[];
};

struct kioctx_cpu {
	unsigned		reqs_available;
};

struct kioctx {
	struct percpu_ref	users;
	atomic_t		dead;

	struct percpu_ref	reqs;

	unsigned long		user_id;

	struct __percpu kioctx_cpu *cpu;

	/*
	 * For percpu reqs_available, number of slots we move to/from global
	 * counter at a time:
	 */
	unsigned		req_batch;
	/*
	 * This is what userspace passed to io_setup(), it's not used for
	 * anything but counting against the global max_reqs quota.
	 *
	 * The real limit is nr_events - 1, which will be larger (see
	 * aio_setup_ring())
	 */
	unsigned		max_reqs;

	/* Size of ringbuffer, in units of struct io_event */
	unsigned		nr_events;

	unsigned long		mmap_base;
	unsigned long		mmap_size;

	struct page		**ring_pages;
	long			nr_pages;

	struct work_struct	free_work;

	struct {
		/*
		 * This counts the number of available slots in the ringbuffer,
		 * so we avoid overflowing it: it's decremented (if positive)
		 * when allocating a kiocb and incremented when the resulting
		 * io_event is pulled off the ringbuffer.
		 *
		 * We batch accesses to it with a percpu version.
		 */
		atomic_t	reqs_available;
	} ____cacheline_aligned_in_smp;

	struct {
		spinlock_t	ctx_lock;
		struct list_head active_reqs;	/* used for cancellation */
	} ____cacheline_aligned_in_smp;

	struct {
		struct mutex	ring_lock;
		wait_queue_head_t wait;
	} ____cacheline_aligned_in_smp;

	struct {
		unsigned	tail;
		spinlock_t	completion_lock;
	} ____cacheline_aligned_in_smp;

	struct page		*internal_pages[AIO_RING_PAGES];
	struct file		*aio_ring_file;

	unsigned		id;
};

/*------ sysctl variables----*/
static DEFINE_SPINLOCK(aio_nr_lock);
unsigned long aio_nr;		/* current system wide number of aio requests */
unsigned long aio_max_nr = 0x10000; /* system wide maximum number of aio requests */
/*----end sysctl variables---*/

static struct kmem_cache	*kiocb_cachep;
static struct kmem_cache	*kioctx_cachep;

static struct vfsmount *aio_mnt;

static const struct file_operations aio_ring_fops;
static const struct address_space_operations aio_ctx_aops;

static struct file *aio_private_file(struct kioctx *ctx, loff_t nr_pages)
{
	struct qstr this = QSTR_INIT("[aio]", 5);
	struct file *file;
	struct path path;
	struct inode *inode = alloc_anon_inode(aio_mnt->mnt_sb);
	if (IS_ERR(inode))
		return ERR_CAST(inode);

	inode->i_mapping->a_ops = &aio_ctx_aops;
	inode->i_mapping->private_data = ctx;
	inode->i_size = PAGE_SIZE * nr_pages;

	path.dentry = d_alloc_pseudo(aio_mnt->mnt_sb, &this);
	if (!path.dentry) {
		iput(inode);
		return ERR_PTR(-ENOMEM);
	}
	path.mnt = mntget(aio_mnt);

	d_instantiate(path.dentry, inode);
	file = alloc_file(&path, FMODE_READ | FMODE_WRITE, &aio_ring_fops);
	if (IS_ERR(file)) {
		path_put(&path);
		return file;
	}

	file->f_flags = O_RDWR;
	file->private_data = ctx;
	return file;
}

static struct dentry *aio_mount(struct file_system_type *fs_type,
				int flags, const char *dev_name, void *data)
{
	static const struct dentry_operations ops = {
		.d_dname	= simple_dname,
	};
	return mount_pseudo(fs_type, "aio:", NULL, &ops, 0xa10a10a1);
}

/* aio_setup
 *	Creates the slab caches used by the aio routines, panic on
 *	failure as this is done early during the boot sequence.
 */
static int __init aio_setup(void)
{
	static struct file_system_type aio_fs = {
		.name		= "aio",
		.mount		= aio_mount,
		.kill_sb	= kill_anon_super,
	};
	aio_mnt = kern_mount(&aio_fs);
	if (IS_ERR(aio_mnt))
		panic("Failed to create aio fs mount.");

	kiocb_cachep = KMEM_CACHE(kiocb, SLAB_HWCACHE_ALIGN|SLAB_PANIC);
	kioctx_cachep = KMEM_CACHE(kioctx,SLAB_HWCACHE_ALIGN|SLAB_PANIC);

	pr_debug("sizeof(struct page) = %zu\n", sizeof(struct page));

	return 0;
}
__initcall(aio_setup);

static void put_aio_ring_file(struct kioctx *ctx)
{
	struct file *aio_ring_file = ctx->aio_ring_file;
	if (aio_ring_file) {
		truncate_setsize(aio_ring_file->f_inode, 0);

		/* Prevent further access to the kioctx from migratepages */
		spin_lock(&aio_ring_file->f_inode->i_mapping->private_lock);
		aio_ring_file->f_inode->i_mapping->private_data = NULL;
		ctx->aio_ring_file = NULL;
		spin_unlock(&aio_ring_file->f_inode->i_mapping->private_lock);

		fput(aio_ring_file);
	}
}

static void aio_free_ring(struct kioctx *ctx)
{
	int i;

	for (i = 0; i < ctx->nr_pages; i++) {
		struct page *page;
		pr_debug("pid(%d) [%d] page->count=%d\n", current->pid, i,
				page_count(ctx->ring_pages[i]));
		page = ctx->ring_pages[i];
		if (!page)
			continue;
		ctx->ring_pages[i] = NULL;
		put_page(page);
	}

	put_aio_ring_file(ctx);

	if (ctx->ring_pages && ctx->ring_pages != ctx->internal_pages) {
		kfree(ctx->ring_pages);
		ctx->ring_pages = NULL;
	}
}

static int aio_ring_mmap(struct file *file, struct vm_area_struct *vma)
{
	vma->vm_ops = &generic_file_vm_ops;
	return 0;
}

static const struct file_operations aio_ring_fops = {
	.mmap = aio_ring_mmap,
};

static int aio_set_page_dirty(struct page *page)
{
	return 0;
}

#if IS_ENABLED(CONFIG_MIGRATION)
static int aio_migratepage(struct address_space *mapping, struct page *new,
			struct page *old, enum migrate_mode mode)
{
	struct kioctx *ctx;
	unsigned long flags;
	int rc;

	rc = 0;

	/* Make sure the old page hasn't already been changed */
	spin_lock(&mapping->private_lock);
	ctx = mapping->private_data;
	if (ctx) {
		pgoff_t idx;
		spin_lock_irqsave(&ctx->completion_lock, flags);
		idx = old->index;
		if (idx < (pgoff_t)ctx->nr_pages) {
			if (ctx->ring_pages[idx] != old)
				rc = -EAGAIN;
		} else
			rc = -EINVAL;
		spin_unlock_irqrestore(&ctx->completion_lock, flags);
	} else
		rc = -EINVAL;
	spin_unlock(&mapping->private_lock);

	if (rc != 0)
		return rc;

	/* Writeback must be complete */
	BUG_ON(PageWriteback(old));
	get_page(new);

	rc = migrate_page_move_mapping(mapping, new, old, NULL, mode, 1);
	if (rc != MIGRATEPAGE_SUCCESS) {
		put_page(new);
		return rc;
	}

	/* We can potentially race against kioctx teardown here.  Use the
	 * address_space's private data lock to protect the mapping's
	 * private_data.
	 */
	spin_lock(&mapping->private_lock);
	ctx = mapping->private_data;
	if (ctx) {
		pgoff_t idx;
		spin_lock_irqsave(&ctx->completion_lock, flags);
		migrate_page_copy(new, old);
		idx = old->index;
		if (idx < (pgoff_t)ctx->nr_pages) {
			/* And only do the move if things haven't changed */
			if (ctx->ring_pages[idx] == old)
				ctx->ring_pages[idx] = new;
			else
				rc = -EAGAIN;
		} else
			rc = -EINVAL;
		spin_unlock_irqrestore(&ctx->completion_lock, flags);
	} else
		rc = -EBUSY;
	spin_unlock(&mapping->private_lock);

	if (rc == MIGRATEPAGE_SUCCESS)
		put_page(old);
	else
		put_page(new);

	return rc;
}
#endif

static const struct address_space_operations aio_ctx_aops = {
	.set_page_dirty = aio_set_page_dirty,
#if IS_ENABLED(CONFIG_MIGRATION)
	.migratepage	= aio_migratepage,
#endif
};

static int aio_setup_ring(struct kioctx *ctx)
{
	struct aio_ring *ring;
	unsigned nr_events = ctx->max_reqs;
	struct mm_struct *mm = current->mm;
	unsigned long size, unused;
	int nr_pages;
	int i;
	struct file *file;

	/* Compensate for the ring buffer's head/tail overlap entry */
	nr_events += 2;	/* 1 is required, 2 for good luck */

	size = sizeof(struct aio_ring);
	size += sizeof(struct io_event) * nr_events;

	nr_pages = PFN_UP(size);
	if (nr_pages < 0)
		return -EINVAL;

	file = aio_private_file(ctx, nr_pages);
	if (IS_ERR(file)) {
		ctx->aio_ring_file = NULL;
		return -EAGAIN;
	}

	ctx->aio_ring_file = file;
	nr_events = (PAGE_SIZE * nr_pages - sizeof(struct aio_ring))
			/ sizeof(struct io_event);

	ctx->ring_pages = ctx->internal_pages;
	if (nr_pages > AIO_RING_PAGES) {
		ctx->ring_pages = kcalloc(nr_pages, sizeof(struct page *),
					  GFP_KERNEL);
		if (!ctx->ring_pages) {
			put_aio_ring_file(ctx);
			return -ENOMEM;
		}
	}

	for (i = 0; i < nr_pages; i++) {
		struct page *page;
		page = find_or_create_page(file->f_inode->i_mapping,
					   i, GFP_HIGHUSER | __GFP_ZERO);
		if (!page)
			break;
		pr_debug("pid(%d) page[%d]->count=%d\n",
			 current->pid, i, page_count(page));
		SetPageUptodate(page);
		SetPageDirty(page);
		unlock_page(page);

		ctx->ring_pages[i] = page;
	}
	ctx->nr_pages = i;

	if (unlikely(i != nr_pages)) {
		aio_free_ring(ctx);
		return -EAGAIN;
	}

	ctx->mmap_size = nr_pages * PAGE_SIZE;
	pr_debug("attempting mmap of %lu bytes\n", ctx->mmap_size);

	down_write(&mm->mmap_sem);
	ctx->mmap_base = do_mmap_pgoff(ctx->aio_ring_file, 0, ctx->mmap_size,
				       PROT_READ | PROT_WRITE,
				       MAP_SHARED, 0, &unused);
	up_write(&mm->mmap_sem);
	if (IS_ERR((void *)ctx->mmap_base)) {
		ctx->mmap_size = 0;
		aio_free_ring(ctx);
		return -EAGAIN;
	}

	pr_debug("mmap address: 0x%08lx\n", ctx->mmap_base);

	ctx->user_id = ctx->mmap_base;
	ctx->nr_events = nr_events; /* trusted copy */

	ring = kmap_atomic(ctx->ring_pages[0]);
	ring->nr = nr_events;	/* user copy */
	ring->id = ~0U;
	ring->head = ring->tail = 0;
	ring->magic = AIO_RING_MAGIC;
	ring->compat_features = AIO_RING_COMPAT_FEATURES;
	ring->incompat_features = AIO_RING_INCOMPAT_FEATURES;
	ring->header_length = sizeof(struct aio_ring);
	kunmap_atomic(ring);
	flush_dcache_page(ctx->ring_pages[0]);

	return 0;
}

#define AIO_EVENTS_PER_PAGE	(PAGE_SIZE / sizeof(struct io_event))
#define AIO_EVENTS_FIRST_PAGE	((PAGE_SIZE - sizeof(struct aio_ring)) / sizeof(struct io_event))
#define AIO_EVENTS_OFFSET	(AIO_EVENTS_PER_PAGE - AIO_EVENTS_FIRST_PAGE)

void kiocb_set_cancel_fn(struct kiocb *req, kiocb_cancel_fn *cancel)
{
	struct kioctx *ctx = req->ki_ctx;
	unsigned long flags;

	spin_lock_irqsave(&ctx->ctx_lock, flags);

	if (!req->ki_list.next)
		list_add(&req->ki_list, &ctx->active_reqs);

	req->ki_cancel = cancel;

	spin_unlock_irqrestore(&ctx->ctx_lock, flags);
}
EXPORT_SYMBOL(kiocb_set_cancel_fn);

static int kiocb_cancel(struct kioctx *ctx, struct kiocb *kiocb)
{
	kiocb_cancel_fn *old, *cancel;

	/*
	 * Don't want to set kiocb->ki_cancel = KIOCB_CANCELLED unless it
	 * actually has a cancel function, hence the cmpxchg()
	 */

	cancel = ACCESS_ONCE(kiocb->ki_cancel);
	do {
		if (!cancel || cancel == KIOCB_CANCELLED)
			return -EINVAL;

		old = cancel;
		cancel = cmpxchg(&kiocb->ki_cancel, old, KIOCB_CANCELLED);
	} while (cancel != old);

	return cancel(kiocb);
}

static void free_ioctx(struct work_struct *work)
{
	struct kioctx *ctx = container_of(work, struct kioctx, free_work);

	pr_debug("freeing %p\n", ctx);

	aio_free_ring(ctx);
	free_percpu(ctx->cpu);
	kmem_cache_free(kioctx_cachep, ctx);
}

static void free_ioctx_reqs(struct percpu_ref *ref)
{
	struct kioctx *ctx = container_of(ref, struct kioctx, reqs);

	INIT_WORK(&ctx->free_work, free_ioctx);
	schedule_work(&ctx->free_work);
}

/*
 * When this function runs, the kioctx has been removed from the "hash table"
 * and ctx->users has dropped to 0, so we know no more kiocbs can be submitted -
 * now it's safe to cancel any that need to be.
 */
static void free_ioctx_users(struct percpu_ref *ref)
{
	struct kioctx *ctx = container_of(ref, struct kioctx, users);
	struct kiocb *req;

	spin_lock_irq(&ctx->ctx_lock);

	while (!list_empty(&ctx->active_reqs)) {
		req = list_first_entry(&ctx->active_reqs,
				       struct kiocb, ki_list);

		list_del_init(&req->ki_list);
		kiocb_cancel(ctx, req);
	}

	spin_unlock_irq(&ctx->ctx_lock);

	percpu_ref_kill(&ctx->reqs);
	percpu_ref_put(&ctx->reqs);
}

static int ioctx_add_table(struct kioctx *ctx, struct mm_struct *mm)
{
	unsigned i, new_nr;
	struct kioctx_table *table, *old;
	struct aio_ring *ring;

	spin_lock(&mm->ioctx_lock);
	rcu_read_lock();
	table = rcu_dereference(mm->ioctx_table);

	while (1) {
		if (table)
			for (i = 0; i < table->nr; i++)
				if (!table->table[i]) {
					ctx->id = i;
					table->table[i] = ctx;
					rcu_read_unlock();
					spin_unlock(&mm->ioctx_lock);

					ring = kmap_atomic(ctx->ring_pages[0]);
					ring->id = ctx->id;
					kunmap_atomic(ring);
					return 0;
				}

		new_nr = (table ? table->nr : 1) * 4;

		rcu_read_unlock();
		spin_unlock(&mm->ioctx_lock);

		table = kzalloc(sizeof(*table) + sizeof(struct kioctx *) *
				new_nr, GFP_KERNEL);
		if (!table)
			return -ENOMEM;

		table->nr = new_nr;

		spin_lock(&mm->ioctx_lock);
		rcu_read_lock();
		old = rcu_dereference(mm->ioctx_table);

		if (!old) {
			rcu_assign_pointer(mm->ioctx_table, table);
		} else if (table->nr > old->nr) {
			memcpy(table->table, old->table,
			       old->nr * sizeof(struct kioctx *));

			rcu_assign_pointer(mm->ioctx_table, table);
			kfree_rcu(old, rcu);
		} else {
			kfree(table);
			table = old;
		}
	}
}

static void aio_nr_sub(unsigned nr)
{
	spin_lock(&aio_nr_lock);
	if (WARN_ON(aio_nr - nr > aio_nr))
		aio_nr = 0;
	else
		aio_nr -= nr;
	spin_unlock(&aio_nr_lock);
}

/* ioctx_alloc
 *	Allocates and initializes an ioctx.  Returns an ERR_PTR if it failed.
 */
static struct kioctx *ioctx_alloc(unsigned nr_events)
{
	struct mm_struct *mm = current->mm;
	struct kioctx *ctx;
	int err = -ENOMEM;

	/*
	 * We keep track of the number of available ringbuffer slots, to prevent
	 * overflow (reqs_available), and we also use percpu counters for this.
	 *
	 * So since up to half the slots might be on other cpu's percpu counters
	 * and unavailable, double nr_events so userspace sees what they
	 * expected: additionally, we move req_batch slots to/from percpu
	 * counters at a time, so make sure that isn't 0:
	 */
	nr_events = max(nr_events, num_possible_cpus() * 4);
	nr_events *= 2;

	/* Prevent overflows */
	if ((nr_events > (0x10000000U / sizeof(struct io_event))) ||
	    (nr_events > (0x10000000U / sizeof(struct kiocb)))) {
		pr_debug("ENOMEM: nr_events too high\n");
		return ERR_PTR(-EINVAL);
	}

	if (!nr_events || (unsigned long)nr_events > (aio_max_nr * 2UL))
		return ERR_PTR(-EAGAIN);

	ctx = kmem_cache_zalloc(kioctx_cachep, GFP_KERNEL);
	if (!ctx)
		return ERR_PTR(-ENOMEM);

	ctx->max_reqs = nr_events;

	if (percpu_ref_init(&ctx->users, free_ioctx_users))
		goto err;

	if (percpu_ref_init(&ctx->reqs, free_ioctx_reqs))
		goto err;

	spin_lock_init(&ctx->ctx_lock);
	spin_lock_init(&ctx->completion_lock);
	mutex_init(&ctx->ring_lock);
	init_waitqueue_head(&ctx->wait);

	INIT_LIST_HEAD(&ctx->active_reqs);

	ctx->cpu = alloc_percpu(struct kioctx_cpu);
	if (!ctx->cpu)
		goto err;

	if (aio_setup_ring(ctx) < 0)
		goto err;

	atomic_set(&ctx->reqs_available, ctx->nr_events - 1);
	ctx->req_batch = (ctx->nr_events - 1) / (num_possible_cpus() * 4);
	if (ctx->req_batch < 1)
		ctx->req_batch = 1;

	/* limit the number of system wide aios */
	spin_lock(&aio_nr_lock);
	if (aio_nr + nr_events > (aio_max_nr * 2UL) ||
	    aio_nr + nr_events < aio_nr) {
		spin_unlock(&aio_nr_lock);
		err = -EAGAIN;
		goto err_ctx;
	}
	aio_nr += ctx->max_reqs;
	spin_unlock(&aio_nr_lock);

	percpu_ref_get(&ctx->users);	/* io_setup() will drop this ref */
	percpu_ref_get(&ctx->reqs);	/* free_ioctx_users() will drop this */

	err = ioctx_add_table(ctx, mm);
	if (err)
		goto err_cleanup;

	pr_debug("allocated ioctx %p[%ld]: mm=%p mask=0x%x\n",
		 ctx, ctx->user_id, mm, ctx->nr_events);
	return ctx;

err_cleanup:
	aio_nr_sub(ctx->max_reqs);
err_ctx:
	aio_free_ring(ctx);
err:
	free_percpu(ctx->cpu);
	free_percpu(ctx->reqs.pcpu_count);
	free_percpu(ctx->users.pcpu_count);
	kmem_cache_free(kioctx_cachep, ctx);
	pr_debug("error allocating ioctx %d\n", err);
	return ERR_PTR(err);
}

/* kill_ioctx
 *	Cancels all outstanding aio requests on an aio context.  Used
 *	when the processes owning a context have all exited to encourage
 *	the rapid destruction of the kioctx.
 */
static void kill_ioctx(struct mm_struct *mm, struct kioctx *ctx)
{
	if (!atomic_xchg(&ctx->dead, 1)) {
		struct kioctx_table *table;

		spin_lock(&mm->ioctx_lock);
		rcu_read_lock();
		table = rcu_dereference(mm->ioctx_table);

		WARN_ON(ctx != table->table[ctx->id]);
		table->table[ctx->id] = NULL;
		rcu_read_unlock();
		spin_unlock(&mm->ioctx_lock);

		/* percpu_ref_kill() will do the necessary call_rcu() */
		wake_up_all(&ctx->wait);

		/*
		 * It'd be more correct to do this in free_ioctx(), after all
		 * the outstanding kiocbs have finished - but by then io_destroy
		 * has already returned, so io_setup() could potentially return
		 * -EAGAIN with no ioctxs actually in use (as far as userspace
		 *  could tell).
		 */
		aio_nr_sub(ctx->max_reqs);

		if (ctx->mmap_size)
			vm_munmap(ctx->mmap_base, ctx->mmap_size);

		percpu_ref_kill(&ctx->users);
	}
}

/* wait_on_sync_kiocb:
 *	Waits on the given sync kiocb to complete.
 */
ssize_t wait_on_sync_kiocb(struct kiocb *req)
{
	while (!req->ki_ctx) {
		set_current_state(TASK_UNINTERRUPTIBLE);
		if (req->ki_ctx)
			break;
		io_schedule();
	}
	__set_current_state(TASK_RUNNING);
	return req->ki_user_data;
}
EXPORT_SYMBOL(wait_on_sync_kiocb);

/*
 * exit_aio: called when the last user of mm goes away.  At this point, there is
 * no way for any new requests to be submited or any of the io_* syscalls to be
 * called on the context.
 *
 * There may be outstanding kiocbs, but free_ioctx() will explicitly wait on
 * them.
 */
void exit_aio(struct mm_struct *mm)
{
	struct kioctx_table *table;
	struct kioctx *ctx;
	unsigned i = 0;

	while (1) {
		rcu_read_lock();
		table = rcu_dereference(mm->ioctx_table);

		do {
			if (!table || i >= table->nr) {
				rcu_read_unlock();
				rcu_assign_pointer(mm->ioctx_table, NULL);
				if (table)
					kfree(table);
				return;
			}

			ctx = table->table[i++];
		} while (!ctx);

		rcu_read_unlock();

		/*
		 * We don't need to bother with munmap() here -
		 * exit_mmap(mm) is coming and it'll unmap everything.
		 * Since aio_free_ring() uses non-zero ->mmap_size
		 * as indicator that it needs to unmap the area,
		 * just set it to 0; aio_free_ring() is the only
		 * place that uses ->mmap_size, so it's safe.
		 */
		ctx->mmap_size = 0;

		kill_ioctx(mm, ctx);
	}
}

static void put_reqs_available(struct kioctx *ctx, unsigned nr)
{
	struct kioctx_cpu *kcpu;

	preempt_disable();
	kcpu = this_cpu_ptr(ctx->cpu);

	kcpu->reqs_available += nr;
	while (kcpu->reqs_available >= ctx->req_batch * 2) {
		kcpu->reqs_available -= ctx->req_batch;
		atomic_add(ctx->req_batch, &ctx->reqs_available);
	}

	preempt_enable();
}

static bool get_reqs_available(struct kioctx *ctx)
{
	struct kioctx_cpu *kcpu;
	bool ret = false;

	preempt_disable();
	kcpu = this_cpu_ptr(ctx->cpu);

	if (!kcpu->reqs_available) {
		int old, avail = atomic_read(&ctx->reqs_available);

		do {
			if (avail < ctx->req_batch)
				goto out;

			old = avail;
			avail = atomic_cmpxchg(&ctx->reqs_available,
					       avail, avail - ctx->req_batch);
		} while (avail != old);

		kcpu->reqs_available += ctx->req_batch;
	}

	ret = true;
	kcpu->reqs_available--;
out:
	preempt_enable();
	return ret;
}

/* aio_get_req
 *	Allocate a slot for an aio request.
 * Returns NULL if no requests are free.
 */
static inline struct kiocb *aio_get_req(struct kioctx *ctx)
{
	struct kiocb *req;

	if (!get_reqs_available(ctx))
		return NULL;

	req = kmem_cache_alloc(kiocb_cachep, GFP_KERNEL|__GFP_ZERO);
	if (unlikely(!req))
		goto out_put;

	percpu_ref_get(&ctx->reqs);

	req->ki_ctx = ctx;
	return req;
out_put:
	put_reqs_available(ctx, 1);
	return NULL;
}

static void kiocb_free(struct kiocb *req)
{
	if (req->ki_filp)
		fput(req->ki_filp);
	if (req->ki_eventfd != NULL)
		eventfd_ctx_put(req->ki_eventfd);
	kmem_cache_free(kiocb_cachep, req);
}

static struct kioctx *lookup_ioctx(unsigned long ctx_id)
{
	struct aio_ring __user *ring  = (void __user *)ctx_id;
	struct mm_struct *mm = current->mm;
	struct kioctx *ctx, *ret = NULL;
	struct kioctx_table *table;
	unsigned id;

	if (get_user(id, &ring->id))
		return NULL;

	rcu_read_lock();
	table = rcu_dereference(mm->ioctx_table);

	if (!table || id >= table->nr)
		goto out;

	ctx = table->table[id];
	if (ctx && ctx->user_id == ctx_id) {
		percpu_ref_get(&ctx->users);
		ret = ctx;
	}
out:
	rcu_read_unlock();
	return ret;
}

/* aio_complete
 *	Called when the io request on the given iocb is complete.
 */
void aio_complete(struct kiocb *iocb, long res, long res2)
{
	struct kioctx	*ctx = iocb->ki_ctx;
	struct aio_ring	*ring;
	struct io_event	*ev_page, *event;
	unsigned long	flags;
	unsigned tail, pos;

	/*
	 * Special case handling for sync iocbs:
	 *  - events go directly into the iocb for fast handling
	 *  - the sync task with the iocb in its stack holds the single iocb
	 *    ref, no other paths have a way to get another ref
	 *  - the sync task helpfully left a reference to itself in the iocb
	 */
	if (is_sync_kiocb(iocb)) {
		iocb->ki_user_data = res;
		smp_wmb();
		iocb->ki_ctx = ERR_PTR(-EXDEV);
		wake_up_process(iocb->ki_obj.tsk);
		return;
	}

	if (iocb->ki_list.next) {
		unsigned long flags;

		spin_lock_irqsave(&ctx->ctx_lock, flags);
		list_del(&iocb->ki_list);
		spin_unlock_irqrestore(&ctx->ctx_lock, flags);
	}

	/*
	 * Add a completion event to the ring buffer. Must be done holding
	 * ctx->completion_lock to prevent other code from messing with the tail
	 * pointer since we might be called from irq context.
	 */
	spin_lock_irqsave(&ctx->completion_lock, flags);

	tail = ctx->tail;
	pos = tail + AIO_EVENTS_OFFSET;

	if (++tail >= ctx->nr_events)
		tail = 0;

	ev_page = kmap_atomic(ctx->ring_pages[pos / AIO_EVENTS_PER_PAGE]);
	event = ev_page + pos % AIO_EVENTS_PER_PAGE;

	event->obj = (u64)(unsigned long)iocb->ki_obj.user;
	event->data = iocb->ki_user_data;
	event->res = res;
	event->res2 = res2;

	kunmap_atomic(ev_page);
	flush_dcache_page(ctx->ring_pages[pos / AIO_EVENTS_PER_PAGE]);

	pr_debug("%p[%u]: %p: %p %Lx %lx %lx\n",
		 ctx, tail, iocb, iocb->ki_obj.user, iocb->ki_user_data,
		 res, res2);

	/* after flagging the request as done, we
	 * must never even look at it again
	 */
	smp_wmb();	/* make event visible before updating tail */

	ctx->tail = tail;

	ring = kmap_atomic(ctx->ring_pages[0]);
	ring->tail = tail;
	kunmap_atomic(ring);
	flush_dcache_page(ctx->ring_pages[0]);

	spin_unlock_irqrestore(&ctx->completion_lock, flags);

	pr_debug("added to ring %p at [%u]\n", iocb, tail);

	/*
	 * Check if the user asked us to deliver the result through an
	 * eventfd. The eventfd_signal() function is safe to be called
	 * from IRQ context.
	 */
	if (iocb->ki_eventfd != NULL)
		eventfd_signal(iocb->ki_eventfd, 1);

	/* everything turned out well, dispose of the aiocb. */
	kiocb_free(iocb);

	/*
	 * We have to order our ring_info tail store above and test
	 * of the wait list below outside the wait lock.  This is
	 * like in wake_up_bit() where clearing a bit has to be
	 * ordered with the unlocked test.
	 */
	smp_mb();

	if (waitqueue_active(&ctx->wait))
		wake_up(&ctx->wait);

	percpu_ref_put(&ctx->reqs);
}
EXPORT_SYMBOL(aio_complete);

/* aio_read_events
 *	Pull an event off of the ioctx's event ring.  Returns the number of
 *	events fetched
 */
static long aio_read_events_ring(struct kioctx *ctx,
				 struct io_event __user *event, long nr)
{
	struct aio_ring *ring;
	unsigned head, tail, pos;
	long ret = 0;
	int copy_ret;

	mutex_lock(&ctx->ring_lock);

	ring = kmap_atomic(ctx->ring_pages[0]);
	head = ring->head;
	tail = ring->tail;
	kunmap_atomic(ring);

	pr_debug("h%u t%u m%u\n", head, tail, ctx->nr_events);

	if (head == tail)
		goto out;

	while (ret < nr) {
		long avail;
		struct io_event *ev;
		struct page *page;

		avail = (head <= tail ?  tail : ctx->nr_events) - head;
		if (head == tail)
			break;

		avail = min(avail, nr - ret);
		avail = min_t(long, avail, AIO_EVENTS_PER_PAGE -
			    ((head + AIO_EVENTS_OFFSET) % AIO_EVENTS_PER_PAGE));

		pos = head + AIO_EVENTS_OFFSET;
		page = ctx->ring_pages[pos / AIO_EVENTS_PER_PAGE];
		pos %= AIO_EVENTS_PER_PAGE;

		ev = kmap(page);
		copy_ret = copy_to_user(event + ret, ev + pos,
					sizeof(*ev) * avail);
		kunmap(page);

		if (unlikely(copy_ret)) {
			ret = -EFAULT;
			goto out;
		}

		ret += avail;
		head += avail;
		head %= ctx->nr_events;
	}

	ring = kmap_atomic(ctx->ring_pages[0]);
	ring->head = head;
	kunmap_atomic(ring);
	flush_dcache_page(ctx->ring_pages[0]);

	pr_debug("%li  h%u t%u\n", ret, head, tail);

	put_reqs_available(ctx, ret);
out:
	mutex_unlock(&ctx->ring_lock);

	return ret;
}

static bool aio_read_events(struct kioctx *ctx, long min_nr, long nr,
			    struct io_event __user *event, long *i)
{
	long ret = aio_read_events_ring(ctx, event + *i, nr - *i);

	if (ret > 0)
		*i += ret;

	if (unlikely(atomic_read(&ctx->dead)))
		ret = -EINVAL;

	if (!*i)
		*i = ret;

	return ret < 0 || *i >= min_nr;
}

static long read_events(struct kioctx *ctx, long min_nr, long nr,
			struct io_event __user *event,
			struct timespec __user *timeout)
{
	ktime_t until = { .tv64 = KTIME_MAX };
	long ret = 0;

	if (timeout) {
		struct timespec	ts;

		if (unlikely(copy_from_user(&ts, timeout, sizeof(ts))))
			return -EFAULT;

		until = timespec_to_ktime(ts);
	}

	/*
	 * Note that aio_read_events() is being called as the conditional - i.e.
	 * we're calling it after prepare_to_wait() has set task state to
	 * TASK_INTERRUPTIBLE.
	 *
	 * But aio_read_events() can block, and if it blocks it's going to flip
	 * the task state back to TASK_RUNNING.
	 *
	 * This should be ok, provided it doesn't flip the state back to
	 * TASK_RUNNING and return 0 too much - that causes us to spin. That
	 * will only happen if the mutex_lock() call blocks, and we then find
	 * the ringbuffer empty. So in practice we should be ok, but it's
	 * something to be aware of when touching this code.
	 */
	wait_event_interruptible_hrtimeout(ctx->wait,
			aio_read_events(ctx, min_nr, nr, event, &ret), until);

	if (!ret && signal_pending(current))
		ret = -EINTR;

	return ret;
}

/* sys_io_setup:
 *	Create an aio_context capable of receiving at least nr_events.
 *	ctxp must not point to an aio_context that already exists, and
 *	must be initialized to 0 prior to the call.  On successful
 *	creation of the aio_context, *ctxp is filled in with the resulting 
 *	handle.  May fail with -EINVAL if *ctxp is not initialized,
 *	if the specified nr_events exceeds internal limits.  May fail 
 *	with -EAGAIN if the specified nr_events exceeds the user's limit 
 *	of available events.  May fail with -ENOMEM if insufficient kernel
 *	resources are available.  May fail with -EFAULT if an invalid
 *	pointer is passed for ctxp.  Will fail with -ENOSYS if not
 *	implemented.
 */
SYSCALL_DEFINE2(io_setup, unsigned, nr_events, aio_context_t __user *, ctxp)
{
	struct kioctx *ioctx = NULL;
	unsigned long ctx;
	long ret;

	ret = get_user(ctx, ctxp);
	if (unlikely(ret))
		goto out;

	ret = -EINVAL;
	if (unlikely(ctx || nr_events == 0)) {
		pr_debug("EINVAL: io_setup: ctx %lu nr_events %u\n",
		         ctx, nr_events);
		goto out;
	}

	ioctx = ioctx_alloc(nr_events);
	ret = PTR_ERR(ioctx);
	if (!IS_ERR(ioctx)) {
		ret = put_user(ioctx->user_id, ctxp);
		if (ret)
			kill_ioctx(current->mm, ioctx);
		percpu_ref_put(&ioctx->users);
	}

out:
	return ret;
}

/* sys_io_destroy:
 *	Destroy the aio_context specified.  May cancel any outstanding 
 *	AIOs and block on completion.  Will fail with -ENOSYS if not
 *	implemented.  May fail with -EINVAL if the context pointed to
 *	is invalid.
 */
SYSCALL_DEFINE1(io_destroy, aio_context_t, ctx)
{
	struct kioctx *ioctx = lookup_ioctx(ctx);
	if (likely(NULL != ioctx)) {
		kill_ioctx(current->mm, ioctx);
		percpu_ref_put(&ioctx->users);
		return 0;
	}
	pr_debug("EINVAL: io_destroy: invalid context id\n");
	return -EINVAL;
}

typedef ssize_t (aio_rw_op)(struct kiocb *, const struct iovec *,
			    unsigned long, loff_t);

static ssize_t aio_setup_vectored_rw(struct kiocb *kiocb,
				     int rw, char __user *buf,
				     unsigned long *nr_segs,
				     struct iovec **iovec,
				     bool compat)
{
	ssize_t ret;

	*nr_segs = kiocb->ki_nbytes;

#ifdef CONFIG_COMPAT
	if (compat)
		ret = compat_rw_copy_check_uvector(rw,
				(struct compat_iovec __user *)buf,
				*nr_segs, 1, *iovec, iovec);
	else
#endif
		ret = rw_copy_check_uvector(rw,
				(struct iovec __user *)buf,
				*nr_segs, 1, *iovec, iovec);
	if (ret < 0)
		return ret;

	/* ki_nbytes now reflect bytes instead of segs */
	kiocb->ki_nbytes = ret;
	return 0;
}

static ssize_t aio_setup_single_vector(struct kiocb *kiocb,
				       int rw, char __user *buf,
				       unsigned long *nr_segs,
				       struct iovec *iovec)
{
	if (unlikely(!access_ok(!rw, buf, kiocb->ki_nbytes)))
		return -EFAULT;

	iovec->iov_base = buf;
	iovec->iov_len = kiocb->ki_nbytes;
	*nr_segs = 1;
	return 0;
}

/*
 * aio_setup_iocb:
 *	Performs the initial checks and aio retry method
 *	setup for the kiocb at the time of io submission.
 */
static ssize_t aio_run_iocb(struct kiocb *req, unsigned opcode,
			    char __user *buf, bool compat)
{
	struct file *file = req->ki_filp;
	ssize_t ret;
	unsigned long nr_segs;
	int rw;
	fmode_t mode;
	aio_rw_op *rw_op;
	struct iovec inline_vec, *iovec = &inline_vec;

	switch (opcode) {
	case IOCB_CMD_PREAD:
	case IOCB_CMD_PREADV:
		mode	= FMODE_READ;
		rw	= READ;
		rw_op	= file->f_op->aio_read;
		goto rw_common;

	case IOCB_CMD_PWRITE:
	case IOCB_CMD_PWRITEV:
		mode	= FMODE_WRITE;
		rw	= WRITE;
		rw_op	= file->f_op->aio_write;
		goto rw_common;
rw_common:
		if (unlikely(!(file->f_mode & mode)))
			return -EBADF;

		if (!rw_op)
			return -EINVAL;

		ret = (opcode == IOCB_CMD_PREADV ||
		       opcode == IOCB_CMD_PWRITEV)
			? aio_setup_vectored_rw(req, rw, buf, &nr_segs,
						&iovec, compat)
			: aio_setup_single_vector(req, rw, buf, &nr_segs,
						  iovec);
		if (ret)
			return ret;

		ret = rw_verify_area(rw, file, &req->ki_pos, req->ki_nbytes);
		if (ret < 0) {
			if (iovec != &inline_vec)
				kfree(iovec);
			return ret;
		}

		req->ki_nbytes = ret;

		/* XXX: move/kill - rw_verify_area()? */
		/* This matches the pread()/pwrite() logic */
		if (req->ki_pos < 0) {
			ret = -EINVAL;
			break;
		}

		if (rw == WRITE)
			file_start_write(file);

		ret = rw_op(req, iovec, nr_segs, req->ki_pos);

		if (rw == WRITE)
			file_end_write(file);
		break;

	case IOCB_CMD_FDSYNC:
		if (!file->f_op->aio_fsync)
			return -EINVAL;

		ret = file->f_op->aio_fsync(req, 1);
		break;

	case IOCB_CMD_FSYNC:
		if (!file->f_op->aio_fsync)
			return -EINVAL;

		ret = file->f_op->aio_fsync(req, 0);
		break;

	default:
		pr_debug("EINVAL: no operation provided\n");
		return -EINVAL;
	}

	if (iovec != &inline_vec)
		kfree(iovec);

	if (ret != -EIOCBQUEUED) {
		/*
		 * There's no easy way to restart the syscall since other AIO's
		 * may be already running. Just fail this IO with EINTR.
		 */
		if (unlikely(ret == -ERESTARTSYS || ret == -ERESTARTNOINTR ||
			     ret == -ERESTARTNOHAND ||
			     ret == -ERESTART_RESTARTBLOCK))
			ret = -EINTR;
		aio_complete(req, ret, 0);
	}

	return 0;
}

static int io_submit_one(struct kioctx *ctx, struct iocb __user *user_iocb,
			 struct iocb *iocb, bool compat)
{
	struct kiocb *req;
	ssize_t ret;

	/* enforce forwards compatibility on users */
	if (unlikely(iocb->aio_reserved1 || iocb->aio_reserved2)) {
		pr_debug("EINVAL: reserve field set\n");
		return -EINVAL;
	}

	/* prevent overflows */
	if (unlikely(
	    (iocb->aio_buf != (unsigned long)iocb->aio_buf) ||
	    (iocb->aio_nbytes != (size_t)iocb->aio_nbytes) ||
	    ((ssize_t)iocb->aio_nbytes < 0)
	   )) {
		pr_debug("EINVAL: io_submit: overflow check\n");
		return -EINVAL;
	}

	req = aio_get_req(ctx);
	if (unlikely(!req))
		return -EAGAIN;

	req->ki_filp = fget(iocb->aio_fildes);
	if (unlikely(!req->ki_filp)) {
		ret = -EBADF;
		goto out_put_req;
	}

	if (iocb->aio_flags & IOCB_FLAG_RESFD) {
		/*
		 * If the IOCB_FLAG_RESFD flag of aio_flags is set, get an
		 * instance of the file* now. The file descriptor must be
		 * an eventfd() fd, and will be signaled for each completed
		 * event using the eventfd_signal() function.
		 */
		req->ki_eventfd = eventfd_ctx_fdget((int) iocb->aio_resfd);
		if (IS_ERR(req->ki_eventfd)) {
			ret = PTR_ERR(req->ki_eventfd);
			req->ki_eventfd = NULL;
			goto out_put_req;
		}
	}

	ret = put_user(KIOCB_KEY, &user_iocb->aio_key);
	if (unlikely(ret)) {
		pr_debug("EFAULT: aio_key\n");
		goto out_put_req;
	}

	req->ki_obj.user = user_iocb;
	req->ki_user_data = iocb->aio_data;
	req->ki_pos = iocb->aio_offset;
	req->ki_nbytes = iocb->aio_nbytes;

	ret = aio_run_iocb(req, iocb->aio_lio_opcode,
			   (char __user *)(unsigned long)iocb->aio_buf,
			   compat);
	if (ret)
		goto out_put_req;

	return 0;
out_put_req:
	put_reqs_available(ctx, 1);
	percpu_ref_put(&ctx->reqs);
	kiocb_free(req);
	return ret;
}

long do_io_submit(aio_context_t ctx_id, long nr,
		  struct iocb __user *__user *iocbpp, bool compat)
{
	struct kioctx *ctx;
	long ret = 0;
	int i = 0;
	struct blk_plug plug;

	if (unlikely(nr < 0))
		return -EINVAL;

	if (unlikely(nr > LONG_MAX/sizeof(*iocbpp)))
		nr = LONG_MAX/sizeof(*iocbpp);

	if (unlikely(!access_ok(VERIFY_READ, iocbpp, (nr*sizeof(*iocbpp)))))
		return -EFAULT;

	ctx = lookup_ioctx(ctx_id);
	if (unlikely(!ctx)) {
		pr_debug("EINVAL: invalid context id\n");
		return -EINVAL;
	}

	blk_start_plug(&plug);

	/*
	 * AKPM: should this return a partial result if some of the IOs were
	 * successfully submitted?
	 */
	for (i=0; i<nr; i++) {
		struct iocb __user *user_iocb;
		struct iocb tmp;

		if (unlikely(__get_user(user_iocb, iocbpp + i))) {
			ret = -EFAULT;
			break;
		}

		if (unlikely(copy_from_user(&tmp, user_iocb, sizeof(tmp)))) {
			ret = -EFAULT;
			break;
		}

		ret = io_submit_one(ctx, user_iocb, &tmp, compat);
		if (ret)
			break;
	}
	blk_finish_plug(&plug);

	percpu_ref_put(&ctx->users);
	return i ? i : ret;
}

/* sys_io_submit:
 *	Queue the nr iocbs pointed to by iocbpp for processing.  Returns
 *	the number of iocbs queued.  May return -EINVAL if the aio_context
 *	specified by ctx_id is invalid, if nr is < 0, if the iocb at
 *	*iocbpp[0] is not properly initialized, if the operation specified
 *	is invalid for the file descriptor in the iocb.  May fail with
 *	-EFAULT if any of the data structures point to invalid data.  May
 *	fail with -EBADF if the file descriptor specified in the first
 *	iocb is invalid.  May fail with -EAGAIN if insufficient resources
 *	are available to queue any iocbs.  Will return 0 if nr is 0.  Will
 *	fail with -ENOSYS if not implemented.
 */
SYSCALL_DEFINE3(io_submit, aio_context_t, ctx_id, long, nr,
		struct iocb __user * __user *, iocbpp)
{
	return do_io_submit(ctx_id, nr, iocbpp, 0);
}

/* lookup_kiocb
 *	Finds a given iocb for cancellation.
 */
static struct kiocb *lookup_kiocb(struct kioctx *ctx, struct iocb __user *iocb,
				  u32 key)
{
	struct list_head *pos;

	assert_spin_locked(&ctx->ctx_lock);

	if (key != KIOCB_KEY)
		return NULL;

	/* TODO: use a hash or array, this sucks. */
	list_for_each(pos, &ctx->active_reqs) {
		struct kiocb *kiocb = list_kiocb(pos);
		if (kiocb->ki_obj.user == iocb)
			return kiocb;
	}
	return NULL;
}

/* sys_io_cancel:
 *	Attempts to cancel an iocb previously passed to io_submit.  If
 *	the operation is successfully cancelled, the resulting event is
 *	copied into the memory pointed to by result without being placed
 *	into the completion queue and 0 is returned.  May fail with
 *	-EFAULT if any of the data structures pointed to are invalid.
 *	May fail with -EINVAL if aio_context specified by ctx_id is
 *	invalid.  May fail with -EAGAIN if the iocb specified was not
 *	cancelled.  Will fail with -ENOSYS if not implemented.
 */
SYSCALL_DEFINE3(io_cancel, aio_context_t, ctx_id, struct iocb __user *, iocb,
		struct io_event __user *, result)
{
	struct kioctx *ctx;
	struct kiocb *kiocb;
	u32 key;
	int ret;

	ret = get_user(key, &iocb->aio_key);
	if (unlikely(ret))
		return -EFAULT;

	ctx = lookup_ioctx(ctx_id);
	if (unlikely(!ctx))
		return -EINVAL;

	spin_lock_irq(&ctx->ctx_lock);

	kiocb = lookup_kiocb(ctx, iocb, key);
	if (kiocb)
		ret = kiocb_cancel(ctx, kiocb);
	else
		ret = -EINVAL;

	spin_unlock_irq(&ctx->ctx_lock);

	if (!ret) {
		/*
		 * The result argument is no longer used - the io_event is
		 * always delivered via the ring buffer. -EINPROGRESS indicates
		 * cancellation is progress:
		 */
		ret = -EINPROGRESS;
	}

	percpu_ref_put(&ctx->users);

	return ret;
}

/* io_getevents:
 *	Attempts to read at least min_nr events and up to nr events from
 *	the completion queue for the aio_context specified by ctx_id. If
 *	it succeeds, the number of read events is returned. May fail with
 *	-EINVAL if ctx_id is invalid, if min_nr is out of range, if nr is
 *	out of range, if timeout is out of range.  May fail with -EFAULT
 *	if any of the memory specified is invalid.  May return 0 or
 *	< min_nr if the timeout specified by timeout has elapsed
 *	before sufficient events are available, where timeout == NULL
 *	specifies an infinite timeout. Note that the timeout pointed to by
 *	timeout is relative.  Will fail with -ENOSYS if not implemented.
 */
SYSCALL_DEFINE5(io_getevents, aio_context_t, ctx_id,
		long, min_nr,
		long, nr,
		struct io_event __user *, events,
		struct timespec __user *, timeout)
{
	struct kioctx *ioctx = lookup_ioctx(ctx_id);
	long ret = -EINVAL;

	if (likely(ioctx)) {
		if (likely(min_nr <= nr && min_nr >= 0))
			ret = read_events(ioctx, min_nr, nr, events, timeout);
		percpu_ref_put(&ioctx->users);
	}
	return ret;
}
