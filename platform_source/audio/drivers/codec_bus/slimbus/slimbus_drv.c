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

#include "slimbus_drv.h"

#include <linux/string.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/mutex.h>
#include <linux/delay.h>
#include <linux/completion.h>
#include <linux/workqueue.h>
#include <linux/device.h>
#include <linux/pm_wakeup.h>
#include <linux/version.h>
#include <rdr_audio_adapter.h>
#ifdef CONFIG_HUAWEI_DSM
#include <dsm_audio/dsm_audio.h>
#endif

#include "slimbus.h"
#include "audio_log.h"

/*lint -e715 -e838 -e730 -e578 -e64 -e647 -e514 -e611*/
#define LOG_TAG "slimbus_drv"

#define RFC_MAX_DEVICES 32
#define SOC_DEVICE_NUM 3
#define DA_COMBINE_DEVICE_NUM 3
#define ENUMERATION_TIMES 400
#define DUMP_SLIMBUS_REGS_SIZE 0x2FC
#define SLIMBUS_PORT_NUM 8

/* 2us */
#define REG_ACCESS_TIME_DIFF 2

#define GIC_BASE_ADDR 0xE82B0000
#define GIC_SIZE 0x7fff

/* used for element RD/WR protection */
static struct mutex slimbus_mutex;
static spinlock_t slimbus_spinlock;

/*
 * List of enumerated devices
 */
struct rfc_enumerated_devices {
	uint8_t devices;
	uint8_t framer_cnt;
	uint8_t generic_cnt;
	uint8_t interface_cnt;
	uint8_t manager_cnt;
	uint8_t framer_logical_addr[RFC_MAX_DEVICES];
	uint8_t generic_logical_addr[RFC_MAX_DEVICES];
	uint8_t interface_logical_addr[RFC_MAX_DEVICES];
	uint8_t manager_logical_addr[RFC_MAX_DEVICES];
};

/*
 * Requested Values and Information Slices
 */
struct rfc_internal_reply {
	uint8_t *ptr;
	uint8_t size;
	uint8_t transaction_id;
	bool received;
	bool value_requested;
	bool information_requested;
	bool error;
	struct completion read_finish;
	struct completion request_finish;
};

/*
 * Variables set by interrupts, used for reporting results
 */
struct rfc_slimbus_events {
	bool ie_unsupported_msg;
	bool ie_ex_error;
	bool ie_reconfig_objection;
	uint8_t reported_presence;
	uint8_t reported_absence;
};

/*
 * Global Variables
 */

/* Pointer to driver object */
static slimbus_drv_obj *slimbus_drv;
static void *devm_slimbus_priv;

static struct rfc_enumerated_devices slimbus_devices;
static struct rfc_internal_reply internal_reply;
static struct rfc_slimbus_events events;

volatile uint32_t lostms_times;
volatile uint32_t slimbus_drv_log_count = 50;
int slimbus_irq_state;
volatile bool int_need_clear;

static int64_t last_time;
static uint32_t dsm_notify_limit = 1;
static uintptr_t asp_base_reg;
static uintptr_t sctrl_base_addr;
static enum platform_type plat_type = PLATFORM_PHONE;
static struct workqueue_struct *slimbus_lost_sync_wq;
static struct delayed_work slimbus_lost_sync_delay_work;
static struct wakeup_source *slimbus_wake_lock;

static void slimbus_dump_state(enum slimbus_dump_state type);

volatile uint32_t slimbus_drv_lostms_get(void)
{
	return lostms_times;
}

void slimbus_drv_lostms_set(uint32_t count)
{
	lostms_times = count;
}

volatile bool slimbus_int_need_clear_get(void)
{
	return int_need_clear;
}

void slimbus_int_need_clear_set(volatile bool flag)
{
	int_need_clear = flag;
}

int64_t get_timeus(void)
{
	int64_t timeus;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,4,0))
	struct timespec64 time;

	ktime_get_real_ts64(&time);
	timeus = 1000000 * time.tv_sec + time.tv_nsec / 1000;
#else
	struct timeval time;

	do_gettimeofday(&time);
	timeus = 1000000 * time.tv_sec + time.tv_usec;
#endif

	return timeus;
}

/*
 * Interrupts Handler
 * When interrupt from SLIMbus Manager was received,
 * isr() function should be called.
 */
static irqreturn_t rfc_irq_handler(int value, void *data)
{
	/* Process interrupt by the driver */

	slimbus_drv->isr(devm_slimbus_priv);

	return IRQ_HANDLED;
}

static void rfc_clear_events(void)
{
	events.ie_ex_error = 0;
	events.ie_reconfig_objection = 0;
	events.ie_unsupported_msg = 0;
	events.reported_presence = 0;
	events.reported_absence = 0;
}

/***********************************************************************************************
 * Event Handlers
 ***********************************************************************************************/

/**
 * Handler for Device Class driver request
 * @param[in] logicalAddress Device's Logical Address
 * @return Device Class
 */
static slimbus_device_class device_class_handler(uint8_t logical_addr)
{
	uint8_t i;

	/* Active Manager always has 0xFF as logical address */
	if (logical_addr == 0xFF)
		return DC_MANAGER;

	/* Loop through list of enumerated Framer devices */
	for (i = 0; i < slimbus_devices.framer_cnt; i++) {
		if (logical_addr == slimbus_devices.framer_logical_addr[i])
			return DC_FRAMER;
	}
	/* Loop through list of enumerated Generic devices */
	for (i = 0; i < slimbus_devices.generic_cnt; i++) {
		if (logical_addr == slimbus_devices.generic_logical_addr[i])
			return DC_GENERIC;
	}

	/* Loop through list of enumerated Interface devices */
	for (i = 0; i < slimbus_devices.interface_cnt; i++) {
		if (logical_addr == slimbus_devices.interface_logical_addr[i])
			return DC_INTERFACE;
	}
	/* Loop through list of enumerated Generic devices
	 * Not needed in most cases.
	 * Usually there is only one manager in the system
	 * and it's active, so always has 0xFF as logical address
	 */
	for (i = 0; i < slimbus_devices.manager_cnt; i++) {
		if (logical_addr == slimbus_devices.manager_logical_addr[i])
			return DC_MANAGER;
	}

	return DC_INTERFACE;
}

static void rfc_print_ie_core_status(const slimbus_information_elements *ie,
	uint8_t source_logical_addr, slimbus_device_class device_class)
{
	if (ie->core_unsprtd_msg || ie->core_data_tx_col || ie->core_reconfig_objection || ie->core_ex_error)
		slimbus_dev_limit_info("Core: (0x%x,0x%x) UNSPRTD_MSG:%X DATA_TX_COL:%X RECONFIG_OBJECTION:%X EX_ERROR:%X \n",
			device_class,
			source_logical_addr,
			ie->core_unsprtd_msg,
			ie->core_data_tx_col,
			ie->core_reconfig_objection,
			ie->core_ex_error);
}

static void rfc_print_ie_interface_status(const slimbus_information_elements *ie,
	uint8_t source_logical_addr, slimbus_device_class device_class)
{
	if (ie->interface_mc_tx_col || ie->interface_lost_fs ||
		ie->interface_lost_sfs || ie->interface_lost_ms || ie->interface_data_slot_overlap)
		slimbus_dev_limit_info("Interface: (0x%x,0x%x) MC_TX_COL:%X LOST_FS:%X LOST_SFS:%X LOST_MS:%X DATA_SLOT_OVERLAP:%X\n",
			device_class,
			source_logical_addr,
			ie->interface_mc_tx_col,
			ie->interface_lost_fs,
			ie->interface_lost_sfs,
			ie->interface_lost_ms,
			ie->interface_data_slot_overlap);
}

static void rfc_print_ie_manager_status(const slimbus_information_elements *ie,
	uint8_t source_logical_addr, slimbus_device_class device_class)
{
	if (ie->manager_active_manager)
		slimbus_dev_limit_info("Manager: (0x%x,0x%x) ACTIVE_MANAGER:%X\n",
			device_class, source_logical_addr, ie->manager_active_manager);
}

