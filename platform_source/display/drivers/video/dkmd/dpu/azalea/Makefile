obj-$(CONFIG_DPU_FB_FAKE) := dpufb_fake.o

ccflags-y += -Wextra -Wdate-time -Wfloat-equal -Wshadow -Wformat=2

dpufb_fake-objs := \
	dpu_fb_fake.o

ifeq ($(CONFIG_FB),y)
obj-$(CONFIG_FB) := dpufb.o
endif

ifeq ($(CONFIG_DPU_FB_MEDIACOMMON),y)
obj-$(CONFIG_DPU_FB_MEDIACOMMON) += mediacommon/
endif

ifeq ($(CONFIG_DPU_FB_ALSC),y)
obj-$(CONFIG_DPU_FB_ALSC) += effect/display_effect_alsc.o
endif

ifeq ($(CONFIG_SYNC_FILE),y)
obj-$(CONFIG_SYNC_FILE) += dpu_sync.o
endif

ifeq ($(CONFIG_DSS_SMMU_V3),y)
obj-$(CONFIG_DSS_SMMU_V3) += dpu_smmuv3.o
endif

ifeq ($(CONFIG_DPU_DISPLAY_DFR),y)
obj-$(CONFIG_DPU_DISPLAY_DFR) += dpu_frame_rate_ctrl.o
endif

ifeq ($(CONFIG_DPU_DPM),y)
obj-$(CONFIG_DPU_DPM) += dpu_dpm.o
endif

ifeq ($(CONFIG_DPU_DPP_CMDLIST),y)
obj-$(CONFIG_DPU_DPP_CMDLIST) += dpu_dpp_cmdlist_utils.o
endif

ifeq ($(CONFIG_VIDEO_IDLE),y)
obj-$(CONFIG_VIDEO_IDLE) += dpu_fb_video_idle.o
endif

ifeq ($(CONFIG_DPU_FB_AOD),y)
obj-$(CONFIG_DPU_FB_AOD) += dpu_sensorhub_aod.o
endif

ifeq ($(CONFIG_ASYNCHRONOUS_PLAY),y)
obj-$(CONFIG_ASYNCHRONOUS_PLAY) += dpu_overlay_asynchronous_play.o
endif

ifeq ($(CONFIG_RCH_CLD),y)
obj-$(CONFIG_RCH_CLD) += overlay/cld/dpu_cld.o
endif

ifeq ($(CONFIG_DPU_EFFECT_HIHDR),y)
obj-$(CONFIG_DPU_EFFECT_HIHDR) += dpu_hihdr.o dpu_hihdr_itm.o dpu_hihdr_gtm.o
endif

dpufb-objs := \
	dpu_mipi_dsi_host.o \
	dpu_mipi_dsi.o \
	dpu_pwm.o \
	dpu_blpwm.o \
	dpu_sensorhub_blpwm.o \
	dpu_dpe.o \
	dpu_fb_panel.o \
	dpu_fb_isr.o \
	dpu_fb_vsync.o \
	dpu_fb_esd.o \
	dpu_fb_secure.o \
	dpu_fb_buf_sync.o \
	dpu_fb_bl.o \
	dpu_fb_utils.o \
	dpu_fb.o \
	dpu_fb_dts.o \
	dpu_fb_debug.o \
	dpu_mdc_resource.o \
	dpu_smmu_base.o \
	overlay/dpu_overlay_utils.o \
	overlay/clip/dpu_clip.o \
	overlay/dfc/dpu_dfc.o \
	overlay/dirty/dpu_dirty.o \
	overlay/dump/dpu_dump.o \
	overlay/handler/dpu_handler.o \
	overlay/init/dpu_init.o \
	overlay/mctl/dpu_mctl.o \
	overlay/mif/dpu_mif.o \
	overlay/ovl/dpu_ovl.o \
	overlay/rdma/dpu_rdma.o \
	overlay/scl/dpu_scl.o \
	overlay/tool/dpu_tool.o \
	overlay/wdma/dpu_wdma.o \
	dpu_overlay_online.o \
	dpu_overlay_offline.o \
	dpu_block_algorithm.o \
	dpu_overlay_cmdlist_utils.o \
	dpu_offlinecomposer_panel.o \
	panel/mipi_lcd_utils.o \
	performance/dpu_performance.o \
	dpu_display_effect.o \
	dpu_mmbuf_manager.o \
	dpu_fb_sysfs.o \
	product/dpu_displayengine_utils.o \
	product/dpu_displayengine_effect.o \
	panel_old/mipi_fake_panel.o \
	product/dpu_panel_switch.o

