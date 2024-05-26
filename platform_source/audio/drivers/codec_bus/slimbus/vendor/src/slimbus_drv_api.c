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

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/types.h>
#include "slimbus_drv_stdtypes.h"
#include "slimbus_drv_errno.h"

#include "slimbus_drv_api.h"
#include "slimbus_drv_types.h"
#include "slimbus_drv_sanity.h"
#include "slimbus_drv_regs.h"
#include "slimbus_drv_ps.h"
#include "slimbus_debug.h"
#include "slimbus_drv.h"

extern int slimbus_irq_state;
extern spinlock_t slimbus_spinlock;

/*
 * Forward declarations
 */

static uint32_t slimbus_drv_msg_assign_logical_address(void* pd, uint64_t destination_ea, uint8_t new_la);
static uint32_t slimbus_drv_msg_clear_information(void* pd, bool broadcast, uint8_t destination_la, uint16_t element_code, uint8_t* clear_mask, uint8_t clear_mask_size);

/*
 * Internal functions
 */

/**
 * Masks or unmasks int_en interrupts
 * @param[in] instance driver instance
 * @param[in] enabled mask(0) unmask(1) int_en interrupts
 */
static void set_int_en_enabled(slimbus_drv_instance *instance, bool enabled)
{
	uint32_t regval = 0;

	regval = slimbus_drv_read_reg(interrupts.int_en);
	INTERRUPTS__INT_EN__INT_EN__MODIFY(regval, enabled);
	slimbus_drv_write_reg(interrupts.int_en, regval);
}

/**
 * Convert slimbus_drv_slice_size enum to size [bytes]
 * @param[in] slice_size slice_size as enum
 * @return slice size in bytes
 */
static inline uint8_t slice_size_to_bytes(slimbus_drv_slice_size slice_size)
{
	switch (slice_size) {
	case SS_1_BYTE:
		return 1;
	case SS_2_BYTES:
		return 2;
	case SS_3_BYTES:
		return 3;
	case SS_4_BYTES:
		return 4;
	case SS_6_BYTES:
		return 6;
	case SS_8_BYTES:
		return 8;
	case SS_12_BYTES:
		return 12;
	case SS_16_BYTES:
		return 16;
	default:
	    slimbus_core_limit_err("slice size is invalid: %d\n", slice_size);
		return 0;
	}
}

/**
 * Fills memory area with zeroes
 * @param[in] mem_ptr pointer to memory
 * @param[in] size size of memory
 */
static void clear_memory(void *mem_ptr, uint16_t size)
{
	uint8_t* memory = (uint8_t*) mem_ptr;
	memset(memory, 0x00, size);
}

/**
 * Calculate Primary Integrity (CRC4)
 * @param[in] data pointer to message buffer
 * @param[in] offset offset
 * @param[in] data_len number of bytes to be used for checksum
 * @param[in] init_val initial value (should be 0)
 * @return CRC4
 */
static uint32_t crc_primary_integrity(uint8_t *data, uint8_t offset, uint8_t data_len, uint8_t init_val)
{
	uint8_t crc = init_val;
	uint8_t tmp0 = 0;
	uint8_t tmp1 = 0;
	uint8_t i, bit;

	for (i = offset; i < offset + data_len; i++) {
		for (bit = 0; bit < 8; bit++) {
			if ((i == data_len -1) && (bit == 4)) // Only lower four bits from last byte
				break;
			tmp0 = (((crc & 8) >> 3) ^ (((data[i] << bit) & 128) >> 7)) & 1;
			tmp1 = (((crc & 1) ^ tmp0) << 1) & 2;
			crc = ((crc << 1) & 12) | tmp1 | tmp0;
		}
	}
	return crc;
}

/**
 * Calculate Message Integrity (CRC4)
 * @param[in] data pointer to message buffer
 * @param[in] offset offset
 * @param[in] data_len number of bytes to be used for checksum
 * @param[in] init_val initial value (should be primary integrity)
 * @return CRC4
 */
static uint32_t crc_message_integrity(uint8_t *data, uint8_t offset, uint8_t data_len, uint8_t init_val)
{
	uint8_t crc = init_val;
	uint8_t tmp0 = 0;
	uint8_t tmp1 = 0;
	uint8_t i, bit;

	for (i = offset; i < offset + data_len; i++) {
		/* Only higher four bits from first byte */
		for (bit = (i == offset) ? 4 : 0; bit < 8; bit++) {
			tmp0 = (((crc & 8) >>3 ) ^ (((data[i] << bit) & 128) >> 7)) & 1;
			tmp1 = (((crc & 1) ^ tmp0) << 1) & 2;
			crc = (((crc << 1) & 12) | tmp1 | tmp0) & 0xFF;
		}
	}
	return crc;
}

/**
 * Decode message - convert array of bytes to structure
 * @param[in] instance driver instance
 * @param[in] received_message buffer with message from TX_FIFO
 * @param[in] message pointer to allocated struct to be filled with contents of message
 * @return 0 if OK
 * @return EINVAL if received message has errors (invalid length, CRC errors etc.)
 */
static uint32_t decode_message(slimbus_drv_instance *instance, uint8_t *received_message, uint8_t message_len, slimbus_drv_message *message)
{
	uint8_t i;
	uint8_t remaining_length = 0;
	uint8_t length_arbitration_field = 0;
	uint8_t length_header_field = 0;
	uint8_t offset = 0;

	/**
	 *	Arbitration Field: 2 or 7 bytes
	 *	BYTE  0:	Arbitration Type, Arbitration Extension, Arbitration Priority
	 *	BYTE  1:	Source Logical Address / Enumeration Address [7:0]
	 *	BYTES 2-6:	NA / Source Enumeration Address [47:8]
	 */
	message->arbitration_type = get_msg_field(received_message, offset, ARBITRATION_TYPE);
	switch (message->arbitration_type) {
	case AT_LONG:
		message->source_address = get_field_6b(received_message, get_msg_field_offset(SOURCE_ADDRESS));
		length_arbitration_field = MESSAGE_ARBITRATION_LENGTH_LONG;
		break;
	case AT_SHORT:
		message->source_address = get_field_1b(received_message, get_msg_field_offset(SOURCE_ADDRESS));
		length_arbitration_field = MESSAGE_ARBITRATION_LENGTH_SHORT;
		break;
	default:
		slimbus_core_limit_err("arbitration type is invalid: %d\n", message->arbitration_type);
		return EINVAL;
	}
	message->arbitration_priority = get_msg_field(received_message, offset, ARBITRATION_PRIORITY);

	/*
	 *	Header Field : 3, 4 or 9 bytes
	 *	BYTE  0:	Message Type, Remaining Length
	 *	BYTE  1:	Message Code
	 *	BYTE  2:	Destination Type, Primary Integrity
	 *	BYTE  3:	Destination Logical Address / Enumeration Address [7:0]
	 *	BYTES 4-8:	NA / Destination Enumeration Address [47:8]
	 */
	offset = length_arbitration_field;

	message->message_type = get_msg_field(received_message, offset, MESSAGE_TYPE);
	remaining_length = get_msg_field(received_message, offset, REMAINING_LENGTH);
	message->message_code = get_msg_field(received_message, offset, MESSAGE_CODE);
	message->destination_type = get_msg_field(received_message, offset, MESSAGE_DESTINATION_TYPE);

	switch (message->destination_type) {
	case DT_BROADCAST:
		length_header_field = MESSAGE_HEADER_LENGTH_BROADCAST;
		message->destination_address = 0x0;
		break;

	case DT_LOGICAL_ADDRESS:
		length_header_field = MESSAGE_HEADER_LENGTH_SHORT;
		message->destination_address = get_field_1b(received_message, offset + get_msg_field_offset(DESTINATION_ADDRESS));
		break;

	case DT_ENUMERATION_ADDRESS:
		length_header_field = MESSAGE_HEADER_LENGTH_LONG;
		message->destination_address = get_field_6b(received_message, offset + get_msg_field_offset(DESTINATION_ADDRESS));
		break;

	default:
		slimbus_core_limit_err("destination type is invalid: %d\n", message->destination_type);
		return EINVAL;
	}

	/* Payload length */
	offset = length_arbitration_field + length_header_field;

	if (remaining_length > (MESSAGE_PAYLOAD_MAX_LENGTH + length_header_field)) {
		slimbus_core_limit_err("remaining length is invalid: %d, %d\n", remaining_length, (MESSAGE_PAYLOAD_MAX_LENGTH + length_header_field));
		return EINVAL;
	}

	message->payload_length = remaining_length - length_header_field;

	/*
	 * Message Integrity and Response Field
	 * BYTE 0:		Message Integrity, Message Response
	 */
	message->response = get_msg_field(received_message, (offset + message->payload_length), RESPONSE_CODE);
	if (message->response != MR_POSITIVE_ACK)
		slimbus_core_limit_err("SLIMbus received message not PACK:%#x ! \n", message->response);//fixme: by lyq

	/* Payload data */
	for (i = 0; i < message->payload_length; i++)
		message->payload[i] = get_field_1b(received_message, offset + i);

	return 0;
}

/**
 * Encode message - convert structure to array of bytes
 * @param[in] instance driver instance
 * @param[out] memory_to_fill allocated memory buffer for TX_FIFO, must be filled with zeroes
 * @param[in] message struct with message
 * @return 0 if message has errors
 * @return size of generated message [bytes] if OK
 */
static uint32_t encode_message(slimbus_drv_instance *instance, uint8_t *memory_to_fill, slimbus_drv_message *message)
{
	uint8_t i;
	uint8_t remaining_length = 0;
	uint8_t primary_integrity = 0;
	uint8_t message_integrity = 0;
	uint8_t offset = 0;
	uint8_t length_arbitration_field = 0;
	uint8_t length_header_field = 0;
	uint8_t primary_integrity_bytes = 0;

	/* Arbitration Field: 2 or 7 bytes */
	set_msg_field(memory_to_fill, message->arbitration_type, offset, ARBITRATION_TYPE);

	switch (message->arbitration_type) {
	case AT_LONG:
		set_field_6b(memory_to_fill, message->source_address, get_msg_field_offset(SOURCE_ADDRESS));
		length_arbitration_field = MESSAGE_ARBITRATION_LENGTH_LONG;
		break;
	case AT_SHORT:
		set_field_1b(memory_to_fill, message->source_address, get_msg_field_offset(SOURCE_ADDRESS));
		length_arbitration_field = MESSAGE_ARBITRATION_LENGTH_SHORT;
		break;
	default:
		slimbus_core_limit_err("arbitration type is invalid: %d\n", message->arbitration_type);
		return 0;
	}
	set_msg_field(memory_to_fill, message->arbitration_priority, offset, ARBITRATION_PRIORITY);
	set_msg_field(memory_to_fill, 0, offset, ARBITRATION_EXTENSION);

	/* Header Field: 3, 4 or 9 bytes */
	offset = length_arbitration_field;

	set_msg_field(memory_to_fill, message->message_type, offset, MESSAGE_TYPE);
	set_msg_field(memory_to_fill, message->message_code, offset, MESSAGE_CODE);
	set_msg_field(memory_to_fill, message->destination_type, offset, MESSAGE_DESTINATION_TYPE);

	switch (message->destination_type) {
	case DT_BROADCAST:
		length_header_field = MESSAGE_HEADER_LENGTH_BROADCAST;
		break;

	case DT_LOGICAL_ADDRESS:
		length_header_field = MESSAGE_HEADER_LENGTH_SHORT;
		set_field_1b(memory_to_fill, message->destination_address, offset + get_msg_field_offset(DESTINATION_ADDRESS));
		break;

	case DT_ENUMERATION_ADDRESS:
		length_header_field = MESSAGE_HEADER_LENGTH_LONG;
		set_field_6b(memory_to_fill, message->destination_address, offset + get_msg_field_offset(DESTINATION_ADDRESS));
		break;

	default:
		slimbus_core_limit_err("destination type is invalid: %d\n", message->destination_type);
		return 0;
	}

	remaining_length = message->payload_length + length_header_field;
	set_msg_field(memory_to_fill, remaining_length, offset, REMAINING_LENGTH);

	/* Primary Integrity */
	if (instance->disable_hardware_crc_calculation) {
		primary_integrity_bytes = offset + get_msg_field_offset(DESTINATION_ADDRESS);
		primary_integrity = crc_primary_integrity(memory_to_fill, 0, primary_integrity_bytes, 0);
		set_msg_field(memory_to_fill, primary_integrity, offset, PRIMARY_INTEGRITY);
	}

	/* Payload data */
	offset = length_arbitration_field + length_header_field;
	for (i = 0; i < message->payload_length; i++)
		set_field_1b(memory_to_fill, message->payload[i], offset + i);

	/* Message integrity */
	if (instance->disable_hardware_crc_calculation) {
		message_integrity = crc_message_integrity(memory_to_fill, primary_integrity_bytes -1, remaining_length - 3 + 1, primary_integrity);
		set_msg_field(memory_to_fill, message_integrity, (offset + message->payload_length), MESSAGE_INTEGRITY);
	}
	return length_arbitration_field + length_header_field + message->payload_length + 1;

}

static uint32_t fifo_wait_reception(slimbus_drv_instance *instance, _Bool wait)
{
	uint32_t timeout = 0;
	uint32_t reg;

	/* If wait == 1 then function waits for confirmation of message reception */
	if (wait) {
		for (;;) {
			/* If message was transmitted in an interrupt (e.g. from callback),
			sending* variables won't be set by another interrupt. In that case polling is used. */
			reg = slimbus_drv_read_reg(interrupts.interrupt);
			if (INTERRUPTS__INT__TX_ERR__READ(reg)) {
				if (slimbus_int_need_clear_get()) {
					reg = 0;
					INTERRUPTS__INT__TX_ERR__SET(reg);
					slimbus_drv_write_reg(interrupts.interrupt, reg);
				}
				slimbus_core_limit_err("SLIMbus send message TX_ERR happened!\n");
				return EIO;
			}

			if (INTERRUPTS__INT__TX_INT__READ(reg)) {
				if (slimbus_int_need_clear_get()) {
					reg = 0;
					INTERRUPTS__INT__TX_INT__SET(reg);
					slimbus_drv_write_reg(interrupts.interrupt, reg);
				}
				return 0;
			}

			/* slimbus_irq will read and clear interrupt flag before it will be accessed here by polling.
			These variables are set by interrupt handling function. */
			if (instance->sending_failed) {
				slimbus_core_limit_err("SLIMbus send message failed!\n");
				return EIO;
			}

			if (instance->sending_finished)
				return 0;

			timeout++;
			if (timeout > FIFO_DELAY_TIMEOUT) {
				udelay(200);
				if (timeout == FIFO_WAIT_TIMEOUT) {
					slimbus_core_limit_err("SLIMbus send message timeout!\n");
					return EIO;
				}
			}
		}
	}

	return 0;
}

/**
 * Transmit data to TX_FIFO
 * @param[in] instance driver instance
 * @param[in] tx_fifo_data pointer to the data buffer
 * @param[in] tx_fifo_data_size size of data to transmit [bytes]
 * @param[in] wait wait until message is sent (TX_ERR interrupt must be enabled!)
 * @return 0 if OK
 * @return EIO if not OK
 */
static uint32_t fifo_transmit(slimbus_drv_instance *instance, uint8_t *tx_fifo_data, uint8_t tx_fifo_data_size, _Bool wait)
{
	uint8_t i;
	uint8_t regs_to_write = 0;
	uint32_t *tx_fifo_data_32b = (uint32_t *) tx_fifo_data;
	uint32_t reg;
	uint32_t timeout = 0;
	uint32_t ret = 0;

	do { //If TX_FIFO is full, wait until it's not
		reg = slimbus_drv_read_reg(command_status.state);
		timeout++;
		if (timeout > FIFO_DELAY_TIMEOUT) {
			udelay(200);
			if (timeout == FIFO_WAIT_TIMEOUT) {
				slimbus_core_limit_err("SLIMbus send message wait TX_FULL timeout!\n");
				return EIO;
			}
		}
	} while ((COMMAND_STATUS__STATE__TX_FULL__READ(reg)) == 1);

	if (slimbus_int_need_clear_get())
		set_int_en_enabled(instance, 0);

	timeout = 0;
	instance->sending_failed = 0;
	instance->sending_finished = 0;
	slimbus_irq_state = 0;
	regs_to_write = (tx_fifo_data_size >> 2) + ((tx_fifo_data_size & 0x3) != 0); //regs_to_write = tx_fifo_data_size / 4 + 1 if tx_fifo_data_size % 4 > 0

	for (i = 0; i < regs_to_write; i++, tx_fifo_data_32b++)
		slimbus_drv_write_reg(message_fifos.mc_fifo[i], *tx_fifo_data_32b);

	slimbus_drv_write_reg(command_status.command, BIT(COMMAND_STATUS__COMMAND__TX_PUSH__SHIFT));

	do { //If TX_PUSH command is still in progress, wait until it's finished
		reg = slimbus_drv_read_reg(command_status.state);
		timeout++;
		if (timeout > FIFO_DELAY_TIMEOUT) {
			udelay(200);
			if (timeout == FIFO_WAIT_TIMEOUT) {
				slimbus_core_limit_err("SLIMbus send message wait TX_PUSH timeout!\n");
				ret = EIO;
				goto exit;
			}
		}
	} while (COMMAND_STATUS__STATE__TX_PUSH__READ(reg));

	ret = fifo_wait_reception(instance, wait);

exit:
	if (slimbus_int_need_clear_get())
		set_int_en_enabled(instance, 1);

	return ret;
}