static void rfc_print_ie_framer_status(const slimbus_information_elements *ie,
	uint8_t source_logical_addr, slimbus_device_class device_class)
{
	if (ie->framer_quality || ie->framer_gc_tx_col ||
		ie->framer_fi_tx_col || ie->framer_fs_tx_col || ie->framer_active_framer)
		slimbus_dev_limit_info("Framer: (0x%x,0x%x) QUALITY:%X GC_TX_COL:%X FI_TX_COL:%X FS_TX_COL:%X ACTIVE_FRAMER:%X\n",
			device_class,
			source_logical_addr,
			ie->framer_quality,
			ie->framer_gc_tx_col,
			ie->framer_fi_tx_col,
			ie->framer_fs_tx_col,
			ie->framer_active_framer);
}

static void rfc_print_ie_status(const slimbus_information_elements *ie, uint8_t source_logical_addr)
{
	slimbus_device_class device_class = device_class_handler(source_logical_addr);

	rfc_print_ie_core_status(ie, source_logical_addr, device_class);

	rfc_print_ie_interface_status(ie, source_logical_addr, device_class);

	rfc_print_ie_manager_status(ie, source_logical_addr, device_class);

	rfc_print_ie_framer_status(ie, source_logical_addr, device_class);

	if (ie->interface_lost_fs || ie->interface_lost_sfs || ie->interface_lost_ms) {
		if (dsm_notify_limit == SLIMBUS_LOSTMS_COUNT) {
			slimbus_dump_state(SLIMBUS_DUMP_LOSTMS);
#ifdef CONFIG_HUAWEI_DSM
			audio_dsm_report_info(AUDIO_CODEC, DSM_HI6402_SLIMBUS_LOST_MS, "DSM_HI6402_SLIMBUS_LOST_MS\n");
#endif
		}
		dsm_notify_limit++;
	}
}

/*
 * Handler for Logical Address for device driver request
 * Will be called for each device which sent REPORT_PRESENCE Message, if enumerate_devices was enabled.
 * @param[in] enumerationAddress Device's Enumeration Address
 * @param[in] class Device Class
 * @return Logical Address
 */
static uint8_t assign_logical_address_handler(uint64_t enumeration_address, slimbus_device_class class)
{
	/*
	 * Valid Logical Addresses are in range 0x00 - 0xEF.
	 * Logical Addresses 0xF[0-E] are reserved.
	 * 0xFF = active Manager
	 * If invalid value is returned, it will be ignored by the driver, and device will not be enumerated.
	 */
	uint8_t logical_addr = 0xF0;

	switch (class) {
	/* New logical address with offset 0x00 */
	case DC_FRAMER:
		logical_addr = 0x20 + slimbus_devices.framer_cnt;
		slimbus_devices.framer_logical_addr[slimbus_devices.framer_cnt++] = logical_addr;
		break;

	/* New logical address with offset 0x40 */
	case DC_INTERFACE:
		logical_addr = 0x40 | slimbus_devices.interface_cnt;
		slimbus_devices.interface_logical_addr[slimbus_devices.interface_cnt++] = logical_addr;
		break;

	case DC_GENERIC:
		/*
		 * Use Enumeration Addresses to assign always the same logical addresses for known devices,
		 * regardless of the presence report order
		 */
		switch (enumeration_address) {
		/* Device 1 Generic Device, example Enumeration Address, Change might be needed */
		case SOC_EA_GENERIC_DEVICE:
			logical_addr = SOC_LA_GENERIC_DEVICE;
			break;

		/* Device 2 Generic Device, example Enumeration Address, Change might be needed */
		case DA_COMBINE_EA_GENERIC_DEVICE:
		case (DA_COMBINE_EA_GENERIC_DEVICE & 0xfffffffffffe):
			logical_addr = DA_COMBINE_LA_GENERIC_DEVICE;
			break;

		/* Unknown Generic Device, New logical address with offset 0x80 */
		default:
			logical_addr = 0x80 | slimbus_devices.generic_cnt;
			break;
		}
		slimbus_devices.generic_logical_addr[slimbus_devices.generic_cnt++] = logical_addr;
		break;

	/*
	 * In most cases not needed.
	 * If there is one manager in the system, it will be active, and always have 0xFF as logical address.
	 * Active Managers don't need enumeration
	 */
	case DC_MANAGER:
		/* New logical address with offset 0xE0 */
		logical_addr = 0xE0 | slimbus_devices.manager_cnt;
		slimbus_devices.manager_logical_addr[slimbus_devices.manager_cnt++] = logical_addr;
		break;

	/* Unknown device class code. Don't assign logical address (Invalid values (0xF[0-F]) are ignored by the driver). */
	default:
		return 0xF0;
	}

	slimbus_devices.devices++;

	AUDIO_LOGI("dev num %d, framer cnt %d, interface cnt %d, generic cnt %d, manager cnt %d",
		slimbus_devices.devices,
		slimbus_devices.framer_cnt,
		slimbus_devices.interface_cnt,
		slimbus_devices.generic_cnt,
		slimbus_devices.manager_cnt);

	return logical_addr;
}

/**
 * Handler for Receiving Message
 * Will be called for message that was received by Manager.
 * @param[in] pd driver's private data.
 * @param[in] message received message as structure
 */
static void received_message_handler(void *pd, struct slimbus_drv_message *message)
{
	slimbus_irq_state = 0x100 + message->message_code;
}

/**
 * Handler for Sending Message
 * Will be called for message that will be send by Manager.
 * @param[in] pd driver's private data.
 * @param[in] message message to be sent as structure
 */
static void sending_message_handler(void *pd, struct slimbus_drv_message *message)
{
	/* todo: add function */
}

/**
 * Handler for Reported Information Element
 * Will be called for REPORT_INFORMATION message that was received by Manager.
 * @param[in] pd driver's private data.
 * @param[in] source_logical_addr source device's Logical Address
 * @param[in] informationElements Structure with decoded information elements
 */
static void information_elements_handler(void *pd,
	uint8_t source_logical_addr, struct slimbus_information_elements *info)
{
	rfc_print_ie_status(info, source_logical_addr);

	/* If value or information was requested but it resulted in error, set variables here */
	if ((internal_reply.value_requested) || (internal_reply.information_requested)) {
		if ((info->core_unsprtd_msg) || (info->core_ex_error)) {
			internal_reply.error = 1;
			internal_reply.value_requested = 0;
			internal_reply.information_requested = 0;
		}
	}

	if (info->core_ex_error)
		events.ie_ex_error = 1;
	if (info->core_unsprtd_msg)
		events.ie_unsupported_msg = 1;
	if (info->core_reconfig_objection)
		events.ie_reconfig_objection = 1;
}

/**
 * Handler for REPLY_VALUE Messages
 * @param[in] pd driver's private data
 * @param[in] source_logical_addr source device's Logical Address
 * @param[in] transaction_id Transaction ID
 * @param[in] value_slice pointer to received Value data
 * @param[in] value_slice_length size of received Value
 */
static void msg_reply_value(void *pd, uint8_t source_logical_addr,
	uint8_t transaction_id, uint8_t *value_slice, uint8_t value_slice_length)
{
	/*
	 * If Value was requested and its transaction ID is equal to received reply,
	 * then copy its value to internal structure
	 */
	if (internal_reply.value_requested) {
		if (internal_reply.transaction_id == transaction_id) {
			memcpy((void *) internal_reply.ptr, (void *) value_slice, value_slice_length);
			internal_reply.received = 1;
			internal_reply.value_requested = 0;
			slimbus_irq_state = 3;
			complete(&(internal_reply.read_finish));
		} else {
			AUDIO_LOGE("transaction_id error! (%#x %#x)", internal_reply.transaction_id, transaction_id);
		}
	} else {
		AUDIO_LOGE("replyvalue error");
	}
}

/**
 * Handler for REPORT_PRESENCE Messages
 * @param[in] pd driver's private data
 * @param[in] source_ea source device's Enumeration Address
 * @param[in] device_class source device's Device Class
 * @param[in] device_class_version source device's Device Class Version
 */
static void msg_report_present_handler(void *pd, uint64_t source_ea,
	slimbus_device_class device_class, uint8_t device_class_version)
{
	events.reported_presence++;
}


/**
 * Handler for REPORT_ABSENT Messages
 * @param[in] pd driver's private data
 * @param[in] source_logical_addr source device's Logical Address
 */
static void msg_report_absent_handler(void *pd, uint8_t source_logical_addr)
{
	events.reported_absence++;
}


/**
 * Handler for REPORT_INFORMATION Messages
 * @param[in] pd driver's private data
 * @param[in] source_logical_addr source device's Logical Address
 * @param[in] element_code Element Code
 * @param[in] information_slice pointer to reported Information Slice
 * @param[in] information_slice_length reported Information Slice size
 */
