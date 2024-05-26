
obj-$(CONFIG_USB_DWC3_NYET_ABNORMAL)       += dwc3-hifi-usb_4_9.o

obj-$(CONFIG_USB_DWC3_NOV)		+= dwc3-nov.o chip_usb2_bc.o
obj-$(CONFIG_USB_DWC3_OCT)		+= dwc3-oct.o chip_usb2_bc.o vregbypass.o
obj-$(CONFIG_USB_DWC3_SEP)		+= dwc3-sep.o chip_usb2_bc.o vregbypass.o
obj-$(CONFIG_USB_DWC3_AUG)		+= dwc3-aug.o chip_usb2_bc.o
# usb3_31phy must before dwc3-plat(e.g mar,apr), module_init called base on the order in Makefile
obj-$(CONFIG_USB_DWC3_APR)		+= combophy_regcfg.o chip_usb3_31phy_v2.o dwc3-apr.o chip_usb3_bc.o chip_usb3_misctrl.o vregbypass.o
obj-$(CONFIG_USB_DWC3_MAR)		+= combophy_regcfg.o chip_usb3_31phy.o dwc3-mar.o chip_usb3_bc.o chip_usb3_misctrl.o vregbypass.o
obj-$(CONFIG_USB_DWC3_FEB)		+= combophy_regcfg.o chip_usb3_31phy.o dwc3-feb.o chip_usb3_bc.o chip_usb3_misctrl.o
obj-$(CONFIG_USB_DWC3_CHIP)		+= dwc3-chip.o chip_usb_bc12.o chip_usb_hw.o

obj-$(CONFIG_DFX_DEBUG_FS)		+= chip_usb_debug.o

ifeq ($(CONFIG_HIFI_MAILBOX), y)
EXTRA_CFLAGS += -I$(srctree)/platform_source/audio/drivers/hifi_mailbox/mailbox
EXTRA_CFLAGS += -I$(srctree)/platform_source/audio/drivers/hifi_mailbox/ipcm
EXTRA_CFLAGS += -I$(srctree)/platform_source/audio/drivers/hifi_dsp
EXTRA_CFLAGS += -I$(srctree)/drivers/platform_drivers/ap/platform/common/
EXTRA_CFLAGS += -I$(srctree)/include/audio/ipc
endif

ifneq ($(filter y,$(CONFIG_USB_DWC3_MAR) $(CONFIG_USB_DWC3_APR)),)
EXTRA_CFLAGS += -DCOMBOPHY_VERSION_1_1
EXTRA_CFLAGS += -DCOMBOPHY_FW_UPDATE
endif

chip_combophy-y				+= combophy_common.o
chip_combophy-$(CONFIG_CHIP_COMBOPHY)	+= combophy_func.o combophy_core.o
chip_combophy-$(CONFIG_USB_DWC3_MAR)	+= combophy.o
chip_combophy-$(CONFIG_USB_DWC3_APR)	+= combophy.o
chip_combophy-$(CONFIG_USB_DWC3_FEB)	+= combophy.o
obj-$(CONFIG_CONTEXTHUB_PD)		+= chip_combophy.o
