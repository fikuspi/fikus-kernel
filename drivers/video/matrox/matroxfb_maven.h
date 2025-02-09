#ifndef __MATROXFB_MAVEN_H__
#define __MATROXFB_MAVEN_H__

#include <fikus/ioctl.h>
#include <fikus/i2c.h>
#include <fikus/i2c-algo-bit.h>
#include "matroxfb_base.h"

struct i2c_bit_adapter {
	struct i2c_adapter		adapter;
	int				initialized;
	struct i2c_algo_bit_data	bac;
	struct matrox_fb_info*		minfo;
	struct {
		unsigned int		data;
		unsigned int		clock;
				      } mask;
};

#endif /* __MATROXFB_MAVEN_H__ */