/**
 * Receive data from RX_FIFO
 * @param[in] instance driver instance
 * @param[out] rx_fifo_data pointer to memory to be filled with received data
 * @param[in] rx_fifo_max_size max size of data buffer
 * @return size of received data if OK
 * @return 0 if message overflowed or received message is bigger than available data buffer
 */
static uint32_t fifo_receive(slimbus_drv_instance *instance, uint8_t *rx_fifo_data, uint8_t rx_fifo_max_size)
{
	uint32_t rx_fifo_flag = 0;
	uint8_t rx_fifo_msg_size = 0;
	uint8_t i;
	uint8_t regs_to_read = 0;
	uint8_t result = 0;
	uint32_t *rx_fifo_data_32b = (uint32_t *) rx_fifo_data;
	rx_fifo_flag = slimbus_drv_read_reg(message_fifos.mc_fifo[RX_FIFO_FLAG_OFFSET]);

	if (rx_fifo_flag & (1 << RX_FIFO_FLAG_RX_OVERFLOW)) { //Message Overflow
		result = 0;
		slimbus_core_limit_err("SLIMbus RX_FIFO Message Overflow! \n");
		goto rx_fifo_end;
	}

	rx_fifo_msg_size = rx_fifo_flag & RX_FIFO_FLAG_RX_MSG_MASK;
	if (rx_fifo_msg_size > rx_fifo_max_size) {
		result = 0;
		goto rx_fifo_end;
	}
	regs_to_read = (rx_fifo_msg_size >> 2) + ((rx_fifo_msg_size & 0x3) != 0); //regs_to_read = rx_fifo_msg_size / 4 + 1 if rx_fifo_size % 4 > 0

	for (i = 0; i < regs_to_read; i++, rx_fifo_data_32b++)
		*rx_fifo_data_32b = slimbus_drv_read_reg(message_fifos.mc_fifo[i]);

	result = rx_fifo_msg_size;

rx_fifo_end:
	slimbus_drv_write_reg(command_status.command, BIT(COMMAND_STATUS__COMMAND__RX_PULL__SHIFT));
	return result;
}

/**
 * Transmit SLIMBus message
 * @param[in] instance driver instance
 * @param[in] message pointer to message struct
 * @return 0 if OK
 * @return EINVAL if message structure is invalid
 * @return EIO if an error occurred while sending data to TX_FIFO
 */
static uint32_t transmit_message(slimbus_drv_instance *instance, slimbus_drv_message *message)
{
	uint8_t tx_fifo_data[TX_FIFO_MSG_MAX_SIZE];
	uint8_t fifo_size = 0;
	uint32_t result = 0;
	unsigned long flag = 0;

	clear_memory((void*) tx_fifo_data, TX_FIFO_MSG_MAX_SIZE);

	if (instance->basic_callbacks.on_message_sending != NULL)
		instance->basic_callbacks.on_message_sending((void *) instance, message);

	fifo_size = encode_message(instance, tx_fifo_data, message);
	if (fifo_size == 0) {
		slimbus_core_limit_err("encode message fail\n");
		return EINVAL;
	}

	if (instance->basic_callbacks.on_raw_message_sending != NULL)
		instance->basic_callbacks.on_raw_message_sending((void *) instance, (void *) tx_fifo_data, fifo_size);

	spin_lock_irqsave(slimbus_drv_get_spinlock(), flag);
	result = fifo_transmit(instance, tx_fifo_data, fifo_size, 1);
	spin_unlock_irqrestore(slimbus_drv_get_spinlock(), flag);
	if (result) {
		slimbus_core_limit_err("fifo transmit fail\n");
		return EIO;
	}

	return 0;
}

/**
 * Assign logical address to a discovered device
 * @param[in] instance driver instance
 * @param[in] message received REPORT_PRESENT message
 * @return 0 if OK
 * @return EIO if there was an IO error
 * @return EINVAL if AssignLogicalAddressHandler is invalid
 */
static inline uint32_t assign_logical_address(slimbus_drv_instance *instance, slimbus_drv_message *message)
{
	uint8_t logical_addr = 0x0;
	if (instance->basic_callbacks.on_assign_logical_address == NULL) {
		slimbus_core_limit_err("instance->basic_callbacks.on_assign_logical_address is NULL\n");
		return EINVAL;
	}

	logical_addr = instance->basic_callbacks.on_assign_logical_address(message->source_address, message->payload[0]);
	if (logical_addr > 0xEF) { //Reserved value was used
		slimbus_core_limit_err("logical addr is invalid: %x\n", logical_addr);
		return EINVAL;
	}
	return slimbus_drv_msg_assign_logical_address((void *) instance, message->source_address, logical_addr);
}

/**
 * Check Received Information Element for errors and possible callbacks
 * Also clears received information elements.
 * @param[in] instance driver instance
 * @param[in] message received REPORT_INFORMATION message
 * @return 0 if OK
 * @return EINVAL if invalid message, or invalid device class was received
 */
static inline uint32_t check_information_element(slimbus_drv_instance *instance, slimbus_drv_message *message)
{
	uint16_t element_code;
	uint16_t ec_byte_address;
	bool ec_access_byte_based = false;       // 1 - Byte-based Access, 0 - Elemental Access
	slimbus_drv_slice_size ec_slice_size;    // For Byte-based Access
	uint8_t ec_bit_number = 0;               // For Elemental Access  //fixme: by lyq
	slimbus_device_class device_class;
	slimbus_information_elements ie;
	uint8_t i;
	uint8_t* information_slice = NULL;
	uint8_t information_slice_len;

	if (message->message_code != MESSAGE_CODE_REPORT_INFORMATION) {
		slimbus_core_limit_err("message code is invalid: %d\n", message->message_code);
		return EINVAL;
	}

	/*
	 * Payload:
	 * 0	EC		 [7:0]
	 * 1	EC		[15:8]
	 * 2	IS		 [7:0]
	 * N+1	IS [8N-1:8N-8]
	 */
	element_code = message->payload[0] | (message->payload[1] << 8);
	ec_access_byte_based = get_msg_payload_field(ELEMENT_CODE_ACCESS_TYPE, element_code);
	ec_byte_address = get_msg_payload_field(ELEMENT_CODE_BYTE_ADDRESS, element_code);
	if (ec_access_byte_based) {
		ec_slice_size = get_msg_payload_field(ELEMENT_CODE_SLICE_SIZE, element_code);
		if (slice_size_to_bytes(ec_slice_size) != (message->payload_length - 2)) {
			slimbus_core_limit_err("payload don't match witch slice size: %d, %d\n", slice_size_to_bytes(ec_slice_size), (message->payload_length - 2));
			return EINVAL; //Received Slice Size is different than payload
		}
	} else {
		ec_bit_number = get_msg_payload_field(ELEMENT_CODE_BIT_NUMBER, element_code);
	}

	information_slice = &(message->payload[ELEMENT_CODE_LENGTH]);
	information_slice_len = message->payload_length - ELEMENT_CODE_LENGTH;

	if (information_slice_len == 0) {
		slimbus_core_limit_err("information slice len is zero\n");
		return EINVAL;
	}

	clear_memory((void *) &ie, sizeof(slimbus_information_elements));

	if (ec_byte_address >= IE_CLASS_SPECIFIC_OFFSET) {
		device_class = instance->basic_callbacks.on_device_class_request(message->source_address);
		switch (device_class) {
		case DC_MANAGER:
			if (ec_access_byte_based) {
				for (i = 0; i < information_slice_len; i++) {
					get_ie_byte_access(ie.manager_active_manager, MANAGER_ACTIVE_MANAGER, ec_byte_address + i, information_slice[i]);
					clear_ie_byte_access(MANAGER_ACTIVE_MANAGER, ec_byte_address + i, information_slice[i]);
				}
			} else {
				get_ie_elemental_access(ie.manager_active_manager, MANAGER_ACTIVE_MANAGER, ec_byte_address, ec_bit_number, information_slice[0]);
				clear_ie_elemental_access(MANAGER_ACTIVE_MANAGER, ec_byte_address, ec_bit_number, information_slice[0]);
			}
			break;

		case DC_FRAMER:
			if (ec_access_byte_based) {
				for (i = 0; i < information_slice_len; i++) {
					get_ie_byte_access(ie.framer_gc_tx_col, FRAMER_GC_TX_COL, ec_byte_address + i, information_slice[i]);
					clear_ie_byte_access(FRAMER_GC_TX_COL, ec_byte_address + i, information_slice[i]);

					get_ie_byte_access(ie.framer_fi_tx_col, FRAMER_FI_TX_COL, ec_byte_address + i, information_slice[i]);
					clear_ie_byte_access(FRAMER_FI_TX_COL, ec_byte_address + i, information_slice[i]);

					get_ie_byte_access(ie.framer_fs_tx_col, FRAMER_FS_TX_COL, ec_byte_address + i, information_slice[i]);
					clear_ie_byte_access(FRAMER_FS_TX_COL, ec_byte_address + i, information_slice[i]);

					get_ie_byte_access(ie.framer_active_framer, FRAMER_ACTIVE_FRAMER, ec_byte_address + i, information_slice[i]);
					clear_ie_byte_access(FRAMER_ACTIVE_FRAMER, ec_byte_address + i, information_slice[i]);

					get_ie_byte_access(ie.framer_quality, FRAMER_QUALITY, ec_byte_address + i, information_slice[i]);
					clear_ie_byte_access(FRAMER_QUALITY, ec_byte_address + i, information_slice[i]);
				}
			} else {
				get_ie_elemental_access(ie.framer_gc_tx_col, FRAMER_GC_TX_COL, ec_byte_address, ec_bit_number, information_slice[0]);
				clear_ie_elemental_access(FRAMER_GC_TX_COL, ec_byte_address, ec_bit_number, information_slice[0]);

				get_ie_elemental_access(ie.framer_fi_tx_col, FRAMER_FI_TX_COL, ec_byte_address, ec_bit_number, information_slice[0]);
				clear_ie_elemental_access(FRAMER_FI_TX_COL, ec_byte_address, ec_bit_number, information_slice[0]);

				get_ie_elemental_access(ie.framer_fs_tx_col, FRAMER_FS_TX_COL, ec_byte_address, ec_bit_number, information_slice[0]);
				clear_ie_elemental_access(FRAMER_FS_TX_COL, ec_byte_address, ec_bit_number, information_slice[0]);

				get_ie_elemental_access(ie.framer_active_framer, FRAMER_ACTIVE_FRAMER, ec_byte_address, ec_bit_number, information_slice[0]);
				clear_ie_elemental_access(FRAMER_ACTIVE_FRAMER, ec_byte_address, ec_bit_number, information_slice[0]);

				get_ie_elemental_access(ie.framer_quality, FRAMER_QUALITY, ec_byte_address, ec_bit_number, information_slice[0]);
				clear_ie_elemental_access(FRAMER_QUALITY, ec_byte_address, ec_bit_number, information_slice[0]);
			}
			break;

		case DC_INTERFACE:
			if (ec_access_byte_based) {
				for (i = 0; i < information_slice_len; i++) {
					get_ie_byte_access(ie.interface_data_slot_overlap, INTERFACE_DATA_SLOT_OVERLAP, ec_byte_address + i, information_slice[i]);
					clear_ie_byte_access(INTERFACE_DATA_SLOT_OVERLAP, ec_byte_address + i, information_slice[i]);

					get_ie_byte_access(ie.interface_lost_ms, INTERFACE_LOST_MS, ec_byte_address + i, information_slice[i]);
					clear_ie_byte_access(INTERFACE_LOST_MS, ec_byte_address + i, information_slice[i]);

					get_ie_byte_access(ie.interface_lost_sfs, INTERFACE_LOST_SFS, ec_byte_address + i, information_slice[i]);
					clear_ie_byte_access(INTERFACE_LOST_SFS, ec_byte_address + i, information_slice[i]);

					get_ie_byte_access(ie.interface_lost_fs, INTERFACE_LOST_FS, ec_byte_address + i, information_slice[i]);
					clear_ie_byte_access(INTERFACE_LOST_FS, ec_byte_address + i, information_slice[i]);

					get_ie_byte_access(ie.interface_mc_tx_col, INTERFACE_MC_TX_COL, ec_byte_address + i, information_slice[i]);
					clear_ie_byte_access(INTERFACE_MC_TX_COL, ec_byte_address + i, information_slice[i]);
				}
			} else {
				get_ie_elemental_access(ie.interface_data_slot_overlap, INTERFACE_DATA_SLOT_OVERLAP, ec_byte_address, ec_bit_number, information_slice[0]);
				clear_ie_elemental_access(INTERFACE_DATA_SLOT_OVERLAP, ec_byte_address, ec_bit_number, information_slice[0]);

				get_ie_elemental_access(ie.interface_lost_ms, INTERFACE_LOST_MS, ec_byte_address, ec_bit_number, information_slice[0]);
				clear_ie_elemental_access(INTERFACE_LOST_MS, ec_byte_address, ec_bit_number, information_slice[0]);

				get_ie_elemental_access(ie.interface_lost_sfs, INTERFACE_LOST_SFS, ec_byte_address, ec_bit_number, information_slice[0]);
				clear_ie_elemental_access(INTERFACE_LOST_SFS, ec_byte_address, ec_bit_number, information_slice[0]);

				get_ie_elemental_access(ie.interface_lost_fs, INTERFACE_LOST_FS, ec_byte_address, ec_bit_number, information_slice[0]);
				clear_ie_elemental_access(INTERFACE_LOST_FS, ec_byte_address, ec_bit_number, information_slice[0]);

				get_ie_elemental_access(ie.interface_mc_tx_col, INTERFACE_MC_TX_COL, ec_byte_address, ec_bit_number, information_slice[0]);
				clear_ie_elemental_access(INTERFACE_MC_TX_COL, ec_byte_address, ec_bit_number, information_slice[0]);
			}
			break;

		case DC_GENERIC:
			break;

		default:
			slimbus_core_limit_err("device class is invalide: %d\n", device_class);
			return EINVAL;
		}
	} else {
		if (ec_access_byte_based) {
			for (i = 0; i < information_slice_len; i++) { //Information Slice is bytes 2 and up of the Payload, loop through all received bytes
				get_ie_byte_access(ie.core_ex_error, CORE_EX_ERROR, ec_byte_address + i, information_slice[i]);
				clear_ie_byte_access(CORE_EX_ERROR, ec_byte_address + i, information_slice[i]);

				get_ie_byte_access(ie.core_reconfig_objection, CORE_RECONFIG_OBJECTION, ec_byte_address + i, information_slice[i]);
				clear_ie_byte_access(CORE_RECONFIG_OBJECTION, ec_byte_address + i, information_slice[i]);

				get_ie_byte_access(ie.core_data_tx_col, CORE_DATA_TX_COL, ec_byte_address + i, information_slice[i]);
				clear_ie_byte_access(CORE_DATA_TX_COL, ec_byte_address + i, information_slice[i]);

				get_ie_byte_access(ie.core_unsprtd_msg, CORE_UNSPRTD_MSG, ec_byte_address + i, information_slice[i]);
				clear_ie_byte_access(CORE_UNSPRTD_MSG, ec_byte_address + i, information_slice[i]);

				clear_ie_byte_access(CORE_DEVICE_CLASS, ec_byte_address + i, information_slice[i]);
				clear_ie_byte_access(CORE_DEVICE_CLASS_VERSION, ec_byte_address + i, information_slice[i]);
			}
		} else {	//Information Slice is 1 Information Element
			get_ie_elemental_access(ie.core_ex_error, CORE_EX_ERROR, ec_byte_address, ec_bit_number, information_slice[0]);
			clear_ie_elemental_access(CORE_EX_ERROR, ec_byte_address, ec_bit_number, information_slice[0]);

			get_ie_elemental_access(ie.core_reconfig_objection, CORE_RECONFIG_OBJECTION, ec_byte_address, ec_bit_number, information_slice[0]);
			clear_ie_elemental_access(CORE_RECONFIG_OBJECTION, ec_byte_address, ec_bit_number, information_slice[0]);

			get_ie_elemental_access(ie.core_data_tx_col, CORE_DATA_TX_COL, ec_byte_address, ec_bit_number, information_slice[0]);
			clear_ie_elemental_access(CORE_DATA_TX_COL, ec_byte_address, ec_bit_number, information_slice[0]);

			get_ie_elemental_access(ie.core_unsprtd_msg, CORE_UNSPRTD_MSG, ec_byte_address, ec_bit_number, information_slice[0]);
			clear_ie_elemental_access(CORE_UNSPRTD_MSG, ec_byte_address, ec_bit_number, information_slice[0]);

			clear_ie_elemental_access(CORE_DEVICE_CLASS, ec_byte_address, ec_bit_number, information_slice[0]);
			clear_ie_elemental_access(CORE_DEVICE_CLASS_VERSION, ec_byte_address, ec_bit_number, information_slice[0]);
		}
	}

	if (instance->basic_callbacks.on_information_element_reported!= NULL)
		instance->basic_callbacks.on_information_element_reported((void *) instance, message->source_address, &ie);

	//Clear received Information Element
	slimbus_drv_msg_clear_information((void *) instance, 0, message->source_address, element_code, information_slice, information_slice_len);

	return 0;
}

