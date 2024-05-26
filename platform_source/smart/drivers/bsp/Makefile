ifneq ($(CONFIG_INPUTHUB_AS),y)
include $(srctree)/platform_source/smart/drivers/bsp/extern.mk
endif

EXTRA_CFLAGS += -Idrivers/platform_drivers/tzdriver
ccflags-y  += -I$(srctree)/platform_source/smart/drivers/channels/

obj-$(CONFIG_INPUTHUB_30) += boot/iomcu_boot.o
obj-$(CONFIG_INPUTHUB_30) += dump/iomcu_dump.o
obj-$(CONFIG_INPUTHUB_30) += dump/iomcu_dump_info_restore.o
obj-$(CONFIG_INPUTHUB_30) += dump/iomcu_dump_sysctrl.o
obj-$(CONFIG_INPUTHUB_AS) += pm/iomcu_pm.o

ifneq ($(CONFIG_IPC_V5),y)
obj-$(CONFIG_INPUTHUB_30) += ipc_v3/iomcu_ipc.o
obj-$(CONFIG_INPUTHUB_30) += ipc_v3/iomcu_shmem.o
endif

obj-$(CONFIG_IPC_V5) += ipc_v5/iomcu_tcp_amsm.o
obj-$(CONFIG_IPC_V5) += ipc_v5/iomcu_tcp_amsm_adapter.o
obj-$(CONFIG_IPC_V5) += ipc_v5/iomcu_tcp_amf20.o
obj-$(CONFIG_IPC_V5) += ipc_v5/iomcu_link_ipc.o
obj-$(CONFIG_IPC_V5) += ipc_v5/iomcu_link_ipc_shm.o
