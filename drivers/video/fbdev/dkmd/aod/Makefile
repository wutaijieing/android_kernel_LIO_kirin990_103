ccflags-y  += -I$(srctree)/drivers/huawei_platform/inputhub/iom7/
#ifdef CONFIG_CONTEXTHUB_SHMEM
ccflags-y  += -I$(srctree)/platform_source/smart/drivers/channels
#endif


obj-y += dpu_aod_device.o

EXTRA_CFLAGS += -I$(srctree)/drivers/huawei_platform/inputhub/default\
		-I$(srctree)/drivers/video/fbdev/dkmd/dpu/azalea \
		-I$(srctree)/drivers/video/dkmd/dpu/azalea \
		-I$(srctree)/drivers/staging/android