ifeq ($(CONFIG_DEVICE_ATTRS),y)
dpufb-objs += \
	product/attrs/dpu_fb_attrs.o \
	product/attrs/dpu_dpe_attrs.o \
	product/attrs/dpu_mipi_attrs.o \
	product/attrs/dpu_panel_attrs.o
endif

ifneq ($(PRODUCT_RANGE),base)
-include $(src)/Makefile.dsm
endif

dpufb-objs += \
	dpu_ion.o \
	dpu_iommu.o

ifneq ($(TARGET_BUILD_VARIANT),user)
dpufb-objs += \
	dpu_fb_panel_debug.o
endif

ifneq ($(filter $(CONFIG_DPU_FB_V410) $(CONFIG_DPU_FB_V501) $(CONFIG_DPU_FB_V510),y),)
dpufb-objs += \
	dp/dp_dbg.o \
	dp/controller/dp_avgen_base.o \
	dp/controller/hidpc/hidpc_dp_aux.o \
	dp/controller/hidpc/hidpc_dp_irq.o \
	dp/controller/hidpc/hidpc_dp_avgen.o \
	dp/controller/hidpc/hidpc_dp_core.o \
	dp/phy/hidpc_phy_config.o \
	dp/link/dp_irq.o \
	dp/link/dp_aux.o \
	dp/link/dp_edid.o \
	dp/link/dp_link_training.o \
	dp/link/dp_mst_topology.o \
	dp/link/drm_dp_helper.o \
	dsc/dsc_algorithm.o \
	dp/controller/dsc/hidpc_dsc_config.o \
	dp/controller/dsc/dsc_config_base.o \
	dp/link/dp_dsc_algorithm.o \
	dsc/utilities/dpu_debug.o \
	dsc/dsc_algorithm_manager.o \
	panel/dp_fake_panel.o

ifeq ($(CONFIG_DP_HDCP),y)
dpufb-objs += \
	hdcp/esm/host_lib_driver_linux_kernel.o \
	hdcp/hdcp_common.o \
	hdcp/hdcp_driver.o \
	hdcp/hdcp_reg_esm.o
endif
endif

ifneq ($(filter $(CONFIG_DPU_FB_V600) $(CONFIG_DPU_FB_V360),y),)
dpufb-objs += \
	dp/dp_dbg.o \
	dp/controller/dp_avgen_base.o \
	dp/controller/hidptx/hidptx_dp_aux.o \
	dp/controller/hidptx/hidptx_dp_irq.o \
	dp/controller/hidptx/hidptx_dp_avgen.o \
	dp/controller/hidptx/hidptx_dp_core.o \
	dp/phy/hidptx_phy_config.o \
	dp/link/dp_irq.o \
	dp/link/dp_aux.o \
	dp/link/dp_edid.o \
	dp/link/dp_link_training.o \
	dp/link/dp_mst_topology.o \
	dp/link/drm_dp_helper.o \
	dp/controller/dsc/hidptx_dsc_config.o \
	dp/controller/dsc/dsc_config_base.o \
	dp/link/dp_dsc_algorithm.o \
	panel/dp_fake_panel.o

ifeq ($(CONFIG_DP_HDCP),y)
dpufb-objs += \
	hdcp/hdcp_common.o \
	hdcp/hdcp_driver.o \
	hdcp/hdcp_reg_soft.o

EXTRA_CFLAGS += -DCONFIG_HDCP_SOFT_AUTH
endif
endif

ifeq ($(CONFIG_DPU_FB_V410),y)
dpufb-objs += \
	dpu_dp.o \
	dpu_dp_hpd.o \
	dpu_dpe_utils_dssv410.o \
	dpu_display_effect_dssv410.o \
	overlay/dpu_overlay_utils_dssv410.o \
	overlay/dump/dpu_dump_register.o \
	dpu_mipi_dsi_v410.o \
	backlight/lm36274.o \
	backlight/lm36923.o \
	backlight/lp8556.o \
	backlight/rt8555.o

