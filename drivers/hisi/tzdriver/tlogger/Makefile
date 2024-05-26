ifeq ($(strip $(TARGET_PRODUCT)), mdc)
	ccflags-y += -DCONFIG_MDC_PLATFORM
endif

ifeq ($(strip $(TARGET_PRODUCT)), mini)
	ccflags-y += -DCONFIG_MINI_PLATFORM
endif

KERNEL_DIR :=$(srctree)

ifneq ($(TARGET_BUILD_VARIANT),user)
	ccflags-y += -DDEF_ENG
endif

EXTRA_CFLAGS += -I$(KERNEL_DIR)/drivers/platform_drivers/tzdriver
EXTRA_CFLAGS += -I$(KERNEL_DIR)/drivers/platform_drivers/tzdriver/core

obj-$(CONFIG_TEELOG) += tlogger.o

# For tee log memory type: bbox, rdr, or pages,
# If no log mechanism is available, the pages memory can be used.
obj-$(CONFIG_BBOX_MEM) += log_bbox_cfg.o
obj-$(CONFIG_RDR_MEM) += log_rdr_cfg.o
obj-$(CONFIG_PAGES_MEM) += log_pages_cfg.o