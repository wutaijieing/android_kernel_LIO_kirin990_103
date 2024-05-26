ccflags-y += -Wall -Werror
EXTRA_CFLAGS += -I$(srctree)/drivers/platform_drivers/tzdriver
ifeq ($(CONFIG_GENERAL_SEE),y)
        EXTRA_CFLAGS += -I$(srctree)/platform_source/see/drivers/general_see
endif
ifeq ($(CONFIG_CRYPTO_CORE),y)
        EXTRA_CFLAGS += -I$(srctree)/platform_source/see/drivers/crypto_core
endif

ifeq ($(TARGET_VERSION_MODE),factory)
ccflags-y += -DFACTORY_TEST_GENERAL_SEE
endif

obj-$(CONFIG_GENERAL_SEE_MNTN) += general_see_mntn.o
