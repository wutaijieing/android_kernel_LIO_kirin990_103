ifneq ($(TARGET_BUILD_VARIANT),user)
EXTRA_CFLAGS += -DLCD_KIT_DEBUG_ENABLE
endif

ifeq ($(TARGET_VERSION_MODE),factory)
EXTRA_CFLAGS += -DLCD_FACTORY_MODE
endif
ifeq ($(CONFIG_HUAWEI_DEVKIT_HISI_3_0),y)
EXTRA_CFLAGS += -DLV_GET_LCDBK_ON
endif
obj-y	+= lcd_kit_disp.o
obj-y	+= lcd_kit_power.o
obj-y 	+= lcd_kit_utils.o
obj-y 	+= lcd_kit_panel.o
obj-y 	+= lcd_kit_ext_disp.o
obj-y	+= lcd_kit_ext_power.o
obj-y	+= lcd_kit_sim_serial_disp.o

EXTRA_CFLAGS += -I$(srctree)/drivers/video/fbdev/dkmd/dpu/azalea \
		-I$(srctree)/drivers/video/dkmd/dpu/azalea \
		-I$(srctree)/drivers/staging/android \
		-I$(srctree)/drivers/atfdriver \
		-I$(srctree)/drivers/hisi/tzdriver \
		-I$(srctree)/drivers/devkit/lcdkit/lcdkit3.0 \
		-I$(srctree)/drivers/devkit/lcdkit/lcdkit3.0/kernel/common/include \
		-I$(srctree)/drivers/devkit/lcdkit/lcdkit3.0/kernel/hisi/include \
		-I$(srctree)/drivers/devkit/lcdkit/lcdkit3.0/kernel/hisi/adapt \
		-I$(srctree)/include/chipset_common/log
