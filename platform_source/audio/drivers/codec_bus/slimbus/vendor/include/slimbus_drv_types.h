/*
 * slimbus is a kernel driver which is used to manager slimbus devices
 *
 * Copyright (c) 2012-2018 Huawei Technologies Co., Ltd.
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

#ifndef __SLIMBUS_DRV_TYPES_H__
#define __SLIMBUS_DRV_TYPES_H__

#include "slimbus_drv_ps.h"

/* SLIMbus Message Fields */
#define MESSAGE_ARBITRATION_TYPE_MASK                    0x0F
#define MESSAGE_ARBITRATION_TYPE_SHIFT                   4
#define MESSAGE_ARBITRATION_TYPE_OFFSET                  0
#define MESSAGE_ARBITRATION_EXTENSION_MASK               0x01
#define MESSAGE_ARBITRATION_EXTENSION_SHIFT              3
#define MESSAGE_ARBITRATION_EXTENSION_OFFSET             0
#define MESSAGE_ARBITRATION_PRIORITY_MASK                0x07
#define MESSAGE_ARBITRATION_PRIORITY_SHIFT               0
#define MESSAGE_ARBITRATION_PRIORITY_OFFSET              0
#define MESSAGE_SOURCE_ADDRESS_OFFSET                    1

#define MESSAGE_MESSAGE_TYPE_MASK                        0x7
#define MESSAGE_MESSAGE_TYPE_SHIFT                       5
#define MESSAGE_MESSAGE_TYPE_OFFSET                      0
#define MESSAGE_REMAINING_LENGTH_MASK                    0x1F
#define MESSAGE_REMAINING_LENGTH_SHIFT                   0
#define MESSAGE_REMAINING_LENGTH_OFFSET                  0
#define MESSAGE_MESSAGE_CODE_MASK                        0x7F
#define MESSAGE_MESSAGE_CODE_SHIFT                       0
#define MESSAGE_MESSAGE_CODE_OFFSET                      1
#define MESSAGE_MESSAGE_DESTINATION_TYPE_MASK            0x3
#define MESSAGE_MESSAGE_DESTINATION_TYPE_SHIFT           4
#define MESSAGE_MESSAGE_DESTINATION_TYPE_OFFSET          2
#define MESSAGE_PRIMARY_INTEGRITY_MASK                   0xF
#define MESSAGE_PRIMARY_INTEGRITY_SHIFT                  0
#define MESSAGE_PRIMARY_INTEGRITY_OFFSET                 2
#define MESSAGE_DESTINATION_ADDRESS_OFFSET               3
#define MESSAGE_MESSAGE_INTEGRITY_OFFSET                 0
#define MESSAGE_MESSAGE_INTEGRITY_MASK                   0xF
#define MESSAGE_MESSAGE_INTEGRITY_SHIFT                  4
#define MESSAGE_RESPONSE_CODE_OFFSET                     0
#define MESSAGE_RESPONSE_CODE_MASK                       0xF
#define MESSAGE_RESPONSE_CODE_SHIFT                      0

/* Other Message related defines */
#define MESSAGE_ARBITRATION_LENGTH_LONG                  7
#define MESSAGE_ARBITRATION_LENGTH_SHORT                 2
#define MESSAGE_HEADER_LENGTH_BROADCAST                  3
#define MESSAGE_HEADER_LENGTH_SHORT                      4
#define MESSAGE_HEADER_LENGTH_LONG                       9

