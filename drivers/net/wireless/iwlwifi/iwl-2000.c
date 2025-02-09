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
#include "iwl-agn-hw.h"
#include "dvm/commands.h" /* needed for BT for now */

/* Highest firmware API version supported */
#define IWL2030_UCODE_API_MAX 6
#define IWL2000_UCODE_API_MAX 6
#define IWL105_UCODE_API_MAX 6
#define IWL135_UCODE_API_MAX 6

/* Oldest version we won't warn about */
#define IWL2030_UCODE_API_OK 6
#define IWL2000_UCODE_API_OK 6
#define IWL105_UCODE_API_OK 6
#define IWL135_UCODE_API_OK 6

/* Lowest firmware API version supported */
#define IWL2030_UCODE_API_MIN 5
#define IWL2000_UCODE_API_MIN 5
#define IWL105_UCODE_API_MIN 5
#define IWL135_UCODE_API_MIN 5

/* EEPROM version */
#define EEPROM_2000_TX_POWER_VERSION	(6)
#define EEPROM_2000_EEPROM_VERSION	(0x805)


#define IWL2030_FW_PRE "/*(DEBLOBBED)*/"
#define IWL2030_MODULE_FIRMWARE(api) IWL2030_FW_PRE /*(DEBLOBBED)*/

#define IWL2000_FW_PRE "/*(DEBLOBBED)*/"
#define IWL2000_MODULE_FIRMWARE(api) IWL2000_FW_PRE /*(DEBLOBBED)*/

#define IWL105_FW_PRE "/*(DEBLOBBED)*/"
#define IWL105_MODULE_FIRMWARE(api) IWL105_FW_PRE /*(DEBLOBBED)*/

#define IWL135_FW_PRE "/*(DEBLOBBED)*/"
#define IWL135_MODULE_FIRMWARE(api) IWL135_FW_PRE /*(DEBLOBBED)*/

static const struct iwl_base_params iwl2000_base_params = {
	.eeprom_size = OTP_LOW_IMAGE_SIZE,
	.num_of_queues = IWLAGN_NUM_QUEUES,
	.pll_cfg_val = 0,
	.max_ll_items = OTP_MAX_LL_ITEMS_2x00,
	.shadow_ram_support = true,
	.led_compensation = 51,
	.wd_timeout = IWL_DEF_WD_TIMEOUT,
	.max_event_log_size = 512,
	.shadow_reg_enable = false, /* TODO: fix bugs using this feature */
};


static const struct iwl_base_params iwl2030_base_params = {
	.eeprom_size = OTP_LOW_IMAGE_SIZE,
	.num_of_queues = IWLAGN_NUM_QUEUES,
	.pll_cfg_val = 0,
	.max_ll_items = OTP_MAX_LL_ITEMS_2x00,
	.shadow_ram_support = true,
	.led_compensation = 57,
	.wd_timeout = IWL_LONG_WD_TIMEOUT,
	.max_event_log_size = 512,
	.shadow_reg_enable = false, /* TODO: fix bugs using this feature */
};

static const struct iwl_ht_params iwl2000_ht_params = {
	.ht_greenfield_support = true,
	.use_rts_for_aggregation = true, /* use rts/cts protection */
	.ht40_bands = BIT(IEEE80211_BAND_2GHZ),
};

static const struct iwl_eeprom_params iwl20x0_eeprom_params = {
	.regulatory_bands = {
		EEPROM_REG_BAND_1_CHANNELS,
		EEPROM_REG_BAND_2_CHANNELS,
		EEPROM_REG_BAND_3_CHANNELS,
		EEPROM_REG_BAND_4_CHANNELS,
		EEPROM_REG_BAND_5_CHANNELS,
		EEPROM_6000_REG_BAND_24_HT40_CHANNELS,
		EEPROM_REGULATORY_BAND_NO_HT40,
	},
	.enhanced_txpower = true,
};

#define IWL_DEVICE_2000						\
	.fw_name_pre = IWL2000_FW_PRE,				\
	.ucode_api_max = IWL2000_UCODE_API_MAX,			\
	.ucode_api_ok = IWL2000_UCODE_API_OK,			\
	.ucode_api_min = IWL2000_UCODE_API_MIN,			\
	.device_family = IWL_DEVICE_FAMILY_2000,		\
	.max_inst_size = IWL60_RTC_INST_SIZE,			\
	.max_data_size = IWL60_RTC_DATA_SIZE,			\
	.nvm_ver = EEPROM_2000_EEPROM_VERSION,			\
	.nvm_calib_ver = EEPROM_2000_TX_POWER_VERSION,		\
	.base_params = &iwl2000_base_params,			\
	.eeprom_params = &iwl20x0_eeprom_params,		\
	.led_mode = IWL_LED_RF_STATE

