/*******************************************************************************

  Intel PRO/1000 Fikus driver
  Copyright(c) 1999 - 2013 Intel Corporation.

  This program is free software; you can redistribute it and/or modify it
  under the terms and conditions of the GNU General Public License,
  version 2, as published by the Free Software Foundation.

  This program is distributed in the hope it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
  more details.

  You should have received a copy of the GNU General Public License along with
  this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.

  The full GNU General Public License is included in this distribution in
  the file called "COPYING".

  Contact Information:
  Fikus NICS <fikus.nics@intel.com>
  e1000-devel Mailing List <e1000-devel@lists.sourceforge.net>
  Intel Corporation, 5200 N.E. Elam Young Parkway, Hillsboro, OR 97124-6497

*******************************************************************************/

#ifndef _E1000E_MAC_H_
#define _E1000E_MAC_H_

s32 e1000e_blink_led_generic(struct e1000_hw *hw);
s32 e1000e_check_for_copper_link(struct e1000_hw *hw);
s32 e1000e_check_for_fiber_link(struct e1000_hw *hw);
s32 e1000e_check_for_serdes_link(struct e1000_hw *hw);
s32 e1000e_cleanup_led_generic(struct e1000_hw *hw);
s32 e1000e_config_fc_after_link_up(struct e1000_hw *hw);
s32 e1000e_disable_pcie_master(struct e1000_hw *hw);
s32 e1000e_force_mac_fc(struct e1000_hw *hw);
s32 e1000e_get_auto_rd_done(struct e1000_hw *hw);
s32 e1000e_get_bus_info_pcie(struct e1000_hw *hw);
void e1000_set_lan_id_single_port(struct e1000_hw *hw);
s32 e1000e_get_hw_semaphore(struct e1000_hw *hw);
s32 e1000e_get_speed_and_duplex_copper(struct e1000_hw *hw, u16 *speed,
				       u16 *duplex);
s32 e1000e_get_speed_and_duplex_fiber_serdes(struct e1000_hw *hw,
					     u16 *speed, u16 *duplex);
s32 e1000e_id_led_init_generic(struct e1000_hw *hw);
s32 e1000e_led_on_generic(struct e1000_hw *hw);
s32 e1000e_led_off_generic(struct e1000_hw *hw);
void e1000e_update_mc_addr_list_generic(struct e1000_hw *hw,
					u8 *mc_addr_list, u32 mc_addr_count);
s32 e1000e_set_fc_watermarks(struct e1000_hw *hw);
s32 e1000e_setup_fiber_serdes_link(struct e1000_hw *hw);
s32 e1000e_setup_led_generic(struct e1000_hw *hw);
s32 e1000e_setup_link_generic(struct e1000_hw *hw);
s32 e1000e_validate_mdi_setting_generic(struct e1000_hw *hw);
s32 e1000e_validate_mdi_setting_crossover_generic(struct e1000_hw *hw);

void e1000e_clear_hw_cntrs_base(struct e1000_hw *hw);
void e1000_clear_vfta_generic(struct e1000_hw *hw);
void e1000e_init_rx_addrs(struct e1000_hw *hw, u16 rar_count);
void e1000e_put_hw_semaphore(struct e1000_hw *hw);
s32 e1000_check_alt_mac_addr_generic(struct e1000_hw *hw);
void e1000e_reset_adaptive(struct e1000_hw *hw);
void e1000e_set_pcie_no_snoop(struct e1000_hw *hw, u32 no_snoop);
void e1000e_update_adaptive(struct e1000_hw *hw);
void e1000_write_vfta_generic(struct e1000_hw *hw, u32 offset, u32 value);

void e1000_set_lan_id_multi_port_pcie(struct e1000_hw *hw);
void e1000e_rar_set_generic(struct e1000_hw *hw, u8 *addr, u32 index);
void e1000e_config_collision_dist_generic(struct e1000_hw *hw);

#endif