dpufb-objs += voltage/tps65132.o
dpufb-objs += voltage/rt4801h.o
dpufb-objs += voltage/ina231.o

ifeq ($(CONFIG_DFX_NOC_MODID_REGISTER),y)
dpufb-objs += \
	dpu_mntn.o \
	dpu_mntn_dssv410.o
endif
endif

ifeq ($(CONFIG_DPU_FB_V501),y)
dpufb-objs += \
	dpu_dp.o \
	dpu_dp_hpd.o \
	dpu_dpe_utils_dssv501.o \
	dpu_display_effect_dssv501.o \
	overlay/dpu_overlay_utils_dssv501.o \
	overlay/aif/dpu_aif_dssv501.o \
	overlay/arsr/dpu_arsr_dssv501.o \
	overlay/bce_bcd/dpu_bce_bcd_dssv501.o \
	overlay/clip/dpu_clip_dssv510.o \
	overlay/csc/dpu_csc_dssv501.o \
	overlay/mctl/dpu_mctl_dssv510.o \
	overlay/ovl/dpu_ovl_dssv510.o \
	overlay/scf/dpu_scf_dssv501.o \
	overlay/scl/dpu_scl_dssv510.o \
	dpu_dpe_pipe_clk_utils.o \
	dpu_fb_hisync.o \
	dpu_mipi_dsi_v501.o \
	dpu_ovl_online_wb.o \
	panel_old/dssv501/mipi_lg_TD4322.o \
	panel_old/dssv501/mipi_jdi_R63452.o \
	panel_old/dssv501/mipi_sharp_NT36870.o

dpufb-objs += voltage/ina231.o
dpufb-objs += voltage/tps65132.o
ifeq ($(CONFIG_DFX_NOC_MODID_REGISTER),y)
dpufb-objs += \
	dpu_mntn.o \
	dpu_mntn_dssv501.o
endif

dpufb-objs += dpu_hiace_single_mode.o
EXTRA_CFLAGS += -DHIACE_SINGLE_MODE_SUPPORT
endif

ifeq ($(CONFIG_DPU_FB_V510),y)
dpufb-objs += \
	dpu_dp.o \
	dpu_dp_hpd.o \
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
	overlay/scl/dpu_scl_dssv510.o \
	dpu_dpe_pipe_clk_utils.o \
	dpu_fb_hisync.o \
	dpu_mipi_dsi_v510.o \
	spr/dpu_spr.o \
	spr/dpu_spr_1_0.o \
	panel/dssv510/mipi_samsung_EA8076.o \
	backlight/lm36923.o

ifeq ($(CONFIG_ICS_DRIVER),y)
ifeq ($(chip_type),cs2)
dpufb-objs += \
	panel/cdc/kernel/drivers/video/thermal/temperature.o \
	panel/cdc/kernel/drivers/video/x1_quadpanel/serdes_ops_and_init.o \
	panel/cdc/kernel/drivers/video/x1_quadpanel/smart_panel/algorithm/x1_display_param.o \
	panel/cdc/kernel/drivers/video/x1_quadpanel/smart_panel/x1_smart_panel.o \
	panel/cdc/kernel/drivers/video/x1_quadpanel/smart_panel/drv_indigo.o \
	panel/cdc/kernel/drivers/video/x1_quadpanel/meter_panel/x1_meter_panel.o \
	panel/cdc/kernel/drivers/video/x1_quadpanel/meter_panel/dic702a_spi.o
else
dpufb-objs += \
	panel/cdc/kernel/drivers/mipi2lvds/serdes_i2c.o \
	panel/cdc/kernel/drivers/mipi2lvds/mipi2lvds_serdes_init.o \
	panel/cdc/kernel/drivers/mipi2lvds/mipi_lvds_panel.o \
	panel/cdc/kernel/drivers/mipi2lvds/drv_indigo.o \
	panel/cdc/kernel/drivers/lcd_panel/pannel_core.o \
	panel/cdc/kernel/drivers/lcd_panel/c385/c385_pannel.o \
	panel/cdc/kernel/drivers/lcd_panel/c385/c385_touchscreen.o \
	panel/cdc/kernel/drivers/lcd_panel/e11/serdes_ops_and_init.o \
	panel/cdc/kernel/drivers/lcd_panel/e11/smart_panel/algorithm/e11_display_param.o \
	panel/cdc/kernel/drivers/lcd_panel/e11/smart_panel/e11_smart_panel.o \
	panel/cdc/kernel/drivers/lcd_panel/e11/smart_panel/drv_indigo.o \
	panel/cdc/kernel/drivers/lcd_panel/e11/sangde_panel/e11_panel.o \
	panel/cdc/kernel/drivers/lcd_panel/e11/sangde_panel/e11_touchscreen.o

