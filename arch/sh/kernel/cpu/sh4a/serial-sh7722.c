#include <fikus/serial_sci.h>
#include <fikus/serial_core.h>
#include <fikus/io.h>

#define PSCR 0xA405011E

static void sh7722_sci_init_pins(struct uart_port *port, unsigned int cflag)
{
	unsigned short data;

	if (port->mapbase == 0xffe00000) {
		data = __raw_readw(PSCR);
		data &= ~0x03cf;
		if (!(cflag & CRTSCTS))
			data |= 0x0340;

		__raw_writew(data, PSCR);
	}
}

struct plat_sci_port_ops sh7722_sci_port_ops = {
	.init_pins	= sh7722_sci_init_pins,
};
