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

#ifndef __SLIMBUS_DRV_API_H__
#define __SLIMBUS_DRV_API_H__

#include "slimbus_drv_stdtypes.h"
#include "slimbus_drv_errno.h"

/** @defgroup ConfigInfo  Configuration and Hardware Operation Information
 *	The following definitions specify the driver operation environment that
 *	is defined by hardware configuration or client code. These defines are
 *	located in the header file of the core driver.
 *	@{
 */

/**********************************************************************
* Defines
**********************************************************************/
/** Maximum message payload length */
#define	MESSAGE_PAYLOAD_MAX_LENGTH 28

/** Maximum message length */
#define	MESSAGE_MAX_LENGTH 39

/** Minimum message length */
#define	MESSAGE_MIN_LENGTH 6

/**
 *	@}
 */


/** @defgroup DataStructure Dynamic Data Structures
 *	This section defines the data structures used by the driver to provide
 *	hardware information, modification and dynamic operation of the driver.
 *	These data structures are defined in the header file of the core driver
 *	and utilized by the API.
 *	@{
 */

/**********************************************************************
 * Forward declarations
 **********************************************************************/
struct slimbus_information_element;
struct slimbus_channel;
struct slimbus_drv_message;
struct slimbus_framer_config;
struct slimbus_generic_device_config;
struct slimbus_information_elements;
struct slimbus_data_port_status;
struct slimbus_drv_config;
struct slimbus_drv_callbacks;
struct slimbus_message_callbacks;

/**********************************************************************
 * Enumerations
 **********************************************************************/
typedef enum
{
	/** No Arbitration */
	AT_NONE = 0,
	/** Long Arbitration */
	AT_LONG = 5,
	/** Short Arbitration */
	AT_SHORT = 15,
} slimbus_arbitration_type;

typedef enum
{
	/** Low Priority Messages */
	AP_LOW = 1u,
	/** Default Messages */
	AP_DEFAULT = 2u,
	/** High Priority Messages */
	AP_HIGH = 3u,
	/** Manager assigned only */
	AP_MANAGER_1 = 4u,
	/** Manager assigned only */
	AP_MANAGER_2 = 5u,
	/** Manager assigned only */
	AP_MANAGER_3 = 6u,
	/** Maximum Priority, for test and debug only */
	AP_MAXIMUM = 7u,
} slimbus_arbitration_priority;

typedef enum
{
	/** Core Message */
	MT_CORE = 0,
	/** Destination-referred Class-specific Message */
	MT_DESTINATION_REFERRED_CLASS_SPECIFIC_MESSAGE = 1,
	/** Destination-referred User Message */
	MT_DESTINATION_REFERRED_USER_MESSAGE = 2,
	/** Source-referred Class-specific Message */
	MT_SOURCE_REFERRED_CLASS_SPECIFIC_MESSAGE = 5,
	/** Source-referred User Message */
	MT_SOURCE_REFERRED_USER_MESSAGE = 6,
} slimbus_drv_message_type;

typedef enum
{
	/** Destination is a Logical Address */
	DT_LOGICAL_ADDRESS = 0u,
	/** Destination is an Enumeration Address */
	DT_ENUMERATION_ADDRESS = 1u,
	/** All Devices are Destinations, no Destination Address included in Header */
	DT_BROADCAST = 3u,
} slimbus_destination_type;

typedef enum
{
	/** Positive Acknowledge */
	MR_POSITIVE_ACK = 10,
	/** Negative Acknowledge */
	MR_NEGATIVE_ACK = 15,
	/** No Response */
	MR_NO_RESPONSE = 0,
} slimbus_message_response;

/** SLIMbus Transport Protocols. */
typedef enum
{
	/** Isochronous Protocol (Multicast). */
	TP_ISOCHRONOUS = 0u,
	/** Pushed Protocol (Multicast). */
	TP_PUSHED = 1u,
	/** Pulled Protocol (Unicast). */
	TP_PULLED = 2u,
	/** Locked Protocol (Multicast). */
	TP_LOCKED = 3u,
	/** Asynchronous Protocol - Simplex (Unicast). */
	TP_ASYNC_SIMPLEX = 4u,
	/** Asynchronous Protocol - Half-duplex (Unicast). */
	TP_ASYNC_HALF_DUPLEX = 5u,
	/** Extended Asynchronous Protocol - Simplex (Unicast). */
	TP_EXT_ASYNC_SIMPLEX = 6u,
	/** Extended Asynchronous Protocol - Half-duplex (Unicast). */
	TP_EXT_ASYNC_HALF_DUPLEX = 7u,
	/** User Defined 1. */
	TP_USER_DEFINED_1 = 14u,
	/** User Defined 2. */
	TP_USER_DEFINED_2 = 15u,
} slimbus_drv_transport_protocol;

/** Presence Rates. */
typedef enum
{
	DRV_PR_12K = 1u,
	DRV_PR_24K = 2u,
	DRV_PR_48K = 3u,
	DRV_PR_96K = 4u,
	DRV_PR_192K = 5u,
	DRV_PR_384K = 6u,
	DRV_PR_768K = 7u,
	DRV_PR_11025 = 9u,
	DRV_PR_22050 = 10u,
	DRV_PR_44100 = 11u,
	DRV_PR_88200 = 12u,
	DRV_PR_176400 = 13u,
	DRV_PR_352800 = 14u,
	DRV_PR_705600 = 15u,
	DRV_PR_4K = 16u,
	DRV_PR_8K = 17u,
	DRV_PR_16K = 18u,
	DRV_PR_32K = 19u,
	DRV_PR_64K = 20u,
	DRV_PR_128K = 21u,
	DRV_PR_256K = 22u,
	DRV_PR_512K = 23u,
} slimbus_drv_presence_rate;

/** Data types. */
typedef enum
{
	/** Not indicated. */
	DF_NOT_INDICATED = 0u,
	/** LPCM audio. */
	DF_LPCM = 1u,
	/** IEC61937 Compressed audio. */
	DF_IEC61937 = 2u,
	/** Packed PDM audio. */
	DF_PACKED_PDM_AUDIO = 3u,
	/** User Defined 1. */
	DF_USER_DEFINED_1 = 14u,
	/** User Defined 2. */
	DF_USER_DEFINED_2 = 15u,
} slimbus_drv_data_type;

/** Auxilary Field formats formats. */
typedef enum
{
	/** Not applicable. */
	AF_NOT_APPLICABLE = 0u,
	/** ZCUV for tunneling IEC60958. */
	AF_ZCUV = 1u,
	/** User defined. */
	AF_USER_DEFINED = 11u,
} slimbus_drv_aux_field_format;

/** SLIMbus interrupts. */
typedef enum
{
	/** Generation of any interrupts, regardless of setting of the other interrupt enable bits. */
	DRV_INT_EN = 1u,
	/** Generation of interrupt when RX_FIFO is not empty. */
	DRV_INT_RX = 2u,
	/** Generation of interrupt when TX_FIFO becomes empty (after transmitting all messages). */
	DRV_INT_TX = 4u,
	/** Generation of interrupt when any message from TX_FIFO is not transmitted successfully. */
	DRV_INT_TX_ERR = 8u,
	/** Generation of interrupt when any of main synchronization bits (F_SYNC, SF_SYNC, M_SYNC) toggles into LOW, and the IP is not detached due to REPORT_ABSENT message. */
	DRV_INT_SYNC_LOST = 16u,
	/** Generation of interrupt at the reconfiguration boundary. */
	DRV_INT_RCFG = 32u,
	/** Generation of interrupt when usage of message channel exceeded 75 percent in 'MCH_LAPSE+1' superframes in a row. */
	DRV_INT_MCH = 64u,
} slimbus_drv_interrupt;

/** Data Port interrupts. */
typedef enum
{
	/** Channel activation. */
	DP_INT_ACT = 1u,
	/** Channel content definition. */
	DP_INT_CON = 2u,
	/** Channel definition. */
	DP_INT_CHAN = 4u,
	/** Data Port DMA request. */
	DP_INT_DMA = 8u,
	/** Data Port FIFO overflow. */
	DP_INT_OVF = 16u,
	/** Data Port FIFO underrun. */
	DP_INT_UND = 32u,
} slimbus_data_port_interrupt;

/** Quality of the CLK signal that is generated by the Framer. */
typedef enum
{
	/** Potentially irregular punctured clock unsuitable for use as a timing reference for generic phaselocked loops. */
	FQ_PUNCTURED = 0,
	/** Irregular clock that has a lower-frequency regular clock embedded. For example, such clocks can be created by cyclic pulse swallowing. */
	FQ_IRREGULAR = 1,
	/** Regular clock that potentially has more than 1 ns RMS of wideband jitter (100 Hz measurement corner, see [AES01]). */
	FQ_REGULAR = 2,
	/** Low-jitter clock that is known to have less than 1 ns RMS of wideband jitter (100 Hz measurement corner, see [AES01]). */
	FQ_LOW_JITTER = 3,
} slimbus_framer_quality;

