#include <linux/printk.h>
#include <linux/slab.h>
#include "orb_enh.h"
#include "cfg_table_orb_enh.h"
#include "cvdr_drv.h"
#include "orb_enh_drv.h"
#include "cvdr_opt.h"
#include "adapter_common.h"
#include "memory.h"
#include "ipp_top_reg_offset.h"

#define LOG_TAG LOG_MODULE_ORB_ENH

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeclaration-after-statement"
/*lint -e429  -e613  -e668*/

static int orb_enh_dump_request(
	struct msg_enh_req_t *req);

static int seg_ipp_cfg_orb_enh_cvdr(
	struct msg_enh_req_t *req,
	struct cfg_tab_cvdr_t *p_cvdr_cfg)
{
	if (req == NULL || p_cvdr_cfg == NULL) {
		E("failed : null pointer");
		return ISP_IPP_ERR;
	}

	struct cvdr_opt_fmt_t cfmt;

	loge_if(memset_s(&cfmt, sizeof(struct cvdr_opt_fmt_t),
		0, sizeof(struct cvdr_opt_fmt_t)));

	unsigned int orb_enh_stride = 0;
	enum pix_format_e format = PIXEL_FMT_IPP_MAX;

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

static int set_ipp_cfg_orb_enh(struct msg_enh_req_t *req,
	struct cfg_tab_orb_enh_t *orb_enh_cfg_tab)
{
	if (req == NULL || orb_enh_cfg_tab == NULL) {
		E("failed : null pointer");
		return ISP_IPP_ERR;
	}

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
	return ISP_IPP_OK;
}

static int seg_ipp_orb_enh_module_config(
	struct msg_enh_req_t *orb_enh_request,
	struct cfg_tab_orb_enh_t *orb_enh_cfg_tab,
	struct cfg_tab_cvdr_t *cvdr_cfg_tab)
{
	if(orb_enh_request == NULL || orb_enh_cfg_tab == NULL
	|| cvdr_cfg_tab == NULL) {
		E("failed : null pointer");
		return ISP_IPP_ERR;
	}

	int ret = 0;
	ret = set_ipp_cfg_orb_enh(orb_enh_request, &orb_enh_cfg_tab[0]);
	if (ret < 0) {
		E("failed : set_ipp_cfg_orb_enh");
		return ISP_IPP_ERR;
	}

	ret = seg_ipp_cfg_orb_enh_cvdr(orb_enh_request, &cvdr_cfg_tab[0]);
	if (ret < 0) {
		E("failed : seg_ipp_cfg_orb_enh_cvdr");
		return ISP_IPP_ERR;
	}

	return ISP_IPP_OK;
}

static int seg_ipp_cfg_orb_enh_cmdlst(
	struct msg_enh_req_t *orb_enh_request,
	struct cfg_tab_orb_enh_t *orb_enh_cfg_tab,
	struct cfg_tab_cvdr_t *cvdr_cfg_tab,
	struct cmdlst_para_t *cmdlst_para)
{
	if(orb_enh_request == NULL || orb_enh_cfg_tab == NULL ||
	cvdr_cfg_tab == NULL || cmdlst_para == NULL) {
		E("failed : null pointer");
		return ISP_IPP_ERR;
	}

	struct cmdlst_stripe_info_t *cmdlst_stripe =
		cmdlst_para->cmd_stripe_info;
	unsigned int i = 0;
	unsigned int stripe_cnt = 1;
	unsigned long long irq = 0;

	for (i = 0; i < stripe_cnt; i++) {
		irq = 0;
		cmdlst_stripe[i].resource_share  = 1 <<
			IPP_CMD_RES_SHARE_ORB_ENH;
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
	}

	cmdlst_stripe[stripe_cnt - 1].is_last_stripe  = 1;
	cmdlst_para->channel_id = IPP_ORB_ENH_CHANNEL_ID;
	cmdlst_para->stripe_cnt = stripe_cnt;
	loge_if_ret(df_sched_prepare(cmdlst_para));
	struct schedule_cmdlst_link_t *cmd_link_entry = cmdlst_para->cmd_entry;

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
	if (enh_req == NULL) {
		E(" failed : enh_req is null");
		return -ISP_IPP_ERR;
	}

