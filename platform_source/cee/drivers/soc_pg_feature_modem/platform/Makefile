SOC_PG_FEATURE_DIR :=$(srctree)/platform_source/cee/drivers/soc_pg_feature_modem/platform

subdir-ccflags-$(CONFIG_SOC_PG_FEATURE_DEVDRV_DEVPLATFORM) += -I$(SOC_PG_FEATURE_DIR)/$(TARGET_BOARD_PLATFORM)

ifneq ($(TARGET_BOARD_PLATFORM),)
  obj-$(CONFIG_SOC_PG_FEATURE_DEVDRV_DEVPLATFORM) += $(TARGET_BOARD_PLATFORM)/soc_pg_feature_adapt.o
endif