/** User Value / Information Elements Slice sizes. */
typedef enum
{
	/** Slice size is 1 byte. */
	SS_1_BYTE = 0,
	/** Slice size is 2 bytes. */
	SS_2_BYTES = 1,
	/** Slice size is 3 bytes. */
	SS_3_BYTES = 2,
	/** Slice size is 4 bytes. */
	SS_4_BYTES = 3,
	/** Slice size is 6 bytes. */
	SS_6_BYTES = 4,
	/** Slice size is 8 bytes. */
	SS_8_BYTES = 5,
	/** Slice size is 12 bytes. */
	SS_12_BYTES = 6,
	/** Slice size is 16 bytes. */
	SS_16_BYTES = 7,
	/** Slice size max */
	SS_MAX	= 8,
} slimbus_drv_slice_size;

/** SLIMbus device classes */
typedef enum
{
	/** Manager Class Device */
	DC_MANAGER = 255,
	/** Framer Class Device */
	DC_FRAMER = 254,
	/** Interface Class Device */
	DC_INTERFACE = 253,
	/** Generic Class Device */
	DC_GENERIC = 0,
} slimbus_device_class;

typedef enum
{
	/** Clock Gear 6 */
	RC_CLOCK_GEAR_6 = 0,
	/** Clock Gear 7 */
	RC_CLOCK_GEAR_7 = 1,
	/** Clock Gear 8 */
	RC_CLOCK_GEAR_8 = 2,
	/** Clock Gear 9 */
	RC_CLOCK_GEAR_9 = 3,
} slimbus_reference_clock;

/** Subframe Mode Codings. CSW - Control Space Width (Slots), SL - Subframe Length (Slots) */
typedef enum
{
	SM_24_CSW_32_SL = 31u,
	SM_16_CSW_32_SL = 29u,
	SM_16_CSW_24_SL = 28u,
	SM_12_CSW_32_SL = 27u,
	SM_12_CSW_24_SL = 26u,
	SM_8_CSW_32_SL = 25u,
	SM_8_CSW_24_SL = 24u,
	SM_6_CSW_32_SL = 23u,
	SM_6_CSW_24_SL = 22u,
	SM_6_CSW_8_SL = 21u,
	SM_4_CSW_32_SL = 19u,
	SM_4_CSW_24_SL = 18u,
	SM_4_CSW_8_SL = 17u,
	SM_4_CSW_6_SL = 16u,
	SM_3_CSW_32_SL = 15u,
	SM_3_CSW_24_SL = 14u,
	SM_3_CSW_8_SL = 13u,
	SM_3_CSW_6_SL = 12u,
	SM_2_CSW_32_SL = 11u,
	SM_2_CSW_24_SL = 10u,
	SM_2_CSW_8_SL = 9u,
	SM_2_CSW_6_SL = 8u,
	SM_1_CSW_32_SL = 7u,
	SM_1_CSW_24_SL = 6u,
	SM_1_CSW_8_SL = 5u,
	SM_1_CSW_6_SL = 4u,
	/** 100% Control Space, 0% Data Space */
	SM_8_CSW_8_SL = 0u,
} slimbus_drv_subframe_mode;

/** SLIMbus Frequencies and Clock Gear Codings. */
typedef enum
{
	/** Not Indicated, Min: 0 MHz, Max: 28.8 MHz */
	CG_0 = 0u,
	/** Min: 0.025 MHz, Max: 0.05625 MHz */
	CG_1 = 1u,
	/** Min: 0.05 MHz, Max: 0.1125 MHz */
	CG_2 = 2u,
	/** Min: 0.1 MHz, Max: 0.225 MHz */
	CG_3 = 3u,
	/** Min: 0.2 MHz, Max: 0.45 MHz */
	CG_4 = 4u,
	/** Min: 0.4 MHz, Max: 0.9 MHz */
	CG_5 = 5u,
	/** Min: 0.8 MHz, Max: 1.8 MHz */
	CG_6 = 6u,
	/** Min: 1.6 MHz, Max: 3.6 MHz */
	CG_7 = 7u,
	/** Min: 3.2 MHz, Max: 7.2 MHz */
	CG_8 = 8u,
	/** Min: 6.4 MHz, Max: 14.4 MHz */
	CG_9 = 9u,
	/** Min: 12.8 MHz, Max: 28.8 MHz */
	CG_10 = 10u,
} slimbus_drv_clock_gear;

/** SLIMbus Root Frequency (RF) and Phase Modulus (PM) Codings. */
typedef enum
{
	/** RF: Not Indicated, PM: 160 */
	RF_0 = 0u,
	/** RF: 24.576 MHz, PM: 160 */
	RF_1 = 1u,
	/** RF: 22.5792 MHz, PM: 147 */
	RF_2 = 2u,
	/** RF: 15.36 MHz, PM: 100 */
	RF_3 = 3u,
	/** RF: 16.8 MHz, PM: 875 */
	RF_4 = 4u,
	/** RF: 19.2 MHz, PM: 125 */
	RF_5 = 5u,
	/** RF: 24 MHz, PM: 625 */
	RF_6 = 6u,
	/** RF: 25 MHz, PM: 15625 */
	RF_7 = 7u,
	/** RF: 26 MHz, PM: 8125 */
	RF_8 = 8u,
	/** RF: 27 MHz, PM: 5625 */
	RF_9 = 9u,
} slimbus_drv_root_frequency;

/** Restart Time Values. */
typedef enum
{
	/** After a restart request, the active Framer shall resume toggling the CLK line within four cycles of the CLK line frequency (as indicated by the Clock Gear and Root Frequency) used for the upcoming Frame. Optional. */
	RT_FAST_RECOVERY = 0u,
	/** After the restart request, the active Framer shall resume toggling the CLK line so the duration of the Pause is an integer number of Superframes in the upcoming Clock Gear. Optional. */
	RT_CONSTANT_PHASE_RECOVERY = 1u,
	/** After a restart request, the active Framer shall resume toggling the CLK line after an unspecified delay. Mandatory */
	RT_UNSPECIFIED_DELAY = 2u,
} slimbus_drv_restart_time;

/** Information Element category. */
typedef enum
{
	IEC_CORE = 0,
	IEC_CLASS = 1,
	IEC_USER = 2,
} slimbus_information_element_category;

/** Information Element access type. */
typedef enum
{
	IEAT_ELEMENTAL = 0,
	IEAT_BYTE = 1,
} slimbus_information_element_access_type;

/**********************************************************************
 * Callbacks
 **********************************************************************/
/** Requests Logical Address for SLIMbus Device. Called function should return proper [0x00 - 0xEF] and unique Logical Address. */
typedef uint8_t (*slimbus_assign_logical_address_handler)(uint64_t enumeration_address, slimbus_device_class class);

/** Requests Device Class for assigned Logical Address. Called function should return Device Class. Mandatory. */
typedef slimbus_device_class (*slimbus_drv_device_class_handler)(uint8_t logical_address);

/** Callback for manager interrupts */
typedef void (*slimbus_manager_interrupts_handler)(void* pd, slimbus_drv_interrupt interrupt);

/** Callback for data ports interrupts */
typedef void (*slimbus_data_port_interrupts_handler)(void* pd, uint8_t data_port_number, slimbus_data_port_interrupt data_port_interrupt);

/** Callback for receiving SLIMbus Raw Message (before decoding). */
typedef void (*slimbus_received_raw_message_handler)(void* pd, void* message, uint8_t message_length);

/** Callback for receiving SLIMbus Message (after decoding). */
typedef void (*slimbus_received_message_handler)(void* pd, struct slimbus_drv_message* message);

/** Callback for sending SLIMbus Raw Message (after encoding). */
typedef void (*slimbus_sending_raw_message_handler)(void* pd, void* message, uint8_t message_length);

/** Callback for sending SLIMbus Message (before encoding). */
typedef void (*slimbus_sending_message_handler)(void* pd, struct slimbus_drv_message* message);

/** Callback for SLIMbus Reports of Information Elements. */
typedef void (*slimbus_information_elements_handler)(void* pd, uint8_t source_la, struct slimbus_information_elements* information_elements);

/** Callback for SLIMbus REPORT_PRESENT Message, which is sent by an unenumerated Device to announce its presence on the bus. */
typedef void (*slimbus_msg_report_present_handler)(void* pd, uint64_t source_ea, slimbus_device_class device_class, uint8_t device_class_version);

/** Callback for SLIMbus REPORT_ABSENT Message, which shall be sent from a Device to announce that it is about to leave the bus. */
typedef void (*slimbus_msg_report_absent_handler)(void* pd, uint8_t source_la);

/** Callback for SLIMbus REPLY_INFORMATION Message, which is sent by a Device in response to a REQUEST_INFORMATION or REQUEST_CLEAR_INFORMATION Message. */
typedef void (*slimbus_msg_reply_information_handler)(void* pd, uint8_t source_la, uint8_t transaction_id, uint8_t* information_slice, uint8_t information_slice_length);

/** Callback for SLIMbus REPORT_INFORMATION Message, which used by a Device to inform another Device about a change in an Information Slice. */
typedef void (*slimbus_msg_report_information_handler)(void* pd, uint8_t source_la, uint16_t element_code, uint8_t* information_slice, uint8_t information_slice_length);

