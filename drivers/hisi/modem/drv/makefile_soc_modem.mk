#
# Makefile for the modem drivers.
#
-include $(srctree)/drivers/hisi/modem/config/product/$(OBB_PRODUCT_NAME)/$(OBB_MODEM_CUST_CONFIG_DIR)/config/balong_product_config.mk

ifeq ($(strip $(CONFIG_HISI_BALONG_MODEM)),m)
drv-y :=
drv-builtin :=

subdir-ccflags-y += -I$(srctree)/lib/libc_sec/securec_v2/include/
subdir-ccflags-y += -I$(srctree)/drivers/hisi/tzdriver/libhwsecurec/
subdir-ccflags-y += -I$(srctree)/drivers/hisi/modem/platform/ccore/$(CFG_PLATFORM)/
subdir-ccflags-y += -I$(srctree)/drivers/hisi/modem/platform/dsp/$(CFG_PLATFORM)/
subdir-ccflags-y += -I$(srctree)/drivers/hisi/modem/platform/acore/$(CFG_PLATFORM)/
subdir-ccflags-y += -I$(srctree)/drivers/hisi/modem/platform/acore/$(CFG_PLATFORM)/drv/
subdir-ccflags-y += -I$(srctree)/drivers/hisi/modem/platform/common/$(CFG_PLATFORM)/
subdir-ccflags-y += -I$(srctree)/drivers/hisi/modem/platform/common/$(CFG_PLATFORM)/soc/
subdir-ccflags-y += -I$(srctree)/drivers/hisi/modem/include/adrv/
subdir-ccflags-y += -I$(srctree)/drivers/hisi/modem/include/drv/acore/
subdir-ccflags-y += -I$(srctree)/drivers/hisi/modem/include/drv/common/
subdir-ccflags-y += -I$(srctree)/drivers/hisi/modem/drv/common/include/
subdir-ccflags-y += -I$(srctree)/drivers/hisi/modem/drv/acore/kernel/drivers/hisi/modem/drv/include/
subdir-ccflags-y += -I$(srctree)/drivers/hisi/modem/drv/include/
subdir-ccflags-y += -I$(srctree)/drivers/hisi/modem/include/nv/tl/drv/
subdir-ccflags-y += -I$(srctree)/drivers/hisi/modem/include/nv/product/
subdir-ccflags-y += -I$(srctree)/drivers/hisi/modem/include/nv/tl/oam/
subdir-ccflags-y += -I$(srctree)/drivers/hisi/modem/include/nv/tl/lps/
subdir-ccflags-y += -I$(srctree)/drivers/hisi/modem/include/nv/acore/sys/
subdir-ccflags-y += -I$(srctree)/drivers/hisi/modem/include/nv/acore/drv/
subdir-ccflags-y += -I$(srctree)/drivers/hisi/modem/include/nv/acore/msp/
subdir-ccflags-y += -I$(srctree)/drivers/hisi/modem/include/nv/acore/pam/
subdir-ccflags-y += -I$(srctree)/drivers/hisi/modem/include/nv/acore/guc_as/
subdir-ccflags-y += -I$(srctree)/drivers/hisi/modem/include/nv/acore/guc_nas/
subdir-ccflags-y += -I$(srctree)/drivers/hisi/modem/include/nv/common/sys/
subdir-ccflags-y += -I$(srctree)/drivers/hisi/modem/include/nv/common/drv/
subdir-ccflags-y += -I$(srctree)/drivers/hisi/modem/include/nv/common/msp/
subdir-ccflags-y += -I$(srctree)/drivers/hisi/modem/include/nv/common/pam/
subdir-ccflags-y += -I$(srctree)/drivers/hisi/modem/include/nv/common/guc_as/
subdir-ccflags-y += -I$(srctree)/drivers/hisi/modem/include/nv/common/guc_nas/
subdir-ccflags-y += -I$(srctree)/drivers/hisi/modem/include/nv/common/tl_as
subdir-ccflags-y += -I$(srctree)/drivers/hisi/modem/include/phy/lphy/
subdir-ccflags-y += -I$(srctree)/drivers/hisi/modem/config/nvim/include/gu/
subdir-ccflags-y += -I$(srctree)/drivers/hisi/modem/include/taf/common/
subdir-ccflags-y += -I$(srctree)/drivers/hisi/modem/include/taf/acore/
subdir-ccflags-y += -I$(srctree)/drivers/hisi/modem/drv/diag/serivce/
subdir-ccflags-y += -I$(srctree)/drivers/hisi/modem/config/product/$(OBB_PRODUCT_NAME)/$(OBB_MODEM_CUST_CONFIG_DIR)/config

