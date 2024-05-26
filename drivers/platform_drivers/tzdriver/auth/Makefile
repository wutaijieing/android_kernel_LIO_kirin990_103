KERNEL_DIR :=$(srctree)

EXTRA_CFLAGS += -I$(KERNEL_DIR)/drivers/platform_drivers/tzdriver
EXTRA_CFLAGS += -I$(KERNEL_DIR)/drivers/platform_drivers/tzdriver/core
EXTRA_CFLAGS += -I$(KERNEL_DIR)/drivers/platform_drivers/tzdriver/tlogger

obj-$(CONFIG_CLIENT_AUTH) += client_hash_auth.o

ifeq ($(findstring y, $(CONFIG_TEECD_AUTH) $(CONFIG_CLIENT_AUTH)), y)
	obj-y += auth_base_impl.o
endif
