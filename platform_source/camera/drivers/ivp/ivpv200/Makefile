# hisilicon ivp ivpv200
ifeq ($(es_low_freq),true)
ccflags-$(CONFIG_IVP_IVPV200) += -DCONFIG_ES_LOW_FREQ
endif

subdir-ccflags-$(CONFIG_IVP_IVPV200) += -I$(srctree)/platform_source/camera/drivers/ivp/

obj-$(CONFIG_IVP_IVPV200) += ivp_platform.o