endif
endif

ifeq ($(CONFIG_DPU_CDC_ACE),y)
obj-y += ace/vfe/
endif

dpufb-objs += voltage/ina231.o
dpufb-objs += voltage/tps65132.o

ifeq ($(CONFIG_DFX_NOC_MODID_REGISTER),y)
dpufb-objs += \
	dpu_mntn.o \
	dpu_mntn_dssv510.o
endif

dpufb-objs += dpu_hiace_single_mode.o
EXTRA_CFLAGS += -DHIACE_SINGLE_MODE_SUPPORT
endif

ifeq ($(CONFIG_POWER_DUBAI_RGB_STATS),y)
dpufb-objs += product/rgb_stats/dpu_fb_rgb_stats.o
endif

ifeq ($(CONFIG_DPU_FB_V320),y)
dpufb-objs += \
	dpu_display_effect_dssv320.o \
	dpu_dpe_effect_utilsv320.o \
	dpu_dpe_utils_dssv320.o \
	overlay/dpu_overlay_utils_dssv320.o \
	overlay/dump/dpu_dump_register.o \
	overlay/aif/dpu_aif_dssv320.o \
	overlay/arsr/dpu_arsr_dssv320.o \
	overlay/clip/dpu_clip_dssv330.o \
	overlay/csc/dpu_csc_dssv330.o \
	overlay/mctl/dpu_mctl_dssv330.o \
	overlay/ovl/dpu_ovl_dssv330.o \
	overlay/scf/dpu_scf_dssv330.o \
	dpu_mipi_dsi_v3xx.o \
	backlight/lm36274.o \
	backlight/lm36923.o \
	backlight/lp8556.o \
	backlight/rt8555.o \
	panel_old/dssv320/mipi_tm_OTM1911A.o \
	panel_old/dssv320/mipi_cmi_NT35521.o \
	panel/dssv320/mipi_boe_RM69330.o

dpufb-objs += \
	panel/dp_fake_panel.o

dpufb-objs += voltage/tps65132.o
dpufb-objs += voltage/rt4801h.o
dpufb-objs += voltage/ina231.o
ifeq ($(CONFIG_DFX_NOC_MODID_REGISTER),y)
dpufb-objs += \
	dpu_mntn.o \
	dpu_mntn_dssv320.o
endif
endif

ifeq ($(CONFIG_DPU_FB_V330),y)
dpufb-objs += \
	dpu_display_effect_dssv330.o \
	dpu_dpe_utils_dssv330.o \
	dpu_dpe_effect_utilsv330.o \
	overlay/dpu_overlay_utils_dssv330.o \
	overlay/dump/dpu_dump_register.o \
	overlay/aif/dpu_aif_dssv330.o \
	overlay/arsr/dpu_arsr_dssv330.o \
	overlay/clip/dpu_clip_dssv330.o \
	overlay/csc/dpu_csc_dssv330.o \
	overlay/mctl/dpu_mctl_dssv330.o \
	overlay/ovl/dpu_ovl_dssv330.o \
	overlay/scf/dpu_scf_dssv330.o \
	dpu_mipi_dsi_v330.o \
	backlight/lm36274.o \
	backlight/lm36923.o \
	backlight/lp8556.o \
	backlight/rt8555.o \
	panel/dssv330/mipi_lg_td4322.o

dpufb-objs += \
	panel/dp_fake_panel.o
dpufb-objs += voltage/tps65132.o
dpufb-objs += voltage/rt4801h.o
dpufb-objs += voltage/ina231.o

ifeq ($(CONFIG_DFX_NOC_MODID_REGISTER),y)
dpufb-objs += \
	dpu_mntn.o \
	dpu_mntn_dssv330.o
