/*
 * stmp3xxx_rtc_wdt.h
 *
 * Copyright (C) 2011 Wolfram Sang, Pengutronix e.K.
 *
 * This file is released under the GPLv2.
 */
#ifndef __FIKUS_STMP3XXX_RTC_WDT_H
#define __FIKUS_STMP3XXX_RTC_WDT_H

struct stmp3xxx_wdt_pdata {
	void (*wdt_set_timeout)(struct device *dev, u32 timeout);
};

#endif /* __FIKUS_STMP3XXX_RTC_WDT_H */
