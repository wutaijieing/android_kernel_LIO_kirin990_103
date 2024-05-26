ccflags-y += -Wextra -Wdate-time -Wfloat-equal -Wshadow -Wformat=2

PM_DIR :=$(srctree)/platform_source/cee/drivers/pm/new

EXTRA_CFLAGS += -I$(PM_DIR)

obj-y += register/register_ops.o
obj-y += dtsi/dtsi_ops.o
obj-$(CONFIG_DFX_DEBUG_FS) += debugfs/debugfs.o