	if (enh_req->streams[BI_ENH_YHIST].buffer== 0
		||enh_req->streams[BI_ENH_YIMAGE].buffer == 0
		||enh_req->streams[BO_ENH_OUT].buffer == 0) {
		E(" failed : orb_enh_image_addr is null");
		return -ISP_IPP_ERR;
	}

	if (enh_req->streams[BI_ENH_YHIST].width == 0
		||enh_req->streams[BI_ENH_YHIST].width > 1280
		||enh_req->streams[BI_ENH_YIMAGE].width == 0
		||enh_req->streams[BI_ENH_YIMAGE].width > 1280
		||enh_req->streams[BO_ENH_OUT].width == 0
		|| enh_req->streams[BO_ENH_OUT].width > 1280) {
		E(" failed : orb_enh_image_width is incorrect");
		return -ISP_IPP_ERR;
	}

	if (enh_req->streams[BI_ENH_YHIST].height == 0
		||enh_req->streams[BI_ENH_YHIST].height > 1280
		||enh_req->streams[BI_ENH_YIMAGE].height == 0
		||enh_req->streams[BI_ENH_YIMAGE].height > 1280
		||enh_req->streams[BO_ENH_OUT].height== 0
		|| enh_req->streams[BO_ENH_OUT].height > 1280) {
		E(" failed : orb_enh_image_height is incorrect");
		return -ISP_IPP_ERR;
	}

	return ISP_IPP_OK;
}

int orb_enh_request_handler(
	struct msg_enh_req_t *orb_enh_request)
{
	unsigned long long va = 0;
	unsigned int da = 0;
	int ret;

	if (orb_enh_request == NULL) {
		E(" failed : orb_enh_request is null");
		return ISP_IPP_ERR;
	}

	ret = orb_enh_check_para(orb_enh_request);
	if (ret != 0) {
		E(" Failed : orb_enh_check_para");
		return ISP_IPP_ERR;
	}

	ret = orb_enh_dump_request(orb_enh_request);
	if (ret != 0) {
		E(" Failed : orb_enh_dump_request");
		return ISP_IPP_ERR;
	}

	ret = cpe_mem_get(MEM_ID_ORB_ENH_CFG_TAB, &va, &da);
	if (ret != 0) {
		E(" Failed : orb_enh_mem_get %d\n", MEM_ID_ORB_ENH_CFG_TAB);
		return ISP_IPP_ERR;
	}

	struct cfg_tab_orb_enh_t *orb_enh_cfg_tab =
		(struct cfg_tab_orb_enh_t *)(uintptr_t)va;

	ret = cpe_mem_get(MEM_ID_CVDR_CFG_TAB_ORB_ENH, &va, &da);
	if (ret != 0) {
		E(" Failed : orb_enh_mem_get %d\n", MEM_ID_CVDR_CFG_TAB_ORB_ENH);
		goto fail_enh_cfg_tab;
	}

	struct cfg_tab_cvdr_t *cvdr_cfg_tab =
		(struct cfg_tab_cvdr_t *)(uintptr_t)va;

	ret = cpe_mem_get(MEM_ID_CMDLST_PARA_ORB_ENH, &va, &da);
	if (ret  != 0) {
		E(" Failed : orb_enh_mem_get %d\n", MEM_ID_CMDLST_PARA_ORB_ENH);
		goto fail_enh_cvdr_tab;
	}

	struct cmdlst_para_t *cmdlst_para =
		(struct cmdlst_para_t *)(uintptr_t)va;