static void msg_report_information_handler(void *pd, uint8_t source_logical_addr,
	uint16_t element_code, uint8_t *information_slice, uint8_t information_slice_length)
{
	/* todo: add function */
}

/**
 * Handler for REPLY_INFORMATION Messages
 * @param[in] pd driver's private data
 * @param[in] source_logical_addr source device's Logical Address
 * @param[in] transaction_id Transaction ID
 * @param[in] information_slice pointer to reported Information Slice
 * @param[in] information_slice_length reported Information Slice size
 */
static void msg_reply_information_handler(void *pd, uint8_t source_logical_addr,
	uint8_t transaction_id, uint8_t *information_slice, uint8_t information_slice_length)
{
	/*
	 * If Information was requested and its transaction ID is equal to received reply,
	 * then copy its value to internal structure
	 */
	if (internal_reply.information_requested) {
		if (internal_reply.transaction_id == transaction_id) {
			memcpy((void *) internal_reply.ptr, (void *) information_slice, information_slice_length);
			internal_reply.received = 1;
			internal_reply.information_requested = 0;
			slimbus_irq_state = 4;
			complete(&(internal_reply.request_finish));
		} else {
			AUDIO_LOGE("transaction_id error! (%#x %#x)", internal_reply.transaction_id, transaction_id);
		}
	} else {
		AUDIO_LOGE("ReplyInformation error!");
	}
}

static void slimbus_lost_sync_cb(struct work_struct *work)
{
	bool fsync = false;
	bool sfsync = false;
	bool msync = false;
	bool sfbsync = false;
	bool phsync = false;
	uint32_t ret;
	uint32_t wait_timeout = 0;

	while ((!fsync || !sfsync || !msync) && (wait_timeout++ < 100)) {
		ret = slimbus_drv->get_status_synchronization(devm_slimbus_priv, &fsync, &sfsync, &msync, &sfbsync, &phsync);
		if (ret)
			slimbus_dev_limit_err("get sync status error\n");
		usleep_range(300, 350);
	}

	if (fsync && sfsync && msync) {
		ret = slimbus_track_recover();
		if (ret)
			slimbus_dev_limit_err(" track recover failed\n");
		slimbus_dev_lostms_recover("wait_timeout:%d, fsync:%x, sfsync:%x, msync:%x, sfbsync:%x, phsync:%x\n",
			wait_timeout, fsync, sfsync, msync, sfbsync, phsync);
	} else {
		slimbus_dev_limit_err("recover failed! wait_timeout:%d \n", wait_timeout);
	}
}

static void manager_interrupts_handler(void *pd, slimbus_drv_interrupt interrupt)
{
	if ((interrupt & DRV_INT_SYNC_LOST) && (slimbus_devices.devices > SOC_DEVICE_NUM)) {
		slimbus_dev_limit_err("LOST SYNC, interrupt:%#x \n", interrupt);
		slimbus_dump_state(SLIMBUS_DUMP_LOSTMS);
		__pm_wakeup_event(slimbus_wake_lock, 1000);
		queue_delayed_work(slimbus_lost_sync_wq, &slimbus_lost_sync_delay_work, msecs_to_jiffies(50));
	}
}

/**
 * Create Element Code
 * @param[in] address element address
 * @param[in] byte_access 1 for byte access, 0 for element access
 * @param[in] slice_size size of slice
 * @return Element Code
 */
static uint16_t rfc_create_element_code(uint16_t address, bool byte_access, slimbus_drv_slice_size slice_size)
{
	/*
	 * Element Code
	 * 16 bit;
	 * [15: 4] element address
	 * [ 3: 3] access type (1 - byte based, 0 - elemental)
	 * [ 2: 0] value slice size
	 */
	return (address << 4) | ((unsigned int)byte_access << 3) | (slice_size);
}

static uint32_t rfc_check_event(void)
{
	uint32_t ret = 0;

	if (events.ie_ex_error)  {
		AUDIO_LOGE("Bus Reconfiguration failed, one or more devices reported Execution Error");
		ret = EIO;
	}
	if (events.ie_unsupported_msg)  {
		AUDIO_LOGE("Bus Reconfiguration failed, one or more devices reported Unsupported Message");
		ret = EIO;
	}
	if (events.ie_reconfig_objection)  {
		AUDIO_LOGE("Bus Reconfiguration failed, one or more devices reported Reconfig Objection");
		ret = EIO;
	}

	return ret;
}

/*
 * Event handlers
 * If handler is not used it should have NULL assigned to its pointer.
 */
static struct slimbus_drv_callbacks event_callbacks = {
	.on_assign_logical_address = assign_logical_address_handler,
	.on_device_class_request = device_class_handler,
	.on_message_received = received_message_handler,
	.on_message_sending = sending_message_handler,
	.on_information_element_reported = information_elements_handler,
	.on_data_port_interrupt = NULL,
	.on_manager_interrupt = manager_interrupts_handler,
	.on_raw_message_received = NULL,
	.on_raw_message_sending = NULL,
};

/*
 * Message handlers
 * If handler is not used it should have NULL assigned to its pointer.
 * Each handler corresponds with one Message that can be received by manager.
 * These handlers can be changed at any time via assignMessageHandlers function.
 */
static struct slimbus_message_callbacks message_callbacks = {
	.on_msg_report_present = msg_report_present_handler,
	.on_msg_report_absent = msg_report_absent_handler,
	.on_msg_reply_information = msg_reply_information_handler,
	.on_msg_report_information = msg_report_information_handler,
	.on_msg_reply_value = msg_reply_value,
};

/*
 * General configuration
 */
static slimbus_drv_config general_cfg = {
	.reg_base						= 0x0,
	.ea_instance_value				= 0x00,
	.ea_product_id					= 0xC100,
	.ea_interface_id				= 0x00,
	.ea_framer_id 					= 0x01,
	.ea_generic_id					= 0x02,
	.enumerate_devices				= 1,
	.enable_device					= 1,
	.enable_framer					= 1,
	.disable_hardware_crc_calculation	= 0,
	.sniffer_mode					= 0,
	.retry_limit 					= 4,
	.limit_reports					= 0,
	.report_at_event				= 0,
};

/*
 * Framer in Manager component configuration
 * If cfg.enable_framer was set to 0, this configuration may be skipped.
 * If cfg.enable_framer was set to 1 and configuration was skipped, then
 * IP's default values will be used.
 */
static slimbus_framer_config framer_cfg = {
	.root_frequencies_supported		= 0xFFFF,
	.quality						= FQ_LOW_JITTER,
	.pause_at_root_frequency_change = 0,
};

/*
 * Generic Device in Manager component configuration
 * If cfg.enable_device was set to 0, this configuration may be skipped
 * If cfg.enable_device was set to 1 and configuration was skipped, then
 * IP's default values will be used.
 */
static slimbus_generic_device_config device_cfg = {
	.presence_rates_supported 		= 0xFFFFFF,
	.reference_clock_selector 		= RC_CLOCK_GEAR_6,
	.transport_protocol_isochronous	= 1,
	.transport_protocol_pushed		= 1,
	.transport_protocol_pulled		= 1,
	.data_port_clock_prescaler 		= 2,
	.cport_clock_divider			= 0,
	.dma_treshold_sink				= 0x10,
	.dma_treshold_source			= 0x10,
	.sink_start_level 				= 0,
};

static int dev_preprocess(void)
{
	int ret;

	/* Initialize the driver and hardware */
	ret = slimbus_drv->init(devm_slimbus_priv, &general_cfg, &event_callbacks);
	if (ret) {
		AUDIO_LOGE("Init function failed with result: %d", ret);
		goto exit;
	}

	/* Assign Message Handlers */
	ret = slimbus_drv->assign_message_callbacks(devm_slimbus_priv, &message_callbacks);
	if (ret) {
		AUDIO_LOGE("assign_message_callbacks function failed with result: %d", ret);
		goto exit;
	}

	/* Set Framer Configuration */
	ret = slimbus_drv->set_framer_config(devm_slimbus_priv, &framer_cfg);
	if (ret) {
		AUDIO_LOGE("set_framer_config function failed with result: %d", ret);
		goto exit;
	}

	/* Set Generic Device Configuration */
	ret = slimbus_drv->set_generic_device_config(devm_slimbus_priv, &device_cfg);
	if (ret) {
		AUDIO_LOGE("set_generic_device_config function failed with result: %d", ret);
		goto exit;
	}

	/* Start the Driver and enable Manager and its interrupts */
	ret = slimbus_drv->start(devm_slimbus_priv);
	if (ret) {
		AUDIO_LOGE("Start function failed with result: %d", ret);
		goto exit;
	}

exit:
	return ret;
}

