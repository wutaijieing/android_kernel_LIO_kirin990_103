EXTRA_CFLAGS += -I$(srctree)/platform_source/basicplatform/drivers/rpmb
EXTRA_CFLAGS += -I$(srctree)/platform_source/basicplatform/drivers/mntn/blackbox/platform_lpm3
EXTRA_CFLAGS += -I$(srctree)/drivers/platform_drivers/tzdriver
EXTRA_CFLAGS += -I$(srctree)/platform_source/basicplatform/drivers/mntn/general_see

subdir-ccflags-y              += -Wall -Werror

ifneq ($(CONFIG_GENERAL_SEE_DISABLE_KEY),y)
ifeq ($(TARGET_VERSION_MODE),factory)
EXTRA_CFLAGS += -DCONFIG_GENERAL_SEE_CHIPTEST_RT
endif

ifneq ($(TARGET_BUILD_VARIANT),user)
EXTRA_CFLAGS += -DCONFIG_GENERAL_SEE_CHIPTEST_SLT
endif

ifeq ($(TARGET_BUILD_VARIANT),user)
EXTRA_CFLAGS += -DCONFIG_GENERAL_SEE_SUPPORT_BYPASS_IMG
endif
endif

obj-$(CONFIG_GENERAL_SEE)  += general_see.o
obj-$(CONFIG_GENERAL_SEE)  += general_see_power.o
obj-$(CONFIG_GENERAL_SEE)  += general_see_upgrade.o
obj-$(CONFIG_GENERAL_SEE)  += general_see_chip_test.o
obj-$(CONFIG_GENERAL_SEE)  += general_see_fs.o
obj-$(CONFIG_GENERAL_SEE_SUPPORT_CASDKEY) += general_see_casdcert.o
obj-$(CONFIG_GENERAL_SEE_SUPPORT_DCS) += general_see_dcs.o

ifneq ($(CONFIG_GENERAL_SEE_DISABLE_KEY),y)
ifeq ($(CONFIG_GENERAL_SEE),y)
obj-$(CONFIG_DFX_DEBUG_FS)  += general_see_autotest.o
endif
endif
