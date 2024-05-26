/******************************************************************
 * Copyright    Copyright (c) 2020- Hisilicon Technologies CO., Ltd.
 * File name    segment_orb_enh.c
 * Description:
 *
 * Date         2020-04-17 16:28:10
 ********************************************************************/

#include <linux/printk.h>
#include <linux/slab.h>
#include "segment_orb_enh.h"
#include "cfg_table_orb_enh.h"
#include "cvdr_drv.h"
#include "orb_enh_drv.h"
#include "ipp_top_drv.h"
#include "cvdr_opt.h"
#include "segment_common.h"
#include "memory.h"
#include "ipp_top_reg_offset.h"

#define LOG_TAG LOG_MODULE_ORB_ENH

static int seg_ipp_cfg_orb_enh_cvdr(
	struct msg_enh_req_t *req,
	struct cfg_tab_cvdr_t *p_cvdr_cfg)
{
	struct cvdr_opt_fmt_t cfmt;
	unsigned int orb_enh_stride = 0;
	enum pix_format_e format = PIXEL_FMT_IPP_MAX;
	loge_if(memset_s(&cfmt, sizeof(struct cvdr_opt_fmt_t),
					 0, sizeof(struct cvdr_opt_fmt_t)));

	if (req->streams[BI_ENH_YHIST].buffer) {
		cfmt.id = IPP_VP_RD_ORB_ENH_Y_HIST;
		cfmt.width = req->streams[BI_ENH_YHIST].width;
		cfmt.line_size = cfmt.width / 2;
		cfmt.height = req->streams[BI_ENH_YHIST].height;
		cfmt.addr = req->streams[BI_ENH_YHIST].buffer;
		cfmt.full_width = req->streams[BI_ENH_YHIST].width / 2;
		cfmt.expand = 0;
		format = req->streams[BI_ENH_YHIST].format;
		orb_enh_stride = req->streams[BI_ENH_YHIST].stride;
		loge_if_ret(dataflow_cvdr_rd_cfg_vp(p_cvdr_cfg,
											&cfmt, orb_enh_stride, ISP_IPP_CLK, format));
	}

	if (req->streams[BI_ENH_YIMAGE].buffer) {
		cfmt.id = IPP_VP_RD_ORB_ENH_Y_IMAGE;
		cfmt.width = req->streams[BI_ENH_YIMAGE].width;
		cfmt.line_size = cfmt.width / 2;
		cfmt.height = req->streams[BI_ENH_YIMAGE].height;
		cfmt.addr = req->streams[BI_ENH_YIMAGE].buffer;
		cfmt.full_width = req->streams[BI_ENH_YIMAGE].width / 2;
		cfmt.expand = 0;
		format = req->streams[BI_ENH_YIMAGE].format;
		orb_enh_stride = req->streams[BI_ENH_YIMAGE].stride;
		loge_if_ret(dataflow_cvdr_rd_cfg_vp(p_cvdr_cfg,
											&cfmt, orb_enh_stride, ISP_IPP_CLK, format));
	}

	if (req->streams[BO_ENH_OUT].buffer) {
		cfmt.id = IPP_VP_WR_ORB_ENH_Y;
		cfmt.width =  req->streams[BO_ENH_OUT].width;
		cfmt.full_width = req->streams[BO_ENH_OUT].width / 2;
		cfmt.height = req->streams[BO_ENH_OUT].height;
		cfmt.expand = 0;
		format = req->streams[BO_ENH_OUT].format;
		cfmt.addr = req->streams[BO_ENH_OUT].buffer;
		orb_enh_stride = req->streams[BO_ENH_OUT].stride;
		loge_if_ret(dataflow_cvdr_wr_cfg_vp(p_cvdr_cfg,
											&cfmt, orb_enh_stride, ISP_IPP_CLK, format));
	}

	return ISP_IPP_OK;
}