subdir-ccflags-y+= -I$(srctree)/drivers/hisi/modem/drv/icc
subdir-ccflags-y+= -I$(srctree)/drivers/hisi/modem/drv/rtc
subdir-ccflags-y+= -I$(srctree)/drivers/hisi/modem/drv/nvim
subdir-ccflags-y+= -I$(srctree)/drivers/hisi/modem/drv/mem \
                   -I$(srctree)/drivers/hisi/modem/drv/memory
subdir-ccflags-y+= -I$(srctree)/drivers/hisi/modem/drv/om \
                   -I$(srctree)/drivers/hisi/modem/drv/om/common \
                   -I$(srctree)/drivers/hisi/modem/drv/om/dump \
                   -I$(srctree)/drivers/hisi/modem/drv/om/log
subdir-ccflags-y+= -I$(srctree)/drivers/hisi/modem/drv/udi
subdir-ccflags-y+= -I$(srctree)/drivers/hisi/modem/drv/balong_timer
subdir-ccflags-y+= -I$(srctree)/drivers/hisi/modem/drv/nmi
subdir-ccflags-y+= -I$(srctree)/drivers/hisi/modem/drv/hds
subdir-ccflags-y+= -I$(srctree)/drivers/usb/gadget
subdir-ccflags-y+= -I$(srctree)/drivers/hisi/modem/drv/diag/scm \
                   -I$(srctree)/drivers/hisi/modem/drv/diag/cpm \
                   -I$(srctree)/drivers/hisi/modem/drv/diag/debug \
                   -I$(srctree)/drivers/hisi/modem/drv/diag/ppm \
                   -I$(srctree)/drivers/hisi/modem/drv/diag/soft_decode \
                   -I$(srctree)/drivers/hisi/modem/drv/diag/message
subdir-ccflags-y+= -I$(srctree)/drivers/hisi/modem/include/tools

ifneq ($(strip $(CFG_ATE_VECTOR)),YES)
drv-y           += adp/adp_ipc.o
drv-y           += adp/adp_icc.o
drv-y           += adp/adp_pm_om.o
drv-y           += adp/adp_version.o
drv-y           += adp/adp_socp.o
drv-y           += onoff/adp_onoff.o
drv-y           += adp/adp_om.o
drv-$(CONFIG_USB_GADGET)     += adp/adp_usb.o
ifeq ($(CONFIG_RFILE_SUPPORT),y)
ifeq ($(strip $(CFG_RFILE_USER)),YES)
drv-y           += rfile/adp_rfile_user.o
else
drv-y           += rfile/adp_rfile.o
endif
else
dry-y           += rfile/adp_rfile_stub.o
endif
drv-y           += adp/adp_nvim.o
drv-y           += adp/adp_reset.o
drv-y           += adp/adp_timer.o
drv-y           += adp/adp_wifi.o
drv-y           += adp/adp_mem_balong.o
drv-y           += adp/adp_cpufreq_balong.o
drv-y           += adp/adp_applog.o
drv-y           += adp/adp_hds.o
ifeq ($(strip $(CFG_CONFIG_BALONG_CORESIGHT)),YES)
drv-y           += adp/adp_coresight.o
endif
drv-y           += adp/adp_charger.o
drv-y           += adp/adp_mmc.o
drv-y           += adp/adp_dload.o
drv-y           += adp/adp_misc.o
drv-y           += adp/adp_vic.o
else
drv-y           += adp/adp_pm_om.o
drv-y           += adp/adp_reset.o
drv-y           += adp/adp_timer.o
drv-y           += adp/adp_vic.o
endif

drv-y           += block/block_balong.o
drv-y           += block/blk_ops_mmc.o

ifeq ($(strip $(CFG_FEATURE_HISOCKET)),FEATURE_ON)
drv-y           += adp/hisocket.o
endif
ifeq ($(strip $(CFG_FEATURE_SVLSOCKET)),YES)
drv-y 			+= svlan_socket/
endif
drv-y           += diag/adp/adp_diag.o
drv-y           += adp/adp_slt.o

ifeq ($(strip $(CFG_CONFIG_APPLOG)),YES)
drv-y       += applog/applog_balong.o
ifeq ($(strip $(CFG_CONFIG_MODULE_BUSSTRESS)),YES)
#drv-y       += applog/applog_balong_test.o
else
drv-$(CONFIG_ENABLE_TEST_CODE) += applog/applog_balong_test.o
endif
endif

ifeq ($(strip $(CFG_CONFIG_MODULE_TIMER)),YES)
drv-y   += balong_timer/timer_slice.o
drv-y   += balong_timer/hardtimer_arm.o
drv-y   += balong_timer/hardtimer_core.o
drv-y   += balong_timer/softtimer_balong.o
endif

ifeq ($(strip $(CFG_CONFIG_NMI)),YES)
drv-y   += nmi/nmi.o
endif

drv-$(CONFIG_BBP_ACORE) += bbp/bbp_balong.o

subdir-ccflags-y += -Idrivers/hisi/modem/drv/hds
ifeq ($(strip $(CFG_BOARD_TRACE)),YES)
drv-y     += board_trace/board_trace_balong.o
endif

