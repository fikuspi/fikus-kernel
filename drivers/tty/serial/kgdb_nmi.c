/*
 * KGDB NMI serial console
 *
 * Copyright 2010 Google, Inc.
 *		  Arve Hjønnevåg <arve@android.com>
 *		  Colin Cross <ccross@android.com>
 * Copyright 2012 Linaro Ltd.
 *		  Anton Vorontsov <anton.vorontsov@linaro.org>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 */

#include <fikus/kernel.h>
#include <fikus/module.h>
#include <fikus/compiler.h>
#include <fikus/init.h>
#include <fikus/slab.h>
#include <fikus/errno.h>
#include <fikus/atomic.h>
#include <fikus/console.h>
#include <fikus/tty.h>
#include <fikus/tty_driver.h>
#include <fikus/tty_flip.h>
#include <fikus/serial_core.h>
#include <fikus/interrupt.h>
#include <fikus/hrtimer.h>
#include <fikus/tick.h>
#include <fikus/kfifo.h>
#include <fikus/kgdb.h>
#include <fikus/kdb.h>

static int kgdb_nmi_knock = 1;
module_param_named(knock, kgdb_nmi_knock, int, 0600);
MODULE_PARM_DESC(knock, "if set to 1 (default), the special '$3#33' command " \
			"must be used to enter the debugger; when set to 0, " \
			"hitting return key is enough to enter the debugger; " \
			"when set to -1, the debugger is entered immediately " \
			"upon NMI");

static char *kgdb_nmi_magic = "$3#33";
module_param_named(magic, kgdb_nmi_magic, charp, 0600);
MODULE_PARM_DESC(magic, "magic sequence to enter NMI debugger (default $3#33)");

static bool kgdb_nmi_tty_enabled;

static void kgdb_nmi_console_write(struct console *co, const char *s, uint c)
{
	int i;

	if (!kgdb_nmi_tty_enabled || atomic_read(&kgdb_active) >= 0)
		return;

	for (i = 0; i < c; i++)
		dbg_io_ops->write_char(s[i]);
}

static struct tty_driver *kgdb_nmi_tty_driver;

static struct tty_driver *kgdb_nmi_console_device(struct console *co, int *idx)
{
	*idx = co->index;
	return kgdb_nmi_tty_driver;
}

static struct console kgdb_nmi_console = {
	.name	= "ttyNMI",
	.write	= kgdb_nmi_console_write,
	.device	= kgdb_nmi_console_device,
	.flags	= CON_PRINTBUFFER | CON_ANYTIME | CON_ENABLED,
	.index	= -1,
};

/*
 * This is usually the maximum rate on debug ports. We make fifo large enough
 * to make copy-pasting to the terminal usable.
 */
#define KGDB_NMI_BAUD		115200
#define KGDB_NMI_FIFO_SIZE	roundup_pow_of_two(KGDB_NMI_BAUD / 8 / HZ)

struct kgdb_nmi_tty_priv {
	struct tty_port port;
	struct tasklet_struct tlet;
	STRUCT_KFIFO(char, KGDB_NMI_FIFO_SIZE) fifo;
};

static struct kgdb_nmi_tty_priv *kgdb_nmi_port_to_priv(struct tty_port *port)
{
	return container_of(port, struct kgdb_nmi_tty_priv, port);
}

/*
 * Our debugging console is polled in a tasklet, so we'll check for input
 * every tick. In HZ-less mode, we should program the next tick.  We have
 * to use the lowlevel stuff as no locks should be grabbed.
 */
#ifdef CONFIG_HIGH_RES_TIMERS
static void kgdb_tty_poke(void)
{
	tick_program_event(ktime_get(), 0);
}
#else
static inline void kgdb_tty_poke(void) {}
#endif

static struct tty_port *kgdb_nmi_port;

static void kgdb_tty_recv(int ch)
{
	struct kgdb_nmi_tty_priv *priv;
	char c = ch;

	if (!kgdb_nmi_port || ch < 0)
		return;
	/*
	 * Can't use port->tty->driver_data as tty might be not there. Tasklet
	 * will check for tty and will get the ref, but here we don't have to
	 * do that, and actually, we can't: we're in NMI context, no locks are
	 * possible.
	 */
	priv = kgdb_nmi_port_to_priv(kgdb_nmi_port);
	kfifo_in(&priv->fifo, &c, 1);
	kgdb_tty_poke();
}