static void set_ipp_cfg_orb_enh(struct msg_enh_req_t *req,
								struct cfg_tab_orb_enh_t *orb_enh_cfg_tab)
{
	orb_enh_cfg_tab->to_use = 1;
	orb_enh_cfg_tab->ctrl_cfg.to_use = 1;
	orb_enh_cfg_tab->ctrl_cfg.enh_en =
		req->hw_config.ctrl_cfg.enh_en;
	orb_enh_cfg_tab->ctrl_cfg.gsblur_en =
		req->hw_config.ctrl_cfg.gsblur_en;
	orb_enh_cfg_tab->adjust_hist_cfg.to_use = 1;
	orb_enh_cfg_tab->adjust_hist_cfg.lutscale =
		req->hw_config.adjust_hist_cfg.lutscale;
	orb_enh_cfg_tab->gsblur_coef_cfg.to_use = 1;
	orb_enh_cfg_tab->gsblur_coef_cfg.coeff_gauss_0 =
		req->hw_config.gsblur_coef_cfg.coeff_gauss_0;
	orb_enh_cfg_tab->gsblur_coef_cfg.coeff_gauss_1 =
		req->hw_config.gsblur_coef_cfg.coeff_gauss_1;
	orb_enh_cfg_tab->gsblur_coef_cfg.coeff_gauss_2 =
		req->hw_config.gsblur_coef_cfg.coeff_gauss_2;
	orb_enh_cfg_tab->gsblur_coef_cfg.coeff_gauss_3 =
		req->hw_config.gsblur_coef_cfg.coeff_gauss_3;
	orb_enh_cfg_tab->cal_hist_cfg.to_use = 1;
	orb_enh_cfg_tab->cal_hist_cfg.blknumy =
		req->hw_config.cal_hist_cfg.blknumy;
	orb_enh_cfg_tab->cal_hist_cfg.blknumx =
		req->hw_config.cal_hist_cfg.blknumx;
	orb_enh_cfg_tab->cal_hist_cfg.blk_ysize =
		req->hw_config.cal_hist_cfg.blk_ysize;
	orb_enh_cfg_tab->cal_hist_cfg.blk_xsize =
		req->hw_config.cal_hist_cfg.blk_xsize;
	orb_enh_cfg_tab->cal_hist_cfg.edgeex_y =
		req->hw_config.cal_hist_cfg.edgeex_y;
	orb_enh_cfg_tab->cal_hist_cfg.edgeex_x =
		req->hw_config.cal_hist_cfg.edgeex_x;
	orb_enh_cfg_tab->cal_hist_cfg.climit =
		req->hw_config.cal_hist_cfg.climit;
	orb_enh_cfg_tab->adjust_image_cfg.to_use = 1;
	orb_enh_cfg_tab->adjust_image_cfg.inv_xsize =
		req->hw_config.adjust_image_cfg.inv_xsize;
	orb_enh_cfg_tab->adjust_image_cfg.inv_ysize =
		req->hw_config.adjust_image_cfg.inv_ysize;
	return;
}

static int seg_ipp_orb_enh_module_config(
	struct msg_enh_req_t *orb_enh_request,
	struct cfg_tab_orb_enh_t *orb_enh_cfg_tab,
	struct cfg_tab_cvdr_t *cvdr_cfg_tab)
{
	int ret = 0;
	set_ipp_cfg_orb_enh(orb_enh_request, &orb_enh_cfg_tab[0]);
	ret = seg_ipp_cfg_orb_enh_cvdr(orb_enh_request, &cvdr_cfg_tab[0]);
	if (ret != 0) {
		loge(" Failed : enh module config");
		return ISP_IPP_ERR;
	} else {
		return ISP_IPP_OK;
	}
}

