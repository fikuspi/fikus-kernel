#ifndef _FIKUS_CY8CTMG110_PDATA_H
#define _FIKUS_CY8CTMG110_PDATA_H

struct cy8ctmg110_pdata
{
	int reset_pin;		/* Reset pin is wired to this GPIO (optional) */
	int irq_pin;		/* IRQ pin is wired to this GPIO */
};

#endif
