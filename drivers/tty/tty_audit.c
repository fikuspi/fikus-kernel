/*
 * Creating audit events from TTY input.
 *
 * Copyright (C) 2007 Red Hat, Inc.  All rights reserved.  This copyrighted
 * material is made available to anyone wishing to use, modify, copy, or
 * redistribute it subject to the terms and conditions of the GNU General
 * Public License v.2.
 *
 * Authors: Miloslav Trmac <mitr@redhat.com>
 */

#include <fikus/audit.h>
#include <fikus/slab.h>
#include <fikus/tty.h>

struct tty_audit_buf {
	atomic_t count;
	struct mutex mutex;	/* Protects all data below */
	int major, minor;	/* The TTY which the data is from */
	unsigned icanon:1;
	size_t valid;
	unsigned char *data;	/* Allocated size N_TTY_BUF_SIZE */
};

static struct tty_audit_buf *tty_audit_buf_alloc(int major, int minor,
						 unsigned icanon)
{
	struct tty_audit_buf *buf;

	buf = kmalloc(sizeof(*buf), GFP_KERNEL);
	if (!buf)
		goto err;
	buf->data = kmalloc(N_TTY_BUF_SIZE, GFP_KERNEL);
	if (!buf->data)
		goto err_buf;
	atomic_set(&buf->count, 1);
	mutex_init(&buf->mutex);
	buf->major = major;
	buf->minor = minor;
	buf->icanon = icanon;
	buf->valid = 0;
	return buf;

err_buf:
	kfree(buf);
err:
	return NULL;
}

static void tty_audit_buf_free(struct tty_audit_buf *buf)
{
	WARN_ON(buf->valid != 0);
	kfree(buf->data);
	kfree(buf);
}

static void tty_audit_buf_put(struct tty_audit_buf *buf)
{
	if (atomic_dec_and_test(&buf->count))
		tty_audit_buf_free(buf);
}

static void tty_audit_log(const char *description, int major, int minor,
			  unsigned char *data, size_t size)
{
	struct audit_buffer *ab;
	struct task_struct *tsk = current;
	uid_t uid = from_kuid(&init_user_ns, task_uid(tsk));
	uid_t loginuid = from_kuid(&init_user_ns, audit_get_loginuid(tsk));
	u32 sessionid = audit_get_sessionid(tsk);

	ab = audit_log_start(NULL, GFP_KERNEL, AUDIT_TTY);
	if (ab) {
		char name[sizeof(tsk->comm)];

		audit_log_format(ab, "%s pid=%u uid=%u auid=%u ses=%u major=%d"
				 " minor=%d comm=", description, tsk->pid, uid,
				 loginuid, sessionid, major, minor);
		get_task_comm(name, tsk);
		audit_log_untrustedstring(ab, name);
		audit_log_format(ab, " data=");
		audit_log_n_hex(ab, data, size);
		audit_log_end(ab);
	}
}

/**
 *	tty_audit_buf_push	-	Push buffered data out
 *
 *	Generate an audit message from the contents of @buf, which is owned by
 *	the current task.  @buf->mutex must be locked.
 */
static void tty_audit_buf_push(struct tty_audit_buf *buf)
{
	if (buf->valid == 0)
		return;
	if (audit_enabled == 0) {
		buf->valid = 0;
		return;
	}
	tty_audit_log("tty", buf->major, buf->minor, buf->data, buf->valid);
	buf->valid = 0;
}

/**
 *	tty_audit_exit	-	Handle a task exit
 *
 *	Make sure all buffered data is written out and deallocate the buffer.
 *	Only needs to be called if current->signal->tty_audit_buf != %NULL.
 */
void tty_audit_exit(void)
{
	struct tty_audit_buf *buf;

	buf = current->signal->tty_audit_buf;
	current->signal->tty_audit_buf = NULL;
	if (!buf)
		return;

	mutex_lock(&buf->mutex);
	tty_audit_buf_push(buf);
	mutex_unlock(&buf->mutex);

	tty_audit_buf_put(buf);
}

/**
 *	tty_audit_fork	-	Copy TTY audit state for a new task
 *
 *	Set up TTY audit state in @sig from current.  @sig needs no locking.
 */
void tty_audit_fork(struct signal_struct *sig)
{
	sig->audit_tty = current->signal->audit_tty;
	sig->audit_tty_log_passwd = current->signal->audit_tty_log_passwd;
}

/**
 *	tty_audit_tiocsti	-	Log TIOCSTI
 */
void tty_audit_tiocsti(struct tty_struct *tty, char ch)
{
	struct tty_audit_buf *buf;
	int major, minor, should_audit;
	unsigned long flags;

	spin_lock_irqsave(&current->sighand->siglock, flags);
	should_audit = current->signal->audit_tty;
	buf = current->signal->tty_audit_buf;
	if (buf)
		atomic_inc(&buf->count);
	spin_unlock_irqrestore(&current->sighand->siglock, flags);

	major = tty->driver->major;
	minor = tty->driver->minor_start + tty->index;
	if (buf) {
		mutex_lock(&buf->mutex);
		if (buf->major == major && buf->minor == minor)
			tty_audit_buf_push(buf);
		mutex_unlock(&buf->mutex);
		tty_audit_buf_put(buf);
	}

	if (should_audit && audit_enabled) {
		kuid_t auid;
		unsigned int sessionid;

		auid = audit_get_loginuid(current);
		sessionid = audit_get_sessionid(current);
		tty_audit_log("ioctl=TIOCSTI", major, minor, &ch, 1);
	}
}

