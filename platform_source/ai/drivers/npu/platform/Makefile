EXTRA_CFLAGS += $(NPU_GLOBAL_CFLAGS)
EXTRA_CFLAGS += -I$(NPU_DIR)/inc/comm
EXTRA_CFLAGS += -I$(NPU_DIR)/inc
EXTRA_CFLAGS += -I$(NPU_DIR)/inc/model
EXTRA_CFLAGS += -I$(NPU_DIR)/device/core
EXTRA_CFLAGS += -I$(NPU_DIR)/device/config
EXTRA_CFLAGS += -I$(NPU_DIR)/device/format_convertor
EXTRA_CFLAGS += -I$(NPU_DIR)/device/heartbeat
EXTRA_CFLAGS += -I$(NPU_DIR)/device/stream_manager
EXTRA_CFLAGS += -I$(NPU_DIR)/facility/atf
EXTRA_CFLAGS += -I$(NPU_DIR)/facility/memory
EXTRA_CFLAGS += -I$(NPU_DIR)/facility/communication
EXTRA_CFLAGS += -I$(NPU_DIR)/manager
EXTRA_CFLAGS += -I$(NPU_DIR)/dfx
EXTRA_CFLAGS += -I$(NPU_DIR)/platform
EXTRA_CFLAGS += -I$(NPU_DIR)/platform/dts
EXTRA_CFLAGS += -I$(NPU_DIR)/platform/$(NPU_ARCH_VERSION)
EXTRA_CFLAGS += -I$(NPU_DIR)/platform/$(NPU_ARCH_VERSION)/$(NPU_CHIP_VERSION)
EXTRA_CFLAGS += -I$(NPU_DIR)/bbit_debugfs
EXTRA_CFLAGS += -I$(NPU_DIR)/device/lowpower
EXTRA_CFLAGS += -I$(srctree)/drivers/platform_drivers/ap/platform/common/
ifeq ($(CONFIG_NPU_SWTS),y)
	EXTRA_CFLAGS += -I$(NPU_DIR)/device/schedule
	EXTRA_CFLAGS += -I$(NPU_DIR)/platform/npu_v200/baltimore/schedule
endif

ifeq ($(NPU_ARCH_VERSION),npu_v100)
	EXTRA_CFLAGS += -I$(NPU_DIR)/device/power_manager/v100
else
	EXTRA_CFLAGS += -I$(NPU_DIR)/device/power_manager
endif

obj-$(CONFIG_NPU_DEVDRV_DEVPLATFORM) += npu_adapt.o
obj-$(CONFIG_NPU_DEVDRV_DEVPLATFORM) += npu_platform_module.o
npu_platform_module-objs := npu_platform.o npu_adapter_pm.o
npu_platform_module-objs += dts/npu_dfx.o dts/npu_feature.o dts/npu_irq.o dts/npu_reg.o npu_iova_dev.o
npu_adapt-objs := $(NPU_ARCH_VERSION)/$(NPU_CHIP_VERSION)/npu_adapter.o
npu_adapt-objs += $(NPU_ARCH_VERSION)/npu_resmem.o

ifneq ($(CONFIG_NPU_SWTS),y)
npu_adapt-objs += $(NPU_ARCH_VERSION)/npu_irq_adapter.o
endif

ifeq ($(NPU_ARCH_VERSION),npu_v100)
npu_adapt-objs += $(NPU_ARCH_VERSION)/npu_sink_task_verify.o
npu_adapt-objs += $(NPU_ARCH_VERSION)/npu_sink_sqe_fmt.o
endif

ifeq ($(NPU_ARCH_VERSION),npu_v200)
	npu_adapt-objs += $(NPU_ARCH_VERSION)/$(NPU_CHIP_VERSION)/npu_hwts_plat.o
	npu_adapt-objs += $(NPU_ARCH_VERSION)/$(NPU_CHIP_VERSION)/npu_sink_task_verify.o
	npu_adapt-objs += $(NPU_ARCH_VERSION)/$(NPU_CHIP_VERSION)/npu_sink_sqe_fmt.o
	npu_adapt-objs += $(NPU_ARCH_VERSION)/$(NPU_CHIP_VERSION)/npu_platform_pm.o
	npu_adapt-objs += $(NPU_ARCH_VERSION)/$(NPU_CHIP_VERSION)/npu_aicore_plat.o
	npu_adapt-$(CONFIG_NPU_SDMA) += $(NPU_ARCH_VERSION)/$(NPU_CHIP_VERSION)/npu_sdma_plat.o
	npu_adapt-objs += $(NPU_ARCH_VERSION)/$(NPU_CHIP_VERSION)/npu_platform_exception.o
	npu_adapt-objs += $(NPU_ARCH_VERSION)/$(NPU_CHIP_VERSION)/npu_chip_cfg.o
endif

ifeq ($(CONFIG_NPU_AUTOFS),y)
	npu_adapt-objs += $(NPU_ARCH_VERSION)/$(NPU_CHIP_VERSION)/npu_autofs_plat.o
endif

ifeq ($(CONFIG_NPU_SWTS),y)
	npu_adapt-objs += $(NPU_ARCH_VERSION)/$(NPU_CHIP_VERSION)/hwts_driver.o
	npu_adapt-objs += $(NPU_ARCH_VERSION)/$(NPU_CHIP_VERSION)/hwts_interrupt_handler.o
endif

npu_adapt-$(NPU_CHIP_CFG) +=  $(NPU_ARCH_VERSION)/$(NPU_CHIP_VERSION)/npu_chip_cfg.o
npu_adapt-$(CONFIG_NPU_PM_FUSION) +=  $(NPU_ARCH_VERSION)/$(NPU_CHIP_VERSION)/npu_pm_top_common.o
npu_adapt-$(CONFIG_NPU_PM_FUSION) += $(NPU_ARCH_VERSION)/$(NPU_CHIP_VERSION)/npu_pm_top_specify.o
npu_adapt-$(CONFIG_NPU_PM_FUSION) += $(NPU_ARCH_VERSION)/$(NPU_CHIP_VERSION)/npu_pm_non_top_common.o
npu_adapt-$(CONFIG_NPU_PM_FUSION) += $(NPU_ARCH_VERSION)/$(NPU_CHIP_VERSION)/npu_pm_non_top_specify.o



