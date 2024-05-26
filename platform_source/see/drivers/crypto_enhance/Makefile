EXTRA_CFLAGS += -I$(srctree)/drivers/tzdriver
EXTRA_CFLAGS += -I$(srctree)/drivers/platform_drivers/tzdriver
EXTRA_CFLAGS += -I$(srctree)/platform_source/see/drivers/crypto_enhance/crypto_engine_at
EXTRA_CFLAGS += -I$(srctree)/platform_source/see/drivers/crypto_enhance/power

subdir-ccflags-y              += -Wall -Werror

obj-$(CONFIG_CRYPTO_ENHANCE)  += crypto_enhance.o

ifeq ($(CONFIG_CRYPTO_ENHANCE_POWERON_WITH_SMMU),y)
obj-$(CONFIG_CRYPTO_ENHANCE)  += power/tcu_power.o
else
obj-$(CONFIG_CRYPTO_ENHANCE)  += power/crypto_enhance_power.o
endif

obj-$(CONFIG_CRYPTO_ENHANCE_SMX_ATCMD)  += \
	crypto_engine_at/crypto_engine_at.o \
	crypto_engine_at/crypto_engine_at_smx.o