/** Callback for SLIMbus REPLY_VALUE Message, which is sent by a Device in response to a REQUEST_VALUE or REQUEST_CHANGE_VALUE Message. */
typedef void (*slimbus_msg_reply_value_handler)(void* pd, uint8_t source_la, uint8_t transaction_id, uint8_t* value_slice, uint8_t value_slice_length);

/**********************************************************************
 * Structures and unions
 **********************************************************************/
/** Structure describing Information Element. */
typedef struct slimbus_information_element
{
	/** Byte address. */
	uint16_t address;
	/** Slice size to be accessed. Valid for elemental access type. */
	slimbus_drv_slice_size ss;
	/** Bit number pointing to Information Element's lowest bit. Valid for Byte access type. */
	uint8_t bn;
	/** Category of Information Element. It can be Core, Class-specific and User defined. */
	slimbus_information_element_category category;
	/** Buffer for storing Information Element Slice. */
	uint8_t slice[16];
}  slimbus_information_element;

/** SLIMbus Data Channel and its content structure definition. */
typedef struct slimbus_channel
{
	/** Channel number. This number is used as a channel identifier. */
	uint8_t cn;
	/** Transport Protocol. Cadence's SLIMbus Manager Controller IP currently supports only Isochronous, Pushed and Pulled Protocols. */
	slimbus_drv_transport_protocol tp;
	/** Length of a Segment (in Slots). Valid value ranges from 1 to 31. */
	uint8_t sl;
	/** Frequency Locked bit indicating whether the content flow is known to be frequency locked to the SLIMbus CLK signal. */
	bool fl;
	/** Specifies presence rate. */
	slimbus_drv_presence_rate pr;
	/** Specifies Auxiliary Field format. */
	slimbus_drv_aux_field_format af;
	/** Data type. */
	slimbus_drv_data_type dt;
	/** Channel link bit. When true, indicates that the content of channel 'cn' is related to that of channel 'ch-1'. */
	bool cl;
	/** Length of the Data field (in Slots). Valid value ranges from 1 to 31 */
	uint8_t dl;
}  slimbus_channel;

/** SLIMbus Message structure definition. */
typedef struct slimbus_drv_message
{
	/** Arbitration Type */
	slimbus_arbitration_type arbitration_type;
	/** Source Address, if Arbitration Type == AT_SHORT used as Logic Address, if Arbitration Type == AT_Long used as Enumeration Address. */
	uint64_t source_address;
	/** Arbitration Priority */
	slimbus_arbitration_priority arbitration_priority;
	/** Message Type */
	slimbus_drv_message_type message_type;
	/** Message Codes are specific to a Message Type. */
	uint8_t message_code;
	/** Destination Type */
	slimbus_destination_type destination_type;
	/** Destination Address, if Destination Type == DT_LOGICAL_ADDRESS used as Logic Address, if Destination Type == DT_ENUMERATION_ADDRESS used as Enumeration Address, ignored if Destination Type == DT_BROADCAST. */
	uint64_t destination_address;
	/** Message Payload */
	uint8_t payload[28];
	/** Message Payload Length */
	uint8_t payload_length;
	/** Message Response */
	slimbus_message_response response;
}  slimbus_drv_message;

/** Structure describing configuration of the Framer device implemented in SLIMbus Manager. */
typedef struct slimbus_framer_config
{
	/** Each bit of this vector corresponds to one of 16 Root Frequencies specified by the SLIMbus standard. When the bit is 1 then the corresponding Root Frequency is supported by the Framer device implemented in SLIMBUS_MANAGER. When bit is 0 then the corresponding Root Frequency is not supported, and the attempt of reconfiguration SLIMBUS_MANAGER using that Root Frequency will result in RECONFIG_OBJECTION. */
	uint16_t root_frequencies_supported;
	/** Used to set quality of the CLK signal that is generated by the FRAMER device implemented in SLIMBUS_MANAGER. */
	slimbus_framer_quality quality;
	/** When 1 and the FRAMER device change the Root Frequency, after reconfiguration boundary the Framer stops the SLIMbus clock leaving the SLIMbus clock line at 0 state. To start toggle the clock line, the UNFREEZE bit in the command register must be set. When 0 the FRAMER device change the Root Frequency without stopping the clock line. */
	bool pause_at_root_frequency_change;
}  slimbus_framer_config;

/** Structure describing configuration of the Generic device and its Data Ports implemented in SLIMbus Manager. */
typedef struct slimbus_generic_device_config
{
	/** Presence Rates supported by Generic device. Each bit of this vector corresponds to one of 24 not reserved values of the Presence Rate specified by the SLIMbus standard, range b0000000...b0010111 is considered. When the bit is 1 then the corresponding Presence Rate is supported by the Generic device implemented in SLIMBUS_MANAGER. When bit is 0 then the corresponding Presence Rate is not supported, and the attempt of connecting SLIMBUS_MANAGER data port to channel with corresponding Presence Rate will result in EX_ERROR. */
	uint32_t presence_rates_supported;
	/** When 1 then the Isochronous Transport protocol is supported by the Generic device implemented in SLIMBUS_MANAGER. When the bit is 0 then the Isochronous Transport Protocol is not supported, and the attempt of connecting SLIMBUS_MANAGER data port to channel with Isochronous Transport protocol will result in EX_ERROR. */
	bool transport_protocol_isochronous;
	/** When 1 then the Pushed Transport protocol is supported by the Generic device implemented in SLIMBUS_MANAGER. When the bit is 0 then the Pushed Transport Protocol is not supported, and the attempt of connecting SLIMBUS_MANAGER data port to channel with Pushed Transport protocol will result in EX_ERROR. */
	bool transport_protocol_pushed;
	/** When 1 then the Pulled Transport protocol is supported by the Generic device implemented in SLIMBUS_MANAGER. When the bit is 0 then the Pulled Transport Protocol is not supported, and the attempt of connecting SLIMBUS_MANAGER data port to channel with Pulled Transport protocol will result in EX_ERROR. */
	bool transport_protocol_pulled;
	/** Level of FIFO at which sink port starts releasing data. Must be lower than DATA_FIFO_SIZE. Recommended value, for DATA_FIFO_SIZE = 4 is 2. */
	uint8_t sink_start_level;
	/** This 4-bit number defining prescaler used for Data Port clock (dport_clock) generation. For each data port, SLIMBUS_MANAGER generates data port clock with frequency related with data channel segment rate. And the ratio between segment rate and generated data port clock is defined as 2*PORT_CLK_PRESC. Accepted values: 2 to 8. For values 0 and 1, functionality is the same as for value 2. For values 9 and above, functionality is not specified. */
	uint8_t data_port_clock_prescaler;
	/** Used for generation of cport_clk_o. Frequency of cport_clk_o = sb_clk_i_n frequency divided by {CPORT_CLK_DIV+1}. */
	uint8_t cport_clock_divider;
	/** Defines the reference clock generated out of the SLIMbus clock. */
	slimbus_reference_clock reference_clock_selector;
	/** When level of Source Data Port FIFO falls below that threshold then DMA request is generated for Source data ports, and if enabled, DMA request interrupt is generated. */
	uint16_t dma_treshold_source;
	/** When level of Sink Data Port FIFO exceeds that threshold then DMA request is generated for Sink data ports, and if enabled, DMA request interrupt is generated. */
	uint16_t dma_treshold_sink;
}  slimbus_generic_device_config;

/** State of Information Elements */
typedef struct slimbus_information_elements
{
	/** Core IE - There has been a Message execution error */
	bool core_ex_error;
	/** Core IE - The Device objects to the bus reconfiguration */
	bool core_reconfig_objection;
	/** Core IE - A Collision was detected in a Data Channel */
	bool core_data_tx_col;
	/** Core IE - An unsupported Message has been received */
	bool core_unsprtd_msg;
	/** Interface Device Class Specific IE - Internal Port contention */
	bool interface_data_slot_overlap;
	/** Interface Device Class Specific IE - Lost Message synchronization */
	bool interface_lost_ms;
	/** Interface Device Class Specific IE - Lost Superframe synchronization */
	bool interface_lost_sfs;
	/** Interface Device Class Specific IE - Lost Frame synchronization */
	bool interface_lost_fs;
	/** Interface Device Class Specific IE - Message Channel Collision detected */
	bool interface_mc_tx_col;
	/** Manager Device Class Specific IE - The Device is the active Manager */
	bool manager_active_manager;
	/** Framer Device Class Specific IE - Quality of the generated clock (Optional) */
	slimbus_framer_quality framer_quality;
	/** Framer Device Class Specific IE - Guide Channel Collision detected */
	bool framer_gc_tx_col;
	/** Framer Device Class Specific IE - Framing Information Collision detected */
	bool framer_fi_tx_col;
	/** Framer Device Class Specific IE - Frame Sync Symbol Collision detected */
	bool framer_fs_tx_col;
	/** Framer Device Class Specific IE - The Device is the active Framer */
	bool framer_active_framer;
}  slimbus_information_elements;