	loge_if(hispcpe_reg_set(CPE_TOP,
		IPP_TOP_ENH_VPB_CFG_REG, CONNECT_TO_CVDR));
	ret = seg_ipp_orb_enh_module_config(
		orb_enh_request, orb_enh_cfg_tab, cvdr_cfg_tab);
	if (ret  != 0) {
		E(" Failed : enh_module_config");
		goto fail_enh_cvdr_tab;
	}
	ret = seg_ipp_cfg_orb_enh_cmdlst(
		orb_enh_request, orb_enh_cfg_tab,
		cvdr_cfg_tab, cmdlst_para);
	if (ret  != 0) {
		E(" Failed : cfg_enh_cmdlst");
		goto fail_enh_cvdr_tab;
	}
	ret = df_sched_start(cmdlst_para);
	if (ret  != 0) {
		E(" Failed : df_sched_start");
	}

fail_enh_cvdr_tab:
	cpe_mem_free(MEM_ID_CVDR_CFG_TAB_ORB_ENH);
fail_enh_cfg_tab:
	cpe_mem_free(MEM_ID_ORB_ENH_CFG_TAB);
	D("-\n");
	return ret;
}

static int orb_enh_dump_request(
	struct msg_enh_req_t *req)
{
#if FLAG_LOG_DEBUG
	int i = 0;
	if (req == NULL) {
		E("failed : null pointer");
		return ISP_IPP_ERR;
	}

	I("size_of_orb_enh_req = %d\n",
		(int)(sizeof(struct msg_enh_req_t)));

	for (i = 0; i < ORB_ENH_STREAM_MAX; i++) {
		D("streams[%d].width = %d,height = %d\n",
		i, req->streams[i].width, req->streams[i].height);
		D("stride = %d,buf = %d,format = %d\n",
		req->streams[i].stride, req->streams[i].buffer,
		req->streams[i].format);
	}

	D("ORB_ENH DUMP:");
	D("req->hw_config.ctrl_cfg.enh_en = %d ",
		req->hw_config.ctrl_cfg.enh_en);
	D("req->hw_config.ctrl_cfg.gsblur_en = %d ",
		req->hw_config.ctrl_cfg.gsblur_en);
	D("req->hw_config.adjust_hist_cfg.lutscale = %d ",
		req->hw_config.adjust_hist_cfg.lutscale);
	D("req->hw_config.adjust_image_cfg.inv_xsize = %d ",
		req->hw_config.adjust_image_cfg.inv_xsize);
	D("req->hw_config.adjust_image_cfg.inv_ysize = %d ",
		req->hw_config.adjust_image_cfg.inv_ysize);
	D("req->hw_config.cal_hist_cfg.blknumx = %d ",
		req->hw_config.cal_hist_cfg.blknumx);
	D("req->hw_config.cal_hist_cfg.blknumy = %d ",
		req->hw_config.cal_hist_cfg.blknumy);
	D("req->hw_config.cal_hist_cfg.blk_xsize = %d ",
		req->hw_config.cal_hist_cfg.blk_xsize);
	D("req->hw_config.cal_hist_cfg.blk_ysize = %d ",
		req->hw_config.cal_hist_cfg.blk_ysize);
	D("req->hw_config.cal_hist_cfg.climit = %d ",
		req->hw_config.cal_hist_cfg.climit);
	D("req->hw_config.cal_hist_cfg.edgeex_x = %d ",
		req->hw_config.cal_hist_cfg.edgeex_x);
	D("req->hw_config.cal_hist_cfg.edgeex_y = %d ",
		req->hw_config.cal_hist_cfg.edgeex_y);
	D("req->hw_config.gsblur_coef_cfg.coeff_gauss_0 = %d ",
		req->hw_config.gsblur_coef_cfg.coeff_gauss_0);
	D("req->hw_config.gsblur_coef_cfg.coeff_gauss_1 = %d ",
		req->hw_config.gsblur_coef_cfg.coeff_gauss_1);
	D("req->hw_config.gsblur_coef_cfg.coeff_gauss_2 = %d ",
		req->hw_config.gsblur_coef_cfg.coeff_gauss_2);
	D("req->hw_config.gsblur_coef_cfg.coeff_gauss_3 = %d ",
		req->hw_config.gsblur_coef_cfg.coeff_gauss_3);
#endif
	return ISP_IPP_OK;
}

/*lint +e429  +e613  +e668*/
#pragma GCC diagnostic pop

