# hisilicon ivp ivpv210
ifeq ($(es_low_freq),true)
ccflags-$(CONFIG_IVP_IVPV210) += -DCONFIG_ES_LOW_FREQ
endif

subdir-ccflags-$(CONFIG_IVP_IVPV210) += -I$(srctree)/platform_source/camera/drivers/ivp/

obj-$(CONFIG_IVP_IVPV210) += ivp_platform.o