ifeq ($(strip $(CFG_CONFIG_CSHELL)),YES)
drv-y += console/ringbuffer.o
drv-y += console/console.o
drv-y += console/virtshell.o
drv-y += console/cshell_port.o
drv-y += console/uart_dev.o
drv-y += console/con_platform.o
drv-y += console/cshell_logger.o
endif

ifeq ($(strip $(CFG_CONFIG_NR_CSHELL)),YES)
drv-y += console/nr_cshell_port.o
endif

ifeq ($(strip $(CFG_CONFIG_BALONG_MSG)),YES)
subdir-ccflags-y+= -Idrivers/hisi/modem/drv/msg
obj-y += msg/msg_base.o
obj-y += msg/msg_tskque.o
obj-y += msg/msg_event.o
obj-y += msg/msg_mem.o
obj-y += msg/msg_core.o
obj-y += msg/msg_eicc.o
obj-y += msg/msg_fixed.o
obj-y += msg/msg_test.o
endif

ifeq ($(strip $(CFG_CONFIG_EICC_V200)),YES)
subdir-ccflags-y += -Idrivers/hisi/modem/drv/eicc200
drv-y += eicc200/eicc_core.o
drv-y += eicc200/eicc_device.o
drv-y += eicc200/eicc_driver.o
drv-y += eicc200/eicc_dts.o
drv-y += eicc200/eicc_platform.o
drv-y += eicc200/eicc_debug.o
drv-y += eicc200/eicc_dump.o
drv-y += eicc200/eicc_pmrst.o
drv-y += eicc200/eicc_test.o
endif

ifneq ($(strip $(OBB_SEPARATE)),true)
ifeq ($(strip $(CFG_CONFIG_DIAG_SYSTEM)),YES)
subdir-ccflags-y+= -I$(srctree)/drivers/hisi/modem/drv/diag/scm \
                   -I$(srctree)/drivers/hisi/modem/drv/diag/cpm \
                   -I$(srctree)/drivers/hisi/modem/drv/diag/ppm \
                   -I$(srctree)/drivers/hisi/modem/drv/diag/debug \
                   -I$(srctree)/drivers/hisi/modem/drv/diag/comm \
                   -I$(srctree)/drivers/hisi/modem/drv/diag/report \
                   -I$(srctree)/drivers/hisi/modem/drv/diag/serivce \
                   -I$(srctree)/drivers/hisi/modem/drv/socp
subdir-ccflags-y+= -I$(srctree)/drivers/hisi/modem/include/tools

drv-y           += diag/cpm/diag_port_manager.o
drv-y           += diag/debug/diag_system_debug.o
drv-y           += diag/ppm/OmCommonPpm.o
drv-y           += diag/ppm/OmPortSwitch.o
drv-y           += diag/ppm/OmSocketPpm.o
drv-y           += diag/ppm/OmUsbPpm.o
drv-y           += diag/ppm/OmVcomPpm.o

drv-y           += diag/scm/scm_ind_dst.o
drv-y           += diag/scm/scm_ind_src.o
drv-y           += diag/scm/scm_cnf_dst.o
drv-y           += diag/scm/scm_cnf_src.o
drv-y           += diag/scm/scm_common.o
drv-y           += diag/scm/scm_debug.o
drv-y           += diag/scm/scm_init.o
drv-y           += diag/comm/diag_comm.o
drv-y           += diag/report/diag_report.o
drv-y           += diag/scm/scm_rate_ctrl.o
drv-y           += diag/serivce/diag_service.o
drv-y           += diag/serivce/msp_service.o

ifeq ($(strip $(CFG_CONFIG_DIAG_NETLINK)),YES)
drv-y           += diag_vcom/diag_vcom_main.o
drv-y           += diag_vcom/diag_vcom_handler.o
endif
endif
endif

ifeq ($(strip $(CFG_CONFIG_DLOCK)),YES)
drv-y           += dlock/dlock_balong.o
endif