/* SLIMBus Messages Codes Defines */
/* Shared Message Channel Management Messages */
#define MESSAGE_CODE_REPORT_PRESENT                      0x01
#define MESSAGE_CODE_ASSIGN_LOGICAL_ADDRESS              0x02
#define MESSAGE_CODE_RESET_DEVICE                        0x04
#define MESSAGE_CODE_CHANGE_LOGICAL_ADDRESS              0x08
#define MESSAGE_CODE_CHANGE_ARBITRATION_PRIORITY         0x09
#define MESSAGE_CODE_REQUEST_SELF_ANNOUNCEMENT           0x0C
#define MESSAGE_CODE_REPORT_ABSENT                       0x0F
/* Data Space Management Messages */
#define MESSAGE_CODE_CONNECT_SOURCE                      0x10
#define MESSAGE_CODE_CONNECT_SINK                        0x11
#define MESSAGE_CODE_DISCONNECT_PORT                     0x14
#define MESSAGE_CODE_CHANGE_CONTENT                      0x18
/* Information Messages */
#define MESSAGE_CODE_REQUEST_INFORMATION                 0x20
#define MESSAGE_CODE_REQUEST_CLEAR_INFORMATION           0x21
#define MESSAGE_CODE_REPLY_INFORMATION                   0x24
#define MESSAGE_CODE_CLEAR_INFORMATION                   0x28
#define MESSAGE_CODE_REPORT_INFORMATION                  0x29
/* Value Element Messages */
#define MESSAGE_CODE_REQUEST_VALUE                       0x60
#define MESSAGE_CODE_REQUEST_CHANGE_VALUE                0x61
#define MESSAGE_CODE_REPLY_VALUE                         0x64
#define MESSAGE_CODE_CHANGE_VALUE                        0x68
/* Synchronized Event Messages */
#define MESSAGE_CODE_BEGIN_RECONFIGURATION               0x40
#define MESSAGE_CODE_NEXT_ACTIVE_FRAMER                  0x44
#define MESSAGE_CODE_NEXT_SUBFRAME_MODE                  0x45
#define MESSAGE_CODE_NEXT_CLOCK_GEAR                     0x46
#define MESSAGE_CODE_NEXT_ROOT_FREQUENCY                 0x47
#define MESSAGE_CODE_NEXT_PAUSE_CLOCK                    0x4A
#define MESSAGE_CODE_NEXT_RESET_BUS                      0x4B
#define MESSAGE_CODE_NEXT_SHUTDOWN_BUS                   0x4C
#define MESSAGE_CODE_NEXT_DEFINE_CHANNEL                 0x50
#define MESSAGE_CODE_NEXT_DEFINE_CONTENT                 0x51
#define MESSAGE_CODE_NEXT_ACTIVATE_CHANNEL               0x54
#define MESSAGE_CODE_NEXT_DEACTIVATE_CHANNEL             0x55
#define MESSAGE_CODE_NEXT_REMOVE_CHANNEL                 0x58
#define MESSAGE_CODE_RECONFIGURE_NOW                     0x5F

