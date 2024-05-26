 /* Copyright (c) 2013-2014, Hisilicon Tech. Co., Ltd. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include "dpu_display_effect.h"
#include "dpu_fb.h"
#include <linux/fb.h>
#include "global_ddr_map.h"
#include "dpu_fb_defconfig.h"

//lint -e747, -e838, -e774

#define COUNT_LIMIT_TO_PRINT_DELAY			(200)

typedef struct time_interval {
	long start; 				// microsecond
	long stop;
} time_interval_t;

typedef struct delay_record {
	const char *name;
	long max;
	long min;
	long sum;
	int count;
} delay_record_t;

static int dpufb_ce_do_contrast(struct dpu_fb_data_type *dpufd);
static void dpufb_ce_service_init(void);
static void dpufb_ce_service_deinit(void);

#define DEBUG_EFFECT_LOG					DPU_FB_ERR

#define define_delay_record(var, name)		static delay_record_t var = {name, 0, 0xFFFFFFFF, 0, 0}
																		\
#define EFFECT_GRADUAL_REFRESH_FRAMES		(30)

#define XCC_COEF_LEN	   12
#define XCC_COEF_INDEX_01	1
#define XCC_COEF_INDEX_12	6
#define XCC_COEF_INDEX_23	11
#define XCC_DEFAULT	0x8000
#define SCREEN_OFF_BLC_DELTA (-10000)
#define DBV_MAP_INDEX 3
#define DBV_MAP_COUNTS 1024
extern unsigned short dbv_curve_noliner_to_liner_map[DBV_MAP_INDEX][DBV_MAP_COUNTS];
static uint32_t xcc_table_temp[12] = {0x0, 0x8000, 0x0, 0x0, 0x0, 0x0, 0x8000, 0x0, 0x0, 0x0, 0x0, 0x8000};
static uint32_t xcc_enable_state = 0;

static bool g_is_effect_init = false;
static bool g_is_ce_service_init = false;
static struct mutex g_ce_service_lock;
static ce_service_t g_hiace_service;

static bool g_is_effect_lock_init = false;
extern struct mutex g_rgbw_lock;
static spinlock_t g_gmp_effect_lock;
static spinlock_t g_igm_effect_lock;
static spinlock_t g_xcc_effect_lock;
static spinlock_t g_gamma_effect_lock;

#define THRESHOLD_HBM_BACKLIGHT			(741)

static time_interval_t interval_wait_hist = {0};
static time_interval_t interval_wait_lut = {0};
static time_interval_t interval_algorithm = {0};
define_delay_record(delay_wait_hist, "event hist waiting");
define_delay_record(delay_wait_lut, "event lut waiting");
define_delay_record(delay_algorithm, "algorithm processing");

uint32_t g_enable_effect = ENABLE_EFFECT_HIACE | ENABLE_EFFECT_BL;
uint32_t g_debug_effect = 0;



#define ce_service_wait_event(wq, condition)		/*lint -save -e* */						\
({																								\
	long _wait_ret = 0; 																		\
	do {																						\
		_wait_ret = wait_event_interruptible_timeout(wq, condition, msecs_to_jiffies(100000));	\
	} while(!_wait_ret);																		\
	_wait_ret;																					\
})		/*lint -restore */


static inline long get_timestamp_in_us(void)
{
	struct timespec64 ts;
	long timestamp;
	ktime_get_ts64(&ts);
	timestamp = ts.tv_sec * USEC_PER_SEC + ts.tv_nsec / NSEC_PER_USEC;
	return timestamp;
}

static inline void count_delay(delay_record_t *record, long delay)
{
	if (NULL == record) {
		return;
	}
	record->count++;
	record->sum += delay;
	if (delay > record->max) record->max = delay;
	if (delay < record->min) record->min = delay;
	if (!(record->count % COUNT_LIMIT_TO_PRINT_DELAY)) {
		DEBUG_EFFECT_LOG("[effect] Delay(us/%4d) | average:%5ld | min:%5ld | max:%8ld | %s\n", record->count, record->sum / record->count, record->min, record->max, record->name);
		record->count = 0;
		record->sum = 0;
		record->max = 0;
		record->min = 0xFFFFFFFF;
	}
}

static inline void enable_blc(struct dpu_fb_data_type *dpufd, bool enable)
{
	dss_display_effect_bl_enable_t *bl_enable_ctrl = &(dpufd->bl_enable_ctrl);
	int bl_enable = enable ? 1 : 0;

	mutex_lock(&(bl_enable_ctrl->ctrl_lock));
	if (bl_enable != bl_enable_ctrl->ctrl_bl_enable) {
		bl_enable_ctrl->ctrl_bl_enable = bl_enable;
	}
	mutex_unlock(&(bl_enable_ctrl->ctrl_lock));
	effect_debug_log(DEBUG_EFFECT_ENTRY, "[effect] bl_enable change to %d\n", bl_enable_ctrl->ctrl_bl_enable);
}


static int dpufb_ce_do_contrast(struct dpu_fb_data_type *dpufd)
{
	ce_service_t *service = &g_hiace_service;
	int ret = 0;
	long wait_ret = 0;
	long timeout = msecs_to_jiffies(100);

	if (dpufd == NULL) {
		DPU_FB_ERR("[effect] dpufd is NULL\n");
		return -EINVAL;
	}

	if (g_is_ce_service_init) {
		service->new_hist = true;
		if (g_debug_effect & DEBUG_EFFECT_DELAY) {
			interval_wait_hist.start = get_timestamp_in_us();
		}

		if (g_is_effect_init) {
			wake_up_interruptible(&service->wq_hist);
			wait_ret = wait_event_interruptible_timeout(service->wq_lut, service->new_lut || service->stop, timeout);
			service->stop = false;
		} else {
			DPU_FB_ERR("[effect] wq_lut uninit\n");
			return -EINVAL;
		}

		if (g_debug_effect & DEBUG_EFFECT_DELAY) {
			interval_wait_lut.stop = get_timestamp_in_us();
			count_delay(&delay_wait_lut, interval_wait_lut.stop - interval_wait_lut.start);
		}
	}

	if (service->new_lut) {
		service->new_lut = false;
		ret = dpufd->hiace_info[dpufd->panel_info.disp_panel_id].algorithm_result;
	} else {
		effect_debug_log(DEBUG_EFFECT_ENTRY, "[effect] wait_event_interruptible_timeout() return %ld, -ERESTARTSYS:%d\n", wait_ret, -ERESTARTSYS);
		ret = -3;
	}

	return ret;
}

bool check_underflow_clear_ack(struct dpu_fb_data_type *dpufd)
{
	char __iomem *mctl_base = NULL;
	uint32_t mctl_status = 0;

	if(NULL == dpufd){
		DPU_FB_ERR("[effect] dpufd is NULL\n");
		return false;
	}

	mctl_base = dpufd->dss_module.mctl_base[DSS_MCTL0];
	if (mctl_base) {
		mctl_status = inp32(mctl_base + MCTL_CTL_STATUS);
		if ((mctl_status & 0x10) == 0x10) {
			DPU_FB_ERR("UNDERFLOW ACK TIMEOUT!\n");
			return false;
		}
	}
	return true;
}

void dpu_effect_init(struct dpu_fb_data_type *dpufd)
{
	struct dpu_panel_info *pinfo = NULL;
	dss_ce_info_t *ce_info = NULL;
	hiace_alg_parameter_t *param = NULL;

	if (!g_is_effect_lock_init) {
		spin_lock_init(&g_gmp_effect_lock);
		spin_lock_init(&g_igm_effect_lock);
		spin_lock_init(&g_xcc_effect_lock);
		spin_lock_init(&g_gamma_effect_lock);
		mutex_init(&g_rgbw_lock);
		g_is_effect_lock_init = true;
	}

	if (NULL == dpufd) {
		DPU_FB_ERR("[effect] dpufd is NULL\n");
		return;
	}

	pinfo = &(dpufd->panel_info);
	if (pinfo->hiace_support == 0) {
		if (g_debug_effect & DEBUG_EFFECT_ENTRY) {
			DEBUG_EFFECT_LOG("[effect] HIACE is not supported!\n");
		}
		return;
	}

	if (dpufd->index == PRIMARY_PANEL_IDX) {
		ce_info = &dpufd->hiace_info[dpufd->panel_info.disp_panel_id];
		param = &pinfo->hiace_param;
	} else {
		DPU_FB_ERR("[effect] fb%d, not support!", dpufd->index);
		return;
	}

	if (!g_is_effect_init) {
		mutex_init(&g_ce_service_lock);
		mutex_init(&(dpufd->al_ctrl.ctrl_lock));
		mutex_init(&(dpufd->ce_ctrl.ctrl_lock));
		mutex_init(&(dpufd->bl_ctrl.ctrl_lock));
		mutex_init(&(dpufd->bl_enable_ctrl.ctrl_lock));
		mutex_init(&(dpufd->metadata_ctrl.ctrl_lock));
		dpufd->bl_enable_ctrl.ctrl_bl_enable = 1;

		memset(ce_info, 0, sizeof(dss_ce_info_t));
		ce_info->algorithm_result = 1;
		mutex_init(&(ce_info->hist_lock));
		mutex_init(&(ce_info->lut_lock));

		param->iWidth = (int)pinfo->xres;
		param->iHeight = (int)pinfo->yres;
		param->iMode = 0;
		param->bitWidth = 10;
		param->iMinBackLight = (int)dpufd->panel_info.bl_min;
		param->iMaxBackLight = (int)dpufd->panel_info.bl_max;
		param->iAmbientLight = -1;

		memset(&g_hiace_service, 0, sizeof(g_hiace_service));
		init_waitqueue_head(&g_hiace_service.wq_hist);
		init_waitqueue_head(&g_hiace_service.wq_lut);

		if (g_debug_effect & DEBUG_EFFECT_ENTRY) {
			DEBUG_EFFECT_LOG("[effect] width:%d, height:%d, minbl:%d, maxbl:%d\n", param->iWidth, param->iHeight, param->iMinBackLight, param->iMaxBackLight);
		}

		g_is_effect_init = true;
		DPU_FB_INFO("[effect] fb%d, dpu effect init here!", dpufd->index);
	} else {
		if (g_debug_effect & DEBUG_EFFECT_ENTRY) {
			DEBUG_EFFECT_LOG("[effect] bypass\n");
		}
		DPU_FB_INFO("[effect] fb%d, dpu effect has been inited!", dpufd->index);
	}
}

void dpu_effect_deinit(struct dpu_fb_data_type *dpufd)
{
	struct dpu_panel_info *pinfo = NULL;
	dss_ce_info_t *ce_info = NULL;

	if (NULL == dpufd) {
		DPU_FB_ERR("[effect] dpufd is NULL\n");
		return;
	}

	pinfo = &(dpufd->panel_info);
	if (pinfo->hiace_support == 0) {
		if (g_debug_effect & DEBUG_EFFECT_ENTRY) {
			DEBUG_EFFECT_LOG("[effect] HIACE is not supported!\n");
		}
		return;
	}

	if (dpufd->index == PRIMARY_PANEL_IDX) {
		ce_info = &dpufd->hiace_info[dpufd->panel_info.disp_panel_id];
	} else {
		DPU_FB_ERR("[effect] fb%d, not support!", dpufd->index);
		return;
	}

	if (g_is_effect_lock_init) {
		g_is_effect_lock_init = false;
		mutex_destroy(&g_rgbw_lock);
	}

	down(&dpufd->hiace_hist_lock_sem);/*avoid  using  mutex_lock() but hist_lock was destoried by mutex_destory in  dpu_effect_deinit*/
	if (g_is_effect_init) {
		g_is_effect_init = false;

		mutex_destroy(&(ce_info->hist_lock));
		mutex_destroy(&(ce_info->lut_lock));

		mutex_destroy(&(dpufd->al_ctrl.ctrl_lock));
		mutex_destroy(&(dpufd->ce_ctrl.ctrl_lock));
		mutex_destroy(&(dpufd->bl_ctrl.ctrl_lock));
		mutex_destroy(&(dpufd->bl_enable_ctrl.ctrl_lock));
		mutex_destroy(&(dpufd->metadata_ctrl.ctrl_lock));

		mutex_destroy(&g_ce_service_lock);
	} else {
		if (g_debug_effect & DEBUG_EFFECT_ENTRY) {
			DEBUG_EFFECT_LOG("[effect] bypass\n");
		}
	}
	up(&dpufd->hiace_hist_lock_sem);
}

static void dpufb_ce_service_init(void)
{
	mutex_lock(&g_ce_service_lock);
	if (!g_is_ce_service_init) {
		effect_debug_log(DEBUG_EFFECT_ENTRY, "[effect] step in\n");

		g_hiace_service.is_ready = true;
		g_hiace_service.stop = false;

		g_is_ce_service_init = true;
	}
	mutex_unlock(&g_ce_service_lock);
}

static void dpufb_ce_service_deinit(void)
{
	mutex_lock(&g_ce_service_lock);
	if (g_is_ce_service_init) {
		effect_debug_log(DEBUG_EFFECT_ENTRY, "[effect] step in\n");

		g_is_ce_service_init = false;

		g_hiace_service.is_ready = false;
		g_hiace_service.stop = true;
		wake_up_interruptible(&g_hiace_service.wq_hist);
		wake_up_interruptible(&g_hiace_service.wq_lut);
	}
	mutex_unlock(&g_ce_service_lock);
}

static inline void enable_hiace(struct dpu_fb_data_type *dpufd, bool enable)
{
	down(&dpufd->blank_sem);
	if (dpufd->panel_power_on) {
		if (dpufd->hiace_info[dpufd->panel_info.disp_panel_id].hiace_enable != enable) { //lint !e731
			char __iomem *hiace_base = dpufd->dss_base + DSS_HI_ACE_OFFSET;

			dpufb_activate_vsync(dpufd);
			if (enable) {
				// enable hiace
				set_reg(hiace_base + HIACE_BYPASS_ACE, 0x0, 1, 0);
#ifndef DISPLAY_EFFECT_USE_FRM_END_INT
				//clean hiace interrupt
				set_reg(hiace_base + HIACE_INT_STAT, 0x1, 1, 0);
#endif
			} else {
				// disable hiace
				set_reg(hiace_base + HIACE_BYPASS_ACE, 0x1, 1, 0);
			}
			dpufb_deactivate_vsync(dpufd);

			dpufd->hiace_info[dpufd->panel_info.disp_panel_id].hiace_enable = enable;
			effect_debug_log(DEBUG_EFFECT_ENTRY, "[effect] hiace:%d\n", (int)enable);
		}
	} else {
		effect_debug_log(DEBUG_EFFECT_ENTRY, "[effect] fb%d, panel power off!\n", dpufd->index);
	}
	up(&dpufd->blank_sem);
}

int dpufb_ce_service_blank(int blank_mode, struct fb_info *info)
{
	struct dpu_fb_data_type *dpufd = NULL;
	struct dpu_panel_info *pinfo = NULL;

	if (NULL == info) {
		DPU_FB_ERR("info is NULL\n");
		return -EINVAL;
	}

	dpufd = (struct dpu_fb_data_type *)info->par;
	if (NULL == dpufd) {
		DPU_FB_ERR("[effect] dpufd is NULL\n");
		return -EINVAL;
	}

	pinfo = &(dpufd->panel_info);

	if (dpufd->index == PRIMARY_PANEL_IDX) {
		if (pinfo->hiace_support) {
			effect_debug_log(DEBUG_EFFECT_ENTRY, "[effect] blank_mode is %d\n", blank_mode);
			if (blank_mode == FB_BLANK_UNBLANK) {
				dpufb_ce_service_init();
				enable_blc(dpufd, true);
				if (dpufb_display_effect_is_need_ace(dpufd)) {
					enable_hiace(dpufd, true);
				}
			} else {
				if (dpufb_display_effect_is_need_ace(dpufd)) {
					g_hiace_service.use_last_value = true;
				}
				dpufd->hiace_info[dpufd->panel_info.disp_panel_id].gradual_frames = 0;
				dpufd->hiace_info[dpufd->panel_info.disp_panel_id].hiace_enable = false;
				dpufd->hiace_info[dpufd->panel_info.disp_panel_id].to_stop_hdr = false;
				dpufd->hiace_info[dpufd->panel_info.disp_panel_id].to_stop_sre = false;
				g_hiace_service.blc_used = false;
				// Since java has no destruct function and Gallery will refresh metadata when power on, always close HDR for Gallery when power off.
				if (dpufd->ce_ctrl.ctrl_ce_mode == CE_MODE_IMAGE) {
					dpufd->ce_ctrl.ctrl_ce_mode = CE_MODE_DISABLE;
				}
				enable_blc(dpufd, false);
				dpufb_ce_service_deinit();
			}
		}
	}
	return 0;
}

int dpufb_ce_service_get_support(struct fb_info *info, void __user *argp)
{
	struct dpu_fb_data_type *dpufd = NULL;
	struct dpu_panel_info *pinfo = NULL;
	unsigned int support = 0;
	int ret = 0;

	if (NULL == info) {
		DPU_FB_ERR("[effect] info is NULL\n");
		return -EINVAL;
	}

	if (NULL == argp) {
		DPU_FB_ERR("[effect] argp is NULL\n");
		return -EINVAL;
	}

	dpufd = (struct dpu_fb_data_type *)info->par;
	if (NULL == dpufd) {
		DPU_FB_ERR("[effect] dpufd is NULL\n");
		return -EINVAL;
	}

	pinfo = &(dpufd->panel_info);
	if (pinfo->hiace_support) {
		support = 1;
	}
	effect_debug_log(DEBUG_EFFECT_ENTRY, "[effect] support:%d\n", support);

	ret = (int)copy_to_user(argp, &support, sizeof(support));
	if (ret) {
		DPU_FB_ERR("[effect] copy_to_user failed! ret=%d.\n", ret);
		return ret;
	}

	return ret;
}

int dpufb_ce_service_get_limit(struct fb_info *info, void __user *argp)
{
	int limit = 0;
	int ret = 0;
	(void *)info;

	if (NULL == argp) {
		DPU_FB_ERR("argp is NULL\n");
		return -EINVAL;
	}

	ret = (int)copy_to_user(argp, &limit, sizeof(limit));
	if (ret) {
		DPU_FB_ERR("copy_to_user failed! ret=%d.\n", ret);
		return ret;
	}

	return ret;
}

int dpufb_ce_service_get_hiace_param(struct fb_info *info, void __user *argp){
	(void)info, (void)argp;
	return 0;
}

int dpufb_ce_service_get_param(struct fb_info *info, void __user *argp)
{
	int ret = 0;
	struct dpu_fb_data_type *dpufd = NULL;
	struct dpu_panel_info *pinfo = NULL;
	dss_display_effect_ce_t *ce_ctrl = NULL;
	dss_display_effect_al_t *al_ctrl = NULL;
	dss_display_effect_sre_t *sre_ctrl = NULL;
	dss_display_effect_metadata_t *metadata_ctrl = NULL;
	dss_ce_info_t *ce_info = NULL;
	int mode = 0;
	struct timespec64 ts;

	if (NULL == argp) {
		DPU_FB_ERR("[effect] argp is NULL\n");
		return -EINVAL;
	}

	if (NULL == info) {
		DPU_FB_ERR("[effect] info is NULL\n");
		return -EINVAL;
	}

	dpufd = (struct dpu_fb_data_type *)info->par;
	if (NULL == dpufd) {
		DPU_FB_ERR("[effect] dpufd is NULL\n");
		return -EINVAL;
	}

	pinfo = &(dpufd->panel_info);
	if (!pinfo->hiace_support) {
		effect_debug_log(DEBUG_EFFECT_ENTRY, "[effect] Don't support HIACE\n");
		return -EINVAL;
	}

	ce_ctrl = &(dpufd->ce_ctrl);
	al_ctrl = &(dpufd->al_ctrl);
	sre_ctrl = &(dpufd->sre_ctrl);
	metadata_ctrl = &(dpufd->metadata_ctrl);
	ce_info = &(dpufd->hiace_info[dpufd->panel_info.disp_panel_id]);
	mode = ce_ctrl->ctrl_ce_mode;
	if (mode != CE_MODE_DISABLE) {
		pinfo->hiace_param.iDoLCE = 1;
		pinfo->hiace_param.iDoAPLC = (mode == CE_MODE_VIDEO && dpufd->bl_level > 0 && al_ctrl->ctrl_al_value >= 0) ? 1 : 0;
	} else {
		if (ce_info->gradual_frames > 0) {
			pinfo->hiace_param.iDoLCE = -1;
			if (pinfo->hiace_param.iDoAPLC == 1) {
				pinfo->hiace_param.iDoAPLC = -1;
			}
		} else {
			pinfo->hiace_param.iDoLCE = 0;
			pinfo->hiace_param.iDoAPLC = 0;
			ce_info->to_stop_hdr = false;
		}
	}
	if (dpufd->bl_level == 0) {
		pinfo->hiace_param.iDoAPLC = 0;
	}
	pinfo->hiace_param.iDoSRE = sre_ctrl->ctrl_sre_enable;
	if (pinfo->hiace_param.iDoSRE == 0) {
		ce_info->to_stop_sre = false;
	}
	pinfo->hiace_param.iLevel = (mode == CE_MODE_VIDEO) ? 0 : 1;
	pinfo->hiace_param.iAmbientLight = (sre_ctrl->ctrl_sre_enable == 1) ? sre_ctrl->ctrl_sre_al : al_ctrl->ctrl_al_value;
	pinfo->hiace_param.iBackLight = (int)dpufd->bl_level;
	pinfo->hiace_param.iLaSensorSREOnTH = sre_ctrl->ctrl_sre_al_threshold;
	effect_debug_log(DEBUG_EFFECT_FRAME, "[effect] BLC:%d, bl:%d, panel_on:%d\n", pinfo->hiace_param.iDoAPLC, pinfo->hiace_param.iBackLight, dpufd->panel_power_on);
	ktime_get_ts64(&ts);
	pinfo->hiace_param.lTimestamp = ts.tv_sec * MSEC_PER_SEC + ts.tv_nsec / NSEC_PER_MSEC;
	if (metadata_ctrl->count <= META_DATA_SIZE) {
		memcpy(pinfo->hiace_param.Classifieresult, metadata_ctrl->metadata, (size_t)metadata_ctrl->count);//lint !e571
		pinfo->hiace_param.iResultLen = metadata_ctrl->count;
	}

	ret = (int)copy_to_user(argp, &pinfo->hiace_param, sizeof(pinfo->hiace_param));
	if (ret) {
		DPU_FB_ERR("[effect] copy_to_user(hiace_param) failed! ret=%d.\n", ret);
		return ret;
	}

	effect_debug_log(DEBUG_EFFECT_FRAME, "[effect] iLevel:%d, iAmbientLight:%d, iBackLight:%d, lTimestamp:%ld(ms)\n",
					 pinfo->hiace_param.iLevel, pinfo->hiace_param.iAmbientLight, pinfo->hiace_param.iBackLight, pinfo->hiace_param.lTimestamp);

	if (g_debug_effect & DEBUG_EFFECT_DELAY) {
		interval_algorithm.start = get_timestamp_in_us();
	}

	return ret;
}
int dpufb_ce_service_get_hist(struct fb_info *info, void __user *argp)
{
	struct dpu_fb_data_type *dpufd = NULL;
	struct dpu_panel_info *pinfo = NULL;
	ce_service_t *service = &g_hiace_service;
	int ret = 0;
	long wait_ret = 0;

	if (NULL == info) {
		DPU_FB_ERR("info is NULL\n");
		return -EINVAL;
	}

	dpufd = (struct dpu_fb_data_type *)info->par;
	if (NULL == dpufd) {
		DPU_FB_ERR("dpufd is NULL\n");
		return -EINVAL;
	}

	if (dpufd->index != PRIMARY_PANEL_IDX) {
		DPU_FB_ERR("fb%d is not supported!\n", dpufd->index);
		return -EINVAL;
	}

	pinfo = &(dpufd->panel_info);
	if (!pinfo->hiace_support) {
		DPU_FB_ERR("[effect] Don't support HIACE\n");
		return -EINVAL;
	}

	if (NULL == argp) {
		DPU_FB_ERR("[effect] argp is NULL\n");
		return -EINVAL;
	}

	DPU_FB_DEBUG("[effect] wait hist\n");

	if (g_is_effect_init) {
		unlock_fb_info(info);
		wait_ret = ce_service_wait_event(service->wq_hist, service->new_hist || service->stop);
		lock_fb_info(info);
		service->stop = false;
	} else {
		DPU_FB_ERR("[effect]wq_hist uninit\n");
		return -EINVAL;
	}

	if (g_debug_effect & DEBUG_EFFECT_DELAY) {
		interval_wait_hist.stop = get_timestamp_in_us();
		count_delay(&delay_wait_hist, interval_wait_hist.stop - interval_wait_hist.start);
	}

	down(&dpufd->hiace_hist_lock_sem);/*avoid  using  mutex_lock() but hist_lock was destoried by mutex_destory in  dpu_effect_deinit*/
	if (service->new_hist) {
		time_interval_t interval_copy_hist = {0};
		define_delay_record(delay_copy_hist, "hist copy");

		service->new_hist = false;

		if(!g_is_effect_init){
			DPU_FB_ERR("[effect] wq_hist uninit here\n");
			up(&dpufd->hiace_hist_lock_sem);
			return -EINVAL;
		}
		mutex_lock(&dpufd->hiace_info[pinfo->disp_panel_id].hist_lock);
		if (g_debug_effect & DEBUG_EFFECT_DELAY) {
			interval_copy_hist.start = get_timestamp_in_us();
		}
		ret = (int)copy_to_user(argp, dpufd->hiace_info[pinfo->disp_panel_id].histogram,
			sizeof(dpufd->hiace_info[pinfo->disp_panel_id].histogram));
		if (ret) {
			DPU_FB_ERR("[effect] copy_to_user failed(param)! ret=%d.\n", ret);
			ret = -1;
		}
		if (g_debug_effect & DEBUG_EFFECT_DELAY) {
			interval_copy_hist.stop = get_timestamp_in_us();
			count_delay(&delay_copy_hist, interval_copy_hist.stop - interval_copy_hist.start);
		}
		mutex_unlock(&dpufd->hiace_info[pinfo->disp_panel_id].hist_lock);
	} else {
		if (dpufd->panel_power_on) {
			effect_debug_log(DEBUG_EFFECT_ENTRY, "[effect] wait_event_interruptible_timeout() return %ld, -ERESTARTSYS:%d\n", wait_ret, -ERESTARTSYS);
			ret = 3;
		} else {
			effect_debug_log(DEBUG_EFFECT_ENTRY, "[effect] Panel off! wait_event_interruptible_timeout() return %ld, -ERESTARTSYS:%d\n", wait_ret, -ERESTARTSYS);
			ret = 1;
		}
	}
	up(&dpufd->hiace_hist_lock_sem);

	return ret;
}