/**
 * Process received message.
 * Calls callbacks (if assigned)
 * Calls decoding and clear of reported information element
 * @param[in] instance driver instance
 * @param[in] message received message
 * @return 0 if OK
 * @return EIO if there was an IO error(s)
 * @return EINVAL if invalid message was received
 * @return ENOMEM if internal reply has been received, but previous one has not been acknowledged
 */
static inline uint32_t process_received_message(slimbus_drv_instance *instance, slimbus_drv_message *message)
{
	switch (message->message_code) {
	case MESSAGE_CODE_REPORT_PRESENT:		   //Payload: 0 - Device Class code, 1 - Device Class Version
		if (instance->message_callbacks.on_msg_report_present != NULL)
			instance->message_callbacks.on_msg_report_present((void *) instance, message->source_address, message->payload[0], message->payload[1]);

		if (instance->enumerate_devices)
			return assign_logical_address(instance, message);
		break;

	case MESSAGE_CODE_REPORT_ABSENT:		   //Payload: None
		if (instance->message_callbacks.on_msg_report_absent != NULL)
			instance->message_callbacks.on_msg_report_absent((void *) instance, message->source_address);
		break;

	case MESSAGE_CODE_REPLY_INFORMATION:	   //Payload: 0 - Transaction ID, 1 => 16 - Information Slice
		if (instance->message_callbacks.on_msg_reply_information != NULL)
			instance->message_callbacks.on_msg_reply_information((void *) instance, message->source_address, message->payload[0], &(message->payload[TRANSACTION_ID_LENGTH]), message->payload_length - TRANSACTION_ID_LENGTH);
		break;

	case MESSAGE_CODE_REPORT_INFORMATION:	   //Payload: 0 - Element Code [7:0], 1 - Element Code [15:8] 2 => 17 - Information Slice
		if (instance->message_callbacks.on_msg_report_information != NULL)
			instance->message_callbacks.on_msg_report_information((void *) instance, message->source_address, message->payload[0] | (message->payload[1] << 8), &(message->payload[ELEMENT_CODE_LENGTH]), message->payload_length - ELEMENT_CODE_LENGTH);
		check_information_element(instance, message);
		break;

	case MESSAGE_CODE_REPLY_VALUE: 		   //Payload: 0 - Transaction ID, 1 => 16 - Value Slice
		if (instance->message_callbacks.on_msg_reply_value != NULL)
			instance->message_callbacks.on_msg_reply_value((void *) instance, message->source_address, message->payload[0], &(message->payload[TRANSACTION_ID_LENGTH]), message->payload_length - TRANSACTION_ID_LENGTH);
		break;

	default:
		slimbus_core_limit_err("mesage code is invalid: %d\n", message->message_code);
		return EINVAL;

	}
	return 0;
}

/**
 * Receive all SLIMbus messages from RX_FIFO
 * @param[in] instance driver instance
 * @return 0 if OK
 * @return EIO if there was an error(s)
 */
static inline uint8_t receive_messages(slimbus_drv_instance *instance)
{
	uint8_t rx_fifo_data[RX_FIFO_MSG_MAX_SIZE];
	uint8_t fifo_size = 0;
	uint32_t reg = 0;
	uint8_t errors = 0;
	slimbus_drv_message rx_msg;
	uint8_t count = 0;

	for (;;) {
		count++;
		if (count % CYCLES_TIME == 0)
			slimbus_core_limit_err("count is %d, rx pull state is %d\n",
				count, COMMAND_STATUS__STATE__RX_NOTEMPTY__READ(reg));
		reg = slimbus_drv_read_reg(command_status.state);
		if (COMMAND_STATUS__STATE__RX_PULL__READ(reg))
			continue;

		if (COMMAND_STATUS__STATE__RX_NOTEMPTY__READ(reg) == 0)
			return 0;
		if (count % CYCLES_TIME == 0)
			slimbus_core_limit_err("begin receive data from rx fifo\n");

		fifo_size = fifo_receive(instance, rx_fifo_data, RX_FIFO_MSG_MAX_SIZE);
		if ((fifo_size > 0) && (fifo_size <= MESSAGE_MAX_LENGTH)) {
			if (instance->basic_callbacks.on_raw_message_received != NULL)
				instance->basic_callbacks.on_raw_message_received((void *) instance, (void *) rx_fifo_data, fifo_size);

			if (count % CYCLES_TIME == 0)
				slimbus_core_limit_err("fifosize is %d\n", fifo_size);

			errors += decode_message(instance, rx_fifo_data, fifo_size, &rx_msg);
			if (errors)
				slimbus_core_limit_err("decode message fail\n");

			if (instance->basic_callbacks.on_message_received != NULL)
				instance->basic_callbacks.on_message_received((void *) instance, &rx_msg);

			errors += (process_received_message(instance, &rx_msg) != 0);
			if (errors)
				slimbus_core_limit_err("process message fail\n");
		}
	}
	return errors ? EIO : 0;
}

/**
 * Wait until previous configuration is set (CFG_STROBE bit is cleared)
 * @param[in] instance driver instance
 * @return 0 if OK
 * @return EIO if polling CFG_STROBE timeouted
 */
static uint32_t cfg_strobe_check(slimbus_drv_instance *instance)
{
	uint32_t reg = 0;
	uint32_t timeout_count = 0;

	reg = slimbus_drv_read_reg(configuration.config_mode);
	/* If Manager component is disabled do not poll CFG_STROBE bit */
	if ((CONFIGURATION__CONFIG_MODE__ENABLE__READ(reg)) == 0)
		return 0;

	reg = slimbus_drv_read_reg(command_status.command);
	while (COMMAND_STATUS__COMMAND__CFG_STROBE__READ(reg)) {
		msleep(1);
		reg = slimbus_drv_read_reg(command_status.command);
		if (++timeout_count == 100) {
			slimbus_core_limit_err("cfg strobe check timeout\n");
			return EIO;
		}
	}

	return 0;
}

/**
 * Set CFG_STROBE bit to apply changes in CONFIG Registers
 * @param[in] instance driver instance
 * @param[in] force force CFG_STROBE regardless of the ENABLE bit
 */
static void cfg_strobe_set(slimbus_drv_instance *instance, bool force)
{
	uint32_t reg = 0;
	if (!force) {
		reg = slimbus_drv_read_reg(configuration.config_mode);
		if ((CONFIGURATION__CONFIG_MODE__ENABLE__READ(reg)) == 0)
			return;
	}

	slimbus_drv_write_reg(command_status.command, BIT(COMMAND_STATUS__COMMAND__CFG_STROBE__SHIFT));
}

static uint32_t slimbus_drv_probe(const slimbus_drv_config* config, uint16_t* required_memory)
{
	uint32_t result = slimbus_probe_sanity(config, required_memory);
	if (result) {
		slimbus_core_limit_err("check sanity fail\n");
		return result;
	}

	*required_memory = sizeof(slimbus_drv_instance);
	return 0;
}

static uint32_t slimbus_drv_api_init(void* pd, const slimbus_drv_config* config, slimbus_drv_callbacks* callbacks)
{
	slimbus_drv_instance* instance = NULL;
	uint32_t reg;

	uint32_t result = slimbus_init_sanity(pd, config, callbacks);
	if (result) {
		slimbus_core_limit_err("check sanity fail\n");
		return result;
	}

	if (callbacks->on_device_class_request == NULL) {
		slimbus_core_limit_err("device class request callback is NULL\n");
		return EINVAL;
	}

	if ((callbacks->on_assign_logical_address == NULL) && (config->enumerate_devices == 1)) {
		slimbus_core_limit_err("assign logical address callback is NULL\n");
		return EINVAL;
	}

	if (config->ea_interface_id == config->ea_generic_id) {
		slimbus_core_limit_err("config->ea_interface_id == config->ea_generic_id\n");
		return EINVAL;
	}

	if (config->ea_interface_id == config->ea_framer_id) {
		slimbus_core_limit_err("config->ea_interface_id == config->ea_framer_id\n");
		return EINVAL;
	}

	if (config->ea_generic_id == config->ea_framer_id) {
		slimbus_core_limit_err("config->ea_generic_id == config->ea_framer_id\n");
		return EINVAL;
	}

	instance = (slimbus_drv_instance*) pd;
	instance->register_base = config->reg_base;
	instance->registers = (slimbus_drv_registers*) config->reg_base;

	/* Exit if Manager component is already enabled */
	reg = slimbus_drv_read_reg(configuration.config_mode);
	if (CONFIGURATION__CONFIG_MODE__ENABLE__READ(reg)) {
		slimbus_core_limit_err("operation now in progress\n");
		return EINPROGRESS;
	}

	instance->sending_failed = 0;
	instance->sending_finished = 0;

	/* config_mode - Configuration Mode */
	reg = slimbus_drv_read_reg(configuration.config_mode);

	CONFIGURATION__CONFIG_MODE__SNIFFER_MODE__MODIFY(reg, config->sniffer_mode);
	CONFIGURATION__CONFIG_MODE__MANAGER_MODE__SET(reg);

	CONFIGURATION__CONFIG_MODE__FR_EN__MODIFY(reg, config->enable_framer);
	CONFIGURATION__CONFIG_MODE__DEV_EN__MODIFY(reg, config->enable_device);

	CONFIGURATION__CONFIG_MODE__RETRY_LMT__MODIFY(reg, config->retry_limit);
	CONFIGURATION__CONFIG_MODE__REPORT_AT_EVENT__MODIFY(reg, config->report_at_event);

	instance->disable_hardware_crc_calculation = config->disable_hardware_crc_calculation;
	CONFIGURATION__CONFIG_MODE__CRC_CALC_DISABLE__MODIFY(reg, config->disable_hardware_crc_calculation);

	CONFIGURATION__CONFIG_MODE__LMTD_REPORT__MODIFY(reg, config->limit_reports);
	CONFIGURATION__CONFIG_MODE__RECONF_TX_DIS__CLR(reg);

	slimbus_drv_write_reg(configuration.config_mode, reg);

	/* config_ea - Enumeration Address Part 1 */
	reg = slimbus_drv_read_reg(configuration.config_ea);
	CONFIGURATION__CONFIG_EA__PRODUCT_ID__MODIFY(reg, config->ea_product_id);
	CONFIGURATION__CONFIG_EA__INSTANCE_VAL__MODIFY(reg, config->ea_instance_value);
	slimbus_drv_write_reg(configuration.config_ea, reg);

	/* config_ea2 - Enumeration Address Part 2 */
	reg = slimbus_drv_read_reg(configuration.config_ea2);
	CONFIGURATION__CONFIG_EA2__DEVICE_ID_1__MODIFY(reg, config->ea_interface_id);
	CONFIGURATION__CONFIG_EA2__DEVICE_ID_2__MODIFY(reg, config->ea_generic_id);
	CONFIGURATION__CONFIG_EA2__DEVICE_ID_3__MODIFY(reg, config->ea_framer_id);
	slimbus_drv_write_reg(configuration.config_ea2, reg);


	/* int_en  - Interrupts Enable */
	reg = slimbus_drv_read_reg(interrupts.int_en);
	INTERRUPTS__INT_EN__RX_INT_EN__SET(reg);		//Enable interrupt for receiving messages
	INTERRUPTS__INT_EN__TX_ERR_EN__SET(reg);		//Enable interrupt for sending messages error
	INTERRUPTS__INT_EN__TX_INT_EN__SET(reg);		//Enable interrupt for sending messages
	INTERRUPTS__INT_EN__SYNC_LOST_EN__SET(reg);		//Enable interrupt for sync lost

	slimbus_drv_write_reg(interrupts.int_en, reg);

	instance->enumerate_devices = config->enumerate_devices;

	slimbus_drv_ps_buffer_copy((uint8_t*) &(instance->basic_callbacks), (uint8_t*) callbacks, sizeof(slimbus_drv_callbacks));
	clear_memory((void*) &(instance->message_callbacks), sizeof(slimbus_message_callbacks));

	return 0;
}

static uint32_t slimbus_drv_isr(void* pd)
{
	slimbus_drv_instance* instance = NULL;
	uint32_t reg, copy;
	bool data_port_interrupt = false;
	uint8_t i, j;
	slimbus_data_port_interrupt data_port_irq;

	uint32_t result = slimbus_isr_sanity(pd);
	if (result) {
		slimbus_core_limit_err("check sanity fail\n");
		return result;
	}

	instance = (slimbus_drv_instance*) pd;

	/* Check interrupt register */
	reg = slimbus_drv_read_reg(interrupts.interrupt);
	if (reg) { //Interrupt occurred in interrupt register
		if (instance->basic_callbacks.on_manager_interrupt != NULL) {
			slimbus_drv_interrupt interrupts;
			interrupts = reg;
			instance->basic_callbacks.on_manager_interrupt(pd, interrupts);
		}

		slimbus_irq_state = (int)reg;

		if (INTERRUPTS__INT__TX_INT__READ(reg))
			instance->sending_finished = 1;
		if (INTERRUPTS__INT__TX_ERR__READ(reg))
			instance->sending_failed = 1;
		if (INTERRUPTS__INT__RX_INT__READ(reg))
			receive_messages(instance);

		data_port_interrupt = INTERRUPTS__INT__PORT_INT__READ(reg);

		/* Clear interrupts */
		slimbus_drv_write_reg(interrupts.interrupt, reg);
	}

	if (data_port_interrupt != 0) {

		for (i = 0; i < 16; i++) {
			reg = slimbus_drv_read_reg(port_interrupts.p_int[i]) & (~FIFO_CLEAR_MASK);

			if (reg == 0) //If all bits in the register are low, then there is no interrupt
				continue;

			if (instance->basic_callbacks.on_data_port_interrupt != NULL) { //There is callback assigned
				copy = reg;
				for (j = 0; j < 4; j++) {	//p_int register has 4 the same 8 bit registers with interrupts
					data_port_irq = 0;
					data_port_irq |=	(PORT_INTERRUPTS__P_INT__P0_ACT_INT__READ(copy)) ? DP_INT_ACT : 0;
					data_port_irq |=	(PORT_INTERRUPTS__P_INT__P0_CON_INT__READ(copy)) ? DP_INT_CON : 0;
					data_port_irq |=	(PORT_INTERRUPTS__P_INT__P0_CHAN_INT__READ(copy)) ? DP_INT_CHAN : 0;
					data_port_irq |=	(PORT_INTERRUPTS__P_INT__P0_DMA_INT__READ(copy)) ? DP_INT_DMA : 0;
					data_port_irq |=	(PORT_INTERRUPTS__P_INT__P0_OVF_INT__READ(copy)) ? DP_INT_OVF : 0;
					data_port_irq |=	(PORT_INTERRUPTS__P_INT__P0_UND_INT__READ(copy)) ? DP_INT_UND : 0;

					copy >>= 8;

					if (data_port_irq != 0)
						instance->basic_callbacks.on_data_port_interrupt(pd, (i * 4 + j), data_port_irq);
				}
			}
			slimbus_drv_write_reg(port_interrupts.p_int[i], reg); // Clear contents of the port interrupt register
		}
	}

	return 0;
}


static uint32_t slimbus_drv_start(void* pd)
{
	slimbus_drv_instance* instance = NULL;
	uint32_t reg;
	uint32_t timeout_count = 0;

	uint32_t result = slimbus_start_sanity(pd);
	if (result) {
		slimbus_core_limit_err("check sanity fail\n");
		return result;
	}

	instance = (slimbus_drv_instance*) pd;

	/* Exit if Manager component is already enabled */
	reg = slimbus_drv_read_reg(configuration.config_mode);
	if (CONFIGURATION__CONFIG_MODE__ENABLE__READ(reg)) {
		slimbus_core_limit_err("operation now in progress\n");
		return EINPROGRESS;
	}

	/* Enable component */
	CONFIGURATION__CONFIG_MODE__ENABLE__SET(reg);
	slimbus_drv_write_reg(configuration.config_mode, reg);

	/* Enable all interrupts */
	reg = slimbus_drv_read_reg(interrupts.int_en);
	INTERRUPTS__INT_EN__INT_EN__SET(reg);
	slimbus_drv_write_reg(interrupts.int_en, reg);

	cfg_strobe_set(instance, 1);

	/* Wait for synchronization with SLIMbus */
	reg = slimbus_drv_read_reg(command_status.state);
	while ((COMMAND_STATUS__STATE__F_SYNC__READ(reg) == 0)
		|| (COMMAND_STATUS__STATE__SF_SYNC__READ(reg) == 0)
		|| (COMMAND_STATUS__STATE__M_SYNC__READ(reg) == 0)) {
		msleep(1);
		reg = slimbus_drv_read_reg(command_status.state);
		if (++timeout_count == 100) {
			slimbus_core_limit_err("SLIMbus start wait sync timeout!\n");
			return EIO;
		}
	}

	return 0;
}

static uint32_t slimbus_drv_api_stop(void* pd)
{
	slimbus_drv_instance* instance = NULL;
	uint32_t reg;
	uint32_t result = slimbus_stop_sanity(pd);
	if (result) {
		slimbus_core_limit_err("check sanity fail\n");
		return result;
	}

	instance = (slimbus_drv_instance*) pd;

	if (cfg_strobe_check(instance)) {
		slimbus_core_limit_err("strobe check fail\n");
		return EIO;
	}

	/* Disable all interrupts */
	reg = slimbus_drv_read_reg(interrupts.int_en);
	INTERRUPTS__INT_EN__INT_EN__CLR(reg);
	slimbus_drv_write_reg(interrupts.int_en, reg);

	/* Disable component */
	reg = slimbus_drv_read_reg(configuration.config_mode);
	CONFIGURATION__CONFIG_MODE__ENABLE__CLR(reg);
	slimbus_drv_write_reg(configuration.config_mode, reg);

	cfg_strobe_set(instance, 1);

	return 0;
}


