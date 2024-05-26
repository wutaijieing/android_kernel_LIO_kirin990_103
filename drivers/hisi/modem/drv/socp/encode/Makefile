#
# Makefile for the memory technology device drivers.
#
# Core functionality.

ifeq ($(strip $(llt_gcov)),y)
GCOV_PROFILE := y
endif

subdir-ccflags-y += -I$(srctree)/drivers/hisi/modem/drv/socp/
obj-y				+= socp_enc_hal.o 

subdir-ccflags-y += -I$(srctree)/drivers/hisi/modem/include/tools