int dpufb_ce_service_set_lut(struct fb_info *info, const void __user *argp)
{
	struct dpu_fb_data_type *dpufd = NULL;
	struct dpu_panel_info *pinfo = NULL;
	dss_display_effect_bl_t *bl_ctrl = NULL;
	hiace_interface_set_t hiace_set_interface;
	ce_service_t *service = &g_hiace_service;
	int ret = 0;
	time_interval_t interval_copy_lut = {0};

	define_delay_record(delay_copy_lut, "lut copy");

	if (g_debug_effect & DEBUG_EFFECT_DELAY) {
		interval_algorithm.stop = get_timestamp_in_us();
		count_delay(&delay_algorithm, interval_algorithm.stop - interval_algorithm.start);
	}

	dpu_check_and_return(!info, -EINVAL, ERR, "info is NULL\n");
	dpu_check_and_return(!argp, -EINVAL, ERR, "[effect] argp is NULL\n");

	dpufd = (struct dpu_fb_data_type *)info->par;
	dpu_check_and_return(!dpufd, -EINVAL, ERR, "[effect] dpufd is NULL\n");

	pinfo = &(dpufd->panel_info);
	if (!pinfo->hiace_support) {
		effect_debug_log(DEBUG_EFFECT_ENTRY, "[effect] Don't support HIACE\n");
		return -EINVAL;
	}

	effect_debug_log(DEBUG_EFFECT_FRAME, "[effect] step in\n");

	bl_ctrl = &(dpufd->bl_ctrl);

	ret = (int)copy_from_user(&hiace_set_interface, argp, sizeof(hiace_interface_set_t));
	dpu_check_and_return((ret != 0), -2, ERR, "[effect] copy_from_user(param) failed! ret=%d.\n", ret);

	dpufd->hiace_info[pinfo->disp_panel_id].thminv = hiace_set_interface.thminv;

	mutex_lock(&dpufd->hiace_info[pinfo->disp_panel_id].lut_lock);
	if (g_debug_effect & DEBUG_EFFECT_DELAY)
		interval_copy_lut.start = get_timestamp_in_us();

	ret = (int)copy_from_user(dpufd->hiace_info[pinfo->disp_panel_id].lut_table, hiace_set_interface.lut,
		sizeof(dpufd->hiace_info[pinfo->disp_panel_id].lut_table));

	if (ret != 0) {
		DPU_FB_ERR("[effect] copy_from_user lut_table failed! ret=%d.\n", ret);
		ret = -2;
	}

	if (g_debug_effect & DEBUG_EFFECT_DELAY) {
		interval_copy_lut.stop = get_timestamp_in_us();
		count_delay(&delay_copy_lut, interval_copy_lut.stop - interval_copy_lut.start);
	}
	mutex_unlock(&dpufd->hiace_info[pinfo->disp_panel_id].lut_lock);
	dpufd->hiace_info[pinfo->disp_panel_id].algorithm_result = 0;

	service->new_lut = true;
	if (g_debug_effect & DEBUG_EFFECT_DELAY)
		interval_wait_lut.start = get_timestamp_in_us();

	wake_up_interruptible(&service->wq_lut);

	return ret;
}

ssize_t dpufb_display_effect_al_ctrl_show(struct fb_info *info, char *buf)
{
	struct dpu_fb_data_type *dpufd = NULL;
	dss_display_effect_al_t *al_ctrl = NULL;

	if (info == NULL) {
		DPU_FB_ERR("[effect] info is NULL\n");
		return -1;
	}

	dpufd = (struct dpu_fb_data_type *)info->par;
	if (dpufd == NULL) {
		DPU_FB_ERR("[effect] dpufd is NULL\n");
		return -1;
	}

	al_ctrl = &(dpufd->al_ctrl);

	return snprintf(buf, PAGE_SIZE, "%d\n", al_ctrl->ctrl_al_value);
}

ssize_t dpufb_display_effect_al_ctrl_store(struct fb_info *info, const char *buf, size_t count)
{
	(void)info, (void)buf;

	return (ssize_t)count;
}

ssize_t dpufb_display_effect_ce_ctrl_show(struct fb_info *info, char *buf)
{
	struct dpu_fb_data_type *dpufd = NULL;
	dss_display_effect_ce_t *ce_ctrl = NULL;

	if (info == NULL) {
		DPU_FB_ERR("[effect] info is NULL\n");
		return -1;
	}

	dpufd = (struct dpu_fb_data_type *)info->par;
	if (dpufd == NULL) {
		DPU_FB_ERR("[effect] dpufd is NULL\n");
		return -1;
	}

	ce_ctrl = &(dpufd->ce_ctrl);

	return snprintf(buf, PAGE_SIZE, "%d\n", ce_ctrl->ctrl_ce_mode);
}

ssize_t dpufb_display_effect_ce_ctrl_store(struct fb_info *info, const char *buf, size_t count)
{
	(void)info, (void)buf;
	return (ssize_t)count;
}

ssize_t dpufb_display_effect_bl_ctrl_show(struct fb_info *info, char *buf)
{
	struct dpu_fb_data_type *dpufd = NULL;
	dss_display_effect_bl_t *bl_ctrl = NULL;

	if (info == NULL) {
		DPU_FB_ERR("[effect] info is NULL\n");
		return -1;
	}

	dpufd = (struct dpu_fb_data_type *)info->par;
	if (dpufd == NULL) {
		DPU_FB_ERR("[effect] dpufd is NULL\n");
		return -1;
	}

	bl_ctrl = &(dpufd->bl_ctrl);

	return snprintf(buf, PAGE_SIZE, "%d\n", bl_ctrl->ctrl_bl_delta);
}

ssize_t dpufb_display_effect_bl_enable_ctrl_show(struct fb_info *info, char *buf)
{
	struct dpu_fb_data_type *dpufd = NULL;
	dss_display_effect_bl_enable_t *bl_enable_ctrl = NULL;

	if (info == NULL) {
		DPU_FB_ERR("[effect] info is NULL\n");
		return -1;
	}

	dpufd = (struct dpu_fb_data_type *)info->par;
	if (dpufd == NULL) {
		DPU_FB_ERR("[effect] dpufd is NULL\n");
		return -1;
	}

	bl_enable_ctrl = &(dpufd->bl_enable_ctrl);

	return snprintf(buf, PAGE_SIZE, "%d\n", bl_enable_ctrl->ctrl_bl_enable);
}

ssize_t dpufb_display_effect_bl_enable_ctrl_store(struct fb_info *info, const char *buf,
	size_t count)
{
	(void)info, (void)buf;

	return (ssize_t)count;
}

ssize_t dpufb_display_effect_sre_ctrl_show(struct fb_info *info, char *buf)
{
	struct dpu_fb_data_type *dpufd = NULL;
	dss_display_effect_sre_t *sre_ctrl = NULL;

	if (info == NULL) {
		DPU_FB_ERR("NULL Pointer\n");
		return -1;
	}

	dpufd = (struct dpu_fb_data_type *)info->par;
	if (dpufd == NULL) {
		DPU_FB_ERR("NULL Pointer\n");
		return -1;
	}

	sre_ctrl = &(dpufd->sre_ctrl);

	return snprintf(buf, PAGE_SIZE, "sre_enable:%d, sre_al:%d\n", sre_ctrl->ctrl_sre_enable,
		sre_ctrl->ctrl_sre_al);
}

ssize_t dpufb_display_effect_sre_ctrl_store(struct fb_info *info, const char *buf, size_t count)
{
	struct dpu_fb_data_type *dpufd = NULL;
	dss_display_effect_sre_t *sre_ctrl = NULL;

	if (info == NULL) {
		DPU_FB_ERR("NULL Pointer\n");
		return -1;
	}

	dpufd = (struct dpu_fb_data_type *)info->par;
	if (dpufd == NULL) {
		DPU_FB_ERR("NULL Pointer\n");
		return -1;
	}

	sre_ctrl = &(dpufd->sre_ctrl);

	return (ssize_t)count;
}

ssize_t dpufb_display_effect_metadata_ctrl_show(struct fb_info *info, char *buf)
{
	struct dpu_fb_data_type *dpufd = NULL;

	if (info == NULL) {
		DPU_FB_ERR("NULL Pointer\n");
		return -1;
	}

	dpufd = (struct dpu_fb_data_type *)info->par;
	if (dpufd == NULL) {
		DPU_FB_ERR("NULL Pointer\n");
		return -1;
	}

	return 0;
}

ssize_t dpufb_display_effect_metadata_ctrl_store(struct fb_info *info, const char *buf,
	size_t count)
{
	(void)info, (void)buf;

	return (ssize_t)count;
}

void dpufb_display_effect_func_switch(struct dpu_fb_data_type *dpufd, const char *command)
{
	if (NULL == dpufd) {
		DPU_FB_ERR("[effect] dpufd is NULL\n");
		return;
	}
	if (NULL == command) {
		DPU_FB_ERR("[effect] command is NULL\n");
		return;
	}

	if (!strncmp("hiace:", command, strlen("hiace:"))) {
		if('0' == command[strlen("hiace:")]) {
			dpufd->panel_info.hiace_support = 0;
			DPU_FB_INFO("[effect] hiace disable\n");
		} else {
			dpufd->panel_info.hiace_support = 1;
			DPU_FB_INFO("[effect] hiace enable\n");
		}
	}
	if (!strncmp("effect_enable:", command, strlen("effect_enable:"))) {
		g_enable_effect = (int)simple_strtoul(&command[strlen("effect_enable:")], NULL, 0);
		DPU_FB_INFO("[effect] effect_enable changed to %d\n", g_enable_effect);
	}
	if (!strncmp("effect_debug:", command, strlen("effect_debug:"))) {
		g_debug_effect = (int)simple_strtoul(&command[strlen("effect_debug:")], NULL, 0);
		DPU_FB_INFO("[effect] effect_debug changed to %d\n", g_debug_effect);
	}
}

bool dpufb_display_effect_is_need_ace(struct dpu_fb_data_type *dpufd)
{
	if (dpufd == NULL) {
		DPU_FB_ERR("[effect] dpufd is NULL\n");
		return false;
	}

	return (g_enable_effect & ENABLE_EFFECT_HIACE) && (dpufd->ce_ctrl.ctrl_ce_mode > 0 ||
		dpufd->sre_ctrl.ctrl_sre_enable == 1 ||
		dpufd->hiace_info[dpufd->panel_info.disp_panel_id].to_stop_hdr ||
		dpufd->hiace_info[dpufd->panel_info.disp_panel_id].to_stop_sre);
}

bool dpufb_display_effect_is_need_blc(struct dpu_fb_data_type *dpufd)
{
	if (dpufd == NULL) {
		DPU_FB_ERR("[effect] dpufd is NULL\n");
		return false;
	}
	return dpufd->de_info.blc_enable;
}
static int delta_bl_process(struct dpu_fb_data_type *dpufd, int backlight_in)
{
	int ret = 0;
	int bl_min;
	int bl_max;
	bool hbm_enable = false;
	bool amoled_diming_enable = false;
	int hbm_threshold_backlight;
	int hbm_min_backlight;
	int hbm_max_backlight;
	int hiac_dbv_thres;
	int hiac_dbv_xcc_thres;
	int hiac_dbv_xcc_min_thres;
	int current_hiac_backlight;
	int origin_hiac_backlight;
	int current_hiac_delta_bl = 0;
	int temp_hiac_backlight = 0;
	int temp_lowc_backlight = 0;

	if (dpufd == NULL) {
		DPU_FB_ERR("[effect] dpufd is NULL \n");
		return -1;
	}

	bl_min = (int)dpufd->panel_info.bl_min;
	bl_max = (int)dpufd->panel_info.bl_max;
	hbm_enable = dpufd->de_info.amoled_param.HBMEnable ? true : false;
	amoled_diming_enable = dpufd->de_info.amoled_param.amoled_diming_enable ? true : false;
	hbm_threshold_backlight = dpufd->de_info.amoled_param.HBM_Threshold_BackLight;
	hbm_min_backlight = dpufd->de_info.amoled_param.HBM_Min_BackLight;
	hbm_max_backlight = dpufd->de_info.amoled_param.HBM_Max_BackLight;
	hiac_dbv_thres = dpufd->de_info.amoled_param.Hiac_DBVThres;
	hiac_dbv_xcc_thres = dpufd->de_info.amoled_param.Hiac_DBV_XCCThres;
	hiac_dbv_xcc_min_thres = dpufd->de_info.amoled_param.Hiac_DBV_XCC_MinThres;

	origin_hiac_backlight = (backlight_in - bl_min) * (hbm_max_backlight - hbm_min_backlight) /
		(bl_max - bl_min) + hbm_min_backlight;
	current_hiac_backlight = origin_hiac_backlight;

	if (hbm_enable && (hbm_max_backlight > hbm_threshold_backlight) &&
		(hbm_threshold_backlight > (hbm_min_backlight + 1))) {
		if ((backlight_in >= bl_min) && (backlight_in < hbm_threshold_backlight)) {
			current_hiac_delta_bl = hbm_min_backlight +
				((current_hiac_backlight - hbm_min_backlight) *
				(hbm_max_backlight - hbm_min_backlight) /
				(hbm_threshold_backlight - 1 - hbm_min_backlight)) -
				current_hiac_backlight;
		} else if ((current_hiac_backlight >= hbm_threshold_backlight)
			&& (backlight_in <= bl_max)) {
			current_hiac_delta_bl = hbm_max_backlight - current_hiac_backlight;
		}
		current_hiac_backlight = current_hiac_backlight + current_hiac_delta_bl;
		DPU_FB_DEBUG("[effect] hiac_delta =  %d hiac_backlight =  %d, backlight = %d",
			current_hiac_delta_bl, current_hiac_backlight, backlight_in);
	}

	if (amoled_diming_enable) {
		if ((current_hiac_backlight <= hiac_dbv_thres) &&
			(current_hiac_backlight > hiac_dbv_xcc_thres)) {
			current_hiac_delta_bl = hiac_dbv_thres - origin_hiac_backlight;
		} else if (current_hiac_backlight <= hiac_dbv_xcc_thres) {
			temp_hiac_backlight = (current_hiac_backlight - hbm_min_backlight) *
				(hiac_dbv_thres - hiac_dbv_xcc_min_thres) /
				(hiac_dbv_xcc_thres - hbm_min_backlight) + hiac_dbv_xcc_min_thres;
			current_hiac_delta_bl = temp_hiac_backlight - origin_hiac_backlight;
		}
		DPU_FB_DEBUG("[effect] hiac_delta =  %d hiac_backlight =  %d, backlight = %d",
			current_hiac_delta_bl, current_hiac_backlight, backlight_in);
	}

	dpufd->de_info.blc_delta = (bl_max - bl_min) * current_hiac_delta_bl /
		(hbm_max_backlight - hbm_min_backlight);

	if (amoled_diming_enable && dpufd->panel_info.dbv_curve_mapped_support &&
		dpufd->panel_info.is_dbv_need_mapped) {
		if ((current_hiac_backlight <= hiac_dbv_thres) &&
			(current_hiac_backlight > hiac_dbv_xcc_thres)) {
			temp_lowc_backlight = (hiac_dbv_thres - hbm_min_backlight) *
			(bl_max - bl_min) / (hbm_max_backlight - hbm_min_backlight) + bl_min;
			dpufd->de_info.blc_delta =
				(int)dbv_curve_noliner_to_liner_map[dpufd->panel_info.dbv_map_index]
				[temp_lowc_backlight] - backlight_in;
		} else if (current_hiac_backlight <= hiac_dbv_xcc_thres) {
			temp_hiac_backlight = (current_hiac_backlight - hbm_min_backlight) *
				(hiac_dbv_thres - hiac_dbv_xcc_min_thres) /
				(hiac_dbv_xcc_thres - hbm_min_backlight) + hiac_dbv_xcc_min_thres;
			temp_lowc_backlight = (temp_hiac_backlight - hbm_min_backlight) *
				(bl_max - bl_min) / (hbm_max_backlight - hbm_min_backlight) +
				bl_min;
			dpufd->de_info.blc_delta =
				(int)dbv_curve_noliner_to_liner_map[dpufd->panel_info.dbv_map_index]
				[temp_lowc_backlight] - backlight_in;
		} else {
			dpufd->de_info.blc_delta =
				(int)dbv_curve_noliner_to_liner_map[dpufd->panel_info.dbv_map_index]
				[backlight_in] - backlight_in;
		}
		DPU_FB_DEBUG("[effect] hiac_delta =  %d hiac_backlight = %d, backlight = %d",
			current_hiac_delta_bl, current_hiac_backlight, backlight_in);
	}

	DPU_FB_DEBUG("[effect] first screen on!dbv_map_index =%d delta: -10000 -> %d bl: %d (%d)\n",
		dpufd->panel_info.dbv_map_index, dpufd->de_info.blc_delta,
		backlight_in, backlight_in + dpufd->de_info.blc_delta);

	return ret;
}

static void handle_first_deltabl(struct dpu_fb_data_type *dpufd, int backlight_in)
{

	bool hbm_enable = false;
	bool amoled_diming_enable = false;
	int bl_min;
	int bl_max;

	if (dpufd == NULL) {
		DPU_FB_ERR("[effect] dpufd is NULL\n");
		return;
	}

	hbm_enable = dpufd->de_info.amoled_param.HBMEnable ? true : false;
	amoled_diming_enable = dpufd->de_info.amoled_param.amoled_diming_enable ? true : false;
	bl_min = (int)dpufd->panel_info.bl_min;
	bl_max = (int)dpufd->panel_info.bl_max;

	if (!dpufb_display_effect_is_need_blc(dpufd))
		return;

	if (!(hbm_enable || amoled_diming_enable))
		return;

	if (dpufd->bl_level == 0) {
		dpufd->de_info.blc_delta = SCREEN_OFF_BLC_DELTA;
		return;
	}

	if (backlight_in > 0 && (backlight_in < bl_min || backlight_in > bl_max))
		return;

	if (dpufd->de_info.blc_delta == SCREEN_OFF_BLC_DELTA)
		delta_bl_process(dpufd, backlight_in);
}

bool dpufb_display_effect_check_bl_value(int curr, int last) {
	return false;
}

bool dpufb_display_effect_check_bl_delta(int curr, int last) {
	return false;
}

static inline void display_engine_bl_debug_print(int bl_in, int bl_out, int delta) {
	static int last_delta = 0;
	static int last_bl = 0;
	static int last_bl_out = 0;
	static int count = 0;
	if (dpufb_display_effect_check_bl_value(bl_in, last_bl) ||
		dpufb_display_effect_check_bl_value(bl_out, last_bl_out) ||
		dpufb_display_effect_check_bl_delta(delta, last_delta)) {
		if (count == 0) {
			DPU_FB_INFO("[effect] last delta:%d bl:%d->%d\n", last_delta, last_bl,
				last_bl_out);
		}
		count = DISPLAYENGINE_BL_DEBUG_FRAMES;
	}
	if (count > 0) {
		DPU_FB_INFO("[effect] delta:%d bl:%d->%d\n", delta, bl_in, bl_out);
		count--;
	}
	last_delta = delta;
	last_bl = bl_in;
	last_bl_out = bl_out;
}

