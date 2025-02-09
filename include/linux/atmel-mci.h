#ifndef __FIKUS_ATMEL_MCI_H
#define __FIKUS_ATMEL_MCI_H

#define ATMCI_MAX_NR_SLOTS	2

/**
 * struct mci_slot_pdata - board-specific per-slot configuration
 * @bus_width: Number of data lines wired up the slot
 * @detect_pin: GPIO pin wired to the card detect switch
 * @wp_pin: GPIO pin wired to the write protect sensor
 * @detect_is_active_high: The state of the detect pin when it is active
 *
 * If a given slot is not present on the board, @bus_width should be
 * set to 0. The other fields are ignored in this case.
 *
 * Any pins that aren't available should be set to a negative value.
 *
 * Note that support for multiple slots is experimental -- some cards
 * might get upset if we don't get the clock management exactly right.
 * But in most cases, it should work just fine.
 */
struct mci_slot_pdata {
	unsigned int		bus_width;
	int			detect_pin;
	int			wp_pin;
	bool			detect_is_active_high;
};

/**
 * struct mci_platform_data - board-specific MMC/SDcard configuration
 * @dma_slave: DMA slave interface to use in data transfers.
 * @slot: Per-slot configuration data.
 */
struct mci_platform_data {
	struct mci_dma_data	*dma_slave;
	struct mci_slot_pdata	slot[ATMCI_MAX_NR_SLOTS];
};

#endif /* __FIKUS_ATMEL_MCI_H */