static uint32_t slimbus_drv_destroy(void* pd)
{
	uint32_t result = slimbus_destroy_sanity(pd);
	if (result) {
		slimbus_core_limit_err("check sanity fail\n");
	   return result;
	}

	return slimbus_drv_api_stop(pd);
}


static uint32_t slimbus_drv_set_interrupts(void* pd, uint8_t interrupt_mask)
{
	slimbus_drv_instance* instance = NULL;
	uint32_t reg = 0;

	uint32_t result = slimbus_set_interrupts_sanity(pd, interrupt_mask);
	if (result) {
		slimbus_core_limit_err("check sanity fail\n");
		return result;
	}

	instance = (slimbus_drv_instance*) pd;

	reg = interrupt_mask;

	/* Make sure that mandatory interrupts are enabled */
	INTERRUPTS__INT_EN__INT_EN__SET(reg);       //Enable interrupts
	INTERRUPTS__INT_EN__RX_INT_EN__SET(reg);    //Interrupt for receiving messages
	INTERRUPTS__INT_EN__TX_ERR_EN__SET(reg);    //Interrupt for sending messages error
	INTERRUPTS__INT_EN__TX_INT_EN__SET(reg);    //Interrupt for sending messages

	slimbus_drv_write_reg(interrupts.int_en, reg);

	return 0;
}


static uint32_t slimbus_drv_get_interrupts(void* pd, uint8_t* interrupt_mask)
{
	slimbus_drv_instance* instance = NULL;
	uint32_t reg;

	uint32_t result = slimbus_get_interrupts_sanity(pd, interrupt_mask);
	if (result) {
		slimbus_core_limit_err("check sanity fail\n");
		return result;
	}

	instance = (slimbus_drv_instance*) pd;

	reg = slimbus_drv_read_reg(interrupts.int_en);
	*interrupt_mask = reg;

	return 0;
}


static uint32_t slimbus_drv_set_data_port_interrupts(void* pd, uint8_t port_number, uint8_t interrupt_mask)
{
	slimbus_drv_instance* instance = NULL;
	uint8_t port_address;
	uint32_t reg;

	uint32_t result = slimbus_set_data_port_interrupts_sanity(pd, port_number, interrupt_mask);
	if (result) {
		slimbus_core_limit_err("check sanity fail\n");
		return result;
	}

	instance = (slimbus_drv_instance*) pd;

	port_address = port_number / 4;
	/* There are 4 Port registers per 1 32 bit p_int_en register */

	reg = slimbus_drv_read_reg(port_interrupts.p_int_en[port_address]);

	switch (port_number % 4) {
	case 0:
		PORT_INTERRUPTS__P_INT_EN__P0_ACT_INT_EN__MODIFY(reg, (interrupt_mask & DP_INT_ACT) != 0 );
		PORT_INTERRUPTS__P_INT_EN__P0_CON_INT_EN__MODIFY(reg, (interrupt_mask & DP_INT_CON) != 0 );
		PORT_INTERRUPTS__P_INT_EN__P0_CHAN_INT_EN__MODIFY(reg, (interrupt_mask & DP_INT_CHAN) != 0 );
		PORT_INTERRUPTS__P_INT_EN__P0_DMA_INT_EN__MODIFY(reg, (interrupt_mask & DP_INT_DMA) != 0 );
		PORT_INTERRUPTS__P_INT_EN__P0_OVF_INT_EN__MODIFY(reg, (interrupt_mask & DP_INT_OVF) != 0 );
		PORT_INTERRUPTS__P_INT_EN__P0_UND_INT_EN__MODIFY(reg, (interrupt_mask & DP_INT_UND) != 0 );
		break;
	case 1:
		PORT_INTERRUPTS__P_INT_EN__P1_ACT_INT_EN__MODIFY(reg, (interrupt_mask & DP_INT_ACT) != 0 );
		PORT_INTERRUPTS__P_INT_EN__P1_CON_INT_EN__MODIFY(reg, (interrupt_mask & DP_INT_CON) != 0 );
		PORT_INTERRUPTS__P_INT_EN__P1_CHAN_INT_EN__MODIFY(reg, (interrupt_mask & DP_INT_CHAN) != 0 );
		PORT_INTERRUPTS__P_INT_EN__P1_DMA_INT_EN__MODIFY(reg, (interrupt_mask & DP_INT_DMA) != 0 );
		PORT_INTERRUPTS__P_INT_EN__P1_OVF_INT_EN__MODIFY(reg, (interrupt_mask & DP_INT_OVF) != 0 );
		PORT_INTERRUPTS__P_INT_EN__P1_UND_INT_EN__MODIFY(reg, (interrupt_mask & DP_INT_UND) != 0 );
		break;
	case 2:
		PORT_INTERRUPTS__P_INT_EN__P2_ACT_INT_EN__MODIFY(reg, (interrupt_mask & DP_INT_ACT) != 0 );
		PORT_INTERRUPTS__P_INT_EN__P2_CON_INT_EN__MODIFY(reg, (interrupt_mask & DP_INT_CON) != 0 );
		PORT_INTERRUPTS__P_INT_EN__P2_CHAN_INT_EN__MODIFY(reg, (interrupt_mask & DP_INT_CHAN) != 0 );
		PORT_INTERRUPTS__P_INT_EN__P2_DMA_INT_EN__MODIFY(reg, (interrupt_mask & DP_INT_DMA) != 0 );
		PORT_INTERRUPTS__P_INT_EN__P2_OVF_INT_EN__MODIFY(reg, (interrupt_mask & DP_INT_OVF) != 0 );
		PORT_INTERRUPTS__P_INT_EN__P2_UND_INT_EN__MODIFY(reg, (interrupt_mask & DP_INT_UND) != 0 );
		break;
	case 3:
		PORT_INTERRUPTS__P_INT_EN__P3_ACT_INT_EN__MODIFY(reg, (interrupt_mask & DP_INT_ACT) != 0 );
		PORT_INTERRUPTS__P_INT_EN__P3_CON_INT_EN__MODIFY(reg, (interrupt_mask & DP_INT_CON) != 0 );
		PORT_INTERRUPTS__P_INT_EN__P3_CHAN_INT_EN__MODIFY(reg, (interrupt_mask & DP_INT_CHAN) != 0 );
		PORT_INTERRUPTS__P_INT_EN__P3_DMA_INT_EN__MODIFY(reg, (interrupt_mask & DP_INT_DMA) != 0 );
		PORT_INTERRUPTS__P_INT_EN__P3_OVF_INT_EN__MODIFY(reg, (interrupt_mask & DP_INT_OVF) != 0 );
		PORT_INTERRUPTS__P_INT_EN__P3_UND_INT_EN__MODIFY(reg, (interrupt_mask & DP_INT_UND) != 0 );
		break;
	default:
		break;
	}
	slimbus_drv_write_reg(port_interrupts.p_int_en[port_address], reg);

	return 0;
}


static uint32_t slimbus_drv_get_data_port_interrupts(void* pd, uint8_t port_number, uint8_t* interrupt_mask)
{
	slimbus_drv_instance* instance = NULL;
	uint8_t port_address;
	uint32_t reg;
	slimbus_data_port_interrupt output = 0;

	uint32_t result = slimbus_get_data_port_interrupts_sanity(pd, port_number, interrupt_mask);
	if (result) {
		slimbus_core_limit_err("check sanity fail\n");
		return result;
	}

	instance = (slimbus_drv_instance*) pd;

	port_address = port_number / 4;
	/* There are 4 Port registers per 1 32 bit p_int_en register */
	reg = slimbus_drv_read_reg(port_interrupts.p_int_en[port_address]);

	switch (port_number % 4) {
	case 0:
		output |=  (PORT_INTERRUPTS__P_INT_EN__P0_ACT_INT_EN__READ(reg)) ? DP_INT_ACT : 0;
		output |=  (PORT_INTERRUPTS__P_INT_EN__P0_CON_INT_EN__READ(reg)) ? DP_INT_CON : 0;
		output |=  (PORT_INTERRUPTS__P_INT_EN__P0_CHAN_INT_EN__READ(reg)) ? DP_INT_CHAN : 0;
		output |=  (PORT_INTERRUPTS__P_INT_EN__P0_DMA_INT_EN__READ(reg)) ? DP_INT_DMA : 0;
		output |=  (PORT_INTERRUPTS__P_INT_EN__P0_OVF_INT_EN__READ(reg)) ? DP_INT_OVF : 0;
		output |=  (PORT_INTERRUPTS__P_INT_EN__P0_UND_INT_EN__READ(reg)) ? DP_INT_UND : 0;
		break;
	case 1:
		output |=  (PORT_INTERRUPTS__P_INT_EN__P1_ACT_INT_EN__READ(reg)) ? DP_INT_ACT : 0;
		output |=  (PORT_INTERRUPTS__P_INT_EN__P1_CON_INT_EN__READ(reg)) ? DP_INT_CON : 0;
		output |=  (PORT_INTERRUPTS__P_INT_EN__P1_CHAN_INT_EN__READ(reg)) ? DP_INT_CHAN : 0;
		output |=  (PORT_INTERRUPTS__P_INT_EN__P1_DMA_INT_EN__READ(reg)) ? DP_INT_DMA : 0;
		output |=  (PORT_INTERRUPTS__P_INT_EN__P1_OVF_INT_EN__READ(reg)) ? DP_INT_OVF : 0;
		output |=  (PORT_INTERRUPTS__P_INT_EN__P1_UND_INT_EN__READ(reg)) ? DP_INT_UND : 0;
		break;
	case 2:
		output |=  (PORT_INTERRUPTS__P_INT_EN__P2_ACT_INT_EN__READ(reg)) ? DP_INT_ACT : 0;
		output |=  (PORT_INTERRUPTS__P_INT_EN__P2_CON_INT_EN__READ(reg)) ? DP_INT_CON : 0;
		output |=  (PORT_INTERRUPTS__P_INT_EN__P2_CHAN_INT_EN__READ(reg)) ? DP_INT_CHAN : 0;
		output |=  (PORT_INTERRUPTS__P_INT_EN__P2_DMA_INT_EN__READ(reg)) ? DP_INT_DMA : 0;
		output |=  (PORT_INTERRUPTS__P_INT_EN__P2_OVF_INT_EN__READ(reg)) ? DP_INT_OVF : 0;
		output |=  (PORT_INTERRUPTS__P_INT_EN__P2_UND_INT_EN__READ(reg)) ? DP_INT_UND : 0;
		break;
	case 3:
		output |=  (PORT_INTERRUPTS__P_INT_EN__P3_ACT_INT_EN__READ(reg)) ? DP_INT_ACT : 0;
		output |=  (PORT_INTERRUPTS__P_INT_EN__P3_CON_INT_EN__READ(reg)) ? DP_INT_CON : 0;
		output |=  (PORT_INTERRUPTS__P_INT_EN__P3_CHAN_INT_EN__READ(reg)) ? DP_INT_CHAN : 0;
		output |=  (PORT_INTERRUPTS__P_INT_EN__P3_DMA_INT_EN__READ(reg)) ? DP_INT_DMA : 0;
		output |=  (PORT_INTERRUPTS__P_INT_EN__P3_OVF_INT_EN__READ(reg)) ? DP_INT_OVF : 0;
		output |=  (PORT_INTERRUPTS__P_INT_EN__P3_UND_INT_EN__READ(reg)) ? DP_INT_UND : 0;
		break;
	default:
		break;
	}
	*interrupt_mask = output;

	return 0;
}


static uint32_t slimbus_drv_clear_data_port_fifo(void* pd, uint8_t port_number)
{
	slimbus_drv_instance* instance = NULL;
	uint8_t port_address;
	uint32_t reg;

	uint32_t result = slimbus_clear_data_port_fifo_sanity(pd, port_number);
	if (result) {
		slimbus_core_limit_err("check sanity fail\n");
		return result;
	}

	instance = (slimbus_drv_instance*) pd;

	port_address = port_number / 4;
	/* There are 4 Port registers per 1 32 bit p_int_en register */

	reg = slimbus_drv_read_reg(port_interrupts.p_int_en[port_address]) & (~FIFO_CLEAR_MASK);

	switch (port_number % 4) {
	case 0:
		PORT_INTERRUPTS__P_INT_EN__P0_FIFO_CLR__SET(reg);
		break;
	case 1:
		PORT_INTERRUPTS__P_INT_EN__P1_FIFO_CLR__SET(reg);
		break;
	case 2:
		PORT_INTERRUPTS__P_INT_EN__P2_FIFO_CLR__SET(reg);
		break;
	case 3:
		PORT_INTERRUPTS__P_INT_EN__P3_FIFO_CLR__SET(reg);
		break;
	default:
		break;
	}
	slimbus_drv_write_reg(port_interrupts.p_int_en[port_address], reg);

	return 0;
}


static uint32_t slimbus_drv_set_presence_rate_generation(void* pd, uint8_t port_number, bool enable)
{
	slimbus_drv_instance* instance = NULL;
	uint8_t port_address;
	uint32_t reg;

	uint32_t result = slimbus_set_presence_rate_generation_sanity(pd, port_number, enable);
	if (result) {
		slimbus_core_limit_err("check sanity fail\n");
		return result;
	}

	instance = (slimbus_drv_instance*) pd;

	port_address = port_number / 4;
	/* There are 4 Port registers per 1 32 bit p_int_en register */

	reg = slimbus_drv_read_reg(port_interrupts.p_int_en[port_address]) & (~FIFO_CLEAR_MASK);

	switch (port_number % 4) {
	case 0:
		PORT_INTERRUPTS__P_INT_EN__P0_PR_GEN_EN__MODIFY(reg, enable);
		break;
	case 1:
		PORT_INTERRUPTS__P_INT_EN__P1_PR_GEN_EN__MODIFY(reg, enable);
		break;
	case 2:
		PORT_INTERRUPTS__P_INT_EN__P2_PR_GEN_EN__MODIFY(reg, enable);
		break;
	case 3:
		PORT_INTERRUPTS__P_INT_EN__P3_PR_GEN_EN__MODIFY(reg, enable);
		break;
	default:
		break;
	}
	slimbus_drv_write_reg(port_interrupts.p_int_en[port_address], reg);

	return 0;
}


static uint32_t slimbus_drv_get_presence_rate_generation(void* pd, uint8_t port_number, bool* enable)
{
	slimbus_drv_instance* instance = NULL;
	uint8_t port_address;
	uint32_t reg;

	uint32_t result = slimbus_get_presence_rate_generation_sanity(pd, port_number, enable);
	if (result) {
		slimbus_core_limit_err("check sanity fail\n");
		return result;
	}

	instance = (slimbus_drv_instance*) pd;

	port_address = port_number / 4;
	reg = slimbus_drv_read_reg(port_interrupts.p_int_en[port_address]);

	switch (port_number % 4) {
	case 0:
		*enable = PORT_INTERRUPTS__P_INT_EN__P0_PR_GEN_EN__READ(reg);
		break;
	case 1:
		*enable = PORT_INTERRUPTS__P_INT_EN__P1_PR_GEN_EN__READ(reg);
		break;
	case 2:
		*enable = PORT_INTERRUPTS__P_INT_EN__P2_PR_GEN_EN__READ(reg);
		break;
	case 3:
		*enable = PORT_INTERRUPTS__P_INT_EN__P3_PR_GEN_EN__READ(reg);
		break;
	default:
		break;
	}

	return 0;
}


static uint32_t slimbus_drv_assign_message_callbacks(void* pd, slimbus_message_callbacks* msg_callbacks)
{
	slimbus_drv_instance* instance = NULL;

	uint32_t result = slimbus_assign_message_callbacks_sanity(pd, msg_callbacks);
	if (result) {
		slimbus_core_limit_err("check sanity fail\n");
		return result;
	}

	instance = (slimbus_drv_instance*) pd;

	slimbus_drv_ps_buffer_copy((uint8_t*) &(instance->message_callbacks), (uint8_t*) msg_callbacks, sizeof(slimbus_message_callbacks));

	return 0;
}


static uint32_t slimbus_drv_send_raw_message(void* pd, void* message, uint8_t message_length)
{
	slimbus_drv_instance* instance = NULL;

	uint32_t result = slimbus_send_raw_message_sanity(pd, message, message_length);
	if (result) {
		slimbus_core_limit_err("check sanity fail\n");
		return result;
	}

	instance = (slimbus_drv_instance*) pd;

	return fifo_transmit(instance, (uint8_t *) message, message_length, 1);
}


static uint32_t slimbus_drv_send_message(void* pd, slimbus_drv_message* message)
{
	slimbus_drv_instance* instance = NULL;

	uint32_t result = slimbus_send_message_sanity(pd, message);
	if (result) {
		slimbus_core_limit_err("check sanity fail\n");
		return result;
	}

	instance = (slimbus_drv_instance*) pd;

	return transmit_message(instance, message);
}

static uint32_t slimbus_drv_get_register_value(void* pd, uint16_t reg_addr, uint32_t* reg_content)
{
	slimbus_drv_instance* instance = NULL;

	uint32_t result = slimbus_get_register_value_sanity(pd, reg_addr, reg_content);
	if (result) {
		slimbus_core_limit_err("check sanity fail\n");
		return result;
	}

	instance = (slimbus_drv_instance*) pd;

	if (reg_addr & 0x3) { //Register address must be aligned to 32-bits
		slimbus_core_limit_err("reg is invalid: 0x%x\n", reg_addr);
		return EINVAL;
	}

	*reg_content = slimbus_drv_ps_uncached_read32((uint32_t *) instance->register_base + (reg_addr >> 2) );

	return 0;
}