unsigned short dbv_curve_noliner_to_liner_map[DBV_MAP_INDEX][DBV_MAP_COUNTS] = {
	{
		0, 1, 2, 3, 4, 5, 6, 7, 8, 9,
		10, 11, 12, 13, 14, 15, 16, 17, 18, 19,
		20, 21, 22, 23, 24, 25, 26, 27, 28, 29,
		30, 31, 39, 55, 79, 94, 104, 119, 136, 142,
		146, 152, 156, 161, 162, 167, 171, 177, 182, 186,
		189, 192, 195, 201, 202, 206, 210, 214, 216, 219,
		225, 225, 229, 231, 234, 236, 240, 243, 245, 245,
		248, 250, 253, 257, 259, 262, 264, 267, 270, 273,
		276, 276, 278, 281, 284, 287, 290, 293, 293, 295,
		299, 301, 304, 306, 307, 309, 310, 311, 313, 316,
		319, 320, 320, 323, 325, 328, 330, 330, 332, 332,
		335, 337, 340, 340, 342, 344, 347, 349, 350, 350,
		351, 354, 356, 359, 360, 361, 361, 363, 366, 368,
		368, 371, 371, 372, 375, 377, 378, 380, 380, 382,
		385, 385, 387, 387, 390, 391, 394, 394, 394, 397,
		399, 401, 402, 402, 403, 406, 407, 408, 409, 411,
		411, 412, 413, 415, 416, 418, 418, 419, 421, 422,
		424, 424, 424, 425, 426, 428, 430, 431, 432, 432,
		434, 435, 437, 438, 440, 441, 441, 442, 444, 445,
		447, 449, 450, 450, 451, 452, 454, 456, 457, 458,
		458, 459, 460, 461, 463, 463, 464, 466, 467, 469,
		470, 470, 471, 474, 474, 476, 478, 479, 480, 482,
		483, 483, 485, 486, 486, 486, 488, 489, 490, 492,
		492, 493, 495, 497, 498, 499, 501, 502, 502, 504,
		505, 507, 508, 509, 509, 510, 511, 511, 512, 514,
		515, 517, 519, 520, 521, 521, 522, 524, 526, 527,
		529, 530, 531, 532, 532, 533, 534, 535, 536, 537,
		539, 540, 542, 543, 544, 544, 544, 545, 546, 547,
		548, 549, 550, 551, 551, 551, 551, 552, 553, 554,
		555, 555, 556, 557, 558, 559, 560, 561, 562, 563,
		564, 565, 565, 566, 566, 567, 567, 568, 569, 569,
		571, 571, 572, 573, 574, 575, 576, 577, 577, 578,
		579, 579, 580, 581, 582, 583, 583, 583, 583, 584,
		585, 586, 587, 588, 589, 590, 591, 592, 593, 594,
		595, 596, 597, 598, 598, 599, 599, 599, 600, 601,
		601, 603, 603, 603, 604, 606, 606, 607, 608, 609,
		610, 611, 612, 613, 614, 615, 615, 615, 615, 615,
		616, 617, 619, 620, 621, 622, 623, 624, 625, 626,
		627, 627, 628, 629, 630, 630, 631, 631, 631, 631,
		632, 633, 633, 635, 635, 636, 638, 639, 640, 641,
		642, 642, 643, 644, 645, 646, 646, 646, 646, 647,
		647, 648, 649, 650, 650, 652, 653, 654, 655, 656,
		657, 658, 659, 661, 662, 662, 662, 662, 662, 663,
		664, 664, 665, 666, 667, 667, 669, 670, 671, 672,
		672, 674, 675, 676, 677, 678, 678, 678, 678, 678,
		679, 680, 681, 681, 682, 684, 685, 686, 686, 686,
		687, 689, 690, 691, 692, 693, 694, 694, 694, 694,
		694, 695, 696, 696, 697, 699, 699, 701, 702, 703,
		704, 705, 706, 707, 708, 709, 710, 710, 710, 710,
		710, 710, 711, 712, 713, 714, 714, 716, 717, 719,
		720, 720, 722, 722, 725, 725, 725, 725, 726, 726,
		726, 726, 726, 727, 730, 730, 732, 733, 735, 735,
		735, 735, 735, 735, 735, 735, 735, 735, 735, 736,
		736, 737, 737, 738, 738, 738, 739, 739, 739, 740,
		740, 741, 741, 742, 742, 742, 743, 743, 744, 744,
		744, 745, 745, 746, 746, 746, 747, 747, 747, 747,
		748, 748, 749, 749, 750, 750, 751, 751, 751, 752,
		752, 752, 753, 753, 754, 754, 754, 754, 755, 756,
		756, 756, 757, 757, 758, 758, 758, 759, 759, 759,
		759, 760, 760, 761, 762, 762, 762, 762, 763, 763,
		764, 764, 764, 764, 765, 765, 765, 766, 766, 766,
		767, 768, 768, 769, 769, 769, 769, 770, 770, 771,
		772, 772, 773, 774, 775, 776, 776, 777, 777, 779,
		779, 779, 780, 781, 781, 781, 782, 783, 783, 784,
		785, 786, 786, 787, 788, 788, 789, 789, 790, 791,
		792, 792, 793, 793, 794, 795, 795, 796, 797, 797,
		798, 798, 799, 799, 799, 804, 805, 805, 806, 806,
		807, 808, 809, 810, 810, 811, 812, 812, 813, 813,
		814, 815, 816, 816, 817, 817, 818, 818, 819, 820,
		820, 821, 821, 822, 822, 823, 824, 824, 824, 826,
		827, 827, 828, 828, 829, 830, 830, 831, 832, 833,
		833, 833, 834, 835, 835, 836, 836, 837, 838, 838,
		838, 839, 840, 840, 841, 841, 842, 843, 844, 845,
		845, 845, 846, 847, 848, 848, 848, 850, 850, 851,
		851, 852, 852, 853, 853, 853, 854, 855, 856, 856,
		857, 857, 858, 858, 859, 859, 861, 861, 862, 862,
		863, 864, 864, 864, 865, 866, 867, 867, 868, 868,
		869, 869, 870, 870, 871, 871, 872, 873, 873, 874,
		874, 875, 875, 876, 876, 876, 878, 879, 879, 880,
		881, 881, 882, 882, 883, 884, 884, 885, 885, 886,
		886, 887, 887, 887, 888, 889, 890, 890, 891, 891,
		892, 892, 893, 893, 894, 894, 895, 896, 896, 897,
		898, 899, 899, 899, 901, 901, 902, 902, 902, 903,
		903, 904, 904, 905, 905, 906, 907, 907, 908, 908,
		909, 909, 910, 910, 911, 912, 912, 913, 914, 915,
		915, 915, 916, 917, 918, 918, 919, 919, 919, 920,
		920, 921, 921, 921, 922, 922, 923, 924, 924, 925,
		926, 926, 926, 927, 928, 928, 928, 929, 930, 930,
		931, 932, 932, 933, 933, 934, 935, 935, 936, 936,
		937, 937, 938, 938, 938, 939, 940, 940, 941, 942,
		942, 943, 943, 944, 944, 944, 946, 946, 947, 947,
		948, 949, 949, 950, 950, 950, 951, 952, 953, 953,
		954, 954, 955, 955, 955, 956, 956, 957, 958, 958,
		959, 959, 960, 960, 961, 961, 962, 963, 963, 964,
		964, 965, 966, 966, 966, 967, 967, 968, 969, 969,
		970, 970, 970, 971, 972, 972, 972, 973, 973, 974,
		975, 976, 976, 976, 977, 977, 978, 978, 979, 980,
		980, 981, 981, 982, 983, 983, 984, 984, 984, 985,
		986, 986, 987, 987, 988, 988, 989, 989, 989, 990,
		990, 991, 992, 993, 994, 995, 996, 997, 998, 999,
		1000, 1000, 1002, 1003, 1004, 1005, 1006, 1007, 1008, 1009,
		1010, 1011, 1012, 1013, 1014, 1015, 1016, 1017, 1018, 1019,
		1020, 1021, 1022, 1023,
	},
	{
		0, 1, 2, 3, 4, 5, 6, 7, 8, 9,
		10, 11, 12, 13, 14, 15, 16, 17, 18, 19,
		20, 21, 22, 23, 24, 25, 26, 27, 28, 29,
		30, 31, 38, 51, 52, 60, 64, 73, 74, 82,
		84, 90, 94, 94, 99, 105, 105, 109, 114, 114,
		118, 123, 123, 129, 133, 133, 138, 138, 143, 147,
		147, 152, 152, 155, 159, 159, 163, 166, 166, 170,
		173, 177, 181, 181, 184, 188, 190, 190, 194, 198,
		201, 201, 205, 208, 208, 212, 216, 218, 222, 225,
		225, 229, 233, 234, 235, 235, 237, 238, 241, 241,
		243, 244, 246, 246, 249, 250, 252, 252, 254, 256,
		258, 258, 260, 262, 263, 263, 266, 268, 269, 269,
		271, 273, 275, 275, 277, 279, 281, 282, 282, 285,
		287, 288, 288, 290, 292, 294, 294, 296, 298, 300,
		301, 301, 301, 301, 304, 306, 307, 309, 309, 311,
		313, 315, 315, 317, 319, 321, 322, 322, 324, 324,
		325, 326, 326, 327, 328, 330, 330, 330, 332, 333,
		333, 334, 335, 336, 337, 337, 339, 339, 340, 341,
		342, 344, 344, 345, 346, 347, 347, 348, 349, 350,
		351, 353, 355, 355, 355, 357, 358, 359, 360, 360,
		361, 362, 363, 364, 364, 365, 366, 367, 369, 370,
		371, 372, 372, 373, 374, 375, 376, 376, 377, 378,
		380, 380, 382, 382, 382, 384, 385, 386, 386, 387,
		388, 389, 390, 391, 391, 392, 393, 394, 396, 396,
		398, 398, 399, 400, 401, 402, 403, 403, 404, 405,
		406, 407, 408, 410, 411, 412, 413, 413, 414, 415,
		416, 417, 418, 419, 419, 420, 421, 423, 423, 425,
		426, 426, 427, 428, 429, 430, 431, 432, 432, 433,
		434, 434, 434, 436, 437, 438, 439, 441, 441, 442,
		443, 444, 446, 447, 448, 449, 449, 450, 452, 453,
		454, 454, 454, 454, 454, 455, 456, 458, 459, 460,
		462, 462, 462, 464, 465, 466, 468, 469, 470, 471,
		471, 472, 474, 475, 476, 478, 478, 480, 481, 482,
		482, 485, 485, 486, 487, 488, 490, 491, 492, 494,
		494, 494, 494, 494, 494, 495, 496, 497, 499, 500,
		501, 502, 503, 504, 504, 507, 507, 508, 510, 510,
		512, 513, 514, 515, 517, 518, 519, 520, 521, 521,
		523, 524, 525, 526, 528, 529, 530, 531, 533, 534,
		534, 534, 534, 534, 534, 535, 536, 537, 537, 539,
		540, 541, 542, 543, 545, 546, 547, 549, 550, 551,
		552, 553, 555, 556, 556, 557, 558, 559, 561, 562,
		563, 565, 565, 567, 568, 569, 569, 569, 569, 569,
		571, 572, 573, 574, 574, 574, 575, 577, 578, 579,
		580, 581, 583, 584, 585, 587, 587, 589, 590, 591,
		593, 594, 594, 596, 597, 597, 599, 599, 599, 599,
		599, 600, 601, 602, 603, 605, 606, 607, 608, 610,
		611, 611, 612, 613, 615, 615, 616, 618, 619, 621,
		622, 623, 624, 625, 627, 627, 628, 629, 629, 629,
		629, 629, 630, 631, 632, 633, 634, 635, 638, 639,
		640, 641, 642, 644, 644, 645, 646, 647, 648, 650,
		651, 652, 654, 654, 655, 656, 658, 659, 659, 659,
		659, 659, 659, 661, 661, 662, 663, 664, 666, 668,
		670, 670, 672, 674, 674, 674, 674, 674, 674, 674,
		674, 674, 674, 674, 675, 675, 675, 676, 676, 677,
		677, 677, 677, 678, 679, 679, 679, 680, 680, 681,
		681, 681, 682, 682, 683, 683, 683, 684, 684, 685,
		685, 685, 686, 686, 687, 687, 687, 688, 689, 689,
		689, 690, 690, 693, 693, 694, 694, 695, 696, 696,
		696, 696, 697, 698, 698, 698, 698, 699, 699, 700,
		700, 700, 701, 702, 702, 702, 703, 704, 704, 704,
		704, 705, 706, 706, 706, 706, 707, 708, 708, 708,
		709, 709, 710, 710, 710, 711, 711, 712, 712, 712,
		713, 713, 714, 714, 715, 715, 715, 716, 716, 717,
		717, 718, 719, 719, 719, 720, 721, 722, 727, 727,
		729, 729, 730, 731, 731, 732, 733, 733, 734, 735,
		735, 736, 736, 737, 738, 738, 740, 740, 741, 742,
		742, 743, 744, 744, 745, 746, 747, 747, 748, 749,
		750, 750, 751, 752, 752, 752, 753, 754, 760, 761,
		761, 762, 763, 763, 765, 765, 765, 766, 767, 768,
		769, 769, 770, 771, 771, 771, 772, 773, 773, 774,
		776, 776, 776, 777, 778, 778, 779, 780, 780, 781,
		782, 782, 783, 784, 784, 785, 786, 786, 791, 792,
		792, 794, 794, 795, 796, 797, 797, 798, 799, 799,
		799, 801, 801, 802, 803, 803, 803, 804, 805, 805,
		806, 807, 807, 809, 809, 810, 811, 811, 812, 813,
		813, 814, 815, 816, 816, 816, 817, 818, 824, 824,
		825, 825, 826, 826, 827, 828, 828, 829, 830, 831,
		831, 832, 832, 833, 834, 835, 835, 836, 837, 837,
		838, 839, 839, 839, 840, 841, 841, 842, 843, 843,
		843, 844, 845, 845, 846, 847, 848, 848, 849, 849,
		849, 855, 857, 857, 858, 859, 859, 860, 861, 861,
		862, 862, 863, 864, 864, 864, 865, 865, 866, 866,
		867, 868, 868, 869, 870, 870, 870, 871, 872, 873,
		873, 874, 874, 877, 878, 878, 879, 879, 880, 880,
		881, 882, 882, 889, 889, 889, 890, 891, 891, 892,
		893, 894, 895, 895, 895, 896, 897, 897, 897, 899,
		899, 899, 900, 901, 901, 901, 902, 903, 903, 904,
		904, 905, 905, 906, 907, 907, 907, 909, 909, 910,
		911, 911, 912, 912, 913, 913, 914, 920, 920, 920,
		921, 922, 922, 923, 923, 924, 925, 925, 926, 927,
		927, 928, 928, 929, 930, 930, 931, 932, 932, 933,
		933, 934, 934, 935, 935, 936, 936, 936, 938, 938,
		938, 938, 939, 940, 940, 941, 942, 943, 943, 944,
		945, 945, 946, 952, 953, 953, 954, 954, 956, 957,
		957, 957, 958, 959, 959, 960, 960, 961, 961, 963,
		963, 963, 964, 965, 966, 966, 967, 968, 968, 968,
		969, 970, 970, 970, 971, 971, 972, 972, 973, 974,
		974, 974, 975, 976, 976, 976, 977, 978, 978, 978,
		986, 986, 987, 987, 988, 988, 989, 989, 990, 990,
		991, 991, 992, 992, 994, 995, 996, 997, 998, 999,
		1000, 1001, 1002, 1003, 1004, 1005, 1006, 1007, 1008, 1009,
		1010, 1011, 1011, 1013, 1014, 1015, 1016, 1017, 1018, 1019,
		1020, 1021, 1022, 1023,
	},
	{
		0, 1, 2, 3, 4, 5, 6, 7, 8, 9,
		10, 11, 12, 13, 14, 15, 16, 17, 18, 19,
		20, 21, 22, 23, 24, 25, 26, 27, 28, 29,
		30, 31, 38, 46, 54, 59, 66, 72, 79, 87,
		93, 98, 103, 109, 116, 119, 122, 123, 125, 127,
		129, 130, 131, 133, 134, 136, 138, 140, 141, 143,
		145, 146, 147, 149, 150, 151, 155, 158, 162, 167,
		170, 174, 178, 182, 185, 189, 192, 196, 199, 202,
		206, 209, 213, 215, 219, 222, 225, 228, 231, 235,
		236, 237, 238, 240, 241, 243, 243, 246, 247, 249,
		250, 252, 252, 254, 255, 256, 258, 259, 260, 262,
		263, 265, 266, 268, 269, 271, 273, 274, 275, 277,
		278, 280, 281, 282, 284, 285, 287, 288, 289, 290,
		292, 294, 295, 297, 298, 299, 301, 303, 304, 306,
		308, 309, 310, 311, 313, 314, 315, 317, 319, 320,
		322, 323, 324, 324, 326, 327, 327, 329, 330, 331,
		332, 333, 334, 335, 336, 337, 338, 340, 341, 341,
		343, 344, 344, 346, 347, 348, 349, 350, 350, 352,
		353, 354, 355, 356, 356, 358, 358, 359, 361, 361,
		362, 364, 364, 365, 366, 367, 368, 369, 370, 370,
		372, 372, 373, 374, 375, 376, 377, 378, 378, 379,
		380, 381, 382, 383, 384, 384, 385, 386, 387, 388,
		389, 390, 390, 391, 392, 393, 394, 395, 396, 396,
		398, 398, 399, 400, 401, 402, 402, 403, 404, 405,
		405, 406, 407, 408, 408, 410, 410, 411, 412, 413,
		413, 414, 415, 416, 417, 417, 418, 419, 420, 421,
		421, 422, 422, 424, 424, 425, 426, 427, 427, 428,
		429, 430, 430, 431, 432, 433, 433, 434, 434, 434,
		435, 436, 437, 437, 439, 440, 441, 442, 443, 444,
		445, 446, 447, 447, 449, 450, 451, 452, 453, 453,
		455, 456, 457, 458, 459, 459, 460, 461, 462, 464,
		465, 465, 466, 467, 468, 469, 469, 472, 472, 473,
		474, 474, 475, 477, 478, 479, 480, 481, 482, 483,
		484, 485, 485, 487, 488, 489, 490, 491, 491, 493,
		494, 495, 496, 496, 497, 499, 500, 501, 502, 502,
		504, 505, 506, 507, 507, 508, 510, 511, 512, 513,
		513, 514, 516, 517, 518, 518, 520, 520, 522, 523,
		524, 524, 526, 527, 528, 529, 529, 530, 532, 533,
		534, 534, 535, 535, 537, 538, 539, 540, 540, 542,
		544, 545, 545, 546, 548, 549, 550, 551, 551, 552,
		554, 555, 556, 556, 557, 558, 560, 561, 562, 562,
		564, 565, 566, 567, 567, 568, 570, 571, 572, 573,
		573, 574, 576, 577, 578, 578, 580, 581, 583, 584,
		584, 584, 586, 587, 588, 589, 589, 590, 592, 593,
		594, 594, 595, 596, 599, 600, 600, 601, 602, 604,
		605, 605, 605, 606, 607, 609, 610, 611, 611, 614,
		614, 615, 616, 616, 618, 620, 621, 622, 622, 622,
		623, 626, 626, 627, 627, 628, 629, 632, 633, 633,
		633, 635, 637, 638, 638, 639, 639, 641, 642, 644,
		644, 645, 645, 647, 647, 649, 650, 651, 653, 654,
		654, 655, 656, 657, 659, 660, 660, 661, 662, 663,
		664, 665, 667, 669, 670, 671, 671, 673, 673, 674,
		674, 674, 674, 674, 674, 674, 674, 674, 674, 675,
		676, 676, 677, 678, 679, 680, 680, 681, 681, 682,
		682, 683, 683, 683, 683, 683, 683, 683, 684, 685,
		686, 687, 688, 688, 689, 690, 691, 691, 692, 693,
		693, 694, 695, 695, 696, 697, 697, 698, 699, 699,
		700, 700, 700, 701, 702, 703, 703, 703, 703, 703,
		703, 703, 704, 705, 706, 706, 707, 708, 708, 709,
		710, 710, 711, 712, 712, 713, 714, 714, 715, 716,
		717, 717, 718, 718, 718, 719, 720, 720, 721, 721,
		722, 723, 723, 723, 723, 723, 723, 723, 724, 725,
		725, 726, 727, 727, 728, 729, 729, 730, 730, 731,
		731, 733, 733, 734, 735, 735, 736, 736, 737, 737,
		738, 739, 739, 740, 741, 742, 742, 743, 743, 743,
		743, 743, 743, 743, 743, 744, 745, 745, 746, 746,
		747, 748, 748, 749, 750, 750, 751, 752, 752, 753,
		753, 753, 754, 754, 755, 756, 757, 757, 758, 761,
		762, 763, 763, 763, 763, 763, 763, 763, 763, 764,
		765, 766, 766, 768, 769, 770, 771, 772, 773, 774,
		775, 776, 778, 780, 780, 783, 783, 783, 783, 783,
		783, 783, 783, 784, 785, 786, 787, 788, 789, 789,
		790, 791, 792, 794, 796, 796, 798, 798, 799, 801,
		802, 802, 803, 803, 803, 803, 803, 803, 803, 804,
		806, 807, 807, 808, 809, 809, 811, 812, 813, 815,
		815, 817, 818, 819, 820, 821, 822, 823, 823, 823,
		823, 823, 823, 823, 824, 825, 825, 825, 827, 827,
		829, 831, 832, 833, 834, 835, 836, 837, 838, 838,
		840, 841, 842, 842, 843, 843, 843, 843, 843, 843,
		843, 843, 844, 844, 846, 847, 850, 852, 853, 854,
		855, 856, 857, 857, 857, 858, 859, 861, 862, 863,
		863, 863, 863, 863, 863, 863, 863, 863, 864, 865,
		867, 868, 870, 871, 872, 873, 873, 873, 875, 876,
		876, 878, 878, 879, 880, 882, 883, 883, 883, 883,
		883, 883, 883, 883, 883, 885, 886, 888, 888, 889,
		889, 890, 891, 891, 894, 894, 895, 896, 897, 898,
		899, 900, 902, 903, 903, 903, 903, 903, 903, 903,
		904, 904, 905, 906, 906, 907, 908, 909, 910, 911,
		912, 913, 914, 915, 916, 917, 918, 920, 921, 921,
		921, 922, 923, 923, 923, 923, 923, 923, 923, 923,
		923, 925, 927, 927, 929, 929, 930, 931, 932, 933,
		934, 936, 937, 937, 938, 939, 940, 941, 942, 943,
		943, 943, 943, 943, 943, 943, 943, 943, 944, 944,
		946, 947, 948, 949, 949, 950, 951, 952, 953, 953,
		953, 954, 959, 960, 961, 961, 962, 963, 963, 963,
		963, 963, 963, 963, 963, 963, 964, 965, 966, 966,
		968, 969, 969, 969, 970, 971, 972, 973, 974, 978,
		979, 979, 980, 982, 983, 983, 983, 983, 983, 983,
		983, 983, 983, 983, 984, 985, 985, 985, 986, 986,
		989, 990, 991, 992, 994, 995, 996, 997, 998, 999,
		1000, 1001, 1002, 1003, 1004, 1005, 1006, 1007, 1008, 1009,
		1010, 1011, 1012, 1013, 1014, 1015, 1016, 1017, 1018, 1019,
		1020, 1021, 1022, 1023,
	}
};

static void dpufb_dbv_curve_mapped(struct dpu_fb_data_type *dpufd, int backlight_in,
	int *backlight_out)
{
	if (dpufd == NULL || backlight_out == NULL) {
		DPU_FB_ERR("[effect] dpufd is NULL\n");
		return;
	}
	if (dpufd->panel_info.dbv_curve_mapped_support && dpufd->panel_info.is_dbv_need_mapped
		&& (backlight_in < DBV_MAP_COUNTS) && (backlight_in >= 0)
		&& (dpufd->panel_info.dbv_map_index < DBV_MAP_INDEX)) {
		int bl = (int)dbv_curve_noliner_to_liner_map[dpufd->panel_info.dbv_map_index]
			[backlight_in];

		*backlight_out = bl;
		DPU_FB_DEBUG("[effect] delta:%d bl:%d(bl_in:%d)->%d\n",
			dpufd->de_info.blc_delta, dpufd->bl_level, backlight_in, *backlight_out);
	}
}

bool dpufb_display_effect_is_low_precision_mapping(int manufacture_brightness_mode) {
	return (dpu_runmode_is_factory() && (manufacture_brightness_mode == 0));
}

void dpufb_display_effect_handle_first_deltabl(struct dpu_fb_data_type *dpufd, int backlight_in) {
	if (dpufd == NULL) {
		DPU_FB_ERR("[effect] dpufd is NULL\n");
		return;
	}
	if (dpufd->de_param.manufacture_brightness.engine_mode == 0) {
		handle_first_deltabl(dpufd, backlight_in);
	} else if (dpufd->de_param.manufacture_brightness.engine_mode == 1 &&
		dpufd->de_info.blc_delta == SCREEN_OFF_BLC_DELTA) {
		dpufd->de_info.blc_delta = 0;
	}
}

static void dpufb_display_effect_update_backlight_param(struct dpu_fb_data_type *dpufd,
	int backlight_in, int *backlight_out, bool *changed, int *delta)
{
	if (changed == NULL || delta == NULL) {
		DPU_FB_ERR("[effect] changed or delta is NULL\n");
		return;
	}
	if (dpufb_display_effect_is_need_blc(dpufd)) {
		int bl = MIN((int)dpufd->panel_info.bl_max,
			MAX((int)dpufd->panel_info.bl_min,
			backlight_in + dpufd->de_info.blc_delta));
		if (dpufd->de_info.amoled_param.dc_brightness_dimming_enable_real) {
			if (backlight_in <=
				dpufd->de_info.amoled_param.dc_lowac_dbv_thre)
				bl = dpufd->de_info.amoled_param.dc_lowac_fixed_dbv_thres;
		} else if (dpufd->de_info.amoled_param.amoled_diming_enable) {
			if (backlight_in >=
				dpufd->de_info.amoled_param.Lowac_DBV_XCCThres &&
				backlight_in <= dpufd->de_info.amoled_param.Lowac_DBVThres)
				bl = dpufd->de_info.amoled_param.Lowac_Fixed_DBVThres;
		}
		if (*backlight_out != bl) {
			DPU_FB_DEBUG("[effect] delta:%d bl:%d(%d)->%d\n",
				dpufd->de_info.blc_delta, backlight_in,
				dpufd->bl_level, bl);
			*backlight_out = bl;
			*changed = true;
			dpufd->de_info.blc_used = true;
		}
		*delta = dpufd->de_info.blc_delta;
	} else {
		if (dpufd->de_info.blc_used) {
			if (*backlight_out != backlight_in) {
				DPU_FB_DEBUG("[effect] bl:%d->%d\n", *backlight_out, backlight_in);
				*backlight_out = backlight_in;
				*changed = true;
			}
			dpufd->de_info.blc_used = false;
		}
		*delta = 0;
	}
}

bool dpufb_display_effect_fine_tune_backlight(struct dpu_fb_data_type *dpufd, int backlight_in,
	int *backlight_out)
{
	bool changed = false;
	struct dpu_panel_info *pinfo = NULL;
	int delta = 0;
	if (dpufd == NULL) {
		DPU_FB_ERR("[effect] dpufd is NULL\n");
		return false;
	}

	pinfo = &(dpufd->panel_info);
	if (pinfo == NULL) {
		DPU_FB_ERR("pinfo is NULL!\n");
		return false;
	}

	if (backlight_out == NULL) {
		DPU_FB_ERR("[effect] backlight_out is NULL\n");
		return false;
	}

	if (dpufd->panel_info.need_skip_delta) {
		dpufd->panel_info.need_skip_delta = 0;
		return changed;
	}
	handle_first_deltabl(dpufd, backlight_in);


	dpufb_display_effect_handle_first_deltabl(dpufd, backlight_in);

	if (dpufd->bl_level > 0)
		dpufb_display_effect_update_backlight_param(dpufd, backlight_in, backlight_out,
			&changed, &delta);

	if (dpufb_display_effect_is_low_precision_mapping(
		dpufd->de_param.manufacture_brightness.engine_mode)) {
		dpufb_dbv_curve_mapped(dpufd,*backlight_out,backlight_out);
		DPU_FB_ERR("[effect] dpu_runmode_is_factory bl:%d->%d\n", *backlight_out, backlight_in);
	}
	display_engine_bl_debug_print(backlight_in, *backlight_out, delta);
	return changed;
}


