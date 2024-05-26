EXTRA_CFLAGS += -I$(srctree)/platform_source/basicplatform/drivers/rpmb
EXTRA_CFLAGS += -I$(srctree)/platform_source/basicplatform/drivers/mntn/general_see
EXTRA_CFLAGS += -I$(srctree)/platform_source/see/drivers/general_see
EXTRA_CFLAGS += -I$(srctree)/platform_source/see/drivers/crypto_core/atcmd

subdir-ccflags-y              += -Wall -Werror

obj-$(CONFIG_CRYPTO_CORE)  += crypto_core.o \
                            crypto_core_fs.o \
                            crypto_core_power.o \
                            crypto_core_smc.o \
                            crypto_core_upgrade.o \
                            crypto_core_factory.o \
                            crypto_core_flash.o \
                            crypto_core_smx.o \
                            atcmd/crypto_core_at.o \
                            atcmd/crypto_core_at_install.o \
                            atcmd/crypto_core_at_smx.o \
                            atcmd/crypto_core_at_socslt.o \
                            atcmd/crypto_core_at_test.o \
                            atcmd/crypto_core_factory_task.o

obj-$(CONFIG_DFX_DEBUG_FS)  += crypto_core_autotest.o

obj-$(CONFIG_CRYPTO_CORE_SUPPORT_DCS) += crypto_core_dcs.o

obj-$(CONFIG_CRYPTO_CORE_DISABLE_KEY) += atcmd/crypto_core_disable.o