/* SLIMbus Payload Fields */
#define MESSAGE_PAYLOAD_ELEMENT_CODE_BYTE_ADDRESS_SHIFT   4
#define MESSAGE_PAYLOAD_ELEMENT_CODE_BYTE_ADDRESS_MASK    0x0FFF
#define MESSAGE_PAYLOAD_ELEMENT_CODE_SLICE_SIZE_SHIFT     0
#define MESSAGE_PAYLOAD_ELEMENT_CODE_SLICE_SIZE_MASK      0x07
#define MESSAGE_PAYLOAD_ELEMENT_CODE_BIT_NUMBER_SHIFT     0
#define MESSAGE_PAYLOAD_ELEMENT_CODE_BIT_NUMBER_MASK      0x07
#define MESSAGE_PAYLOAD_ELEMENT_CODE_ACCESS_TYPE_SHIFT    3
#define MESSAGE_PAYLOAD_ELEMENT_CODE_ACCESS_TYPE_MASK     0x01
#define MESSAGE_PAYLOAD_DATA_CHANNEL_NUMBER_SHIFT         0
#define MESSAGE_PAYLOAD_DATA_CHANNEL_NUMBER_MASK          0xFF
#define MESSAGE_PAYLOAD_PRESENCE_RATE_SHIFT               0
#define MESSAGE_PAYLOAD_PRESENCE_RATE_MASK                0x7F
#define MESSAGE_PAYLOAD_FREQUENCY_LOCKED_BIT_SHIFT        7
#define MESSAGE_PAYLOAD_FREQUENCY_LOCKED_BIT_MASK         0x01
#define MESSAGE_PAYLOAD_DATA_TYPE_SHIFT                   0
#define MESSAGE_PAYLOAD_DATA_TYPE_MASK                    0x0F
#define MESSAGE_PAYLOAD_AUXILIARY_BIT_FORMAT_SHIFT        4
#define MESSAGE_PAYLOAD_AUXILIARY_BIT_FORMAT_MASK         0x0F
#define MESSAGE_PAYLOAD_DATA_LENGTH_SHIFT                 0
#define MESSAGE_PAYLOAD_DATA_LENGTH_MASK                  0x1F
#define MESSAGE_PAYLOAD_CHANNEL_LINK_SHIFT                5
#define MESSAGE_PAYLOAD_CHANNEL_LINK_MASK                 0x01
#define MESSAGE_PAYLOAD_SEGMENT_DISTRIBUTION_LOW_SHIFT    0
#define MESSAGE_PAYLOAD_SEGMENT_DISTRIBUTION_LOW_MASK     0xFF
#define MESSAGE_PAYLOAD_SEGMENT_DISTRIBUTION_HIGH_SHIFT   0
#define MESSAGE_PAYLOAD_SEGMENT_DISTRIBUTION_HIGH_MASK    0x0F
#define MESSAGE_PAYLOAD_TRANSPORT_PROTOCOL_SHIFT          4
#define MESSAGE_PAYLOAD_TRANSPORT_PROTOCOL_MASK           0x0F
#define MESSAGE_PAYLOAD_SEGMENT_LENGTH_SHIFT              0
#define MESSAGE_PAYLOAD_SEGMENT_LENGTH_MASK               0x1F
#define MESSAGE_PAYLOAD_PORT_NUMBER_SHIFT                 0
#define MESSAGE_PAYLOAD_PORT_NUMBER_MASK                  0x3F
#define MESSAGE_PAYLOAD_TRANSACTION_ID_SHIFT              0
#define MESSAGE_PAYLOAD_TRANSACTION_ID_MASK               0xFF
#define MESSAGE_PAYLOAD_LOGICAL_ADDRESS_SHIFT             0
#define MESSAGE_PAYLOAD_LOGICAL_ADDRESS_MASK              0xFF
#define MESSAGE_PAYLOAD_ARBITRATION_PRIORITY_SHIFT        0
#define MESSAGE_PAYLOAD_ARBITRATION_PRIORITY_MASK         0x07
#define MESSAGE_PAYLOAD_OUTGOING_FRAMER_CYCLES_LOW_SHIFT  0
#define MESSAGE_PAYLOAD_OUTGOING_FRAMER_CYCLES_LOW_MASK   0xFF
#define MESSAGE_PAYLOAD_OUTGOING_FRAMER_CYCLES_HIGH_SHIFT 0
#define MESSAGE_PAYLOAD_OUTGOING_FRAMER_CYCLES_HIGH_MASK  0x0F
#define MESSAGE_PAYLOAD_INCOMING_FRAMER_CYCLES_LOW_SHIFT  4
#define MESSAGE_PAYLOAD_INCOMING_FRAMER_CYCLES_LOW_MASK   0x0F
#define MESSAGE_PAYLOAD_INCOMING_FRAMER_CYCLES_HIGH_SHIFT 0
#define MESSAGE_PAYLOAD_INCOMING_FRAMER_CYCLES_HIGH_MASK  0xFF
#define MESSAGE_PAYLOAD_SUBFRAME_MODE_SHIFT               0
#define MESSAGE_PAYLOAD_SUBFRAME_MODE_MASK                0x1F
#define MESSAGE_PAYLOAD_CLOCK_GEAR_SHIFT                  0
#define MESSAGE_PAYLOAD_CLOCK_GEAR_MASK                   0x0F
#define MESSAGE_PAYLOAD_ROOT_FREQUENCY_SHIFT              0
#define MESSAGE_PAYLOAD_ROOT_FREQUENCY_MASK               0x0F
#define MESSAGE_PAYLOAD_RESTART_TIME_SHIFT                0
#define MESSAGE_PAYLOAD_RESTART_TIME_MASK                 0xFF

#define ELEMENT_CODE_BYTE_BASED_ACCESS                    1
#define ELEMENT_CODE_ELEMENTAL_ACCESS                     0
#define ELEMENT_CODE_LENGTH                               2
#define TRANSACTION_ID_LENGTH                             1
/* SLIMbus Information Elements */
/* Interface Device Class-specific Information Elements */
#define IE_INTERFACE_DATA_SLOT_OVERLAP_ADDRESS            0x400
#define IE_INTERFACE_DATA_SLOT_OVERLAP_SHIFT              4
#define IE_INTERFACE_DATA_SLOT_OVERLAP_MASK               0x1
#define IE_INTERFACE_DATA_SLOT_OVERLAP_READONLY           0

#define IE_INTERFACE_LOST_MS_ADDRESS                      0x400
#define IE_INTERFACE_LOST_MS_SHIFT                        3
#define IE_INTERFACE_LOST_MS_MASK                         0x1
#define IE_INTERFACE_LOST_MS_READONLY                     0