int dpufb_display_effect_blc_cabc_update(struct dpu_fb_data_type *dpufd)
{
	if (dpufd == NULL) {
		DPU_FB_ERR("[effect] dpufd is NULL\n");
		return -1;
	}

	if ((dpufd->panel_info.blpwm_input_ena || dpufb_display_effect_is_need_blc(dpufd)) &&
		dpufd->cabc_update)
		dpufd->cabc_update(dpufd);

	return 0;
}

void init_hiace(struct dpu_fb_data_type *dpufd)
{
	struct dpu_panel_info *pinfo = NULL;
	char __iomem *hiace_base = NULL;

	unsigned long dw_jiffies = 0;
	uint32_t tmp = 0;
	bool is_ready = false;

	uint32_t global_hist_ab_work;
	uint32_t global_hist_ab_shadow;
	uint32_t gamma_ab_work;
	uint32_t gamma_ab_shadow;
	uint32_t width;
	uint32_t height;
	uint32_t half_block_w;
	uint32_t half_block_h;
	uint32_t lhist_sft;
	uint32_t slop;
	uint32_t th_max;
	uint32_t th_min;
	uint32_t up_thres;
	uint32_t low_thres;
	uint32_t fixbit_x;
	uint32_t fixbit_y;
	uint32_t reciprocal_x;
	uint32_t reciprocal_y;

	uint32_t block_pixel_num;
	uint32_t max_lhist_block_pixel_num;
	uint32_t max_lhist_bin_reg_num;

	if (NULL == dpufd) {
		DPU_FB_ERR("[effect] dpufd is NULL\n");
		return;
	}

	pinfo = &(dpufd->panel_info);
	if (pinfo->hiace_support == 0) {
		effect_debug_log(DEBUG_EFFECT_ENTRY, "[effect] HIACE is not supported!\n");
		return;
	}

	if (dpufd->index == PRIMARY_PANEL_IDX) {
		hiace_base = dpufd->dss_base + DSS_HI_ACE_OFFSET;
	} else {
		DPU_FB_ERR("[effect] fb%d, not support!", dpufd->index);
		return;
	}

	if(!check_underflow_clear_ack(dpufd)){
		DPU_FB_ERR("[effect] fb%d, clear_ack is not return!\n", dpufd->index);
		return;
	}

	set_reg(hiace_base + HIACE_BYPASS_ACE, 0x1, 1, 0);
	set_reg(hiace_base + HIACE_INIT_GAMMA, 0x1, 1, 0);
	set_reg(hiace_base + HIACE_UPDATE_LOCAL, 0x1, 1, 0);

	//parameters
	width = dpufd->panel_info.xres & 0x1fff;
	height = dpufd->panel_info.yres & 0x1fff;
	set_reg(hiace_base + HIACE_IMAGE_INFO, (height << 16) | width, 32, 0);

	half_block_w = (width / 12) & 0x1ff;
	half_block_h = ((height + 11) / 12) & 0x1ff;
	set_reg(hiace_base + HIACE_HALF_BLOCK_H_W,
		(half_block_h << 16) | half_block_w, 32, 0);

	block_pixel_num = (half_block_w * half_block_h) << 2;
	max_lhist_block_pixel_num = block_pixel_num << 2;
	max_lhist_bin_reg_num = (1 << 16) - 1; // each local hist bin 20bit -> 16bit
	if (max_lhist_block_pixel_num < (max_lhist_bin_reg_num)) {
		lhist_sft = 0;
	} else if (max_lhist_block_pixel_num < (max_lhist_bin_reg_num << 1)) {
		lhist_sft = 1;
	} else if (max_lhist_block_pixel_num < (max_lhist_bin_reg_num << 2)) {
		lhist_sft = 2;
	} else if (max_lhist_block_pixel_num < (max_lhist_bin_reg_num << 3)) {
		lhist_sft = 3;
	} else {
		lhist_sft = 4;
	}
	set_reg(hiace_base + HIACE_LHIST_SFT, lhist_sft, 3, 0);
	pinfo->hiace_param.ilhist_sft = (int)lhist_sft;

	slop = 68 & 0xff;
	th_min = 0 & 0x1ff;
	th_max = 30 & 0x1ff;
	set_reg(hiace_base + HIACE_HUE, (slop << 24) | (th_max << 12) | th_min, 32, 0);

	slop = 68 & 0xff;
	th_min = 80 & 0xff;
	th_max = 140 & 0xff;
	set_reg(hiace_base + HIACE_SATURATION, (slop << 24) | (th_max << 12) | th_min, 32, 0);

	slop = 68 & 0xff;
	th_min = 100 & 0xff;
	th_max = 255 & 0xff;
	set_reg(hiace_base + HIACE_VALUE, (slop << 24) | (th_max << 12) | th_min, 32, 0);

	set_reg(hiace_base + HIACE_SKIN_GAIN, 128, 8, 0);

	up_thres = 248 & 0xff;
	low_thres = 8 & 0xff;
	set_reg(hiace_base + HIACE_UP_LOW_TH, (up_thres << 8) | low_thres, 32, 0);

	fixbit_x = get_fixed_point_offset(half_block_w) & 0x1f;
	fixbit_y = get_fixed_point_offset(half_block_h) & 0x1f;
	reciprocal_x = (1U << (fixbit_x + 8)) / (2 * MAX(half_block_w, 1)) & 0x3ff;
	reciprocal_y = (1U << (fixbit_y + 8)) / (2 * MAX(half_block_h, 1)) & 0x3ff;
	set_reg(hiace_base + HIACE_XYWEIGHT, (fixbit_y << 26) | (reciprocal_y << 16)
		| (fixbit_x << 10) | reciprocal_x, 32, 0);

	effect_debug_log(DEBUG_EFFECT_ENTRY, "[effect] half_block_w:%d, half_block_h:%d, fixbit_x:%d, fixbit_y:%d, reciprocal_x:%d, reciprocal_y:%d, lhist_sft:%d\n",
					 half_block_w, half_block_h, fixbit_x, fixbit_y, reciprocal_x, reciprocal_y, lhist_sft);

	//wait for gamma init finishing
	dw_jiffies = jiffies + HZ / 2;
	do {
		tmp = inp32(hiace_base + HIACE_INIT_GAMMA);
		if ((tmp & 0x1) != 0x1) {
			is_ready = true;
			break;
		}
	} while (time_after(dw_jiffies, jiffies)); //lint !e550

	if (!is_ready) {
		DPU_FB_INFO("[effect] fb%d, HIACE_INIT_GAMMA is not ready! HIACE_INIT_GAMMA=0x%08X.\n",
					 dpufd->index, tmp);
	}

	global_hist_ab_work = inp32(hiace_base + HIACE_GLOBAL_HIST_AB_WORK);
	global_hist_ab_shadow = !global_hist_ab_work;

	gamma_ab_work = inp32(hiace_base + HIACE_GAMMA_AB_WORK);
	gamma_ab_shadow = !gamma_ab_work;

	set_reg(hiace_base + HIACE_GLOBAL_HIST_AB_SHADOW, global_hist_ab_shadow, 1, 0);
	set_reg(hiace_base + HIACE_GAMMA_AB_SHADOW, gamma_ab_shadow, 1, 0);

#ifndef DISPLAY_EFFECT_USE_FRM_END_INT
	//unmask hiace interrupt
	set_reg(hiace_base + HIACE_INT_UNMASK, 0x1, 1, 0);
#endif
}

void dpu_dpp_hiace_set_reg(struct dpu_fb_data_type *dpufd)
{
	char __iomem *hiace_base = NULL;
	dss_ce_info_t *ce_info = NULL;

	if (NULL == dpufd) {
		DPU_FB_ERR("[effect] dpufd is NULL\n");
		return;
	}

	if (NULL == dpufd->dss_base) {
		DPU_FB_ERR("[effect] dss_base is NULL\n");
		return;
	}

	if (dpufd->panel_info.hiace_support == 0) {
		effect_debug_log(DEBUG_EFFECT_FRAME, "[effect] HIACE is not support!\n");
		return;
	}

	if (PRIMARY_PANEL_IDX == dpufd->index) {
		hiace_base = dpufd->dss_base + DSS_HI_ACE_OFFSET;
		ce_info = &(dpufd->hiace_info[dpufd->panel_info.disp_panel_id]);
	} else {
		DPU_FB_ERR("[effect] fb%d, not support!\n", dpufd->index);
		return;
	}

	if (!dpufb_display_effect_is_need_ace(dpufd)) {
		g_hiace_service.use_last_value = false;
		return;
	}

	if(!check_underflow_clear_ack(dpufd)){
		DPU_FB_ERR("[effect] fb%d, clear_ack is not return!\n", dpufd->index);
		return;
	}

	effect_debug_log(DEBUG_EFFECT_FRAME, "[effect] step in\n");

	//lint -e{438}
	if (g_hiace_service.use_last_value) {
		int gamma_ab_shadow = inp32(hiace_base + HIACE_GAMMA_AB_SHADOW);
		int gamma_ab_work = inp32(hiace_base + HIACE_GAMMA_AB_WORK);

		set_reg(hiace_base + HIACE_VALUE, (uint32_t)ce_info->thminv, 8, 0);

		if (gamma_ab_shadow == gamma_ab_work) {
			int i = 0;

			//write gamma lut
			set_reg(hiace_base + HIACE_GAMMA_EN, 1, 1, 31);

			mutex_lock(&ce_info->lut_lock);
			for (i = 0; i < (6 * 6 * 11); i++) {
				// cppcheck-suppress *
				outp32(hiace_base + HIACE_GAMMA_VxHy_3z2_3z1_3z_W, dpufd->hiace_info[dpufd->panel_info.disp_panel_id].lut_table[i]);
			}
			mutex_unlock(&ce_info->lut_lock);

			set_reg(hiace_base + HIACE_GAMMA_EN, 0, 1, 31);

			gamma_ab_shadow ^= 1;
			outp32(hiace_base + HIACE_GAMMA_AB_SHADOW, gamma_ab_shadow);
		}

		g_hiace_service.use_last_value = false;
	}
}

void dpu_dpp_hiace_end_handle_func(struct work_struct *work)
{
	struct dpu_fb_data_type *dpufd = NULL;
	char __iomem *hiace_base = NULL;
	uint32_t * global_hist_ptr = NULL;
	uint32_t * local_hist_ptr = NULL;
	dss_ce_info_t *ce_info = NULL;

	int i = 0;
	int global_hist_ab_shadow = 0;
	int global_hist_ab_work = 0;
	int local_valid = 0;

	time_interval_t interval_total = {0};
	time_interval_t interval_hist_global = {0};
	time_interval_t interval_hist_local = {0};
	define_delay_record(delay_total, "interrupt handling");
	define_delay_record(delay_hist_global, "global hist reading");
	define_delay_record(delay_hist_local, "local hist reading");

	dpufd = container_of(work, struct dpu_fb_data_type, hiace_end_work);
	if (NULL == dpufd) {
		DPU_FB_ERR("[effect] dpufd is NULL\n");
		return;
	}

	if (dpufd->panel_info.hiace_support == 0) {
		effect_debug_log(DEBUG_EFFECT_FRAME, "[effect] HIACE is not support!\n");
		return;
	}

	if (PRIMARY_PANEL_IDX == dpufd->index) {
		ce_info = &(dpufd->hiace_info[dpufd->panel_info.disp_panel_id]);
		hiace_base = dpufd->dss_base + DSS_HI_ACE_OFFSET;
	} else {
		DPU_FB_ERR("[effect] fb%d, not support!\n", dpufd->index);
		return;
	}

	if (!dpufb_display_effect_is_need_ace(dpufd)) {
		enable_hiace(dpufd, false);
		return;
	}

	down(&dpufd->blank_sem);
	if (!dpufd->panel_power_on) {
		effect_debug_log(DEBUG_EFFECT_ENTRY, "[effect] panel power off!\n");
		goto ERR_OUT;
	}

	if(!check_underflow_clear_ack(dpufd)){
		DPU_FB_ERR("[effect] fb%d, clear_ack is not return!\n", dpufd->index);
		goto ERR_OUT;
	}

	if (g_hiace_service.is_ready) {
		g_hiace_service.is_ready = false;
	} else {
		effect_debug_log(DEBUG_EFFECT_ENTRY, "[effect] service is not ready!\n");
		goto ERR_OUT;
	}

	effect_debug_log(DEBUG_EFFECT_FRAME, "[effect] step in\n");

	if (g_debug_effect & DEBUG_EFFECT_DELAY) {
		interval_total.start = get_timestamp_in_us();
	}

	dpufb_activate_vsync(dpufd);

	down(&dpufd->hiace_clear_sem);

	mutex_lock(&ce_info->hist_lock);

	local_valid = inp32(hiace_base + HIACE_LOCAL_VALID);
	if (local_valid == 1) {
		//read local hist
		local_hist_ptr = &dpufd->hiace_info[dpufd->panel_info.disp_panel_id].histogram[HIACE_GHIST_RANK];
		outp32(hiace_base + HIACE_LHIST_EN, (1 << 31));

		if (g_debug_effect & DEBUG_EFFECT_DELAY) {
			interval_hist_local.start = get_timestamp_in_us();
		}
		for (i = 0; i < (6 * 6 * 16); i++) {//H  L
			local_hist_ptr[i] = inp32(hiace_base + HIACE_LOCAL_HIST_VxHy_2z_2z1);
		}
		if (g_debug_effect & DEBUG_EFFECT_DELAY) {
			interval_hist_local.stop = get_timestamp_in_us();
			count_delay(&delay_hist_local, interval_hist_local.stop - interval_hist_local.start);
		}

		outp32(hiace_base + HIACE_LHIST_EN, (0 << 31));
		outp32(hiace_base + HIACE_UPDATE_LOCAL, 1);
	}

	global_hist_ab_shadow = inp32(hiace_base + HIACE_GLOBAL_HIST_AB_SHADOW);
	global_hist_ab_work = inp32(hiace_base + HIACE_GLOBAL_HIST_AB_WORK);
	if (global_hist_ab_shadow == global_hist_ab_work) {
		//read global hist
		global_hist_ptr = &dpufd->hiace_info[dpufd->panel_info.disp_panel_id].histogram[0];//HIACE_GHIST_RANK

		if (g_debug_effect & DEBUG_EFFECT_DELAY) {
			interval_hist_global.start = get_timestamp_in_us();
		}
		for (i = 0; i < 32; i++) {
			global_hist_ptr[i] = inp32(hiace_base + HIACE_GLOBAL_HIST_LUT_ADDR + i * 4);
		}
		if (g_debug_effect & DEBUG_EFFECT_DELAY) {
			interval_hist_global.stop = get_timestamp_in_us();
			count_delay(&delay_hist_global, interval_hist_global.stop - interval_hist_global.start);
		}

		outp32(hiace_base +  HIACE_GLOBAL_HIST_AB_SHADOW, global_hist_ab_shadow ^ 1);
	}

	mutex_unlock(&ce_info->hist_lock);

	up(&dpufd->hiace_clear_sem);

	dpufb_deactivate_vsync(dpufd);

	up(&dpufd->blank_sem);

	if ((local_valid == 1) || (global_hist_ab_shadow == global_hist_ab_work)) { // global or local hist is updated
		ce_info->algorithm_result = dpufb_ce_do_contrast(dpufd);
	}
	g_hiace_service.is_ready = true;

	down(&dpufd->blank_sem);
	if (!dpufd->panel_power_on) {
		effect_debug_log(DEBUG_EFFECT_ENTRY, "[effect] panel power off!\n");
		goto ERR_OUT;
	}

	dpufb_activate_vsync(dpufd);

	down(&dpufd->hiace_clear_sem);

	//lint -e{438}
	if (ce_info->algorithm_result == 0) {
		int gamma_ab_shadow = inp32(hiace_base + HIACE_GAMMA_AB_SHADOW);
		int gamma_ab_work = inp32(hiace_base + HIACE_GAMMA_AB_WORK);
		time_interval_t interval_lut = {0};
		define_delay_record(delay_lut, "lut writing");

		set_reg(hiace_base + HIACE_VALUE, (uint32_t)ce_info->thminv, 12, 0);

		if (gamma_ab_shadow == gamma_ab_work) {
			//write gamma lut
			set_reg(hiace_base + HIACE_GAMMA_EN, 1, 1, 31);

			if (g_debug_effect & DEBUG_EFFECT_DELAY) {
				interval_lut.start = get_timestamp_in_us();
			}
			mutex_lock(&ce_info->lut_lock);
			for (i = 0; i < (6 * 6 * 11); i++) {
				// cppcheck-suppress *
				outp32(hiace_base + HIACE_GAMMA_VxHy_3z2_3z1_3z_W, dpufd->hiace_info[dpufd->panel_info.disp_panel_id].lut_table[i]);
			}
			mutex_unlock(&ce_info->lut_lock);
			if (g_debug_effect & DEBUG_EFFECT_DELAY) {
				interval_lut.stop = get_timestamp_in_us();
				count_delay(&delay_lut, interval_lut.stop - interval_lut.start);
			}

			set_reg(hiace_base + HIACE_GAMMA_EN, 0, 1, 31);

			gamma_ab_shadow ^= 1;
			outp32(hiace_base + HIACE_GAMMA_AB_SHADOW, gamma_ab_shadow);
		}

		ce_info->algorithm_result = 1;
	}

#ifndef DISPLAY_EFFECT_USE_FRM_END_INT
	//clear INT
	outp32(hiace_base + HIACE_INT_STAT, 0x1);
#endif

	up(&dpufd->hiace_clear_sem);

	dpufb_deactivate_vsync(dpufd);

	if (ce_info->gradual_frames > 0) {
		ce_info->gradual_frames--;
	}

	if (g_debug_effect & DEBUG_EFFECT_DELAY) {
		interval_total.stop = get_timestamp_in_us();
		count_delay(&delay_total, interval_total.stop - interval_total.start);
	}

ERR_OUT:
	up(&dpufd->blank_sem);
} //lint !e550
//lint +e845, +e732, +e774

/*******************************************************************************
** GM IGM
*/
#define GM_LUT_LEN 257
#define GM_LUT_MHLEN 254
static uint16_t degm_gm_lut[GM_LUT_LEN *6];

int dpufb_use_dynamic_gamma(struct dpu_fb_data_type *dpufd, char __iomem *dpp_base)
{
	uint32_t i = 0;
	uint32_t index = 0;
	struct dpu_panel_info *pinfo = NULL;
	char __iomem *gamma_pre_lut_base = NULL;//lint !e838

	if (dpufd == NULL) {
		return -1;
	}

	if (dpp_base == NULL) {
		return -1;
	}

	pinfo = &(dpufd->panel_info);

	if(dpufd->dynamic_gamma_info.use_dynamic_gm_init == 1) {
		uint16_t* gm_lut_r =  degm_gm_lut;
		uint16_t* gm_lut_g =  gm_lut_r + GM_LUT_LEN;
		uint16_t* gm_lut_b =  gm_lut_g + GM_LUT_LEN;

		for (i = 0; i < pinfo->gamma_lut_table_len / 2; i++) {
			index = i << 1;
			if (index >= GM_LUT_MHLEN)
			{
				index = GM_LUT_MHLEN;
			}
			outp32(dpp_base + (U_GAMA_R_COEF + i * 4), gm_lut_r[index] | gm_lut_r[index+1] << 16);
			outp32(dpp_base + (U_GAMA_G_COEF + i * 4), gm_lut_g[index] | gm_lut_g[index+1] << 16);
			outp32(dpp_base + (U_GAMA_B_COEF + i * 4), gm_lut_b[index] | gm_lut_b[index+1] << 16);

			if (g_dss_version_tag == FB_ACCEL_DPUV410) {
				gamma_pre_lut_base = dpufd->dss_base + DSS_DPP_GAMA_PRE_LUT_OFFSET;
				//GAMA PRE LUT
				outp32(gamma_pre_lut_base + (U_GAMA_PRE_R_COEF + i * 4), gm_lut_r[index] | gm_lut_r[index+1] << 16);
				outp32(gamma_pre_lut_base + (U_GAMA_PRE_G_COEF + i * 4), gm_lut_g[index] | gm_lut_g[index+1] << 16);
				outp32(gamma_pre_lut_base + (U_GAMA_PRE_B_COEF + i * 4), gm_lut_b[index] | gm_lut_b[index+1] << 16);
			}
		}
		outp32(dpp_base + U_GAMA_R_LAST_COEF, gm_lut_r[pinfo->gamma_lut_table_len - 1]);
		outp32(dpp_base + U_GAMA_G_LAST_COEF, gm_lut_g[pinfo->gamma_lut_table_len - 1]);
		outp32(dpp_base + U_GAMA_B_LAST_COEF, gm_lut_b[pinfo->gamma_lut_table_len - 1]);

		if (g_dss_version_tag == FB_ACCEL_DPUV410) {
			//GAMA PRE LUT
			outp32(gamma_pre_lut_base + U_GAMA_PRE_R_LAST_COEF, gm_lut_r[pinfo->gamma_lut_table_len - 1]);
			outp32(gamma_pre_lut_base + U_GAMA_PRE_G_LAST_COEF, gm_lut_g[pinfo->gamma_lut_table_len - 1]);
			outp32(gamma_pre_lut_base + U_GAMA_PRE_B_LAST_COEF, gm_lut_b[pinfo->gamma_lut_table_len - 1]);
		}
		return 1;//lint !e438
	}

	return 0;//lint !e438

}//lint !e550

int dpufb_use_dynamic_degamma(struct dpu_fb_data_type *dpufd, char __iomem *dpp_base)
{
	uint32_t i = 0;
	uint32_t index = 0;
	struct dpu_panel_info *pinfo = NULL;

	if (dpufd == NULL) {
		return -1;
	}

	if (dpp_base == NULL) {
		return -1;
	}

	pinfo = &(dpufd->panel_info);

	if(dpufd->dynamic_gamma_info.use_dynamic_gm_init == 1) {

		uint16_t* degm_lut_r = degm_gm_lut + GM_LUT_LEN * 3;
		uint16_t* degm_lut_g = degm_lut_r + GM_LUT_LEN;
		uint16_t* degm_lut_b = degm_lut_g + GM_LUT_LEN;

		for (i = 0; i < pinfo->igm_lut_table_len / 2; i++) {
			index = i << 1;
			if(index >= GM_LUT_MHLEN)
			{
				index = GM_LUT_MHLEN;
			}
			outp32(dpp_base + (U_DEGAMA_R_COEF +  i * 4), degm_lut_r[index] | degm_lut_r[index+1] << 16);
			outp32(dpp_base + (U_DEGAMA_G_COEF +  i * 4), degm_lut_g[index] | degm_lut_g[index+1] << 16);
			outp32(dpp_base + (U_DEGAMA_B_COEF +  i * 4), degm_lut_b[index] | degm_lut_b[index+1] << 16);
		}
		outp32(dpp_base + U_DEGAMA_R_LAST_COEF, degm_lut_r[pinfo->igm_lut_table_len - 1]);
		outp32(dpp_base + U_DEGAMA_G_LAST_COEF, degm_lut_g[pinfo->igm_lut_table_len - 1]);
		outp32(dpp_base + U_DEGAMA_B_LAST_COEF, degm_lut_b[pinfo->igm_lut_table_len - 1]);

		return 1;
	}

	return 0;

}

void dpufb_update_dynamic_gamma(struct dpu_fb_data_type *dpufd, const char* buffer, size_t len)
{
	struct dpu_panel_info *pinfo = NULL;
	if (dpufd == NULL || buffer == NULL) {
		return;
	}

	pinfo = &(dpufd->panel_info);
	if (pinfo == NULL) { //lint !e774
		return;
	}

	if (!DPU_SUPPORT_DPP_MODULE_BIT(DPP_MODULE_GAMA)){//lint !e774
		return;
	}

	dpufd->dynamic_gamma_info.use_dynamic_gm = 0;
	dpufd->dynamic_gamma_info.use_dynamic_gm_init = 0;

	if (pinfo->gamma_support == 1) {
		if ((len > 0) && (len <= (int)sizeof(degm_gm_lut))) {
			memcpy((char*)degm_gm_lut, buffer, len);
			dpufd->dynamic_gamma_info.use_dynamic_gm = 1;
			dpufd->dynamic_gamma_info.use_dynamic_gm_init = 1;
		}
	}

}

