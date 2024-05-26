/*
 * slimbus is a kernel driver which is used to manager slimbus devices
 *
 * Copyright (c) 2012-2018 Huawei Technologies Co., Ltd.
 *
 * slimbus Core Driver for SLIMbus Manager Interface
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */

/**
 * This file contains sanity API functions. The purpose of sanity functions
 * is to check input parameters validity. They take the same parameters as
 * original API functions and return 0 on success or EINVAL on wrong parameter
 * value(s).
 */

#ifndef __SLIMBUS_DRV_SANITY_H__
#define __SLIMBUS_DRV_SANITY_H__

#include "slimbus_drv_api.h"
#include "slimbus_drv_stdtypes.h"
#include "slimbus_drv_errno.h"

/**
 * Checks validity of parameters for function probe
 * @param[out] required_memory Required memory in bytes.
 * @param[in] config Driver and hardware configuration.
 * @return 0 success
 * @return EINVAL invalid parameters
 */
static inline int slimbus_probe_sanity(const slimbus_drv_config* config, uint16_t* required_memory)
{
	if (config == NULL)
		return EINVAL;
	if (required_memory == NULL)
		return EINVAL;

	return 0;
}

/**
 * Checks validity of parameters for function init
 * @param[in] callbacks Event Handlers and Callbacks.
 * @param[in] config Specifies driver/hardware configuration.
 * @param[in] pd Pointer to driver's private data object.
 * @return 0 success
 * @return EINVAL invalid parameters
 */
static inline int slimbus_init_sanity(void* pd, const slimbus_drv_config* config, slimbus_drv_callbacks* callbacks)
{
	if (pd == NULL)
		return EINVAL;
	if (config == NULL)
		return EINVAL;
	if (callbacks == NULL)
		return EINVAL;

	return 0;
}

/**
 * Checks validity of parameters for function isr
 * @param[in] pd Pointer to driver's private data object filled by init.
 * @return 0 success
 * @return EINVAL invalid parameters
 */
static inline int slimbus_isr_sanity(void* pd)
{
	if (pd == NULL)
		return EINVAL;

	return 0;
}

/**
 * Checks validity of parameters for function start
 * @param[in] pd Pointer to driver's private data object.
 * @return 0 success
 * @return EINVAL invalid parameters
 */
static inline int slimbus_start_sanity(void* pd)
{
	if (pd == NULL)
		return EINVAL;

	return 0;
}

/**
 * Checks validity of parameters for function stop
 * @param[in] pd Pointer to driver's private data object.
 * @return 0 success
 * @return EINVAL invalid parameters
 */
static inline int slimbus_stop_sanity(void* pd)
{
	if (pd == NULL)
		return EINVAL;

	return 0;
}

/**
 * Checks validity of parameters for function destroy
 * @param[in] pd Pointer to driver's private data object.
 * @return 0 success
 * @return EINVAL invalid parameters
 */
static inline int slimbus_destroy_sanity(void* pd)
{
	if (pd == NULL)
		return EINVAL;

	return 0;
}

/**
 * Checks validity of parameters for function set_interrupts
 * @param[in] interrupt_mask SLIMbus Manager interrupt mask.
 * @param[in] pd Pointer to driver's private data object.
 * @return 0 success
 * @return EINVAL invalid parameters
 */
static inline int slimbus_set_interrupts_sanity(void* pd, uint8_t interrupt_mask)
{
	if (pd == NULL)
		return EINVAL;

	return 0;
}

/**
 * Checks validity of parameters for function get_interrupts
 * @param[out] interrupt_mask Pointer to place where SLIMbus Manager interrupt mask will be written.
 * @param[in] pd Pointer to driver's private data object.
 * @return 0 success
 * @return EINVAL invalid parameters
 */
static inline int slimbus_get_interrupts_sanity(void* pd, uint8_t* interrupt_mask)
{
	if (pd == NULL)
		return EINVAL;
	if (interrupt_mask == NULL)
		return EINVAL;

	return 0;
}

/**
 * Checks validity of parameters for function set_data_port_interrupts
 * @param[in] interrupt_mask Data Port interrupt mask.
 * @param[in] port_number Data Port number. Valid value ranges from 0 to 63.
 * @param[in] pd Pointer to driver's private data object.
 * @return 0 success
 * @return EINVAL invalid parameters
 */
static inline int slimbus_set_data_port_interrupts_sanity(void* pd, uint8_t port_number, uint8_t interrupt_mask)
{
	if (pd == NULL)
		return EINVAL;

	if (port_number > (63))
		return EINVAL;


	if (interrupt_mask > (63))
		return EINVAL;

	return 0;
}
/**
 * Checks validity of parameters for function get_data_port_interrupts
 * @param[out] interrupt_mask Pointer to place where Data Port interrupt mask will be written.
 * @param[in] port_number Data Port number. Valid value ranges from 0 to 63.
 * @param[in] pd Pointer to driver's private data object.
 * @return 0 success
 * @return EINVAL invalid parameters
 */
static inline int slimbus_get_data_port_interrupts_sanity(void* pd, uint8_t port_number, uint8_t* interrupt_mask)
{
	if (pd == NULL)
		return EINVAL;
	if (interrupt_mask == NULL)
		return EINVAL;

	if (port_number > (63))
		return EINVAL;

	return 0;
}

/**
 * Checks validity of parameters for function clear_data_port_fifo
 * @param[in] port_number Data Port number. Valid value ranges from 0 to 63.
 * @param[in] pd Pointer to driver's private data object.
 * @return 0 success
 * @return EINVAL invalid parameters
 */
