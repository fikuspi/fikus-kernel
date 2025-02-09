#ifndef BCM63XX_DEV_ENET_H_
#define BCM63XX_DEV_ENET_H_

#include <fikus/if_ether.h>
#include <fikus/init.h>

#include <bcm63xx_regs.h>

/*
 * on board ethernet platform data
 */
struct bcm63xx_enet_platform_data {
	char mac_addr[ETH_ALEN];

	int has_phy;

	/* if has_phy, then set use_internal_phy */
	int use_internal_phy;

	/* or fill phy info to use an external one */
	int phy_id;
	int has_phy_interrupt;
	int phy_interrupt;

	/* if has_phy, use autonegociated pause parameters or force
	 * them */
	int pause_auto;
	int pause_rx;
	int pause_tx;

	/* if !has_phy, set desired forced speed/duplex */
	int force_speed_100;
	int force_duplex_full;

	/* if !has_phy, set callback to perform mii device
	 * init/remove */
	int (*mii_config)(struct net_device *dev, int probe,
			  int (*mii_read)(struct net_device *dev,
					  int phy_id, int reg),
			  void (*mii_write)(struct net_device *dev,
					    int phy_id, int reg, int val));

	/* DMA channel enable mask */
	u32 dma_chan_en_mask;

	/* DMA channel interrupt mask */
	u32 dma_chan_int_mask;

	/* DMA engine has internal SRAM */
	bool dma_has_sram;

	/* DMA channel register width */
	unsigned int dma_chan_width;

	/* DMA descriptor shift */
	unsigned int dma_desc_shift;
};

/*
 * on board ethernet switch platform data
 */
#define ENETSW_MAX_PORT	8
#define ENETSW_PORTS_6328 5 /* 4 FE PHY + 1 RGMII */
#define ENETSW_PORTS_6368 6 /* 4 FE PHY + 2 RGMII */

#define ENETSW_RGMII_PORT0	4

struct bcm63xx_enetsw_port {
	int		used;
	int		phy_id;

	int		bypass_link;
	int		force_speed;
	int		force_duplex_full;

	const char	*name;
};

struct bcm63xx_enetsw_platform_data {
	char mac_addr[ETH_ALEN];
	int num_ports;
	struct bcm63xx_enetsw_port used_ports[ENETSW_MAX_PORT];

	/* DMA channel enable mask */
	u32 dma_chan_en_mask;

	/* DMA channel interrupt mask */
	u32 dma_chan_int_mask;

	/* DMA channel register width */
	unsigned int dma_chan_width;

	/* DMA engine has internal SRAM */
	bool dma_has_sram;
};

int __init bcm63xx_enet_register(int unit,
				 const struct bcm63xx_enet_platform_data *pd);

int bcm63xx_enetsw_register(const struct bcm63xx_enetsw_platform_data *pd);

enum bcm63xx_regs_enetdmac {
	ENETDMAC_CHANCFG,
	ENETDMAC_IR,
	ENETDMAC_IRMASK,
	ENETDMAC_MAXBURST,
	ENETDMAC_BUFALLOC,
	ENETDMAC_RSTART,
	ENETDMAC_FC,
	ENETDMAC_LEN,
};

static inline unsigned long bcm63xx_enetdmacreg(enum bcm63xx_regs_enetdmac reg)
{
#ifdef BCMCPU_RUNTIME_DETECT
	extern const unsigned long *bcm63xx_regs_enetdmac;

	return bcm63xx_regs_enetdmac[reg];
#else
#ifdef CONFIG_BCM63XX_CPU_6345
	switch (reg) {
	case ENETDMAC_CHANCFG:
		return ENETDMA_6345_CHANCFG_REG;
	case ENETDMAC_IR:
		return ENETDMA_6345_IR_REG;
	case ENETDMAC_IRMASK:
		return ENETDMA_6345_IRMASK_REG;
	case ENETDMAC_MAXBURST:
		return ENETDMA_6345_MAXBURST_REG;
	case ENETDMAC_BUFALLOC:
		return ENETDMA_6345_BUFALLOC_REG;
	case ENETDMAC_RSTART:
		return ENETDMA_6345_RSTART_REG;
	case ENETDMAC_FC:
		return ENETDMA_6345_FC_REG;
	case ENETDMAC_LEN:
		return ENETDMA_6345_LEN_REG;
	}
#endif
#if defined(CONFIG_BCM63XX_CPU_6328) || \
	defined(CONFIG_BCM63XX_CPU_6338) || \
	defined(CONFIG_BCM63XX_CPU_6348) || \
	defined(CONFIG_BCM63XX_CPU_6358) || \
	defined(CONFIG_BCM63XX_CPU_6362) || \
	defined(CONFIG_BCM63XX_CPU_6368)
	switch (reg) {
	case ENETDMAC_CHANCFG:
		return ENETDMAC_CHANCFG_REG;
	case ENETDMAC_IR:
		return ENETDMAC_IR_REG;
	case ENETDMAC_IRMASK:
		return ENETDMAC_IRMASK_REG;
	case ENETDMAC_MAXBURST:
		return ENETDMAC_MAXBURST_REG;
	case ENETDMAC_BUFALLOC:
	case ENETDMAC_RSTART:
	case ENETDMAC_FC:
	case ENETDMAC_LEN:
		return 0;
	}
#endif
#endif
	return 0;
}


#endif /* ! BCM63XX_DEV_ENET_H_ */