void dpufb_update_gm_from_reserved_mem(uint32_t *gm_r, uint32_t *gm_g, uint32_t *gm_b,
	uint32_t *igm_r, uint32_t *igm_g, uint32_t *igm_b)
{
	int i = 0;
	int len = 0;
	uint16_t *u16_gm_r = NULL;
	uint16_t *u16_gm_g = NULL;
	uint16_t *u16_gm_b = NULL;
	uint16_t *u16_igm_r = NULL;
	uint16_t *u16_igm_g = NULL;
	uint16_t *u16_igm_b = NULL;
	void *mem = NULL;
	unsigned long gm_addr = 0;
	unsigned long gm_size = 0;

	g_factory_gamma_enable = 0;

	if (gm_r == NULL || gm_g == NULL || gm_b == NULL
		|| igm_r == NULL || igm_g == NULL || igm_b == NULL) {
		return;
	}

	gm_addr = SUB_RESERVED_LCD_GAMMA_MEM_PHYMEM_BASE;
	gm_size = SUB_RESERVED_LCD_GAMMA_MEM_PHYMEM_SIZE;

	DPU_FB_INFO("gamma kernel gm_addr = 0x%lx  gm_size = 0x%lx \n", gm_addr, gm_size);

	mem = (void *)ioremap_wc(gm_addr, gm_size);
	if (mem == NULL) {
		DPU_FB_ERR("mem ioremap error ! \n");
		return;
	}
	memcpy(&len, mem, 4UL);
	DPU_FB_INFO("gamma read len = %d \n", len);
	if (len != GM_IGM_LEN) {
		DPU_FB_ERR("gamma read len error ! \n");
		iounmap(mem);
		return;
	}

	u16_gm_r = (uint16_t *)(mem + 4);
	u16_gm_g = u16_gm_r + GM_LUT_LEN;
	u16_gm_b = u16_gm_g + GM_LUT_LEN;

	u16_igm_r = u16_gm_b + GM_LUT_LEN;
	u16_igm_g = u16_igm_r + GM_LUT_LEN;
	u16_igm_b = u16_igm_g + GM_LUT_LEN;

	for (i = 0; i < GM_LUT_LEN; i++) {
		gm_r[i] = u16_gm_r[i];
		gm_g[i] = u16_gm_g[i];
		gm_b[i] = u16_gm_b[i];

		igm_r[i]  = u16_igm_r[i];
		igm_g[i] = u16_igm_g[i];
		igm_b[i] = u16_igm_b[i];
	}
	iounmap(mem);

	g_factory_gamma_enable = 1;
	return;
}

/*lint -e571, -e573, -e737, -e732, -e850, -e730, -e713, -e574, -e679, -e732, -e845, -e570, -e774*/

#define ACM_HUE_LUT_LENGTH ((uint32_t)256)
#define ACM_SATA_LUT_LENGTH ((uint32_t)256)
#define ACM_SATR_LUT_LENGTH ((uint32_t)64)

#define LCP_GMP_LUT_LENGTH	((uint32_t)9*9*9)
#define LCP_XCC_LUT_LENGTH	((uint32_t)12)

#define IGM_LUT_LEN ((uint32_t)257)
#define GAMMA_LUT_LEN ((uint32_t)257)

#define BYTES_PER_TABLE_ELEMENT 4

static int dpu_effect_copy_to_user(uint32_t *table_dst, uint32_t *table_src, uint32_t table_length)
{
	unsigned long table_size = 0;

	if ((NULL == table_dst) || (NULL == table_src) || (table_length == 0)) {
		DPU_FB_ERR("invalid input parameters.\n");
		return -EINVAL;
	}

	table_size = (unsigned long)table_length * BYTES_PER_TABLE_ELEMENT;

	if (copy_to_user(table_dst, table_src, table_size)) {
		DPU_FB_ERR("failed to copy table to user.\n");
		return -EINVAL;
	}

	return 0;
}

static int dpu_effect_alloc_and_copy(uint32_t **table_dst, uint32_t *table_src,
	uint32_t lut_table_length, bool copy_user)
{
	uint32_t *table_new = NULL;
	unsigned long table_size = 0;

	if ((NULL == table_dst) ||(NULL == table_src) ||  (lut_table_length == 0)) {
		DPU_FB_ERR("invalid input parameter");
		return -EINVAL;
	}

	table_size = (unsigned long)lut_table_length * BYTES_PER_TABLE_ELEMENT;

	if (*table_dst == NULL) {
		table_new = (uint32_t *)kmalloc(table_size, GFP_ATOMIC);
		if (table_new) {
			memset(table_new, 0, table_size);
			*table_dst = table_new;
		} else {
			DPU_FB_ERR("failed to kmalloc lut_table!\n");
			return -EINVAL;
		}
	}

	if (copy_user) {
		if (copy_from_user(*table_dst, table_src, table_size)) {
			DPU_FB_ERR("failed to copy table from user\n");
			if (table_new)
				kfree(table_new);
			*table_dst = NULL;
			return -EINVAL;
		}
	} else {
		memcpy(*table_dst, table_src, table_size);
	}

	return 0;
}

static void dpu_effect_kfree(uint32_t **free_table)
{
	if (*free_table) {
		kfree((uint32_t *) *free_table);
		*free_table = NULL;
	}
}

static void free_acm_table(struct acm_info *acm)
{
	if(acm == NULL){
		DPU_FB_ERR("acm is NULL!\n");
		return;
	}

	dpu_effect_kfree(&acm->hue_table);
	dpu_effect_kfree(&acm->sata_table);
	dpu_effect_kfree(&acm->satr0_table);
	dpu_effect_kfree(&acm->satr1_table);
	dpu_effect_kfree(&acm->satr2_table);
	dpu_effect_kfree(&acm->satr3_table);
	dpu_effect_kfree(&acm->satr4_table);
	dpu_effect_kfree(&acm->satr5_table);
	dpu_effect_kfree(&acm->satr6_table);
	dpu_effect_kfree(&acm->satr7_table);
}

static void free_gamma_table(struct gamma_info *gamma)
{
	if(gamma == NULL){
		DPU_FB_ERR("gamma is NULL!\n");
		return;
	}

	dpu_effect_kfree(&gamma->gamma_r_table);
	dpu_effect_kfree(&gamma->gamma_g_table);
	dpu_effect_kfree(&gamma->gamma_b_table);
}

int dpu_effect_arsr2p_info_get(struct dpu_fb_data_type *dpufd, struct arsr2p_info *arsr2p)
{
	if (NULL == dpufd) {
		DPU_FB_ERR("dpufd is NULL!\n");
		return -EINVAL;
	}

	if (NULL == arsr2p) {
		DPU_FB_ERR("fb%d, arsr2p is NULL!\n", dpufd->index);
		return -EINVAL;
	}

	if (!dpufd->effect_ctl.arsr2p_sharp_support) {
		DPU_FB_INFO("fb%d, arsr2p is not supported!\n", dpufd->index);
		return 0;
	}

	memcpy(&arsr2p[0], &(dpufd->dss_module_default.arsr2p[DSS_RCHN_V1].arsr2p_effect), sizeof(struct arsr2p_info));
	memcpy(&arsr2p[1], &(dpufd->dss_module_default.arsr2p[DSS_RCHN_V1].arsr2p_effect_scale_up), sizeof(struct arsr2p_info));
	memcpy(&arsr2p[2], &(dpufd->dss_module_default.arsr2p[DSS_RCHN_V1].arsr2p_effect_scale_down), sizeof(struct arsr2p_info));
	arsr2p[0].sharp_enable = dpufd->panel_info.prefix_sharpness2D_support;
	arsr2p[1].sharp_enable = dpufd->panel_info.prefix_sharpness2D_support;
	arsr2p[2].sharp_enable = dpufd->panel_info.prefix_sharpness2D_support;

	return 0;
}

int dpu_effect_arsr1p_info_get(struct dpu_fb_data_type *dpufd, struct arsr1p_info *arsr1p)
{
    (void)dpufd, (void)arsr1p;
    return 0;

}

int dpu_effect_acm_info_get(struct dpu_fb_data_type *dpufd, struct acm_info *acm_dst)
{
	struct dpu_panel_info *pinfo = NULL;

	if (NULL == dpufd) {
		DPU_FB_ERR("dpufd is NULL!\n");
		return -EINVAL;
	}

	if (NULL == acm_dst) {
		DPU_FB_ERR("fb%d, acm is NULL!\n", dpufd->index);
		return -EINVAL;
	}

	if (!dpufd->effect_ctl.acm_support) {
		DPU_FB_INFO("fb%d, acm is not supported!\n", dpufd->index);
		return 0;
	}

	pinfo = &dpufd->panel_info;

	acm_dst->acm_hue_rlh01 = (pinfo->r1_lh<<16) | pinfo->r0_lh;
	acm_dst->acm_hue_rlh23 = (pinfo->r3_lh<<16) | pinfo->r2_lh;
	acm_dst->acm_hue_rlh45 = (pinfo->r5_lh<<16) | pinfo->r4_lh;
	acm_dst->acm_hue_rlh67 = (pinfo->r6_hh<<16) | pinfo->r6_lh;
	acm_dst->acm_hue_param01 = pinfo->hue_param01;
	acm_dst->acm_hue_param23 = pinfo->hue_param23;
	acm_dst->acm_hue_param45 = pinfo->hue_param45;
	acm_dst->acm_hue_param67 = pinfo->hue_param67;
	acm_dst->acm_hue_smooth0 = pinfo->hue_smooth0;
	acm_dst->acm_hue_smooth1 = pinfo->hue_smooth1;
	acm_dst->acm_hue_smooth2 = pinfo->hue_smooth2;
	acm_dst->acm_hue_smooth3 = pinfo->hue_smooth3;
	acm_dst->acm_hue_smooth4 = pinfo->hue_smooth4;
	acm_dst->acm_hue_smooth5 = pinfo->hue_smooth5;
	acm_dst->acm_hue_smooth6 = pinfo->hue_smooth6;
	acm_dst->acm_hue_smooth7 = pinfo->hue_smooth7;
	acm_dst->acm_color_choose = pinfo->color_choose;
	acm_dst->acm_l_cont_en = pinfo->l_cont_en;
	acm_dst->acm_lc_param01  = pinfo->lc_param01;
	acm_dst->acm_lc_param23  = pinfo->lc_param23;
	acm_dst->acm_lc_param45  = pinfo->lc_param45;
	acm_dst->acm_lc_param67  = pinfo->lc_param67;
	acm_dst->acm_l_adj_ctrl = pinfo->l_adj_ctrl;
	acm_dst->acm_capture_ctrl = pinfo->capture_ctrl;
	acm_dst->acm_capture_in = pinfo->capture_in;
	acm_dst->acm_capture_out = pinfo->capture_out;
	acm_dst->acm_ink_ctrl = pinfo->ink_ctrl;
	acm_dst->acm_ink_out = pinfo->ink_out;
	acm_dst->acm_en = pinfo->acm_ce_support;

	if (dpu_effect_copy_to_user(acm_dst->hue_table, pinfo->acm_lut_hue_table, ACM_HUE_LUT_LENGTH)) {
		DPU_FB_ERR("fb%d, failed to copy acm hue table to user!\n", dpufd->index);
		return -EINVAL;
	}

	if (dpu_effect_copy_to_user(acm_dst->sata_table, pinfo->acm_lut_sata_table, ACM_SATA_LUT_LENGTH)) {
		DPU_FB_ERR("fb%d, failed to copy acm sata table to user!\n", dpufd->index);
		return -EINVAL;
	}

	if (dpu_effect_copy_to_user(acm_dst->satr0_table, pinfo->acm_lut_satr0_table, ACM_SATR_LUT_LENGTH)) {
		DPU_FB_ERR("fb%d, failed to copy acm satr0 table to user!\n", dpufd->index);
		return -EINVAL;
	}

	if (dpu_effect_copy_to_user(acm_dst->satr1_table, pinfo->acm_lut_satr1_table, ACM_SATR_LUT_LENGTH)) {
		DPU_FB_ERR("fb%d, failed to copy acm satr1 table to user!\n", dpufd->index);
		return -EINVAL;
	}

	if (dpu_effect_copy_to_user(acm_dst->satr2_table, pinfo->acm_lut_satr2_table, ACM_SATR_LUT_LENGTH)) {
		DPU_FB_ERR("fb%d, failed to copy acm satr2 table to user!\n", dpufd->index);
		return -EINVAL;
	}

	if (dpu_effect_copy_to_user(acm_dst->satr3_table, pinfo->acm_lut_satr3_table, ACM_SATR_LUT_LENGTH)) {
		DPU_FB_ERR("fb%d, failed to copy acm satr3 table to user!\n", dpufd->index);
		return -EINVAL;
	}

	if (dpu_effect_copy_to_user(acm_dst->satr4_table, pinfo->acm_lut_satr4_table, ACM_SATR_LUT_LENGTH)) {
		DPU_FB_ERR("fb%d, failed to copy acm satr4 table to user!\n", dpufd->index);
		return -EINVAL;
	}

	if (dpu_effect_copy_to_user(acm_dst->satr5_table, pinfo->acm_lut_satr5_table, ACM_SATR_LUT_LENGTH)) {
		DPU_FB_ERR("fb%d, failed to copy acm satr5 table to user!\n", dpufd->index);
		return -EINVAL;
	}

	if (dpu_effect_copy_to_user(acm_dst->satr6_table, pinfo->acm_lut_satr6_table, ACM_SATR_LUT_LENGTH)) {
		DPU_FB_ERR("fb%d, failed to copy acm satr6 table to user!\n", dpufd->index);
		return -EINVAL;
	}

	if (dpu_effect_copy_to_user(acm_dst->satr7_table, pinfo->acm_lut_satr7_table, ACM_SATR_LUT_LENGTH)) {
		DPU_FB_ERR("fb%d, failed to copy acm satr7 table to user!\n", dpufd->index);
		return -EINVAL;
	}

	return 0;
}

int dpu_effect_lcp_info_get(struct dpu_fb_data_type *dpufd, struct lcp_info *lcp)
{
	int ret = 0;
	struct dpu_panel_info *pinfo = NULL;

	if (NULL == dpufd) {
		DPU_FB_ERR("dpufd is NULL!\n");
		return -EINVAL;
	}

	if (NULL == lcp) {
		DPU_FB_ERR("fb%d, lcp is NULL!\n", dpufd->index);
		return -EINVAL;
	}

	pinfo = &(dpufd->panel_info);

	if (dpufd->effect_ctl.lcp_gmp_support && (pinfo->gmp_lut_table_len == LCP_GMP_LUT_LENGTH)) {
		ret = dpu_effect_copy_to_user(lcp->gmp_table_low32, pinfo->gmp_lut_table_low32bit, LCP_GMP_LUT_LENGTH);
		if (ret) {
			DPU_FB_ERR("fb%d, failed to copy gmp_table_low32 to user!\n", dpufd->index);
			goto err_ret;
		}

		ret = dpu_effect_copy_to_user(lcp->gmp_table_high4, pinfo->gmp_lut_table_high4bit, LCP_GMP_LUT_LENGTH);
		if (ret) {
			DPU_FB_ERR("fb%d, failed to copy gmp_table_high4 to user!\n", dpufd->index);
			goto err_ret;
		}
	}

	if (dpufd->effect_ctl.lcp_xcc_support && (pinfo->xcc_table_len == LCP_XCC_LUT_LENGTH)) {
		ret = dpu_effect_copy_to_user(lcp->xcc_table, pinfo->xcc_table, LCP_XCC_LUT_LENGTH);
		if (ret) {
			DPU_FB_ERR("fb%d, failed to copy xcc_table to user!\n", dpufd->index);
			goto err_ret;
		}
	}

	if (dpufd->effect_ctl.lcp_igm_support && (pinfo->igm_lut_table_len == IGM_LUT_LEN)) {
		ret = dpu_effect_copy_to_user(lcp->igm_r_table, pinfo->igm_lut_table_R, IGM_LUT_LEN);
		if (ret) {
			DPU_FB_ERR("fb%d, failed to copy igm_r_table to user!\n", dpufd->index);
			goto err_ret;
		}

		ret = dpu_effect_copy_to_user(lcp->igm_g_table, pinfo->igm_lut_table_G, IGM_LUT_LEN);
		if (ret) {
			DPU_FB_ERR("fb%d, failed to copy igm_g_table to user!\n", dpufd->index);
			goto err_ret;
		}

		ret = dpu_effect_copy_to_user(lcp->igm_b_table, pinfo->igm_lut_table_B, IGM_LUT_LEN);
		if (ret) {
			DPU_FB_ERR("fb%d, failed to copy igm_b_table to user!\n", dpufd->index);
			goto err_ret;
		}
	}

err_ret:
	return ret;
}

int dpu_effect_gamma_info_get(struct dpu_fb_data_type *dpufd, struct gamma_info *gamma)
{
	struct dpu_panel_info *pinfo = NULL;
	int ret = 0;

	if (NULL == dpufd) {
		DPU_FB_ERR("dpufd is NULL!\n");
		return -EINVAL;
	}

	if (NULL == gamma) {
		DPU_FB_ERR("fb%d, gamma is NULL!\n", dpufd->index);
		return -EINVAL;
	}

	if (!dpufd->effect_ctl.gamma_support) {
		DPU_FB_INFO("fb%d, gamma is not supported!\n", dpufd->index);
		return 0;
	}

	pinfo = &(dpufd->panel_info);

	if (dpufd->effect_ctl.lcp_gmp_support && (pinfo->gamma_lut_table_len== GAMMA_LUT_LEN)) {
		gamma->para_mode = 0;

		ret = dpu_effect_copy_to_user(gamma->gamma_r_table, pinfo->gamma_lut_table_R, GAMMA_LUT_LEN);
		if (ret) {
			DPU_FB_ERR("fb%d, failed to copy gamma_r_table to user!\n", dpufd->index);
			goto err_ret;
		}

		ret = dpu_effect_copy_to_user(gamma->gamma_g_table, pinfo->gamma_lut_table_G, GAMMA_LUT_LEN);
		if (ret) {
			DPU_FB_ERR("fb%d, failed to copy gamma_g_table to user!\n", dpufd->index);
			goto err_ret;
		}

		ret = dpu_effect_copy_to_user(gamma->gamma_b_table, pinfo->gamma_lut_table_B, GAMMA_LUT_LEN);
		if (ret) {
			DPU_FB_ERR("fb%d, failed to copy gamma_b_table to user!\n", dpufd->index);
			goto err_ret;
		}

	}

err_ret:
	return ret;
}
#define ARSR2P_MAX_NUM 3

int dpu_effect_save_arsr2p_info(struct dpu_fb_data_type *dpufd, struct dss_effect_info *effect_info_src)
{
	uint32_t i;
	if (dpufd == NULL) {
		DPU_FB_ERR("dpufd is NULL!\n");
		return -EINVAL;
	}

	if (effect_info_src == NULL) {
		DPU_FB_ERR("fb%d, effect_info_src is NULL!\n", dpufd->index);
		return -EINVAL;
	}

	if (!dpufd->effect_ctl.arsr2p_sharp_support) {
		DPU_FB_INFO("fb%d, arsr2p sharp is not supported!\n", dpufd->index);
		return 0;
	}

	for (i = 0; i < ARSR2P_MAX_NUM; i++) {
		if (effect_info_src->arsr2p[i].update == 1)
			memcpy(&(dpufd->effect_info[effect_info_src->disp_panel_id].arsr2p[i]), &(effect_info_src->arsr2p[i]), sizeof(struct arsr2p_info));
	}

	dpufd->effect_updated_flag[effect_info_src->disp_panel_id].arsr2p_effect_updated = true;

	for (i = 0; i < ARSR2P_MAX_NUM; i++) {
		if (dpufd->effect_info[effect_info_src->disp_panel_id].arsr2p[i].update)
			DPU_FB_INFO("mode %d: enable : %u, sharp_enable:%u, shoot_enable:%u, skin_enable:%u, update: %u\n",
				i, dpufd->effect_info[effect_info_src->disp_panel_id].arsr2p[i].enable,
				dpufd->effect_info[effect_info_src->disp_panel_id].arsr2p[i].sharp_enable,
				dpufd->effect_info[effect_info_src->disp_panel_id].arsr2p[i].shoot_enable,
				dpufd->effect_info[effect_info_src->disp_panel_id].arsr2p[i].skin_enable,
				dpufd->effect_info[effect_info_src->disp_panel_id].arsr2p[i].update);
	}

	return 0;
}

int dpu_effect_save_arsr1p_info(struct dpu_fb_data_type *dpufd, struct dss_effect_info *effect_info_src)
{
	(void)dpufd, (void)effect_info_src;
	return 0;
}

static int set_acm_normal_param(struct acm_info *acm_dst, struct dpu_panel_info *pinfo)
{
	if (acm_dst == NULL) {
		DPU_FB_DEBUG("acm_dst is NULL!\n");
		return -1;
	}

	if (pinfo == NULL) {
		DPU_FB_DEBUG("pinfo is NULL!\n");
		return -1;
	}

	acm_dst->acm_hue_rlh01 = (pinfo->r1_lh<<16) | pinfo->r0_lh;
	acm_dst->acm_hue_rlh23 = (pinfo->r3_lh<<16) | pinfo->r2_lh;
	acm_dst->acm_hue_rlh45 = (pinfo->r5_lh<<16) | pinfo->r4_lh;
	acm_dst->acm_hue_rlh67 = (pinfo->r6_hh<<16) | pinfo->r6_lh;
	acm_dst->acm_hue_param01 = pinfo->hue_param01;
	acm_dst->acm_hue_param23 = pinfo->hue_param23;
	acm_dst->acm_hue_param45 = pinfo->hue_param45;
	acm_dst->acm_hue_param67 = pinfo->hue_param67;
	acm_dst->acm_hue_smooth0 = pinfo->hue_smooth0;
	acm_dst->acm_hue_smooth1 = pinfo->hue_smooth1;
	acm_dst->acm_hue_smooth2 = pinfo->hue_smooth2;
	acm_dst->acm_hue_smooth3 = pinfo->hue_smooth3;
	acm_dst->acm_hue_smooth4 = pinfo->hue_smooth4;
	acm_dst->acm_hue_smooth5 = pinfo->hue_smooth5;
	acm_dst->acm_hue_smooth6 = pinfo->hue_smooth6;
	acm_dst->acm_hue_smooth7 = pinfo->hue_smooth7;
	acm_dst->acm_color_choose = pinfo->color_choose;
	acm_dst->acm_l_cont_en = pinfo->l_cont_en;
	acm_dst->acm_lc_param01  = pinfo->lc_param01;
	acm_dst->acm_lc_param23  = pinfo->lc_param23;
	acm_dst->acm_lc_param45  = pinfo->lc_param45;
	acm_dst->acm_lc_param67  = pinfo->lc_param67;
	acm_dst->acm_l_adj_ctrl = pinfo->l_adj_ctrl;
	acm_dst->acm_capture_ctrl = pinfo->capture_ctrl;
	acm_dst->acm_ink_ctrl = pinfo->ink_ctrl;
	acm_dst->acm_ink_out = pinfo->ink_out;

	/* malloc acm_dst lut table memory*/
	if (dpu_effect_alloc_and_copy(&acm_dst->hue_table, pinfo->acm_lut_hue_table,
		ACM_HUE_LUT_LENGTH, false)) {
		DPU_FB_ERR("failed to set acm hut table from panel\n");
		return -EINVAL;
	}
	if (dpu_effect_alloc_and_copy(&acm_dst->sata_table, pinfo->acm_lut_sata_table,
		ACM_SATA_LUT_LENGTH, false)) {
		DPU_FB_ERR("failed to set acm sata table from panel\n");
		return -EINVAL;
	}
	if (dpu_effect_alloc_and_copy(&acm_dst->satr0_table, pinfo->acm_lut_satr0_table,
		ACM_SATR_LUT_LENGTH, false)) {
		DPU_FB_ERR("failed to set acm satr0 table from panel\n");
		return -EINVAL;
	}
	if (dpu_effect_alloc_and_copy(&acm_dst->satr1_table, pinfo->acm_lut_satr1_table,
		ACM_SATR_LUT_LENGTH, false)) {
		DPU_FB_ERR("failed to set acm satr1 table from panel\n");
		return -EINVAL;
	}
	if (dpu_effect_alloc_and_copy(&acm_dst->satr2_table, pinfo->acm_lut_satr2_table,
		ACM_SATR_LUT_LENGTH, false)) {
		DPU_FB_ERR("failed to set acm satr2 table from panel\n");
		return -EINVAL;
	}
	if (dpu_effect_alloc_and_copy(&acm_dst->satr3_table, pinfo->acm_lut_satr3_table,
		ACM_SATR_LUT_LENGTH, false)) {
		DPU_FB_ERR("failed to set acm satr3 table from panel\n");
		return -EINVAL;
	}
	if (dpu_effect_alloc_and_copy(&acm_dst->satr4_table, pinfo->acm_lut_satr4_table,
		ACM_SATR_LUT_LENGTH, false)) {
		DPU_FB_ERR("failed to set acm satr4 table from panel\n");
		return -EINVAL;
	}
	if (dpu_effect_alloc_and_copy(&acm_dst->satr5_table, pinfo->acm_lut_satr5_table,
		ACM_SATR_LUT_LENGTH, false)) {
		DPU_FB_ERR("failed to set acm satr5 table from panel\n");
		return -EINVAL;
	}
	if (dpu_effect_alloc_and_copy(&acm_dst->satr6_table, pinfo->acm_lut_satr6_table,
		ACM_SATR_LUT_LENGTH, false)) {
		DPU_FB_ERR("failed to set acm satr6 table from panel\n");
		return -EINVAL;
	}
	if (dpu_effect_alloc_and_copy(&acm_dst->satr7_table, pinfo->acm_lut_satr7_table,
		ACM_SATR_LUT_LENGTH, false)) {
		DPU_FB_ERR("failed to set acm satr7 table from panel\n");
		return -EINVAL;
	}

	return 0;
}

