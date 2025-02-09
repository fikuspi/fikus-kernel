#ifndef _COMEDI_INTERNAL_H
#define _COMEDI_INTERNAL_H

#include <fikus/types.h>

/*
 * various internal comedi stuff
 */
int do_rangeinfo_ioctl(struct comedi_device *dev,
		       struct comedi_rangeinfo __user *arg);
struct comedi_device *comedi_alloc_board_minor(struct device *hardware_device);
void comedi_release_hardware_device(struct device *hardware_device);
int comedi_alloc_subdevice_minor(struct comedi_subdevice *s);
void comedi_free_subdevice_minor(struct comedi_subdevice *s);

int comedi_buf_alloc(struct comedi_device *dev, struct comedi_subdevice *s,
		     unsigned long new_size);
void comedi_buf_reset(struct comedi_async *async);
unsigned int comedi_buf_write_n_allocated(struct comedi_async *async);

extern unsigned int comedi_default_buf_size_kb;
extern unsigned int comedi_default_buf_maxsize_kb;

/* drivers.c */

extern struct comedi_driver *comedi_drivers;
extern struct mutex comedi_drivers_list_lock;

int insn_inval(struct comedi_device *, struct comedi_subdevice *,
	       struct comedi_insn *, unsigned int *);

void comedi_device_detach(struct comedi_device *);
int comedi_device_attach(struct comedi_device *, struct comedi_devconfig *);

#ifdef CONFIG_PROC_FS

/* proc.c */

void comedi_proc_init(void);
void comedi_proc_cleanup(void);
#else
static inline void comedi_proc_init(void)
{
}
static inline void comedi_proc_cleanup(void)
{
}
#endif

#endif /* _COMEDI_INTERNAL_H */
