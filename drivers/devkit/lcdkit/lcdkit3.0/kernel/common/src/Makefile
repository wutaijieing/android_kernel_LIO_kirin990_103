
ifneq ($(TARGET_BUILD_VARIANT),user)
EXTRA_CFLAGS += -DLCD_KIT_DEBUG_ENABLE
endif

ifneq ($(TARGET_BUILD_VARIANT),user)
	obj-y += lcd_kit_debug.o
endif

ifeq ($(TARGET_VERSION_MODE),factory)
obj-y   += lcd_kit_factory.o
EXTRA_CFLAGS += -DLCD_FACTORY_MODE
endif

ifeq ($(CONFIG_HUAWEI_DEVKIT_HISI_3_0),y)
	EXTRA_CFLAGS += -DLV_GET_LCDBK_ON
endif

obj-y	+= lcd_kit_sysfs.o
obj-y	+= lcd_kit_common.o
obj-y	+= lcd_kit_parse.o
obj-y	+= lcd_kit_core.o
obj-y	+= lcd_kit_bl.o
obj-y	+= lcd_kit_bias.o
obj-y	+= bias_ic_common.o
obj-y	+= bias_bl_common.o
obj-y   += lcd_kit_spidev.o
ifneq ($(filter denver, $(TARGET_DEVICE)),)
obj-y	+= thirdsrc/
endif
EXTRA_CFLAGS += -I$(srctree)/drivers/devkit/lcdkit/lcdkit3.0/kernel/common/include
EXTRA_CFLAGS += -I$(srctree)/drivers/devkit/lcdkit/lcdkit3.0/kernel/hisi/include
EXTRA_CFLAGS += -I$(srctree)/drivers/video/fbdev/dkmd/dpu/azalea
EXTRA_CFLAGS += -I$(srctree)/drivers/video/dkmd/dpu/azalea \
				-I$(srctree)/drivers/base
