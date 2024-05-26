NPU_ARCH_VERSION=npu_v100

export NPU_ARCH_VERSION

NPU_GLOBAL_CFLAGS += -DCONFIG_NPU_NOC

ifneq ($(chip_type),)
	NPU_CHIP_VERSION=$(TARGET_BOARD_PLATFORM)_$(chip_type)
	NPU_CHIP_CFG=y
	export NPU_CHIP_CFG
endif
