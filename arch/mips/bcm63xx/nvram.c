/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 2008 Maxime Bizon <mbizon@freebox.fr>
 * Copyright (C) 2008 Florian Fainelli <florian@openwrt.org>
 * Copyright (C) 2012 Jonas Gorski <jonas.gorski@gmail.com>
 */

#define pr_fmt(fmt) "bcm63xx_nvram: " fmt

#include <fikus/init.h>
#include <fikus/crc32.h>
#include <fikus/export.h>
#include <fikus/kernel.h>
#include <fikus/if_ether.h>

#include <bcm63xx_nvram.h>

/*
 * nvram structure
 */
struct bcm963xx_nvram {
	u32	version;
	u8	reserved1[256];
	u8	name[16];
	u32	main_tp_number;
	u32	psi_size;
	u32	mac_addr_count;
	u8	mac_addr_base[ETH_ALEN];
	u8	reserved2[2];
	u32	checksum_old;
	u8	reserved3[720];
	u32	checksum_high;
};

#define BCM63XX_DEFAULT_PSI_SIZE	64

static struct bcm963xx_nvram nvram;
static int mac_addr_used;

void __init bcm63xx_nvram_init(void *addr)
{
	unsigned int check_len;
	u32 crc, expected_crc;
	u8 hcs_mac_addr[ETH_ALEN] = { 0x00, 0x10, 0x18, 0xff, 0xff, 0xff };

	/* extract nvram data */
	memcpy(&nvram, addr, sizeof(nvram));

	/* check checksum before using data */
	if (nvram.version <= 4) {
		check_len = offsetof(struct bcm963xx_nvram, reserved3);
		expected_crc = nvram.checksum_old;
		nvram.checksum_old = 0;
	} else {
		check_len = sizeof(nvram);
		expected_crc = nvram.checksum_high;
		nvram.checksum_high = 0;
	}

	crc = crc32_le(~0, (u8 *)&nvram, check_len);

	if (crc != expected_crc)
		pr_warn("nvram checksum failed, contents may be invalid (expected %08x, got %08x)\n",
			expected_crc, crc);

	/* Cable modems have a different NVRAM which is embedded in the eCos
	 * firmware and not easily extractible, give at least a MAC address
	 * pool.
	 */
	if (BCMCPU_IS_3368()) {
		memcpy(nvram.mac_addr_base, hcs_mac_addr, ETH_ALEN);
		nvram.mac_addr_count = 2;
	}
}

u8 *bcm63xx_nvram_get_name(void)
{
	return nvram.name;
}
EXPORT_SYMBOL(bcm63xx_nvram_get_name);

int bcm63xx_nvram_get_mac_address(u8 *mac)
{
	u8 *oui;
	int count;

	if (mac_addr_used >= nvram.mac_addr_count) {
		pr_err("not enough mac addresses\n");
		return -ENODEV;
	}

	memcpy(mac, nvram.mac_addr_base, ETH_ALEN);
	oui = mac + ETH_ALEN/2 - 1;
	count = mac_addr_used;

	while (count--) {
		u8 *p = mac + ETH_ALEN - 1;

		do {
			(*p)++;
			if (*p != 0)
				break;
			p--;
		} while (p != oui);

		if (p == oui) {
			pr_err("unable to fetch mac address\n");
			return -ENODEV;
		}
	}

	mac_addr_used++;
	return 0;
}
EXPORT_SYMBOL(bcm63xx_nvram_get_mac_address);

int bcm63xx_nvram_get_psi_size(void)
{
	if (nvram.psi_size > 0)
		return nvram.psi_size;

	return BCM63XX_DEFAULT_PSI_SIZE;
}
EXPORT_SYMBOL(bcm63xx_nvram_get_psi_size);