static uint32_t slimbus_drv_set_register_value(void* pd, uint16_t reg_addr, uint32_t reg_content)
{
	slimbus_drv_instance* instance = NULL;

	uint32_t result = slimbus_set_register_value_sanity(pd, reg_addr, reg_content);
	if (result) {
		slimbus_core_limit_err("check sanity fail\n");
		return result;
	}

	instance = (slimbus_drv_instance*) pd;

	if (reg_addr & 0x3) { //Register address must be aligned to 32-bits
		slimbus_core_limit_err("reg is invalid: 0x%x\n", reg_addr);
		return EINVAL;
	}

	slimbus_drv_ps_uncached_write32((uint32_t *) instance->register_base + (reg_addr >> 2), reg_content);

	return 0;
}


static uint32_t slimbus_drv_set_message_channel_lapse(void* pd, uint8_t mch_lapse)
{
	slimbus_drv_instance* instance = NULL;
	uint32_t reg;

	uint32_t result = slimbus_set_message_channel_lapse_sanity(pd, mch_lapse);
	if (result) {
		slimbus_core_limit_err("check sanity fail\n");
		return result;
	}

	instance = (slimbus_drv_instance*) pd;

	reg = slimbus_drv_read_reg(command_status.mch_usage);
	COMMAND_STATUS__MCH_USAGE__MCH_LAPSE__MODIFY(reg, mch_lapse);
	slimbus_drv_write_reg(command_status.mch_usage, reg);

	return 0;
}

static uint32_t slimbus_drv_get_message_channel_lapse(void* pd, uint8_t* mch_lapse)
{
	slimbus_drv_instance* instance = NULL;
	uint32_t reg;

	uint32_t result = slimbus_get_message_channel_lapse_sanity(pd, mch_lapse);
	if (result) {
		slimbus_core_limit_err("check sanity fail\n");
		return result;
	}

	instance = (slimbus_drv_instance*) pd;

	reg = slimbus_drv_read_reg(command_status.mch_usage);
	*mch_lapse = COMMAND_STATUS__MCH_USAGE__MCH_LAPSE__READ(reg);

	return 0;
}

static uint32_t slimbus_drv_get_message_channel_usage(void* pd, uint16_t* mch_usage)
{
	slimbus_drv_instance* instance = NULL;
	uint32_t reg;

	uint32_t result = slimbus_get_message_channel_usage_sanity(pd, mch_usage);
	if (result) {
		slimbus_core_limit_err("check sanity fail\n");
		return result;
	}


	instance = (slimbus_drv_instance*) pd;

	reg = slimbus_drv_read_reg(command_status.mch_usage);
	*mch_usage = COMMAND_STATUS__MCH_USAGE__MCH_USAGE__READ(reg);

	return 0;
}

static uint32_t slimbus_drv_get_message_channel_capacity(void* pd, uint16_t* mch_capacity)
{
	slimbus_drv_instance* instance = NULL;
	uint32_t reg;

	uint32_t result = slimbus_get_message_channel_capacity_sanity(pd, mch_capacity);
	if (result) {
		slimbus_core_limit_err("check sanity fail\n");
		return result;
	}

	instance = (slimbus_drv_instance*) pd;

	reg = slimbus_drv_read_reg(command_status.mch_usage);
	*mch_capacity = COMMAND_STATUS__MCH_USAGE__MCH_CAPACITY__READ(reg);

	return 0;
}


static uint32_t slimbus_drv_set_sniffer_mode(void* pd, bool state)
{
	slimbus_drv_instance* instance = NULL;
	uint32_t reg;

	uint32_t result = slimbus_set_sniffer_mode_sanity(pd, state);
	if (result) {
		slimbus_core_limit_err("check sanity fail\n");
		return result;
	}

	instance = (slimbus_drv_instance*) pd;

	if (cfg_strobe_check(instance)) {
		slimbus_core_limit_err("strobe check fail\n");
		return EIO;
	}

	reg = slimbus_drv_read_reg(configuration.config_mode);
	CONFIGURATION__CONFIG_MODE__SNIFFER_MODE__MODIFY(reg, state);
	slimbus_drv_write_reg(configuration.config_mode, reg);

	cfg_strobe_set(instance, 0);

	return 0;
}


static uint32_t slimbus_drv_get_sniffer_mode(void* pd, bool* state)
{
	slimbus_drv_instance* instance = NULL;
	uint32_t reg;

	uint32_t result = slimbus_get_sniffer_mode_sanity(pd, state);
	if (result) {
		slimbus_core_limit_err("check sanity fail\n");
		return result;
	}

	instance = (slimbus_drv_instance*) pd;

	reg = slimbus_drv_read_reg(configuration.config_mode);
	*state = CONFIGURATION__CONFIG_MODE__SNIFFER_MODE__READ(reg);

	return 0;
}


static uint32_t slimbus_drv_set_framer_enabled(void* pd, bool state)
{
	slimbus_drv_instance* instance = NULL;
	uint32_t reg;

	uint32_t result = slimbus_set_framer_enabled_sanity(pd, state);
	if (result) {
		slimbus_core_limit_err("check sanity fail\n");
		return result;
	}

	instance = (slimbus_drv_instance*) pd;

	if (cfg_strobe_check(instance)) {
		slimbus_core_limit_err("strobe check fail\n");
		return EIO;
	}

	reg = slimbus_drv_read_reg(configuration.config_mode);
	CONFIGURATION__CONFIG_MODE__FR_EN__MODIFY(reg, state);
	slimbus_drv_write_reg(configuration.config_mode, reg);

	cfg_strobe_set(instance, 0);

	return 0;
}


static uint32_t slimbus_drv_get_framer_enabled(void* pd, bool* state)
{
	slimbus_drv_instance* instance = NULL;
	uint32_t reg;

	uint32_t result = slimbus_get_framer_enabled_sanity(pd, state);
	if (result) {
		slimbus_core_limit_err("check sanity fail\n");
		return result;
	}

	instance = (slimbus_drv_instance*) pd;

	reg = slimbus_drv_read_reg(configuration.config_mode);
	*state = CONFIGURATION__CONFIG_MODE__FR_EN__READ(reg);

	return 0;
}


static uint32_t slimbus_drv_set_device_enabled(void* pd, bool state)
{
	slimbus_drv_instance* instance = NULL;
	uint32_t reg;

	uint32_t result = slimbus_set_device_enabled_sanity(pd, state);
	if (result) {
		slimbus_core_limit_err("check sanity fail\n");
		return result;
	}

	instance = (slimbus_drv_instance*) pd;

	if (cfg_strobe_check(instance)) {
		slimbus_core_limit_err("strobe check fail\n");
		return EIO;
	}

	reg = slimbus_drv_read_reg(configuration.config_mode);
	CONFIGURATION__CONFIG_MODE__DEV_EN__MODIFY(reg, state);
	slimbus_drv_write_reg(configuration.config_mode, reg);

	cfg_strobe_set(instance, 0);

	return 0;
}


static uint32_t slimbus_drv_get_device_enabled(void* pd, bool* state)
{
	slimbus_drv_instance* instance = NULL;
	uint32_t reg;

	uint32_t result = slimbus_get_device_enabled_sanity(pd, state);
	if (result) {
		slimbus_core_limit_err("check sanity fail\n");
		return result;
	}

	instance = (slimbus_drv_instance*) pd;

	reg = slimbus_drv_read_reg(configuration.config_mode);
	*state = CONFIGURATION__CONFIG_MODE__DEV_EN__READ(reg);

	return 0;
}


static uint32_t slimbus_drv_set_go_absent(void* pd, bool state)
{
	slimbus_drv_instance* instance = NULL;
	uint32_t reg;

	uint32_t result = slimbus_set_go_absent_sanity(pd, state);
	if (result) {
		slimbus_core_limit_err("check sanity fail\n");
		return result;
	}

	instance = (slimbus_drv_instance*) pd;

	if (cfg_strobe_check(instance)) {
		slimbus_core_limit_err("strobe check fail\n");
		return EIO;
	}

	reg = slimbus_drv_read_reg(configuration.config_mode);
	CONFIGURATION__CONFIG_MODE__GO_ABSENT__MODIFY(reg, state);
	slimbus_drv_write_reg(configuration.config_mode, reg);

	cfg_strobe_set(instance, 0);

	return 0;
}


static uint32_t slimbus_drv_get_go_absent(void* pd, bool* state)
{
	slimbus_drv_instance* instance = NULL;
	uint32_t reg;

	uint32_t result = slimbus_get_go_absent_sanity(pd, state);
	if (result) {
		slimbus_core_limit_err("check sanity fail\n");
		return result;
	}

	instance = (slimbus_drv_instance*) pd;

	reg = slimbus_drv_read_reg(configuration.config_mode);
	*state = CONFIGURATION__CONFIG_MODE__GO_ABSENT__READ(reg);

	return 0;
}


static uint32_t slimbus_drv_set_framer_config(void* pd, slimbus_framer_config* framer_config)
{
	slimbus_drv_instance* instance = NULL;
	uint32_t reg;

	uint32_t result = slimbus_set_framer_config_sanity(pd, framer_config);
	if (result) {
		slimbus_core_limit_err("check sanity fail\n");
		return result;
	}

	if ((framer_config->quality != FQ_IRREGULAR)
	     && (framer_config->quality != FQ_LOW_JITTER)
	     && (framer_config->quality != FQ_PUNCTURED)
	     && (framer_config->quality != FQ_REGULAR)) {
		slimbus_core_limit_err("framer config's quality is invalid: %d\n", framer_config->quality);
		return EINVAL;
	}

	instance = (slimbus_drv_instance*) pd;

	if (cfg_strobe_check(instance)) {
		slimbus_core_limit_err("strobe check fail\n");
		return EIO;
	}

	reg = slimbus_drv_read_reg(configuration.config_fr);
	CONFIGURATION__CONFIG_FR__PAUSE_AT_RFCHNG__MODIFY(reg, framer_config->pause_at_root_frequency_change);
	CONFIGURATION__CONFIG_FR__QUALITY__MODIFY(reg, framer_config->quality);
	CONFIGURATION__CONFIG_FR__RF_SUPP__MODIFY(reg, framer_config->root_frequencies_supported);
	slimbus_drv_write_reg(configuration.config_fr, reg);

	cfg_strobe_set(instance, 0);

	return 0;
}


static uint32_t slimbus_drv_get_framer_config(void* pd, slimbus_framer_config* framer_config)
{
	slimbus_drv_instance* instance = NULL;
	uint32_t reg;

	uint32_t result = slimbus_get_framer_config_sanity(pd, framer_config);
	if (result) {
		slimbus_core_limit_err("check sanity fail\n");
		return result;
	}

	instance = (slimbus_drv_instance*) pd;

	reg = slimbus_drv_read_reg(configuration.config_fr);
	framer_config->pause_at_root_frequency_change = CONFIGURATION__CONFIG_FR__PAUSE_AT_RFCHNG__READ(reg);
	framer_config->quality = CONFIGURATION__CONFIG_FR__QUALITY__READ(reg);
	framer_config->root_frequencies_supported = CONFIGURATION__CONFIG_FR__RF_SUPP__READ(reg);

	return 0;
}


static uint32_t slimbus_drv_set_generic_device_config(void* pd, slimbus_generic_device_config* generic_device_config)
{
	slimbus_drv_instance* instance = NULL;
	uint32_t reg;
	uint8_t temp;

	uint32_t result = slimbus_set_generic_device_config_sanity(pd, generic_device_config);
	if (result) {
		slimbus_core_limit_err("check sanity fail\n");
		return result;
	}

	/* Checking input structure parameters */
	/* Limit maximum values, based on register field length */
	if (generic_device_config->presence_rates_supported > 0xFFFFFF) {
		slimbus_core_limit_err("device config param is invalid: 0x%x\n", generic_device_config->presence_rates_supported);
		return EINVAL;
	}

	if (generic_device_config->data_port_clock_prescaler > 0xF) {
		slimbus_core_limit_err("device config param is invalid: 0x%x\n", generic_device_config->data_port_clock_prescaler);
		return EINVAL;
	}

	if (generic_device_config->cport_clock_divider > 0x7) {
		slimbus_core_limit_err("device config param is invalid: 0x%x\n", generic_device_config->cport_clock_divider);
		return EINVAL;
	}

	if ((generic_device_config->reference_clock_selector != RC_CLOCK_GEAR_6)
			&& (generic_device_config->reference_clock_selector != RC_CLOCK_GEAR_7)
			&& (generic_device_config->reference_clock_selector != RC_CLOCK_GEAR_8)
			&& (generic_device_config->reference_clock_selector != RC_CLOCK_GEAR_9)) {
		slimbus_core_limit_err("device config param is invalid: 0x%x\n", generic_device_config->reference_clock_selector);
		return EINVAL;
	}

	instance = (slimbus_drv_instance*) pd;

	if (cfg_strobe_check(instance)) {
		slimbus_core_limit_err("strobe check fail\n");
		return EIO;
	}

	reg = slimbus_drv_read_reg(configuration.config_pr_tp);
	CONFIGURATION__CONFIG_PR_TP__PR_SUPP__MODIFY(reg, generic_device_config->presence_rates_supported);
	temp  = tp_field(ISOCHRONOUS, generic_device_config->transport_protocol_isochronous);
	temp |= (uint32_t)tp_field(PUSHED, generic_device_config->transport_protocol_pushed);
	temp |= (uint32_t)tp_field(PULLED, generic_device_config->transport_protocol_pulled);
	CONFIGURATION__CONFIG_PR_TP__TP_SUPP__MODIFY(reg, temp);
	slimbus_drv_write_reg(configuration.config_pr_tp, reg);

	reg = slimbus_drv_read_reg(configuration.config_cport);
	CONFIGURATION__CONFIG_CPORT__CPORT_CLK_DIV__MODIFY(reg, generic_device_config->cport_clock_divider);
	slimbus_drv_write_reg(configuration.config_cport, reg);

	reg = slimbus_drv_read_reg(configuration.config_dport);
	CONFIGURATION__CONFIG_DPORT__SINK_START_LVL__MODIFY(reg, generic_device_config->sink_start_level);
	CONFIGURATION__CONFIG_DPORT__DPORT_CLK_PRESC__MODIFY(reg, generic_device_config->data_port_clock_prescaler);
	CONFIGURATION__CONFIG_DPORT__REFCLK_SEL__MODIFY(reg, generic_device_config->reference_clock_selector);
	slimbus_drv_write_reg(configuration.config_dport, reg);

	reg = slimbus_drv_read_reg(configuration.config_thr);
	CONFIGURATION__CONFIG_THR__SRC_THR__MODIFY(reg, generic_device_config->dma_treshold_source);
	CONFIGURATION__CONFIG_THR__SINK_THR__MODIFY(reg, generic_device_config->dma_treshold_sink);
	slimbus_drv_write_reg(configuration.config_thr, reg);

	cfg_strobe_set(instance, 0);

	return 0;
}


static uint32_t slimbus_drv_get_generic_device_config(void* pd, slimbus_generic_device_config* generic_device_config)
{
	slimbus_drv_instance* instance = NULL;
	uint32_t reg;
	uint8_t temp;

	uint32_t result = slimbus_get_generic_device_config_sanity(pd, generic_device_config);
	if (result) {
		slimbus_core_limit_err("check sanity fail\n");
		return result;
	}

	instance = (slimbus_drv_instance*) pd;

	reg = slimbus_drv_read_reg(configuration.config_pr_tp);
	generic_device_config->presence_rates_supported = CONFIGURATION__CONFIG_PR_TP__PR_SUPP__READ(reg);
	temp = CONFIGURATION__CONFIG_PR_TP__TP_SUPP__READ(reg);
	generic_device_config->transport_protocol_isochronous = get_tp_field(ISOCHRONOUS, temp);
	generic_device_config->transport_protocol_pushed = get_tp_field(PUSHED, temp);
	generic_device_config->transport_protocol_pulled = get_tp_field(PULLED, temp);

	reg = slimbus_drv_read_reg(configuration.config_cport);
	generic_device_config->cport_clock_divider = CONFIGURATION__CONFIG_CPORT__CPORT_CLK_DIV__READ(reg);

	reg = slimbus_drv_read_reg(configuration.config_dport);
	generic_device_config->sink_start_level = CONFIGURATION__CONFIG_DPORT__SINK_START_LVL__READ(reg);
	generic_device_config->data_port_clock_prescaler = CONFIGURATION__CONFIG_DPORT__DPORT_CLK_PRESC__READ(reg);
	generic_device_config->reference_clock_selector = CONFIGURATION__CONFIG_DPORT__REFCLK_SEL__READ(reg);

	reg = slimbus_drv_read_reg(configuration.config_thr);
	generic_device_config->dma_treshold_source = CONFIGURATION__CONFIG_THR__SRC_THR__READ(reg);
	generic_device_config->dma_treshold_sink = CONFIGURATION__CONFIG_THR__SINK_THR__READ(reg);

	return 0;
}