static int set_acm_cinema_param(struct acm_info *acm_dst, struct dpu_panel_info *pinfo)
{
	if (acm_dst == NULL) {
		DPU_FB_DEBUG("acm_dst is NULL!\n");
		return -1;
	}

	if (pinfo == NULL) {
		DPU_FB_DEBUG("pinfo is NULL!\n");
		return -1;
	}

	acm_dst->acm_hue_rlh01 = (pinfo->cinema_r1_lh<<16) | pinfo->cinema_r0_lh;
	acm_dst->acm_hue_rlh23 = (pinfo->cinema_r3_lh<<16) | pinfo->cinema_r2_lh;
	acm_dst->acm_hue_rlh45 = (pinfo->cinema_r5_lh<<16) | pinfo->cinema_r4_lh;
	acm_dst->acm_hue_rlh67 = (pinfo->cinema_r6_hh<<16) | pinfo->cinema_r6_lh;

	acm_dst->acm_hue_param01 = pinfo->hue_param01;
	acm_dst->acm_hue_param23 = pinfo->hue_param23;
	acm_dst->acm_hue_param45 = pinfo->hue_param45;
	acm_dst->acm_hue_param67 = pinfo->hue_param67;
	acm_dst->acm_hue_smooth0 = pinfo->hue_smooth0;
	acm_dst->acm_hue_smooth1 = pinfo->hue_smooth1;
	acm_dst->acm_hue_smooth2 = pinfo->hue_smooth2;
	acm_dst->acm_hue_smooth3 = pinfo->hue_smooth3;
	acm_dst->acm_hue_smooth4 = pinfo->hue_smooth4;
	acm_dst->acm_hue_smooth5 = pinfo->hue_smooth5;
	acm_dst->acm_hue_smooth6 = pinfo->hue_smooth6;
	acm_dst->acm_hue_smooth7 = pinfo->hue_smooth7;
	acm_dst->acm_color_choose = pinfo->color_choose;
	acm_dst->acm_l_cont_en = pinfo->l_cont_en;
	acm_dst->acm_lc_param01  = pinfo->lc_param01;
	acm_dst->acm_lc_param23  = pinfo->lc_param23;
	acm_dst->acm_lc_param45  = pinfo->lc_param45;
	acm_dst->acm_lc_param67  = pinfo->lc_param67;
	acm_dst->acm_l_adj_ctrl = pinfo->l_adj_ctrl;
	acm_dst->acm_capture_ctrl = pinfo->capture_ctrl;
	acm_dst->acm_ink_ctrl = pinfo->ink_ctrl;
	acm_dst->acm_ink_out = pinfo->ink_out;
	/* malloc acm_dst lut table memory*/
	if (dpu_effect_alloc_and_copy(&acm_dst->hue_table, pinfo->cinema_acm_lut_hue_table,
		ACM_HUE_LUT_LENGTH, false)) {
		DPU_FB_ERR("failed to set acm hut table from panel cinema mode\n");
		return -EINVAL;
	}
	if (dpu_effect_alloc_and_copy(&acm_dst->sata_table, pinfo->cinema_acm_lut_sata_table,
		ACM_SATA_LUT_LENGTH, false)) {
		DPU_FB_ERR("failed to set acm sata table from panel cinema mode\n");
		return -EINVAL;
	}
	if (dpu_effect_alloc_and_copy(&acm_dst->satr0_table, pinfo->cinema_acm_lut_satr0_table,
		ACM_SATR_LUT_LENGTH, false)) {
		DPU_FB_ERR("failed to set acm satr0 table from panel cinema mode\n");
		return -EINVAL;
	}
	if (dpu_effect_alloc_and_copy(&acm_dst->satr1_table, pinfo->cinema_acm_lut_satr1_table,
		ACM_SATR_LUT_LENGTH, false)) {
		DPU_FB_ERR("failed to set acm satr1 table from panel cinema mode\n");
		return -EINVAL;
	}
	if (dpu_effect_alloc_and_copy(&acm_dst->satr2_table, pinfo->cinema_acm_lut_satr2_table,
		ACM_SATR_LUT_LENGTH, false)) {
		DPU_FB_ERR("failed to set acm satr2 table from panel cinema mode\n");
		return -EINVAL;
	}
	if (dpu_effect_alloc_and_copy(&acm_dst->satr3_table, pinfo->cinema_acm_lut_satr3_table,
		ACM_SATR_LUT_LENGTH, false)) {
		DPU_FB_ERR("failed to set acm satr3 table from panel cinema mode\n");
		return -EINVAL;
	}
	if (dpu_effect_alloc_and_copy(&acm_dst->satr4_table, pinfo->cinema_acm_lut_satr4_table,
		ACM_SATR_LUT_LENGTH, false)) {
		DPU_FB_ERR("failed to set acm satr4 table from panel cinema mode\n");
		return -EINVAL;
	}
	if (dpu_effect_alloc_and_copy(&acm_dst->satr5_table, pinfo->cinema_acm_lut_satr5_table,
		ACM_SATR_LUT_LENGTH, false)) {
		DPU_FB_ERR("failed to set acm satr5 table from panel cinema mode\n");
		return -EINVAL;
	}
	if (dpu_effect_alloc_and_copy(&acm_dst->satr6_table, pinfo->cinema_acm_lut_satr6_table,
		ACM_SATR_LUT_LENGTH, false)) {
		DPU_FB_ERR("failed to set acm satr6 table from panel cinema mode\n");
		return -EINVAL;
	}
	if (dpu_effect_alloc_and_copy(&acm_dst->satr7_table, pinfo->cinema_acm_lut_satr7_table,
		ACM_SATR_LUT_LENGTH, false)) {
		DPU_FB_ERR("failed to set acm satr7 table from panel cinema mode\n");
		return -EINVAL;
	}

	return 0;
}

static int set_acm_user_param(struct acm_info *acm_dst, struct acm_info *acm_src)
{
	if (acm_dst == NULL) {
		DPU_FB_DEBUG("acm_dst is NULL!\n");
		return -1;
	}

	if (acm_src == NULL) {
		DPU_FB_DEBUG("acm_src is NULL!\n");
		return -1;
	}

	acm_dst->acm_en = acm_src->acm_en;
	acm_dst->sata_offset = acm_src->sata_offset;
	acm_dst->acm_hue_rlh01 = acm_src->acm_hue_rlh01;
	acm_dst->acm_hue_rlh23 = acm_src->acm_hue_rlh23;
	acm_dst->acm_hue_rlh45 = acm_src->acm_hue_rlh45;
	acm_dst->acm_hue_rlh67 = acm_src->acm_hue_rlh67;
	acm_dst->acm_hue_param01 = acm_src->acm_hue_param01;
	acm_dst->acm_hue_param23 = acm_src->acm_hue_param23;
	acm_dst->acm_hue_param45 = acm_src->acm_hue_param45;
	acm_dst->acm_hue_param67 = acm_src->acm_hue_param67;
	acm_dst->acm_hue_smooth0 = acm_src->acm_hue_smooth0;
	acm_dst->acm_hue_smooth1 = acm_src->acm_hue_smooth1;
	acm_dst->acm_hue_smooth2 = acm_src->acm_hue_smooth2;
	acm_dst->acm_hue_smooth3 = acm_src->acm_hue_smooth3;
	acm_dst->acm_hue_smooth4 = acm_src->acm_hue_smooth4;
	acm_dst->acm_hue_smooth5 = acm_src->acm_hue_smooth5;
	acm_dst->acm_hue_smooth6 = acm_src->acm_hue_smooth6;
	acm_dst->acm_hue_smooth7 = acm_src->acm_hue_smooth7;
	acm_dst->acm_color_choose = acm_src->acm_color_choose;
	acm_dst->acm_l_cont_en = acm_src->acm_l_cont_en;
	acm_dst->acm_lc_param01 = acm_src->acm_lc_param01;
	acm_dst->acm_lc_param23 = acm_src->acm_lc_param23;
	acm_dst->acm_lc_param45 = acm_src->acm_lc_param45;
	acm_dst->acm_lc_param67 = acm_src->acm_lc_param67;
	acm_dst->acm_l_adj_ctrl = acm_src->acm_l_adj_ctrl;
	acm_dst->acm_capture_out = acm_src->acm_capture_out;
	acm_dst->acm_ink_ctrl = acm_src->acm_ink_ctrl;
	acm_dst->acm_ink_out = acm_src->acm_ink_out;

	/* malloc acm_dst lut table memory*/
	if (dpu_effect_alloc_and_copy(&acm_dst->hue_table, acm_src->hue_table,
		ACM_HUE_LUT_LENGTH, true)) {
		DPU_FB_ERR("failed to copy acm hut table from user\n");
		return -EINVAL;
	}

	if (dpu_effect_alloc_and_copy(&acm_dst->sata_table, acm_src->sata_table,
		ACM_SATA_LUT_LENGTH, true)) {
		DPU_FB_ERR("failed to copy acm sata table from user\n");
		return -EINVAL;
	}
	if (dpu_effect_alloc_and_copy(&acm_dst->satr0_table, acm_src->satr0_table,
		ACM_SATR_LUT_LENGTH, true)) {
		DPU_FB_ERR("failed to copy acm satr0 table from user\n");
		return -EINVAL;
	}
	if (dpu_effect_alloc_and_copy(&acm_dst->satr1_table, acm_src->satr1_table,
		ACM_SATR_LUT_LENGTH, true)) {
		DPU_FB_ERR("failed to copy acm satr1 table from user\n");
		return -EINVAL;
	}
	if (dpu_effect_alloc_and_copy(&acm_dst->satr2_table, acm_src->satr2_table,
		ACM_SATR_LUT_LENGTH, true)) {
		DPU_FB_ERR("failed to copy acm satr2 table from user\n");
		return -EINVAL;
	}
	if (dpu_effect_alloc_and_copy(&acm_dst->satr3_table, acm_src->satr3_table,
		ACM_SATR_LUT_LENGTH, true)) {
		DPU_FB_ERR("failed to copy acm satr3 table from user\n");
		return -EINVAL;
	}
	if (dpu_effect_alloc_and_copy(&acm_dst->satr4_table, acm_src->satr4_table,
		ACM_SATR_LUT_LENGTH, true)) {
		DPU_FB_ERR("failed to copy acm satr4 table from user\n");
		return -EINVAL;
	}
	if (dpu_effect_alloc_and_copy(&acm_dst->satr5_table, acm_src->satr5_table,
		ACM_SATR_LUT_LENGTH, true)) {
		DPU_FB_ERR("failed to copy acm satr5 table from user\n");
		return -EINVAL;
	}
	if (dpu_effect_alloc_and_copy(&acm_dst->satr6_table, acm_src->satr6_table,
		ACM_SATR_LUT_LENGTH, true)) {
		DPU_FB_ERR("failed to copy acm satr6 table from user\n");
		return -EINVAL;
	}
	if (dpu_effect_alloc_and_copy(&acm_dst->satr7_table, acm_src->satr7_table,
		ACM_SATR_LUT_LENGTH, true)) {
		DPU_FB_ERR("failed to copy acm satr7 table from user\n");
		return -EINVAL;
	}

	return 0;
}

int dpu_effect_save_acm_info(struct dpu_fb_data_type *dpufd, struct dss_effect_info *effect_info_src)
{
	struct acm_info *acm_dst = NULL;
	struct dpu_panel_info *pinfo = NULL;

	if (NULL == dpufd) {
		DPU_FB_ERR("dpufd is NULL!\n");
		return -EINVAL;
	}

	if (effect_info_src == NULL) {
		DPU_FB_ERR("fb%d, effect_info_src is NULL!\n", dpufd->index);
		return -EINVAL;
	}

	acm_dst = &(dpufd->effect_info[effect_info_src->disp_panel_id].acm);
	pinfo = &(dpufd->panel_info);

	if (!dpufd->effect_ctl.acm_support) {
		DPU_FB_INFO("fb%d, acm is not supported!\n", dpufd->index);
		return 0;
	}

	/*set acm info*/
	if (dpufd->effect_updated_flag[effect_info_src->disp_panel_id].acm_effect_updated == false) {
		acm_dst->acm_en = effect_info_src->acm.acm_en;
		if (effect_info_src->acm.param_mode == 0) {
			if (set_acm_normal_param(acm_dst, pinfo)) {
				DPU_FB_ERR("fb%d, failed to set acm normal mode parameters\n", dpufd->index);
				goto err_ret;
			}
		} else if (effect_info_src->acm.param_mode == 1) {
			if (set_acm_cinema_param(acm_dst, pinfo)) {
				DPU_FB_ERR("fb%d, failed to set acm cinema mode parameters\n", dpufd->index);
				goto err_ret;
			}
		} else if (effect_info_src->acm.param_mode == 2) {
			if (set_acm_user_param(acm_dst, &effect_info_src->acm)) {
				DPU_FB_ERR("fb%d, failed to set acm user mode parameters\n", dpufd->index);
				goto err_ret;
			}
		} else {
			DPU_FB_ERR("fb%d, invalid acm para mode!\n", dpufd->index);
			return -EINVAL;
		}

		dpufd->effect_updated_flag[effect_info_src->disp_panel_id].acm_effect_updated = true;
	}
	return 0;

err_ret:
	free_acm_table(acm_dst);
	return -EINVAL;
}

int dpu_effect_gmp_info_set(struct dpu_fb_data_type *dpufd, struct lcp_info *lcp_src){
	struct lcp_info *lcp_dst = NULL;
	struct dss_effect *effect = NULL;

	if (NULL == dpufd) {
		DPU_FB_ERR("dpufd is NULL!\n");
		return -EINVAL;
	}

	if (!g_is_effect_lock_init) {
		DPU_FB_INFO("display effect lock is not init!\n");
		return -EINVAL;
	}

	if (NULL == lcp_src) {
		DPU_FB_ERR("fb%d, lcp_src is NULL!\n", dpufd->index);
		return -EINVAL;
	}

	lcp_dst = &(dpufd->effect_info[dpufd->panel_info.disp_panel_id].lcp);
	effect = &(dpufd->effect_ctl);

	if (!effect->lcp_gmp_support) {
		DPU_FB_INFO("fb%d, lcp gmp is not supported!\n", dpufd->index);
		return 0;
	}

	spin_lock(&g_gmp_effect_lock);

	lcp_dst->gmp_enable = lcp_src->gmp_enable;
	if (dpu_effect_alloc_and_copy(&lcp_dst->gmp_table_high4, lcp_src->gmp_table_high4,
		LCP_GMP_LUT_LENGTH, true)) {
		DPU_FB_ERR("fb%d, failed to set gmp_table_high4!\n", dpufd->index);
		goto err_ret;
	}

	if (dpu_effect_alloc_and_copy(&lcp_dst->gmp_table_low32, lcp_src->gmp_table_low32,
		LCP_GMP_LUT_LENGTH, true)) {
		DPU_FB_ERR("fb%d, failed to set gmp_lut_table_low32bit!\n", dpufd->index);
		goto err_ret;
	}

	dpufd->effect_updated_flag[dpufd->panel_info.disp_panel_id].gmp_effect_updated = true;

	spin_unlock(&g_gmp_effect_lock);
	return 0;

err_ret:
	dpu_effect_kfree(&lcp_dst->gmp_table_high4);
	dpu_effect_kfree(&lcp_dst->gmp_table_low32);

	spin_unlock(&g_gmp_effect_lock);
	return -EINVAL;
}

int dpu_effect_igm_info_set(struct dpu_fb_data_type *dpufd, struct lcp_info *lcp_src){
	struct lcp_info *lcp_dst = NULL;
	struct dss_effect *effect = NULL;

	if (NULL == dpufd) {
		DPU_FB_ERR("dpufd is NULL!\n");
		return -EINVAL;
	}

	if (!g_is_effect_lock_init) {
		DPU_FB_INFO("display effect lock is not init!\n");
		return -EINVAL;
	}

	if (NULL == lcp_src) {
		DPU_FB_ERR("fb%d, lcp_src is NULL!\n", dpufd->index);
		return -EINVAL;
	}

	lcp_dst = &(dpufd->effect_info[dpufd->panel_info.disp_panel_id].lcp);
	effect = &(dpufd->effect_ctl);

	if (!effect->lcp_igm_support) {
		DPU_FB_INFO("fb%d, lcp degamma is not supported!\n", dpufd->index);
		return 0;
	}

	spin_lock(&g_igm_effect_lock);

	lcp_dst->igm_enable = lcp_src->igm_enable;

	if (dpu_effect_alloc_and_copy(&lcp_dst->igm_r_table, lcp_src->igm_r_table,
		IGM_LUT_LEN, true)) {
		DPU_FB_ERR("fb%d, failed to set igm_r_table!\n", dpufd->index);
		goto err_ret;
	}

	if (dpu_effect_alloc_and_copy(&lcp_dst->igm_g_table, lcp_src->igm_g_table,
		IGM_LUT_LEN, true)) {
		DPU_FB_ERR("fb%d, failed to set igm_g_table!\n", dpufd->index);
		goto err_ret;
	}

	if (dpu_effect_alloc_and_copy(&lcp_dst->igm_b_table, lcp_src->igm_b_table,
		IGM_LUT_LEN, true)) {
		DPU_FB_ERR("fb%d, failed to set igm_b_table!\n", dpufd->index);
		goto err_ret;
	}

	dpufd->effect_updated_flag[dpufd->panel_info.disp_panel_id].igm_effect_updated = true;

	spin_unlock(&g_igm_effect_lock);
	return 0;

err_ret:
	dpu_effect_kfree(&lcp_dst->igm_r_table);
	dpu_effect_kfree(&lcp_dst->igm_g_table);
	dpu_effect_kfree(&lcp_dst->igm_b_table);

	spin_unlock(&g_igm_effect_lock);
	return -EINVAL;
}

int dpu_effect_xcc_info_set(struct dpu_fb_data_type *dpufd, struct lcp_info *lcp_src){
	struct lcp_info *lcp_dst = NULL;
	struct dss_effect *effect = NULL;

	if (NULL == dpufd) {
		DPU_FB_ERR("dpufd is NULL!\n");
		return -EINVAL;
	}

	if (!g_is_effect_lock_init) {
		DPU_FB_INFO("display effect lock is not init!\n");
		return -EINVAL;
	}

	if (NULL == lcp_src) {
		DPU_FB_ERR("fb%d, lcp_src is NULL!\n", dpufd->index);
		return -EINVAL;
	}

	lcp_dst = &(dpufd->effect_info[dpufd->panel_info.disp_panel_id].lcp);
	effect = &(dpufd->effect_ctl);

	if (!effect->lcp_xcc_support) {
		DPU_FB_INFO("fb%d, lcp xcc are not supported!\n", dpufd->index);
		return 0;
	}

	spin_lock(&g_xcc_effect_lock);

	lcp_dst->xcc_enable = lcp_src->xcc_enable;

	if (dpu_effect_alloc_and_copy(&lcp_dst->xcc_table, lcp_src->xcc_table,
		LCP_XCC_LUT_LENGTH, true)) {
		DPU_FB_ERR("fb%d, failed to set xcc_table!\n", dpufd->index);
		goto err_ret;
	}

	dpufd->effect_updated_flag[dpufd->panel_info.disp_panel_id].xcc_effect_updated = true;

	spin_unlock(&g_xcc_effect_lock);
	return 0;

err_ret:
	dpu_effect_kfree(&lcp_dst->xcc_table);

	spin_unlock(&g_xcc_effect_lock);
	return -EINVAL;
}

int dpu_effect_xcc_info_set_kernel(struct dpu_fb_data_type *dpufd, struct dss_display_effect_xcc *lcp_src) {
	struct lcp_info *lcp_dst = NULL;
	struct dss_effect *effect = NULL;
	struct dpu_fb_data_type *dpufd_primary = NULL;

	dpufd_primary = dpufd_list[PRIMARY_PANEL_IDX];

	if (dpufd_primary == NULL) {
		DPU_FB_ERR("dpufd_primary is NULL pointer, return!\n");
		return -EINVAL;
	}

	if (NULL == dpufd) {
		DPU_FB_ERR("dpufd is NULL!\n");
		return -EINVAL;
	}

	if (!g_is_effect_lock_init) {
		DPU_FB_INFO("display effect lock is not init!\n");
		return -EINVAL;
	}

	if (NULL == lcp_src) {
		DPU_FB_ERR("fb%d, lcp_src is NULL!\n", dpufd->index);
		return -EINVAL;
	}

	lcp_dst = &(dpufd->effect_info[dpufd->panel_info.disp_panel_id].lcp);
	effect = &(dpufd->effect_ctl);

	if (!effect->lcp_xcc_support) {
		DPU_FB_INFO("fb%d, lcp xcc are not supported!\n", dpufd->index);
		return 0;
	}

	spin_lock(&g_xcc_effect_lock);

	lcp_dst->xcc_enable = lcp_src->xcc_enable;

	if (dpu_effect_alloc_and_copy(&lcp_dst->xcc_table, lcp_src->xcc_table,
		LCP_XCC_LUT_LENGTH, false)) {
		DPU_FB_ERR("fb%d, failed to set xcc_table!\n", dpufd->index);
		goto err_ret;
	}
	/*the display effect is not allowed to set reg when the partical update*/
	if (dpufd_primary->display_effect_flag < 5)
		dpufd_primary->display_effect_flag = 4;

	dpufd->effect_updated_flag[dpufd->panel_info.disp_panel_id].xcc_effect_updated = true;
	spin_unlock(&g_xcc_effect_lock);

	return 0;

err_ret:
	dpu_effect_kfree(&lcp_dst->xcc_table);

	spin_unlock(&g_xcc_effect_lock);
	return -EINVAL;
}

static int dpu_efffect_gamma_param_set(struct gamma_info *gammaDst, struct gamma_info *gammaSrc,
                                  struct dpu_panel_info* pInfo) {

	if((gammaDst == NULL) || (gammaSrc == NULL) || (pInfo == NULL)){
		DPU_FB_ERR("gammaDst or gammaSrc or pInfo is null pointer\n");
		return -1;
	}

	if (gammaSrc->para_mode == 0) {
		//Normal mode
		if (dpu_effect_alloc_and_copy(&gammaDst->gamma_r_table, pInfo->gamma_lut_table_R,
			GAMMA_LUT_LEN, false)) {
			DPU_FB_ERR("failed to set gamma_r_table!\n");
			goto err_ret;
		}

		if (dpu_effect_alloc_and_copy(&gammaDst->gamma_g_table, pInfo->gamma_lut_table_G,
			GAMMA_LUT_LEN, false)) {
			DPU_FB_ERR("failed to set gamma_g_table!\n");
			goto err_ret;
		}

		if (dpu_effect_alloc_and_copy(&gammaDst->gamma_b_table, pInfo->gamma_lut_table_B,
			GAMMA_LUT_LEN, false)) {
			DPU_FB_ERR("failed to set gamma_b_table!\n");
			goto err_ret;
		}
	} else if (gammaSrc->para_mode == 1) {
		//Cinema mode
		if (dpu_effect_alloc_and_copy(&gammaDst->gamma_r_table, pInfo->cinema_gamma_lut_table_R,
			GAMMA_LUT_LEN, false)) {
			DPU_FB_ERR("failed to set gamma_r_table!\n");
			goto err_ret;
		}

		if (dpu_effect_alloc_and_copy(&gammaDst->gamma_g_table, pInfo->cinema_gamma_lut_table_G,
			GAMMA_LUT_LEN, false)) {
			DPU_FB_ERR("failed to set gamma_g_table!\n");
			goto err_ret;
		}

		if (dpu_effect_alloc_and_copy(&gammaDst->gamma_b_table, pInfo->cinema_gamma_lut_table_B,
			GAMMA_LUT_LEN, false)) {
			DPU_FB_ERR("failed to set gamma_b_table!\n");
			goto err_ret;
		}
	} else if (gammaSrc->para_mode == 2) {
		if (dpu_effect_alloc_and_copy(&gammaDst->gamma_r_table, gammaSrc->gamma_r_table,
			GAMMA_LUT_LEN, true)) {
			DPU_FB_ERR("failed to copy gamma_r_table from user!\n");
			goto err_ret;
		}

		if (dpu_effect_alloc_and_copy(&gammaDst->gamma_g_table, gammaSrc->gamma_g_table,
			GAMMA_LUT_LEN, true)) {
			DPU_FB_ERR("failed to copy gamma_g_table from user!\n");
			goto err_ret;
		}

		if (dpu_effect_alloc_and_copy(&gammaDst->gamma_b_table, gammaSrc->gamma_b_table,
			GAMMA_LUT_LEN, true)) {
			DPU_FB_ERR("failed to copy gamma_b_table from user!\n");
			goto err_ret;
		}
	} else {
		DPU_FB_ERR("not supported gamma para_mode!\n");
		return -EINVAL;
	}

	return 0;

err_ret:
	free_gamma_table(gammaDst);

	return -EINVAL;
}

