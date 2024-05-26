/*
 * dwc3_usb_interface.h
 *
 * dwc3 interface defination
 *
 * Copyright (c) 2016-2020 Technologies Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#ifndef DWC3_USB_INTERFACE_H
#define DWC3_USB_INTERFACE_H

#include <linux/notifier.h>
#include <linux/types.h>

#define SET_NBITS_MASK(start, end) (((2u << ((end) - (start))) - 1) << (start))
#define SET_BIT_MASK(bit) SET_NBITS_MASK(bit, bit)

#define DEVICE_EVENT_CONNECT_DONE	0
#define DEVICE_EVENT_PULLUP	1
#define DEVICE_EVENT_CMD_TMO	2
#define DEVICE_EVENT_SETCONFIG	3

enum usb_power_state {
	USB_POWER_OFF = 0,
	USB_POWER_ON = 1,
	USB_POWER_HOLD = 2,
};

struct usb3_core_ops {
	void (*disable_pipe_clock)(void);
	int (*enable_u3)(void);
	void (*dump_regs)(void);
#ifdef CONFIG_DFX_DEBUG_FS
	void (*link_state_print)(void);
#endif
	void (*logic_analyzer_trace_set)(u32 value);
};

void set_chip_dwc3_power_flag(int val);
int get_chip_dwc3_power_flag(void);
int chip_dwc3_is_powerdown(void);
struct usb3_core_ops *get_usb3_core_ops(void);
void dwc3_lscdtimer_set(void);
int dwc3_compliance_mode_enable(void);

#if IS_ENABLED(CONFIG_USB_DWC3_CHIP)
void usb_controller_irq_clr(void);
int dwc3_device_event_notifier_register(struct notifier_block *nb);
int dwc3_device_event_notifier_unregister(struct notifier_block *nb);
int dwc3_atomic_notifier_call_chain(unsigned long val, void *v);
#else
static inline void usb_controller_irq_clr(void) { }
static inline int dwc3_device_event_notifier_register(struct notifier_block *nb)
{ return 0; }
static inline int dwc3_device_event_notifier_unregister(struct notifier_block *nb)
{ return 0; }
static inline int dwc3_atomic_notifier_call_chain(unsigned long val, void *v)
{ return 0; }
#endif

#endif /* dwc3_usb_interface.h */