static inline int slimbus_clear_data_port_fifo_sanity(void* pd, uint8_t port_number)
{
	if (pd == NULL)
		return EINVAL;

	if (port_number > (63))
		return EINVAL;

	return 0;
}
/**
 * Checks validity of parameters for function
 * set_presence_rate_generation
 * @param[in] enable If true Presence Rate Generation for specified Data Port will be enabled; if not then it will be disabled.
 * @param[in] port_number Data Port number. Valid value ranges from 0 to 63.
 * @param[in] pd Pointer to driver's private data object.
 * @return 0 success
 * @return EINVAL invalid parameters
 */
static inline int slimbus_set_presence_rate_generation_sanity(void* pd, uint8_t port_number, bool enable)
{
	if (pd == NULL)
		return EINVAL;

	if (port_number > (63))
		return EINVAL;

	return 0;
}
/**
 * Checks validity of parameters for function
 * get_presence_rate_generation
 * @param[out] enable Pointer to place where information about Presence Rate Generation will be written.
 * @param[in] port_number Data Port number. Valid value ranges from 0 to 63.
 * @param[in] pd Pointer to driver's private data object.
 * @return 0 success
 * @return EINVAL invalid parameters
 */
static inline int slimbus_get_presence_rate_generation_sanity(void* pd, uint8_t port_number, bool* enable)
{
	if (pd == NULL)
		return EINVAL;
	if (enable == NULL)
		return EINVAL;

	if (port_number > (63))
		return EINVAL;

	return 0;
}
/**
 * Checks validity of parameters for function assign_message_callbacks
 * @param[in] msg_callbacks SLIMbus Messages Callbacks.
 * @param[in] pd Pointer to driver's private data object.
 * @return 0 success
 * @return EINVAL invalid parameters
 */
static inline int slimbus_assign_message_callbacks_sanity(void* pd, slimbus_message_callbacks* msg_callbacks)
{
	if (pd == NULL)
		return EINVAL;
	if (msg_callbacks == NULL)
		return EINVAL;

	return 0;
}
/**
 * Checks validity of parameters for function send_raw_message
 * @param[in] message_length Raw message length.
 * @param[in] message Pointer to raw message.
 * @param[in] pd Pointer to driver's private data object.
 * @return 0 success
 * @return EINVAL invalid parameters
 */
static inline int slimbus_send_raw_message_sanity(void* pd, void* message, uint8_t message_length)
{
	if (pd == NULL)
		return EINVAL;
	if (message == NULL)
		return EINVAL;

	if (message_length < (MESSAGE_MIN_LENGTH) || message_length > (MESSAGE_MAX_LENGTH))
		return EINVAL;

	return 0;
}
/**
 * Checks validity of parameters for function send_message
 * @param[in] message Pointer to struct message.
 * @param[in] pd Pointer to driver's private data object.
 * @return 0 success
 * @return EINVAL invalid parameters
 */
static inline int slimbus_send_message_sanity(void* pd, slimbus_drv_message* message)
{
	if (pd == NULL)
		return EINVAL;
	if (message == NULL)
		return EINVAL;

	return 0;
}
/**
 * Checks validity of parameters for function get_register_value
 * @param[out] reg_content Register data output.
 * @param[in] reg_address Register address.
 * @param[in] pd Pointer to driver's private data object.
 * @return 0 success
 * @return EINVAL invalid parameters
 */
static inline int slimbus_get_register_value_sanity(void* pd, uint16_t reg_address, uint32_t* reg_content)
{
	if (pd == NULL)
		return EINVAL;
	if (reg_content == NULL)
		return EINVAL;

	return 0;
}
/**
 * Checks validity of parameters for function set_register_value
 * @param[in] reg_content Register data input.
 * @param[in] reg_address Register address.
 * @param[in] pd Pointer to driver's private data object.
 * @return 0 success
 * @return EINVAL invalid parameters
 */
static inline int slimbus_set_register_value_sanity(void* pd, uint16_t reg_address, uint32_t reg_content)
{
	if (pd == NULL)
		return EINVAL;

	return 0;
}
/**
 * Checks validity of parameters for function set_message_channel_lapse
 * @param[in] mch_lapse New Message Channel Lapse value
 * @param[in] pd Pointer to driver's private data object.
 * @return 0 success
 * @return EINVAL invalid parameters
 */
static inline int slimbus_set_message_channel_lapse_sanity(void* pd, uint8_t mch_lapse)
{
	if (pd == NULL)
		return EINVAL;

	if (mch_lapse > (15))
		return EINVAL;

	return 0;
}
/**
 * Checks validity of parameters for function get_message_channel_lapse
 * @param[out] mch_lapse Current Message Channel Lapse value
 * @param[in] pd Pointer to driver's private data object.
 * @return 0 success
 * @return EINVAL invalid parameters
 */
static inline int slimbus_get_message_channel_lapse_sanity(void* pd, uint8_t* mch_lapse)
{
	if (pd == NULL)
		return EINVAL;
	if (mch_lapse == NULL)
		return EINVAL;

	return 0;
}
/**
 * Checks validity of parameters for function get_message_channel_usage
 * @param[out] mch_usage Current Message Channel Usage
 * @param[in] pd Pointer to driver's private data object.
 * @return 0 success
 * @return EINVAL invalid parameters
 */
static inline int slimbus_get_message_channel_usage_sanity(void* pd, uint16_t* mch_usage)
{
	if (pd == NULL)
		return EINVAL;
	if (mch_usage == NULL)
		return EINVAL;

	return 0;
}
/**
 * Checks validity of parameters for function
 * get_message_channel_capacity
 * @param[out] mch_capacity Current Message Channel Capacity
 * @param[in] pd Pointer to driver's private data object.
 * @return 0 success
 * @return EINVAL invalid parameters
 */