static int seg_ipp_cfg_orb_enh_cmdlst(
	struct msg_enh_req_t *orb_enh_request,
	struct cfg_tab_orb_enh_t *orb_enh_cfg_tab,
	struct cfg_tab_cvdr_t *cvdr_cfg_tab,
	struct cmdlst_para_t *cmdlst_para)
{
	struct cmdlst_stripe_info_t *cmdlst_stripe =
			cmdlst_para->cmd_stripe_info;
	unsigned int i = 0;
	unsigned int stripe_cnt = 1;
	unsigned long long irq = 0;
	struct schedule_cmdlst_link_t *cmd_link_entry = NULL;

	for (i = 0; i < stripe_cnt; i++) {
		irq = 0;
		cmdlst_stripe[i].resource_share  = 1 << IPP_CMD_RES_SHARE_ORB_ENH;
		cmdlst_stripe[i].en_link         = 0;
		cmdlst_stripe[i].ch_link         = 0;
		cmdlst_stripe[i].ch_link_act_nbr = 0;
		cmdlst_stripe[i].is_last_stripe  = 0;
		cmdlst_stripe[i].is_need_set_sop = 0;
		irq = irq | (((unsigned long long)(1)) <<
					 IPP_ORB_ENH_CVDR_VP_RD_EOF_CMDLST);
		irq = irq | (((unsigned long long)(1)) <<
					 IPP_ORB_ENH_DONE);
		irq = irq | (((unsigned long long)(1)) <<
					 IPP_ORB_ENH_CVDR_VP_RD_EOF_YHIST);
		irq = irq | (((unsigned long long)(1)) <<
					 IPP_ORB_ENH_CVDR_VP_RD_EOF_YIMG);
		irq = irq | (((unsigned long long)(1)) <<
					 IPP_ORB_ENH_CVDR_VP_WR_EOF_YGUASS);
		cmdlst_stripe[i].hw_priority     = CMD_PRIO_LOW;
		cmdlst_stripe[i].irq_mode        = irq;
		logi("orb_enh_stripe=%d, irq=0x%llx", i, irq);
	}

	cmdlst_stripe[stripe_cnt - 1].is_last_stripe  = 1;
	cmdlst_para->channel_id = IPP_ORB_ENH_CHANNEL_ID;
	cmdlst_para->stripe_cnt = stripe_cnt;
	loge_if_ret(df_sched_prepare(cmdlst_para));
	cmd_link_entry = cmdlst_para->cmd_entry;

	for (i = 0; i < cmdlst_para->stripe_cnt; i++) {
		loge_if_ret(orb_enh_prepare_cmd(&g_orb_enh_devs[0],
										&cmd_link_entry[0].cmd_buf, &orb_enh_cfg_tab[0]));
		loge_if_ret(cvdr_prepare_cmd(&g_cvdr_devs[0],
									 &cmd_link_entry[0].cmd_buf, &cvdr_cfg_tab[0]));
	}

	return ISP_IPP_OK;
}

static int orb_enh_check_para(
	struct msg_enh_req_t *enh_req)
{
	if (enh_req->streams[BI_ENH_YHIST].buffer == 0
		|| enh_req->streams[BI_ENH_YIMAGE].buffer == 0
		|| enh_req->streams[BO_ENH_OUT].buffer == 0) {
		loge(" Failed : orb_enh_image_addr is null");
		return -ISP_IPP_ERR;
	}

	if (enh_req->streams[BI_ENH_YHIST].width == 0
		|| enh_req->streams[BI_ENH_YHIST].width > 1280
		|| enh_req->streams[BI_ENH_YIMAGE].width == 0
		|| enh_req->streams[BI_ENH_YIMAGE].width > 1280
		|| enh_req->streams[BO_ENH_OUT].width == 0
		|| enh_req->streams[BO_ENH_OUT].width > 1280) {
		loge(" Failed : orb_enh_image_width is incorrect");
		return -ISP_IPP_ERR;
	}

