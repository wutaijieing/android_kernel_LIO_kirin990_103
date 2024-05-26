#include <linux/string.h>
#include <linux/printk.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/io.h>
#include "mc.h"
#include "cfg_table_mc.h"
#include "cvdr_drv.h"
#include "mc_drv.h"
#include "cvdr_opt.h"
#include "adapter_common.h"
#include "memory.h"
#include "ipp_top_reg_offset.h"

#define LOG_TAG LOG_MODULE_MC

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeclaration-after-statement"
/*lint -e429  -e613  -e668*/
#define CMP_INDEX_NUM 3216
static int mc_dump_request(struct msg_req_mc_request_t* req);

static int mc_update_cvdr_cfg_tab(
	struct msg_req_mc_request_t *mc_request,
	struct cfg_tab_cvdr_t *p_cvdr_cfg)
{
	if (mc_request == NULL || p_cvdr_cfg == NULL) {
		E("failed : null pointer");
		return ISP_IPP_ERR;
	}

	if (mc_request->mc_buff_cfg.ref_first_32k_page &&
		mc_request->mc_buff_cfg.cur_first_32k_page)
		loge_if_ret(cfg_tbl_cvdr_nr_rd_cfg(p_cvdr_cfg, IPP_NR_RD_MC));

	return ISP_IPP_OK;
}

