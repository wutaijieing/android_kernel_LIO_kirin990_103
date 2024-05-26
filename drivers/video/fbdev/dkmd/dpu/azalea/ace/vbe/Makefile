ifeq ($(CONFIG_DPU_CDC_ACE),y)
obj-$(CONFIG_DPU_CDC_ACE) := dpufb.o
endif

ccflags-y += -Wextra -Wdate-time -Wfloat-equal -Wshadow -Wformat=2

dpufb-objs := \
	dpu_mipi_dsi_host.o \
	dpu_mipi_dsi.o \
	dpu_dpe.o \
	dpu_fb_panel.o \
	dpu_fb_isr.o \
	dpu_fb_vsync.o \
	dpu_fb_bl.o \
	dpu_fb_utils.o \
	dpu_fb.o \
	dpu_fb_dts.o \
	dpu_fb_debug.o \
	dpu_smmu_base.o \
	dpu_isr_dev.o \
	overlay/dpu_overlay_utils.o \
	overlay/clip/dpu_clip.o \
	overlay/dfc/dpu_dfc.o \
	overlay/dump/dpu_dump.o \
	overlay/handler/dpu_handler.o \
	overlay/init/dpu_init.o \
	overlay/mctl/dpu_mctl.o \
	overlay/mif/dpu_mif.o \
	overlay/ovl/dpu_ovl.o \
	overlay/rdma/dpu_rdma.o \
	overlay/scl/dpu_scl.o \
	overlay/tool/dpu_tool.o \
	dpu_overlay_online.o \
	dpu_block_algorithm.o \
	dpu_overlay_cmdlist_utils.o \
	performance/dpu_performance.o \
	dpu_display_effect.o \
	dpu_mmbuf_manager.o \
	dpu_fb_sysfs.o \
	dpu_iommu.o \
	chrdev/dpu_chrdev.o \
	merger_mgr/dpu_disp_merger_mgr.o \
	fault_mgr/dpu_fault_mgr.o \
	fault_mgr/dpu_fault_strategy.o \
	merger_mgr/dpu_frame_buffer_mgr.o \
	dpu_disp_recorder.o

ifeq ($(CONFIG_DPU_FB_V510),y)
dpufb-objs += \
	dpu_dpe_utils_dssv510.o \
	dpu_dpe_utils_clk_dssv510.o \
	dpu_dpe_clk_sub_dssv510.o \
	dpu_dpe_effect_utilsv510.o \
	performance/dpu_performance_v510.o \
	dpu_display_effect_dssv510.o \
	overlay/dpu_overlay_utils_dssv510.o \
	overlay/aif/dpu_aif_dssv510.o \
	overlay/arsr/dpu_arsr_dssv510.o \
	overlay/bce_bcd/dpu_bce_bcd_dssv510.o \
	overlay/clip/dpu_clip_dssv510.o \
	overlay/csc/dpu_csc_dssv510.o \
	overlay/mctl/dpu_mctl_dssv510.o \
	overlay/ovl/dpu_ovl_dssv510.o \
	overlay/scf/dpu_scf_dssv510.o \
	dpu_mipi_dsi_v510.o \
	panel/dssv510/mipi_samsung_ea8074.o \
	panel/dssv510/mipi_lg_td4322.o
endif

ifeq ($(CONFIG_ICS_DRIVER),y)
dpufb-objs += \
	panel/cdc/video/ics_dsi_panel.o
endif

ifeq ($(CONFIG_PERI_DVFS),y)
EXTRA_CFLAGS += -Ivendor/drivers/clk/dvfs/
endif