static inline int slimbus_get_message_channel_capacity_sanity(void* pd, uint16_t* mch_capacity)
{
	if (pd == NULL)
		return EINVAL;
	if (mch_capacity == NULL)
		return EINVAL;

	return 0;
}
/**
 * Checks validity of parameters for function set_sniffer_mode
 * @param[in] state New state
 * @param[in] pd Pointer to driver's private data object.
 * @return 0 success
 * @return EINVAL invalid parameters
 */
static inline int slimbus_set_sniffer_mode_sanity(void* pd, bool state)
{
	if (pd == NULL)
		return EINVAL;

	return 0;
}
/**
 * Checks validity of parameters for function get_sniffer_mode
 * @param[out] state Current state
 * @param[in] pd Pointer to driver's private data object.
 * @return 0 success
 * @return EINVAL invalid parameters
 */
static inline int slimbus_get_sniffer_mode_sanity(void* pd, bool* state)
{
	if (pd == NULL)
		return EINVAL;
	if (state == NULL)
		return EINVAL;

	return 0;
}
/**
 * Checks validity of parameters for function set_framer_enabled
 * @param[in] state New state
 * @param[in] pd Pointer to driver's private data object.
 * @return 0 success
 * @return EINVAL invalid parameters
 */
static inline int slimbus_set_framer_enabled_sanity(void* pd, bool state)
{
	if (pd == NULL)
		return EINVAL;

	return 0;
}
/**
 * Checks validity of parameters for function get_framer_enabled
 * @param[out] state Current state
 * @param[in] pd Pointer to driver's private data object.
 * @return 0 success
 * @return EINVAL invalid parameters
 */
static inline int slimbus_get_framer_enabled_sanity(void* pd, bool* state)
{
	if (pd == NULL)
		return EINVAL;
	if (state == NULL)
		return EINVAL;

	return 0;
}
/**
 * Checks validity of parameters for function set_device_enabled
 * @param[in] state New state
 * @param[in] pd Pointer to driver's private data object.
 * @return 0 success
 * @return EINVAL invalid parameters
 */
static inline int slimbus_set_device_enabled_sanity(void* pd, bool state)
{
	if (pd == NULL)
		return EINVAL;

	return 0;
}
/**
 * Checks validity of parameters for function get_device_enabled
 * @param[out] state Current state
 * @param[in] pd Pointer to driver's private data object.
 * @return 0 success
 * @return EINVAL invalid parameters
 */
static inline int slimbus_get_device_enabled_sanity(void* pd, bool* state)
{
	if (pd == NULL)
		return EINVAL;
	if (state == NULL)
		return EINVAL;

	return 0;
}
/**
 * Checks validity of parameters for function set_go_absent
 * @param[in] state New state
 * @param[in] pd Pointer to driver's private data object.
 * @return 0 success
 * @return EINVAL invalid parameters
 */
static inline int slimbus_set_go_absent_sanity(void* pd, bool state)
{
	if (pd == NULL)
		return EINVAL;

	return 0;
}
/**
 * Checks validity of parameters for function get_go_absent
 * @param[out] state Current state
 * @param[in] pd Pointer to driver's private data object.
 * @return 0 success
 * @return EINVAL invalid parameters
 */
static inline int slimbus_get_go_absent_sanity(void* pd, bool* state)
{
	if (pd == NULL)
		return EINVAL;
	if (state == NULL)
		return EINVAL;

	return 0;
}
/**
 * Checks validity of parameters for function set_framer_config
 * @param[in] framer_config Pointer to structure containing Framer configuration.
 * @param[in] pd Pointer to driver's private data.
 * @return 0 success
 * @return EINVAL invalid parameters
 */
static inline int slimbus_set_framer_config_sanity(void* pd, slimbus_framer_config* framer_config)
{
	if (pd == NULL)
		return EINVAL;
	if (framer_config == NULL)
		return EINVAL;

	return 0;
}
/**
 * Checks validity of parameters for function get_framer_config
 * @param[out] framer_config Pointer to structure to which Framer configuration will be written.
 * @param[in] pd Pointer to driver's private data.
 * @return 0 success
 * @return EINVAL invalid parameters
 */
static inline int slimbus_get_framer_config_sanity(void* pd, slimbus_framer_config* framer_config)
{
	if (pd == NULL)
		return EINVAL;
	if (framer_config == NULL)
		return EINVAL;

	return 0;
}
/**
 * Checks validity of parameters for function set_generic_device_config
 * @param[in] generic_device_config Pointer to structure containing Generic Device configuration.
 * @param[in] pd Pointer to driver's private data.
 * @return 0 success
 * @return EINVAL invalid parameters
 */
static inline int slimbus_set_generic_device_config_sanity(void* pd, slimbus_generic_device_config* generic_device_config)
{
	if (pd == NULL)
		return EINVAL;
	if (generic_device_config == NULL)
		return EINVAL;

	return 0;
}
/**
 * Checks validity of parameters for function get_generic_device_config
 * @param[out] generic_device_config Pointer to structure to which Generic Device configuration will be written.
 * @param[in] pd Pointer to driver's private data.
 * @return 0 success
 * @return EINVAL invalid parameters
 */
static inline int slimbus_get_generic_device_config_sanity(void* pd, slimbus_generic_device_config* generic_device_config)
{
	if (pd == NULL)
		return EINVAL;
	if (generic_device_config == NULL)
		return EINVAL;

	return 0;
}
/**
 * Checks validity of parameters for function get_data_port_status
 * @param[in] port_number Selected Port Number
 * @param[out] port_status Pointer to structure to which Framer configuration will be written.
 * @param[in] pd Pointer to driver's private data.
 * @return 0 success
 * @return EINVAL invalid parameters
 */