ifeq ($(strip $(CFG_ENABLE_BUILD_OM)),YES)
drv-y           += dump/apr/dump_apr.o
drv-y           += dump/comm/dump_config.o
drv-y           += dump/comm/dump_logs.o
drv-y           += dump/comm/dump_debug.o
drv-y           += dump/comm/dump_log_agent.o
drv-y           += dump/comm/dump_logzip.o
drv-y           += dump/core/dump_core.o
drv-y           += dump/core/dump_exc_handle.o
drv-y           += dump/core/dump_boot_check.o
drv-y           += dump/mdmap/dump_area.o
drv-y           += dump/mdmap/dump_baseinfo.o
drv-y           += dump/mdmap/dump_hook.o
drv-y           += dump/mdmap/dump_mdmap_core.o
drv-y           += dump/mdmcp/dump_cp_agent.o
drv-y           += dump/mdmcp/dump_cp_core.o
drv-y           += dump/mdmcp/dump_cp_wdt.o
drv-y           += dump/mdmcp/dump_cp_logs.o
drv-y           += dump/mdmcp/dump_cphy_tcm.o
drv-y           += dump/mdmcp/dump_easyrf_tcm.o
drv-y           += dump/mdmcp/dump_lphy_tcm.o
drv-y           += dump/mdmcp/dump_sec_mem.o
ifeq ($(strip $(CFG_ENABLE_BUILD_DUMP_MDM_LPM3)),YES)
drv-y           += dump/mdmm3/dump_m3_agent.o
endif
ifeq ($(strip $(CFG_ENABLE_BUILD_NRRDR)),YES)
drv-y           += dump/mdmnr/nrrdr_agent.o
drv-y           += dump/mdmnr/nrrdr_core.o
drv-y           += dump/mdmnr/nrrdr_logs.o
endif

endif

subdir-ccflags-y+= -I$(srctree)/drivers/hisi/modem/drv/dump/comm\
                   -I$(srctree)/drivers/hisi/modem/drv/dump/mdmap\
                   -I$(srctree)/drivers/hisi/modem/drv/dump/mdmcp\
                   -I$(srctree)/drivers/hisi/modem/drv/dump/mdmnr\
                   -I$(srctree)/drivers/hisi/modem/drv/dump/core\
                   -I$(srctree)/drivers/hisi/modem/drv/dump/mdmm3\
                   -I$(srctree)/drivers/hisi/modem/drv/dump/apr

ifeq ($(strip $(CFG_CONFIG_ECDC)),YES)
drv-y           += ecdc/ecdc_serviceman.o
drv-y           += ecdc/ecdc_genlserver.o
drv-y           += ecdc/ecdc_panrpc.o
drv-y           += ecdc/llt_distribute.o
endif

ifeq ($(strip $(CFG_CONFIG_EFUSE)),YES)
drv-$(CONFIG_EFUSE_BALONG)      += efuse/efuse_balong.o
drv-$(CONFIG_EFUSE_BALONG_AGENT)    += efuse/efuse_balong_agent.o
endif
drv-y               += efuse/efuse_balong_ioctl.o

ifeq ($(strip $(CONFIG_BALONG_ESPE)),y)
drv-y               += espe/espe_core.o espe/espe_platform_balong.o espe/espe_interrupt.o espe/espe_desc.o espe/espe_ip_entry.o espe/espe_mac_entry.o espe/espe_port.o espe/espe_dbg.o espe/espe_modem_reset.o
drv-y               += espe/func/espe_sysfs.o
drv-y               += espe/func/espe_sysfs_main.o
ifeq ($(strip $(CFG_CONFIG_ESPE_DIRECT_FW)),YES)
drv-y               += espe/func/espe_direct_fw.o
endif
endif

ifeq ($(strip $(CONFIG_AP_WARM_UP)),y)
drv-y               += ap_warm_up.o
endif



ifeq ($(strip $(CFG_CONFIG_CCPU_FIQ_SMP)),YES)
drv-y           += fiq/fiq_smp.o
else
drv-y           += fiq/fiq.o
endif

ifeq ($(strip $(CFG_CONFIG_HAS_NRCCPU)),YES)
drv-y           += fiq/fiq_smp_5g.o
endif

drv-y               += hds/bsp_hds_service.o hds/bsp_hds_log.o hds/bsp_hds_ind.o

subdir-ccflags-y += -Idrivers/hisi/modem/drv/om/common \
                    -Idrivers/hisi/modem/drv/om/oms\
                    -Idrivers/hisi/modem/drv/om/log \
                    -Idrivers/hisi/modem/drv/om/sys_view \
                    -Idrivers/modem/drv/om/usbtrace

drv-$(CONFIG_HW_IP_BASE_ADDR)   += hwadp/hwadp_balong.o hwadp/hwadp_core.o hwadp/hwadp_debug.o

ifeq ($(strip $(CFG_CONFIG_MALLOC_UNIFIED)),YES)
drv-y               += hwadp/malloc_interface.o
endif
subdir-ccflags-y += -I$(srctree)/drivers/hisi/tzdriver
drv-$(CONFIG_ICC_BALONG)        += icc/icc_core.o icc/icc_linux.o icc/icc_debug.o
drv-$(CONFIG_CA_ICC)            += icc/ca_icc.o
drv-$(CONFIG_ENABLE_TEST_CODE)  += icc/icc_test.o

ifeq ($(strip $(CFG_CONFIG_MODULE_IPC)),YES)
drv-$(CONFIG_IPC_DRIVER)        += ipc/ipc_balong.o
drv-$(CONFIG_ENABLE_TEST_CODE)  += ipc/ipc_balong_test.o
endif

