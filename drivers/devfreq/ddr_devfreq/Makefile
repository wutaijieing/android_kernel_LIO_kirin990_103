obj-$(CONFIG_DEVFREQ_GOV_PM_QOS)            += governor_pm_qos.o
obj-$(CONFIG_DDR_DEVFREQ)                   += ddr_devfreq.o
ccflags-$(CONFIG_DEVFREQ_GOV_PM_QOS)        += -I$(srctree)/drivers/devfreq

obj-$(CONFIG_DEVFREQ_GOV_PLATFORM_QOS)      += ddr_governor_platform_qos.o
obj-$(CONFIG_DDR_DEVFREQ_PLATFORM)          += ddr_devfreq_platform.o
ccflags-$(CONFIG_DEVFREQ_GOV_PLATFORM_QOS)  += -I$(srctree)/drivers/devfreq

ccflags-y += -Wdate-time -Wextra -Wfloat-equal -Wformat=2 -Wshadow
