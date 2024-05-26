obj-$(CONFIG_CPU_EDAC_ARM64) += cpu_edac_arm64.o

ccflags-$(CONFIG_CPU_EDAC_ARM64)  += -I$(srctree)/drivers/edac

obj-$(CONFIG_RAS_EDAC) += ras_common.o
obj-$(CONFIG_RAS_EDAC) += ras_device.o
obj-$(CONFIG_RAS_EDAC_DEBUG) += ras_err_inj.o
ccflags-$(CONFIG_RAS_EDAC)  += -I$(srctree)/drivers/edac
ccflags-y += -Wextra -Wdate-time -Wfloat-equal -Wshadow