int slimbus_dev_init(enum platform_type platform_type)
{
	int ret;
	int wait_times = 0;
	uint32_t slimbus_dev_num = SOC_DEVICE_NUM;
	static uint32_t first_init;

	if (platform_type > PLATFORM_FPGA) {
		AUDIO_LOGE("invalid platform type: %d", platform_type);
		return -EINVAL;
	}

	mutex_lock(&slimbus_mutex);
	lostms_times = 0;

	rfc_clear_events();
	memset(&slimbus_devices, 0, sizeof(struct rfc_enumerated_devices));

	ret = dev_preprocess();
	if (ret)
		goto exit;

	first_init++;
	int_need_clear = false;

	/*
	 * Wait some time.
	 * SLIMBus works asynchronically to the application.
	 * During this delay devices will be enumerated using interrupts and handlers.
	 */

	/* Print number of devices which sent REPORT_PRESENCR message */
	if (platform_type != PLATFORM_FPGA)
		slimbus_dev_num = SOC_DEVICE_NUM + DA_COMBINE_DEVICE_NUM - ((first_init == 1) ? 1 : 0);

	if (first_init == 1) {
		/* only soc devices(3) can enumerate on armpc platform */
		AUDIO_LOGI("revise device number on pc platform when first init");
		slimbus_dev_num = SOC_DEVICE_NUM;
	}

	while ((slimbus_devices.devices < slimbus_dev_num) && (wait_times < ENUMERATION_TIMES)) {
		/* enumeration need 4ms normally, wait 40ms here is enough */
		usleep_range(100, 150);
		wait_times++;
	}

	AUDIO_LOGI("platype:%d, Discovered devices: %d, enumerated devices: %d",
		platform_type, events.reported_presence, slimbus_devices.devices);

	mutex_unlock(&slimbus_mutex);

	if ((slimbus_devices.devices <= SOC_DEVICE_NUM) && (slimbus_dev_num > SOC_DEVICE_NUM)) {
		slimbus_drv_reset_bus();
		/* wait for bus reset until device enumerated */
		wait_times = 0;
		while ((slimbus_devices.devices < slimbus_dev_num) && (wait_times < ENUMERATION_TIMES)) {
			/* enumeration need 4ms normally, wait 40ms here is enough */
			usleep_range(100, 150);
			wait_times++;
		}

		AUDIO_LOGI("After busreset, Discovered devices: %d, enumerated devices: %d",
			events.reported_presence, slimbus_devices.devices);
		if (wait_times == ENUMERATION_TIMES)
			AUDIO_LOGE("retry fail, please check chip is connected or not");
	}
	slimbus_dev_lostms_recover("slimbus init\n");

	return ret;

exit:
	slimbus_dump_state(SLIMBUS_DUMP_LOSTMS);
	mutex_unlock(&slimbus_mutex);
	return ret;
}

static int resource_init(enum platform_type platform_type,
	const void *slimbus_reg, const void *asp_reg, const void *sctrl_reg)
{
	mutex_init(&slimbus_mutex);
	spin_lock_init(&slimbus_spinlock);
	init_completion(&(internal_reply.read_finish));
	init_completion(&(internal_reply.request_finish));
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,4,0))
	slimbus_wake_lock = wakeup_source_register(NULL, "slimbus_wake_lock");
#else
	slimbus_wake_lock = wakeup_source_register("slimbus_wake_lock");
#endif
	if (slimbus_wake_lock == NULL)
		return -ENOMEM;

	general_cfg.reg_base = (uintptr_t)slimbus_reg;
	asp_base_reg = (uintptr_t)asp_reg;
	sctrl_base_addr = (uintptr_t)sctrl_reg;
	plat_type = platform_type;

	return 0;
}

static int drv_preprocess(void)
{
	uint16_t slimbus_priv_obj_size = 0;
	int ret;

	/* Get API function pointers for this Driver instance */
	slimbus_drv = slimbus_drv_get_instance();

	/* Probe driver for required memory */
	ret = slimbus_drv->probe(&general_cfg, &slimbus_priv_obj_size);
	if (ret) {
		AUDIO_LOGE("Probe function failed with result: %d", ret);
		goto exit;
	}

	if (slimbus_priv_obj_size == 0) {
		AUDIO_LOGE("slimbus prve obj size is zero");
		ret = -ENOMEM;
		goto exit;
	}

	/* Allocation of memory required by the driver */
	devm_slimbus_priv = kmalloc(slimbus_priv_obj_size, GFP_KERNEL);
	if (!devm_slimbus_priv) {
		AUDIO_LOGE("malloc private data failed");
		ret = -ENOMEM;
		goto exit;
	}

exit:
	return ret;
}

int slimbus_drv_init(enum platform_type platform_type, const void *slimbus_reg,
		const void *asp_reg, const void *sctrl_reg, int irq)
{
	int ret;

	if (platform_type > PLATFORM_FPGA) {
		AUDIO_LOGE("invalid platform type: %d", platform_type);
		return -EINVAL;
	}

	if (!slimbus_reg || !asp_reg) {
		AUDIO_LOGE("reg is null");
		return -EINVAL;
	}

	ret = resource_init(platform_type, slimbus_reg, asp_reg, sctrl_reg);
	if (ret != 0) {
		AUDIO_LOGE("resource init failed: %d", ret);
		goto exit;
	}

	ret = drv_preprocess();
	if (ret)
		goto exit;

	/* Enable SLIMbus interrupt */
	ret = request_irq(irq, rfc_irq_handler, IRQF_TRIGGER_HIGH | IRQF_SHARED | IRQF_NO_SUSPEND,
		"asp_irq_slimbus", devm_slimbus_priv);
	if (ret < 0) {
		AUDIO_LOGE("could not claim irq: %d", ret);
		ret = -ENODEV;
		goto request_irq_failed;
	}

	ret = slimbus_dev_init(platform_type);
	if (ret) {
		ret = -ENODEV;
		goto dev_init_failed;
	}

	slimbus_lost_sync_wq = create_singlethread_workqueue("slimbus_lost_sync_wq");
	if (!slimbus_lost_sync_wq) {
		AUDIO_LOGE("work queue create failed");
		goto request_irq_failed;
	}
	INIT_DELAYED_WORK(&slimbus_lost_sync_delay_work, slimbus_lost_sync_cb);

	return 0;

dev_init_failed:
	free_irq(irq, devm_slimbus_priv);
request_irq_failed:
	kfree(devm_slimbus_priv);
	devm_slimbus_priv = NULL;
exit:
	mutex_destroy(&slimbus_mutex);
	wakeup_source_unregister(slimbus_wake_lock);
	slimbus_wake_lock = NULL;

	return ret;
}

int slimbus_drv_stop(void)
{
	int result;

	mutex_lock(&slimbus_mutex);

	result = slimbus_drv->stop(devm_slimbus_priv);
	if (result != 0)
		AUDIO_LOGE("SLIMbus stop failed with result: %d", result);

	mutex_unlock(&slimbus_mutex);
	return result;
}

int slimbus_drv_release(int irq)
{
	free_irq(irq, devm_slimbus_priv);
	mutex_destroy(&slimbus_mutex);
	wakeup_source_unregister(slimbus_wake_lock);
	slimbus_wake_lock = NULL;

	if (slimbus_lost_sync_wq) {
		cancel_delayed_work(&slimbus_lost_sync_delay_work);
		flush_workqueue(slimbus_lost_sync_wq);
		destroy_workqueue(slimbus_lost_sync_wq);
	}

	if (devm_slimbus_priv != NULL) {
		kfree(devm_slimbus_priv);
		devm_slimbus_priv = NULL;
	}

	return 0;
}