static inline int slimbus_get_data_port_status_sanity(void* pd, uint8_t port_number, slimbus_data_port_status* port_status)
{
	if (pd == NULL)
		return EINVAL;
	if (port_status == NULL)
		return EINVAL;

	if (port_number > (63))
		return EINVAL;

	return 0;
}
/**
 * Checks validity of parameters for function unfreeze
 * @param[in] pd Pointer to driver's private data.
 * @return 0 success
 * @return EINVAL invalid parameters
 */
static inline int slimbus_unfreeze_sanity(void* pd)
{
	if (pd == NULL)
		return EINVAL;

	return 0;
}
/**
 * Checks validity of parameters for function cancel_configuration
 * @param[in] pd Pointer to driver's private data.
 * @return 0 success
 * @return EINVAL invalid parameters
 */
static inline int slimbus_cancel_configuration_sanity(void* pd)
{
	if (pd == NULL)
		return EINVAL;

	return 0;
}
/**
 * Checks validity of parameters for function get_status_synchronization
 * @param[out] sf_sync If 1, the SLIMbus Manager achieved superframe synchronization. Not returned if NULL was passed.
 * @param[out] sfb_sync If 1, the SLIMbus Manager achieved superframe block synchronization. Not returned if NULL was passed.
 * @param[out] m_sync If 1, the SLIMbus Manager achieved message synchronization. Not returned if NULL was passed.
 * @param[out] f_sync If 1, the SLIMbus Manager achieved frame synchronization. Not returned if NULL was passed.
 * @param[out] ph_sync If 1, the SLIMbus Manager achieved phase synchronization and clock signal generated by the Generic device is valid. Not returned if NULL was passed.
 * @param[in] pd Pointer to driver's private data.
 * @return 0 success
 * @return EINVAL invalid parameters
 */
static inline int slimbus_get_status_synchronization_sanity(void* pd, bool* f_sync, bool* sf_sync, bool* m_sync, bool* sfb_sync, bool* ph_sync)
{
	if (pd == NULL)
		return EINVAL;

	return 0;
}
/**
 * Checks validity of parameters for function get_status_detached
 * @param[out] detached If 1, the SLIMbus Manager is detached from the bus after successful transmission of REPORT_ABSENT message.
 * @param[in] pd Pointer to driver's private data.
 * @return 0 success
 * @return EINVAL invalid parameters
 */
static inline int slimbus_get_status_detached_sanity(void* pd, bool* detached)
{
	if (pd == NULL)
		return EINVAL;
	if (detached == NULL)
		return EINVAL;

	return 0;
}
/**
 * Checks validity of parameters for function get_status_slimbus
 * @param[out] subframe_mode This variable contains the Subframe Mode read from the Framing Information during superframe synchronization and after reconfiguration (from the payload of the NEXT_SUBFRAME_MODE message sent by the SLIMbus Manager). Not returned if NULL was passed.
 * @param[out] clock_gear This variable contains the Clock Gear read from the Framing Information during superframe synchronization and after reconfiguration (from the payload of the NEXT_SUBFRAME_MODE message sent by the SLIMbus Manager). Not returned if NULL was passed.
 * @param[out] root_fr This variable contains the Root Frequency read from the Framing Information during superframe synchronization and after reconfiguration (from the payload of the NEXT_SUBFRAME_MODE message sent by the SLIMbus Manager). Not returned if NULL was passed.
 * @param[in] pd Pointer to driver's private data.
 * @return 0 success
 * @return EINVAL invalid parameters
 */
static inline int slimbus_get_status_slimbus_sanity(void* pd, slimbus_drv_subframe_mode* subframe_mode,
	slimbus_drv_clock_gear* clock_gear, slimbus_drv_root_frequency* root_fr)
{
	if (pd == NULL)
		return EINVAL;

	return 0;
}
/**
 * Checks validity of parameters for function msg_assign_logical_address
 * @param[in] new_la New Logical Address
 * @param[in] destination_ea Destination's Enumeration Address
 * @param[in] pd Pointer to driver's private data.
 * @return 0 success
 * @return EINVAL invalid parameters
 */
static inline int slimbus_msg_assign_logical_address_sanity(void* pd, uint64_t destination_ea, uint8_t new_la)
{
	if (pd == NULL)
		return EINVAL;
	if (new_la > (239))
		return EINVAL;

	return 0;
}
/**
 * Checks validity of parameters for function msg_reset_device
 * @param[in] destination_la Destination's Logic Address
 * @param[in] pd Pointer to driver's private data.
 * @return 0 success
 * @return EINVAL invalid parameters
 */
static inline int slimbus_msg_reset_device_sanity(void* pd, uint8_t destination_la)
{
	if (pd == NULL)
		return EINVAL;

	return 0;
}
/**
 * Checks validity of parameters for function msg_change_logical_address
 * @param[in] destination_la Destination's Logic Address
 * @param[in] new_la New Logical Address
 * @param[in] pd Pointer to driver's private data.
 * @return 0 success
 * @return EINVAL invalid parameters
 */
static inline int slimbus_msg_change_logical_address_sanity(void* pd, uint8_t destination_la, uint8_t new_la)
{
	if (pd == NULL)
		return EINVAL;

	if (new_la > (239))
		return EINVAL;

	return 0;
}
/**
 * Checks validity of parameters for function
 * msg_change_arbitration_priority
 * @param[in] broadcast Broadcast Message to all devices
 * @param[in] destination_la Destination's Logic Address, used if broadcast = 0
 * @param[in] new_arbitration_priority New Arbitration Priority
 * @param[in] pd Pointer to driver's private data.
 * @return 0 success
 * @return EINVAL invalid parameters
 */
