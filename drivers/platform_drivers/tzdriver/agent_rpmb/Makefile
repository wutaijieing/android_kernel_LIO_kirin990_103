KERNEL_DIR :=$(srctree)

ifneq ($(TARGET_BUILD_VARIANT),user)
	ccflags-y += -DDEF_ENG
endif

EXTRA_CFLAGS += -I$(KERNEL_DIR)/drivers/platform_drivers/tzdriver
EXTRA_CFLAGS += -I$(KERNEL_DIR)/drivers/platform_drivers/tzdriver/core

ifeq ($(CONFIG_MEDIATEK_SOLUTION),y)
	MTK_PLATFORM := $(subst ",,$(CONFIG_MTK_PLATFORM))
	EXTRA_CFLAGS += -I$(KERNEL_DIR)/drivers/mmc/core
	EXTRA_CFLAGS += -I$(KERNEL_DIR)/drivers/mmc/card
	EXTRA_CFLAGS += -I$(KERNEL_DIR)/drivers/misc/mediatek/include
	EXTRA_CFLAGS += -I$(KERNEL_DIR)/drivers/misc/mediatek/include/mt-plat/$(MTK_PLATFORM)/include
	EXTRA_CFLAGS += -I$(KERNEL_DIR)/drivers/misc/mediatek/include/mt-plat
	EXTRA_CFLAGS += -I$(KERNEL_DIR)/drivers/misc/mediatek/base/power/$(MTK_PLATFORM)
	EXTRA_CFLAGS += -I$(KERNEL_DIR)/drivers/misc/mediatek/base/power/include
	EXTRA_CFLAGS += -I$(KERNEL_DIR)/drivers/devfreq
	EXTRA_CFLAGS += -I$(KERNEL_DIR)/drivers/scsi/ufs
	EXTRA_CFLAGS += -I$(KERNEL_DIR)/drivers/mmc/host/mediatek/ComboA
	ifeq ($(CONFIG_MTK_PLATFORM), "mt6761")
		EXTRA_CFLAGS+= -I$(KERNEL_DIR)/drivers/mmc/host/mediatek/ComboA/mt6765
	else
		EXTRA_CFLAGS+= -I$(KERNEL_DIR)/drivers/mmc/host/mediatek/ComboA/$(MTK_PLATFORM)
	endif
	EXTRA_CFLAGS += -I$(KERNEL_DIR)/drivers/mmc/host/mediatek/$(MTK_PLATFORM)
	EXTRA_CFLAGS += -I$(KERNEL_DIR)/drivers/mmc/host/mediatek/$(MTK_PLATFORM)/$(MTK_PLATFORM)
	EXTRA_CFLAGS += -I$(KERNEL_DIR)/drivers/platform_drivers/tzdriver/agent_rpmb/mplat
	obj-$(CONFIG_RPMB_AGENT) += core/agent_rpmb.o mplat/rpmb_driver.o
else
	EXTRA_CFLAGS += -I$(KERNEL_DIR)/drivers/platform_drivers/tzdriver/agent_rpmb/generic
	obj-$(CONFIG_RPMB_AGENT) += core/agent_rpmb.o generic/rpmb_driver.o
endif