int slimbus_drv_bus_configure(const struct slimbus_bus_config *bus_config)
{
	int ret = 0;
	slimbus_drv_subframe_mode sm;
	slimbus_drv_root_frequency rf;
	slimbus_drv_clock_gear cg;

	if (!bus_config) {
		AUDIO_LOGE("bus config is null");
		return -EINVAL;
	}

	mutex_lock(&slimbus_mutex);

	rfc_clear_events();

	/* configure bus */
	ret += slimbus_drv->msg_begin_reconfiguration(devm_slimbus_priv);
	ret += slimbus_drv->msg_next_clock_gear(devm_slimbus_priv, (slimbus_drv_clock_gear)bus_config->cg);
	ret += slimbus_drv->msg_next_subframe_mode(devm_slimbus_priv, (slimbus_drv_subframe_mode)bus_config->sm);
	ret += slimbus_drv->msg_reconfigure_now(devm_slimbus_priv);
	if (ret) {
		AUDIO_LOGE("Bus reconfiguration failed with error: %d", ret);
		goto exit;
	}

	/* wait for param update */
	if (plat_type == PLATFORM_FPGA)
		usleep_range(10000, 10050);
	else
		usleep_range(1000, 1050);

	slimbus_drv->get_status_slimbus(devm_slimbus_priv, &sm, &cg, &rf);

	if ((sm != (slimbus_drv_subframe_mode)bus_config->sm)
		|| (rf != (slimbus_drv_root_frequency)bus_config->rf)
		|| (cg != (slimbus_drv_clock_gear)bus_config->cg)) {
		AUDIO_LOGE("Bus Reconfiguration failed, ret: %d", ret);
		ret = EINVAL;
	}

exit:
	mutex_unlock(&slimbus_mutex);

	return ret;
}

int slimbus_drv_request_info(uint8_t target_la, uint16_t address, enum slimbus_slice_size slice_size, void *value_read)
{
	int ret;
	uint16_t element_code;

	if (!value_read) {
		AUDIO_LOGE("input value is null");
		return -EINVAL;
	}

	if (slice_size >= SLIMBUS_SS_SLICE_BUT) {
		AUDIO_LOGE("slice size is invalid, slice_size:%d", slice_size);
		return -EINVAL;
	}

	mutex_lock(&slimbus_mutex);
	internal_reply.received = 0;
	internal_reply.transaction_id += 1;
	internal_reply.ptr = value_read;
	internal_reply.information_requested = 1;

	reinit_completion(&internal_reply.request_finish);
	/* create element code set byte address mode */
	element_code = rfc_create_element_code(address, 1, (slimbus_drv_slice_size)slice_size);

	rfc_clear_events();
	mb();
	ret = slimbus_drv->msg_request_information(devm_slimbus_priv,
		0, target_la, internal_reply.transaction_id, element_code);
	if (ret) {
		slimbus_drv_limit_err("Sending REQUEST_INFORMATION failed with ret: %d, ID:%#x !\n",
			ret, internal_reply.transaction_id);
		goto exit;
	}

	/* Wait for reply (received using callback) */
	if (wait_for_completion_timeout(&(internal_reply.request_finish), msecs_to_jiffies(500)) == 0) {
		slimbus_drv_limit_err("REQUEST_INFORMATION Message timeout: received:%#x ID:%#x slimbus_irq_state:%#x !\n",
			internal_reply.received, internal_reply.transaction_id, slimbus_irq_state);
		ret = EIO;
		goto exit;
	}

	if (internal_reply.error) {
		internal_reply.error = 0;
		internal_reply.information_requested = 0;

		slimbus_drv_limit_err("REQUEST_INFORMATION Message error\n");
		ret = EIO;
	}

exit:
	mutex_unlock(&slimbus_mutex);

	return ret;
}

static void dump_irq(void)
{
	uint32_t state[7] = {0};
	uintptr_t gic_base_addr;

	state[0] = readl((void __iomem *)(general_cfg.reg_base + 0x3c));
	state[1] = readl((void __iomem *)(general_cfg.reg_base + 0x24));
	gic_base_addr = (uintptr_t)ioremap(GIC_BASE_ADDR, GIC_SIZE);
	if (gic_base_addr == 0) {
		AUDIO_LOGE("ioremap failed");
		return;
	}

	state[2] = readl((void __iomem *)(gic_base_addr + 0x1214));
	state[3] = readl((void __iomem *)(gic_base_addr + 0x1314));
	state[4] = readl((void __iomem *)(gic_base_addr + 0x1414));
	state[5] = readl((void __iomem *)(gic_base_addr + 0x2014));
	state[6] = readl((void __iomem *)(gic_base_addr + 0x2018));
	iounmap((void __iomem *)gic_base_addr);
	gic_base_addr = (uintptr_t)NULL;
	slimbus_drv_limit_err("0x3c:%#x 0X24:%#x; gic:(%#x,%#x,%#x,%#x,%#x)!\n",
		state[0], state[1], state[2], state[3], state[4], state[5], state[6]);
}

static void dump_framer(void)
{
	uint8_t framersoc = 0;
	uint8_t framercodec = 0;
	uint32_t state = readl((void __iomem *)(general_cfg.reg_base + 0x28));

	slimbus_drv_request_info(0x20, SLIMBUS_DRV_ADDRESS, SLIMBUS_SS_1_BYTE, &framersoc);
	slimbus_drv_request_info(0x21, SLIMBUS_DRV_ADDRESS, SLIMBUS_SS_1_BYTE, &framercodec);
	AUDIO_LOGI("0x28:%#x, framer(0x20):%#x, framer(0x21):%#x", state, framersoc, framercodec);
}

static void dump_lostms(void)
{
	uint32_t state[13] = {0};

	state[0] = readl((void __iomem *)(general_cfg.reg_base + 0x0));
	state[1] = readl((void __iomem *)(general_cfg.reg_base + 0x20));
	state[2] = readl((void __iomem *)(general_cfg.reg_base + 0x24));
	state[3] = readl((void __iomem *)(general_cfg.reg_base + 0x28));
	state[4] = readl((void __iomem *)(asp_base_reg + 0x8));
	state[5] = readl((void __iomem *)(asp_base_reg + 0x14));
	state[6] = readl((void __iomem *)(asp_base_reg + 0x18));
	state[7] = readl((void __iomem *)(asp_base_reg + 0x20));
	state[8] = readl((void __iomem *)(asp_base_reg + 0x38));
	state[9] = readl((void __iomem *)(asp_base_reg + 0x1c));
	state[10] = readl((void __iomem *)(asp_base_reg + 0x100));
	state[11] = readl((void __iomem *)(sctrl_base_addr + 0x19c));
	state[12] = readl((void __iomem *)(sctrl_base_addr + 0x260));

	slimbus_drv_limit_err("0x0:%#x, 0x20:%#x, 0x24:%#x, 0x28:%#x\n", state[0], state[1], state[2], state[3]);
	slimbus_drv_limit_err("0xe008:%#x, 0xe014:%#x, 0xe018:%#x, 0xe020:%#x, 0xe038:%#x;\n",
		state[4], state[5], state[6], state[7], state[8]);
	slimbus_drv_limit_err("0xe01c:%#x, 0xe100:%#x, 0x19c:%#x, 0x260:%#x;\n",
		state[9], state[10], state[11], state[12]);
}

static void dump_all(void)
{
	uint32_t state[4] = {0};
	uint32_t i;

	slimbus_drv_limit_err("dump all slimbus regs\n");
	for (i = 0; i < DUMP_SLIMBUS_REGS_SIZE; i += 0x10) {
		state[0] = readl((void __iomem *)(general_cfg.reg_base + i));
		state[1] = readl((void __iomem *)(general_cfg.reg_base + i + 4));
		state[2] = readl((void __iomem *)(general_cfg.reg_base + i + 8));
		state[3] = readl((void __iomem *)(general_cfg.reg_base + i + 0xc));
		slimbus_drv_limit_err("0x%x - 0x%x: %#x, %#x, %#x, %#x\n",
			i, i + 0xC, state[0], state[1], state[2], state[3]);
	}
}


static void slimbus_dump_state(enum slimbus_dump_state type)
{
	if (plat_type == PLATFORM_FPGA) {
		slimbus_drv_limit_err("fpga platform, don't dump register!\n");
		return;
	}

	switch (type) {
	case SLIMBUS_DUMP_IRQ:
		dump_irq();
		break;
	case SLIMBUS_DUMP_FRAMER:
		dump_framer();
		break;
	case SLIMBUS_DUMP_LOSTMS:
		dump_lostms();
		break;
	case SLIMBUS_DUMP_ALL:
		dump_all();
		break;
	default:
		AUDIO_LOGE("default");
		break;
	}
}

spinlock_t *slimbus_drv_get_spinlock(void)
{
	return &slimbus_spinlock;
}

