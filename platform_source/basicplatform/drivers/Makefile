obj-$(CONFIG_SPMI_PLATFORM)	+=spmi_platform/
obj-$(CONFIG_BOOTDEVICE)   += bootdevice/

obj-y += mfd/
obj-y += nve/
obj-y += partition/

obj-$(CONFIG_PLAT_CLK)     += clk/
obj-y	+= regulator/
obj-$(CONFIG_CMDLINE_PARSE)	+= cmdline/
obj-$(CONFIG_PLATFORM_IPC_MAILBOX)        += mailbox/
obj-$(CONFIG_VENDOR_RPMB) += rpmb/
ifneq ($(strip $(CFG_HISI_OPENSOURCE)), true)
obj-$(CONFIG_DFX_WATCHPOINT_CB)+= watchpoint/
endif
obj-y += gpio/
obj-$(CONFIG_I2C) += i2c/
obj-$(CONFIG_USB_PHY_CHIP)			+= usb_phy_chip/
obj-y += phy/
obj-y += usb/
obj-$(CONFIG_AB_PARTITION_TABLE)      += bootctrl/
obj-y += pinctrl/
obj-$(CONFIG_VENDOR_HWSPINLOCK)     += hwspinlock/
obj-$(CONFIG_PWM_HISI)     += pwm/
obj-$(CONFIG_RTC_DRV_PMIC_SPMI)     += rtc/
obj-$(CONFIG_I3C) += i3c/
obj-$(CONFIG_DFX_KERNELDUMP) += memory_dump/
obj-$(CONFIG_DFX_NOC) += noc/
obj-$(CONFIG_MM_LB)                       += mm_lb/
obj-$(CONFIG_AP_CP_DECOUPL)           += modem_decoupl/
obj-$(CONFIG_DFX_PLATFORM_MAINTAIN) += mntn/
obj-$(CONFIG_CORESIGHT)		+= hwtracing/dfx_coresight/
ifeq ($(TARGET_VERSION_MODE),factory)
obj-$(CONFIG_PLATFORM_DIEID) += dieid/
endif
obj-$(CONFIG_HISI_DUMMY_KO) += hisi_dummy_ko/
obj-$(CONFIG_DMADEVICES) += dma/
obj-y += tty/serial/
obj-$(CONFIG_SPI) += spi/
obj-$(CONFIG_DFX_SEC_QIC) += qic/
obj-y += watchdog/
obj-$(CONFIG_HISI_TIMEKEEPER) += timekeeper/
obj-y += hisi_cma/
obj-y += hisi_lmk/
obj-$(CONFIG_HCK) += hck/
obj-$(CONFIG_PCI) += pci/
obj-$(CONFIG_NEW_LEDS) += leds/
obj-y += input/keyboard/
obj-y += input/misc/
