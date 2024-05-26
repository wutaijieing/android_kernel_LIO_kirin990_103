ccflags-y += -Wextra -Wdate-time -Wfloat-equal -Wshadow -Wformat=2

obj-$(CONFIG_NPUFREQ)      += npu_devfreq.o
ifeq ($(CONFIG_NPU_PM_SMC), y)
obj-$(CONFIG_NPU_PM)       += smc/npu_pm_smc.o
obj-$(CONFIG_NPU_PM)       += smc/npu_dvfs_smc.o
obj-$(CONFIG_NPU_PM_DEBUG) += smc/npu_pm_debug_smc.o
else
obj-$(CONFIG_NPU_PM)       += npu_pm.o
obj-$(CONFIG_NPU_PM)      += npu_dvfs.o
obj-$(CONFIG_NPU_PM_DEBUG) += npu_pm_debug.o
obj-$(CONFIG_NPU_PM_AVS_VDM)  += npu_avs.o
endif

ccflags-$(CONFIG_NPU_PM)  +=