ifeq ($(strip $(CFG_CONFIG_MODULE_IPC_FUSION)),YES)
drv-$(CONFIG_IPC_DRIVER)        += ipc/ipc_fusion.o
drv-y  += ipc/ipc_fusion_test.o
endif

ifeq ($(strip $(CFG_CONFIG_BALONG_IPC_MSG_V230)),YES)
drv-$(CONFIG_IPC_MSG_DRIVER) += ipcmsg/ipc_msg.o
drv-$(CONFIG_IPC_MSG_TEST)   += ipcmsg/ipc_msg_test.o
endif

ifneq ($(strip $(CFG_CONFIG_NEW_PLATFORM)),YES)
drv-$(CONFIG_IPF_SUPPORT)       += ipf/ipf_balong.o
endif

ifeq ($(strip $(OBB_LLT_MDRV)),y)
ifeq ($(strip $(llt_gcov)),y)
GCOV_PROFILE := y
drv-y += llt_tools/llt_gcov.o
endif
ifeq ($(strip $(LLTMDRV_HUTAF_COV)),true)
drv-y += llt_tools/ltcov.o
endif
drv-y += llt_tools/llt_tool.o
endif

ifeq ($(strip $(CFG_FEATURE_TDS_WCDMA_DYNAMIC_LOAD)),FEATURE_ON)
drv-y       += load_ps/bsp_loadps.o
endif

ifneq ($(strip $(CFG_CONFIG_MLOADER)),YES)
drv-y           += loadm/load_image.o
ifeq ($(strip $(CFG_CONFIG_COLD_PATCH)),YES)
drv-y           += loadm/modem_cold_patch.o
else
drv-y           += loadm/modem_cold_patch_stub.o
endif
ifeq ($(strip $(CFG_FEATURE_DELAY_MODEM_INIT)),FEATURE_ON)
drv-y           += loadm/loadm_phone.o
else
drv-y           += loadm/loadm_mbb.o
endif
endif

ifeq ($(bbit_type),nr)
subdir-ccflags-y += -DBBIT_TYPE_NR=FEATURE_ON
endif

drv-y               += log/bsp_om_log.o
drv-y               += adp/adp_print.o
ifeq ($(strip $(CFG_CONFIG_AXIMEM_BALONG)),YES)
drv-y               += aximem/aximem.o
endif
ifeq ($(strip $(CFG_CONFIG_SHARED_MEMORY)),YES)
drv-y              += memory/memory_driver.o
endif
ifeq ($(strip $(CFG_CONFIG_MAA_BALONG)),YES)
drv-y               += maa/maa_acore.o maa/bsp_maa.o
endif
drv-y               += mbb_modem_stub/mbb_modem_stub.o

ifeq ($(strip $(CONFIG_MEM_BALONG)),y)
drv-$(CONFIG_MEM_BALONG)    += mem/mem_balong.o
drv-$(CONFIG_ENABLE_TEST_CODE) += mem/mem_balong_test.o
endif

ifeq ($(strip $(CFG_CONFIG_MLOADER)),YES)
drv-y           += mloader/mloader_comm.o
drv-y           += mloader/mloader_load_ccore_imgs.o
drv-y           += mloader/mloader_load_dts.o
drv-y           += mloader/mloader_load_image.o
drv-y           += mloader/mloader_load_lr.o
drv-y           += mloader/mloader_load_patch.o
drv-y           += mloader/mloader_main.o
drv-y           += mloader/mloader_sec_call.o
drv-y           += mloader/mloader_debug.o
endif

ifeq ($(strip $(CFG_CONFIG_BALONG_TRANS_REPORT)),YES)
drv-y           += trans_report/trans_report_pkt.o
drv-y           += trans_report/trans_report_cnt.o
endif

drv-y           += mperf/s_mperf.o

ifeq ($(strip $(CFG_CONFIG_NEW_PLATFORM)),YES)
drv-$(CONFIG_IPF_SUPPORT)           += n_ipf/ipf_balong.o n_ipf/ipf_pm.o n_ipf/ipf_filter.o n_ipf/ipf_desc.o n_ipf/ipf_desc64.o n_ipf/ipf_debug.o
drv-$(CONFIG_PSAM_SUPPORT)          += n_psam/psam_balong.o n_psam/psam_hal32.o n_psam/psam_hal64.o n_psam/psam_debug.o
endif

drv-y           += net_helper/ip_limit.o

ifeq ($(strip $(CFG_CONFIG_NRDSP)),YES)
drv-y           += nrdsp/nrdsp_dump_tcm.o
endif