#define IE_INTERFACE_LOST_SFS_ADDRESS                     0x400
#define IE_INTERFACE_LOST_SFS_SHIFT                       2
#define IE_INTERFACE_LOST_SFS_MASK                        0x1
#define IE_INTERFACE_LOST_SFS_READONLY                    0

#define IE_INTERFACE_LOST_FS_ADDRESS                      0x400
#define IE_INTERFACE_LOST_FS_SHIFT                        1
#define IE_INTERFACE_LOST_FS_MASK                         0x1
#define IE_INTERFACE_LOST_FS_READONLY                     0

#define IE_INTERFACE_MC_TX_COL_ADDRESS                    0x400
#define IE_INTERFACE_MC_TX_COL_SHIFT                      0
#define IE_INTERFACE_MC_TX_COL_MASK                       0x1
#define IE_INTERFACE_MC_TX_COL_READONLY                   0

/* Manager Class-specific Information Elements */
#define IE_MANAGER_ACTIVE_MANAGER_ADDRESS                0x400
#define IE_MANAGER_ACTIVE_MANAGER_SHIFT                  0
#define IE_MANAGER_ACTIVE_MANAGER_MASK                   0x1
#define IE_MANAGER_ACTIVE_MANAGER_READONLY               1

/* Framer Class-specific Information Elements */
#define IE_FRAMER_QUALITY_ADDRESS                        0x400
#define IE_FRAMER_QUALITY_SHIFT                          6
#define IE_FRAMER_QUALITY_MASK                           0x3
#define IE_FRAMER_QUALITY_READONLY                       1

#define IE_FRAMER_GC_TX_COL_ADDRESS                      0x400
#define IE_FRAMER_GC_TX_COL_SHIFT                        3
#define IE_FRAMER_GC_TX_COL_MASK                         0x1
#define IE_FRAMER_GC_TX_COL_READONLY                     0

#define IE_FRAMER_FI_TX_COL_ADDRESS                      0x400
#define IE_FRAMER_FI_TX_COL_SHIFT                        2
#define IE_FRAMER_FI_TX_COL_MASK                         0x1
#define IE_FRAMER_FI_TX_COL_READONLY                     0

#define IE_FRAMER_FS_TX_COL_ADDRESS                      0x400
#define IE_FRAMER_FS_TX_COL_SHIFT                        1
#define IE_FRAMER_FS_TX_COL_MASK                         0x1
#define IE_FRAMER_FS_TX_COL_READONLY                     0

#define IE_FRAMER_ACTIVE_FRAMER_ADDRESS                  0x400
#define IE_FRAMER_ACTIVE_FRAMER_SHIFT                    0
#define IE_FRAMER_ACTIVE_FRAMER_MASK                     0x1
#define IE_FRAMER_ACTIVE_FRAMER_READONLY                 1

/* Core Information Elements */
#define IE_CORE_DEVICE_CLASS_ADDRESS                     0x009
#define IE_CORE_DEVICE_CLASS_SHIFT                       0
#define IE_CORE_DEVICE_CLASS_MASK                        0xFF
#define IE_CORE_DEVICE_CLASS_READONLY                    1

#define IE_CORE_DEVICE_CLASS_VERSION_ADDRESS            0x008
#define IE_CORE_DEVICE_CLASS_VERSION_SHIFT               0
#define IE_CORE_DEVICE_CLASS_VERSION_MASK               0xFF
#define IE_CORE_DEVICE_CLASS_VERSION_READONLY           1

#define IE_CORE_EX_ERROR_ADDRESS                        0x000
#define IE_CORE_EX_ERROR_SHIFT                          3
#define IE_CORE_EX_ERROR_MASK                           0x1
#define IE_CORE_EX_ERROR_READONLY                       0

#define IE_CORE_RECONFIG_OBJECTION_ADDRESS              0x000
#define IE_CORE_RECONFIG_OBJECTION_SHIFT                2
#define IE_CORE_RECONFIG_OBJECTION_MASK                 0x1
#define IE_CORE_RECONFIG_OBJECTION_READONLY             1

#define IE_CORE_DATA_TX_COL_ADDRESS                     0x000
#define IE_CORE_DATA_TX_COL_SHIFT                       1
#define IE_CORE_DATA_TX_COL_MASK                        0x1
#define IE_CORE_DATA_TX_COL_READONLY                    0