static uint32_t slimbus_drv_get_data_port_status(void* pd, uint8_t port_number, slimbus_data_port_status* port_status)
{
	slimbus_drv_instance* instance = NULL;
	uint32_t reg;

	uint32_t result = slimbus_get_data_port_status_sanity(pd, port_number, port_status);
	if (result) {
		slimbus_core_limit_err("check sanity fail\n");
		return result;
	}

	instance = (slimbus_drv_instance*) pd;

	reg = slimbus_drv_read_reg(port_state[port_number].p_state_0);
	port_status->active = PORT_STATE__P_STATE_0__ACTIVE__READ(reg);
	port_status->content_defined = PORT_STATE__P_STATE_0__CONTENT_DEFINED__READ(reg);
	port_status->channel_defined = PORT_STATE__P_STATE_0__CHANNEL_DEFINED__READ(reg);
	port_status->sink = PORT_STATE__P_STATE_0__SINK__READ(reg);
	port_status->overflow = PORT_STATE__P_STATE_0__OVF__READ(reg);
	port_status->underrun = PORT_STATE__P_STATE_0__UND__READ(reg);
	port_status->dport_ready = PORT_STATE__P_STATE_0__DPORT_READY__READ(reg);
	port_status->segment_interval = PORT_STATE__P_STATE_0__S_INTERVAL__READ(reg);
	port_status->transport_protocol = PORT_STATE__P_STATE_0__TR_PROTOCOL__READ(reg);

	reg = slimbus_drv_read_reg(port_state[port_number].p_state_1);
	port_status->presence_rate = PORT_STATE__P_STATE_1__P_RATE__READ(reg);
	port_status->frequency_lock = PORT_STATE__P_STATE_1__FR_LOCK__READ(reg);
	port_status->data_type = PORT_STATE__P_STATE_1__DATA_TYPE__READ(reg);
	port_status->data_length = PORT_STATE__P_STATE_1__DATA_LENGTH__READ(reg);
	port_status->port_linked = PORT_STATE__P_STATE_1__PORT_LINKED__READ(reg);
	port_status->channel_link = PORT_STATE__P_STATE_1__CH_LINK__READ(reg);

	return 0;
}

static uint32_t slimbus_drv_unfreeze(void* pd)
{
	slimbus_drv_instance* instance = NULL;

	uint32_t result = slimbus_unfreeze_sanity(pd);
	if (result) {
		slimbus_core_limit_err("check sanity fail\n");
		return result;
	}

	instance = (slimbus_drv_instance*) pd;
	slimbus_drv_write_reg(command_status.command, BIT(COMMAND_STATUS__COMMAND__UNFREEZE__SHIFT));

	return 0;
}


static uint32_t slimbus_drv_cancel_configuration(void* pd)
{
	slimbus_drv_instance* instance = NULL;

	uint32_t result = slimbus_cancel_configuration_sanity(pd);
	if (result) {
		slimbus_core_limit_err("check sanity fail\n");
		return result;
	}

	instance = (slimbus_drv_instance*) pd;
	slimbus_drv_write_reg(command_status.command, BIT(COMMAND_STATUS__COMMAND__CFG_STROBE_CLR__SHIFT));

	return 0;
}


static uint32_t slimbus_drv_get_status_synchronization(void* pd, bool* f_sync, bool* sf_sync, bool* m_sync, bool* sfb_sync, bool* ph_sync)
{
	slimbus_drv_instance* instance = NULL;
	uint32_t reg;

	uint32_t result = slimbus_get_status_synchronization_sanity(pd, f_sync, sf_sync, m_sync, sfb_sync, ph_sync);
	if (result) {
		slimbus_core_limit_err("check sanity fail\n");
		return result;
	}

	instance = (slimbus_drv_instance*) pd;

	reg = slimbus_drv_read_reg(command_status.state);
	if (f_sync != NULL)
		*f_sync = COMMAND_STATUS__STATE__F_SYNC__READ(reg);
	if (sf_sync != NULL)
		*sf_sync = COMMAND_STATUS__STATE__SF_SYNC__READ(reg);
	if (m_sync != NULL)
		*m_sync = COMMAND_STATUS__STATE__M_SYNC__READ(reg);
	if (sfb_sync != NULL)
		*sfb_sync = COMMAND_STATUS__STATE__SFB_SYNC__READ(reg);
	if (ph_sync != NULL)
		*ph_sync = COMMAND_STATUS__STATE__PH_SYNC__READ(reg);

	return 0;
}

static uint32_t slimbus_drv_get_status_detached(void* pd, bool* detached)
{
	slimbus_drv_instance* instance = NULL;
	uint32_t reg;

	uint32_t result = slimbus_get_status_detached_sanity(pd, detached);
	if (result) {
		slimbus_core_limit_err("check sanity fail\n");
		return result;
	}

	instance = (slimbus_drv_instance*) pd;

	reg = slimbus_drv_read_reg(command_status.state);
	*detached = COMMAND_STATUS__STATE__DETACHED__READ(reg);

	return 0;
}

static uint32_t slimbus_drv_get_status_slimbus(void* pd, slimbus_drv_subframe_mode* subframe_mode, slimbus_drv_clock_gear* clock_gear, slimbus_drv_root_frequency* root_fr)
{
	slimbus_drv_instance* instance = NULL;
	uint32_t reg;

	uint32_t result = slimbus_get_status_slimbus_sanity(pd, subframe_mode, clock_gear, root_fr);
	if (result) {
		slimbus_core_limit_err("check sanity fail\n");
		return result;
	}

	instance = (slimbus_drv_instance*) pd;

	reg = slimbus_drv_read_reg(command_status.state);
	if (subframe_mode != NULL)
	   *subframe_mode = COMMAND_STATUS__STATE__SUBFRAME_MODE__READ(reg);
	if (clock_gear != NULL)
	   *clock_gear = COMMAND_STATUS__STATE__CLOCK_GEAR__READ(reg);
	if (root_fr != NULL)
	   *root_fr = COMMAND_STATUS__STATE__ROOT_FR__READ(reg);

	return 0;
}

static uint32_t slimbus_drv_msg_assign_logical_address(void* pd, uint64_t destination_ea, uint8_t new_la)
{
	slimbus_drv_instance* instance = NULL;
	slimbus_drv_message tx_msg;

	uint32_t result = slimbus_msg_assign_logical_address_sanity(pd, destination_ea, new_la);
	if (result) {
		slimbus_core_limit_err("check sanity fail\n");
		return result;
	}

	instance = (slimbus_drv_instance*) pd;

	tx_msg.arbitration_type = AT_SHORT;
	tx_msg.source_address = MESSAGE_SOURCE_ACTIVE_MANAGER;
	tx_msg.arbitration_priority = AP_DEFAULT;
	tx_msg.message_type = MT_CORE;
	tx_msg.message_code = MESSAGE_CODE_ASSIGN_LOGICAL_ADDRESS;
	tx_msg.destination_type = DT_ENUMERATION_ADDRESS;
	tx_msg.destination_address = destination_ea;

	/*
	 * Payload:
	 * 0	LA [7:0]
	 */
	tx_msg.payload[0] = msg_payload_field(LOGICAL_ADDRESS, new_la);
	tx_msg.payload_length = 1;

	return transmit_message(instance, &tx_msg);
}


static uint32_t slimbus_drv_msg_reset_device(void* pd, uint8_t destination_la)
{
	slimbus_drv_instance* instance = NULL;
	slimbus_drv_message tx_msg;

	uint32_t result = slimbus_msg_reset_device_sanity(pd, destination_la);
	if (result) {
		slimbus_core_limit_err("check sanity fail\n");
		return result;
	}

	instance = (slimbus_drv_instance*) pd;

	tx_msg.arbitration_type = AT_SHORT;
	tx_msg.source_address = MESSAGE_SOURCE_ACTIVE_MANAGER;
	tx_msg.arbitration_priority = AP_DEFAULT;
	tx_msg.message_type = MT_CORE;
	tx_msg.message_code = MESSAGE_CODE_RESET_DEVICE;
	tx_msg.destination_type = DT_LOGICAL_ADDRESS;
	tx_msg.destination_address = destination_la;

	/*
	 * Payload:
	 * None
	 */
	tx_msg.payload_length = 0;

	return transmit_message(instance, &tx_msg);
}


static uint32_t slimbus_drv_msg_change_logical_address(void* pd, uint8_t destination_la, uint8_t new_la)
{
	slimbus_drv_instance* instance = NULL;
	slimbus_drv_message tx_msg;

	uint32_t result = slimbus_msg_change_logical_address_sanity(pd, destination_la, new_la);
	if (result) {
		slimbus_core_limit_err("check sanity fail\n");
		return result;
	}

	instance = (slimbus_drv_instance*) pd;

	tx_msg.arbitration_type = AT_SHORT;
	tx_msg.source_address = MESSAGE_SOURCE_ACTIVE_MANAGER;
	tx_msg.arbitration_priority = AP_DEFAULT;
	tx_msg.message_type = MT_CORE;
	tx_msg.message_code = MESSAGE_CODE_CHANGE_LOGICAL_ADDRESS;
	tx_msg.destination_type = DT_LOGICAL_ADDRESS;
	tx_msg.destination_address = destination_la;

	/*
	 * Payload:
	 * 0	LA [7:0]
	 */
	tx_msg.payload[0] = msg_payload_field(LOGICAL_ADDRESS, new_la);
	tx_msg.payload_length = 1;

	return transmit_message(instance, &tx_msg);
}


static uint32_t slimbus_drv_msg_change_arbitration_priority(void* pd, bool broadcast, uint8_t destination_la, slimbus_arbitration_priority new_arbitration_priority)
{
	slimbus_drv_instance* instance = NULL;
	slimbus_drv_message tx_msg;

	uint32_t result = slimbus_msg_change_arbitration_priority_sanity(pd, broadcast, destination_la, new_arbitration_priority);
	if (result) {
		slimbus_core_limit_err("check sanity fail\n");
		return result;
	}

	instance = (slimbus_drv_instance*) pd;

	tx_msg.arbitration_type = AT_SHORT;
	tx_msg.source_address = MESSAGE_SOURCE_ACTIVE_MANAGER;
	tx_msg.arbitration_priority = AP_DEFAULT;
	tx_msg.message_type = MT_CORE;
	tx_msg.message_code = MESSAGE_CODE_CHANGE_ARBITRATION_PRIORITY;
	if (broadcast) {
		tx_msg.destination_type = DT_BROADCAST;
		tx_msg.destination_address = 0x00;
	} else {
		tx_msg.destination_type = DT_LOGICAL_ADDRESS;
		tx_msg.destination_address = destination_la;
	}

	/*
	 * Payload:
	 * 0	AP [2:0]
	 */
	tx_msg.payload[0] = msg_payload_field(ARBITRATION_PRIORITY, new_arbitration_priority);
	tx_msg.payload_length = 1;

	return transmit_message(instance, &tx_msg);
}


static uint32_t slimbus_drv_msg_request_self_announcement(void* pd)
{
	slimbus_drv_instance* instance = NULL;
	slimbus_drv_message tx_msg;

	uint32_t result = slimbus_msg_request_self_announcement_sanity(pd);
	if (result) {
		slimbus_core_limit_err("check sanity fail\n");
		return result;
	}

	instance = (slimbus_drv_instance*) pd;

	tx_msg.arbitration_type = AT_SHORT;
	tx_msg.source_address = MESSAGE_SOURCE_ACTIVE_MANAGER;
	tx_msg.arbitration_priority = AP_DEFAULT;
	tx_msg.message_type = MT_CORE;
	tx_msg.message_code = MESSAGE_CODE_REQUEST_SELF_ANNOUNCEMENT;
	tx_msg.destination_type = DT_BROADCAST;
	tx_msg.destination_address = 0x00;

	/*
	 * Payload:
	 * None
	 */
	tx_msg.payload_length = 0;

	return transmit_message(instance, &tx_msg);
}


static uint32_t slimbus_drv_msg_connect_source(void* pd, uint8_t destination_la, uint8_t port_number, uint8_t channel_number)
{
	slimbus_drv_instance* instance = NULL;
	slimbus_drv_message tx_msg;

	uint32_t result = slimbus_msg_connect_source_sanity(pd, destination_la, port_number, channel_number);
	if (result) {
		slimbus_core_limit_err("check sanity fail\n");
		return result;
	}

	instance = (slimbus_drv_instance*) pd;

	tx_msg.arbitration_type = AT_SHORT;
	tx_msg.source_address = MESSAGE_SOURCE_ACTIVE_MANAGER;
	tx_msg.arbitration_priority = AP_DEFAULT;
	tx_msg.message_type = MT_CORE;
	tx_msg.message_code = MESSAGE_CODE_CONNECT_SOURCE;
	tx_msg.destination_type = DT_LOGICAL_ADDRESS;
	tx_msg.destination_address = destination_la;

	/*
	 * Payload:
	 * 0	PN [5:0]
	 * 1	CN [7:0]
	 */
	tx_msg.payload[0] = msg_payload_field(PORT_NUMBER, port_number);
	tx_msg.payload[1] = msg_payload_field(DATA_CHANNEL_NUMBER, channel_number);
	tx_msg.payload_length = 2;

	return transmit_message(instance, &tx_msg);
}


static uint32_t slimbus_drv_msg_connect_sink(void* pd, uint8_t destination_la, uint8_t port_number, uint8_t channel_number)
{
	slimbus_drv_instance* instance = NULL;
	slimbus_drv_message tx_msg;

	uint32_t result = slimbus_msg_connect_sink_sanity(pd, destination_la, port_number, channel_number);
	if (result) {
		slimbus_core_limit_err("check sanity fail\n");
		return result;
	}

	instance = (slimbus_drv_instance*) pd;

	tx_msg.arbitration_type = AT_SHORT;
	tx_msg.source_address = MESSAGE_SOURCE_ACTIVE_MANAGER;
	tx_msg.arbitration_priority = AP_DEFAULT;
	tx_msg.message_type = MT_CORE;
	tx_msg.message_code = MESSAGE_CODE_CONNECT_SINK;
	tx_msg.destination_type = DT_LOGICAL_ADDRESS;
	tx_msg.destination_address = destination_la;

	/*
	 * Payload:
	 * 0	PN [5:0]
	 * 1	CN [7:0]
	 */
	tx_msg.payload[0] = msg_payload_field(PORT_NUMBER, port_number);
	tx_msg.payload[1] = msg_payload_field(DATA_CHANNEL_NUMBER, channel_number);
	tx_msg.payload_length = 2;

	return transmit_message(instance, &tx_msg);
}


static uint32_t slimbus_drv_msg_disconnect_port(void* pd, uint8_t destination_la, uint8_t port_number)
{
	slimbus_drv_instance* instance = NULL;
	slimbus_drv_message tx_msg;

	uint32_t result = slimbus_msg_disconnect_port_sanity(pd, destination_la, port_number);
	if (result) {
		slimbus_core_limit_err("check sanity fail\n");
		return result;
	}

	instance = (slimbus_drv_instance*) pd;

	tx_msg.arbitration_type = AT_SHORT;
	tx_msg.source_address = MESSAGE_SOURCE_ACTIVE_MANAGER;
	tx_msg.arbitration_priority = AP_DEFAULT;
	tx_msg.message_type = MT_CORE;
	tx_msg.message_code = MESSAGE_CODE_DISCONNECT_PORT;
	tx_msg.destination_type = DT_LOGICAL_ADDRESS;
	tx_msg.destination_address = destination_la;

	/*
	 * Payload:
	 * 0	PN [5:0]
	 */
	tx_msg.payload[0] = msg_payload_field(PORT_NUMBER, port_number);
	tx_msg.payload_length = 1;

	return transmit_message(instance, &tx_msg);
}


static uint32_t slimbus_drv_msg_change_content(void* pd, uint8_t channel_number, bool frequency_locked_bit, slimbus_drv_presence_rate presence_rate, slimbus_drv_aux_field_format auxiliary_bit_format,  slimbus_drv_data_type data_type, bool channel_link, uint8_t data_length)
{
	slimbus_drv_instance* instance = NULL;
	slimbus_drv_message tx_msg;

	uint32_t result = slimbus_msg_change_content_sanity(pd, channel_number, frequency_locked_bit, presence_rate, auxiliary_bit_format, data_type, channel_link, data_length);
	if (result) {
		slimbus_core_limit_err("check sanity fail\n");
		return result;
	}

	instance = (slimbus_drv_instance*) pd;

	tx_msg.arbitration_type = AT_SHORT;
	tx_msg.source_address = MESSAGE_SOURCE_ACTIVE_MANAGER;
	tx_msg.arbitration_priority = AP_DEFAULT;
	tx_msg.message_type = MT_CORE;
	tx_msg.message_code = MESSAGE_CODE_CHANGE_CONTENT;
	tx_msg.destination_type = DT_BROADCAST;
	tx_msg.destination_address = 0x00;

	/*
	 * Payload:
	 * 0	CN [7:0]
	 * 1	FL [7:7]	PR [6:0]
	 * 2	AF [7:4]	DT [3:0]
	 * 3	CL [5:5]	DL [4:0]
	 */
	tx_msg.payload[0] = msg_payload_field(DATA_CHANNEL_NUMBER, channel_number);
	tx_msg.payload[1] = msg_payload_field(PRESENCE_RATE, presence_rate) | msg_payload_field(FREQUENCY_LOCKED_BIT, (uint8_t)frequency_locked_bit);
	tx_msg.payload[2] = msg_payload_field(DATA_TYPE, data_type) | msg_payload_field(AUXILIARY_BIT_FORMAT, auxiliary_bit_format);
	tx_msg.payload[3] = msg_payload_field(DATA_LENGTH, data_length) | msg_payload_field(CHANNEL_LINK, (uint8_t)channel_link);
	tx_msg.payload_length = 4;

	return transmit_message(instance, &tx_msg);
}


