/*
 *pxa168 ethernet platform device data definition file.
 */
#ifndef __FIKUS_PXA168_ETH_H
#define __FIKUS_PXA168_ETH_H

struct pxa168_eth_platform_data {
	int	port_number;
	int	phy_addr;

	/*
	 * If speed is 0, then speed and duplex are autonegotiated.
	 */
	int	speed;		/* 0, SPEED_10, SPEED_100 */
	int	duplex;		/* DUPLEX_HALF or DUPLEX_FULL */

	/*
	 * Override default RX/TX queue sizes if nonzero.
	 */
	int	rx_queue_size;
	int	tx_queue_size;

	/*
	 * init callback is used for board specific initialization
	 * e.g on Aspenite its used to initialize the PHY transceiver.
	 */
	int (*init)(void);
};

#endif /* __FIKUS_PXA168_ETH_H */