/**
 * tty_audit_push_current -	Flush current's pending audit data
 *
 * Try to lock sighand and get a reference to the tty audit buffer if available.
 * Flush the buffer or return an appropriate error code.
 */
int tty_audit_push_current(void)
{
	struct tty_audit_buf *buf = ERR_PTR(-EPERM);
	struct task_struct *tsk = current;
	unsigned long flags;

	if (!lock_task_sighand(tsk, &flags))
		return -ESRCH;

	if (tsk->signal->audit_tty) {
		buf = tsk->signal->tty_audit_buf;
		if (buf)
			atomic_inc(&buf->count);
	}
	unlock_task_sighand(tsk, &flags);

	/*
	 * Return 0 when signal->audit_tty set
	 * but tsk->signal->tty_audit_buf == NULL.
	 */
	if (!buf || IS_ERR(buf))
		return PTR_ERR(buf);

	mutex_lock(&buf->mutex);
	tty_audit_buf_push(buf);
	mutex_unlock(&buf->mutex);

	tty_audit_buf_put(buf);
	return 0;
}

/**
 *	tty_audit_buf_get	-	Get an audit buffer.
 *
 *	Get an audit buffer for @tty, allocate it if necessary.  Return %NULL
 *	if TTY auditing is disabled or out of memory.  Otherwise, return a new
 *	reference to the buffer.
 */
static struct tty_audit_buf *tty_audit_buf_get(struct tty_struct *tty,
		unsigned icanon)
{
	struct tty_audit_buf *buf, *buf2;
	unsigned long flags;

	buf = NULL;
	buf2 = NULL;
	spin_lock_irqsave(&current->sighand->siglock, flags);
	if (likely(!current->signal->audit_tty))
		goto out;
	buf = current->signal->tty_audit_buf;
	if (buf) {
		atomic_inc(&buf->count);
		goto out;
	}
	spin_unlock_irqrestore(&current->sighand->siglock, flags);

	buf2 = tty_audit_buf_alloc(tty->driver->major,
				   tty->driver->minor_start + tty->index,
				   icanon);
	if (buf2 == NULL) {
		audit_log_lost("out of memory in TTY auditing");
		return NULL;
	}

	spin_lock_irqsave(&current->sighand->siglock, flags);
	if (!current->signal->audit_tty)
		goto out;
	buf = current->signal->tty_audit_buf;
	if (!buf) {
		current->signal->tty_audit_buf = buf2;
		buf = buf2;
		buf2 = NULL;
	}
	atomic_inc(&buf->count);
	/* Fall through */
 out:
	spin_unlock_irqrestore(&current->sighand->siglock, flags);
	if (buf2)
		tty_audit_buf_free(buf2);
	return buf;
}

/**
 *	tty_audit_add_data	-	Add data for TTY auditing.
 *
 *	Audit @data of @size from @tty, if necessary.
 */
void tty_audit_add_data(struct tty_struct *tty, unsigned char *data,
			size_t size, unsigned icanon)
{
	struct tty_audit_buf *buf;
	int major, minor;
	int audit_log_tty_passwd;
	unsigned long flags;

	if (unlikely(size == 0))
		return;

	spin_lock_irqsave(&current->sighand->siglock, flags);
	audit_log_tty_passwd = current->signal->audit_tty_log_passwd;
	spin_unlock_irqrestore(&current->sighand->siglock, flags);
	if (!audit_log_tty_passwd && icanon && !L_ECHO(tty))
		return;

	if (tty->driver->type == TTY_DRIVER_TYPE_PTY
	    && tty->driver->subtype == PTY_TYPE_MASTER)
		return;

	buf = tty_audit_buf_get(tty, icanon);
	if (!buf)
		return;

	mutex_lock(&buf->mutex);
	major = tty->driver->major;
	minor = tty->driver->minor_start + tty->index;
	if (buf->major != major || buf->minor != minor
	    || buf->icanon != icanon) {
		tty_audit_buf_push(buf);
		buf->major = major;
		buf->minor = minor;
		buf->icanon = icanon;
	}
	do {
		size_t run;

		run = N_TTY_BUF_SIZE - buf->valid;
		if (run > size)
			run = size;
		memcpy(buf->data + buf->valid, data, run);
		buf->valid += run;
		data += run;
		size -= run;
		if (buf->valid == N_TTY_BUF_SIZE)
			tty_audit_buf_push(buf);
	} while (size != 0);
	mutex_unlock(&buf->mutex);
	tty_audit_buf_put(buf);
}

/**
 *	tty_audit_push	-	Push buffered data out
 *
 *	Make sure no audit data is pending for @tty on the current process.
 */
void tty_audit_push(struct tty_struct *tty)
{
	struct tty_audit_buf *buf;
	unsigned long flags;

	spin_lock_irqsave(&current->sighand->siglock, flags);
	if (likely(!current->signal->audit_tty)) {
		spin_unlock_irqrestore(&current->sighand->siglock, flags);
		return;
	}
	buf = current->signal->tty_audit_buf;
	if (buf)
		atomic_inc(&buf->count);
	spin_unlock_irqrestore(&current->sighand->siglock, flags);

	if (buf) {
		int major, minor;

		major = tty->driver->major;
		minor = tty->driver->minor_start + tty->index;
		mutex_lock(&buf->mutex);
		if (buf->major == major && buf->minor == minor)
			tty_audit_buf_push(buf);
		mutex_unlock(&buf->mutex);
		tty_audit_buf_put(buf);
	}
}