endif
endif

ifeq ($(CONFIG_DPU_FB_V350),y)
dpufb-objs += \
	dpu_display_effect_dssv510.o \
	dpu_dpe_utils_dssv510.o \
	dpu_dpe_utils_clk_dssv510.o \
	dpu_dpe_clk_sub_dssv510.o \
	dpu_dpe_effect_utilsv510.o \
	performance/dpu_performance_v510.o \
	overlay/dpu_overlay_utils_dssv510.o \
	overlay/aif/dpu_aif_dssv510.o \
	overlay/arsr/dpu_arsr_dssv510.o \
	overlay/bce_bcd/dpu_bce_bcd_dssv510.o \
	overlay/clip/dpu_clip_dssv510.o \
	overlay/csc/dpu_csc_dssv510.o \
	overlay/mctl/dpu_mctl_dssv510.o \
	overlay/ovl/dpu_ovl_dssv510.o \
	overlay/scf/dpu_scf_dssv510.o \
	overlay/scl/dpu_scl_dssv510.o \
	dpu_dpe_pipe_clk_utils.o \
	dpu_fb_hisync.o \
	dpu_mipi_dsi_v510.o \
	backlight/lm36274.o \
	backlight/lm36923.o \
	backlight/lp8556.o \
	panel_old/dssv350/mipi_samsung_EA8076.o \
	panel/dssv330/mipi_lg_td4322.o

ifeq ($(CONFIG_DFX_NOC_MODID_REGISTER),y)
dpufb-objs += \
	dpu_mntn.o \
	dpu_mntn_dssv350.o
endif
endif

ifeq ($(CONFIG_DPU_FB_V600),y)
dpufb-objs += \
	dpu_dp.o \
	dpu_dp_hpd.o \
	dpu_dpe_utils_dssv510.o \
	dpu_dpe_utils_clk_dssv510.o \
	dpu_dpe_clk_sub_dssv600.o \
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
	overlay/scl/dpu_scl_dssv510.o \
	dpu_dpe_pipe_clk_utils.o \
	dpu_fb_hisync.o \
	dpu_mipi_dsi_v510.o \
	spr/dpu_spr.o \
	spr/dpu_spr_1_0.o \
	backlight/lm36923.o \
	product/bridge/edp_bridge.o \
	panel_old/mipi_samsung_EA8076.o \
	panel_old/dssv600/mipi_sharp_NT36870.o \
	panel_old/dssv600/mipi_sharp_TD4336.o \
	panel/dssv330/mipi_lg_td4322.o \
	dsc/dsc_algorithm.o \
	dsc/utilities/dpu_debug.o \
	dsc/dsc_algorithm_manager.o \
	dpu_dpe_clk_low_temp.o \
	panel_old/dssv600/mipi_boe_nt37700p.o

dpufb-objs += dpu_hiace_single_mode.o
EXTRA_CFLAGS += -DHIACE_SINGLE_MODE_SUPPORT
endif


ifeq ($(CONFIG_DPU_FB_V360),y)
dpufb-objs += \
	dpu_dp.o \
	dpu_dp_hpd.o \
	dpu_dpe_utils_dssv510.o \
	dpu_dpe_utils_clk_dssv510.o \
	dpu_dpe_clk_sub_dssv360.o \
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
	overlay/scl/dpu_scl_dssv510.o \
	dpu_dpe_pipe_clk_utils.o \
	dpu_fb_hisync.o \
	dpu_mipi_dsi_v510.o \
	spr/dpu_spr.o \
	spr/dpu_spr_2_0.o \
	backlight/lm36923.o \
	panel/dssv330/mipi_lg_td4322.o \
	panel/dssv360/mipi_samsung_EA8076.o \
	panel/dssv360/mipi_sharp_NT36870.o \
	panel/mipi_boe_nt37800a.o \
	dsc/dsc_algorithm.o \
	dsc/utilities/dpu_debug.o \
	dsc/dsc_algorithm_manager.o

dpufb-objs += dpu_hiace_single_mode.o
EXTRA_CFLAGS += -DHIACE_SINGLE_MODE_SUPPORT
endif