const struct iwl_cfg iwl2000_2bgn_cfg = {
	.name = "Intel(R) Centrino(R) Wireless-N 2200 BGN",
	IWL_DEVICE_2000,
	.ht_params = &iwl2000_ht_params,
};

const struct iwl_cfg iwl2000_2bgn_d_cfg = {
	.name = "Intel(R) Centrino(R) Wireless-N 2200D BGN",
	IWL_DEVICE_2000,
	.ht_params = &iwl2000_ht_params,
};

#define IWL_DEVICE_2030						\
	.fw_name_pre = IWL2030_FW_PRE,				\
	.ucode_api_max = IWL2030_UCODE_API_MAX,			\
	.ucode_api_ok = IWL2030_UCODE_API_OK,			\
	.ucode_api_min = IWL2030_UCODE_API_MIN,			\
	.device_family = IWL_DEVICE_FAMILY_2030,		\
	.max_inst_size = IWL60_RTC_INST_SIZE,			\
	.max_data_size = IWL60_RTC_DATA_SIZE,			\
	.nvm_ver = EEPROM_2000_EEPROM_VERSION,		\
	.nvm_calib_ver = EEPROM_2000_TX_POWER_VERSION,	\
	.base_params = &iwl2030_base_params,			\
	.eeprom_params = &iwl20x0_eeprom_params,		\
	.led_mode = IWL_LED_RF_STATE

const struct iwl_cfg iwl2030_2bgn_cfg = {
	.name = "Intel(R) Centrino(R) Wireless-N 2230 BGN",
	IWL_DEVICE_2030,
	.ht_params = &iwl2000_ht_params,
};

#define IWL_DEVICE_105						\
	.fw_name_pre = IWL105_FW_PRE,				\
	.ucode_api_max = IWL105_UCODE_API_MAX,			\
	.ucode_api_ok = IWL105_UCODE_API_OK,			\
	.ucode_api_min = IWL105_UCODE_API_MIN,			\
	.device_family = IWL_DEVICE_FAMILY_105,			\
	.max_inst_size = IWL60_RTC_INST_SIZE,			\
	.max_data_size = IWL60_RTC_DATA_SIZE,			\
	.nvm_ver = EEPROM_2000_EEPROM_VERSION,		\
	.nvm_calib_ver = EEPROM_2000_TX_POWER_VERSION,	\
	.base_params = &iwl2000_base_params,			\
	.eeprom_params = &iwl20x0_eeprom_params,		\
	.led_mode = IWL_LED_RF_STATE,				\
	.rx_with_siso_diversity = true

const struct iwl_cfg iwl105_bgn_cfg = {
	.name = "Intel(R) Centrino(R) Wireless-N 105 BGN",
	IWL_DEVICE_105,
	.ht_params = &iwl2000_ht_params,
};

const struct iwl_cfg iwl105_bgn_d_cfg = {
	.name = "Intel(R) Centrino(R) Wireless-N 105D BGN",
	IWL_DEVICE_105,
	.ht_params = &iwl2000_ht_params,
};

#define IWL_DEVICE_135						\
	.fw_name_pre = IWL135_FW_PRE,				\
	.ucode_api_max = IWL135_UCODE_API_MAX,			\
	.ucode_api_ok = IWL135_UCODE_API_OK,			\
	.ucode_api_min = IWL135_UCODE_API_MIN,			\
	.device_family = IWL_DEVICE_FAMILY_135,			\
	.max_inst_size = IWL60_RTC_INST_SIZE,			\
	.max_data_size = IWL60_RTC_DATA_SIZE,			\
	.nvm_ver = EEPROM_2000_EEPROM_VERSION,		\
	.nvm_calib_ver = EEPROM_2000_TX_POWER_VERSION,	\
	.base_params = &iwl2030_base_params,			\
	.eeprom_params = &iwl20x0_eeprom_params,		\
	.led_mode = IWL_LED_RF_STATE,				\
	.rx_with_siso_diversity = true

const struct iwl_cfg iwl135_bgn_cfg = {
	.name = "Intel(R) Centrino(R) Wireless-N 135 BGN",
	IWL_DEVICE_135,
	.ht_params = &iwl2000_ht_params,
};

/*(DEBLOBBED)*/