int dpu_effect_gamma_info_set(struct dpu_fb_data_type *dpufd, struct gamma_info *gamma_src)
{
	struct gamma_info *gamma_dst = NULL;
	struct dpu_panel_info *pinfo = NULL;
	int ret;

	if (NULL == dpufd) {
		DPU_FB_ERR("dpufd is NULL!\n");
		return -EINVAL;
	}

	if (!g_is_effect_lock_init) {
		DPU_FB_INFO("display effect lock is not init!\n");
		return -EINVAL;
	}

	if (NULL == gamma_src) {
		DPU_FB_ERR("fb%d, gamma_src is NULL!\n", dpufd->index);
		return -EINVAL;
	}

	gamma_dst = &(dpufd->effect_info[dpufd->panel_info.disp_panel_id].gamma);
	pinfo = &(dpufd->panel_info);

	if (!dpufd->effect_ctl.gamma_support) {
		DPU_FB_INFO("fb%d, gamma is not supported!\n", dpufd->index);
		return 0;
	}

	spin_lock(&g_gamma_effect_lock);

	gamma_dst->enable = gamma_src->enable;
	gamma_dst->para_mode = gamma_src->para_mode;
	ret = dpu_efffect_gamma_param_set(gamma_dst, gamma_src, pinfo);
	if (ret < 0) {
		DPU_FB_ERR("fb%d, failed to set gamma table!\n", dpufd->index);
		spin_unlock(&g_gamma_effect_lock);
		return ret;
	}

	dpufd->effect_updated_flag[dpufd->panel_info.disp_panel_id].gamma_effect_updated = true;

	spin_unlock(&g_gamma_effect_lock);

	return 0;
}

void dpu_effect_acm_set_reg(struct dpu_fb_data_type *dpufd)
{
	struct acm_info *acm_param = NULL;
	char __iomem *acm_base = NULL;
	char __iomem *acm_lut_base = NULL;
	static uint32_t acm_config_flag = 0;
	uint32_t acm_lut_sel;

	if (NULL == dpufd) {
		DPU_FB_ERR("dpufd is NULL!");
		return;
	}

	if (!dpufd->effect_ctl.acm_support) {
		return;
	}

	if (!dpufd->effect_updated_flag[dpufd->panel_info.disp_panel_id].acm_effect_updated) {
		return;
	}

	acm_base = dpufd->dss_base + DSS_DPP_ACM_OFFSET;
	acm_lut_base = dpufd->dss_base + DSS_DPP_ACM_LUT_OFFSET;

	if (acm_config_flag == 0) {
		//Disable ACM
		set_reg(acm_base + ACM_EN, 0x0, 1, 0);
		acm_config_flag = 1;
		return;
	}

	acm_param = &(dpufd->effect_info[dpufd->panel_info.disp_panel_id].acm);

	if (NULL == acm_param->hue_table) {
		DPU_FB_ERR("fb%d, invalid acm hue table param!\n", dpufd->index);
		goto err_ret;
	}

	if (NULL == acm_param->sata_table) {
		DPU_FB_ERR("fb%d, invalid acm sata table param!\n", dpufd->index);
		goto err_ret;
	}

	if ((NULL == acm_param->satr0_table) || (NULL == acm_param->satr1_table)
		|| (NULL == acm_param->satr2_table) || (NULL == acm_param->satr3_table)
		|| (NULL == acm_param->satr4_table) || (NULL == acm_param->satr5_table)
		|| (NULL == acm_param->satr6_table) || (NULL == acm_param->satr7_table)) {
		DPU_FB_ERR("fb%d, invalid acm satr table param!\n", dpufd->index);
		goto err_ret;
	}

	set_reg(acm_base + ACM_SATA_OFFSET, acm_param->sata_offset, 6, 0);
	set_reg(acm_base + ACM_HUE_RLH01, acm_param->acm_hue_rlh01, 32, 0);
	set_reg(acm_base + ACM_HUE_RLH23, acm_param->acm_hue_rlh23, 32, 0);
	set_reg(acm_base + ACM_HUE_RLH45, acm_param->acm_hue_rlh45, 32, 0);
	set_reg(acm_base + ACM_HUE_RLH67, acm_param->acm_hue_rlh67, 32, 0);
	set_reg(acm_base + ACM_HUE_PARAM01, acm_param->acm_hue_param01, 32,0);
	set_reg(acm_base + ACM_HUE_PARAM23, acm_param->acm_hue_param23, 32,0);
	set_reg(acm_base + ACM_HUE_PARAM45, acm_param->acm_hue_param45, 32,0);
	set_reg(acm_base + ACM_HUE_PARAM67, acm_param->acm_hue_param67, 32,0);
	set_reg(acm_base + ACM_HUE_SMOOTH0, acm_param->acm_hue_smooth0, 32,0);
	set_reg(acm_base + ACM_HUE_SMOOTH1, acm_param->acm_hue_smooth1, 32,0);
	set_reg(acm_base + ACM_HUE_SMOOTH2, acm_param->acm_hue_smooth2, 32,0);
	set_reg(acm_base + ACM_HUE_SMOOTH3, acm_param->acm_hue_smooth3, 32,0);
	set_reg(acm_base + ACM_HUE_SMOOTH4, acm_param->acm_hue_smooth4, 32,0);
	set_reg(acm_base + ACM_HUE_SMOOTH5, acm_param->acm_hue_smooth5, 32,0);
	set_reg(acm_base + ACM_HUE_SMOOTH6, acm_param->acm_hue_smooth6, 32,0);
	set_reg(acm_base + ACM_HUE_SMOOTH7, acm_param->acm_hue_smooth7, 32,0);
	set_reg(acm_base + ACM_COLOR_CHOOSE,acm_param->acm_color_choose,1,0);
	set_reg(acm_base + ACM_L_CONT_EN, acm_param->acm_l_cont_en, 1,0);
	set_reg(acm_base + ACM_LC_PARAM01, acm_param->acm_lc_param01, 32,0);
	set_reg(acm_base + ACM_LC_PARAM23, acm_param->acm_lc_param23, 32,0);
	set_reg(acm_base + ACM_LC_PARAM45, acm_param->acm_lc_param45, 32,0);
	set_reg(acm_base + ACM_LC_PARAM67, acm_param->acm_lc_param67, 32,0);
	set_reg(acm_base + ACM_L_ADJ_CTRL, acm_param->acm_l_adj_ctrl, 32,0);
	set_reg(acm_base + ACM_CAPTURE_CTRL, acm_param->acm_capture_ctrl,32,0);
	set_reg(acm_base + ACM_INK_CTRL, acm_param->acm_ink_ctrl, 32,0);
	set_reg(acm_base + ACM_INK_OUT, acm_param->acm_ink_out, 30,0);

	acm_set_lut_hue(acm_lut_base + ACM_U_H_COEF, acm_param->hue_table, ACM_HUE_LUT_LENGTH);
	acm_set_lut(acm_lut_base + ACM_U_SATA_COEF, acm_param->sata_table, ACM_SATA_LUT_LENGTH);
	acm_set_lut(acm_lut_base + ACM_U_SATR0_COEF, acm_param->satr0_table, ACM_SATR_LUT_LENGTH);
	acm_set_lut(acm_lut_base + ACM_U_SATR1_COEF, acm_param->satr1_table, ACM_SATR_LUT_LENGTH);
	acm_set_lut(acm_lut_base + ACM_U_SATR2_COEF, acm_param->satr2_table, ACM_SATR_LUT_LENGTH);
	acm_set_lut(acm_lut_base + ACM_U_SATR3_COEF, acm_param->satr3_table, ACM_SATR_LUT_LENGTH);
	acm_set_lut(acm_lut_base + ACM_U_SATR4_COEF, acm_param->satr4_table, ACM_SATR_LUT_LENGTH);
	acm_set_lut(acm_lut_base + ACM_U_SATR5_COEF, acm_param->satr5_table, ACM_SATR_LUT_LENGTH);
	acm_set_lut(acm_lut_base + ACM_U_SATR6_COEF, acm_param->satr6_table, ACM_SATR_LUT_LENGTH);
	acm_set_lut(acm_lut_base + ACM_U_SATR7_COEF, acm_param->satr7_table, ACM_SATR_LUT_LENGTH);

	acm_lut_sel = (uint32_t)inp32(acm_base + ACM_LUT_SEL);
	set_reg(acm_base + ACM_LUT_SEL, (~(acm_lut_sel & 0x380)) & (acm_lut_sel | 0x380), 16, 0);

	set_reg(acm_base + ACM_EN, acm_param->acm_en, 1, 0);
err_ret:
	dpufd->effect_updated_flag[dpufd->panel_info.disp_panel_id].acm_effect_updated = false;

	//free_acm_table(acm_param);
	acm_config_flag = 0;
	return;
}

#define XCC_COEF_LEN	12
#define GMP_BLOCK_SIZE	137
#define GMP_CNT_NUM 18
#define GMP_COFE_CNT 729

static bool lcp_igm_set_reg(char __iomem *degamma_lut_base, struct lcp_info *lcp_param)
{
	int cnt;

	if (degamma_lut_base == NULL) {
		DPU_FB_ERR("lcp_lut_base is NULL!\n");
		return false;
	}

	if (lcp_param == NULL) {
		DPU_FB_ERR("lcp_param is NULL!\n");
		return false;
	}

	if (lcp_param->igm_r_table == NULL || lcp_param->igm_g_table == NULL || lcp_param->igm_b_table == NULL) {
		DPU_FB_INFO("igm_r_table or igm_g_table or igm_b_table is NULL!\n");
		return false;
	}

	for (cnt = 0; cnt < IGM_LUT_LEN; cnt = cnt + 2) {
		set_reg(degamma_lut_base + (U_DEGAMA_R_COEF + cnt * 2), lcp_param->igm_r_table[cnt], 12,0);
		if(cnt != IGM_LUT_LEN-1)
			set_reg(degamma_lut_base + (U_DEGAMA_R_COEF + cnt * 2), lcp_param->igm_r_table[cnt+1], 12,16);

		set_reg(degamma_lut_base + (U_DEGAMA_G_COEF + cnt * 2), lcp_param->igm_g_table[cnt], 12,0);
		if(cnt != IGM_LUT_LEN-1)
			set_reg(degamma_lut_base + (U_DEGAMA_G_COEF + cnt * 2), lcp_param->igm_g_table[cnt+1], 12,16);

		set_reg(degamma_lut_base + (U_DEGAMA_B_COEF + cnt * 2), lcp_param->igm_b_table[cnt], 12,0);
		if(cnt != IGM_LUT_LEN-1)
			set_reg(degamma_lut_base + (U_DEGAMA_B_COEF + cnt * 2), lcp_param->igm_b_table[cnt+1], 12,16);
	}
	return true;
}

static bool lcp_xcc_set_reg(char __iomem *xcc_base, struct lcp_info *lcp_param)
{
	int cnt;

	if (xcc_base == NULL) {
		DPU_FB_DEBUG("lcp_base is NULL!\n");
		return false;
	}

	if (lcp_param == NULL) {
		DPU_FB_DEBUG("lcp_param is NULL!\n");
		return false;
	}

	if (lcp_param->xcc_table == NULL) {
		DPU_FB_DEBUG("xcc_table is NULL!\n");
		return false;
	}
	for (cnt = 0; cnt < XCC_COEF_LEN; cnt++) {
		set_reg(xcc_base + LCP_XCC_COEF_00 +cnt * 4,  lcp_param->xcc_table[cnt], 17, 0);
	}
	return true;
}

static void reset_xcc_reg(char __iomem *xcc_base)
{
	int cnt;
	uint32_t coef_01 = 0;
	uint32_t coef_12 = 0;
	uint32_t coef_23 = 0;
	uint32_t max_coef = 0;

	coef_01 = inp32(xcc_base + LCP_XCC_COEF_01);
	coef_12 = inp32(xcc_base + LCP_XCC_COEF_12);
	coef_23 = inp32(xcc_base + LCP_XCC_COEF_23);
	max_coef = max(coef_01, coef_12);
	max_coef = max(max_coef, coef_23);
	if (max_coef > 0xFFFF) {
		DPU_FB_INFO("xcc coef is negative!\n");
		return;
	}
	if (max_coef != 0) {
		coef_01 = coef_01 * XCC_DEFAULT / max_coef;
		coef_12 = coef_12 * XCC_DEFAULT / max_coef;
		coef_23 = coef_23 * XCC_DEFAULT / max_coef;
	} else {
		coef_01 = XCC_DEFAULT;
		coef_12 = XCC_DEFAULT;
		coef_23 = XCC_DEFAULT;
	}
	set_reg(xcc_base + LCP_XCC_COEF_01,  coef_01, 17, 0);
	set_reg(xcc_base + LCP_XCC_COEF_12,  coef_12, 17, 0);
	set_reg(xcc_base + LCP_XCC_COEF_23,  coef_23, 17, 0);

	for (cnt = 0; cnt < XCC_COEF_LEN; cnt++) {
		if (cnt == XCC_COEF_INDEX_01 || cnt == XCC_COEF_INDEX_12 || cnt == XCC_COEF_INDEX_23) {
		} else {
			set_reg(xcc_base + LCP_XCC_COEF_00 +cnt * 4,  0x0, 17, 0);
		}
	}
}


void clear_xcc_table(struct dpu_fb_data_type *dpufd)
{
	char __iomem *xcc_base = NULL;
	int cnt;

	if (dpufd == NULL) {
		DPU_FB_ERR("dpufd is null!\n");
		return;
	}

	xcc_base = dpufd->dss_base + DSS_DPP_XCC_OFFSET;

	for (cnt = 0; cnt < XCC_COEF_LEN; cnt++) {
		xcc_table_temp[cnt] = inp32(xcc_base + LCP_XCC_COEF_00 +cnt * 4);//lint !e679
	}

	//bypass xcc
	reset_xcc_reg(xcc_base);
	xcc_enable_state = inp32(xcc_base + LCP_XCC_BYPASS_EN);
}

void restore_xcc_table(struct dpu_fb_data_type *dpufd)
{
	int cnt;
	char __iomem *xcc_base = NULL;
	struct lcp_info *lcp_param = NULL;

	if (dpufd == NULL) {
		DPU_FB_ERR("dpufd is null!\n");
		return;
	}

	xcc_base = dpufd->dss_base + DSS_DPP_XCC_OFFSET;
	lcp_param = &(dpufd->effect_info[dpufd->panel_info.disp_panel_id].lcp);

	if (xcc_base == NULL) {
		DPU_FB_DEBUG("lcp_base is NULL!\n");
		return;
	}

	if (lcp_param == NULL) {
		DPU_FB_ERR("lcp_param is null!\n");
		return;
	}

	if (lcp_param->xcc_table == NULL) {
		DPU_FB_DEBUG("xcc_table is NULL!\n");
		return;
	}

	//enable xcc
	if (!spin_is_locked(&g_xcc_effect_lock)) {
		spin_lock(&g_xcc_effect_lock);
		if (!dpufd->effect_updated_flag[dpufd->panel_info.disp_panel_id].xcc_effect_updated) {
			for (cnt = 0; cnt < XCC_COEF_LEN; cnt++) {
				lcp_param->xcc_table[cnt] = xcc_table_temp[cnt];
			}
			lcp_xcc_set_reg(xcc_base, lcp_param);
			set_reg(xcc_base + LCP_XCC_BYPASS_EN, xcc_enable_state & 0x1, 1, 0);
		} else {
			lcp_xcc_set_reg(xcc_base, lcp_param);
			set_reg(xcc_base + LCP_XCC_BYPASS_EN, ~(lcp_param->xcc_enable & 0x1), 1, 0);
			dpufd->effect_updated_flag[dpufd->panel_info.disp_panel_id].xcc_effect_updated = FALSE;
		}
		spin_unlock(&g_xcc_effect_lock);
	} else {
		DPU_FB_INFO("xcc effect param is being updated, delay set reg to next frame!\n");
	}

}

static bool lcp_gmp_set_reg(char __iomem *gmp_lut_base, struct lcp_info *lcp_param)
{
	int i;

	if (gmp_lut_base == NULL) {
		DPU_FB_DEBUG("lcp_lut_base is NULL!\n");
		return false;
	}

	if (lcp_param == NULL) {
		DPU_FB_DEBUG("lcp_param is NULL!\n");
		return false;
	}
	if (lcp_param->gmp_table_low32 == NULL || lcp_param->gmp_table_high4 == NULL) {
		DPU_FB_DEBUG("gmp_table_low32 or gmp_table_high4 is NULL!\n");
		return false;
	}
	for (i = 0; i < GMP_COFE_CNT; i++) {
		set_reg(gmp_lut_base + i * 2 * 4, lcp_param->gmp_table_low32[i], 32, 0);
		set_reg(gmp_lut_base + i * 2 * 4 + 4, lcp_param->gmp_table_high4[i], 4, 0);
	}
	return true;
}

void dpu_effect_lcp_set_reg(struct dpu_fb_data_type *dpufd)
{
	struct dss_effect *effect = NULL;
	struct lcp_info *lcp_param = NULL;
	char __iomem *xcc_base = NULL;
	char __iomem *gmp_base = NULL;
	char __iomem *degamma_base = NULL;
	char __iomem *gmp_lut_base = NULL;
	char __iomem *degamma_lut_base = NULL;
	char __iomem *lcp_base = NULL;
	uint32_t degama_lut_sel;
	uint32_t gmp_lut_sel;
	bool ret = false;

	if (NULL == dpufd) {
		DPU_FB_ERR("dpufd is NULL!");
		return;
	}

	if (!g_is_effect_lock_init) {
		DPU_FB_INFO("display effect lock is not init!\n");
		return;
	}

	effect = &dpufd->effect_ctl;

	lcp_base = dpufd->dss_base + DSS_DPP_LCP_OFFSET;
	xcc_base = dpufd->dss_base + DSS_DPP_XCC_OFFSET;
	gmp_base = dpufd->dss_base + DSS_DPP_GMP_OFFSET;
	degamma_base = dpufd->dss_base + DSS_DPP_DEGAMMA_OFFSET;
	degamma_lut_base = dpufd->dss_base + DSS_DPP_DEGAMMA_LUT_OFFSET;
	gmp_lut_base = dpufd->dss_base + DSS_DPP_GMP_LUT_OFFSET;

	lcp_param = &(dpufd->effect_info[dpufd->panel_info.disp_panel_id].lcp);

	//Update De-Gamma LUT
	if (effect->lcp_igm_support && dpufd->effect_updated_flag[dpufd->panel_info.disp_panel_id].igm_effect_updated) {
		if (!spin_is_locked(&g_igm_effect_lock)) {
			spin_lock(&g_igm_effect_lock);
			ret = lcp_igm_set_reg(degamma_lut_base, lcp_param);
			//Enable De-Gamma
			if (ret) {
				degama_lut_sel = inp32(degamma_base + DEGAMA_LUT_SEL);
				set_reg(degamma_base + DEGAMA_LUT_SEL, (~(degama_lut_sel & 0x1)) & 0x1, 1, 0);
				//set_reg(degamma_base + DEGAMA_EN,  lcp_param->igm_enable, 1, 0);
			}
			dpufd->effect_updated_flag[dpufd->panel_info.disp_panel_id].igm_effect_updated = false;
			spin_unlock(&g_igm_effect_lock);
		} else {
			DPU_FB_INFO("igm effect param is being updated, delay set reg to next frame!\n");
		}
	}

	//Update XCC Coef
	if (effect->lcp_xcc_support && dpufd->effect_updated_flag[dpufd->panel_info.disp_panel_id].xcc_effect_updated
		&& !dpufd->mask_layer_xcc_flag && dpufd->dirty_region_updt_enable == 0) {
		if (!spin_is_locked(&g_xcc_effect_lock)) {
			spin_lock(&g_xcc_effect_lock);
			ret = lcp_xcc_set_reg(xcc_base, lcp_param);
			//Enable XCC
			if (ret) {
				set_reg(xcc_base + LCP_XCC_BYPASS_EN,	~(lcp_param->xcc_enable), 1, 0);
				//set_reg(xcc_base + LCP_XCC_BYPASS_EN,	lcp_param->xcc_enable, 1, 0);
				//Enable XCC pre
				//set_reg(xcc_base + XCC_EN,  lcp_param->xcc_enable, 1, 1);
				dpufd->panel_info.dc_switch_xcc_updated =
					dpufd->de_info.amoled_param.dc_brightness_dimming_enable_real !=
					dpufd->de_info.amoled_param.dc_brightness_dimming_enable ||
					(dpufd->de_info.amoled_param.amoled_diming_enable !=
					dpufd->de_info.amoled_param.amoled_enable_from_hal);
			}
			dpufd->effect_updated_flag[dpufd->panel_info.disp_panel_id].xcc_effect_updated = false;
			spin_unlock(&g_xcc_effect_lock);
		} else {
			DPU_FB_INFO("xcc effect param is being updated, delay set reg to next frame!\n");
		}
	}

	//Update GMP LUT
	if (effect->lcp_gmp_support && dpufd->effect_updated_flag[dpufd->panel_info.disp_panel_id].gmp_effect_updated) {
		if (!spin_is_locked(&g_gmp_effect_lock)) {
			spin_lock(&g_gmp_effect_lock);
			ret = lcp_gmp_set_reg(gmp_lut_base, lcp_param);
			//Enable GMP
			if (ret) {
				gmp_lut_sel = inp32(gmp_base + GMP_LUT_SEL);
				set_reg(gmp_base + GMP_LUT_SEL, (~(gmp_lut_sel & 0x1)) & 0x1, 1, 0);
				set_reg(gmp_base + GMP_EN, lcp_param->gmp_enable, 1, 0);
			}
			dpufd->effect_updated_flag[dpufd->panel_info.disp_panel_id].gmp_effect_updated = false;
			spin_unlock(&g_gmp_effect_lock);
		} else {
			DPU_FB_INFO("gmp effect param is being updated, delay set reg to next frame!\n");
		}
	}

	//free_lcp_table(lcp_param);
	return;
}

