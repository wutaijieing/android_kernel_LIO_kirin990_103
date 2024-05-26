obj-$(CONFIG_LCD_KIT_QCOM)	+= lcd_kit_power.o
obj-$(CONFIG_LCD_KIT_QCOM) 	+= lcd_kit_utils.o
obj-$(CONFIG_LCD_KIT_QCOM) 	+= lcd_kit_panel.o
obj-$(CONFIG_LCD_KIT_QCOM) 	+= lcd_kit_drm_panel.o
obj-$(CONFIG_LCD_KIT_QCOM) 	+= lcd_kit_pinctrl.o
obj-$(CONFIG_LCD_KIT_QCOM) 	+= bias/
obj-$(CONFIG_LCD_KIT_QCOM) 	+= backlight/

ifneq ($(TARGET_BUILD_VARIANT),user)
EXTRA_CFLAGS += -DLCD_KIT_DEBUG_ENABLE
endif
ifeq ($(TARGET_VERSION_MODE),factory)
EXTRA_CFLAGS += -DLCD_FACTORY_MODE
endif
ifeq ($(TARGET_PRODUCT),bengal)
$(info "TARGET_PRODUCT = bengal_2")
EXTRA_CFLAGS += -DQCOM_PLATFORM_6225
endif
EXTRA_CFLAGS +=-I$(srctree)/drivers/staging/android \
		-I$(srctree)/drivers/atfdriver \
		-I$(srctree)/drivers/devkit/lcdkit/lcdkit3.0 \
		-I$(srctree)/drivers/devkit/lcdkit/lcdkit3.0/kernel/qcom/common/include \
		-I$(srctree)/drivers/devkit/lcdkit/lcdkit3.0/kernel/qcom/include \
		-I$(srctree)/drivers/devkit/lcdkit/lcdkit3.0/kernel/qcom/adapt \
		-I$(srctree)/drivers/devkit/lcdkit/lcdkit3.0/kernel/qcom/src/backlight \
		-I$(srctree)/include/drm \
		-I$(srctree)/drivers/gpu/drm \
		-I$(srctree)/techpack/display/msm \
		-I$(srctree)/techpack/display/msm/dsi \
		-I$(srctree)/techpack/display/include \
		-I$(srctree)/techpack/display/include/uapi/display \
		-I$(srctree)/drivers/clk/qcom \
		-I$(srctree)/drivers/devkit/lcdkit/lcdkit3.0/kernel/common/displayengine/include
