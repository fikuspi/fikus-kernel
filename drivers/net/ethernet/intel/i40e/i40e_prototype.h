/*******************************************************************************
 *
 * Intel Ethernet Controller XL710 Family Fikus Driver
 * Copyright(c) 2013 Intel Corporation.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * The full GNU General Public License is included in this distribution in
 * the file called "COPYING".
 *
 * Contact Information:
 * e1000-devel Mailing List <e1000-devel@lists.sourceforge.net>
 * Intel Corporation, 5200 N.E. Elam Young Parkway, Hillsboro, OR 97124-6497
 *
 ******************************************************************************/

#ifndef _I40E_PROTOTYPE_H_
#define _I40E_PROTOTYPE_H_

#include "i40e_type.h"
#include "i40e_alloc.h"
#include "i40e_virtchnl.h"

/* Prototypes for shared code functions that are not in
 * the standard function pointer structures.  These are
 * mostly because they are needed even before the init
 * has happened and will assist in the early SW and FW
 * setup.
 */

/* adminq functions */
i40e_status i40e_init_adminq(struct i40e_hw *hw);
i40e_status i40e_shutdown_adminq(struct i40e_hw *hw);
void i40e_adminq_init_ring_data(struct i40e_hw *hw);
i40e_status i40e_clean_arq_element(struct i40e_hw *hw,
					     struct i40e_arq_event_info *e,
					     u16 *events_pending);
i40e_status i40e_asq_send_command(struct i40e_hw *hw,
				struct i40e_aq_desc *desc,
				void *buff, /* can be NULL */
				u16  buff_size,
				struct i40e_asq_cmd_details *cmd_details);
bool i40e_asq_done(struct i40e_hw *hw);

/* debug function for adminq */
void i40e_debug_aq(struct i40e_hw *hw,
		   enum i40e_debug_mask mask,
		   void *desc,
		   void *buffer);

void i40e_idle_aq(struct i40e_hw *hw);
void i40e_resume_aq(struct i40e_hw *hw);

u32 i40e_led_get(struct i40e_hw *hw);
void i40e_led_set(struct i40e_hw *hw, u32 mode);

/* admin send queue commands */

i40e_status i40e_aq_get_firmware_version(struct i40e_hw *hw,
				u16 *fw_major_version, u16 *fw_minor_version,
				u16 *api_major_version, u16 *api_minor_version,
				struct i40e_asq_cmd_details *cmd_details);
i40e_status i40e_aq_queue_shutdown(struct i40e_hw *hw,
					     bool unloading);
i40e_status i40e_aq_set_phy_reset(struct i40e_hw *hw,
				struct i40e_asq_cmd_details *cmd_details);
i40e_status i40e_aq_set_default_vsi(struct i40e_hw *hw, u16 vsi_id,
				struct i40e_asq_cmd_details *cmd_details);
i40e_status i40e_aq_set_link_restart_an(struct i40e_hw *hw,
				struct i40e_asq_cmd_details *cmd_details);
i40e_status i40e_aq_get_link_info(struct i40e_hw *hw,
				bool enable_lse, struct i40e_link_status *link,
				struct i40e_asq_cmd_details *cmd_details);
i40e_status i40e_aq_set_local_advt_reg(struct i40e_hw *hw,
				u64 advt_reg,
				struct i40e_asq_cmd_details *cmd_details);
i40e_status i40e_aq_send_driver_version(struct i40e_hw *hw,
				struct i40e_driver_version *dv,
				struct i40e_asq_cmd_details *cmd_details);
i40e_status i40e_aq_add_vsi(struct i40e_hw *hw,
				struct i40e_vsi_context *vsi_ctx,
				struct i40e_asq_cmd_details *cmd_details);
i40e_status i40e_aq_set_vsi_broadcast(struct i40e_hw *hw,
				u16 vsi_id, bool set_filter,
				struct i40e_asq_cmd_details *cmd_details);
i40e_status i40e_aq_set_vsi_unicast_promiscuous(struct i40e_hw *hw,
				u16 vsi_id, bool set, struct i40e_asq_cmd_details *cmd_details);
i40e_status i40e_aq_set_vsi_multicast_promiscuous(struct i40e_hw *hw,
				u16 vsi_id, bool set, struct i40e_asq_cmd_details *cmd_details);