static inline int slimbus_msg_change_arbitration_priority_sanity(void* pd, bool broadcast,
	uint8_t destination_la, slimbus_arbitration_priority new_arbitration_priority)
{
	if (pd == NULL)
		return EINVAL;

	if (new_arbitration_priority != AP_LOW       &&
		new_arbitration_priority != AP_DEFAULT   &&
		new_arbitration_priority != AP_HIGH      &&
		new_arbitration_priority != AP_MANAGER_1 &&
		new_arbitration_priority != AP_MANAGER_2 &&
		new_arbitration_priority != AP_MANAGER_3 &&
		new_arbitration_priority != AP_MAXIMUM)
		return EINVAL;

	return 0;
}
/**
 * Checks validity of parameters for function
 * msg_request_self_announcement
 * @param[in] pd Pointer to driver's private data.
 * @return 0 success
 * @return EINVAL invalid parameters
 */
static inline int slimbus_msg_request_self_announcement_sanity(void* pd)
{
	if (pd == NULL)
		return EINVAL;

	return 0;
}
/**
 * Checks validity of parameters for function msg_connect_source
 * @param[in] destination_la Destination's Logic Address
 * @param[in] channel_number Data Channel Number
 * @param[in] port_number Port Number to be connected to data channel
 * @param[in] pd Pointer to driver's private data.
 * @return 0 success
 * @return EINVAL invalid parameters
 */
static inline int slimbus_msg_connect_source_sanity(void* pd, uint8_t destination_la, uint8_t port_number, uint8_t channel_number)
{
	if (pd == NULL)
		return EINVAL;

	if (port_number > (63))
		return EINVAL;

	return 0;
}
/**
 * Checks validity of parameters for function msg_connect_sink
 * @param[in] destination_la Destination's Logic Address
 * @param[in] channel_number Data Channel Number
 * @param[in] port_number Port Number to be connected to data channel
 * @param[in] pd Pointer to driver's private data.
 * @return 0 success
 * @return EINVAL invalid parameters
 */
static inline int slimbus_msg_connect_sink_sanity(void* pd, uint8_t destination_la, uint8_t port_number, uint8_t channel_number)
{
	if (pd == NULL)
		return EINVAL;

	if (port_number > (63))
		return EINVAL;

	return 0;
}
/**
 * Checks validity of parameters for function msg_disconnect_port
 * @param[in] destination_la Destination's Logic Address
 * @param[in] port_number Port Number to be disconnected from data channel
 * @param[in] pd Pointer to driver's private data.
 * @return 0 success
 * @return EINVAL invalid parameters
 */
static inline int slimbus_msg_disconnect_port_sanity(void* pd, uint8_t destination_la, uint8_t port_number)
{
	if (pd == NULL)
		return EINVAL;

	if (port_number > (63))
		return EINVAL;

	return 0;
}
/**
 * Checks validity of parameters for function msg_change_content
 * @param[in] data_length Data Length
 * @param[in] data_type Data Type
 * @param[in] presence_rate Presence Rate
 * @param[in] frequency_locked_bit Frequency Locked Bit
 * @param[in] channel_link Channel Link
 * @param[in] channel_number Data Channel Number
 * @param[in] pd Pointer to driver's private data.
 * @param[in] auxiliary_bit_format Auxiliary Bit Format
 * @return 0 success
 * @return EINVAL invalid parameters
 */
static inline int slimbus_msg_change_content_sanity(void* pd, uint8_t channel_number, bool frequency_locked_bit, slimbus_drv_presence_rate presence_rate,
	slimbus_drv_aux_field_format auxiliary_bit_format, slimbus_drv_data_type data_type, bool channel_link, uint8_t data_length)
{
	if (pd == NULL)
		return EINVAL;

	if (presence_rate != DRV_PR_12K    &&
		presence_rate != DRV_PR_24K    &&
		presence_rate != DRV_PR_48K    &&
		presence_rate != DRV_PR_96K    &&
		presence_rate != DRV_PR_192K   &&
		presence_rate != DRV_PR_384K   &&
		presence_rate != DRV_PR_768K   &&
		presence_rate != DRV_PR_11025  &&
		presence_rate != DRV_PR_22050  &&
		presence_rate != DRV_PR_44100  &&
		presence_rate != DRV_PR_88200  &&
		presence_rate != DRV_PR_176400 &&
		presence_rate != DRV_PR_352800 &&
		presence_rate != DRV_PR_705600 &&
		presence_rate != DRV_PR_4K     &&
		presence_rate != DRV_PR_8K     &&
		presence_rate != DRV_PR_16K    &&
		presence_rate != DRV_PR_32K    &&
		presence_rate != DRV_PR_64K    &&
		presence_rate != DRV_PR_128K   &&
		presence_rate != DRV_PR_256K   &&
		presence_rate != DRV_PR_512K)
		return EINVAL;

	if (auxiliary_bit_format != AF_NOT_APPLICABLE &&
		auxiliary_bit_format != AF_ZCUV           &&
		auxiliary_bit_format != AF_USER_DEFINED)
		return EINVAL;

	if (data_type != DF_NOT_INDICATED    &&
		data_type != DF_LPCM             &&
		data_type != DF_IEC61937         &&
		data_type != DF_PACKED_PDM_AUDIO &&
		data_type != DF_USER_DEFINED_1   &&
		data_type != DF_USER_DEFINED_2)
		return EINVAL;

	if (data_length > (31))
		return EINVAL;

	return 0;
}
/**
 * Checks validity of parameters for function msg_request_information
 * @param[in] broadcast Broadcast Message to all devices
 * @param[in] destination_la Destination's Logic Address, used if broadcast = 0
 * @param[in] transaction_id Transaction ID
 * @param[in] element_code Element Code
 * @param[in] pd Pointer to driver's private data.
 * @return 0 success
 * @return EINVAL invalid parameters
 */