EXTRA_CFLAGS += -I$(srctree)/drivers/hisi/modem/include/nv/product/
ifneq ($(strip $(CFG_FEATURE_NV_SEC_ON)),YES)
ifeq ($(strip $(CFG_FEATURE_NV_FLASH_ON)),YES)
drv-$(CONFIG_NVIM)          += nvim/nv_flash.o
endif
ifeq ($(strip $(CFG_FEATURE_NV_EMMC_ON)),YES)
drv-$(CONFIG_NVIM)          += nvim/nv_emmc.o
endif
drv-$(CONFIG_NVIM)          += nvim/nv_ctrl.o \
                               nvim/nv_comm.o \
                               nvim/nv_base.o \
                               nvim/nv_cust.o \
                               nvim/nv_xml_dec.o \
                               nvim/nv_debug.o \
                               nvim/nv_crc.o \
                               nvim/nv_index.o \
                               nvim/nv_partition_upgrade.o \
                               nvim/nv_partition_img.o \
                               nvim/nv_partition_bakup.o \
                               nvim/nv_factory_check.o \
                               nvim/nv_msg.o \
                               nvim/nv_proc.o
endif

drv-$(CONFIG_PMIC_OCP)      += ocp/pmic_ocp.o

ifeq ($(strip $(CFG_CONFIG_PHONE_DCXO_AP)),YES)
drv-y           += ocp/pmic_dcxo.o
endif

drv-$(CONFIG_BALONG_ONOFF)  += onoff/power_para.o
drv-$(CONFIG_BALONG_MODEM_ONOFF)    += onoff/power_exchange.o
drv-$(CONFIG_BALONG_MODEM_ONOFF)    += onoff/power_on.o
drv-$(CONFIG_BALONG_MODEM_ONOFF)    += onoff/power_off.o
drv-$(CONFIG_BALONG_MODEM_ONOFF)    += onoff/bsp_modem_boot.o

drv-$(CONFIG_PM_OM_BALONG) +=  pm_om/pm_om.o pm_om/pm_om_platform.o pm_om/pm_om_debug.o pm_om/pm_om_pressure.o pm_om/bsp_ring_buffer.o pm_om/modem_log_linux.o pm_om/pm_om_nrccpu.o pm_om/pm_om_hac.o
drv-$(CONFIG_PM_OM_BALONG_TEST) +=  pm_om/pm_om_test.o

ifneq ($(strip $(CFG_CONFIG_NEW_PLATFORM)),YES)
drv-$(CONFIG_PSAM_SUPPORT)      += psam/psam_balong.o
endif

drv-$(CONFIG_BALONG_MODEM_RESET)+= reset/reset_balong.o

ifeq ($(strip $(CFG_CONFIG_RFILE_USER)),YES)
drv-$(CONFIG_RFILE_SUPPORT)     += rfile/rfile_balong_user.o
else
drv-$(CONFIG_RFILE_SUPPORT)     += rfile/rfile_balong.o
endif
drv-$(CONFIG_ENABLE_TEST_CODE)  += rfile/rfile_api_test.o



drv-y += s_memory/s_memory.o
drv-$(CONFIG_S_MEMORY_TEST)     += s_memory/s_memory_test.o

ifeq ($(strip $(CFG_CONFIG_SC)),YES)
drv-y                           += sc/sc_balong.o
endif

drv-$(CONFIG_SEC_CALL)          += sec_call/sec_call.o

ifeq ($(strip $(CFG_FEATURE_NV_SEC_ON)),YES)
drv-$(CONFIG_NVIM)              += sec_nvim/nv_ctrl.o \
                                   sec_nvim/nv_comm.o \
                                   sec_nvim/nv_base.o \
                                   sec_nvim/nv_emmc.o \
                                   sec_nvim/nv_debug.o \
                                   sec_nvim/nv_index.o \
                                   sec_nvim/nv_partition_upgrade.o \
                                   sec_nvim/nv_partition_img.o \
                                   sec_nvim/nv_partition_bakup.o \
                                   sec_nvim/nv_partition_factory.o \
                                   sec_nvim/nv_factory_check.o \
                                   sec_nvim/nv_msg.o \
                                   sec_nvim/nv_proc.o \
                                   sec_nvim/nv_verify.o
endif

drv-$(CONFIG_HISI_SIM_HOTPLUG_SPMI)     += sim_hotplug/hisi_sim_hotplug_spmi.o

subdir-ccflags-y    +=  -I$(srctree)/drivers/hisi/modem/drv/socp/
subdir-ccflags-y    +=  -I$(srctree)/drivers/hisi/modem/drv/socp/soft_decode
subdir-ccflags-y    +=  -I$(srctree)/drivers/hisi/modem/drv/socp/encode
subdir-ccflags-y    +=  -I$(srctree)/drivers/hisi/modem/drv/socp/deflate
ifeq ($(strip $(CFG_ENABLE_BUILD_SOCP)),YES)
ifeq ($(strip $(CFG_DIAG_SYSTEM_5G)),YES)
drv-y       += socp/socp_enc.o
drv-y       += socp/encode/socp_enc_hal.o
else
drv-y       += socp/socp_balong.o
endif
drv-y       += socp/socp_ind_delay.o socp/socp_debug.o
ifeq ($(strip $(CFG_CONFIG_DEFLATE)),YES)
drv-y       += socp/deflate/deflate.o
endif
ifneq ($(strip $(OBB_SEPARATE)),true)
ifeq ($(strip $(CFG_CONFIG_DIAG_SYSTEM)),YES)
drv-y       += socp/soft_decode/hdlc.o
drv-y       += socp/soft_decode/ring_buffer.o
drv-y       += socp/soft_decode/soft_decode.o
drv-y       += socp/soft_decode/soft_cfg.o
endif
endif
endif

