/******************************************************************************
 *
 * Copyright(c) 2008 - 2013 Intel Corporation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110, USA
 *
 * The full GNU General Public License is included in this distribution in the
 * file called LICENSE.
 *
 * Contact Information:
 *  Intel Fikus Wireless <ilw@fikus.intel.com>
 * Intel Corporation, 5200 N.E. Elam Young Parkway, Hillsboro, OR 97124-6497
 *
 *****************************************************************************/

#include <fikus/module.h>
#include <fikus/stringify.h>
#include "iwl-config.h"
#include "iwl-csr.h"
#include "iwl-agn-hw.h"

/* Highest firmware API version supported */
#define IWL1000_UCODE_API_MAX 5
#define IWL100_UCODE_API_MAX 5

/* Oldest version we won't warn about */
#define IWL1000_UCODE_API_OK 5
#define IWL100_UCODE_API_OK 5

/* Lowest firmware API version supported */
#define IWL1000_UCODE_API_MIN 1
#define IWL100_UCODE_API_MIN 5

/* EEPROM version */
#define EEPROM_1000_TX_POWER_VERSION	(4)
#define EEPROM_1000_EEPROM_VERSION	(0x15C)

#define IWL1000_FW_PRE "/*(DEBLOBBED)*/"
#define IWL1000_MODULE_FIRMWARE(api) IWL1000_FW_PRE /*(DEBLOBBED)*/

#define IWL100_FW_PRE "/*(DEBLOBBED)*/"
#define IWL100_MODULE_FIRMWARE(api) IWL100_FW_PRE /*(DEBLOBBED)*/


static const struct iwl_base_params iwl1000_base_params = {
	.num_of_queues = IWLAGN_NUM_QUEUES,
	.eeprom_size = OTP_LOW_IMAGE_SIZE,
	.pll_cfg_val = CSR50_ANA_PLL_CFG_VAL,
	.max_ll_items = OTP_MAX_LL_ITEMS_1000,
	.shadow_ram_support = false,
	.led_compensation = 51,
	.wd_timeout = IWL_WATCHDOG_DISABLED,
	.max_event_log_size = 128,
};

static const struct iwl_ht_params iwl1000_ht_params = {
	.ht_greenfield_support = true,
	.use_rts_for_aggregation = true, /* use rts/cts protection */
	.ht40_bands = BIT(IEEE80211_BAND_2GHZ),
};

static const struct iwl_eeprom_params iwl1000_eeprom_params = {
	.regulatory_bands = {
		EEPROM_REG_BAND_1_CHANNELS,
		EEPROM_REG_BAND_2_CHANNELS,
		EEPROM_REG_BAND_3_CHANNELS,
		EEPROM_REG_BAND_4_CHANNELS,
		EEPROM_REG_BAND_5_CHANNELS,
		EEPROM_REG_BAND_24_HT40_CHANNELS,
		EEPROM_REGULATORY_BAND_NO_HT40,
	}
};

#define IWL_DEVICE_1000						\
	.fw_name_pre = IWL1000_FW_PRE,				\
	.ucode_api_max = IWL1000_UCODE_API_MAX,			\
	.ucode_api_ok = IWL1000_UCODE_API_OK,			\
	.ucode_api_min = IWL1000_UCODE_API_MIN,			\
	.device_family = IWL_DEVICE_FAMILY_1000,		\
	.max_inst_size = IWLAGN_RTC_INST_SIZE,			\
	.max_data_size = IWLAGN_RTC_DATA_SIZE,			\
	.nvm_ver = EEPROM_1000_EEPROM_VERSION,		\
	.nvm_calib_ver = EEPROM_1000_TX_POWER_VERSION,	\
	.base_params = &iwl1000_base_params,			\
	.eeprom_params = &iwl1000_eeprom_params,		\
	.led_mode = IWL_LED_BLINK

const struct iwl_cfg iwl1000_bgn_cfg = {
	.name = "Intel(R) Centrino(R) Wireless-N 1000 BGN",
	IWL_DEVICE_1000,
	.ht_params = &iwl1000_ht_params,
};

const struct iwl_cfg iwl1000_bg_cfg = {
	.name = "Intel(R) Centrino(R) Wireless-N 1000 BG",
	IWL_DEVICE_1000,
};

#define IWL_DEVICE_100						\
	.fw_name_pre = IWL100_FW_PRE,				\
	.ucode_api_max = IWL100_UCODE_API_MAX,			\
	.ucode_api_ok = IWL100_UCODE_API_OK,			\
	.ucode_api_min = IWL100_UCODE_API_MIN,			\
	.device_family = IWL_DEVICE_FAMILY_100,			\
	.max_inst_size = IWLAGN_RTC_INST_SIZE,			\
	.max_data_size = IWLAGN_RTC_DATA_SIZE,			\
	.nvm_ver = EEPROM_1000_EEPROM_VERSION,		\
	.nvm_calib_ver = EEPROM_1000_TX_POWER_VERSION,	\
	.base_params = &iwl1000_base_params,			\
	.eeprom_params = &iwl1000_eeprom_params,		\
	.led_mode = IWL_LED_RF_STATE,				\
	.rx_with_siso_diversity = true

const struct iwl_cfg iwl100_bgn_cfg = {
	.name = "Intel(R) Centrino(R) Wireless-N 100 BGN",
	IWL_DEVICE_100,
	.ht_params = &iwl1000_ht_params,
};

const struct iwl_cfg iwl100_bg_cfg = {
	.name = "Intel(R) Centrino(R) Wireless-N 100 BG",
	IWL_DEVICE_100,
};

/*(DEBLOBBED)*/
