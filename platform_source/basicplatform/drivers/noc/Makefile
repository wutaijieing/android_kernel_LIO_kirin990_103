#
#NoC Makefile
#

subdir-ccflags-$(CONFIG_DFX_BB)               += -Idrivers/platform_drivers/mntn/blackbox/
subdir-ccflags-$(CONFIG_DFX_NOC)              += -Idrivers/platform_drivers/mntn/

obj-$(CONFIG_DFX_NOC) +=  dfx_noc.o             \
                           dfx_noc_err_probe.o       \
                           dfx_noc_packet.o      \
                           dfx_noc_transcation.o \
                           dfx_noc_info.o        \
                           dfx_noc_dump.o        \
                           dfx_noc_info_mouse.o \
                           dfx_noc_info_cows.o \
                           dfx_noc_info_capricorn.o \
                           dfx_noc_info_tiger_es.o \
                           dfx_noc_info_tiger.o \
                           dfx_noc_info_aquarius.o \
                           dfx_noc_info_rabbit_es.o \
                           dfx_noc_info_rabbit.o \
                           dfx_noc_info_pisces.o \
                           dfx_noc_get_data_from_dts.o \
                           dfx_noc_info_dragon_cs2.o \

obj-$(CONFIG_DFX_NOC_DBG) += dfx_noc_dbg.o
obj-$(CONFIG_DFX_QIC) +=  dfx_qic.o             \
                           dfx_qic_dump.o        \