ifeq ($(strip $(CFG_CONFIG_BBPDS)),YES)
drv-y      += bbpds/bbpds.o 
endif

drv-$(CONFIG_SYNC_BALONG)       += sync/sync_balong.o
drv-$(CONFIG_ENABLE_TEST_CODE)  += sync/sync_balong_test.o

ifeq ($(strip $(CFG_CONFIG_SYSBUS)),YES)
drv-y += sys_bus/sys_bus_core.o
drv-y += sys_bus/sys_bus_single.o
drv-y += sys_bus/sys_bus_pressure.o
drv-y += sys_bus/sys_monitor.o
endif

drv-y += sysboot/sysboot_balong.o
drv-y += sysctrl/sysctrl.o

drv-$(CONFIG_UDI_SUPPORT)   += udi/udi_balong.o udi/adp_udi.o

drv-y       += version/version_balong.o

ifeq ($(strip $(CFG_CONFIG_WAN)),YES)
drv-y       += wan/ipf_ap.o wan/wan.o
else
drv-y       += wan/wan_stub.o
endif

#trng_seed
ifeq ($(strip $(CFG_CONFIG_TRNG_SEED)),YES)
drv-y += trng_seed/trng_seed.o
endif


ifeq ($(strip $(CFG_CONFIG_AVS_TEST)),YES)
drv-y         += avs/avs_test.o
endif

# module makefile end

else

ifneq ($(strip $(CFG_ATE_VECTOR)),YES)
obj-y               += sysctrl/
obj-y               += s_memory/
obj-y               += mperf/
obj-y               += fiq/
ifeq ($(strip $(CFG_CONFIG_MODULE_TIMER)),YES)
obj-y               += balong_timer/
endif
ifeq ($(strip $(CFG_CONFIG_NMI)),YES)
obj-y               += nmi/
endif
ifeq ($(strip $(CFG_CONFIG_MODULE_IPC)),YES)
obj-y               += ipc/
endif
ifeq ($(strip $(CFG_CONFIG_MODULE_IPC_FUSION)),YES)
obj-y               += ipc/
endif
ifeq ($(strip $(CFG_CONFIG_ICC)),YES)
obj-y               += icc/
endif

ifeq ($(strip $(CFG_CONFIG_BALONG_MSG)),YES)
obj-y               += msg/
endif

ifeq ($(strip $(CFG_CONFIG_EICC_V200)),YES)
obj-y               += eicc200/
endif

obj-y               += reset/
obj-y               += sec_call/

ifeq ($(strip $(CFG_CONFIG_ECDC)),YES)
obj-y               += ecdc/
endif
obj-y               += rfile/
ifeq ($(strip $(CFG_FEATURE_TDS_WCDMA_DYNAMIC_LOAD)),FEATURE_ON)
obj-y               += load_ps/
endif
obj-y               += sync/
ifeq ($(strip $(CFG_ENABLE_BUILD_SOCP)),YES)
obj-y           += socp/
endif
ifeq ($(strip $(CFG_CONFIG_BBPDS)),YES)
obj-y           += bbpds/
endif
ifeq ($(strip $(CFG_CONFIG_NRDSP)),YES)
obj-y               += nrdsp/
endif
ifeq ($(strip $(CFG_ENABLE_BUILD_OM)),YES)
obj-y               += dump/
endif
ifneq ($(strip $(CFG_FEATURE_NV_SEC_ON)),YES)
obj-y               += nvim/
endif

ifeq ($(strip $(CFG_CONFIG_MLOADER)),YES)
obj-y += mloader/
endif

ifeq ($(strip $(CFG_FEATURE_NV_SEC_ON)),YES)
obj-y               += sec_nvim/
endif

ifeq ($(strip $(CONFIG_IMAGE_LOAD)),y)
obj-y += loadm/
else
ifneq ($(strip $(CFG_CONFIG_MLOADER)),YES)
obj-y += mloader/
endif
endif

obj-y           += adp/
obj-y           += block/
obj-y           += hwadp/

obj-y           += version/

#trng_seed
ifeq ($(strip $(CFG_CONFIG_TRNG_SEED)),YES)
obj-y += trng_seed/
endif

obj-y           += log/

ifeq ($(strip $(CFG_FEATURE_SVLSOCKET)),YES)
obj-y += svlan_socket/
endif