	if (enh_req->streams[BI_ENH_YHIST].height == 0
		|| enh_req->streams[BI_ENH_YHIST].height > 1280
		|| enh_req->streams[BI_ENH_YIMAGE].height == 0
		|| enh_req->streams[BI_ENH_YIMAGE].width > 1280
		|| enh_req->streams[BO_ENH_OUT].height == 0
		|| enh_req->streams[BO_ENH_OUT].height > 1280) {
		loge(" Failed : orb_enh_image_height is incorrect");
		return -ISP_IPP_ERR;
	}

	return ISP_IPP_OK;
}

static int seg_ipp_orb_enh_get_cpe_mem(struct seg_orb_enh_cfg_t **seg_orb_enh_cfg,
									   struct cmdlst_para_t **cmdlst_para)
{
	unsigned long long va = 0;
	unsigned int da = 0;
	int ret = ISP_IPP_OK;
	// 1. seg_orb_enh_cfg_t
	ret = cpe_mem_get(MEM_ID_ORB_ENH_CFG_TAB, &va, &da);
	if (ret != 0) {
		loge(" Failed : orb_enh_mem_get %d", MEM_ID_ORB_ENH_CFG_TAB);
		return ISP_IPP_ERR;
	}

	*seg_orb_enh_cfg = (struct seg_orb_enh_cfg_t *)(uintptr_t)va;
	ret = memset_s(*seg_orb_enh_cfg, sizeof(struct seg_orb_enh_cfg_t), 0, sizeof(struct seg_orb_enh_cfg_t));
	if (ret  != 0) {
		loge(" Failed : memset seg_orb_enh_cfg failed");
		goto orb_enh_get_cpe_mem_failed;
	}

	// 2. cmdlst_para_t
	ret = cpe_mem_get(MEM_ID_CMDLST_PARA_ORB_ENH, &va, &da);
	if (ret  != 0) {
		loge(" Failed : orb_enh_mem_get %d", MEM_ID_CMDLST_PARA_ORB_ENH);
		goto orb_enh_get_cpe_mem_failed;
	}

	*cmdlst_para = (struct cmdlst_para_t *)(uintptr_t)va;
	ret = memset_s(*cmdlst_para, sizeof(struct cmdlst_para_t), 0, sizeof(struct cmdlst_para_t));
	if (ret  != 0) {
		loge(" Failed : memset cmdlst_para failed");
		goto orb_enh_get_cpe_mem_failed;
	}

	return ISP_IPP_OK;
orb_enh_get_cpe_mem_failed:
	cpe_mem_free(MEM_ID_ORB_ENH_CFG_TAB);
	return ISP_IPP_ERR;
}