#define IE_CORE_UNSPRTD_MSG_ADDRESS                     0x000
#define IE_CORE_UNSPRTD_MSG_SHIFT                       0
#define IE_CORE_UNSPRTD_MSG_MASK                        0x1
#define IE_CORE_UNSPRTD_MSG_READONLY                    0

#define IE_CLASS_SPECIFIC_OFFSET                        0x400
#define IE_CORE_OFFSET                                  0x0

/* Transport Protocols */
#define TP_ISOCHRONOUS_MASK                             1
#define TP_ISOCHRONOUS_SHIFT                            0
#define TP_PUSHED_MASK                                  1
#define TP_PUSHED_SHIFT                                 1
#define TP_PULLED_MASK                                  1
#define TP_PULLED_SHIFT                                 2

/* SLIMbus Device Classes */
#define DEVICE_CLASS_MANAGER                            0xFF
#define DEVICE_CLASS_FRAMER                             0xFE
#define DEVICE_CLASS_INTERFACE                          0xFD
#define DEVICE_CLASS_GENERIC                            0x00

/* Misc defines */
#define RX_FIFO_MSG_MAX_SIZE                            64
#define TX_FIFO_MSG_MAX_SIZE                            64
#define DATA_DIRECTION_SOURCE                           0
#define DATA_DIRECTION_SINK                             1
#define MESSAGE_SOURCE_ACTIVE_MANAGER                   0xFF

#define RX_FIFO_FLAG_RX_OVERFLOW                        8
#define RX_FIFO_FLAG_RX_MSG_LEN                         0
#define RX_FIFO_FLAG_RX_MSG_MASK                        0x3F
#define RX_FIFO_FLAG_OFFSET                             15

#define FIFO_DELAY_TIMEOUT                              5000
#define FIFO_WAIT_TIMEOUT                               5500

#define FIFO_CLEAR_MASK                                 0x40404040
#define CYCLES_TIME                                     20


/* Get/Set Message fields Macros */

/**
 * Returns Message field Mask
 * @param[in] name message field name
 * @return message field mask
 */
#define get_msg_field_mask(name)						   (MESSAGE_ ## name ## _MASK)

/**
 * Returns Message field Shift bits
 * @param[in] name message field name
 * @return message field Shift bits
 */
#define get_msg_field_shift(name) 						   (MESSAGE_ ## name ## _SHIFT)

/**
 * Returns Message field Offset
 * @param[in] name message field name
 * @return message field offset
 */
#define get_msg_field_offset(name)						   (MESSAGE_ ## name ## _OFFSET)

/*
 * Calculates Message field Offset
 * @param[in] name message field name
 * @param[in] offset additional offset
 * @return message field offset
 */
#define calc_msg_field_offset(name, offset)				   (offset + (get_msg_field_offset(name)))

/*
 * Returns Message field value
 * @param[in] source raw message byte array
 * @param[in] offset additional offset
 * @param[in] name message field name
 * @return message field value
 */
#define get_msg_field(source, offset, name)				   ((source[calc_msg_field_offset(name, offset)] >> (get_msg_field_shift(name))) & (get_msg_field_mask(name)))

/*
 * Sets Message field value
 * @param[in] target raw message byte array
 * @param[in] value message field value
 * @param[in] offset additional offset
 * @param[in] name message field name
 */
#define set_msg_field(target, value, offset, name)		   target[calc_msg_field_offset(name, offset)] &= ~((get_msg_field_mask(name)) << (get_msg_field_shift(name))); \
																target[calc_msg_field_offset(name, offset)] |= ((value) & (get_msg_field_mask(name))) << (get_msg_field_shift(name))


/* Get/Set Message Payload Fields Macros */

/**
 * Returns Message Payload field Mask
 * @param[in] name message payload field name
 * @return message payload field mask
 */
#define get_msg_payload_field_mask(name)					   (MESSAGE_PAYLOAD_ ## name ## _MASK)

/**
 * Returns Message Payload field Shift bits
 * @param[in] name message payload field name
 * @return message payload field Shift bits
 */
#define get_msg_payload_field_shift(name)					   (MESSAGE_PAYLOAD_ ## name ## _SHIFT)

/*
 * Calculates Message payload field
 * @param[in] name message payload field name
 * @param[in] value message payload field value
 * @return message payload field
 */
