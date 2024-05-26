# hisilicon ivp ivpv300
ifeq ($(es_low_freq),true)
ccflags-$(CONFIG_IVP_IVPV300) += -DCONFIG_ES_LOW_FREQ
endif

ccflags-y += -Wextra -Wdate-time -Wfloat-equal -Wformat=2 -Wshadow

subdir-ccflags-$(CONFIG_IVP_IVPV300) += -I$(srctree)/platform_source/camera/drivers/ivp/

obj-$(CONFIG_IVP_IVPV300) += ivp_platform.o
