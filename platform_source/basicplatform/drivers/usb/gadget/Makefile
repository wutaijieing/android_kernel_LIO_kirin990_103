# SPDX-License-Identifier: GPL-2.0
#
# USB peripheral controller drivers
#
subdir-ccflags-$(CONFIG_USB_GADGET_DEBUG)	:= -DDEBUG
subdir-ccflags-$(CONFIG_USB_GADGET_VERBOSE)	+= -DVERBOSE_DEBUG
ccflags-y += -I$(srctree)/drivers/usb/gadget
ccflags-y += -I$(srctree)/drivers/usb/gadget/udc

usb_f_mtp-y                     := function-vendor/f_mtp.o
obj-$(CONFIG_USB_F_MTP)         += usb_f_mtp.o
usb_f_ptp-y                     := function-vendor/f_ptp.o
obj-$(CONFIG_USB_F_PTP)         += usb_f_ptp.o
obj-y				+= function-vendor/