static int set_ipp_cfg_mc(
	struct msg_req_mc_request_t *req,
	struct cfg_tab_mc_t *mc_cfg_tab,
	unsigned int mc_update_flag)
{
	if (req == NULL || mc_cfg_tab == NULL) {
		E("failed : null pointer");
		return ISP_IPP_ERR;
	}

	mc_cfg_tab->to_use = 1;
	mc_cfg_tab->mc_ctrl_cfg.to_use = 1;
	mc_cfg_tab->mc_ctrl_cfg.bf_mode =
		req->mc_hw_config.mc_ctrl_cfg.bf_mode;
	mc_cfg_tab->mc_ctrl_cfg.max_iterations =
		req->mc_hw_config.mc_ctrl_cfg.max_iterations;
	mc_cfg_tab->mc_ctrl_cfg.svd_iterations =
		req->mc_hw_config.mc_ctrl_cfg.svd_iterations;
	mc_cfg_tab->mc_ctrl_cfg.flag_10_11 =
		req->mc_hw_config.mc_ctrl_cfg.flag_10_11;
	mc_cfg_tab->mc_ctrl_cfg.push_inliers_en =
		req->mc_hw_config.mc_ctrl_cfg.push_inliers_en;
	mc_cfg_tab->mc_ctrl_cfg.h_cal_en =
		req->mc_hw_config.mc_ctrl_cfg.h_cal_en;
	mc_cfg_tab->mc_ctrl_cfg.cfg_mode =
		req->mc_hw_config.mc_ctrl_cfg.cfg_mode;
	mc_cfg_tab->mc_ctrl_cfg.mc_en =
		req->mc_hw_config.mc_ctrl_cfg.mc_en;
	mc_cfg_tab->mc_inlier_thld_cfg.to_use = 1;
	mc_cfg_tab->mc_inlier_thld_cfg.inlier_th =
		req->mc_hw_config.mc_inlier_thld_cfg.inlier_th;
	mc_cfg_tab->mc_kpt_num_cfg.to_use = 1;
	mc_cfg_tab->mc_kpt_num_cfg.matched_kpts =
		req->mc_hw_config.mc_kpt_num_cfg.matched_kpts;

	mc_cfg_tab->mc_cascade_en = (mc_update_flag == 0) ?
		CASCADE_DISABLE : CASCADE_ENABLE;
	
	if (mc_cfg_tab->mc_ctrl_cfg.cfg_mode == CFG_MODE_INDEX_PAIRS) {
		mc_cfg_tab->mc_prefech_cfg.to_use = 1;
		mc_cfg_tab->mc_prefech_cfg.ref_prefetch_enable = 0;
		mc_cfg_tab->mc_prefech_cfg.cur_prefetch_enable = 0;
		mc_cfg_tab->mc_prefech_cfg.ref_first_32k_page =
			req->mc_buff_cfg.ref_first_32k_page >> 15;
		mc_cfg_tab->mc_prefech_cfg.cur_first_32k_page =
			req->mc_buff_cfg.cur_first_32k_page >> 15;
		mc_cfg_tab->mc_index_pairs_cfg.to_use = 1;
		mc_cfg_tab->mc_coord_pairs_cfg.to_use = 0;
		loge_if(memcpy_s(
			&mc_cfg_tab->mc_index_pairs_cfg.cfg_best_dist[0],
			KPT_NUM_MAX * sizeof(unsigned int),
			 &req->mc_hw_config.mc_index_pairs_cfg.cfg_best_dist[0],
			 KPT_NUM_MAX * sizeof(unsigned int)));
		loge_if(memcpy_s(
			&mc_cfg_tab->mc_index_pairs_cfg.cfg_ref_index[0],
			KPT_NUM_MAX * sizeof(unsigned int),
			&req->mc_hw_config.mc_index_pairs_cfg.cfg_ref_index[0],
			KPT_NUM_MAX * sizeof(unsigned int)));
		loge_if(memcpy_s(
			&mc_cfg_tab->mc_index_pairs_cfg.cfg_cur_index[0],
			KPT_NUM_MAX * sizeof(unsigned int),
			&req->mc_hw_config.mc_index_pairs_cfg.cfg_cur_index[0],
			KPT_NUM_MAX * sizeof(unsigned int)));
	}

	if (mc_cfg_tab->mc_ctrl_cfg.cfg_mode == CFG_MODE_COOR_PAIRS) {
		mc_cfg_tab->mc_prefech_cfg.to_use = 0;
		mc_cfg_tab->mc_index_pairs_cfg.to_use = 0;
		mc_cfg_tab->mc_coord_pairs_cfg.to_use = 1;
		loge_if(memcpy_s(
		&mc_cfg_tab->mc_coord_pairs_cfg.cur_cfg_coordinate_y[0],
		KPT_NUM_MAX * sizeof(unsigned int),
		&req->mc_hw_config.mc_coord_pairs_cfg.cur_cfg_coordinate_y[0],
		KPT_NUM_MAX * sizeof(unsigned int)));
		loge_if(memcpy_s(
		&mc_cfg_tab->mc_coord_pairs_cfg.cur_cfg_coordinate_x[0],
		KPT_NUM_MAX * sizeof(unsigned int),
		&req->mc_hw_config.mc_coord_pairs_cfg.cur_cfg_coordinate_x[0],
		KPT_NUM_MAX * sizeof(unsigned int)));
		loge_if(memcpy_s(
		&mc_cfg_tab->mc_coord_pairs_cfg.ref_cfg_coordinate_y[0],
		KPT_NUM_MAX * sizeof(unsigned int),
		&req->mc_hw_config.mc_coord_pairs_cfg.ref_cfg_coordinate_y[0],
		KPT_NUM_MAX * sizeof(unsigned int)));
		loge_if(memcpy_s(
		&mc_cfg_tab->mc_coord_pairs_cfg.ref_cfg_coordinate_x[0],
		KPT_NUM_MAX * sizeof(unsigned int),
		&req->mc_hw_config.mc_coord_pairs_cfg.ref_cfg_coordinate_x[0],
		KPT_NUM_MAX * sizeof(unsigned int)));
	}

	return ISP_IPP_OK;
}

static int seg_ipp_mc_module_config(
	struct msg_req_mc_request_t *mc_request,
	struct cfg_tab_mc_t *mc_cfg_tab,
	struct cfg_tab_cvdr_t *mc_cvdr_cfg_tab,
	unsigned int mc_update_flag)
{
	int ret = 0;
	if (mc_request == NULL || mc_cfg_tab == NULL ||
	mc_cvdr_cfg_tab == NULL) {
		E("failed : null pointer");
		return ISP_IPP_ERR;
	}

	ret = set_ipp_cfg_mc(mc_request, &mc_cfg_tab[0], mc_update_flag);
	if (ret < 0) {
		E("failed : set_ipp_cfg_mc");
		return ISP_IPP_ERR;
	}
	ret = mc_update_cvdr_cfg_tab(mc_request, &mc_cvdr_cfg_tab[0]);
	if (ret < 0) {
		E("failed : mc_update_cvdr_cfg_tab");
		return ISP_IPP_ERR;
	}

	return ISP_IPP_OK;
}

