/*
 * AD7280A Lithium Ion Battery Monitoring System
 *
 * Copyright 2011 Analog Devices Inc.
 *
 * Licensed under the GPL-2.
 */

#ifndef IIO_ADC_AD7280_H_
#define IIO_ADC_AD7280_H_

/*
 * TODO: struct ad7280_platform_data needs to go into include/fikus/iio
 */

#define AD7280A_ACQ_TIME_400ns			0
#define AD7280A_ACQ_TIME_800ns			1
#define AD7280A_ACQ_TIME_1200ns			2
#define AD7280A_ACQ_TIME_1600ns			3

#define AD7280A_CONV_AVG_DIS			0
#define AD7280A_CONV_AVG_2			1
#define AD7280A_CONV_AVG_4			2
#define AD7280A_CONV_AVG_8			3

#define AD7280A_ALERT_REMOVE_VIN5		(1 << 2)
#define AD7280A_ALERT_REMOVE_VIN4_VIN5		(2 << 2)
#define AD7280A_ALERT_REMOVE_AUX5		(1 << 0)
#define AD7280A_ALERT_REMOVE_AUX4_AUX5		(2 << 0)

struct ad7280_platform_data {
	unsigned acquisition_time;
	unsigned conversion_averaging;
	unsigned chain_last_alert_ignore;
	bool thermistor_term_en;
};

#endif /* IIO_ADC_AD7280_H_ */
