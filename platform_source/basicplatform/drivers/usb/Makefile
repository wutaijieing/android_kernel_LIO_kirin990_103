# SPDX-License-Identifier: GPL-2.0
#
# Makefile for the kernel USB device drivers.
#

# Object files in subdirectories

obj-$(CONFIG_USB_PROXY_HCD)	+= proxy-hcd/
obj-$(CONFIG_VENDOR_USB_TYPEC)	+= usb-typec/
obj-$(CONFIG_USB_HIUSBC)	+= hiusbc/
obj-y				+= chip-usb/
obj-y				+= chip-usb-debug/
obj-y               += core/
obj-y               += dwc3/
obj-y               += gadget/
obj-y               += host/
obj-y               += misc/
obj-$(CONFIG_USB_VBUS_SPMI)	+= hisi-usb-vbus/