void orb_enh_dump_request(struct msg_enh_req_t *req)
{
	int i = 0;
	char *stream_name[] = {
		"BI_ENH_YHIST",
		"BI_ENH_YIMAGE",
		"BO_ENH_OUT",
	};
	logi("size_of orb_enh_req = %d", (int)(sizeof(struct msg_enh_req_t)));

	for (i = 0; i < ORB_ENH_STREAM_MAX; i++)
		logi("streams[%s]: width = %d, height = %d, stride = %d, buf = 0x%x, format = %d",
			 stream_name[i], req->streams[i].width, req->streams[i].height,
			 req->streams[i].stride, req->streams[i].buffer, req->streams[i].format);

#if defined(FLAG_LOG_DEBUG) && (FLAG_LOG_DEBUG == 1)
	logi("ORB_ENH DUMP REG CFG:");
	logi("req->hw_config.ctrl_cfg.enh_en = %d", req->hw_config.ctrl_cfg.enh_en);
	logi("req->hw_config.ctrl_cfg.gsblur_en = %d", req->hw_config.ctrl_cfg.gsblur_en);
	logi("req->hw_config.adjust_hist_cfg.lutscale = %d", req->hw_config.adjust_hist_cfg.lutscale);
	logi("req->hw_config.adjust_image_cfg.inv_xsize = %d", req->hw_config.adjust_image_cfg.inv_xsize);
	logi("req->hw_config.adjust_image_cfg.inv_ysize = %d", req->hw_config.adjust_image_cfg.inv_ysize);
	logi("req->hw_config.cal_hist_cfg.blknumx = %d", req->hw_config.cal_hist_cfg.blknumx);
	logi("req->hw_config.cal_hist_cfg.blknumy = %d", req->hw_config.cal_hist_cfg.blknumy);
	logi("req->hw_config.cal_hist_cfg.blk_xsize = %d", req->hw_config.cal_hist_cfg.blk_xsize);
	logi("req->hw_config.cal_hist_cfg.blk_ysize = %d", req->hw_config.cal_hist_cfg.blk_ysize);
	logi("req->hw_config.cal_hist_cfg.climit = %d", req->hw_config.cal_hist_cfg.climit);
	logi("req->hw_config.cal_hist_cfg.edgeex_x = %d", req->hw_config.cal_hist_cfg.edgeex_x);
	logi("req->hw_config.cal_hist_cfg.edgeex_y = %d", req->hw_config.cal_hist_cfg.edgeex_y);
	logi("req->hw_config.gsblur_coef_cfg.coeff_gauss_0 = %d", req->hw_config.gsblur_coef_cfg.coeff_gauss_0);
	logi("req->hw_config.gsblur_coef_cfg.coeff_gauss_1 = %d", req->hw_config.gsblur_coef_cfg.coeff_gauss_1);
	logi("req->hw_config.gsblur_coef_cfg.coeff_gauss_2 = %d", req->hw_config.gsblur_coef_cfg.coeff_gauss_2);
	logi("req->hw_config.gsblur_coef_cfg.coeff_gauss_3 = %d", req->hw_config.gsblur_coef_cfg.coeff_gauss_3);
#endif
	return;
}

int orb_enh_request_handler(struct msg_enh_req_t *orb_enh_request)
{
	int ret = ISP_IPP_OK;
	struct seg_orb_enh_cfg_t *seg_orb_enh_cfg = NULL;
	struct cmdlst_para_t *cmdlst_para = NULL;
	dataflow_check_para(orb_enh_request);
	loge_if_ret(orb_enh_check_para(orb_enh_request));
	orb_enh_dump_request(orb_enh_request);
#if IPP_UT_DEBUG
	set_dump_register_init(UT_REG_ADDR, IPP_MAX_REG_OFFSET, 0);
#endif
	ret = seg_ipp_orb_enh_get_cpe_mem(&seg_orb_enh_cfg, &cmdlst_para);
	if (ret != 0) {
		loge(" Failed : seg_ipp_orb_enh_get_cpe_mem");
		return ISP_IPP_ERR;
	}

	hispcpe_reg_set(CPE_TOP, IPP_TOP_ENH_VPB_CFG_REG, CONNECT_TO_CVDR);
	ret = seg_ipp_orb_enh_module_config(orb_enh_request, &(seg_orb_enh_cfg->enh_cfg_tab),
										&(seg_orb_enh_cfg->enh_cvdr_cfg_tab));
	if (ret  != 0) {
		loge(" Failed : enh_module_config");
		goto SEG_ORB_ENH_BUFF_FREE;
	}

	ret = seg_ipp_cfg_orb_enh_cmdlst(orb_enh_request, &(seg_orb_enh_cfg->enh_cfg_tab),
									 &(seg_orb_enh_cfg->enh_cvdr_cfg_tab), cmdlst_para);
	if (ret  != 0) {
		loge(" Failed : cfg_enh_cmdlst");
		goto SEG_ORB_ENH_BUFF_FREE;
	}

	ret = df_sched_start(cmdlst_para);
	if (ret  != 0)
		loge(" Failed : df_sched_start");

SEG_ORB_ENH_BUFF_FREE:
	cpe_mem_free(MEM_ID_ORB_ENH_CFG_TAB);
	return ret;
}