/*
 * Slimbus Element read
 * @param[in] address 16 bits element address
 * @param[in] slicesize size of slize
 * @param[in] value_read value of the address
 * @return value of element address
 */
int slimbus_drv_element_read(uint8_t target_la, uint16_t address, enum slimbus_slice_size slice_size, void *value_read)
{
	int ret;
	uint16_t element_code;

	if (!value_read) {
		AUDIO_LOGE("input value is null");
		return -EINVAL;
	}

	if (slice_size >= SLIMBUS_SS_SLICE_BUT) {
		AUDIO_LOGE("slice size is invalid, slice_size:%d\n", slice_size);
		return -EINVAL;
	}

	mutex_lock(&slimbus_mutex);

	/*lint -e666*/
	if (abs(get_timeus() - last_time) < REG_ACCESS_TIME_DIFF)
		udelay(2);

	/*lint +e666*/
	internal_reply.received = 0;
	internal_reply.transaction_id += 1;
	internal_reply.value_requested = 1;
	internal_reply.ptr = value_read;
	slimbus_irq_state = 0;

	reinit_completion(&internal_reply.read_finish);
	/* create element code set byte address mode */
	element_code = rfc_create_element_code(address, 1, (slimbus_drv_slice_size)slice_size);

	rfc_clear_events();
	mb();
	ret = slimbus_drv->msg_request_value(devm_slimbus_priv, 0, target_la, internal_reply.transaction_id, element_code);
	if (ret) {
		slimbus_drv_limit_err("Sending REQUEST_VALUE failed with ret: %d ID:%#x slimbus_irq_state:%#x \n",
			ret, internal_reply.transaction_id, slimbus_irq_state);
		slimbus_dump_state(SLIMBUS_DUMP_IRQ);
		goto exit;
	}

	if (wait_for_completion_timeout(&(internal_reply.read_finish), msecs_to_jiffies(500)) == 0) {
		slimbus_drv_limit_err("REQUEST_VALUE Message timeout: received:%#x ID:%#x slimbus_irq_state:%#x !\n",
			internal_reply.received, internal_reply.transaction_id, slimbus_irq_state);
		slimbus_dump_state(SLIMBUS_DUMP_IRQ);
		ret = EIO;
		goto exit;
	}

	if (internal_reply.error) {
		internal_reply.error = 0;
		internal_reply.value_requested = 0;

		slimbus_drv_limit_err("REQUEST_VALUE Message error\n");
		ret = EIO;
	} else {
		slimbus_dev_lostms_recover("slimbus recover!\n");
	}

exit:
	last_time = get_timeus();
	mutex_unlock(&slimbus_mutex);

	return ret;
}

/*
 * Slimbus Element write
 * @param[in] address 16 bits element address
 * @param[in] value_write value write to element address
 * @param[in] slicesize size of slize
 * @return
 */
int slimbus_drv_element_write(uint8_t target_la, uint16_t address,
	enum slimbus_slice_size slice_size, void *value_write)
{
	int ret;
	uint16_t element_code;
	uint32_t value_size[8] = { 1, 2, 3, 4, 6, 8, 12, 16 };

	if (!value_write) {
		AUDIO_LOGE("input value is null");
		return -EINVAL;
	}

	if (slice_size >= SLIMBUS_SS_SLICE_BUT) {
		AUDIO_LOGE("slice size is invalid, slice_size:%d", slice_size);
		return -EINVAL;
	}

	mutex_lock(&slimbus_mutex);

	/*lint -e666*/
	if (abs(get_timeus() - last_time) < REG_ACCESS_TIME_DIFF)
		udelay(2);

	/*lint +e666*/
	rfc_clear_events();

	element_code = rfc_create_element_code(address, 1, (slimbus_drv_slice_size)slice_size);

	ret = slimbus_drv->msg_change_value(devm_slimbus_priv,
		0, target_la, element_code, (uint8_t *)value_write, value_size[slice_size]);
	if (ret)
		slimbus_drv_limit_err("Sending CHANGE_VALUE failed with ret: %d", ret);
	else
		slimbus_dev_lostms_recover("slimbus recover");

	last_time = get_timeus();

	mutex_unlock(&slimbus_mutex);

	return ret;
}

static int slimbus_drv_connect_track(const struct slimbus_channel_property *channel, uint32_t ch_num)
{
	int ret = 0;
	uint32_t i;
	uint32_t j;

	if (!channel || (ch_num == 0))
		return ret;

	/* connect the active source and sink */
	for (i = 0; i < ch_num; i++) {
		/* Connecting Source to Data Channel */

		ret += slimbus_drv->msg_connect_source(devm_slimbus_priv,
			channel[i].source.la,
			channel[i].source.pn,
			channel[i].cn);

		/* Connecting Sink(s) to Data Channel */
		for (j = 0; j < channel[i].sink_num; j++)
			ret += slimbus_drv->msg_connect_sink(devm_slimbus_priv,
				channel[i].sinks[j].la,
				channel[i].sinks[j].pn,
				channel[i].cn);

		if (ret)
			AUDIO_LOGE("Connecting source or sink(s) failed, i= %d", ch_num);
	}

	return ret;
}

int slimbus_drv_track_activate(const struct slimbus_channel_property *channel, uint32_t ch_num)
{
	int ret;
	uint32_t i;
	slimbus_drv_subframe_mode cur_sm;
	slimbus_drv_root_frequency cur_rf;
	slimbus_drv_clock_gear cur_cg;
	unsigned long delay_time = 2000; /* 2000us */

	if (!channel) {
		AUDIO_LOGE("channel is null");
		return -EINVAL;
	}

	mutex_lock(&slimbus_mutex);

	/* configure channels */
	rfc_clear_events();

	ret = slimbus_drv_connect_track(channel, ch_num);
	if (ret) {
		AUDIO_LOGE("Connecting source or sink(s) failed");
		goto exit;
	}

	/* Configure and define Channels */
	ret = slimbus_drv->msg_begin_reconfiguration(devm_slimbus_priv);
	if (ret) {
		AUDIO_LOGE("Sending BEGIN_RECONFIGURATION failed with ret: %d", ret);
		goto exit;
	}

	for (i = 0; i < ch_num; i++) {
		/* Configuring Data Channel */
		ret += slimbus_drv->msg_next_define_channel(devm_slimbus_priv,
			channel[i].cn,
			(slimbus_drv_transport_protocol)channel[i].tp,
			channel[i].sd,
			channel[i].sl);
		ret += slimbus_drv->msg_next_define_content(devm_slimbus_priv,
			channel[i].cn, channel[i].fl,
			(slimbus_drv_presence_rate)channel[i].pr,
			(slimbus_drv_aux_field_format)channel[i].af,
			(slimbus_drv_data_type)channel[i].dt,
			channel[i].cl,
			channel[i].dl);
		/* Activating Data Channel */
		ret += slimbus_drv->msg_next_activate_channel(devm_slimbus_priv, channel[i].cn);

		AUDIO_LOGI("i %d, cn %d, sl %d, sd 0x%x, pr %d, dl %d, tp %d", i, channel[i].cn, channel[i].sl,
			channel[i].sd, channel[i].pr, channel[i].dl, channel[i].tp);
	}
	if (ret) {
		AUDIO_LOGE("Configuring and Activating Data Channels failed");
		goto exit;
	}

	ret = slimbus_drv->msg_reconfigure_now(devm_slimbus_priv);
	if (ret) {
		AUDIO_LOGE("Sending RECONFIGURE_NOW failed with ret: %d", ret);
		goto exit;
	}

	if (!slimbus_drv->get_status_slimbus(devm_slimbus_priv, &cur_sm, &cur_cg, &cur_rf))
		delay_time = (1 << (CG_10 - cur_cg)) * 300;

	udelay(delay_time);

	if (rfc_check_event())
		AUDIO_LOGE("Channel configuration failed");

exit:
	mutex_unlock(&slimbus_mutex);
	return ret;
}