#define msg_payload_field(name, value)					   (((value) & (get_msg_payload_field_mask(name))) << (get_msg_payload_field_shift(name)))

/*
 * Returns Message Payload field value
 * @param[in] payload raw message payload byte array
 * @param[in] name message field name
 * @return message payload field value
 */
#define get_msg_payload_field(name, payload)				   (((payload) >> (get_msg_payload_field_shift(name))) & (get_msg_payload_field_mask(name)))

/*
 * Calculates Byte Based Element Code
 * @param[in] byteAddress
 * @param[in] slice_size
 * @return element code
 */
#define element_code_byte_based(byteAddress, slice_size)	   (msg_payload_field(ELEMENT_CODE_BYTE_ADDRESS, byteAddress) | msg_payload_field(ELEMENT_CODE_SLICE_SIZE, slice_size) | msg_payload_field(ELEMENT_CODE_ACCESS_TYPE, ELEMENT_CODE_BYTE_BASED_ACCESS))


/* Get/Set Information Elements Macros */

/**
 * Returns Information Element Address
 * @param[in] name information element name
 * @return information element address
 */
#define get_ie_field_address(name)					    (IE_ ## name ## _ADDRESS)

/**
 * Returns Information Element Mask
 * @param[in] name information element name
 * @return information element mask
 */
#define get_ie_field_mask(name)							   (IE_ ## name ## _MASK)

/**
 * Returns Information Element Shift bits
 * @param[in] name information element name
 * @return information element shift bits
 */
#define get_ie_field_shift(name)						  (IE_ ## name ## _SHIFT)

/**
 * Returns Information Element Read Only value
 * @param[in] name information element name
 * @return information element read only value
 */
#define get_ie_field_readonly(name)						   (IE_ ## name ## _READONLY)

/**
 * Returns Information Element Shifted Mask
 * @param[in] name information element name
 * @return information element shifted mask
 */
#define get_ie_mask_shifted(name) 						   ((get_ie_field_mask(name)) << (get_ie_field_shift(name)))

/**
 * Returns Information Element field value
 * @param[in] is source information slice
 * @param[in] name information element field name
 * @return information element value
 */
#define get_ie_field(name, is)							   (((is) >> get_ie_field_shift(name)) & get_ie_field_mask(name))

/**
 * Assigns Information Element value with Byte Access to target variable
 * @param[out] output target variable
 * @param[in] name information element field name
 * @param[in] address source information slice address
 * @param[in] is source information slice
 */
#define get_ie_byte_access(output, name, address, is) 	   if ((get_ie_field_address(name)) == (address)) \
																	output = get_ie_field(name, (is))
/**
 * Assigns Information Element value with Elemental Access to target variable
 * @param[out] output target variable
 * @param[in] name information element field name
 * @param[in] address source information slice address
 * @param[in] bitNumber source information slice bit number
 * @param[in] is source information slice
 */
#define get_ie_elemental_access(output, name, address, bitNumber, is) \
																if (((get_ie_field_address(name)) == (address)) && ((get_ie_field_shift(name)) == (bitNumber))) \
																	output = (is) & (get_ie_field_mask(name))
/**
 * Sets up Information Element mask with Byte Access to target variable
 * If Information Element is not clearable (is read-only) it will be removed from clear mask
 * @param[in] name information element field name
 * @param[in] address source information slice address
 * @param[inout] is source information slice
 */
#define clear_ie_byte_access(name, address, is)			   if ((get_ie_field_readonly(name)) && ((get_ie_field_address(name)) == (address))) \
																	is &= (uint8_t)(~(get_ie_mask_shifted(name)))

/**
 * Sets up Information Element mask with Elemental Access to target variable
 * If Information Element is not clearable (is read-only) it will be removed from clear mask
 * @param[in] name information element field name
 * @param[in] address source information slice address
 * @param[in] bitNumber source information slice bit number
 * @param[inout] is source information slice
 */
#define clear_ie_elemental_access(name, address, bitNumber, is) \
																if ((get_ie_field_readonly(name)) && ((get_ie_field_address(name)) == (address)) && ((get_ie_field_shift(name)) == (bitNumber))) \
																	is &= (uint8_t)(~(get_ie_field_mask(name)))

/**
 * Returns Transport Protocol bitfield Mask
 * @param[in] name transport protocol name
 * @return Transport Protocol bitfield Mask
 */