static inline int slimbus_msg_request_information_sanity(void* pd, bool broadcast, uint8_t destination_la, uint8_t transaction_id, uint16_t element_code)
{
	if (pd == NULL)
		return EINVAL;

	return 0;
}
/**
 * Checks validity of parameters for function
 * msg_request_clear_information
 * @param[in] destination_la Destination's Logic Address, used if broadcast = 0
 * @param[in] clear_mask_size Clear Mask size
 * @param[in] broadcast Broadcast Message to all devices
 * @param[in] clear_mask Pointer to Clear Mask (0 (NULL) to 16 bytes). If the size of Clear Mask is smaller than the size of the Information Slice, the Device shall pad the MSBs of Clear Mask with ones. If the size of Clear Mask is larger than the size of the Information Slice, the Device shall change only the portion of the Information Map corresponding to the identified Information Slice.
 * @param[in] element_code Element Code
 * @param[in] pd Pointer to driver's private data.
 * @param[in] transaction_id Transaction ID
 * @return 0 success
 * @return EINVAL invalid parameters
 */
static inline int slimbus_msg_request_clear_information_sanity(void* pd, bool broadcast, uint8_t destination_la, uint8_t transaction_id, uint16_t element_code, uint8_t* clear_mask, uint8_t clear_mask_size)
{
	if (pd == NULL)
		return EINVAL;

	return 0;
}
/**
 * Checks validity of parameters for function slimbus_drv_msg_clear_information
 * @param[in] destination_la Destination's Logic Address, used if broadcast = 0
 * @param[in] clear_mask_size Clear Mask size
 * @param[in] broadcast Broadcast Message to all devices
 * @param[in] clear_mask Pointer to Clear Mask (0 (NULL) to 16 bytes). If the size of Clear Mask is smaller than the size of the Information Slice, the Device shall pad the MSBs of Clear Mask with ones. If the size of Clear Mask is larger than the size of the Information Slice, the Device shall change only the portion of the Information Map corresponding to the identified Information Slice.
 * @param[in] element_code Element Code
 * @param[in] pd Pointer to driver's private data.
 * @return 0 success
 * @return EINVAL invalid parameters
 */
static inline int slimbus_msg_clear_information_sanity(void* pd, bool broadcast, uint8_t destination_la, uint16_t element_code, uint8_t* clear_mask, uint8_t clear_mask_size)
{
	if (pd == NULL)
		return EINVAL;

	return 0;
}
/**
 * Checks validity of parameters for function msg_begin_reconfiguration
 * @param[in] pd Pointer to driver's private data.
 * @return 0 success
 * @return EINVAL invalid parameters
 */
static inline int slimbus_msg_begin_reconfiguration_sanity(void* pd)
{
	if (pd == NULL)
		return EINVAL;

	return 0;
}
/**
 * Checks validity of parameters for function msg_next_active_framer
 * @param[in] outgoing_framer_clock_cycles Number of Outgoing Framer Clock cycles
 * @param[in] incoming_framer_la Incoming Framer's Logic Address
 * @param[in] pd Pointer to driver's private data.
 * @param[in] incoming_framer_clock_cycles Number of Incoming Framer Clock cycles
 * @return 0 success
 * @return EINVAL invalid parameters
 */
static inline int slimbus_msg_next_active_framer_sanity(void* pd,
	uint8_t incoming_framer_la, uint16_t outgoing_framer_clock_cycles, uint16_t incoming_framer_clock_cycles)
{
	if (pd == NULL)
		return EINVAL;

	if (incoming_framer_la > (239))
		return EINVAL;

	if (outgoing_framer_clock_cycles > (4095))
		return EINVAL;

	if (incoming_framer_clock_cycles > (4095))
		return EINVAL;

	return 0;
}
/**
 * Checks validity of parameters for function msg_next_subframe_mode
 * @param[in] new_subframe_mode New Subframe Mode
 * @param[in] pd Pointer to driver's private data.
 * @return 0 success
 * @return EINVAL invalid parameters
 */
static inline int slimbus_msg_next_subframe_mode_sanity(void* pd, slimbus_drv_subframe_mode new_subframe_mode)
{
	if (pd == NULL)
		return EINVAL;

	if (new_subframe_mode != SM_24_CSW_32_SL   &&
		new_subframe_mode != SM_16_CSW_32_SL   &&
		new_subframe_mode != SM_16_CSW_24_SL   &&
		new_subframe_mode != SM_12_CSW_32_SL   &&
		new_subframe_mode != SM_12_CSW_24_SL   &&
		new_subframe_mode != SM_8_CSW_32_SL    &&
		new_subframe_mode != SM_8_CSW_24_SL    &&
		new_subframe_mode != SM_6_CSW_32_SL    &&
		new_subframe_mode != SM_6_CSW_24_SL    &&
		new_subframe_mode != SM_6_CSW_8_SL     &&
		new_subframe_mode != SM_4_CSW_32_SL    &&
		new_subframe_mode != SM_4_CSW_24_SL    &&
		new_subframe_mode != SM_4_CSW_8_SL     &&
		new_subframe_mode != SM_4_CSW_6_SL     &&
		new_subframe_mode != SM_3_CSW_32_SL    &&
		new_subframe_mode != SM_3_CSW_24_SL    &&
		new_subframe_mode != SM_3_CSW_8_SL     &&
		new_subframe_mode != SM_3_CSW_6_SL     &&
		new_subframe_mode != SM_2_CSW_32_SL    &&
		new_subframe_mode != SM_2_CSW_24_SL    &&
		new_subframe_mode != SM_2_CSW_8_SL     &&
		new_subframe_mode != SM_2_CSW_6_SL     &&
		new_subframe_mode != SM_1_CSW_32_SL    &&
		new_subframe_mode != SM_1_CSW_24_SL    &&
		new_subframe_mode != SM_1_CSW_8_SL     &&
		new_subframe_mode != SM_1_CSW_6_SL     &&
		new_subframe_mode != SM_8_CSW_8_SL)
		return EINVAL;

	return 0;
}
/**
 * Checks validity of parameters for function msg_next_clock_gear
 * @param[in] new_clock_gear New Clock Gear
 * @param[in] pd Pointer to driver's private data.
 * @return 0 success
 * @return EINVAL invalid parameters
 */
