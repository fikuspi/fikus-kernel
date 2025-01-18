#ifndef __FIKUS_IIO_KFIFO_BUF_H__
#define __FIKUS_IIO_KFIFO_BUF_H__

#include <fikus/kfifo.h>
#include <fikus/iio/iio.h>
#include <fikus/iio/buffer.h>

struct iio_buffer *iio_kfifo_allocate(struct iio_dev *indio_dev);
void iio_kfifo_free(struct iio_buffer *r);

#endif
