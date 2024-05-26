ifeq ($(pcie_fpga_verify),true)
EXTRA_CFLAGS += -DCONFIG_NPU_PCIE
endif
obj-y +=ipc_mailbox.o
obj-y +=ipc_rproc.o
obj-y +=ipc_rproc_id_mgr.o
obj-y +=ipc_mailbox_event.o
obj-y +=ipc_mailbox_mntn.o
obj-$(CONFIG_IPC_RPROC_TESTFILE) +=ipc_rproc_test.o
obj-$(CONFIG_IPC_MAILBOX) +=ipc_mailbox_dev.o
subdir-ccflags-$(CONFIG_DFX_BB)    += -Idrivers/platform_drivers/mntn/blackbox/
obj-$(CONFIG_IPC_MAILBOX_VM) += ipc_vm_dev.o