/** Structure describing status of Generic Data Port implemented in the Manager component. */
typedef struct slimbus_data_port_status
{
	/** 1 if Data Port is activated. Data Port is regarded to be activated if it is connected, configured and activated. Otherwise it is regarded as deactivated. */
	bool active;
	/** 1 if content the channel connected with Data Port is defined (consequence of NEXT_DEFINE_CONTENT or CHANGE_CONTENT message). */
	bool content_defined;
	/** 1 if the channel connected with Data Port is defined (consequence of NEXT_DEFINE_CHANNEL message). */
	bool channel_defined;
	/** 1 if Data Port is connected as Sink. 0 if Data Port is disconnected or connected as Source. */
	bool sink;
	/** 1 if overflow occurred for Data Port. */
	bool overflow;
	/** 1 if underrun occurred for Data Port. */
	bool underrun;
	/** Implemented only when access to the Data Port is selected through AHB. In such case it notifies about one of the following conditions: - Corresponding Data Port is configured as SOURCE and its internal FIFO is not full (ready for next data). - Corresponding Data Port FIFO is not empty. When access to the Data Port is selected through Direct interface then this bit is fixed to 0. */
	bool dport_ready;
	/** Segment Interval decoded from Segment Distribution defined for the connected channel. Part of Channel Definition. */
	uint16_t segment_interval;
	/** Transport Protocol defined for the connected channel. Part of Channel Definition. */
	slimbus_drv_transport_protocol transport_protocol;
	/** Presence Rate defined for the connected channel. Part of Content Definition. */
	slimbus_drv_presence_rate presence_rate;
	/** FR bit defined for the connected channel. Part of Content Definition. */
	bool frequency_lock;
	/** Data Type defined for the connected channel. Part of Content Definition. */
	slimbus_drv_data_type data_type;
	/** Data Length defined for the connected channel. Part of Content Definition. */
	uint8_t data_length;
	/** This variable contains number of port to which this Data Port is linked to. Valid of corresponding CH_LINK is HIGH. Value 63 may indicate that the linked port has not been found despite of CH_LINK=1. Part of Content Definition. */
	uint8_t port_linked;
	/** CL bit defined for the connected channel. Part of Content Definition. */
	bool channel_link;
}  slimbus_data_port_status;

/** Configuration parameters passed to probe & init functions. */
typedef struct slimbus_drv_config
{
	/** Base address of the register space. */
	uintptr_t reg_base;
	/** Base address of the DMA descriptors space. */
	uintptr_t dma_base;
	/** If 1, the Sniffer functionality is enabled. In this mode all messages tracked in the message channel are written to RX_FIFO, regardless of response status. */
	bool sniffer_mode;
	/** If 1, the internal Framer device is enabled. If set 0, the internal Framer device is disabled. */
	bool enable_framer;
	/** If 1, the internal Generic device is enabled. If set 0, the internal Generic device is disabled. */
	bool enable_device;
	/** Maximum number of retransmissions in case of collision or not positive acknowledgement. */
	uint8_t retry_limit;
	/** When 1, REPORT_INFORMATION message is transmitted independent on state of Information Element before the event occurrence (i.e. Information Element does not have to be cleared to enable next generation of corresponding REPORT_INFORMATION). */
	bool report_at_event;
	/** If 1, CRC fields in the messages transmitted from TX_FIFO hardware calculation of CRC is disabled (CRC will be calculated by the CPU). If 0, CRC fields are calculated automatically in the IP. */
	bool disable_hardware_crc_calculation;
	/** If 1, reporting using REPORT_INFORMATION is limited only to mandatory reporting. I.e. only REPORT_INFORMATION messages related with Information Elements specific for the Interface Device Class are supported (MC_TX_COL, LOST_FS, LOST_SFS, LOST_MS, DATA_SLOT_OVERLAP) */
	bool limit_reports;
	/** Product ID. Part of SLIMbus Enumeration Address. */
	uint16_t ea_product_id;
	/** Instance Value. Part of SLIMbus Enumeration Address. */
	uint8_t ea_instance_value;
	/** Device ID of the Interface device - part of Enumeration Address. */
	uint8_t ea_interface_id;
	/** Device ID of the Generic device - part of Enumeration Address. */
	uint8_t ea_generic_id;
	/** Device ID of the Framer device - part of Enumeration Address. */
	uint8_t ea_framer_id;
	/** If enabled, the driver will enumerate devices using AssignLogicalAddressHandler. If disabled user will have to perform enumeration of devices using Messages. */
	bool enumerate_devices;
}  slimbus_drv_config;

/** Structure containing pointers to functions defined by user that will be called when specific event occurs. */
typedef struct slimbus_drv_callbacks
{
	/** Request for Logical Address. Required if enumerate_devices in slimbus_drv_config is enabled. */
	slimbus_assign_logical_address_handler on_assign_logical_address;
	/** Request for Device Class for Logical Address. Mandatory. */
	slimbus_drv_device_class_handler on_device_class_request;
	/** Callback for SLIMbus Reports of Information Elements. Optional, unused if NULL was assigned. */
	slimbus_information_elements_handler on_information_element_reported;
	/** Callback for Manager Interrupts. Optional, unused if NULL was assigned. */
	slimbus_manager_interrupts_handler on_manager_interrupt;
	/** Callback for Data Ports Interrupts. Optional, unused if NULL was assigned. */
	slimbus_data_port_interrupts_handler on_data_port_interrupt;
	/** Callback for Receiving SLIMbus Raw Message (before decoding). Optional, unused if NULL was assigned. */
	slimbus_received_raw_message_handler on_raw_message_received;
	/** Callback for receiving SLIMbus Message (after decoding). Optional, unused if NULL was assigned. */
	slimbus_received_message_handler on_message_received;
	/** Callback for sending SLIMbus Raw Message (after encoding). Optional, unused if NULL was assigned. */
	slimbus_sending_raw_message_handler on_raw_message_sending;
	/** Callback for sending SLIMbus Message (before encoding). Optional, unused if NULL was assigned. */
	slimbus_sending_message_handler on_message_sending;
}  slimbus_drv_callbacks;

/** Structure containing pointers to functions defined by user that will be called when specific SLIMbus message has been received. */
typedef struct slimbus_message_callbacks
{
	/** Callback for REPORT_PRESENT Message. Unused if NULL was assigned. */
	slimbus_msg_report_present_handler on_msg_report_present;
	/** Callback for REPORT_ABSENT Message. Unused if NULL was assigned. */
	slimbus_msg_report_absent_handler on_msg_report_absent;
	/** Callback for REPLY_INFORMATION Message. Unused if NULL was assigned. */
	slimbus_msg_reply_information_handler on_msg_reply_information;
	/** Callback for REPORT_INFORMATION Message. Unused if NULL was assigned. */
	slimbus_msg_report_information_handler on_msg_report_information;
	/** Callback for REPLY_VALUE Message. Unused if NULL was assigned. */
	slimbus_msg_reply_value_handler on_msg_reply_value;
}  slimbus_message_callbacks;

/**
 *	@}
 */

/** @defgroup DriverObject Driver API Object
 *	API listing for the driver. The API is contained in the object as
 *	function pointers in the object structure. As the actual functions
 *	resides in the Driver Object, the client software must first use the
 *	global GetInstance function to obtain the Driver Object Pointer.
 *	The actual APIs then can be invoked using obj->(api_name)() syntax.
 *	These functions are defined in the header file of the core driver
 *	and utilized by the API.
 *	@{
 */

/**********************************************************************
 * API methods
 **********************************************************************/