int slimbus_drv_track_deactivate(const struct slimbus_channel_property *channel, uint32_t ch_num)
{
	int ret;
	uint32_t i;
	slimbus_drv_subframe_mode cur_sm;
	slimbus_drv_root_frequency cur_rf;
	slimbus_drv_clock_gear cur_cg;
	unsigned long delay_time = 2000; /* 2000us */

	if (!channel) {
		AUDIO_LOGE("channel is null");
		return -EINVAL;
	}

	mutex_lock(&slimbus_mutex);

	rfc_clear_events();

	/* Configure and remove Channels */
	ret = slimbus_drv->msg_begin_reconfiguration(devm_slimbus_priv);
	if (ret) {
		AUDIO_LOGE("Sending BEGIN_RECONFIGURATION failed with ret: %d", ret);
		goto exit;
	}

	for (i = 0; i < ch_num; i++) {
		/* Deactivating Data Channel */
		ret += slimbus_drv->msg_next_deactivate_channel(devm_slimbus_priv, channel[i].cn);

		/* remove Data Channel */
		ret += slimbus_drv->msg_next_remove_channel(devm_slimbus_priv, channel[i].cn);
	}
	if (ret) {
		AUDIO_LOGE("Configuring and remove Data Channels failed");
		goto exit;
	}

	slimbus_drv->msg_reconfigure_now(devm_slimbus_priv);
	if (ret) {
		AUDIO_LOGE("Sending RECONFIGURE_NOW failed with ret: %d", ret);
		goto exit;
	}

	if (!slimbus_drv->get_status_slimbus(devm_slimbus_priv, &cur_sm, &cur_cg, &cur_rf))
		delay_time = (1 << (CG_10 - cur_cg)) * 300;

	udelay(delay_time);

	if (rfc_check_event())
		AUDIO_LOGE("Channel removed failed");

exit:
	mutex_unlock(&slimbus_mutex);

	return ret;
}

int slimbus_drv_switch_framer(uint8_t laif, uint16_t nco, uint16_t nci, const struct slimbus_bus_config *bus_config)
{
	int ret = 0;
	static int8_t last_laif;
	slimbus_drv_subframe_mode cur_sm;
	slimbus_drv_root_frequency cur_rf;
	slimbus_drv_clock_gear cur_cg;
	unsigned long delay_time = 2000; /* 2000us */

	if (!bus_config) {
		AUDIO_LOGE("bus config is null");
		return -EINVAL;
	}

	mutex_lock(&slimbus_mutex);

	if (laif != last_laif)	{
		AUDIO_LOGI("la:0x%x, nco:0x%x, nci:0x%x cg:0x%x", laif, nco, nci, bus_config->cg);
		rfc_clear_events();
		ret += slimbus_drv->msg_begin_reconfiguration(devm_slimbus_priv);
		if (laif == slimbus_devices.framer_logical_addr[SLIMBUS_FRAMER_DA_COMBINE_ID])
			ret += slimbus_drv->msg_next_clock_gear(devm_slimbus_priv, (slimbus_drv_clock_gear)bus_config->cg);
		ret += slimbus_drv->msg_next_active_framer(devm_slimbus_priv, laif, nco, nci);
		if (laif == slimbus_devices.framer_logical_addr[SLIMBUS_FRAMER_SOC_ID])
			ret += slimbus_drv->msg_next_clock_gear(devm_slimbus_priv, (slimbus_drv_clock_gear)bus_config->cg);
		ret += slimbus_drv->msg_reconfigure_now(devm_slimbus_priv);
		if (ret) {
			AUDIO_LOGE("Bus switch framer failed with error: %d", ret);
			goto exit;
		}

		/* param would be updated in one superframe, 250us */
		if (!slimbus_drv->get_status_slimbus(devm_slimbus_priv, &cur_sm, &cur_cg, &cur_rf))
			delay_time = (1 << (CG_10 - cur_cg)) * 300;

		udelay(delay_time);

		last_laif = laif;
	} else {
		AUDIO_LOGI("switch to the same framer la:0x%x", laif);
	}

exit:
	mutex_unlock(&slimbus_mutex);

	return ret;
}

int slimbus_drv_pause_clock(enum slimbus_restart_time time)
{
	int ret = 0;
	slimbus_drv_subframe_mode cur_sm;
	slimbus_drv_root_frequency cur_rf;
	slimbus_drv_clock_gear cur_cg;
	unsigned long delay_time = 2000; /* 2000us */

	mutex_lock(&slimbus_mutex);

	/* make sure boundary of pause_clock is clean */
	udelay(300);
	rfc_clear_events();
	ret += slimbus_drv->msg_begin_reconfiguration(devm_slimbus_priv);
	ret += slimbus_drv->msg_next_pause_clock(devm_slimbus_priv, (slimbus_drv_restart_time)time);
	ret += slimbus_drv->msg_reconfigure_now(devm_slimbus_priv);
	if (ret)
		AUDIO_LOGE("Bus switch framer failed with error: %d", ret);

	if (!slimbus_drv->get_status_slimbus(devm_slimbus_priv, &cur_sm, &cur_cg, &cur_rf))
		delay_time = (1 << (CG_10 - cur_cg)) * 300;

	udelay(delay_time);

	mutex_unlock(&slimbus_mutex);

	return ret;
}

int slimbus_drv_resume_clock(void)
{
	int ret;
	mutex_lock(&slimbus_mutex);

	ret = slimbus_drv->unfreeze(devm_slimbus_priv);
	if (ret)
		AUDIO_LOGE("unfreeze clock failed with error: %d", ret);

	lostms_times = 0;
	slimbus_rdwrerr_times = 0;

	mutex_unlock(&slimbus_mutex);

	return ret;
}

int slimbus_drv_reset_bus(void)
{
	int ret = 0;
	slimbus_drv_subframe_mode cur_sm;
	slimbus_drv_root_frequency cur_rf;
	slimbus_drv_clock_gear cur_cg;
	unsigned long delay_time = 2000; /* 2000us */

	mutex_lock(&slimbus_mutex);

	rfc_clear_events();

	memset(&slimbus_devices, 0, sizeof(struct rfc_enumerated_devices));

	ret += slimbus_drv->msg_begin_reconfiguration(devm_slimbus_priv);
	ret += slimbus_drv->msg_next_reset_bus(devm_slimbus_priv);
	ret += slimbus_drv->msg_reconfigure_now(devm_slimbus_priv);
	if (ret)
		AUDIO_LOGE("Reset bus failed with error: %d", ret);

	if (!slimbus_drv->get_status_slimbus(devm_slimbus_priv, &cur_sm, &cur_cg, &cur_rf))
		delay_time = (1 << (CG_10 - cur_cg)) * 300;

	udelay(delay_time);

	mutex_unlock(&slimbus_mutex);

	return ret;
}

int slimbus_drv_shutdown_bus(void)
{
	int ret = 0;
	slimbus_drv_subframe_mode cur_sm;
	slimbus_drv_root_frequency cur_rf;
	slimbus_drv_clock_gear cur_cg;
	unsigned long delay_time = 2000; /* 2000us */

	mutex_lock(&slimbus_mutex);

	rfc_clear_events();

	ret += slimbus_drv->msg_begin_reconfiguration(devm_slimbus_priv);
	ret += slimbus_drv->msg_next_shutdown_bus(devm_slimbus_priv);
	ret += slimbus_drv->msg_reconfigure_now(devm_slimbus_priv);
	if (ret)
		AUDIO_LOGE("Shutdown bus failed with error: %d", ret);

	if (!slimbus_drv->get_status_slimbus(devm_slimbus_priv, &cur_sm, &cur_cg, &cur_rf))
		delay_time = (1 << (CG_10 - cur_cg)) * 300;

	udelay(delay_time);

	mutex_unlock(&slimbus_mutex);

	return ret;
}

uint8_t slimbus_drv_get_framerla(int framer_id)
{
	uint8_t la = 0;

	if (framer_id < slimbus_devices.framer_cnt) {
		la = slimbus_devices.framer_logical_addr[framer_id];
	} else {
		if (slimbus_devices.framer_cnt > 0)
			la = slimbus_devices.framer_logical_addr[0];
		AUDIO_LOGI("wrong framer_id ! framer_id is %d, framerCount is %d. la:%d", framer_id, slimbus_devices.framer_cnt, la);
	}

	return la;
}

static uint32_t set_presence_rate_generation(bool flag)
{
	uint32_t i;
	uint32_t ret = 0;

	for (i = 0; i < SLIMBUS_PORT_NUM; i++)
		ret += slimbus_drv->set_presence_rate_generation(devm_slimbus_priv, i, flag);

	return ret;
}

static uint32_t set_presence_rate_generation_by_tp(enum slimbus_transport_protocol tp)
{
	bool flag = tp == SLIMBUS_TP_PUSHED ? true : false;

	return set_presence_rate_generation(flag);
}

