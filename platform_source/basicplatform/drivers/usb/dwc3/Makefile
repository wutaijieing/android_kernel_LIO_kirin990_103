# SPDX-License-Identifier: GPL-2.0
# define_trace.h needs to know how to find our header

ccflags-y += -I$(srctree)/drivers/usb/dwc3

ifneq ($(CONFIG_DEBUG_FS),)
	obj-$(CONFIG_DEBUG_FS)	+= dwc3-debugfs-ext.o
endif
obj-$(CONFIG_USB_DWC3_CHIP)		+= dwc3-usb-interface.o

obj-$(CONFIG_USB_DWC3_CHIP)		+= chip/
