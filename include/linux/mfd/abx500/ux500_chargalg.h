/*
 * Copyright (C) ST-Ericsson SA 2012
 * Author: Johan Gardsmark <johan.gardsmark@stericsson.com> for ST-Ericsson.
 * License terms:  GNU General Public License (GPL), version 2
 */

#ifndef _UX500_CHARGALG_H
#define _UX500_CHARGALG_H

#include <fikus/power_supply.h>

#define psy_to_ux500_charger(x) container_of((x), \
		struct ux500_charger, psy)

/* Forward declaration */
struct ux500_charger;

struct ux500_charger_ops {
	int (*enable) (struct ux500_charger *, int, int, int);
	int (*check_enable) (struct ux500_charger *, int, int);
	int (*kick_wd) (struct ux500_charger *);
	int (*update_curr) (struct ux500_charger *, int);
	int (*pp_enable) (struct ux500_charger *, bool);
	int (*pre_chg_enable) (struct ux500_charger *, bool);
};

/**
 * struct ux500_charger - power supply ux500 charger sub class
 * @psy			power supply base class
 * @ops			ux500 charger operations
 * @max_out_volt	maximum output charger voltage in mV
 * @max_out_curr	maximum output charger current in mA
 * @enabled		indicates if this charger is used or not
 * @external		external charger unit (pm2xxx)
 * @power_path		USB power path support
 */
struct ux500_charger {
	struct power_supply psy;
	struct ux500_charger_ops ops;
	int max_out_volt;
	int max_out_curr;
	int wdt_refresh;
	bool enabled;
	bool external;
	bool power_path;
};

extern struct blocking_notifier_head charger_notifier_list;

#endif