static int seg_ipp_cfg_mc_cmdlst(
	struct msg_req_mc_request_t *mc_request,
	struct cfg_tab_mc_t *mc_cfg_tab,
	struct cfg_tab_cvdr_t *cvdr_cfg_tab,
	struct cmdlst_para_t *cmdlst_para,
	struct orb_schedule_flag_t * flag)
{
	struct cmdlst_stripe_info_t *cmdlst_stripe =
		cmdlst_para->cmd_stripe_info;
	unsigned int i = 0;
	unsigned int stripe_cnt = 1;
	unsigned long long irq = 0;
	struct cmdlst_wr_cfg_t cmdlst_wr_cfg;

	memset_s(&cmdlst_wr_cfg, sizeof(struct cmdlst_wr_cfg_t),
		0, sizeof(struct cmdlst_wr_cfg_t));
	cmdlst_wr_cfg.read_mode = 0;
	stripe_cnt = (mc_request->mc_buff_cfg.inlier_num_buff == 0) ?
		(stripe_cnt) : (stripe_cnt + 1);
	stripe_cnt = (mc_request->mc_buff_cfg.h_matrix_buff == 0) ?
		(stripe_cnt) : (stripe_cnt + 1);
	stripe_cnt = (mc_request->mc_buff_cfg.coordinate_pairs_buff == 0) ?
		(stripe_cnt) : (stripe_cnt + 1);
	flag->mc_token_num = stripe_cnt;

	for (i = 0; i < stripe_cnt; i++) {
		irq = 0;

		if (i != 0) {
			cmdlst_stripe[i].hw_priority     = CMD_PRIO_LOW;
			irq = irq | (((unsigned long long)(1)) <<
				IPP_MC_CVDR_VP_WR_EOF_CMDLST);
		} else {
			cmdlst_stripe[i].hw_priority = CMD_PRIO_LOW;
			irq = irq | (((unsigned long long)(1)) <<
				IPP_MC_IRQ_DONE);
		}

		cmdlst_stripe[i].hw_priority     = CMD_PRIO_LOW;
		cmdlst_stripe[i].resource_share  = 1 <<
			IPP_CMD_RES_SHARE_MC;
		cmdlst_stripe[i].irq_mode        = irq;
		cmdlst_stripe[i].en_link         = 0;
		cmdlst_stripe[i].ch_link         = 0;
		cmdlst_stripe[i].ch_link_act_nbr = 0;
		cmdlst_stripe[i].is_last_stripe  = 0;
		cmdlst_stripe[i].is_need_set_sop = 0;
	}

	cmdlst_stripe[stripe_cnt - 1].is_last_stripe  = 1;
	cmdlst_para->channel_id = IPP_MC_CHANNEL_ID;
	cmdlst_para->stripe_cnt = stripe_cnt;
	loge_if_ret(df_sched_prepare(cmdlst_para));
	struct schedule_cmdlst_link_t *cmd_link_entry = cmdlst_para->cmd_entry;