ifeq ($(CONFIG_DPU_FB_V345),y)
dpufb-objs += \
	dpu_display_effect_dssv510.o \
	dpu_dpe_utils_dssv510.o \
	dpu_dpe_utils_clk_dssv510.o \
	dpu_dpe_clk_sub_dssv510.o \
	dpu_dpe_effect_utilsv510.o \
	performance/dpu_performance_v510.o \
	overlay/dpu_overlay_utils_dssv510.o \
	overlay/aif/dpu_aif_dssv510.o \
	overlay/arsr/dpu_arsr_dssv510.o \
	overlay/bce_bcd/dpu_bce_bcd_dssv510.o \
	overlay/clip/dpu_clip_dssv510.o \
	overlay/csc/dpu_csc_dssv510.o \
	overlay/mctl/dpu_mctl_dssv510.o \
	overlay/ovl/dpu_ovl_dssv510.o \
	overlay/scf/dpu_scf_dssv510.o \
	overlay/scl/dpu_scl_dssv510.o \
	dpu_dpe_pipe_clk_utils.o \
	dpu_fb_hisync.o \
	dpu_mipi_dsi_v510.o \
	backlight/lm36274.o \
	backlight/lm36923.o \
	backlight/lp8556.o \
	backlight/ktz8864.o \
	panel/dssv330/mipi_lg_td4322.o \
	panel_old/dssv350/mipi_hx_nt36682c.o

ifeq ($(CONFIG_DFX_NOC_MODID_REGISTER),y)
dpufb-objs += \
	dpu_mntn.o \
	dpu_mntn_dssv350.o
endif
endif

ifeq ($(CONFIG_DPU_FB_V346),y)
dpufb-objs += \
	dpu_display_effect_dssv510.o \
	dpu_dpe_utils_dssv510.o \
	dpu_dpe_utils_clk_dssv510.o \
	dpu_dpe_clk_sub_dssv510.o \
	dpu_dpe_effect_utilsv510.o \
	performance/dpu_performance_v510.o \
	overlay/dpu_overlay_utils_dssv510.o \
	overlay/aif/dpu_aif_dssv510.o \
	overlay/arsr/dpu_arsr_dssv510.o \
	overlay/bce_bcd/dpu_bce_bcd_dssv510.o \
	overlay/clip/dpu_clip_dssv510.o \
	overlay/csc/dpu_csc_dssv510.o \
	overlay/mctl/dpu_mctl_dssv510.o \
	overlay/ovl/dpu_ovl_dssv510.o \
	overlay/scf/dpu_scf_dssv510.o \
	overlay/scl/dpu_scl_dssv510.o \
	dpu_dpe_pipe_clk_utils.o \
	dpu_fb_hisync.o \
	dpu_mipi_dsi_v510.o \
	spr/dpu_spr.o \
	spr/dpu_spr_1_0.o \
	backlight/lm36274.o \
	backlight/lm36923.o \
	backlight/lp8556.o \
	backlight/ktz8864.o \
	panel/dssv330/mipi_lg_td4322.o \
	panel_old/dssv346/mipi_tcl_nt36682c.o \
	panel_old/dssv346/mipi_boe_nt37701_brq.o \
	panel_old/dssv501/mipi_sharp_NT36870.o

endif

dpufb-objs += \
	panel/hdmi_fake_panel.o

EXTRA_CFLAGS += -I$(srctree)/drivers/video/dkmd/dpu/azalea \
		-I$(srctree)/drivers/video/dkmd/dpu/azalea/overlay \
		-I$(srctree)/drivers/video/dkmd/panel \
		-I$(srctree)/drivers/video/dkmd \
		-I$(srctree)/include \
		-I$(srctree)/drivers/staging/android

ifeq ($(CONFIG_LCDKIT_DRIVER),y)
EXTRA_CFLAGS += -I$(srctree)/drivers/devkit/lcdkit/lcdkit1.0/include
endif

ifneq ($(PRODUCT_RANGE),base)
EXTRA_CFLAGS += -I$(srctree)/drivers/huawei_platform/inputhub/default
endif

ifeq ($(CONFIG_TEE_TUI),y)
EXTRA_CFLAGS += -I$(srctree)/drivers/platform_drivers/tzdriver
endif

ifneq ($(PRODUCT_RANGE),base)
-include $(src)/Makefile.lcd_kit
endif

clean:
	rm *.o .*cmd