static int kgdb_nmi_poll_one_knock(void)
{
	static int n;
	int c = -1;
	const char *magic = kgdb_nmi_magic;
	size_t m = strlen(magic);
	bool printch = 0;

	c = dbg_io_ops->read_char();
	if (c == NO_POLL_CHAR)
		return c;

	if (!kgdb_nmi_knock && (c == '\r' || c == '\n')) {
		return 1;
	} else if (c == magic[n]) {
		n = (n + 1) % m;
		if (!n)
			return 1;
		printch = 1;
	} else {
		n = 0;
	}

	if (kgdb_nmi_tty_enabled) {
		kgdb_tty_recv(c);
		return 0;
	}

	if (printch) {
		kdb_printf("%c", c);
		return 0;
	}

	kdb_printf("\r%s %s to enter the debugger> %*s",
		   kgdb_nmi_knock ? "Type" : "Hit",
		   kgdb_nmi_knock ? magic  : "<return>", (int)m, "");
	while (m--)
		kdb_printf("\b");
	return 0;
}

/**
 * kgdb_nmi_poll_knock - Check if it is time to enter the debugger
 *
 * "Serial ports are often noisy, especially when muxed over another port (we
 * often use serial over the headset connector). Noise on the async command
 * line just causes characters that are ignored, on a command line that blocked
 * execution noise would be catastrophic." -- Colin Cross
 *
 * So, this function implements KGDB/KDB knocking on the serial line: we won't
 * enter the debugger until we receive a known magic phrase (which is actually
 * "$3#33", known as "escape to KDB" command. There is also a relaxed variant
 * of knocking, i.e. just pressing the return key is enough to enter the
 * debugger. And if knocking is disabled, the function always returns 1.
 */
bool kgdb_nmi_poll_knock(void)
{
	if (kgdb_nmi_knock < 0)
		return 1;

	while (1) {
		int ret;

		ret = kgdb_nmi_poll_one_knock();
		if (ret == NO_POLL_CHAR)
			return 0;
		else if (ret == 1)
			break;
	}
	return 1;
}

/*
 * The tasklet is cheap, it does not cause wakeups when reschedules itself,
 * instead it waits for the next tick.
 */
static void kgdb_nmi_tty_receiver(unsigned long data)
{
	struct kgdb_nmi_tty_priv *priv = (void *)data;
	char ch;

	tasklet_schedule(&priv->tlet);

	if (likely(!kgdb_nmi_tty_enabled || !kfifo_len(&priv->fifo)))
		return;

	while (kfifo_out(&priv->fifo, &ch, 1))
		tty_insert_flip_char(&priv->port, ch, TTY_NORMAL);
	tty_flip_buffer_push(&priv->port);
}

static int kgdb_nmi_tty_activate(struct tty_port *port, struct tty_struct *tty)
{
	struct kgdb_nmi_tty_priv *priv = tty->driver_data;

	kgdb_nmi_port = port;
	tasklet_schedule(&priv->tlet);
	return 0;
}

static void kgdb_nmi_tty_shutdown(struct tty_port *port)
{
	struct kgdb_nmi_tty_priv *priv = port->tty->driver_data;

	tasklet_kill(&priv->tlet);
	kgdb_nmi_port = NULL;
}

static const struct tty_port_operations kgdb_nmi_tty_port_ops = {
	.activate	= kgdb_nmi_tty_activate,
	.shutdown	= kgdb_nmi_tty_shutdown,
};