	for (i = 0; i < cmdlst_para->stripe_cnt;) {
		loge_if_ret(cvdr_prepare_cmd(&g_cvdr_devs[0],
			&cmd_link_entry[i].cmd_buf, &cvdr_cfg_tab[0]));
		loge_if_ret(mc_prepare_cmd(&g_mc_devs[0],
			&cmd_link_entry[i].cmd_buf, &mc_cfg_tab[0]));
		if(flag->mc_en_flag == 1) {
			flag->mc_matched_num_addr =
				mc_cfg_tab[0].mc_match_points_addr;
			flag->mc_index_addr =
				mc_cfg_tab[0].mc_index_addr;
		}

		loge_if_ret(mc_enable_cmd(&g_mc_devs[0],
        		&cmd_link_entry[i++].cmd_buf, &mc_cfg_tab[0]));

		if (mc_request->mc_buff_cfg.inlier_num_buff) {
			cmdlst_wr_cfg.is_incr = 0;
			cmdlst_wr_cfg.data_size = 1;
			cmdlst_wr_cfg.is_wstrb = 0;
			cmdlst_wr_cfg.buff_wr_addr = mc_request->mc_buff_cfg.inlier_num_buff;
			cmdlst_wr_cfg.reg_rd_addr =
				IPP_BASE_ADDR_MC + MC_INLIER_NUMBER_REG;
			loge_if_ret(seg_ipp_set_cmdlst_wr_buf(
				&cmd_link_entry[i++].cmd_buf, &cmdlst_wr_cfg));
		}

		if (mc_request->mc_buff_cfg.h_matrix_buff) {
			cmdlst_wr_cfg.is_incr = 1;
			cmdlst_wr_cfg.data_size = MC_H_MATRIX_NUM;
			cmdlst_wr_cfg.is_wstrb = 0;
			cmdlst_wr_cfg.buff_wr_addr =
				mc_request->mc_buff_cfg.h_matrix_buff;
			cmdlst_wr_cfg.reg_rd_addr =
				IPP_BASE_ADDR_MC + MC_H_MATRIX_0_REG;
			loge_if_ret(seg_ipp_set_cmdlst_wr_buf(
				&cmd_link_entry[i++].cmd_buf, &cmdlst_wr_cfg));
		}

		if (mc_request->mc_buff_cfg.coordinate_pairs_buff) {
			cmdlst_wr_cfg.is_incr = 0;
			cmdlst_wr_cfg.data_size = MC_COORDINATE_PAIRS;
			cmdlst_wr_cfg.is_wstrb = 0;
			cmdlst_wr_cfg.buff_wr_addr =
				mc_request->mc_buff_cfg.coordinate_pairs_buff;
			cmdlst_wr_cfg.reg_rd_addr =
				IPP_BASE_ADDR_MC + MC_COORDINATE_PAIRS_0_REG;
			loge_if_ret(seg_ipp_set_cmdlst_wr_buf(
				&cmd_link_entry[i++].cmd_buf, &cmdlst_wr_cfg));
		}
	}

	return ISP_IPP_OK;
}

int mc_request_handler(
	struct msg_req_mc_request_t *mc_request,
	struct orb_schedule_flag_t * flag)
{
	unsigned long long va = 0;
	unsigned int da = 0;
	unsigned int mc_mode = CFG_MODE_INDEX_PAIRS;
	unsigned int mc_update_flag;
	int ret;

	if (mc_request == NULL ||
		flag == NULL) {
		E(" failed : mc_request is null");
		return ISP_IPP_ERR;
	}

	mc_update_flag = flag->mc_en_flag;
	mc_mode = mc_request->mc_hw_config.mc_ctrl_cfg.cfg_mode;
	if((mc_mode == CFG_MODE_INDEX_PAIRS) &&
		(mc_request->mc_buff_cfg.cur_first_32k_page == 0
		|| mc_request->mc_buff_cfg.ref_first_32k_page == 0)){
		E(" failed : mc_addr is incorrect");
		return ISP_IPP_ERR;	
	}

	ret = mc_dump_request(mc_request);
	if (ret != 0) {
		E("failed : mc_dump_request");
		return ISP_IPP_ERR;
	}

	unsigned int mc_token_nbr_en = flag->mc_en_flag;
	unsigned int nrt_channel  = 1;

	unsigned int cmdlst_channel_value =
		(mc_token_nbr_en << 7) | (nrt_channel << 8);
	hispcpe_reg_set(CMDLIST_REG, 0x80 +
		IPP_MC_CHANNEL_ID * 0x80, cmdlst_channel_value);

	ret = cpe_mem_get(MEM_ID_CFG_TAB_MC, &va, &da);
	if (ret != 0) {
		E(" Failed : cpe_mem_get %d\n", MEM_ID_CFG_TAB_MC);
		return ISP_IPP_ERR;
	}

	struct cfg_tab_mc_t *mc_cfg_tab = (struct cfg_tab_mc_t *)(uintptr_t)va;

	ret = cpe_mem_get(MEM_ID_CVDR_CFG_TAB_MC, &va, &da);
	if (ret != 0) {
		E(" Failed : cpe_mem_get %d\n", MEM_ID_CVDR_CFG_TAB_MC);
		goto fail_mc_cfg_tab_mem;
	}

