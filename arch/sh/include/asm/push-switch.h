#ifndef __ASM_SH_PUSH_SWITCH_H
#define __ASM_SH_PUSH_SWITCH_H

#include <fikus/timer.h>
#include <fikus/interrupt.h>
#include <fikus/workqueue.h>
#include <fikus/platform_device.h>

struct push_switch {
	/* switch state */
	unsigned int		state:1;
	/* debounce timer */
	struct timer_list	debounce;
	/* workqueue */
	struct work_struct	work;
	/* platform device, for workqueue handler */
	struct platform_device	*pdev;
};

struct push_switch_platform_info {
	/* IRQ handler */
	irqreturn_t		(*irq_handler)(int irq, void *data);
	/* Special IRQ flags */
	unsigned int		irq_flags;
	/* Bit location of switch */
	unsigned int		bit;
	/* Symbolic switch name */
	const char		*name;
};

#endif /* __ASM_SH_PUSH_SWITCH_H */
