ifeq ($(TARGET_VERSION_MODE),factory)
obj-$(CONFIG_LCD_KIT_QCOM)   += lcd_kit_factory.o
EXTRA_CFLAGS += -DLCD_FACTORY_MODE
endif
ifneq ($(TARGET_BUILD_VARIANT),user)
EXTRA_CFLAGS += -DLCD_KIT_DEBUG_ENABLE
endif

ifeq ($(TARGET_PRODUCT),bengal)
$(info "TARGET_PRODUCT = bengal_1")
EXTRA_CFLAGS += -DQCOM_PLATFORM_6225
endif

obj-$(CONFIG_LCD_KIT_QCOM)	+= lcd_kit_sysfs.o
obj-$(CONFIG_LCD_KIT_QCOM)	+= lcd_kit_common.o
obj-$(CONFIG_LCD_KIT_QCOM)	+= lcd_kit_parse.o
obj-$(CONFIG_LCD_KIT_QCOM)	+= lcd_kit_core.o
obj-$(CONFIG_LCD_KIT_QCOM)	+= lcd_kit_bl.o
obj-$(CONFIG_LCD_KIT_QCOM)	+= lcd_kit_bias.o
ifneq ($(TARGET_BUILD_VARIANT),user)
obj-$(CONFIG_LCD_KIT_QCOM)	+= lcd_kit_debug.o
endif

EXTRA_CFLAGS += -I$(srctree)/drivers/devkit/lcdkit/lcdkit3.0/kernel/qcom/common/include \
		-I$(srctree)/drivers/devkit/lcdkit/lcdkit3.0/kernel/common/displayengine/include \
		-I$(srctree)/drivers/base \
		-I$(srctree)/include/drm