typedef struct slimbus_drv_obj
{
	/**
	 * Returns the memory requirements for a driver instance.
	 * @param[out] required_memory Required memory in bytes.
	 * @param[in] config Driver and hardware configuration.
	 * @return EOK On success (required_memory variable filled).
	 * @return EINVAL If config contains invalid values or not supported configuration.
	 */
	uint32_t (*probe)(const slimbus_drv_config* config, uint16_t* required_memory);

	/**
	 * Instantiates the slimbus Core Driver, given the required blocks of
	 * memory (this includes initializing the instance and the underlying
	 * hardware). If a client configuration is required (likely to always
	 * be true), it is passed in also. Returns an instance pointer, which
	 * the client must maintain and pass to all other driver functions.
	 * (except probe).
	 * @param[in] callbacks Event Handlers and Callbacks.
	 * @param[in] config Specifies driver/hardware configuration.
	 * @param[in] pd Pointer to driver's private data object.
	 * @return EOK On success
	 * @return EINPROGRESS If SLIMbus Manager is already running.
	 * @return EINVAL If illegal/inconsistent values in 'config' doesn't support feature(s) required by 'config' parameters.
	 */
	uint32_t (*init)(void* pd, const slimbus_drv_config* config, slimbus_drv_callbacks* callbacks);

	/**
	 * slimbus Core Driver's ISR. Platform-specific code is responsible for
	 * ensuring this gets called when the corresponding hardware's
	 * interrupt is asserted. Registering the ISR should be done after
	 * calling init, and before calling start. The driver's ISR will not
	 * attempt to lock any locks, but will perform client callbacks. If
	 * the client wishes to defer processing to non-interrupt time, it is
	 * responsible for doing so. This function must not be called after
	 * calling destroy and releasing private data memory.
	 * @param[in] pd Pointer to driver's private data object filled by init.
	 * @return 0 on success
	 * @return EINVAL if input parameters are invalid
	 */
	uint32_t (*isr)(void* pd);

	/**
	 * Start the slimbus driver, enabling interrupts. This is called after
	 * the client has successfully initialized the driver and hooked the
	 * driver's ISR (the isr member of this struct) to the IRQ. Driver
	 * will also enumerate all discovered devices.
	 * @param[in] pd Pointer to driver's private data object.
	 * @return 0 on success
	 * @return EINPROGRESS If SLIMbus Manager is already running.
	 * @return EIO if operation failed
	 * @return EINVAL if input parameters are invalid
	 */
	uint32_t (*start)(void* pd);

	/**
	 * The client may call this to disable the hardware (disabling its
	 * IRQ at the source and disconnecting it if applicable). Also, a
	 * best-effort is made to cancel any pending transactions.
	 * @param[in] pd Pointer to driver's private data object.
	 * @return 0 on success
	 * @return EIO if operation failed
	 * @return EINVAL if input parameters are invalid
	 */
	uint32_t (*stop)(void* pd);

	/**
	 * This performs an automatic stop and then de-initializes the
	 * driver. The client may not make further requests on this instance.
	 * @param[in] pd Pointer to driver's private data object.
	 * @return 0 on success
	 * @return EIO if operation failed
	 * @return EINVAL if input parameters are invalid
	 */
	uint32_t (*destroy)(void* pd);

	/**
	 * Sets (enable or disable) SLIMbus Manager interrupt mask.
	 * @param[in] interrupt_mask SLIMbus Manager interrupt mask.
	 * @param[in] pd Pointer to driver's private data object.
	 * @return EOK On success.
	 * @return EINVAL if input parameters are invalid.
	 */
	uint32_t (*set_interrupts)(void* pd, uint8_t interrupt_mask);

	/**
	 * Obtains information about SLIMbus Manager enabled interrupts.
	 * @param[out] interrupt_mask Pointer to place where SLIMbus Manager interrupt mask will be written.
	 * @param[in] pd Pointer to driver's private data object.
	 * @return EOK On success.
	 * @return EINVAL If pd or interrupt_mask is NULL.
	 */
	uint32_t (*get_interrupts)(void* pd, uint8_t* interrupt_mask);

	/**
	 * Sets (enable or disable) Data Port interrupt mask.
	 * @param[in] interrupt_mask Data Port interrupt mask.
	 * @param[in] port_number Data Port number. Valid value ranges from 0 to 63.
	 * @param[in] pd Pointer to driver's private data object.
	 * @return EOK On success.
	 * @return EINVAL If pd is NULL or port_number is invalid.
	 */
	uint32_t (*set_data_port_interrupts)(void* pd, uint8_t port_number, uint8_t interrupt_mask);

	/**
	 * Obtains information about Data Port enabled interrupts.
	 * @param[out] interrupt_mask Pointer to place where Data Port interrupt mask will be written.
	 * @param[in] port_number Data Port number. Valid value ranges from 0 to 63.
	 * @param[in] pd Pointer to driver's private data object.
	 * @return EOK On success.
	 * @return EINVAL If port_number is invalid.
	 */
	uint32_t (*get_data_port_interrupts)(void* pd, uint8_t port_number, uint8_t* interrupt_mask);

	/**
	 * Clears Data Port FIFO. Used when Data Port FIFO is accessed
	 * through AHB.
	 * @param[in] port_number Data Port number. Valid value ranges from 0 to 63.
	 * @param[in] pd Pointer to driver's private data object.
	 * @return EOK On success.
	 * @return EINVAL If pd is NULL or port_number is invalid.
	 */
	uint32_t (*clear_data_port_fifo)(void* pd, uint8_t port_number);

	/**
	 * Sets (enable or disable) Presence Rate Generation.
	 * @param[in] enable If true Presence Rate Generation for specified Data Port will be enabled; if not then it will be disabled.
	 * @param[in] port_number Data Port number. Valid value ranges from 0 to 63.
	 * @param[in] pd Pointer to driver's private data object.
	 * @return EOK On success.
	 * @return EINVAL If pd is NULL or port_number is invalid.
	 */
	uint32_t (*set_presence_rate_generation)(void* pd, uint8_t port_number, bool enable);

	/**
	 * Obtains information about Presence Rate Generation.
	 * @param[out] enable Pointer to place where information about Presence Rate Generation will be written.
	 * @param[in] port_number Data Port number. Valid value ranges from 0 to 63.
	 * @param[in] pd Pointer to driver's private data object.
	 * @return EOK On success.
	 * @return EINVAL If pd or enabled is NULL or port_number is invalid.
	 */
	uint32_t (*get_presence_rate_generation)(void* pd, uint8_t port_number, bool* enable);

	/**
	 * Assigns callbacks for receiving SLIMbus messages
	 * @param[in] msg_callbacks SLIMbus Messages Callbacks.
	 * @param[in] pd Pointer to driver's private data object.
	 * @return 0 on success
	 * @return EIO if operation failed
	 * @return EINVAL if input parameters are invalid
	 */
	uint32_t (*assign_message_callbacks)(void* pd, slimbus_message_callbacks* msg_callbacks);

	/**
	 * Send Raw SLIMbus Message (bytes)
	 * @param[in] message_length Raw message length.
	 * @param[in] message Pointer to raw message.
	 * @param[in] pd Pointer to driver's private data object.
	 * @return 0 on success
	 * @return EIO if operation failed
	 * @return EINVAL if input parameters are invalid
	 */
	uint32_t (*send_raw_message)(void* pd, void* message, uint8_t message_length);

	/**
	 * Send SLIMbus message.
	 * @param[in] message Pointer to struct message.
	 * @param[in] pd Pointer to driver's private data object.
	 * @return 0 on success
	 * @return EIO if operation failed
	 * @return EINVAL if input parameters are invalid
	 */
	uint32_t (*send_message)(void* pd, slimbus_drv_message* message);

	/**
	 * Read contents of SLIMbus Manager's register. Register Address must
	 * be aligned to 32-bits.
	 * @param[out] reg_content Register data output.
	 * @param[in] reg_address Register address.
	 * @param[in] pd Pointer to driver's private data object.
	 * @return 0 on success
	 * @return EINVAL if input parameters are invalid
	 */
	uint32_t (*get_register_value)(void* pd, uint16_t reg_address, uint32_t* reg_content);

	/**
	 * Write contents to SLIMbus Manager's register. Register Address
	 * must be aligned to 32-bits.
	 * @param[in] reg_content Register data input.
	 * @param[in] reg_address Register address.
	 * @param[in] pd Pointer to driver's private data object.
	 * @return 0 on success
	 * @return EINVAL if input parameters are invalid
	 */
	uint32_t (*set_register_value)(void* pd, uint16_t reg_address, uint32_t reg_content);

	/**
	 * Sets message channel Lapse. 4-bit variable defining number of
	 * superframes for which messages channel usage is being monitored to
	 * not exceed 75% bandwidth. If message channel usage exceeds 75% in
	 * MCH_LAPSE+1 superframes in a row, then MCH_INT can be generated,
	 * if enabled.
	 * @param[in] mch_lapse New Message Channel Lapse value
	 * @param[in] pd Pointer to driver's private data object.
	 * @return 0 on success
	 * @return EINVAL if input parameters are invalid
	 */
	uint32_t (*set_message_channel_lapse)(void* pd, uint8_t mch_lapse);

	/**
	 * Returns current value of message channel Lapse.
	 * @param[out] mch_lapse Current Message Channel Lapse value
	 * @param[in] pd Pointer to driver's private data object.
	 * @return 0 on success
	 * @return EINVAL if input parameters are invalid
	 */
	uint32_t (*get_message_channel_lapse)(void* pd, uint8_t* mch_lapse);

	/**
	 * Returns current value of message channel usage.
	 * @param[out] mch_usage Current Message Channel Usage
	 * @param[in] pd Pointer to driver's private data object.
	 * @return 0 on success
	 * @return EINVAL if input parameters are invalid
	 */
	uint32_t (*get_message_channel_usage)(void* pd, uint16_t* mch_usage);

	/**
	 * Returns current value of message channel capacity.
	 * @param[out] mch_capacity Current Message Channel Capacity
	 * @param[in] pd Pointer to driver's private data object.
	 * @return 0 on success
	 * @return EINVAL if input parameters are invalid
	 */
	uint32_t (*get_message_channel_capacity)(void* pd, uint16_t* mch_capacity);

	/**
	 * Enable or Disable Sniffer Mode. If 1, the Sniffer functionality is
	 * enabled. In this mode all messages tracked in the message channel
	 * are written to RX_FIFO, regardless of response status.
	 * @param[in] state New state
	 * @param[in] pd Pointer to driver's private data object.
	 * @return 0 on success
	 * @return EIO if operation failed
	 * @return EINVAL if input parameters are invalid
	 */
	uint32_t (*set_sniffer_mode)(void* pd, bool state);

	/**
	 * Returns state of Sniffer Mode feature.
	 * @param[out] state Current state
	 * @param[in] pd Pointer to driver's private data object.
	 * @return 0 on success
	 * @return EIO if operation failed
	 * @return EINVAL if input parameters are invalid
	 */
	uint32_t (*get_sniffer_mode)(void* pd, bool* state);

	/**
	 * Enable or Disable internal Framer. If 1, the internal Framer
	 * device is enabled. If set 0, the internal Framer device is
	 * disabled.
	 * @param[in] state New state
	 * @param[in] pd Pointer to driver's private data object.
	 * @return 0 on success
	 * @return EIO if operation failed
	 * @return EINVAL if input parameters are invalid
	 */
	uint32_t (*set_framer_enabled)(void* pd, bool state);

	/**
	 * Returns state of internal Framer.
	 * @param[out] state Current state
	 * @param[in] pd Pointer to driver's private data object.
	 * @return 0 on success
	 * @return EIO if operation failed
	 * @return EINVAL if input parameters are invalid
	 */
	uint32_t (*get_framer_enabled)(void* pd, bool* state);

	/**
	 * Enable or Disable internal Generic Device. If 1, the internal
	 * Generic device is enabled. If set 0, the internal Generic device
	 * is disabled.
	 * @param[in] state New state
	 * @param[in] pd Pointer to driver's private data object.
	 * @return 0 on success
	 * @return EIO if operation failed
	 * @return EINVAL if input parameters are invalid
	 */
	uint32_t (*set_device_enabled)(void* pd, bool state);

	/**
	 * Returns state of internal Generic Device.
	 * @param[out] state Current state
	 * @param[in] pd Pointer to driver's private data object.
	 * @return 0 on success
	 * @return EIO if operation failed
	 * @return EINVAL if input parameters are invalid
	 */
	uint32_t (*get_device_enabled)(void* pd, bool* state);

	/**
	 * If set to 1, then whole SLIMbus Manager will be detached from
	 * SLIMbus. While detached it will not be able to receive and send
	 * messages, and internal Framer and Generic Device will be disabled.
	 * It will also lose assigned Logical Addresses, and will have to be
	 * enumerated again
	 * @param[in] state New state
	 * @param[in] pd Pointer to driver's private data object.
	 * @return 0 on success
	 * @return EIO if operation failed
	 * @return EINVAL if input parameters are invalid
	 */
	uint32_t (*set_go_absent)(void* pd, bool state);

	/**
	 * Informs if SLIMbus Manager Component was set to Go Absent.
	 * @param[out] state Current state
	 * @param[in] pd Pointer to driver's private data object.
	 * @return 0 on success
	 * @return EIO if operation failed
	 * @return EINVAL if input parameters are invalid
	 */
	uint32_t (*get_go_absent)(void* pd, bool* state);

	/**
	 * Sets Framer configuration.
	 * @param[in] framer_config Pointer to structure containing Framer configuration.
	 * @param[in] pd Pointer to driver's private data.
	 * @return 0 on success
	 * @return EIO if operation failed
	 * @return EINVAL if input parameters are invalid
	 */
	uint32_t (*set_framer_config)(void* pd, slimbus_framer_config* framer_config);

	/**
	 * Obtains Framer configuration.
	 * @param[out] framer_config Pointer to structure to which Framer configuration will be written.
	 * @param[in] pd Pointer to driver's private data.
	 * @return 0 on success
	 * @return EIO if operation failed
	 * @return EINVAL if input parameters are invalid
	 */
	uint32_t (*get_framer_config)(void* pd, slimbus_framer_config* framer_config);

	/**
	 * Sets Generic Device configuration.
	 * @param[in] generic_device_config Pointer to structure containing Generic Device configuration.
	 * @param[in] pd Pointer to driver's private data.
	 * @return 0 on success
	 * @return EIO if operation failed
	 * @return EINVAL if input parameters are invalid
	 */
	uint32_t (*set_generic_device_config)(void* pd, slimbus_generic_device_config* generic_device_config);

	/**
	 * Obtains Generic Device configuration.
	 * @param[out] generic_device_config Pointer to structure to which Generic Device configuration will be written.
	 * @param[in] pd Pointer to driver's private data.
	 * @return 0 on success
	 * @return EIO if operation failed
	 * @return EINVAL if input parameters are invalid
	 */
	uint32_t (*get_generic_device_config)(void* pd, slimbus_generic_device_config* generic_device_config);

	/**
	 * Obtains status of Data port implemented in the Generic Device in
	 * Manager component.
	 * @param[in] port_number Selected Port Number
	 * @param[out] port_status Pointer to structure to which Framer configuration will be written.
	 * @param[in] pd Pointer to driver's private data.
	 * @return 0 on success
	 * @return EIO if operation failed
	 * @return EINVAL if input parameters are invalid
	 */
	uint32_t (*get_data_port_status)(void* pd, uint8_t port_number, slimbus_data_port_status* port_status);

	/**
	 * When the IP is in PAUSE state, and the Framer function is enabled,
	 * then calling this function forces to generate a toggle at SLIMbus
	 * data line, what in turn wakes-up SLIMbus from PAUSE state. When
	 * the Framer function is enabled, internal Framer is active and
	 * pausing at Root Frequency change is enabled using
	 * pause_at_root_frequency_change parameter, then calling this function
	 * resumes the operation of the Framer after change of Root
	 * Frequency.
	 * @param[in] pd Pointer to driver's private data.
	 * @return 0 on success
	 * @return EINVAL if input parameters are invalid
	 */
	uint32_t (*unfreeze)(void* pd);

	/**
	 * Cancels pending configuration (clears CFG_STROBE).
	 * @param[in] pd Pointer to driver's private data.
	 * @return 0 on success
	 * @return EINVAL if input parameters are invalid
	 */
	uint32_t (*cancel_configuration)(void* pd);

	/**
	 * Returns status of Manager Synchronization with SLIMbus
	 * @param[out] sf_sync If 1, the SLIMbus Manager achieved superframe synchronization. Not returned if NULL was passed.
	 * @param[out] sfb_sync If 1, the SLIMbus Manager achieved superframe block synchronization. Not returned if NULL was passed.
	 * @param[out] m_sync If 1, the SLIMbus Manager achieved message synchronization. Not returned if NULL was passed.
	 * @param[out] f_sync If 1, the SLIMbus Manager achieved frame synchronization. Not returned if NULL was passed.
	 * @param[out] ph_sync If 1, the SLIMbus Manager achieved phase synchronization and clock signal generated by the Generic device is valid. Not returned if NULL was passed.
	 * @param[in] pd Pointer to driver's private data.
	 * @return 0 on success
	 * @return EINVAL if input parameters are invalid
	 */
	uint32_t (*get_status_synchronization)(void* pd, bool* f_sync, bool* sf_sync, bool* m_sync, bool* sfb_sync, bool* ph_sync);

	/**
	 * Informs if SLIMbus Manager is detached from the bus after
	 * successful transmission of REPORT_ABSENT message.
	 * @param[out] detached If 1, the SLIMbus Manager is detached from the bus after successful transmission of REPORT_ABSENT message.
	 * @param[in] pd Pointer to driver's private data.
	 * @return 0 on success
	 * @return EINVAL if input parameters are invalid
	 */
	uint32_t (*get_status_detached)(void* pd, bool* detached);

	/**
	 * Returns information about SLIMbus operation properties
	 * @param[out] subframe_mode This variable contains the Subframe Mode read from the Framing Information during superframe synchronization and after reconfiguration (from the payload of the NEXT_SUBFRAME_MODE message sent by the SLIMbus Manager). Not returned if NULL was passed.
	 * @param[out] clock_gear This variable contains the Clock Gear read from the Framing Information during superframe synchronization and after reconfiguration (from the payload of the NEXT_SUBFRAME_MODE message sent by the SLIMbus Manager). Not returned if NULL was passed.
	 * @param[out] root_fr This variable contains the Root Frequency read from the Framing Information during superframe synchronization and after reconfiguration (from the payload of the NEXT_SUBFRAME_MODE message sent by the SLIMbus Manager). Not returned if NULL was passed.
	 * @param[in] pd Pointer to driver's private data.
	 * @return 0 on success
	 * @return EINVAL if input parameters are invalid
	 */
	uint32_t (*get_status_slimbus)(void* pd, slimbus_drv_subframe_mode* subframe_mode, slimbus_drv_clock_gear* clock_gear, slimbus_drv_root_frequency* root_fr);

	/**
	 * Sends ASSIGN_LOGICAL_ADDRESS Message, which assigns a Logical
	 * Address to a Device.
	 * @param[in] new_la New Logical Address
	 * @param[in] destination_ea Destination's Enumeration Address
	 * @param[in] pd Pointer to driver's private data.
	 * @return 0 on success
	 * @return EIO if operation failed
	 * @return EINVAL if input parameters are invalid
	 */
	uint32_t (*msg_assign_logical_address)(void* pd, uint64_t destination_ea, uint8_t new_la);

	/**
	 * Sends RESET_DEVICE Message, which informs a Device to perform its
	 * reset procedure.
	 * @param[in] destination_la Destination's Logic Address
	 * @param[in] pd Pointer to driver's private data.
	 * @return 0 on success
	 * @return EIO if operation failed
	 * @return EINVAL if input parameters are invalid
	 */
	uint32_t (*msg_reset_device)(void* pd, uint8_t destination_la);

	/**
	 * Sends CHANGE_LOGICAL_ADDRESS Message, which changes the value of
	 * the Logical Address of the destination Device.
	 * @param[in] destination_la Destination's Logic Address
	 * @param[in] new_la New Logical Address
	 * @param[in] pd Pointer to driver's private data.
	 * @return 0 on success
	 * @return EIO if operation failed
	 * @return EINVAL if input parameters are invalid
	 */
	uint32_t (*msg_change_logical_address)(void* pd, uint8_t destination_la, uint8_t new_la);

	/**
	 * Sends CHANGE_ARBITRATION_PRIORITY Message, which is sent to one or
	 * more Devices to change the value of their Arbitration Priority.
	 * @param[in] broadcast Broadcast Message to all devices
	 * @param[in] destination_la Destination's Logic Address, used if broadcast = 0
	 * @param[in] new_arbitration_priority New Arbitration Priority
	 * @param[in] pd Pointer to driver's private data.
	 * @return 0 on success
	 * @return EIO if operation failed
	 * @return EINVAL if input parameters are invalid
	 */
	uint32_t (*msg_change_arbitration_priority)(void* pd, bool broadcast, uint8_t destination_la, slimbus_arbitration_priority new_arbitration_priority);

	/**
	 * Sends REQUEST_SELF_ANNOUNCEMENT Message, which requests an
	 * unenumerated Device retransmit a REPORT_PRESENT Message.
	 * @param[in] pd Pointer to driver's private data.
	 * @return 0 on success
	 * @return EIO if operation failed
	 * @return EINVAL if input parameters are invalid
	 */
	uint32_t (*msg_request_self_announcement)(void* pd);

	/**
	 * Sends CONNECT_SOURCE Message, which informs the Device to connect
	 * the specified Port, to the specified Data Channel. The Port shall
	 * act as the data source for the Data Channel.
	 * @param[in] destination_la Destination's Logic Address
	 * @param[in] channel_number Data Channel Number
	 * @param[in] port_number Port Number to be connected to data channel
	 * @param[in] pd Pointer to driver's private data.
	 * @return 0 on success
	 * @return EIO if operation failed
	 * @return EINVAL if input parameters are invalid
	 */
	uint32_t (*msg_connect_source)(void* pd, uint8_t destination_la, uint8_t port_number, uint8_t channel_number);

	/**
	 * Sends CONNECT_SINK Message, which informs the Device to connect
	 * the specified Port, to the specified Data Channel. The Port shall
	 * act as the data sink for the Data Channel.
	 * @param[in] destination_la Destination's Logic Address
	 * @param[in] channel_number Data Channel Number
	 * @param[in] port_number Port Number to be connected to data channel
	 * @param[in] pd Pointer to driver's private data.
	 * @return 0 on success
	 * @return EIO if operation failed
	 * @return EINVAL if input parameters are invalid
	 */
	uint32_t (*msg_connect_sink)(void* pd, uint8_t destination_la, uint8_t port_number, uint8_t channel_number);

	/**
	 * Sends DISCONNECT_PORT Message, which informs the Device to
	 * disconnect the Port specified by Port Number.
	 * @param[in] destination_la Destination's Logic Address
	 * @param[in] port_number Port Number to be disconnected from data channel
	 * @param[in] pd Pointer to driver's private data.
	 * @return 0 on success
	 * @return EIO if operation failed
	 * @return EINVAL if input parameters are invalid
	 */
	uint32_t (*msg_disconnect_port)(void* pd, uint8_t destination_la, uint8_t port_number);

	/**
	 * Sends CHANGE_CONTENT Message, which broadcasts detailed
	 * information about the structure of the Data Channel contents.
	 * @param[in] data_length Data Length
	 * @param[in] data_type Data Type
	 * @param[in] presence_rate Presence Rate
	 * @param[in] frequency_locked_bit Frequency Locked Bit
	 * @param[in] channel_link Channel Link
	 * @param[in] channel_number Data Channel Number
	 * @param[in] pd Pointer to driver's private data.
	 * @param[in] auxiliary_bit_format Auxiliary Bit Format
	 * @return 0 on success
	 * @return EIO if operation failed
	 * @return EINVAL if input parameters are invalid
	 */
	uint32_t (*msg_change_content)(void* pd, uint8_t channel_number, bool frequency_locked_bit, slimbus_drv_presence_rate presence_rate,
	    slimbus_drv_aux_field_format auxiliary_bit_format, slimbus_drv_data_type data_type, bool channel_link, uint8_t data_length);

	/**
	 * Sends REQUEST_INFORMATION Message, which instructs a Device to
	 * send the indicated Information Slice.
	 * @param[in] broadcast Broadcast Message to all devices
	 * @param[in] destination_la Destination's Logic Address, used if broadcast = 0
	 * @param[in] transaction_id Transaction ID
	 * @param[in] element_code Element Code
	 * @param[in] pd Pointer to driver's private data.
	 * @return 0 on success
	 * @return EIO if operation failed
	 * @return EINVAL if input parameters are invalid
	 */
	uint32_t (*msg_request_information)(void* pd, bool broadcast, uint8_t destination_la, uint8_t transaction_id, uint16_t element_code);

	/**
	 * Sends REQUEST_CLEAR_INFORMATION Message, which instructs a Device
	 * to send the indicated Information Slice and to clear all, or
	 * parts, of that Information Slice.
	 * @param[in] destination_la Destination's Logic Address, used if broadcast = 0
	 * @param[in] clear_mask_size Clear Mask size
	 * @param[in] broadcast Broadcast Message to all devices
	 * @param[in] clear_mask Pointer to Clear Mask (0 (NULL) to 16 bytes). If the size of Clear Mask is smaller than the size of the Information Slice, the Device shall pad the MSBs of Clear Mask with ones. If the size of Clear Mask is larger than the size of the Information Slice, the Device shall change only the portion of the Information Map corresponding to the identified Information Slice.
	 * @param[in] element_code Element Code
	 * @param[in] pd Pointer to driver's private data.
	 * @param[in] transaction_id Transaction ID
	 * @return 0 on success
	 * @return EIO if operation failed
	 * @return EINVAL if input parameters are invalid
	 */
	uint32_t (*msg_request_clear_information)(void* pd, bool broadcast, uint8_t destination_la, uint8_t transaction_id, uint16_t element_code, uint8_t* clear_mask, uint8_t clear_mask_size);

	/**
	 * Sends CLEAR_INFORMATION Message, which instructs a Device to
	 * selectively clear all, or parts, of the Information Slice.
	 * @param[in] destination_la Destination's Logic Address, used if broadcast = 0
	 * @param[in] clear_mask_size Clear Mask size
	 * @param[in] broadcast Broadcast Message to all devices
	 * @param[in] clear_mask Pointer to Clear Mask (0 (NULL) to 16 bytes). If the size of Clear Mask is smaller than the size of the Information Slice, the Device shall pad the MSBs of Clear Mask with ones. If the size of Clear Mask is larger than the size of the Information Slice, the Device shall change only the portion of the Information Map corresponding to the identified Information Slice.
	 * @param[in] element_code Element Code
	 * @param[in] pd Pointer to driver's private data.
	 * @return 0 on success
	 * @return EIO if operation failed
	 * @return EINVAL if input parameters are invalid
	 */
	uint32_t (*msg_clear_information)(void* pd, bool broadcast, uint8_t destination_la, uint16_t element_code, uint8_t* clear_mask, uint8_t clear_mask_size);

	/**
	 * Sends BEGIN_RECONFIGURATION Message, which informs all Devices of
	 * the start of a Reconfiguration Sequence.
	 * @param[in] pd Pointer to driver's private data.
	 * @return 0 on success
	 * @return EIO if operation failed
	 * @return EINVAL if input parameters are invalid
	 */
	uint32_t (*msg_begin_reconfiguration)(void* pd);

	/**
	 * Sends Message NEXT_ACTIVE_FRAMER, which informs a Device that the
	 * active Framer is going to hand over the role of Framer to a
	 * specified inactive Framer with the Logical Address.
	 * @param[in] outgoing_framer_clock_cycles Number of Outgoing Framer Clock cycles
	 * @param[in] incoming_framer_la Incoming Framer's Logic Address
	 * @param[in] pd Pointer to driver's private data.
	 * @param[in] incoming_framer_clock_cycles Number of Incoming Framer Clock cycles
	 * @return 0 on success
	 * @return EIO if operation failed
	 * @return EINVAL if input parameters are invalid
	 */
	uint32_t (*msg_next_active_framer)(void* pd, uint8_t incoming_framer_la, uint16_t outgoing_framer_clock_cycles, uint16_t incoming_framer_clock_cycles);

	/**
	 * Sends Message NEXT_SUBFRAME_MODE, which informs all destinations
	 * that the active Manager intends to change the Subframe Mode.
	 * @param[in] new_subframe_mode New Subframe Mode
	 * @param[in] pd Pointer to driver's private data.
	 * @return 0 on success
	 * @return EIO if operation failed
	 * @return EINVAL if input parameters are invalid
	 */
	uint32_t (*msg_next_subframe_mode)(void* pd, slimbus_drv_subframe_mode new_subframe_mode);

	/**
	 * Sends NEXT_CLOCK_GEAR Message, which informs all destinations that
	 * the active Manager intends to change the Clock Gear.
	 * @param[in] new_clock_gear New Clock Gear
	 * @param[in] pd Pointer to driver's private data.
	 * @return 0 on success
	 * @return EIO if operation failed
	 * @return EINVAL if input parameters are invalid
	 */
	uint32_t (*msg_next_clock_gear)(void* pd, slimbus_drv_clock_gear new_clock_gear);

	/**
	 * Sends NEXT_ROOT_FREQUENCY Message, which informs all destinations
	 * that the active Manager intends to change the Root Frequency.
	 * @param[in] new_root_frequency New Root Frequency
	 * @param[in] pd Pointer to driver's private data.
	 * @return 0 on success
	 * @return EIO if operation failed
	 * @return EINVAL if input parameters are invalid
	 */
	uint32_t (*msg_next_root_frequency)(void* pd, slimbus_drv_root_frequency new_root_frequency);

	/**
	 * Sends NEXT_PAUSE_CLOCK Message, which informs all destinations
	 * that the active Manager intends to have the active Framer pause
	 * the bus.
	 * @param[in] new_restart_time New Restart Time
	 * @param[in] pd Pointer to driver's private data.
	 * @return 0 on success
	 * @return EIO if operation failed
	 * @return EINVAL if input parameters are invalid
	 */
	uint32_t (*msg_next_pause_clock)(void* pd, slimbus_drv_restart_time new_restart_time);

	/**
	 * Sends NEXT_RESET_BUS Message, which informs all destinations that
	 * the active Manager intends to have the active Framer reset the
	 * bus.
	 * @param[in] pd Pointer to driver's private data.
	 * @return 0 on success
	 * @return EIO if operation failed
	 * @return EINVAL if input parameters are invalid
	 */
	uint32_t (*msg_next_reset_bus)(void* pd);

	/**
	 * Sends NEXT_SHUTDOWN_BUS Message, which informs all destinations
	 * that the active Manager intends to shutdown the bus.
	 * @param[in] pd Pointer to driver's private data.
	 * @return 0 on success
	 * @return EIO if operation failed
	 * @return EINVAL if input parameters are invalid
	 */
	uint32_t (*msg_next_shutdown_bus)(void* pd);

	/**
	 * Sends NEXT_DEFINE_CHANNEL Message, which informs a Device that the
	 * active Manager intends to define the parameters of a Data Channel.
	 * @param[in] channel_number Data Channel Number
	 * @param[in] segment_distribution Segment Distribution
	 * @param[in] transport_protocol Transport Protocol
	 * @param[in] pd Pointer to driver's private data.
	 * @param[in] segment_length Segment Length
	 * @return 0 on success
	 * @return EIO if operation failed
	 * @return EINVAL if input parameters are invalid
	 */
	uint32_t (*msg_next_define_channel)(void* pd, uint8_t channel_number, slimbus_drv_transport_protocol transport_protocol, uint16_t segment_distribution, uint8_t segment_length);

	/**
	 * Sends NEXT_DEFINE_CONTENT Message, which informs a Device how the
	 * Data Channel CN shall be used.
	 * @param[in] data_length Data Length
	 * @param[in] data_type Data Type
	 * @param[in] presence_rate Presence Rate
	 * @param[in] frequency_locked_bit Frequency Locked Bit
	 * @param[in] channel_link Channel Link
	 * @param[in] channel_number Data Channel Number
	 * @param[in] pd Pointer to driver's private data.
	 * @param[in] auxiliary_bit_format Auxiliary Bit Format
	 * @return 0 on success
	 * @return EIO if operation failed
	 * @return EINVAL if input parameters are invalid
	 */
	uint32_t (*msg_next_define_content)(void* pd, uint8_t channel_number, bool frequency_locked_bit, slimbus_drv_presence_rate presence_rate,
	    slimbus_drv_aux_field_format auxiliary_bit_format, slimbus_drv_data_type data_type, bool channel_link, uint8_t data_length);

	/**
	 * Sends NEXT_ACTIVATE_CHANNEL Message, which is used to switch on
	 * the specified Data Channel.
	 * @param[in] channel_number Data Channel Number
	 * @param[in] pd Pointer to driver's private data.
	 * @return 0 on success
	 * @return EIO if operation failed
	 * @return EINVAL if input parameters are invalid
	 */
	uint32_t (*msg_next_activate_channel)(void* pd, uint8_t channel_number);

	/**
	 * Sends NEXT_DEACTIVATE_CHANNEL Message, which is used to switch off
	 * the specified Data Channel.
	 * @param[in] channel_number Data Channel Number
	 * @param[in] pd Pointer to driver's private data.
	 * @return 0 on success
	 * @return EIO if operation failed
	 * @return EINVAL if input parameters are invalid
	 */
	uint32_t (*msg_next_deactivate_channel)(void* pd, uint8_t channel_number);

	/**
	 * Sends NEXT_REMOVE_CHANNEL Message, which is used to switch off and
	 * disconnect the specified Data Channel.
	 * @param[in] channel_number Data Channel Number
	 * @param[in] pd Pointer to driver's private data.
	 * @return 0 on success
	 * @return EIO if operation failed
	 * @return EINVAL if input parameters are invalid
	 */
	uint32_t (*msg_next_remove_channel)(void* pd, uint8_t channel_number);

	/**
	 * Sends RECONFIGURE_NOW Message, which is used to change the bus
	 * configuration.
	 * @param[in] pd Pointer to driver's private data.
	 * @return 0 on success
	 * @return EIO if operation failed
	 * @return EINVAL if input parameters are invalid
	 */
	uint32_t (*msg_reconfigure_now)(void* pd);

	/**
	 * Sends REQUEST_VALUE Message, which instructs a Device to send the
	 * indicated Value Slice.
	 * @param[in] broadcast Broadcast Message to all devices
	 * @param[in] destination_la Destination's Logic Address, used if broadcast = 0
	 * @param[in] transaction_id Transaction ID
	 * @param[in] element_code Element Code
	 * @param[in] pd Pointer to driver's private data.
	 * @return 0 on success
	 * @return EIO if operation failed
	 * @return EINVAL if input parameters are invalid
	 */
	uint32_t (*msg_request_value)(void* pd, bool broadcast, uint8_t destination_la, uint8_t transaction_id, uint16_t element_code);

	/**
	 * Sends REQUEST_CHANGE_VALUE Message, which instructs a Device to
	 * send the specified Value Slice and to update that Value Slice.
	 * @param[in] destination_la Destination's Logic Address, used if broadcast = 0
	 * @param[in] value_update Pointer to Value Update (0 (NULL) to 16 bytes).
	 * @param[in] broadcast Broadcast Message to all devices
	 * @param[in] element_code Element Code
	 * @param[in] pd Pointer to driver's private data.
	 * @param[in] value_update_size Value Update size
	 * @param[in] transaction_id Transaction ID
	 * @return 0 on success
	 * @return EIO if operation failed
	 * @return EINVAL if input parameters are invalid
	 */
	uint32_t (*msg_request_change_value)(void* pd, bool broadcast, uint8_t destination_la, uint8_t transaction_id, uint16_t element_code, uint8_t* value_update, uint8_t value_update_size);

	/**
	 * Sends CHANGE_VALUE Message, which instructs a Device to update the
	 * indicated Value Slice.
	 * @param[in] destination_la Destination's Logic Address, used if broadcast = 0
	 * @param[in] value_update Pointer to Value Update (0 (NULL) to 16 bytes).
	 * @param[in] broadcast Broadcast Message to all devices
	 * @param[in] element_code Element Code
	 * @param[in] pd Pointer to driver's private data.
	 * @param[in] value_update_size Value Update size
	 * @return 0 on success
	 * @return EIO if operation failed
	 * @return EINVAL if input parameters are invalid
	 */
	uint32_t (*msg_change_value)(void* pd, bool broadcast, uint8_t destination_la, uint16_t element_code, uint8_t* value_update, uint8_t value_update_size);

} slimbus_drv_obj;

/**
 * In order to access the slimbus APIs, the upper layer software must call
 * this global function to obtain the pointer to the driver object.
 * @return slimbus_drv_obj* Driver Object Pointer
 */
extern slimbus_drv_obj *slimbus_drv_get_instance(void);

/**
 *	@}
 */


 #endif