static int start_channel(struct slimbus_device_info *dev, struct slimbus_channel_property *channel,
		int track, uint32_t ch_num, unsigned long *msg_count)
{
	struct slimbus_channel_property *active_channel = NULL;
	uint32_t active_ch_num = 0;
	enum slimbus_presence_rate rate;
	enum slimbus_transport_protocol tp;
	uint32_t ret = 0;
	uint32_t i;
	uint32_t j;

	for (i = 0; i < dev->slimbus_da_combine_para->slimbus_track_max; i++) {
		active_channel = NULL;
		active_ch_num = 0;

		if (slimbus_track_state_is_on(i)) {
			active_channel = dev->slimbus_da_combine_para->track_config_table[i].channel_pro;
			active_ch_num = dev->slimbus_da_combine_para->track_config_table[i].params.channels;
		} else if ((i == track) && (channel != NULL)) {
			active_channel = channel;
			active_ch_num = ch_num;
		}

		if (active_channel != NULL) {
			for (j = 0; j < active_ch_num; j++) {
				/* Configuring Data Channel */
				ret += slimbus_drv->msg_next_define_channel(devm_slimbus_priv,
					active_channel[j].cn,
					(slimbus_drv_transport_protocol)active_channel[j].tp,
					active_channel[j].sd,
					active_channel[j].sl);
				ret += slimbus_drv->msg_next_define_content(devm_slimbus_priv,
					active_channel[j].cn,
					active_channel[j].fl,
					(slimbus_drv_presence_rate)active_channel[j].pr,
					(slimbus_drv_aux_field_format)active_channel[j].af,
					(slimbus_drv_data_type)active_channel[j].dt,
					active_channel[j].cl,
					active_channel[j].dl);
				rate = active_channel[j].pr;
				tp = active_channel[j].tp;

				ret += set_presence_rate_generation_by_tp(tp);
				/* Activating Data Channel */
				ret += slimbus_drv->msg_next_activate_channel(devm_slimbus_priv, active_channel[j].cn);
				*msg_count = *msg_count + 3 + SLIMBUS_PORT_NUM;

				AUDIO_LOGI("j %d, cn %d, sl %d, sd 0x%x, pr %d, dl:%d, tp:%d, track:%d\n", j,
						active_channel[j].cn, active_channel[j].sl, active_channel[j].sd, active_channel[j].pr,
						active_channel[j].dl, active_channel[j].tp, i);
			}
			if (ret)
				AUDIO_LOGE("Configuring and Activating Data Channels i: %d, j: %d failed: %d", i, j, ret);
		}
	}
	return ret;
}

int slimbus_drv_track_update(int cg, int sm, int track, struct slimbus_device_info *dev,
		uint32_t ch_num, struct slimbus_channel_property *channel)
{
	uint32_t ret;
	slimbus_drv_subframe_mode cur_sm = (slimbus_drv_subframe_mode)0;
	slimbus_drv_root_frequency cur_rf = (slimbus_drv_root_frequency)0;
	slimbus_drv_clock_gear cur_cg = (slimbus_drv_clock_gear)0;
	unsigned long msg_count = 0;

	if (!dev) {
		AUDIO_LOGE("invalid input params");
		return -EINVAL;
	}

	mutex_lock(&slimbus_mutex);
	rfc_clear_events();

	AUDIO_LOGI("track %d, cg %d", track, cg);
	ret = slimbus_drv_connect_track(channel, ch_num);
	if (ret) {
		slimbus_dump_state(SLIMBUS_DUMP_ALL);
		AUDIO_LOGE("Connecting source or sink(s) failed");
		goto exit;
	}

	slimbus_drv->get_status_slimbus(devm_slimbus_priv, &cur_sm, &cur_cg, &cur_rf);

	ret = slimbus_drv->msg_begin_reconfiguration(devm_slimbus_priv);
	msg_count++;
	if (cur_cg != (slimbus_drv_clock_gear)cg) {
		ret = slimbus_drv->msg_next_clock_gear(devm_slimbus_priv, cg);
		msg_count++;
		AUDIO_LOGI("cg change return: %d", ret);
	}

	if (cur_sm != (slimbus_drv_subframe_mode)sm) {
		ret = slimbus_drv->msg_next_subframe_mode(devm_slimbus_priv, sm);
		msg_count++;
	}

	ret = start_channel(dev, channel, track, ch_num, &msg_count);
	if (ret) {
		AUDIO_LOGE("Configuring and Activating Data Channels failed");
		goto exit;
	}

	ret += slimbus_drv->msg_reconfigure_now(devm_slimbus_priv);
	msg_count++;
	if (ret)
		AUDIO_LOGE("cg change failed with error: %d", ret);

exit:
	if (cur_cg == 8)
		udelay(500 * msg_count + 2000);
	else
		udelay(250 * msg_count + 500);

	slimbus_drv->get_status_slimbus(devm_slimbus_priv, &cur_sm, &cur_cg, &cur_rf);

	if (cur_cg != cg) {
		AUDIO_LOGE("cur_sm %d, cur_cg %d, cg %d, cur_rf %d", cur_sm, cur_cg, cg, cur_rf);
		slimbus_dump_state(SLIMBUS_DUMP_ALL);
		slimbus_dump_state(SLIMBUS_DUMP_LOSTMS);
#ifdef CONFIG_DFX_BB
		rdr_system_error(RDR_AUDIO_SLIMBUS_LOSTSYNC_MODID, 0, 0);
#endif
	}
	mutex_unlock(&slimbus_mutex);

	return (int)ret;
}

void slimbus_drv_get_params_la(int track_type, uint8_t *source_la,
	uint8_t *sink_la, enum slimbus_transport_protocol *tp)
{
	if (!source_la || !sink_la || !tp) {
		AUDIO_LOGE("invalid input params");
		return;
	}

	switch (track_type) {
	case DA_COMBINE_V3_BUS_TRACK_AUDIO_PLAY:
	case DA_COMBINE_V3_BUS_TRACK_DIRECT_PLAY:
	case DA_COMBINE_V3_BUS_TRACK_FAST_PLAY:
	case DA_COMBINE_V3_BUS_TRACK_VOICE_DOWN:
		break;

	case DA_COMBINE_V3_BUS_TRACK_AUDIO_CAPTURE:
	case DA_COMBINE_V3_BUS_TRACK_VOICE_UP:
	case DA_COMBINE_V3_BUS_TRACK_ECREF:
	case DA_COMBINE_V3_BUS_TRACK_SOUND_TRIGGER:
	case DA_COMBINE_V3_BUS_TRACK_DEBUG:
	case DA_COMBINE_V3_BUS_TRACK_KWS:
		*source_la = DA_COMBINE_LA_GENERIC_DEVICE;
		*sink_la = SOC_LA_GENERIC_DEVICE;
		break;

	case DA_COMBINE_V3_BUS_TRACK_IMAGE_LOAD:
		*tp = SLIMBUS_TP_PUSHED;
		break;

	default:
		AUDIO_LOGE("track type is invalid: %d", track_type);
		return;
	}
}

int slimbus_wait_codec_device_enum(void)
{
	int wait_times = 0;
	int expect_devices = SOC_DEVICE_NUM + DA_COMBINE_DEVICE_NUM - 1;
	if (slimbus_devices.devices < expect_devices) {
		/* if current device count is less than expected, delay more */
		/* enumeration need 4ms */
		AUDIO_LOGI("delay 4ms if device count is less than expected");
		udelay(ENUMERATION_TIMES * 10);
	}

	AUDIO_LOGI("After delay, enumerated devices: %d", slimbus_devices.devices);
	if (slimbus_devices.devices < expect_devices) {
		slimbus_drv_reset_bus();
		/* wait for bus reset until device enumerated */
		while ((slimbus_devices.devices < expect_devices) && (wait_times < ENUMERATION_TIMES)) {
			/* enumeration need 4ms normally, wait 40ms here is enough */
			usleep_range(100, 150);
			wait_times++;
		}

		AUDIO_LOGI("After busreset, Discovered devices: %d, enumerated devices: %d",
			events.reported_presence, slimbus_devices.devices);
		if (slimbus_devices.devices < expect_devices) {
			AUDIO_LOGE("retry fail, please check chip is connected or not");
			return -ENODEV;
		}
	}

	AUDIO_LOGI("slimbus codec enumerate success");

	return 0;
}

void slimbus_clear_port_fifo(uint8_t port)
{
	slimbus_drv->clear_data_port_fifo(devm_slimbus_priv, port);
}