static uint32_t slimbus_drv_msg_request_information(void* pd, bool broadcast, uint8_t destination_la, uint8_t transaction_id, uint16_t element_code)
{
	slimbus_drv_instance* instance = NULL;
	slimbus_drv_message tx_msg;

	uint32_t result = slimbus_msg_request_information_sanity(pd, broadcast, destination_la, transaction_id, element_code);
	if (result) {
		slimbus_core_limit_err("check sanity fail\n");
		return result;
	}

	instance = (slimbus_drv_instance*) pd;

	tx_msg.arbitration_type = AT_SHORT;
	tx_msg.source_address = MESSAGE_SOURCE_ACTIVE_MANAGER;
	tx_msg.arbitration_priority = AP_DEFAULT;
	tx_msg.message_type = MT_CORE;
	tx_msg.message_code = MESSAGE_CODE_REQUEST_INFORMATION;
	if (broadcast) {
		tx_msg.destination_type = DT_BROADCAST;
		tx_msg.destination_address = 0x00;
	} else {
		tx_msg.destination_type = DT_LOGICAL_ADDRESS;
		tx_msg.destination_address = destination_la;
	}

	/*
	 * Payload:
	 * 0	TID [7:0]
	 * 1	EC [ 7:0]
	 * 2	EC [15:8]
	 */
	tx_msg.payload[0] = msg_payload_field(TRANSACTION_ID, transaction_id);
	tx_msg.payload[1] = lower_byte(element_code);
	tx_msg.payload[2] = higher_byte(element_code);
	tx_msg.payload_length = 3;

	return transmit_message(instance, &tx_msg);
}


static uint32_t slimbus_drv_msg_request_clear_information(void* pd, bool broadcast, uint8_t destination_la, uint8_t transaction_id, uint16_t element_code, uint8_t* clear_mask, uint8_t clear_mask_size)
{
	slimbus_drv_instance* instance = NULL;
	slimbus_drv_message tx_msg;
	uint8_t i;
	uint32_t result = slimbus_msg_request_clear_information_sanity(pd, broadcast, destination_la, transaction_id, element_code, clear_mask, clear_mask_size);

	if (result) {
		slimbus_core_limit_err("check sanity fail\n");
		return result;
	}

	if ((clear_mask == NULL) && (clear_mask_size > 0)) {
		slimbus_core_limit_err("input param is invalid\n");
		return EINVAL;
	}

	instance = (slimbus_drv_instance*) pd;

	tx_msg.arbitration_type = AT_SHORT;
	tx_msg.source_address = MESSAGE_SOURCE_ACTIVE_MANAGER;
	tx_msg.arbitration_priority = AP_DEFAULT;
	tx_msg.message_type = MT_CORE;
	tx_msg.message_code = MESSAGE_CODE_REQUEST_CLEAR_INFORMATION;
	if (broadcast) {
		tx_msg.destination_type = DT_BROADCAST;
		tx_msg.destination_address = 0x00;
	} else {
		tx_msg.destination_type = DT_LOGICAL_ADDRESS;
		tx_msg.destination_address = destination_la;
	}

	/*
	 * Payload:
	 * 0	TID [7:0]
	 * 1	EC [ 7:0]
	 * 2	EC [15:8]
	 * 3	CM [ 7:0]
	 * N+2	CM [8N-1: 8N-8]
	 */
	tx_msg.payload[0] = msg_payload_field(TRANSACTION_ID, transaction_id);
	tx_msg.payload[1] = lower_byte(element_code);
	tx_msg.payload[2] = higher_byte(element_code);
	for (i = 0; i < clear_mask_size; i++)
		tx_msg.payload[i+3] = clear_mask[i];
	tx_msg.payload_length = 3 + clear_mask_size;

	return transmit_message(instance, &tx_msg);
}


static uint32_t slimbus_drv_msg_clear_information(void* pd, bool broadcast, uint8_t destination_la, uint16_t element_code, uint8_t* clear_mask, uint8_t clear_mask_size)
{
	slimbus_drv_instance* instance = NULL;
	slimbus_drv_message tx_msg;
	uint8_t i;
	uint32_t result = slimbus_msg_clear_information_sanity(pd, broadcast, destination_la, element_code, clear_mask, clear_mask_size);

	if (result) {
		slimbus_core_limit_err("check sanity fail\n");
		return result;
	}

	if ((clear_mask == NULL) && (clear_mask_size > 0)) {
		slimbus_core_limit_err("input param is invalid\n");
		return EINVAL;
	}

	instance = (slimbus_drv_instance*) pd;

	tx_msg.arbitration_type = AT_SHORT;
	tx_msg.source_address = MESSAGE_SOURCE_ACTIVE_MANAGER;
	tx_msg.arbitration_priority = AP_DEFAULT;
	tx_msg.message_type = MT_CORE;
	tx_msg.message_code = MESSAGE_CODE_CLEAR_INFORMATION;
	if (broadcast) {
		tx_msg.destination_type = DT_BROADCAST;
		tx_msg.destination_address = 0x00;
	} else {
		tx_msg.destination_type = DT_LOGICAL_ADDRESS;
		tx_msg.destination_address = destination_la;
	}

	/*
	 * Payload:
	 * 0	EC [ 7:0]
	 * 1	EC [15:8]
	 * 2	CM [ 7:0]
	 * N+1	CM [8N-1: 8N-8]
	 */
	tx_msg.payload[0] = lower_byte(element_code);
	tx_msg.payload[1] = higher_byte(element_code);
	for (i = 0; i < clear_mask_size; i++)
		tx_msg.payload[i+2] = clear_mask[i];
	tx_msg.payload_length = 2 + clear_mask_size;

	return transmit_message(instance, &tx_msg);
}


static uint32_t slimbus_drv_msg_begin_reconfiguration(void* pd)
{
	slimbus_drv_instance* instance = NULL;
	slimbus_drv_message tx_msg;

	uint32_t result = slimbus_msg_begin_reconfiguration_sanity(pd);
	if (result) {
		slimbus_core_limit_err("check sanity fail\n");
		return result;
	}

	instance = (slimbus_drv_instance*) pd;

	tx_msg.arbitration_type = AT_SHORT;
	tx_msg.source_address = MESSAGE_SOURCE_ACTIVE_MANAGER;
	tx_msg.arbitration_priority = AP_DEFAULT;
	tx_msg.message_type = MT_CORE;
	tx_msg.message_code = MESSAGE_CODE_BEGIN_RECONFIGURATION;
	tx_msg.destination_type = DT_BROADCAST;
	tx_msg.destination_address = 0x00;

	/*
	 * Payload:
	 * None
	 */
	tx_msg.payload_length = 0;

	return transmit_message(instance, &tx_msg);
}


static uint32_t slimbus_drv_msg_next_active_framer(void* pd, uint8_t incoming_framer_la, uint16_t outgoing_framer_clock_cycles, uint16_t incoming_framer_clock_cycles)
{
	slimbus_drv_instance* instance = NULL;
	slimbus_drv_message tx_msg;

	uint32_t result = slimbus_msg_next_active_framer_sanity(pd, incoming_framer_la, outgoing_framer_clock_cycles, incoming_framer_clock_cycles);
	if (result) {
		slimbus_core_limit_err("check sanity fail\n");
		return result;
	}

	instance = (slimbus_drv_instance*) pd;

	tx_msg.arbitration_type = AT_SHORT;
	tx_msg.source_address = MESSAGE_SOURCE_ACTIVE_MANAGER;
	tx_msg.arbitration_priority = AP_DEFAULT;
	tx_msg.message_type = MT_CORE;
	tx_msg.message_code = MESSAGE_CODE_NEXT_ACTIVE_FRAMER;
	tx_msg.destination_type = DT_BROADCAST;
	tx_msg.destination_address = 0x00;

	/*
	 * Payload:
	 * 0	LAIF [7:0]
	 * 1	NCo  [7:0]
	 * 2	NCi  [3:0]	NCo [11:8]
	 * 3	NCo [11:4]
	 */
	tx_msg.payload[0] = msg_payload_field(LOGICAL_ADDRESS, incoming_framer_la);
	tx_msg.payload[1] = msg_payload_field(OUTGOING_FRAMER_CYCLES_LOW, lower_byte(outgoing_framer_clock_cycles));
	tx_msg.payload[2] = msg_payload_field(OUTGOING_FRAMER_CYCLES_HIGH, higher_byte(outgoing_framer_clock_cycles))
					 | msg_payload_field(INCOMING_FRAMER_CYCLES_LOW, lower_byte(incoming_framer_clock_cycles));
	tx_msg.payload[3] = msg_payload_field(INCOMING_FRAMER_CYCLES_HIGH, higher_byte(incoming_framer_clock_cycles));
	tx_msg.payload_length = 4;

	return transmit_message(instance, &tx_msg);
}


static uint32_t slimbus_drv_msg_next_subframe_mode(void* pd, slimbus_drv_subframe_mode new_subframe_mode)
{
	slimbus_drv_instance* instance = NULL;
	slimbus_drv_message tx_msg;

	uint32_t result = slimbus_msg_next_subframe_mode_sanity(pd, new_subframe_mode);
	if (result) {
		slimbus_core_limit_err("check sanity fail\n");
		return result;
	}

	instance = (slimbus_drv_instance*) pd;

	tx_msg.arbitration_type = AT_SHORT;
	tx_msg.source_address = MESSAGE_SOURCE_ACTIVE_MANAGER;
	tx_msg.arbitration_priority = AP_DEFAULT;
	tx_msg.message_type = MT_CORE;
	tx_msg.message_code = MESSAGE_CODE_NEXT_SUBFRAME_MODE;
	tx_msg.destination_type = DT_BROADCAST;
	tx_msg.destination_address = 0x00;

	/*
	 * Payload:
	 * 0	SM [4:0]
	 */
	tx_msg.payload[0] = msg_payload_field(SUBFRAME_MODE, new_subframe_mode);
	tx_msg.payload_length = 1;

	return transmit_message(instance, &tx_msg);
}


static uint32_t slimbus_drv_msg_next_clock_gear(void* pd, slimbus_drv_clock_gear new_clock_gear)
{
	slimbus_drv_instance* instance = NULL;
	slimbus_drv_message tx_msg;

	uint32_t result = slimbus_msg_next_clock_gear_sanity(pd, new_clock_gear);
	if (result) {
		slimbus_core_limit_err("check sanity fail\n");
		return result;
	}

	instance = (slimbus_drv_instance*) pd;

	tx_msg.arbitration_type = AT_SHORT;
	tx_msg.source_address = MESSAGE_SOURCE_ACTIVE_MANAGER;
	tx_msg.arbitration_priority = AP_DEFAULT;
	tx_msg.message_type = MT_CORE;
	tx_msg.message_code = MESSAGE_CODE_NEXT_CLOCK_GEAR;
	tx_msg.destination_type = DT_BROADCAST;
	tx_msg.destination_address = 0x00;

	/*
	 * Payload:
	 * 0	CG [3:0]
	 */
	tx_msg.payload[0] = msg_payload_field(CLOCK_GEAR, new_clock_gear);
	tx_msg.payload_length = 1;

	return transmit_message(instance, &tx_msg);
}


static uint32_t slimbus_drv_msg_next_root_frequency(void* pd, slimbus_drv_root_frequency new_root_frequency)
{
	slimbus_drv_instance* instance = NULL;
	slimbus_drv_message tx_msg;

	uint32_t result = slimbus_msg_next_root_frequency_sanity(pd, new_root_frequency);
	if (result) {
		slimbus_core_limit_err("check sanity fail\n");
		return result;
	}

	instance = (slimbus_drv_instance*) pd;

	tx_msg.arbitration_type = AT_SHORT;
	tx_msg.source_address = MESSAGE_SOURCE_ACTIVE_MANAGER;
	tx_msg.arbitration_priority = AP_DEFAULT;
	tx_msg.message_type = MT_CORE;
	tx_msg.message_code = MESSAGE_CODE_NEXT_ROOT_FREQUENCY;
	tx_msg.destination_type = DT_BROADCAST;
	tx_msg.destination_address = 0x00;

	/*
	 * Payload:
	 * 0	RF [3:0]
	 */
	tx_msg.payload[0] = msg_payload_field(ROOT_FREQUENCY, new_root_frequency);
	tx_msg.payload_length = 1;

	return transmit_message(instance, &tx_msg);
}


static uint32_t slimbus_drv_msg_next_pause_clock(void* pd, slimbus_drv_restart_time new_restart_time)
{
	slimbus_drv_instance* instance = NULL;
	slimbus_drv_message tx_msg;

	uint32_t result = slimbus_msg_next_pause_clock_sanity(pd, new_restart_time);
	if (result) {
		slimbus_core_limit_err("check sanity fail\n");
		return result;
	}

	instance = (slimbus_drv_instance*) pd;

	tx_msg.arbitration_type = AT_SHORT;
	tx_msg.source_address = MESSAGE_SOURCE_ACTIVE_MANAGER;
	tx_msg.arbitration_priority = AP_DEFAULT;
	tx_msg.message_type = MT_CORE;
	tx_msg.message_code = MESSAGE_CODE_NEXT_PAUSE_CLOCK;
	tx_msg.destination_type = DT_BROADCAST;
	tx_msg.destination_address = 0x00;

	/*
	 * Payload:
	 * 0	RT [7:0]
	 */
	tx_msg.payload[0] = msg_payload_field(RESTART_TIME, new_restart_time);
	tx_msg.payload_length = 1;

	return transmit_message(instance, &tx_msg);
}


static uint32_t slimbus_drv_msg_next_reset_bus(void* pd)
{
	slimbus_drv_instance* instance = NULL;
	slimbus_drv_message tx_msg;

	uint32_t result = slimbus_msg_next_reset_bus_sanity(pd);
	if (result) {
		slimbus_core_limit_err("check sanity fail\n");
		return result;
	}

	instance = (slimbus_drv_instance*) pd;

	tx_msg.arbitration_type = AT_SHORT;
	tx_msg.source_address = MESSAGE_SOURCE_ACTIVE_MANAGER;
	tx_msg.arbitration_priority = AP_DEFAULT;
	tx_msg.message_type = MT_CORE;
	tx_msg.message_code = MESSAGE_CODE_NEXT_RESET_BUS;
	tx_msg.destination_type = DT_BROADCAST;
	tx_msg.destination_address = 0x00;

	/*
	 * Payload:
	 * None
	 */
	tx_msg.payload_length = 0;

	return transmit_message(instance, &tx_msg);
}


static uint32_t slimbus_drv_msg_next_shutdown_bus(void* pd)
{
	slimbus_drv_instance* instance = NULL;
	slimbus_drv_message tx_msg;

	uint32_t result = slimbus_msg_next_shutdown_bus_sanity(pd);
	if (result) {
		slimbus_core_limit_err("check sanity fail\n");
		return result;
	}

	instance = (slimbus_drv_instance*) pd;

	tx_msg.arbitration_type = AT_SHORT;
	tx_msg.source_address = MESSAGE_SOURCE_ACTIVE_MANAGER;
	tx_msg.arbitration_priority = AP_DEFAULT;
	tx_msg.message_type = MT_CORE;
	tx_msg.message_code = MESSAGE_CODE_NEXT_SHUTDOWN_BUS;
	tx_msg.destination_type = DT_BROADCAST;
	tx_msg.destination_address = 0x00;

	/*
	 * Payload:
	 * None
	 */
	tx_msg.payload_length = 0;

	return transmit_message(instance, &tx_msg);
}


static uint32_t slimbus_drv_msg_next_define_channel(void* pd, uint8_t channel_number, slimbus_drv_transport_protocol transport_protocol, uint16_t segment_distribution, uint8_t segment_length)
{
	slimbus_drv_instance* instance = NULL;
	slimbus_drv_message tx_msg;

	uint32_t result = slimbus_msg_next_define_channel_sanity(pd, channel_number, transport_protocol, segment_distribution, segment_length);
	if (result) {
		slimbus_core_limit_err("check sanity fail\n");
		return result;
	}

	instance = (slimbus_drv_instance*) pd;

	tx_msg.arbitration_type = AT_SHORT;
	tx_msg.source_address = MESSAGE_SOURCE_ACTIVE_MANAGER;
	tx_msg.arbitration_priority = AP_DEFAULT;
	tx_msg.message_type = MT_CORE;
	tx_msg.message_code = MESSAGE_CODE_NEXT_DEFINE_CHANNEL;
	tx_msg.destination_type = DT_BROADCAST;
	tx_msg.destination_address = 0x00;

	/*
	 * Payload:
	 * 0	CN [7:0]
	 * 1	SD [7:0]
	 * 2	TP [7:4]	SD [11:8]
	 * 3				SL [4:0]
	 */
	tx_msg.payload[0] = msg_payload_field(DATA_CHANNEL_NUMBER, channel_number);
	tx_msg.payload[1] = msg_payload_field(SEGMENT_DISTRIBUTION_LOW, lower_byte(segment_distribution));
	tx_msg.payload[2] = msg_payload_field(SEGMENT_DISTRIBUTION_HIGH, higher_byte(segment_distribution))
					 | msg_payload_field(TRANSPORT_PROTOCOL, transport_protocol);
	tx_msg.payload[3] = msg_payload_field(SEGMENT_LENGTH, segment_length);
	tx_msg.payload_length = 4;

	return transmit_message(instance, &tx_msg);
}