#define get_tp_field_mask(name)							   (TP_ ## name ## _MASK)

/**
 * Returns Transport Protocol bitfield Shift bits
 * @param[in] name transport protocol name
 * @return Transport Protocol bitfield Shift bits
 */
#define get_tp_field_shift(name)							   (TP_ ## name ## _SHIFT)

/*
 * Calculates Transport Protocol bit field
 * @param[in] name transport protocol name
 * @param[in] value 1 if transport protocol is supported, 0 otherwise
 * @return transport protocol supported bitfield
 */
#define tp_field(name, value)							   (((value) & (get_tp_field_mask(name))) << (get_tp_field_shift(name)))

/*
 * Returns Transport Protocol bit field value
 * @param[in] name transport protocol name
 * @param[in] reg register value
 * @return 1 if transport protocol is supported, 0 otherwise
 */
#define get_tp_field(name, reg)							   (((reg) >> (get_tp_field_shift(name))) & (get_tp_field_mask(name)))

/**
 * Returns lower byte of variable
 * @param[in] data input data
 * @return lower byte
 */
#define lower_byte(data)								   ((data) & 0xFF)

/**
 * Returns higher byte of variable
 * @param[in] data input data
 * @return higher byte
 */
#define higher_byte(data)								   (((data) >> 8) & 0xFF)

/**
 * Returns lower nibble of the higher byte of variable
 * @param[in] data input data
 * @return lower nibble of higher byte
 */
#define higher_byte_lower_nibble(data)					   (((data) >> 8) & 0x0F)

/**
 * Returns 48-bit long field
 * @param[in] source source byte array
 * @param[in] offset offset of array
 * @return uint64_t with 48 bits of data
 */
#define get_field_6b(source, offset) \
	(((uint64_t)(source)[(offset)] << 40) \
	| ((uint64_t)(source)[(offset) + 1] << 32) \
	| ((uint64_t)(source)[(offset) + 2] << 24) \
	| ((uint64_t)(source)[(offset) + 3] << 16) \
	| ((uint64_t)(source)[(offset) + 4] << 8) \
	| ((uint64_t)(source)[(offset) + 5]))

/**
 * Sets 48-bit long field in target byte array
 * @param[out] target target byte array
 * @param[in] value input value
 * @param[in] offset offset of array
 */
#define set_field_6b(target, value, offset) \
	(target)[offset] = (uint8_t)((uint64_t)(value) >> 40); \
	(target)[(offset) + 1] = (uint8_t)((uint64_t)(value) >> 32); \
	(target)[(offset) + 2] = (uint8_t)((uint64_t)(value) >> 24); \
	(target)[(offset) + 3] = (uint8_t)((uint64_t)(value) >> 16); \
	(target)[(offset) + 4] = (uint8_t)((uint64_t)(value) >> 8); \
	(target)[(offset) + 5] = (uint8_t)(value)

/**
 * Returns 8-bit long field
 * @param[in] source source byte array
 * @param[in] offset offset of array
 * @return 8 bits of data
 */
#define get_field_1b(source, offset) 					   (source[offset])

/**
 * Sets 8-bit long field in target byte array
 * @param[out] target target byte array
 * @param[in] value input value
 * @param[in] offset offset of array
 */
#define set_field_1b(target, value, offset)				   (target[offset] = (value) & 0xFF)


/**
 * Reads Register's value
 * @param[in] reg register address in slimbus_drv_regs structure
 * @return register's value
 */
#define slimbus_drv_read_reg(reg)									   slimbus_drv_ps_uncached_read32((uint32_t*) &(instance->registers->reg))

/**
 * Writes Register's value
 * @param[in] reg register address in slimbus_drv_regs structure
 * @param[in] data register value
 */
#define slimbus_drv_write_reg(reg, data)							   slimbus_drv_ps_uncached_write32((uint32_t*) &(instance->registers->reg), (data))

/*
 * Structures
 */

typedef struct slimbus_drv_regs slimbus_drv_registers;

typedef struct {
	uintptr_t register_base;
	slimbus_drv_registers *registers;
	bool disable_hardware_crc_calculation;
	bool enumerate_devices;
	volatile bool sending_failed;
	volatile bool sending_finished;
	slimbus_drv_callbacks basic_callbacks;
	slimbus_message_callbacks message_callbacks;
} slimbus_drv_instance;

#endif