i40e_status i40e_aq_get_vsi_params(struct i40e_hw *hw,
				struct i40e_vsi_context *vsi_ctx,
				struct i40e_asq_cmd_details *cmd_details);
i40e_status i40e_aq_update_vsi_params(struct i40e_hw *hw,
				struct i40e_vsi_context *vsi_ctx,
				struct i40e_asq_cmd_details *cmd_details);
i40e_status i40e_aq_add_veb(struct i40e_hw *hw, u16 uplink_seid,
				u16 downlink_seid, u8 enabled_tc,
				bool default_port, u16 *pveb_seid,
				struct i40e_asq_cmd_details *cmd_details);
i40e_status i40e_aq_get_veb_parameters(struct i40e_hw *hw,
				u16 veb_seid, u16 *switch_id, bool *floating,
				u16 *statistic_index, u16 *vebs_used,
				u16 *vebs_free,
				struct i40e_asq_cmd_details *cmd_details);
i40e_status i40e_aq_add_macvlan(struct i40e_hw *hw, u16 vsi_id,
			struct i40e_aqc_add_macvlan_element_data *mv_list,
			u16 count, struct i40e_asq_cmd_details *cmd_details);
i40e_status i40e_aq_remove_macvlan(struct i40e_hw *hw, u16 vsi_id,
			struct i40e_aqc_remove_macvlan_element_data *mv_list,
			u16 count, struct i40e_asq_cmd_details *cmd_details);
i40e_status i40e_aq_add_vlan(struct i40e_hw *hw, u16 vsi_id,
			struct i40e_aqc_add_remove_vlan_element_data *v_list,
			u8 count, struct i40e_asq_cmd_details *cmd_details);
i40e_status i40e_aq_remove_vlan(struct i40e_hw *hw, u16 vsi_id,
			struct i40e_aqc_add_remove_vlan_element_data *v_list,
			u8 count, struct i40e_asq_cmd_details *cmd_details);
i40e_status i40e_aq_send_msg_to_vf(struct i40e_hw *hw, u16 vfid,
				u32 v_opcode, u32 v_retval, u8 *msg, u16 msglen,
				struct i40e_asq_cmd_details *cmd_details);
i40e_status i40e_aq_get_switch_config(struct i40e_hw *hw,
				struct i40e_aqc_get_switch_config_resp *buf,
				u16 buf_size, u16 *start_seid,
				struct i40e_asq_cmd_details *cmd_details);
i40e_status i40e_aq_request_resource(struct i40e_hw *hw,
				enum i40e_aq_resources_ids resource,
				enum i40e_aq_resource_access_type access,
				u8 sdp_number, u64 *timeout,
				struct i40e_asq_cmd_details *cmd_details);
i40e_status i40e_aq_release_resource(struct i40e_hw *hw,
				enum i40e_aq_resources_ids resource,
				u8 sdp_number,
				struct i40e_asq_cmd_details *cmd_details);
i40e_status i40e_aq_read_nvm(struct i40e_hw *hw, u8 module_pointer,
				u32 offset, u16 length, void *data,
				bool last_command,
				struct i40e_asq_cmd_details *cmd_details);
i40e_status i40e_aq_discover_capabilities(struct i40e_hw *hw,
				void *buff, u16 buff_size, u16 *data_size,
				enum i40e_admin_queue_opc list_type_opc,
				struct i40e_asq_cmd_details *cmd_details);
i40e_status i40e_aq_update_nvm(struct i40e_hw *hw, u8 module_pointer,
				u32 offset, u16 length, void *data,
				bool last_command,
				struct i40e_asq_cmd_details *cmd_details);
i40e_status i40e_aq_get_lldp_mib(struct i40e_hw *hw, u8 bridge_type,
				u8 mib_type, void *buff, u16 buff_size,
				u16 *local_len, u16 *remote_len,
				struct i40e_asq_cmd_details *cmd_details);
i40e_status i40e_aq_cfg_lldp_mib_change_event(struct i40e_hw *hw,
				bool enable_update,
				struct i40e_asq_cmd_details *cmd_details);
i40e_status i40e_aq_stop_lldp(struct i40e_hw *hw, bool shutdown_agent,
				struct i40e_asq_cmd_details *cmd_details);
i40e_status i40e_aq_start_lldp(struct i40e_hw *hw,
				struct i40e_asq_cmd_details *cmd_details);