static uint32_t slimbus_drv_msg_next_define_content(void* pd, uint8_t channel_number, bool frequency_locked_bit, slimbus_drv_presence_rate presence_rate, slimbus_drv_aux_field_format auxiliary_bit_format,  slimbus_drv_data_type data_type, bool channel_link, uint8_t data_length)
{
	slimbus_drv_instance* instance = NULL;
	slimbus_drv_message tx_msg;

	uint32_t result = slimbus_msg_next_define_content_sanity(pd, channel_number, frequency_locked_bit, presence_rate, auxiliary_bit_format, data_type, channel_link, data_length);
	if (result) {
		slimbus_core_limit_err("check sanity fail\n");
		return result;
	}

	instance = (slimbus_drv_instance*) pd;

	tx_msg.arbitration_type = AT_SHORT;
	tx_msg.source_address = MESSAGE_SOURCE_ACTIVE_MANAGER;
	tx_msg.arbitration_priority = AP_DEFAULT;
	tx_msg.message_type = MT_CORE;
	tx_msg.message_code = MESSAGE_CODE_NEXT_DEFINE_CONTENT;
	tx_msg.destination_type = DT_BROADCAST;
	tx_msg.destination_address = 0x00;

	/*
	 * Payload:
	 * 0	CN [7:0]
	 * 1	FL [7:7]	PR [6:0]
	 * 2	AF [7:4]	DT [3:0]
	 * 3	CL [5:5]	DL [4:0]
	 */
	tx_msg.payload[0] = msg_payload_field(DATA_CHANNEL_NUMBER, channel_number);
	tx_msg.payload[1] = msg_payload_field(FREQUENCY_LOCKED_BIT, (uint8_t)frequency_locked_bit)
					 | msg_payload_field(PRESENCE_RATE, presence_rate);
	tx_msg.payload[2] = msg_payload_field(AUXILIARY_BIT_FORMAT, auxiliary_bit_format)
					 | msg_payload_field(DATA_TYPE, data_type);
	tx_msg.payload[3] = msg_payload_field(CHANNEL_LINK, (uint8_t)channel_link)
					 | msg_payload_field(DATA_LENGTH, data_length);
	tx_msg.payload_length = 4;

	return transmit_message(instance, &tx_msg);
}


static uint32_t slimbus_drv_msg_next_activate_channel(void* pd, uint8_t channel_number)
{
	slimbus_drv_instance* instance = NULL;
	slimbus_drv_message tx_msg;
	uint32_t result = slimbus_msg_next_activate_channel_sanity(pd, channel_number);

	if (result) {
		slimbus_core_limit_err("check sanity fail\n");
		return result;
	}

	instance = (slimbus_drv_instance*) pd;

	tx_msg.arbitration_type = AT_SHORT;
	tx_msg.source_address = MESSAGE_SOURCE_ACTIVE_MANAGER;
	tx_msg.arbitration_priority = AP_DEFAULT;
	tx_msg.message_type = MT_CORE;
	tx_msg.message_code = MESSAGE_CODE_NEXT_ACTIVATE_CHANNEL;
	tx_msg.destination_type = DT_BROADCAST;
	tx_msg.destination_address = 0x00;

	/*
	 * Payload:
	 * 0	CN [7:0]
	 */
	tx_msg.payload[0] = msg_payload_field(DATA_CHANNEL_NUMBER, channel_number);
	tx_msg.payload_length = 1;

	return transmit_message(instance, &tx_msg);
}


static uint32_t slimbus_drv_msg_next_deactivate_channel(void* pd, uint8_t channel_number)
{
	slimbus_drv_instance* instance = NULL;
	slimbus_drv_message tx_msg;

	uint32_t result = slimbus_msg_next_deactivate_channel_sanity(pd, channel_number);
	if (result) {
		slimbus_core_limit_err("check sanity fail\n");
		return result;
	}

	instance = (slimbus_drv_instance*) pd;

	tx_msg.arbitration_type = AT_SHORT;
	tx_msg.source_address = MESSAGE_SOURCE_ACTIVE_MANAGER;
	tx_msg.arbitration_priority = AP_DEFAULT;
	tx_msg.message_type = MT_CORE;
	tx_msg.message_code = MESSAGE_CODE_NEXT_DEACTIVATE_CHANNEL;
	tx_msg.destination_type = DT_BROADCAST;
	tx_msg.destination_address = 0x00;

	/*
	 * Payload:
	 * 0	CN [7:0]
	 */
	tx_msg.payload[0] = msg_payload_field(DATA_CHANNEL_NUMBER, channel_number);
	tx_msg.payload_length = 1;

	return transmit_message(instance, &tx_msg);
}


static uint32_t slimbus_drv_msg_next_remove_channel(void* pd, uint8_t channel_number)
{
	slimbus_drv_instance* instance = NULL;
	slimbus_drv_message tx_msg;

	uint32_t result = slimbus_msg_next_remove_channel_sanity(pd, channel_number);
	if (result) {
		slimbus_core_limit_err("check sanity fail\n");
		return result;
	}

	instance = (slimbus_drv_instance*) pd;

	tx_msg.arbitration_type = AT_SHORT;
	tx_msg.source_address = MESSAGE_SOURCE_ACTIVE_MANAGER;
	tx_msg.arbitration_priority = AP_DEFAULT;
	tx_msg.message_type = MT_CORE;
	tx_msg.message_code = MESSAGE_CODE_NEXT_REMOVE_CHANNEL;
	tx_msg.destination_type = DT_BROADCAST;
	tx_msg.destination_address = 0x00;

	/*
	 * Payload:
	 * 0	CN [7:0]
	 */
	tx_msg.payload[0] = msg_payload_field(DATA_CHANNEL_NUMBER, channel_number);
	tx_msg.payload_length = 1;

	return transmit_message(instance, &tx_msg);
}


static uint32_t slimbus_drv_msg_reconfigure_now(void* pd)
{
	slimbus_drv_instance* instance = NULL;
	slimbus_drv_message tx_msg;

	uint32_t result = slimbus_msg_reconfigure_now_sanity(pd);
	if (result) {
		slimbus_core_limit_err("check sanity fail\n");
		return result;
	}

	instance = (slimbus_drv_instance*) pd;

	tx_msg.arbitration_type = AT_SHORT;
	tx_msg.source_address = MESSAGE_SOURCE_ACTIVE_MANAGER;
	tx_msg.arbitration_priority = AP_DEFAULT;
	tx_msg.message_type = MT_CORE;
	tx_msg.message_code = MESSAGE_CODE_RECONFIGURE_NOW;
	tx_msg.destination_type = DT_BROADCAST;
	tx_msg.destination_address = 0x00;

	/*
	 * Payload:
	 * None
	 */
	tx_msg.payload_length = 0;

	return transmit_message(instance, &tx_msg);
}


static uint32_t slimbus_drv_msg_request_value(void* pd, bool broadcast, uint8_t destination_la, uint8_t transaction_id, uint16_t element_code)
{
	slimbus_drv_instance* instance = NULL;
	slimbus_drv_message tx_msg;

	uint32_t result = slimbus_msg_request_value_sanity(pd, broadcast, destination_la, transaction_id, element_code);
	if (result) {
		slimbus_core_limit_err("check sanity fail\n");
		return result;
	}

	instance = (slimbus_drv_instance*) pd;

	tx_msg.arbitration_type = AT_SHORT;
	tx_msg.source_address = MESSAGE_SOURCE_ACTIVE_MANAGER;
	tx_msg.arbitration_priority = AP_DEFAULT;
	tx_msg.message_type = MT_CORE;
	tx_msg.message_code = MESSAGE_CODE_REQUEST_VALUE;
	if (broadcast) {
		tx_msg.destination_type = DT_BROADCAST;
		tx_msg.destination_address = 0x00;
	} else {
		tx_msg.destination_type = DT_LOGICAL_ADDRESS;
		tx_msg.destination_address = destination_la;
	}

	/*
	 * Payload:
	 * 0	TID [7:0]
	 * 1	EC [ 7:0]
	 * 2	EC [15:8]
	 */
	tx_msg.payload[0] = msg_payload_field(TRANSACTION_ID, transaction_id);
	tx_msg.payload[1] = lower_byte(element_code);
	tx_msg.payload[2] = higher_byte(element_code);
	tx_msg.payload_length = 3;

	return transmit_message(instance, &tx_msg);
}

static uint32_t slimbus_drv_msg_request_change_value(void* pd, bool broadcast, uint8_t destination_la, uint8_t transaction_id, uint16_t element_code, uint8_t* value_update, uint8_t value_update_size)
{
	slimbus_drv_instance* instance = NULL;
	slimbus_drv_message tx_msg;
	uint8_t i;

	uint32_t result = slimbus_msg_request_change_value_sanity(pd, broadcast, destination_la, transaction_id, element_code, value_update, value_update_size);
	if (result) {
		slimbus_core_limit_err("check sanity fail\n");
		return result;
	}

	if ((value_update == NULL) && (value_update_size > 0)) {
		slimbus_core_limit_err("input param is invalid\n");
		return EINVAL;
	}

	instance = (slimbus_drv_instance*) pd;

	tx_msg.arbitration_type = AT_SHORT;
	tx_msg.source_address = MESSAGE_SOURCE_ACTIVE_MANAGER;
	tx_msg.arbitration_priority = AP_DEFAULT;
	tx_msg.message_type = MT_CORE;
	tx_msg.message_code = MESSAGE_CODE_REQUEST_CHANGE_VALUE;
	if (broadcast) {
		tx_msg.destination_type = DT_BROADCAST;
		tx_msg.destination_address = 0x00;
	} else {
		tx_msg.destination_type = DT_LOGICAL_ADDRESS;
		tx_msg.destination_address = destination_la;
	}

	/*
	 * Payload:
	 * 0	TID [7:0]
	 * 1	EC [ 7:0]
	 * 2	EC [15:8]
	 * 3	VU [ 7:0]
	 * N+2	VU [8N-1: 8N-8]
	 */
	tx_msg.payload[0] = msg_payload_field(TRANSACTION_ID, transaction_id);
	tx_msg.payload[1] = lower_byte(element_code);
	tx_msg.payload[2] = higher_byte(element_code);
	for (i = 0; i < value_update_size; i++)
		tx_msg.payload[i + ELEMENT_CODE_LENGTH + TRANSACTION_ID_LENGTH] = value_update[i];
	tx_msg.payload_length = ELEMENT_CODE_LENGTH + TRANSACTION_ID_LENGTH + value_update_size;

	return transmit_message(instance, &tx_msg);
}

static uint32_t slimbus_drv_msg_change_value(void* pd, bool broadcast, uint8_t destination_la, uint16_t element_code, uint8_t* value_update, uint8_t value_update_size)
{
	slimbus_drv_instance* instance = NULL;
	slimbus_drv_message tx_msg;
	uint8_t i;

	uint32_t result = slimbus_msg_change_value_sanity(pd, broadcast, destination_la, element_code, value_update, value_update_size);
	if (result) {
		slimbus_core_limit_err("check sanity fail\n");
		return result;
	}

	if ((value_update == NULL) && (value_update_size > 0)) {
		slimbus_core_limit_err("input param is invalid\n");
		return EINVAL;
	}

	instance = (slimbus_drv_instance*) pd;

	tx_msg.arbitration_type = AT_SHORT;
	tx_msg.source_address = MESSAGE_SOURCE_ACTIVE_MANAGER;
	tx_msg.arbitration_priority = AP_DEFAULT;
	tx_msg.message_type = MT_CORE;
	tx_msg.message_code = MESSAGE_CODE_CHANGE_VALUE;
	if (broadcast) {
		tx_msg.destination_type = DT_BROADCAST;
		tx_msg.destination_address = 0x00;
	} else {
		tx_msg.destination_type = DT_LOGICAL_ADDRESS;
		tx_msg.destination_address = destination_la;
	}

	/*
	 * Payload:
	 * 0	EC [ 7:0]
	 * 1	EC [15:8]
	 * 2	VU [ 7:0]
	 * N+1	VU [8N-1: 8N-8]
	 */
	tx_msg.payload[0] = lower_byte(element_code);
	tx_msg.payload[1] = higher_byte(element_code);
	for (i = 0; i < value_update_size; i++)
		tx_msg.payload[i + ELEMENT_CODE_LENGTH] = value_update[i];
	tx_msg.payload_length = ELEMENT_CODE_LENGTH + value_update_size;

	return transmit_message(instance, &tx_msg);
}

slimbus_drv_obj driver = {
	.probe = slimbus_drv_probe,
	.init = slimbus_drv_api_init,
	.isr = slimbus_drv_isr,
	.start = slimbus_drv_start,
	.stop = slimbus_drv_api_stop,
	.destroy = slimbus_drv_destroy,
	.set_interrupts = slimbus_drv_set_interrupts,
	.get_interrupts = slimbus_drv_get_interrupts,
	.set_data_port_interrupts = slimbus_drv_set_data_port_interrupts,
	.get_data_port_interrupts = slimbus_drv_get_data_port_interrupts,
	.clear_data_port_fifo = slimbus_drv_clear_data_port_fifo,
	.set_presence_rate_generation = slimbus_drv_set_presence_rate_generation,
	.get_presence_rate_generation = slimbus_drv_get_presence_rate_generation,
	.assign_message_callbacks = slimbus_drv_assign_message_callbacks,
	.send_raw_message = slimbus_drv_send_raw_message,
	.send_message = slimbus_drv_send_message,
	.get_register_value = slimbus_drv_get_register_value,
	.set_register_value = slimbus_drv_set_register_value,
	.set_message_channel_lapse = slimbus_drv_set_message_channel_lapse,
	.get_message_channel_lapse = slimbus_drv_get_message_channel_lapse,
	.get_message_channel_usage = slimbus_drv_get_message_channel_usage,
	.get_message_channel_capacity = slimbus_drv_get_message_channel_capacity,
	.set_sniffer_mode = slimbus_drv_set_sniffer_mode,
	.get_sniffer_mode = slimbus_drv_get_sniffer_mode,
	.set_framer_enabled = slimbus_drv_set_framer_enabled,
	.get_framer_enabled = slimbus_drv_get_framer_enabled,
	.set_device_enabled = slimbus_drv_set_device_enabled,
	.get_device_enabled = slimbus_drv_get_device_enabled,
	.set_go_absent = slimbus_drv_set_go_absent,
	.get_go_absent = slimbus_drv_get_go_absent,
	.set_framer_config = slimbus_drv_set_framer_config,
	.get_framer_config = slimbus_drv_get_framer_config,
	.set_generic_device_config = slimbus_drv_set_generic_device_config,
	.get_generic_device_config = slimbus_drv_get_generic_device_config,
	.unfreeze = slimbus_drv_unfreeze,
	.cancel_configuration = slimbus_drv_cancel_configuration,
	.get_status_synchronization = slimbus_drv_get_status_synchronization,
	.get_status_detached = slimbus_drv_get_status_detached,
	.get_status_slimbus = slimbus_drv_get_status_slimbus,
	.get_data_port_status = slimbus_drv_get_data_port_status,
	.msg_assign_logical_address = slimbus_drv_msg_assign_logical_address,
	.msg_reset_device = slimbus_drv_msg_reset_device,
	.msg_change_logical_address = slimbus_drv_msg_change_logical_address,
	.msg_change_arbitration_priority = slimbus_drv_msg_change_arbitration_priority,
	.msg_request_self_announcement = slimbus_drv_msg_request_self_announcement,
	.msg_connect_source = slimbus_drv_msg_connect_source,
	.msg_connect_sink = slimbus_drv_msg_connect_sink,
	.msg_disconnect_port = slimbus_drv_msg_disconnect_port,
	.msg_change_content = slimbus_drv_msg_change_content,
	.msg_request_information = slimbus_drv_msg_request_information,
	.msg_request_clear_information = slimbus_drv_msg_request_clear_information,
	.msg_clear_information = slimbus_drv_msg_clear_information,
	.msg_begin_reconfiguration = slimbus_drv_msg_begin_reconfiguration,
	.msg_next_active_framer = slimbus_drv_msg_next_active_framer,
	.msg_next_subframe_mode = slimbus_drv_msg_next_subframe_mode,
	.msg_next_clock_gear = slimbus_drv_msg_next_clock_gear,
	.msg_next_root_frequency = slimbus_drv_msg_next_root_frequency,
	.msg_next_pause_clock = slimbus_drv_msg_next_pause_clock,
	.msg_next_reset_bus = slimbus_drv_msg_next_reset_bus,
	.msg_next_shutdown_bus = slimbus_drv_msg_next_shutdown_bus,
	.msg_next_define_channel = slimbus_drv_msg_next_define_channel,
	.msg_next_define_content = slimbus_drv_msg_next_define_content,
	.msg_next_activate_channel = slimbus_drv_msg_next_activate_channel,
	.msg_next_deactivate_channel = slimbus_drv_msg_next_deactivate_channel,
	.msg_next_remove_channel = slimbus_drv_msg_next_remove_channel,
	.msg_reconfigure_now = slimbus_drv_msg_reconfigure_now,
	.msg_request_value = slimbus_drv_msg_request_value,
	.msg_request_change_value = slimbus_drv_msg_request_change_value,
	.msg_change_value = slimbus_drv_msg_change_value,
};

slimbus_drv_obj *slimbus_drv_get_instance(void) {
	return &driver;
}


