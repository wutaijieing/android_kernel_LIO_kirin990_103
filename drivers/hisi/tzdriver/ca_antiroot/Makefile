# Makefile for the TEE root scan client.
#
# Each configuration option enables a list of files.

ifneq ($(TARGET_BUILD_VARIANT),user)
	ccflags-y += -DDEBUG_LOGMASK
endif

obj-$(CONFIG_TEE_ANTIROOT_CLIENT)  += rootagent_api.o \
	rootagent.o rootagent_crypto.o rootagent_mem.o

obj-$(CONFIG_TEE_ANTIROOT_CLIENT_ENG_DEBUG) += rootagent_check.o

ifdef CONFIG_MTK_PLATFORM
ccflags-y += -Idrivers/tzdriver
else
ccflags-y += -I$(srctree)/drivers/hisi/tzdriver
endif

ccflags-$(CONFIG_TEE_ANTIROOT_CLIENT) += \
	-I$(srctree)/security/hw_root_scan/include

