obj-y	+= lcd_kit_adapt.o
obj-y	+= lcd_kit_sysfs_hs.o
obj-y	+= bias_bl_utils.o
ifneq ($(TARGET_BUILD_VARIANT),user)
obj-y += lcd_kit_debug_hs.o
endif
ifeq ($(TARGET_VERSION_MODE),factory)
obj-y   += lcd_kit_factory_hs.o
EXTRA_CFLAGS += -DLCD_FACTORY_MODE
endif
EXTRA_CFLAGS += -I$(srctree)/drivers/video/fbdev/dkmd/dpu/azalea \
		-I$(srctree)/drivers/video/dkmd/dpu/azalea \
		-I$(srctree)/drivers/staging/android \
		-I$(srctree)/drivers/atfdriver \
		-I$(srctree)/drivers/hisi/tzdriver \
		-I$(srctree)/drivers/base \
		-I$(srctree)/drivers/devkit/lcdkit/lcdkit3.0/kernel/common/include \
		-I$(srctree)/drivers/devkit/lcdkit/lcdkit3.0/kernel/hisi/include