void dpu_effect_gamma_set_reg(struct dpu_fb_data_type *dpufd)
{
	struct gamma_info *gamma_param = NULL;
	char __iomem *gamma_base = NULL;
	char __iomem *gamma_lut_base = NULL;
	char __iomem *gamma_pre_lut_base = NULL;
	int cnt = 0;
	uint32_t gama_lut_sel;

	if (NULL == dpufd) {
		DPU_FB_ERR("dpufd is NULL!");
		return;
	}

	if (!g_is_effect_lock_init) {
		DPU_FB_INFO("display effect lock is not init!\n");
		return;
	}

	if (!dpufd->effect_ctl.gamma_support) {
		return;
	}

	if (!dpufd->effect_updated_flag[dpufd->panel_info.disp_panel_id].gamma_effect_updated) {
		return;
	}

	gamma_base = dpufd->dss_base + DSS_DPP_GAMA_OFFSET;
	gamma_lut_base = dpufd->dss_base + DSS_DPP_GAMA_LUT_OFFSET;
	gamma_pre_lut_base = dpufd->dss_base + DSS_DPP_GAMA_PRE_LUT_OFFSET;

	gamma_param = &(dpufd->effect_info[dpufd->panel_info.disp_panel_id].gamma);

	if (spin_is_locked(&g_gamma_effect_lock)) {
	    DPU_FB_INFO("gamma effect param is being updated, delay set reg to next frame!\n");
	    return;
	}
	spin_lock(&g_gamma_effect_lock);

	if ((NULL == gamma_param->gamma_r_table) ||
		(NULL == gamma_param->gamma_g_table) ||
		(NULL == gamma_param->gamma_b_table)) {
		DPU_FB_INFO("fb%d, gamma table is null!\n", dpufd->index);
		goto err_ret;
	}

	//Update Gamma LUT
	for (cnt = 0; cnt < GAMMA_LUT_LEN; cnt = cnt + 2) {
		set_reg(gamma_lut_base + (U_GAMA_R_COEF + cnt * 2), gamma_param->gamma_r_table[cnt], 12,0);
		if (cnt != GAMMA_LUT_LEN - 1)
			set_reg(gamma_lut_base + (U_GAMA_R_COEF + cnt * 2), gamma_param->gamma_r_table[cnt+1], 12,16);

		set_reg(gamma_lut_base + (U_GAMA_G_COEF + cnt * 2), gamma_param->gamma_g_table[cnt], 12,0);
		if (cnt != GAMMA_LUT_LEN - 1)
			set_reg(gamma_lut_base + (U_GAMA_G_COEF + cnt * 2), gamma_param->gamma_g_table[cnt+1], 12,16);

		set_reg(gamma_lut_base + (U_GAMA_B_COEF + cnt * 2), gamma_param->gamma_b_table[cnt], 12,0);
		if (cnt != GAMMA_LUT_LEN - 1)
			set_reg(gamma_lut_base + (U_GAMA_B_COEF + cnt * 2), gamma_param->gamma_b_table[cnt+1], 12,16);
	}
	gama_lut_sel = inp32(gamma_base + GAMA_LUT_SEL);
	set_reg(gamma_base + GAMA_LUT_SEL, (~(gama_lut_sel & 0x1)) & 0x1, 1, 0);

	//Enable Gamma
	set_reg(gamma_base + GAMA_EN,  gamma_param->enable, 1, 0);

err_ret:
	dpufd->effect_updated_flag[dpufd->panel_info.disp_panel_id].gamma_effect_updated = false;
	//free_gamma_table(gamma_param);

	spin_unlock(&g_gamma_effect_lock);

	return;
}

/*******************************************************************************
**ACM REG DIMMING
*/
/*lint -e866*/
static inline uint32_t cal_hue_value(uint32_t cur,uint32_t target,int count) {
	if (count <= 0) {
		return cur;
	}

	if (abs((int)(cur - target)) > HUE_REG_RANGE) {
		if (cur > target) {
			target += HUE_REG_OFFSET;
		} else {
			cur += HUE_REG_OFFSET;
		}
	}

	if (target > cur) {
		cur += (target - cur) / (uint32_t)count;
	} else if (cur > target) {
		cur -= (cur - target) / (uint32_t)count;
	}

	cur %= HUE_REG_OFFSET;

	return cur;
}

static inline uint32_t cal_sata_value(uint32_t cur,uint32_t target,int count) {
	if (count <= 0) {
		return cur;
	}

	if (target > cur) {
		cur += (target - cur) / (uint32_t)count;
	} else if (cur > target) {
		cur -= (cur - target) / (uint32_t)count;
	}

	return cur;
}

/*lint -e838*/
static inline int satr_unint32_to_int(uint32_t input) {
	int result = 0;

	if (input >= SATR_REG_RANGE) {
		result = (int)input - SATR_REG_OFFSET;
	} else {
		result = (int)input;
	}
	return result;
}

/*lint -e838*/
static inline uint32_t cal_satr_value(uint32_t cur,uint32_t target,int count) {
	int i_cur = 0;
	int i_target = 0;

	if (count <= 0) {
		return cur;
	}

	i_cur = satr_unint32_to_int(cur);
	i_target = satr_unint32_to_int(target);

	return (uint32_t)(i_cur + (i_target - i_cur) / count) % SATR_REG_OFFSET;
}

/*lint -e838*/
static inline void cal_cur_acm_reg(int count,acm_reg_t *cur_acm_reg,acm_reg_table_t *target_acm_reg) {
	int index = 0;

	if (NULL == cur_acm_reg || NULL == target_acm_reg) {
		return;
	}

	for (index = 0;index < ACM_HUE_SIZE;index++) {
		cur_acm_reg->acm_lut_hue_table[index] = cal_hue_value(cur_acm_reg->acm_lut_hue_table[index],target_acm_reg->acm_lut_hue_table[index],count);
	}
	for (index = 0;index < ACM_SATA_SIZE;index++) {
		cur_acm_reg->acm_lut_sata_table[index] = cal_sata_value(cur_acm_reg->acm_lut_sata_table[index],target_acm_reg->acm_lut_sata_table[index],count);
	}
	for (index = 0;index < ACM_SATR_SIZE;index++) {
		cur_acm_reg->acm_lut_satr0_table[index] = cal_satr_value(cur_acm_reg->acm_lut_satr0_table[index],target_acm_reg->acm_lut_satr0_table[index],count);
		cur_acm_reg->acm_lut_satr1_table[index] = cal_satr_value(cur_acm_reg->acm_lut_satr1_table[index],target_acm_reg->acm_lut_satr1_table[index],count);
		cur_acm_reg->acm_lut_satr2_table[index] = cal_satr_value(cur_acm_reg->acm_lut_satr2_table[index],target_acm_reg->acm_lut_satr2_table[index],count);
		cur_acm_reg->acm_lut_satr3_table[index] = cal_satr_value(cur_acm_reg->acm_lut_satr3_table[index],target_acm_reg->acm_lut_satr3_table[index],count);
		cur_acm_reg->acm_lut_satr4_table[index] = cal_satr_value(cur_acm_reg->acm_lut_satr4_table[index],target_acm_reg->acm_lut_satr4_table[index],count);
		cur_acm_reg->acm_lut_satr5_table[index] = cal_satr_value(cur_acm_reg->acm_lut_satr5_table[index],target_acm_reg->acm_lut_satr5_table[index],count);
		cur_acm_reg->acm_lut_satr6_table[index] = cal_satr_value(cur_acm_reg->acm_lut_satr6_table[index],target_acm_reg->acm_lut_satr6_table[index],count);
		cur_acm_reg->acm_lut_satr7_table[index] = cal_satr_value(cur_acm_reg->acm_lut_satr7_table[index],target_acm_reg->acm_lut_satr7_table[index],count);
	}
}

/*lint -e838*/
void dpu_effect_color_dimming_acm_reg_set(struct dpu_fb_data_type *dpufd, acm_reg_t *cur_acm_reg) {
	acm_reg_table_t target_acm_reg = {0};
	struct dpu_panel_info *pinfo = NULL;

	if (NULL == dpufd) {
		DPU_FB_ERR("dpufd is NULL!");
		return;
	}

	if (NULL == cur_acm_reg) {
		DPU_FB_ERR("cur_acm_reg is NULL!");
		return;
	}

	pinfo = &(dpufd->panel_info);

	if (SCENE_MODE_GALLERY == dpufd->user_scene_mode || SCENE_MODE_DEFAULT == dpufd->user_scene_mode) {
		target_acm_reg.acm_lut_hue_table = pinfo->acm_lut_hue_table;
		target_acm_reg.acm_lut_sata_table = pinfo->acm_lut_sata_table;
		target_acm_reg.acm_lut_satr0_table = pinfo->acm_lut_satr0_table;
		target_acm_reg.acm_lut_satr1_table = pinfo->acm_lut_satr1_table;
		target_acm_reg.acm_lut_satr2_table = pinfo->acm_lut_satr2_table;
		target_acm_reg.acm_lut_satr3_table = pinfo->acm_lut_satr3_table;
		target_acm_reg.acm_lut_satr4_table = pinfo->acm_lut_satr4_table;
		target_acm_reg.acm_lut_satr5_table = pinfo->acm_lut_satr5_table;
		target_acm_reg.acm_lut_satr6_table = pinfo->acm_lut_satr6_table;
		target_acm_reg.acm_lut_satr7_table = pinfo->acm_lut_satr7_table;
	} else {
		target_acm_reg.acm_lut_hue_table = pinfo->video_acm_lut_hue_table;
		target_acm_reg.acm_lut_sata_table = pinfo->video_acm_lut_sata_table;
		target_acm_reg.acm_lut_satr0_table = pinfo->video_acm_lut_satr0_table;
		target_acm_reg.acm_lut_satr1_table = pinfo->video_acm_lut_satr1_table;
		target_acm_reg.acm_lut_satr2_table = pinfo->video_acm_lut_satr2_table;
		target_acm_reg.acm_lut_satr3_table = pinfo->video_acm_lut_satr3_table;
		target_acm_reg.acm_lut_satr4_table = pinfo->video_acm_lut_satr4_table;
		target_acm_reg.acm_lut_satr5_table = pinfo->video_acm_lut_satr5_table;
		target_acm_reg.acm_lut_satr6_table = pinfo->video_acm_lut_satr6_table;
		target_acm_reg.acm_lut_satr7_table = pinfo->video_acm_lut_satr7_table;
	}

	cal_cur_acm_reg(dpufd->dimming_count,cur_acm_reg,&target_acm_reg);
}

/*lint -e838*/
void dpu_effect_color_dimming_acm_reg_init(struct dpu_fb_data_type *dpufd) {//add dimming reg init
	struct dpu_panel_info *pinfo = NULL;
	acm_reg_t *cur_acm_reg = NULL;
	int index = 0;

	if (NULL == dpufd) {
		DPU_FB_ERR("dpufd, null pointer warning.\n");
		return;
	}

	pinfo = &(dpufd->panel_info);

	cur_acm_reg = &(dpufd->acm_reg);

	if (SCENE_MODE_GALLERY == dpufd->user_scene_mode || SCENE_MODE_DEFAULT == dpufd->user_scene_mode) {
		for (index = 0;index < ACM_HUE_SIZE;index++) {
			cur_acm_reg->acm_lut_hue_table[index] = pinfo->acm_lut_hue_table[index];
		}
		for (index = 0;index < ACM_SATA_SIZE;index++) {
			cur_acm_reg->acm_lut_sata_table[index] = pinfo->acm_lut_sata_table[index];
		}
		for (index = 0;index < ACM_SATR_SIZE;index++) {
			cur_acm_reg->acm_lut_satr0_table[index] = pinfo->acm_lut_satr0_table[index];
			cur_acm_reg->acm_lut_satr1_table[index] = pinfo->acm_lut_satr1_table[index];
			cur_acm_reg->acm_lut_satr2_table[index] = pinfo->acm_lut_satr2_table[index];
			cur_acm_reg->acm_lut_satr3_table[index] = pinfo->acm_lut_satr3_table[index];
			cur_acm_reg->acm_lut_satr4_table[index] = pinfo->acm_lut_satr4_table[index];
			cur_acm_reg->acm_lut_satr5_table[index] = pinfo->acm_lut_satr5_table[index];
			cur_acm_reg->acm_lut_satr6_table[index] = pinfo->acm_lut_satr6_table[index];
			cur_acm_reg->acm_lut_satr7_table[index] = pinfo->acm_lut_satr7_table[index];
		}
	} else {
		for (index = 0;index < ACM_HUE_SIZE;index++) {
			cur_acm_reg->acm_lut_hue_table[index] = pinfo->video_acm_lut_hue_table[index];
		}
		for (index = 0;index < ACM_SATA_SIZE;index++) {
			cur_acm_reg->acm_lut_sata_table[index] = pinfo->video_acm_lut_sata_table[index];
		}
		for (index = 0;index < ACM_SATR_SIZE;index++) {
			cur_acm_reg->acm_lut_satr0_table[index] = pinfo->video_acm_lut_satr0_table[index];
			cur_acm_reg->acm_lut_satr1_table[index] = pinfo->video_acm_lut_satr1_table[index];
			cur_acm_reg->acm_lut_satr2_table[index] = pinfo->video_acm_lut_satr2_table[index];
			cur_acm_reg->acm_lut_satr3_table[index] = pinfo->video_acm_lut_satr3_table[index];
			cur_acm_reg->acm_lut_satr4_table[index] = pinfo->video_acm_lut_satr4_table[index];
			cur_acm_reg->acm_lut_satr5_table[index] = pinfo->video_acm_lut_satr5_table[index];
			cur_acm_reg->acm_lut_satr6_table[index] = pinfo->video_acm_lut_satr6_table[index];
			cur_acm_reg->acm_lut_satr7_table[index] = pinfo->video_acm_lut_satr7_table[index];
		}
	}
}

static void dpu_effect_arsr2p_update(struct dpu_fb_data_type *dpufd, struct dss_arsr2p *arsr2p)
{
	int disp_panel_id = dpufd->panel_info.disp_panel_id;

	if (dpufd->effect_info[disp_panel_id].arsr2p[0].update == 1) {
		memcpy(&(arsr2p->arsr2p_effect), &(dpufd->effect_info[disp_panel_id].arsr2p[0]), sizeof(struct arsr2p_info));
		dpufd->effect_info[disp_panel_id].arsr2p[0].update = 0;
	}

	if (dpufd->effect_info[disp_panel_id].arsr2p[1].update == 1) {
		memcpy(&(arsr2p->arsr2p_effect_scale_up), &(dpufd->effect_info[disp_panel_id].arsr2p[1]), sizeof(struct arsr2p_info));
		dpufd->effect_info[disp_panel_id].arsr2p[1].update = 0;
	}

	if (dpufd->effect_info[disp_panel_id].arsr2p[2].update == 1) {
		memcpy(&(arsr2p->arsr2p_effect_scale_down), &(dpufd->effect_info[disp_panel_id].arsr2p[2]), sizeof(struct arsr2p_info));
		dpufd->effect_info[disp_panel_id].arsr2p[2].update = 0;
	}
}


int dpu_effect_arsr2p_config(struct arsr2p_info *arsr2p_effect_dst, int ih_inc, int iv_inc)
{
	struct dpu_fb_data_type *dpufd_primary = NULL;
	struct dss_arsr2p *arsr2p = NULL;

	dpufd_primary = dpufd_list[PRIMARY_PANEL_IDX];
	if (NULL == dpufd_primary) {
		DPU_FB_ERR("dpufd_primary is NULL pointer, return!\n");
		return -EINVAL;
	}

	if (NULL == arsr2p_effect_dst) {
		DPU_FB_ERR("arsr2p_effect_dst is NULL pointer, return!\n");
		return -EINVAL;
	}

	arsr2p = &(dpufd_primary->dss_module_default.arsr2p[DSS_RCHN_V1]);
	dpu_effect_arsr2p_update(dpufd_primary, arsr2p);

	if ((ih_inc == ARSR2P_INC_FACTOR) && (iv_inc == ARSR2P_INC_FACTOR)) {
		memcpy(arsr2p_effect_dst, &(arsr2p->arsr2p_effect), sizeof(struct arsr2p_info));
	} else if ((ih_inc < ARSR2P_INC_FACTOR) || (iv_inc < ARSR2P_INC_FACTOR)) {
		memcpy(arsr2p_effect_dst, &(arsr2p->arsr2p_effect_scale_up), sizeof(struct arsr2p_info));
	} else {
		memcpy(arsr2p_effect_dst, &(arsr2p->arsr2p_effect_scale_down), sizeof(struct arsr2p_info));
	}

	return 0;
}
int dpufb_ce_service_enable_hiace(struct fb_info *info, const void __user *argp)
{
	struct dpu_panel_info *pinfo = NULL;
	dss_display_effect_ce_t *ce_ctrl = NULL;
	dss_ce_info_t *ce_info = NULL;
	struct dpu_fb_data_type *dpufd = NULL;
	dss_display_effect_metadata_t *metadata_ctrl = NULL;
	struct hiace_enable_set enable_set = { 0 };
	int ret;
	int mode;
	if (NULL == info) {
		DPU_FB_ERR("info is NULL\n");
		return -EINVAL;
	}
	if (NULL == argp) {
		DPU_FB_ERR("[effect] argp is NULL\n");
		return -EINVAL;
	}
	dpufd = (struct dpu_fb_data_type *)info->par;
	if (NULL == dpufd) {
		DPU_FB_ERR("[effect] dpufd is NULL\n");
		return -EINVAL;
	}
	pinfo = &(dpufd->panel_info);
	if (pinfo->hiace_support == 0) {
		effect_debug_log(DEBUG_EFFECT_ENTRY, "[effect] HIACE is not supported!\n");
		return -1;
	}
	if (dpufd->index == PRIMARY_PANEL_IDX) {
		ce_ctrl = &(dpufd->ce_ctrl);
		ce_info = &(dpufd->hiace_info[dpufd->panel_info.disp_panel_id]);
		metadata_ctrl = &(dpufd->metadata_ctrl);
	} else {
		DPU_FB_ERR("[effect] fb%d, not support!", dpufd->index);
		return -1;
	}
	ret = (int)copy_from_user(&enable_set, argp, sizeof(enable_set));
	if (ret) {
		DPU_FB_ERR("[effect] arg is invalid");
		return -EINVAL;
	}
	mode = enable_set.enable;
	if (mode < 0) {
		mode = 0;
	} else if (mode >= CE_MODE_COUNT) {
		mode = CE_MODE_COUNT - 1;
	}
	if (mode != ce_ctrl->ctrl_ce_mode) {
		mutex_lock(&(ce_ctrl->ctrl_lock));
		ce_ctrl->ctrl_ce_mode = mode;
		mutex_unlock(&(ce_ctrl->ctrl_lock));
		if (mode == CE_MODE_DISABLE) {
			ce_info->gradual_frames = EFFECT_GRADUAL_REFRESH_FRAMES;
			ce_info->to_stop_hdr = true;
		}
		enable_hiace(dpufd, mode);
	}
	return 0;
}
int dpufb_get_reg_val(struct fb_info *info, void __user *argp) {
	struct dpu_fb_data_type *dpufd = NULL;
	struct dpu_panel_info *pinfo = NULL;
	struct dss_reg reg;
	uint32_t addr;
	int ret = 0;
	if (NULL == info) {
		DPU_FB_ERR("info is NULL\n");
		return -EINVAL;
	}
	if (NULL == argp) {
		DPU_FB_ERR("[effect] argp is NULL\n");
		return -EINVAL;
	}
	dpufd = (struct dpu_fb_data_type *)info->par;
	if (NULL == dpufd) {
		DPU_FB_ERR("[effect] dpufd is NULL\n");
		return -EINVAL;
	}

	pinfo = &(dpufd->panel_info);
	if (!pinfo->hiace_support) {
		effect_debug_log(DEBUG_EFFECT_ENTRY, "[effect] Don't support HIACE\n");
		return -EINVAL;
	}
	effect_debug_log(DEBUG_EFFECT_FRAME, "[effect] step in\n");
	ret = (int)copy_from_user(&reg, argp, sizeof(struct dss_reg));
	if (ret) {
		DPU_FB_ERR("[effect] copy_from_user(param) failed! ret=%d.\n", ret);
		return -2;
	}

	switch(reg.tag) {
		//case TAG_ARSR_1P_ENABLE:
		//	addr = DSS_POST_SCF_OFFSET + ARSR_POST_MODE;
		//	break;
		case TAG_LCP_XCC_ENABLE:
			addr = DSS_DPP_XCC_OFFSET + LCP_XCC_BYPASS_EN;
			break;
		case TAG_LCP_GMP_ENABLE:
			addr = DSS_DPP_GMP_OFFSET + GMP_EN;
			break;
		case TAG_LCP_IGM_ENABLE:
			addr = DSS_DPP_DEGAMMA_OFFSET + DEGAMA_EN;
			break;
		case TAG_GAMMA_ENABLE:
			addr = DSS_DPP_GAMA_OFFSET + GAMA_EN;
			break;
		case TAG_HIACE_LHIST_SFT:
			addr = DSS_HI_ACE_OFFSET + DPE_LHIST_SFT;
			break;
		default:
			DPU_FB_ERR("[effect] invalid tag : %u", reg.tag);
			return -EINVAL;
	}

	down(&dpufd->blank_sem);
	if (dpufd->panel_power_on == false) {
		DPU_FB_ERR("[effect] panel power off\n");
		up(&dpufd->blank_sem);
		return -EINVAL;
	}
	dpufb_activate_vsync(dpufd);

	reg.value = (uint32_t)inp32(dpufd->dss_base + addr);

	dpufb_deactivate_vsync(dpufd);
	up(&dpufd->blank_sem);

	ret = (int)copy_to_user(argp, &reg, sizeof(struct dss_reg));
	if (ret) {
		DPU_FB_ERR("[effect] copy_to_user failed(param)! ret=%d.\n", ret);
		ret = -EINVAL;
	}
	return 0;
}

int dpufb_ce_service_set_param(struct fb_info *info, const void __user *argp) {
	struct dpu_fb_data_type *dpufd = NULL;
	struct dpu_panel_info *pinfo = NULL;
	int ret = 0;
	if (NULL == info) {
		DPU_FB_ERR("[effect] info is NULL\n");
		return -EINVAL;
	}
	if (NULL == argp) {
		DPU_FB_ERR("[effect] argp is NULL\n");
		return -EINVAL;
	}
	dpufd = (struct dpu_fb_data_type *)info->par;
	if (NULL == dpufd) {
		DPU_FB_ERR("[effect] dpufd is NULL\n");
		return -EINVAL;
	}
	pinfo = &(dpufd->panel_info);
	if (!pinfo->hiace_support) {
		effect_debug_log(DEBUG_EFFECT_ENTRY, "[effect] Don't support HIACE\n");
		return -EINVAL;
	}
	effect_debug_log(DEBUG_EFFECT_FRAME, "[effect] step in\n");
	ret = (int)copy_from_user(&(dpufd->effect_info[pinfo->disp_panel_id].hiace), argp, sizeof(struct hiace_info));
	if (ret) {
		DPU_FB_ERR("[effect] copy_from_user(param) failed! ret=%d.\n", ret);
		return -2;
	}

	dpufd->effect_updated_flag[pinfo->disp_panel_id].hiace_effect_updated = true;
	return ret;
}

static int set_hiace_param(struct dpu_fb_data_type *dpufd) {
	char __iomem *hiace_base = NULL;
	int disp_panel_id;

	if (dpufd == NULL) {
		DPU_FB_DEBUG("dpufd is NULL!\n");
		return -1;
	}
	if (dpufd->index == PRIMARY_PANEL_IDX) {
		hiace_base = dpufd->dss_base + DSS_HI_ACE_OFFSET;
	} else {
		DPU_FB_DEBUG("[effect] fb%d, not support!", dpufd->index);
		return 0;
	}

	if(!check_underflow_clear_ack(dpufd)){
		DPU_FB_ERR("[effect] fb%d, clear_ack is not return!\n", dpufd->index);
		return -1;
	}

	disp_panel_id = dpufd->panel_info.disp_panel_id;

	if (dpufd->effect_updated_flag[disp_panel_id].hiace_effect_updated) {
		set_reg(hiace_base + HIACE_HALF_BLOCK_H_W, dpufd->effect_info[disp_panel_id].hiace.half_block_info, 32, 0);
		set_reg(hiace_base + HIACE_XYWEIGHT, dpufd->effect_info[disp_panel_id].hiace.xyweight, 32, 0);
		set_reg(hiace_base + HIACE_LHIST_SFT, dpufd->effect_info[disp_panel_id].hiace.lhist_sft, 32, 0);
		set_reg(hiace_base + HIACE_HUE, dpufd->effect_info[disp_panel_id].hiace.hue, 32, 0);
		set_reg(hiace_base + HIACE_SATURATION, dpufd->effect_info[disp_panel_id].hiace.saturation, 32, 0);
		set_reg(hiace_base + HIACE_VALUE, dpufd->effect_info[disp_panel_id].hiace.value, 32, 0);
		set_reg(hiace_base + HIACE_SKIN_GAIN, dpufd->effect_info[disp_panel_id].hiace.skin_gain, 32, 0);
		set_reg(hiace_base + HIACE_UP_LOW_TH, dpufd->effect_info[disp_panel_id].hiace.up_low_th, 32, 0);
		set_reg(hiace_base + HIACE_UP_CNT, dpufd->effect_info[disp_panel_id].hiace.up_cnt, 32, 0);
		set_reg(hiace_base + HIACE_LOW_CNT, dpufd->effect_info[disp_panel_id].hiace.low_cnt, 32, 0);
		set_reg(hiace_base + HIACE_GAMMA_EN, dpufd->effect_info[disp_panel_id].hiace.gamma_w, 32, 0);
		set_reg(hiace_base + HIACE_GAMMA_EN_HV_R, dpufd->effect_info[disp_panel_id].hiace.gamma_r, 32, 0);
	}
	dpufd->effect_updated_flag[disp_panel_id].hiace_effect_updated = false;
	return 0;
}

int dpu_effect_hiace_config(struct dpu_fb_data_type *dpufd) {
	return set_hiace_param(dpufd);
}

void deinit_effect(struct dpu_fb_data_type *dpufd)
{
	void_unused(dpufd);
}

/*lint +e571, +e573, +e737, +e732, +e850, +e730, +e713, +e574, +e679, +e732, +e845, +e570, +e774*/
//lint +e747, +e838