obj-y               += diag/

ifneq ($(strip $(OBB_SEPARATE)),true)
ifeq ($(strip $(CFG_CONFIG_DIAG_SYSTEM)),YES)
ifeq ($(strip $(CFG_CONFIG_DIAG_NETLINK)),YES)
obj-y               += diag_vcom/
endif
endif
endif

obj-y           += hds/

ifeq ($(strip $(CFG_BOARD_TRACE)),YES)
obj-y           += board_trace/
endif

ifeq ($(strip $(CFG_CONFIG_SYSBUS)),YES)
obj-y           += sys_bus/
endif

ifeq ($(strip $(CFG_CONFIG_AXIMEM_BALONG)),YES)
obj-y               += aximem/
endif
ifeq ($(strip $(CFG_CONFIG_SHARED_MEMORY)),YES)
obj-y           += memory/
endif
ifeq ($(strip $(CFG_CONFIG_MAA_BALONG)),YES)
obj-y           += maa/
endif


obj-$(CONFIG_BALONG_ESPE)   += espe/

obj-$(CONFIG_AP_WARM_UP)   += ap_warm_up/

ifeq ($(strip $(CFG_CONFIG_NEW_PLATFORM)),YES)
obj-y           += n_ipf/
obj-y           += n_psam/
else
obj-y           += ipf/
obj-y           += psam/
endif

obj-y           += udi/

obj-$(CONFIG_MEM_BALONG)    += mem/

obj-y   += onoff/

obj-y   += efuse/

ifeq ($(strip $(CFG_CONFIG_CSHELL)),YES)
obj-y += console/
endif

ifeq ($(strip $(CFG_CONFIG_SC)),YES)
obj-y               += sc/
endif
obj-y           += pm_om/

obj-$(CONFIG_PMIC_OCP) += ocp/

# llt module
ifeq ($(strip $(OBB_LLT_MDRV)),y)
obj-y           += llt_tools/
endif

ifeq ($(strip $(llt_gcov)),y)
subdir-ccflags-y += -fno-inline
endif

ifeq ($(strip $(CFG_CONFIG_MODULE_BUSSTRESS)),YES)
obj-y                   += busstress/$(OBB_PRODUCT_NAME)/
endif

obj-$(CONFIG_HISI_SIM_HOTPLUG_SPMI)     += sim_hotplug/

else
obj-y               += sysctrl/
obj-y               += s_memory/

obj-y               += mperf/

ifeq ($(strip $(CFG_CONFIG_MODULE_TIMER)),YES)
obj-y               += balong_timer/
endif
obj-y               += icc/
obj-y               += reset/
obj-y               += adp/
obj-y               += hwadp/
ifeq ($(strip $(CFG_ENABLE_BUILD_OM)),YES)
obj-y               += om/
endif
obj-y               += pm_om/
endif
ifeq ($(strip $(CFG_CONFIG_APPLOG)),YES)
obj-y               += applog/
endif
ifeq ($(strip $(CFG_CONFIG_DLOCK)),YES)
obj-y               += dlock/
endif
obj-$(CONFIG_BBP_ACORE)             += bbp/
obj-y               += net_helper/
obj-y               += mbb_modem_stub/

ifeq ($(strip $(CFG_FEATURE_PC5_DATA_CHANNEL)),FEATURE_ON)
obj-y				+= pcv_adaptor/pcv_adaptor.o
endif

ifeq ($(strip $(CFG_CONFIG_WAN)),YES)
obj-y               += wan/
else
obj-y               += wan/wan_stub.o
endif

obj-$(CONFIG_PCIE_BALONG_DEV)          += pcie_balong_dev/

ifeq ($(strip $(CFG_CONFIG_VCOM_AGENT)),YES)
obj-y += vcom/vcom_agent.o
endif

ifeq ($(strip $(CFG_CONFIG_BALONG_PCIE_CDEV)),YES)
obj-y += pfunc/
endif

ifeq ($(strip $(CFG_CONFIG_BALONG_WETH_DEV)),YES)
obj-y         += wan_eth/
endif

ifeq ($(strip $(CFG_CONFIG_MODEM_BOOT)),YES)
obj-y += modem_boot/modem_boot_mbb.o
endif

ifeq ($(strip $(CFG_CONFIG_BALONG_TRANS_REPORT)),YES)
obj-y += trans_report/
endif

ifeq ($(strip $(CFG_CONFIG_BALONG_IPC_MSG_V230)),YES)
obj-y         += ipcmsg/
endif

ifeq ($(strip $(CFG_CONFIG_AVS_TEST)),YES)
obj-y         += avs/
endif

endif


ifeq ($(strip $(CFG_CONFIG_FUZZ_MDRV)),YES)
include $(srctree)/../../vendor/hisi/llt/mdrv/api_fuzz/build/makefile_acore.mk
endif

