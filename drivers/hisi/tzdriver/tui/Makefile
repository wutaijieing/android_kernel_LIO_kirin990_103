KERNEL_DIR :=$(srctree)

ifneq ($(TARGET_BUILD_VARIANT),user)
	ccflags-y += -DDEBUG_TUI
endif

ifeq ($(CONFIG_TEE_TUI_MTK), y)
EXTRA_CFLAGS += -Idrivers/tzdriver
EXTRA_CFLAGS += -Idrivers/tzdriver/core
EXTRA_CFLAGS += -Idrivers/tzdriver/kthread_affinity
EXTRA_CFLAGS += -Idrivers/gpu/drm/mediatek
EXTRA_CFLAGS += -Idrivers/misc/mediatek/memory-ssmr
EXTRA_CFLAGS += -Idrivers/devkit/lcdkit/lcdkit3.0/kernel/mtk/include
EXTRA_CFLAGS += -Idrivers/devkit/lcdkit/lcdkit3.0/kernel/common/include
EXTRA_CFLAGS += -Idrivers/devkit/lcdkit/lcdkit3.0/kernel/mtk/adapt
else
EXTRA_CFLAGS += -I$(KERNEL_DIR)/drivers/platform_drivers/tzdriver
EXTRA_CFLAGS += -I$(KERNEL_DIR)/drivers/platform_drivers/tzdriver/core
EXTRA_CFLAGS += -I$(KERNEL_DIR)/drivers/platform_drivers/tzdriver/kthread_affinity
EXTRA_CFLAGS += -I$(KERNEL_DIR)/drivers/video/dkmd/dpu/azalea
EXTRA_CFLAGS += -I$(KERNEL_DIR)/drivers/video/fbdev/dkmd/dpu/azalea
endif

ifeq ($(CONFIG_HW_SECMEM), y)
EXTRA_CFLAGS += -Idrivers/uni_drivers/secmem
endif

obj-$(CONFIG_TEE_TUI) += tui.o