static inline int slimbus_msg_next_clock_gear_sanity(void* pd, slimbus_drv_clock_gear new_clock_gear)
{
	if (pd == NULL)
		return EINVAL;

	if (new_clock_gear != CG_0	&&
		new_clock_gear != CG_1	&&
		new_clock_gear != CG_2	&&
		new_clock_gear != CG_3	&&
		new_clock_gear != CG_4	&&
		new_clock_gear != CG_5	&&
		new_clock_gear != CG_6	&&
		new_clock_gear != CG_7	&&
		new_clock_gear != CG_8	&&
		new_clock_gear != CG_9	&&
		new_clock_gear != CG_10)
		return EINVAL;

	return 0;
}
/**
 * Checks validity of parameters for function msg_next_root_frequency
 * @param[in] new_root_frequency New Root Frequency
 * @param[in] pd Pointer to driver's private data.
 * @return 0 success
 * @return EINVAL invalid parameters
 */
static inline int slimbus_msg_next_root_frequency_sanity(void* pd, slimbus_drv_root_frequency new_root_frequency)
{
	if (pd == NULL)
		return EINVAL;

	if (new_root_frequency != RF_0	&&
		new_root_frequency != RF_1	&&
		new_root_frequency != RF_2	&&
		new_root_frequency != RF_3	&&
		new_root_frequency != RF_4	&&
		new_root_frequency != RF_5	&&
		new_root_frequency != RF_6	&&
		new_root_frequency != RF_7	&&
		new_root_frequency != RF_8	&&
		new_root_frequency != RF_9)
		return EINVAL;

	return 0;
}
/**
 * Checks validity of parameters for function msg_next_pause_clock
 * @param[in] new_restart_time New Restart Time
 * @param[in] pd Pointer to driver's private data.
 * @return 0 success
 * @return EINVAL invalid parameters
 */
static inline int slimbus_msg_next_pause_clock_sanity(void* pd, slimbus_drv_restart_time new_restart_time)
{
	if (pd == NULL)
		return EINVAL;

	if (new_restart_time != RT_FAST_RECOVERY   &&
		new_restart_time != RT_CONSTANT_PHASE_RECOVERY	&&
		new_restart_time != RT_UNSPECIFIED_DELAY)
		return EINVAL;

	return 0;
}
/**
 * Checks validity of parameters for function msg_next_reset_bus
 * @param[in] pd Pointer to driver's private data.
 * @return 0 success
 * @return EINVAL invalid parameters
 */
static inline int slimbus_msg_next_reset_bus_sanity(void* pd)
{
	if (pd == NULL)
		return EINVAL;

	return 0;
}
/**
 * Checks validity of parameters for function msg_next_shutdown_bus
 * @param[in] pd Pointer to driver's private data.
 * @return 0 success
 * @return EINVAL invalid parameters
 */
static inline int slimbus_msg_next_shutdown_bus_sanity(void* pd)
{
	if (pd == NULL)
		return EINVAL;

	return 0;
}
/**
 * Checks validity of parameters for function msg_next_define_channel
 * @param[in] channel_number Data Channel Number
 * @param[in] segment_distribution Segment Distribution
 * @param[in] transport_protocol Transport Protocol
 * @param[in] pd Pointer to driver's private data.
 * @param[in] segment_length Segment Length
 * @return 0 success
 * @return EINVAL invalid parameters
 */
static inline int slimbus_msg_next_define_channel_sanity(void* pd, uint8_t channel_number, slimbus_drv_transport_protocol transport_protocol, uint16_t segment_distribution, uint8_t segment_length)
{
	if (pd == NULL)
		return EINVAL;

	if (transport_protocol != TP_ISOCHRONOUS           &&
		transport_protocol != TP_PUSHED                &&
		transport_protocol != TP_PULLED                &&
		transport_protocol != TP_LOCKED                &&
		transport_protocol != TP_ASYNC_SIMPLEX         &&
		transport_protocol != TP_ASYNC_HALF_DUPLEX     &&
		transport_protocol != TP_EXT_ASYNC_SIMPLEX     &&
		transport_protocol != TP_EXT_ASYNC_HALF_DUPLEX &&
		transport_protocol != TP_USER_DEFINED_1        &&
		transport_protocol != TP_USER_DEFINED_2)
		return EINVAL;

	if (segment_distribution > (4095))
		return EINVAL;

	if (segment_length > (31))
		return EINVAL;

	return 0;
}
/**
 * Checks validity of parameters for function msg_next_define_content
 * @param[in] data_length Data Length
 * @param[in] data_type Data Type
 * @param[in] presence_rate Presence Rate
 * @param[in] frequency_locked_bit Frequency Locked Bit
 * @param[in] channel_link Channel Link
 * @param[in] channel_number Data Channel Number
 * @param[in] pd Pointer to driver's private data.
 * @param[in] auxiliary_bit_format Auxiliary Bit Format
 * @return 0 success
 * @return EINVAL invalid parameters
 */