i40e_status i40e_aq_delete_element(struct i40e_hw *hw, u16 seid,
				struct i40e_asq_cmd_details *cmd_details);
i40e_status i40e_aq_mac_address_write(struct i40e_hw *hw,
				    u16 flags, u8 *mac_addr,
				    struct i40e_asq_cmd_details *cmd_details);
i40e_status i40e_aq_set_hmc_resource_profile(struct i40e_hw *hw,
				enum i40e_aq_hmc_profile profile,
				u8 pe_vf_enabled_count,
				struct i40e_asq_cmd_details *cmd_details);
i40e_status i40e_aq_config_switch_comp_bw_limit(struct i40e_hw *hw,
				u16 seid, u16 credit, u8 max_bw,
				struct i40e_asq_cmd_details *cmd_details);
i40e_status i40e_aq_config_vsi_tc_bw(struct i40e_hw *hw, u16 seid,
			struct i40e_aqc_configure_vsi_tc_bw_data *bw_data,
			struct i40e_asq_cmd_details *cmd_details);
i40e_status i40e_aq_query_vsi_bw_config(struct i40e_hw *hw,
			u16 seid,
			struct i40e_aqc_query_vsi_bw_config_resp *bw_data,
			struct i40e_asq_cmd_details *cmd_details);
i40e_status i40e_aq_query_vsi_ets_sla_config(struct i40e_hw *hw,
			u16 seid,
			struct i40e_aqc_query_vsi_ets_sla_config_resp *bw_data,
			struct i40e_asq_cmd_details *cmd_details);
i40e_status i40e_aq_query_switch_comp_ets_config(struct i40e_hw *hw,
		u16 seid,
		struct i40e_aqc_query_switching_comp_ets_config_resp *bw_data,
		struct i40e_asq_cmd_details *cmd_details);
i40e_status i40e_aq_query_port_ets_config(struct i40e_hw *hw,
		u16 seid,
		struct i40e_aqc_query_port_ets_config_resp *bw_data,
		struct i40e_asq_cmd_details *cmd_details);
i40e_status i40e_aq_query_switch_comp_bw_config(struct i40e_hw *hw,
		u16 seid,
		struct i40e_aqc_query_switching_comp_bw_config_resp *bw_data,
		struct i40e_asq_cmd_details *cmd_details);
/* i40e_common */
i40e_status i40e_init_shared_code(struct i40e_hw *hw);
i40e_status i40e_pf_reset(struct i40e_hw *hw);
void i40e_clear_pxe_mode(struct i40e_hw *hw);
bool i40e_get_link_status(struct i40e_hw *hw);
i40e_status i40e_get_mac_addr(struct i40e_hw *hw,
						u8 *mac_addr);
i40e_status i40e_validate_mac_addr(u8 *mac_addr);
i40e_status i40e_read_lldp_cfg(struct i40e_hw *hw,
					struct i40e_lldp_variables *lldp_cfg);
/* prototype for functions used for NVM access */
i40e_status i40e_init_nvm(struct i40e_hw *hw);
i40e_status i40e_acquire_nvm(struct i40e_hw *hw,
				      enum i40e_aq_resource_access_type access);
void i40e_release_nvm(struct i40e_hw *hw);
i40e_status i40e_read_nvm_srrd(struct i40e_hw *hw, u16 offset,
					 u16 *data);
i40e_status i40e_read_nvm_word(struct i40e_hw *hw, u16 offset,
					 u16 *data);
i40e_status i40e_read_nvm_buffer(struct i40e_hw *hw, u16 offset,
					   u16 *words, u16 *data);
i40e_status i40e_validate_nvm_checksum(struct i40e_hw *hw,
						 u16 *checksum);

/* prototype for functions used for SW locks */

/* i40e_common for VF drivers*/
void i40e_vf_parse_hw_config(struct i40e_hw *hw,
			     struct i40e_virtchnl_vf_resource *msg);
i40e_status i40e_vf_reset(struct i40e_hw *hw);
i40e_status i40e_aq_send_msg_to_pf(struct i40e_hw *hw,
				enum i40e_virtchnl_ops v_opcode,
				i40e_status v_retval,
				u8 *msg, u16 msglen,
				struct i40e_asq_cmd_details *cmd_details);
i40e_status i40e_set_filter_control(struct i40e_hw *hw,
				struct i40e_filter_control_settings *settings);
#endif /* _I40E_PROTOTYPE_H_ */
