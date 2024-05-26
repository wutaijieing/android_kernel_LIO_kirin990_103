ifeq ($(TARGET_VERSION_MODE),factory)
EXTRA_CFLAGS += -DLCD_FACTORY_MODE
endif

ifneq ($(TARGET_PRODUCT),bengal)
$(info $(TARGET_PRODUCT))
obj-$(CONFIG_LCD_KIT_QCOM)	+= lcd_kit_displayengine.o
endif

EXTRA_CFLAGS += -I$(srctree)/drivers/devkit/lcdkit/lcdkit3.0/kernel/common/displayengine/include \
		-I$(srctree)/drivers/staging/android \
		-I$(srctree)/drivers/atfdriver \
		-I$(srctree)/drivers/devkit/lcdkit/lcdkit3.0 \
		-I$(srctree)/drivers/devkit/lcdkit/lcdkit3.0/kernel/qcom/common/include \
		-I$(srctree)/drivers/devkit/lcdkit/lcdkit3.0/kernel/qcom/include \
		-I$(srctree)/drivers/devkit/lcdkit/lcdkit3.0/kernel/qcom/adapt \
		-I$(srctree)/drivers/devkit/lcdkit/lcdkit3.0/kernel/qcom/src/backlight \
		-I$(srctree)/include/drm \
		-I$(srctree)/include/uapi/drm \
		-I$(srctree)/drivers/gpu/drm \
		-I$(srctree)/techpack/display/msm \
		-I$(srctree)/techpack/display/msm/dsi \
		-I$(srctree)/techpack/display/include \
		-I$(srctree)/techpack/display/include/uapi/display \
		-I$(srctree)/techpack/display/include/uapi/display/drm \
		-I$(srctree)/drivers/clk/qcom \
		-I$(srctree)/techpack/display/msm/sde