	struct cfg_tab_cvdr_t *cvdr_cfg_tab =
		(struct cfg_tab_cvdr_t *)(uintptr_t)va;

	ret = cpe_mem_get(MEM_ID_CMDLST_PARA_MC, &va, &da);
	if (ret != 0) {
		E(" Failed : cpe_mem_get %d\n", MEM_ID_CMDLST_PARA_MC);
		goto fail_mc_cvdr_tab_mem;
	}

	struct cmdlst_para_t *cmdlst_para =
		(struct cmdlst_para_t *)(uintptr_t)va;

	ret = seg_ipp_mc_module_config(
		mc_request,
		mc_cfg_tab,
		cvdr_cfg_tab,
		mc_update_flag);
	if (ret  != 0) {
		E("Failed : mc_module_config");
		goto fail_mc_cvdr_tab_mem;
	}

	ret = seg_ipp_cfg_mc_cmdlst(
		mc_request, mc_cfg_tab,
		cvdr_cfg_tab, cmdlst_para,
		flag);
	if (ret  != 0) {
		E("Failed : cfg_mc_cmdlst");
		goto fail_mc_cvdr_tab_mem;
	}

	ret = df_sched_start(cmdlst_para);
	if (ret  != 0) {
		E(" Failed : df_sched_start");
	}

fail_mc_cvdr_tab_mem:
	cpe_mem_free(MEM_ID_CVDR_CFG_TAB_MC);
fail_mc_cfg_tab_mem:
	cpe_mem_free(MEM_ID_CFG_TAB_MC);
	return ret;
}

static int mc_dump_request(struct msg_req_mc_request_t *req)
{
#if FLAG_LOG_DEBUG

	if (req == NULL) {
		E("req is NULL!!\n");
		return ISP_IPP_ERR;
	}

	D("size of msg_req_mc_request_t = %lu",
		sizeof(struct msg_req_mc_request_t));
	D("frame_number = %d",
		req->frame_number);
	D("mc_buff_cfg.ref_first_32k_page = %d",
		req->mc_buff_cfg.ref_first_32k_page);
	D("mc_buff_cfg.cur_first_32k_page = %d",
		req->mc_buff_cfg.cur_first_32k_page);
	D("mc_buff_cfg.inlier_num_buff = %d",
		req->mc_buff_cfg.inlier_num_buff);
	D("mc_buff_cfg.h_matrix_buff = %d",
		req->mc_buff_cfg.h_matrix_buff);
	D("mc_buff_cfg.coordinate_pairs_buff = %d",
		req->mc_buff_cfg.coordinate_pairs_buff);
	D("mc_ctrl_cfg: bf_mode= %d",
		req->mc_hw_config.mc_ctrl_cfg.bf_mode);
	D("mc_ctrl_cfg: push_inliers_en= %d",
		req->mc_hw_config.mc_ctrl_cfg.push_inliers_en);
	D("mc_ctrl_cfg: svd_iterations= %d",
		req->mc_hw_config.mc_ctrl_cfg.svd_iterations);
	D("mc_ctrl_cfg: max_iterations= %d",
		req->mc_hw_config.mc_ctrl_cfg.max_iterations);
	D("mc_ctrl_cfg: flag_10_11= %d",
		req->mc_hw_config.mc_ctrl_cfg.flag_10_11);
	D("mc_ctrl_cfg: h_cal_en= %d",
		req->mc_hw_config.mc_ctrl_cfg.h_cal_en);
	D("mc_ctrl_cfg: cfg_mode= %d",
		req->mc_hw_config.mc_ctrl_cfg.cfg_mode);
	D("mc_ctrl_cfg: mc_en= %d",
		req->mc_hw_config.mc_ctrl_cfg.mc_en);
	D("mc_inlier_thld_cfg: inlier_th= %d",
		req->mc_hw_config.mc_inlier_thld_cfg.inlier_th);
	D("mc_kpt_num_cfg: matched_kpts= %d",
		req->mc_hw_config.mc_kpt_num_cfg.matched_kpts);
#endif
	return ISP_IPP_OK;
}