static inline int slimbus_msg_next_define_content_sanity(void* pd, uint8_t channel_number, bool frequency_locked_bit, slimbus_drv_presence_rate presence_rate, slimbus_drv_aux_field_format auxiliary_bit_format,  slimbus_drv_data_type data_type, bool channel_link, uint8_t data_length)
{
	if (pd == NULL)
		return EINVAL;

	if (presence_rate != DRV_PR_12K    &&
		presence_rate != DRV_PR_24K    &&
		presence_rate != DRV_PR_48K    &&
		presence_rate != DRV_PR_96K    &&
		presence_rate != DRV_PR_192K   &&
		presence_rate != DRV_PR_384K   &&
		presence_rate != DRV_PR_768K   &&
		presence_rate != DRV_PR_11025  &&
		presence_rate != DRV_PR_22050  &&
		presence_rate != DRV_PR_44100  &&
		presence_rate != DRV_PR_88200  &&
		presence_rate != DRV_PR_176400 &&
		presence_rate != DRV_PR_352800 &&
		presence_rate != DRV_PR_705600 &&
		presence_rate != DRV_PR_4K     &&
		presence_rate != DRV_PR_8K     &&
		presence_rate != DRV_PR_16K    &&
		presence_rate != DRV_PR_32K    &&
		presence_rate != DRV_PR_64K    &&
		presence_rate != DRV_PR_128K   &&
		presence_rate != DRV_PR_256K   &&
		presence_rate != DRV_PR_512K)
		return EINVAL;

	if (auxiliary_bit_format != AF_NOT_APPLICABLE   &&
		auxiliary_bit_format != AF_ZCUV             &&
		auxiliary_bit_format != AF_USER_DEFINED)
		return EINVAL;

	if (data_type != DF_NOT_INDICATED    &&
		data_type != DF_LPCM             &&
		data_type != DF_IEC61937         &&
		data_type != DF_PACKED_PDM_AUDIO &&
		data_type != DF_USER_DEFINED_1	 &&
		data_type != DF_USER_DEFINED_2)
		return EINVAL;

	if (data_length > (31))
		return EINVAL;

	return 0;
}
/**
 * Checks validity of parameters for function msg_next_activate_channel
 * @param[in] channel_number Data Channel Number
 * @param[in] pd Pointer to driver's private data.
 * @return 0 success
 * @return EINVAL invalid parameters
 */
static inline int slimbus_msg_next_activate_channel_sanity(void* pd, uint8_t channel_number)
{
	if (pd == NULL)
		return EINVAL;

	return 0;
}
/**
 * Checks validity of parameters for function msg_next_deactivate_channel
 * @param[in] channel_number Data Channel Number
 * @param[in] pd Pointer to driver's private data.
 * @return 0 success
 * @return EINVAL invalid parameters
 */
static inline int slimbus_msg_next_deactivate_channel_sanity(void* pd, uint8_t channel_number)
{
	if (pd == NULL)
		return EINVAL;

	return 0;
}
/**
 * Checks validity of parameters for function msg_next_remove_channel
 * @param[in] channel_number Data Channel Number
 * @param[in] pd Pointer to driver's private data.
 * @return 0 success
 * @return EINVAL invalid parameters
 */
static inline int slimbus_msg_next_remove_channel_sanity(void* pd, uint8_t channel_number)
{
	if (pd == NULL)
		return EINVAL;

	return 0;
}
/**
 * Checks validity of parameters for function msg_reconfigure_now
 * @param[in] pd Pointer to driver's private data.
 * @return 0 success
 * @return EINVAL invalid parameters
 */
static inline int slimbus_msg_reconfigure_now_sanity(void* pd)
{
	if (pd == NULL)
		return EINVAL;

	return 0;
}
/**
 * Checks validity of parameters for function msg_request_value
 * @param[in] broadcast Broadcast Message to all devices
 * @param[in] destination_la Destination's Logic Address, used if broadcast = 0
 * @param[in] transaction_id Transaction ID
 * @param[in] element_code Element Code
 * @param[in] pd Pointer to driver's private data.
 * @return 0 success
 * @return EINVAL invalid parameters
 */
static inline int slimbus_msg_request_value_sanity(void* pd, bool broadcast, uint8_t destination_la, uint8_t transaction_id, uint16_t element_code)
{
	if (pd == NULL)
		return EINVAL;

	return 0;
}
/**
 * Checks validity of parameters for function msg_request_change_value
 * @param[in] destination_la Destination's Logic Address, used if broadcast = 0
 * @param[in] value_update Pointer to Value Update (0 (NULL) to 16 bytes).
 * @param[in] broadcast Broadcast Message to all devices
 * @param[in] element_code Element Code
 * @param[in] pd Pointer to driver's private data.
 * @param[in] value_update_size Value Update size
 * @param[in] transaction_id Transaction ID
 * @return 0 success
 * @return EINVAL invalid parameters
 */
static inline int slimbus_msg_request_change_value_sanity(void* pd, bool broadcast, uint8_t destination_la, uint8_t transaction_id, uint16_t element_code, uint8_t* value_update, uint8_t value_update_size)
{
	if (pd == NULL)
		return EINVAL;

	if (value_update_size > (16))
		return EINVAL;

	return 0;
}
/**
 * Checks validity of parameters for function msg_change_value
 * @param[in] destination_la Destination's Logic Address, used if broadcast = 0
 * @param[in] value_update Pointer to Value Update (0 (NULL) to 16 bytes).
 * @param[in] broadcast Broadcast Message to all devices
 * @param[in] element_code Element Code
 * @param[in] pd Pointer to driver's private data.
 * @param[in] value_update_size Value Update size
 * @return 0 success
 * @return EINVAL invalid parameters
 */
static inline int slimbus_msg_change_value_sanity(void* pd, bool broadcast, uint8_t destination_la, uint16_t element_code, uint8_t* value_update, uint8_t value_update_size)
{
	if (pd == NULL)
		return EINVAL;

	if (value_update_size > (16))
		return EINVAL;

	return 0;
}

#endif	/* __SLIMBUS_DRV_SANITY_H__ */
