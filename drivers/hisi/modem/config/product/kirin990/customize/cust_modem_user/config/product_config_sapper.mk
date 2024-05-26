#lr
RTOSCK_OS_PATH := $(BALONG_TOPDIR)/modem/system/rtosck_smp
RTOSCK_INCLUDE_PATH := $(BALONG_TOPDIR)/modem/include/rtosck_smp/ccpu
RTOSCK_LR_OS_PATH := $(RTOSCK_OS_PATH)/ccpu/lrcpu
RTOSCK_LR_INCLUDE_PATH := $(RTOSCK_INCLUDE_PATH)/ccpu
ifeq ($(modem_sanitizer),true)
RTOSCK_OS_LIB_PATH := $(OBB_PRODUCT_LIB_DIR)/rtosck_os/smpv2r2_lr_asan
else
RTOSCK_OS_LIB_PATH := $(OBB_PRODUCT_LIB_DIR)/rtosck_os/smpv2r2
endif

CFG_CONFIG_SAPPER_SETS :=

ifneq ($(extra_modem),)
CFG_EXTRA_MODEM_MODE := FEATURE_ON
CFG_CONFIG_SAPPER_SETS += CFG_EXTRA_MODEM_MODE
endif