static int kgdb_nmi_tty_install(struct tty_driver *drv, struct tty_struct *tty)
{
	struct kgdb_nmi_tty_priv *priv;
	int ret;

	priv = kzalloc(sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	INIT_KFIFO(priv->fifo);
	tasklet_init(&priv->tlet, kgdb_nmi_tty_receiver, (unsigned long)priv);
	tty_port_init(&priv->port);
	priv->port.ops = &kgdb_nmi_tty_port_ops;
	tty->driver_data = priv;

	ret = tty_port_install(&priv->port, drv, tty);
	if (ret) {
		pr_err("%s: can't install tty port: %d\n", __func__, ret);
		goto err;
	}
	return 0;
err:
	tty_port_destroy(&priv->port);
	kfree(priv);
	return ret;
}

static void kgdb_nmi_tty_cleanup(struct tty_struct *tty)
{
	struct kgdb_nmi_tty_priv *priv = tty->driver_data;

	tty->driver_data = NULL;
	tty_port_destroy(&priv->port);
	kfree(priv);
}

static int kgdb_nmi_tty_open(struct tty_struct *tty, struct file *file)
{
	struct kgdb_nmi_tty_priv *priv = tty->driver_data;

	return tty_port_open(&priv->port, tty, file);
}

static void kgdb_nmi_tty_close(struct tty_struct *tty, struct file *file)
{
	struct kgdb_nmi_tty_priv *priv = tty->driver_data;

	tty_port_close(&priv->port, tty, file);
}

static void kgdb_nmi_tty_hangup(struct tty_struct *tty)
{
	struct kgdb_nmi_tty_priv *priv = tty->driver_data;

	tty_port_hangup(&priv->port);
}

static int kgdb_nmi_tty_write_room(struct tty_struct *tty)
{
	/* Actually, we can handle any amount as we use polled writes. */
	return 2048;
}

static int kgdb_nmi_tty_write(struct tty_struct *tty, const unchar *buf, int c)
{
	int i;

	for (i = 0; i < c; i++)
		dbg_io_ops->write_char(buf[i]);
	return c;
}

static const struct tty_operations kgdb_nmi_tty_ops = {
	.open		= kgdb_nmi_tty_open,
	.close		= kgdb_nmi_tty_close,
	.install	= kgdb_nmi_tty_install,
	.cleanup	= kgdb_nmi_tty_cleanup,
	.hangup		= kgdb_nmi_tty_hangup,
	.write_room	= kgdb_nmi_tty_write_room,
	.write		= kgdb_nmi_tty_write,
};

static int kgdb_nmi_enable_console(int argc, const char *argv[])
{
	kgdb_nmi_tty_enabled = !(argc == 1 && !strcmp(argv[1], "off"));
	return 0;
}

int kgdb_register_nmi_console(void)
{
	int ret;

	if (!arch_kgdb_ops.enable_nmi)
		return 0;

	kgdb_nmi_tty_driver = alloc_tty_driver(1);
	if (!kgdb_nmi_tty_driver) {
		pr_err("%s: cannot allocate tty\n", __func__);
		return -ENOMEM;
	}
	kgdb_nmi_tty_driver->driver_name	= "ttyNMI";
	kgdb_nmi_tty_driver->name		= "ttyNMI";
	kgdb_nmi_tty_driver->num		= 1;
	kgdb_nmi_tty_driver->type		= TTY_DRIVER_TYPE_SERIAL;
	kgdb_nmi_tty_driver->subtype		= SERIAL_TYPE_NORMAL;
	kgdb_nmi_tty_driver->flags		= TTY_DRIVER_REAL_RAW;
	kgdb_nmi_tty_driver->init_termios	= tty_std_termios;
	tty_termios_encode_baud_rate(&kgdb_nmi_tty_driver->init_termios,
				     KGDB_NMI_BAUD, KGDB_NMI_BAUD);
	tty_set_operations(kgdb_nmi_tty_driver, &kgdb_nmi_tty_ops);

	ret = tty_register_driver(kgdb_nmi_tty_driver);
	if (ret) {
		pr_err("%s: can't register tty driver: %d\n", __func__, ret);
		goto err_drv_reg;
	}

	ret = kdb_register("nmi_console", kgdb_nmi_enable_console, "[off]",
			   "switch to Fikus NMI console", 0);
	if (ret) {
		pr_err("%s: can't register kdb command: %d\n", __func__, ret);
		goto err_kdb_reg;
	}

	register_console(&kgdb_nmi_console);
	arch_kgdb_ops.enable_nmi(1);

	return 0;
err_kdb_reg:
	tty_unregister_driver(kgdb_nmi_tty_driver);
err_drv_reg:
	put_tty_driver(kgdb_nmi_tty_driver);
	return ret;
}
EXPORT_SYMBOL_GPL(kgdb_register_nmi_console);

int kgdb_unregister_nmi_console(void)
{
	int ret;

	if (!arch_kgdb_ops.enable_nmi)
		return 0;
	arch_kgdb_ops.enable_nmi(0);

	kdb_unregister("nmi_console");

	ret = unregister_console(&kgdb_nmi_console);
	if (ret)
		return ret;

	ret = tty_unregister_driver(kgdb_nmi_tty_driver);
	if (ret)
		return ret;
	put_tty_driver(kgdb_nmi_tty_driver);

	return 0;
}
EXPORT_SYMBOL_GPL(kgdb_unregister_nmi_console);
