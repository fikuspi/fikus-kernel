#ifndef FIKUS_BCM47XX_WDT_H_
#define FIKUS_BCM47XX_WDT_H_

#include <fikus/notifier.h>
#include <fikus/timer.h>
#include <fikus/types.h>
#include <fikus/watchdog.h>


struct bcm47xx_wdt {
	u32 (*timer_set)(struct bcm47xx_wdt *, u32);
	u32 (*timer_set_ms)(struct bcm47xx_wdt *, u32);
	u32 max_timer_ms;

	void *driver_data;

	struct watchdog_device wdd;
	struct notifier_block notifier;

	struct timer_list soft_timer;
	atomic_t soft_ticks;
};

static inline void *bcm47xx_wdt_get_drvdata(struct bcm47xx_wdt *wdt)
{
	return wdt->driver_data;
}
#endif /* FIKUS_BCM47XX_WDT_H_ */
