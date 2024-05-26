/*
 * lcd_kit_displayengine.c
 *
 * display engine common function in lcd
 *
 * Copyright (c) 2021-2022 Huawei Technologies Co., Ltd.
 *
 *
 */

#include <linux/backlight.h>
#include <linux/device.h>
#include <linux/workqueue.h>
#ifndef CONFIG_QCOM_PLATFORM_8425
#include <sde_drm.h>
#endif
#include <huawei_platform/fingerprint_interface/fingerprint_interface.h>

extern void ud_fp_on_hbm_completed(void);

#include "lcd_kit_displayengine.h"
#include "lcd_kit_adapt.h"
#include "lcd_kit_common.h"
#include "lcd_kit_utils.h"
#include "display_engine_kernel.h"
#include "lcd_kit_drm_panel.h"
#include "sde_trace.h"

#if defined(CONFIG_DPU_FB_V501) || defined(CONFIG_DPU_FB_V330) || \
	defined(CONFIG_DPU_FB_V350) || defined(CONFIG_DPU_FB_V510) || \
	defined(CONFIG_DPU_FB_V600)
#include "lcd_kit_disp.h"
#define CONFIG_DE_XCC 1
#else
/* The same define with lcd_kit_disp.h */
#define LCD_KIT_ENABLE_ELVSSDIM  0
#define LCD_KIT_DISABLE_ELVSSDIM 1
#define LCD_KIT_ELVSSDIM_NO_WAIT 0
#endif

#define BACKLIGHT_HIGH_LEVEL 1
#define BACKLIGHT_LOW_LEVEL  2
#define MAX_DBV 4095
#define FINGERPRINT_HBM_NOTIFY_WAIT_FRAME_COUNT 2
#define SN_TEXT_SIZE 128
/* DDIC deburn-in need 12 RGB weight */
#define DDIC_COMPENSATION_LUT_COUNT 12

#define LCD_KIT_ALPHA_DEFAULT 4095

struct gamma_linear_interpolation_info {
	uint32_t grayscale_before;
	uint32_t grayscale;
	uint32_t grayscale_after;
	uint32_t gamma_node_value_before;
	uint32_t gamma_node_value;
	uint32_t gamma_node_value_after;
};

/* Structure types for different features, you can add your type after de_context_drm_hist. */
/* DRM modules: */
struct de_context_drm_hist {
	bool is_enable;
	bool is_enable_change;
};

struct local_hbm_info {
	bool need_set_later;
	int last_grayscale;
	int current_grayscale;
};

/* Displayengine brightness related parameters */
struct de_context_brightness {
	unsigned int mode_set_by_ioctl;
	unsigned int mode_in_use;
	bool is_alpha_updated_at_mode_change;
	struct display_engine_brightness_lut lut[DISPLAY_ENGINE_MODE_MAX];
	unsigned int last_level_in;
	unsigned int last_level_out;
	unsigned int current_alpha;
	unsigned int last_alpha;
	bool is_alpha_bypass;
	ktime_t te_timestamp;
	ktime_t te_timestamp_last;
	u32 power_mode;
	struct local_hbm_info local_hbm;
	/* UD fingerprint backlight */
	struct display_engine_fingerprint_backlight fingerprint_backlight;
	struct display_engine_alpha_map alpha_map;
};

/* Parameters required for aging record data */
struct display_engine_compensation_aging {
	unsigned int support;
	bool is_inited;
	struct mutex lock;
	unsigned int min_hbm_dbv;
	unsigned int orig_level;
	unsigned int mipi_level;
	unsigned int fps_index;
	int last_dbi_set_ret;
	bool is_display_region_enable[DE_REGION_NUM];
	unsigned int data[DE_STATISTICS_TYPE_MAX][DE_REGION_NUM][DE_FPS_MAX];
	struct timeval last_time[DE_STATISTICS_TYPE_MAX][DE_REGION_NUM][DE_FPS_MAX];
};

/* Parameters for compensation AB */
struct display_engine_compensation_ddic_data {
	struct display_engine_roi_param roi_param;
	uint16_t dbi_weight[DE_COLOR_INDEX_COUNT][DDIC_COMPENSATION_LUT_COUNT];
};

/* Context for all display engine features, which will be instantiated for all panel IDs . */
struct display_engine_context {
	bool is_inited;
	/* DRM modules */
	struct de_context_drm_hist drm_hist;
	/* Brightness */
	struct de_context_brightness brightness;
};

/* Context for all display engine features, which not care about Panel ID . */
struct display_engine_common_context {
	/* DBV statistics */
	struct display_engine_compensation_aging aging_data;
	struct display_engine_compensation_ddic_data compensation_data;
};

static struct display_engine_context g_de_context[DISPLAY_ENGINE_PANEL_COUNT] = {
	{ /* Inner panel or primay panel */
		.is_inited = false,
		.drm_hist.is_enable = false,
		.drm_hist.is_enable_change = false,
		.brightness.mode_in_use = 0,
		.brightness.mode_set_by_ioctl = 0,
		.brightness.is_alpha_updated_at_mode_change = false,
		.brightness.last_level_in = MAX_DBV,
		.brightness.last_level_out = MAX_DBV,
		.brightness.current_alpha = ALPHA_DEFAULT,
		.brightness.last_alpha = ALPHA_DEFAULT,
		.brightness.is_alpha_bypass = false,
		.brightness.te_timestamp = 0,
		.brightness.te_timestamp_last = 0,
		.brightness.fingerprint_backlight.scene_info = 0,
		.brightness.fingerprint_backlight.hbm_level = MAX_DBV,
		.brightness.fingerprint_backlight.current_level = MAX_DBV,
		.brightness.power_mode = SDE_MODE_DPMS_OFF,
		.brightness.local_hbm.need_set_later = false,
		.brightness.local_hbm.last_grayscale = 0,
		.brightness.local_hbm.current_grayscale = 0,
	},
	{ /* Outer panel */
		.is_inited = false,
		.drm_hist.is_enable = false,
		.drm_hist.is_enable_change = false,
		.brightness.mode_in_use = 0,
		.brightness.mode_set_by_ioctl = 0,
		.brightness.is_alpha_updated_at_mode_change = false,
		.brightness.last_level_in = MAX_DBV,
		.brightness.last_level_out = MAX_DBV,
		.brightness.current_alpha = ALPHA_DEFAULT,
		.brightness.last_alpha = ALPHA_DEFAULT,
		.brightness.is_alpha_bypass = false,
		.brightness.te_timestamp = 0,
		.brightness.te_timestamp_last = 0,
		.brightness.fingerprint_backlight.scene_info = 0,
		.brightness.fingerprint_backlight.hbm_level = MAX_DBV,
		.brightness.fingerprint_backlight.current_level = MAX_DBV,
	},
};

static struct display_engine_common_context g_de_common_context = {
	/* {0, 0, 1160, 2480} is PAL primary rectangle */
	.compensation_data.roi_param.top = 0,
	.compensation_data.roi_param.bottom = 1160,
	.compensation_data.roi_param.left = 0,
	.compensation_data.roi_param.right = 2480,
};

enum hbm_gamma_index {
	GAMMA_INDEX_RED_HIGH = 0,
	GAMMA_INDEX_RED_LOW = 1,
	GAMMA_INDEX_GREEN_HIGH = 24,
	GAMMA_INDEX_GREEN_LOW = 25,
	GAMMA_INDEX_BLUE_HIGH = 48,
	GAMMA_INDEX_BLUE_LOW = 49
};

enum color_cmds_index {
	CMDS_RED_HIGH = 9,
	CMDS_RED_LOW = 10,
	CMDS_GREEN_HIGH = 11,
	CMDS_GREEN_LOW = 12,
	CMDS_BLUE_HIGH = 13,
	CMDS_BLUE_LOW = 14,
	CMDS_COLOR_MAX = 15
};

enum force_alpha_enable_state {
	NO_FORCING = -1,
	FORCE_DISABLE,
	FORCE_ENABLE
};

enum te_interval_type {
	TE_INTERVAL_60_FPS_US = 16667,
	TE_INTERVAL_90_FPS_US = 11111,
	TE_INTERVAL_120_FPS_US = 8334
};

/* The maximum number of enter_ddic_alpha commands is 16, you can change it if needed */
static const int cmd_cnt_max = 16;

/* The numbers of enter_alpha_cmds.cmds's payload, you can change them if needed */
static const int payload_num_min = 3;
static const int payload_num_max = 8;

static const int color_cmd_index = 1;

static int g_fingerprint_hbm_level = MAX_DBV;
static int g_fingerprint_backlight_type = BACKLIGHT_LOW_LEVEL;
static int g_fingerprint_backlight_type_real = BACKLIGHT_LOW_LEVEL;

static int display_engine_panel_id_to_de(enum lcd_active_panel_id lcd_id)
{
	switch (lcd_id) {
	case PRIMARY_PANEL:
		return DISPLAY_ENGINE_PANEL_INNER;
	case SECONDARY_PANEL:
		return DISPLAY_ENGINE_PANEL_OUTER;
	default:
		LCD_KIT_WARNING("unknown lcd kit panel id [%d]!\n", lcd_id);
	}
	return DISPLAY_ENGINE_PANEL_INNER;
}

static uint32_t display_engine_panel_id_to_lcdkit(uint32_t de_id)
{
	switch (de_id) {
	case DISPLAY_ENGINE_PANEL_INNER:
		return PRIMARY_PANEL;
	case DISPLAY_ENGINE_PANEL_OUTER:
		return SECONDARY_PANEL;
	default:
		LCD_KIT_WARNING("unknown display engine panel id [%d]!\n", de_id);
		break;
	}
	return PRIMARY_PANEL;
}

static struct display_engine_context *get_de_context(uint32_t panel_id)
{
	if (panel_id >= DISPLAY_ENGINE_PANEL_COUNT) {
		LCD_KIT_ERR("Invalid input: panel_id=%u\n", panel_id);
		return NULL;
	}
	return &g_de_context[panel_id];
}

uint32_t display_engine_get_hbm_level(uint32_t panel_id)
{
	struct display_engine_context *de_context = get_de_context(panel_id);

	if (!de_context) {
		LCD_KIT_ERR("de_context is null\n");
		return MAX_DBV;
	}
	return de_context->brightness.fingerprint_backlight.hbm_level;
}

#ifdef CONFIG_QCOM_MODULE_KO
static void display_engine_brightness_update_backlight(void)
{
	struct backlight_device *bd = NULL;

	bd = backlight_device_get_by_type_and_display_count(BACKLIGHT_RAW, 0);
	if (!bd) {
		LCD_KIT_ERR("bd is null\n");
		return;
	}
	SDE_ATRACE_BEGIN("backlight_update_status");
	backlight_update_status(bd);
	SDE_ATRACE_END("backlight_update_status");
}

static int display_engine_brightness_get_backlight(void)
{
	struct backlight_device *bd = NULL;

	bd = backlight_device_get_by_type_and_display_count(BACKLIGHT_RAW, 0);
	if (!bd) {
		LCD_KIT_ERR("bd is null\n");
		return MAX_DBV;
	}
	return bd->props.brightness;
}
#else
static void display_engine_brightness_update_backlight(void)
{
	struct backlight_device *bd;

	/* 0 refers to the first display */
	bd = backlight_device_get_by_type_and_display_count(BACKLIGHT_RAW, 0);
	if (!bd) {
		LCD_KIT_ERR("bd is null\n");
		return;
	}
	SDE_ATRACE_BEGIN("backlight_update_status");
	backlight_update_status(bd);
	SDE_ATRACE_END("backlight_update_status");
}

static int display_engine_brightness_get_backlight(void)
{
	struct backlight_device *bd;

	/* 0 refers to the first display */
	bd = backlight_device_get_by_type_and_display_count(BACKLIGHT_RAW, 0);
	if (!bd) {
		LCD_KIT_ERR("bd is null\n");
		return MAX_DBV;
	}
	return bd->props.brightness;
}
#endif

static void display_engine_brightness_handle_fingerprint_hbm_work(struct work_struct *work)
{
	struct qcom_panel_info *panel_info = container_of(work,
		struct qcom_panel_info, backlight_work);
	struct dsi_panel *panel = NULL;
	uint32_t panel_id;

	if (!panel_info || !panel_info->display || !panel_info->display->panel) {
		LCD_KIT_ERR("panel_info is NULL!\n");
		return;
	}

	panel = panel_info->display->panel;
	panel_id = lcd_kit_get_current_panel_id(panel);

	LCD_KIT_INFO("work start, panel_id %u, hbm_level %d, type %d\n",
		panel_id, g_fingerprint_hbm_level, g_fingerprint_backlight_type);
	display_engine_brightness_update_backlight();
	LCD_KIT_INFO("work done, hbm_level %d, type %d\n",
		g_fingerprint_hbm_level, g_fingerprint_backlight_type);
	g_fingerprint_backlight_type_real = g_fingerprint_backlight_type;
}

static void display_engine_brightness_print_power_mode(int mode)
{
	switch (mode) {
	case SDE_MODE_DPMS_ON:
		LCD_KIT_DEBUG("sde power mode [SDE_MODE_DPMS_ON]");
		break;
	case SDE_MODE_DPMS_LP1:
		LCD_KIT_DEBUG("bd->props.power [SDE_MODE_DPMS_LP1]");
		break;
	case SDE_MODE_DPMS_LP2:
		LCD_KIT_DEBUG("sde power mode [SDE_MODE_DPMS_LP2]");
		break;
	case SDE_MODE_DPMS_STANDBY:
		LCD_KIT_DEBUG("sde power mode [SDE_MODE_DPMS_STANDBY]");
		break;
	case SDE_MODE_DPMS_SUSPEND:
		LCD_KIT_DEBUG("sde power mode [SDE_MODE_DPMS_SUSPEND]");
		break;
	case SDE_MODE_DPMS_OFF:
		LCD_KIT_DEBUG("sde power mode [SDE_MODE_DPMS_OFF]");
		break;
	default:
		LCD_KIT_WARNING("sde power mode unknown value [%d]", mode);
	}
}

static int display_engine_brightness_get_power_mode(void)
{
	struct qcom_panel_info *panel_info = lcm_get_panel_info(PRIMARY_PANEL);

	if (!panel_info || !panel_info->display || !panel_info->display->panel) {
		LCD_KIT_ERR("panel_info is NULL!\n");
		return SDE_MODE_DPMS_ON;
	}
	display_engine_brightness_print_power_mode(panel_info->display->panel->power_mode);
	return panel_info->display->panel->power_mode;
}

static void display_engine_brightness_handle_vblank_work(struct work_struct *work)
{
	struct display_engine_context *de_context = get_de_context(DISPLAY_ENGINE_PANEL_INNER);
	ktime_t te_diff_ms = 0;
	static u32 vblank_count;

	(void)work;
	if (!de_context) {
		LCD_KIT_ERR("de_context is NULL\n");
		return;
	}
	/* ns to ms */
	te_diff_ms = (de_context->brightness.te_timestamp -
		de_context->brightness.te_timestamp_last) / 1000000;

	LCD_KIT_DEBUG("te: diff [%lld] ms, timestamp [%lld]\n",
		te_diff_ms, de_context->brightness.te_timestamp);

	/* Handle fingerprint hbm notification */
	if (g_fingerprint_backlight_type_real == BACKLIGHT_HIGH_LEVEL &&
		display_engine_brightness_should_do_fingerprint()) {
		vblank_count++;
		if (vblank_count == FINGERPRINT_HBM_NOTIFY_WAIT_FRAME_COUNT) {
			LCD_KIT_INFO("Fingerprint HBM notify...\n");
			ud_fp_on_hbm_completed();
		}
	} else {
		vblank_count = 0;
	}
}

static void display_engine_brightness_handle_backlight_update(struct work_struct *work)
{
	(void)work;
	display_engine_brightness_update_backlight();
}

static void display_engine_set_max_mipi_backlight(uint32_t panel_id, void *hld)
{
#if defined(CONFIG_DE_XCC)
	struct hisi_fb_data_type *hisifd = hld;
	lcd_kit_get_common_ops()->set_mipi_backlight(panel_id, hld,
		hisifd->panel_info.bl_max);
#endif
	return;
}

static void display_engine_clear_xcc(void *hld)
{
#if defined(CONFIG_DE_XCC)
	struct hisi_fb_data_type *hisifd = hld;
		/*
		 * clear XCC config(include XCC disable state)
		 * while fp is using
		 */
		hisifd->mask_layer_xcc_flag = 1;
		clear_xcc_table(hld);
#endif
	return;
}

static void display_engine_restore_xcc(void *hld)
{
#if defined(CONFIG_DE_XCC)
	struct hisi_fb_data_type *hisifd = hld;
		/*
		 * restore XCC config(include XCC enable state)
		 * while fp is end
		 */
		restore_xcc_table(hisifd);
		hisifd->mask_layer_xcc_flag = 0;
#endif
	return;
}

static int lcd_kit_set_elvss_dim_fp(uint32_t panel_id, void *hld,
	struct lcd_kit_adapt_ops *adapt_ops, int disable_elvss_dim)
{
	int ret = LCD_KIT_OK;

	if (!hld || !adapt_ops) {
		LCD_KIT_ERR("param is NULL!\n");
		return LCD_KIT_FAIL;
	}
	if (!common_info->hbm.hbm_fp_elvss_support) {
		LCD_KIT_INFO("Do not support ELVSS Dim, just return\n");
		return ret;
	}
	if (common_info->hbm.elvss_prepare_cmds.cmds) {
		if (adapt_ops->mipi_tx)
			ret = adapt_ops->mipi_tx(hld, &common_info->hbm.elvss_prepare_cmds);
	}
	if (common_info->hbm.elvss_write_cmds.cmds) {
		if (!adapt_ops->mipi_tx)
			return LCD_KIT_FAIL;
#if defined(CONFIG_DE_XCC)
		if (disable_elvss_dim != 0)
			display_engine_disable_elvss_dim_write(panel_id);
		else
			display_engine_enable_elvss_dim_write(panel_id);
#endif
		ret = adapt_ops->mipi_tx(hld, &common_info->hbm.elvss_write_cmds);
		LCD_KIT_INFO("[fp set elvss dim] send:0x%x\n",
			common_info->hbm.elvss_write_cmds.cmds[0].payload[1]);
	}
	if (common_info->hbm.elvss_post_cmds.cmds) {
		if (adapt_ops->mipi_tx)
			ret = adapt_ops->mipi_tx(hld, &common_info->hbm.elvss_post_cmds);
	}
	return ret;
}

static bool lcd_kit_fps_is_high(uint32_t panel_id)
{
#if defined(CONFIG_DE_XCC)
	return disp_info->fps.last_update_fps == LCD_KIT_FPS_HIGH;
#endif
	return false;
}

static int lcd_kit_enter_hbm_fb(uint32_t panel_id, void *hld, struct lcd_kit_adapt_ops *adapt_ops)
{
	int ret = LCD_KIT_OK;

	if (!hld || !adapt_ops) {
		LCD_KIT_ERR("param is NULL!\n");
		return LCD_KIT_FAIL;
	}

	if (lcd_kit_fps_is_high(panel_id) && common_info->dfr_info.fps_lock_command_support) {
		ret = adapt_ops->mipi_tx(hld, &common_info->dfr_info.cmds[FPS_90_HBM_NO_DIM]);
		LCD_KIT_INFO("fp enter hbm when 90hz\n");
	} else if (common_info->hbm.fp_enter_cmds.cmds) {
		if (adapt_ops->mipi_tx)
			ret = adapt_ops->mipi_tx(hld, &common_info->hbm.fp_enter_cmds);
		LCD_KIT_INFO("fp enter hbm\n");
	}
	return ret;
}

static int lcd_kit_enter_fp_hbm(uint32_t panel_id, void *hld, uint32_t level)
{
	int ret = LCD_KIT_OK;
	struct lcd_kit_adapt_ops *adapt_ops = NULL;

	if (!hld) {
		LCD_KIT_ERR("hld is NULL!\n");
		return LCD_KIT_FAIL;
	}
	adapt_ops = lcd_kit_get_adapt_ops();
	if (!adapt_ops) {
		LCD_KIT_ERR("can not register adapt_ops!\n");
		return LCD_KIT_FAIL;
	}
	if (lcd_kit_set_elvss_dim_fp(panel_id, hld, adapt_ops,
		LCD_KIT_DISABLE_ELVSSDIM) < 0)
		LCD_KIT_ERR("set_elvss_dim_fp: disable failed!\n");
	if (lcd_kit_enter_hbm_fb(panel_id, hld, adapt_ops) < 0)
		LCD_KIT_ERR("enter_hbm_fb: enable hbm failed!\n");
	if (lcd_kit_get_common_ops()->hbm_set_level(panel_id, hld, level) < 0)
		LCD_KIT_ERR("hbm_set_level: set level failed!\n");
	return ret;
}

static int lcd_kit_disable_hbm_fp(uint32_t panel_id, void *hld, struct lcd_kit_adapt_ops *adapt_ops)
{
	int ret = LCD_KIT_OK;

	if (!hld || !adapt_ops) {
		LCD_KIT_ERR("param is NULL!\n");
		return LCD_KIT_FAIL;
	}

	if (lcd_kit_fps_is_high(panel_id) &&
		common_info->dfr_info.fps_lock_command_support) {
		ret = adapt_ops->mipi_tx(hld, &common_info->dfr_info.cmds[FPS_90_NORMAL_NO_DIM]);
		LCD_KIT_INFO("fp exit hbm no dim with 90hz\n");
	} else if (common_info->hbm.exit_dim_cmds.cmds) {
		if (adapt_ops->mipi_tx)
			ret = adapt_ops->mipi_tx((void *)hld, &common_info->hbm.exit_dim_cmds);
		LCD_KIT_INFO("fp exit dim\n");
	}
	if (common_info->hbm.exit_cmds.cmds) {
		if (adapt_ops->mipi_tx)
			ret = adapt_ops->mipi_tx((void *)hld, &common_info->hbm.exit_cmds);
		LCD_KIT_INFO("fp exit hbm\n");
	}

	return ret;
}

static int lcd_kit_exit_fp_hbm(uint32_t panel_id, void *hld, uint32_t level)
{
	int ret = LCD_KIT_OK;
	struct lcd_kit_adapt_ops *adapt_ops = NULL;

	if (!hld) {
		LCD_KIT_ERR("hld is NULL!\n");
		return LCD_KIT_FAIL;
	}
	adapt_ops = lcd_kit_get_adapt_ops();
	if (level > 0) {
		if (lcd_kit_get_common_ops()->hbm_set_level(panel_id, hld, level) < 0)
			LCD_KIT_ERR("hbm_set_level: set level failed!\n");
	} else {
		if (lcd_kit_disable_hbm_fp(panel_id, hld, adapt_ops) < 0)
			LCD_KIT_ERR("disable hbm failed!\n");
	}
	if (lcd_kit_set_elvss_dim_fp(panel_id, hld, adapt_ops, LCD_KIT_ENABLE_ELVSSDIM) < 0)
		LCD_KIT_ERR("set_elvss_dim_fp: enable failed!\n");
	return ret;
}

int display_engine_set_fp_backlight(uint32_t panel_id, void *hld, u32 level, u32 backlight_type)
{
	int ret = LCD_KIT_OK;

	if (!hld) {
		LCD_KIT_ERR("hld is NULL!\n");
		return LCD_KIT_FAIL;
	}
	switch (backlight_type) {
	case BACKLIGHT_HIGH_LEVEL:
		common_info->hbm.hbm_if_fp_is_using = 1;
		LCD_KIT_DEBUG("bl_type=%d, level=%d\n", backlight_type, level);
		/*
		 * Below codes will write level to set fp backlight.
		 * We suggest set hbm_fp_support TRUE to set backlight.
		 * If hbm_fp_support(<PanelHbmFpCaptureSupport>) is 1,
		 * it will write <PanelHbmFpPrepareCommand>,<PanelHbmCommand>,
		 * PanelHbmFpPostCommand>.
		 * Those cmds are also be used by hbm backlight.
		 * Or not, set_mipi_backlight will write <PanelBacklightCommand>
		 */
		if (common_info->hbm.hbm_fp_support) {
			display_engine_set_max_mipi_backlight(panel_id, hld);
			mutex_lock(&COMMON_LOCK->hbm_lock);
			lcd_kit_enter_fp_hbm(panel_id, hld, level);
			mutex_unlock(&COMMON_LOCK->hbm_lock);
		} else {
			LCD_KIT_DEBUG("set_mipi_backlight +\n");
			ret = lcd_kit_get_common_ops()->set_mipi_backlight(panel_id, hld,
				level);
			LCD_KIT_DEBUG("set_mipi_backlight -\n");
		}
		/*
		 * The function is only used for UD PrintFinger HBM
		 * It will write <PanelFpEnterExCommand> after write fp level.
		 */
		display_engine_fp_hbm_extern(panel_id, hld, BACKLIGHT_HIGH_LEVEL);
		display_engine_clear_xcc(hld);
		break;
	case BACKLIGHT_LOW_LEVEL:
		LCD_KIT_DEBUG("bl_type=%d  level=%d\n", backlight_type, level);
		/*
		 * The function is will restore hbm level.
		 * if <PanelHbmCommand> is same as <PanelBacklightCommand>
		 * it will be cover by set_mipi_backlight.
		 * If they are different,
		 * <PanelHbmCommand> write common_info->hbm.hbm_level_current,
		 * and <PanelBacklightCommand> write level by param,
		 * <PanelBacklightCommand> usually write 51h.
		 */
		if (common_info->hbm.hbm_fp_support)
			display_engine_restore_hbm_level(panel_id, hld);
		common_info->hbm.hbm_if_fp_is_using = 0;
		LCD_KIT_DEBUG("set_mipi_backlight +\n");
		ret = lcd_kit_get_common_ops()->set_mipi_backlight(panel_id, hld, level);
		LCD_KIT_DEBUG("set_mipi_backlight -\n");
		/*
		 * The function is only used for UD PrintFinger HBM
		 * It will write <PanelFpExitExCommand> after restore normal bl.
		 */
		display_engine_fp_hbm_extern(panel_id, hld, BACKLIGHT_LOW_LEVEL);
		display_engine_restore_xcc(hld);
		break;
	default:
		LCD_KIT_ERR("bl_type is not define(%d)\n", backlight_type);
		break;
	}
	LCD_KIT_DEBUG("bl_type=%d  level=%d\n", backlight_type, level);
	return ret;
}

#if defined(CONFIG_DE_XCC)
void display_engine_disable_elvss_dim_write(uint32_t panel_id)
{
	common_info->hbm.elvss_write_cmds.cmds[0].wait =
		common_info->hbm.hbm_fp_elvss_cmd_delay;
	common_info->hbm.elvss_write_cmds.cmds[0].payload[1] =
		common_info->hbm.ori_elvss_val & LCD_KIT_DISABLE_ELVSSDIM_MASK;
}

void display_engine_enable_elvss_dim_write(uint32_t panel_id)
{
	common_info->hbm.elvss_write_cmds.cmds[0].wait =
		LCD_KIT_ELVSSDIM_NO_WAIT;
	common_info->hbm.elvss_write_cmds.cmds[0].payload[1] =
		common_info->hbm.ori_elvss_val | LCD_KIT_ENABLE_ELVSSDIM_MASK;
}
#endif

int display_engine_fp_hbm_extern(uint32_t panel_id, void *hld, int type)
{
	int ret = LCD_KIT_OK;
	struct lcd_kit_adapt_ops *adapt_ops = NULL;

	if (!hld) {
		LCD_KIT_ERR("hld is NULL!\n");
		return LCD_KIT_FAIL;
	}
	adapt_ops = lcd_kit_get_adapt_ops();
	if (!adapt_ops) {
		LCD_KIT_ERR("can not register adapt_ops!\n");
		return LCD_KIT_FAIL;
	}

	switch (type) {
	case BACKLIGHT_HIGH_LEVEL:
		if (common_ops->fp_hbm_enter_extern)
			ret = common_ops->fp_hbm_enter_extern(panel_id, hld);
		break;
	case BACKLIGHT_LOW_LEVEL:
		if (common_ops->fp_hbm_exit_extern)
			ret = common_ops->fp_hbm_exit_extern(panel_id, hld);
		break;
	default:
		LCD_KIT_ERR("unknown case!\n");
		break;
	}
	if (ret != LCD_KIT_OK)
		LCD_KIT_ERR("display_engine_fp_hbm_extern fail!\n");
	return ret;
}

int display_engine_restore_hbm_level(uint32_t panel_id, void *hld)
{
	int ret = LCD_KIT_OK;

	if (!hld) {
		LCD_KIT_ERR("hld is NULL!\n");
		return ret;
	}
	if (common_info->hbm.hbm_fp_support)
		LCD_KIT_INFO("hbm_fp_support is not support!\n");
	mutex_lock(&COMMON_LOCK->hbm_lock);
	ret = lcd_kit_exit_fp_hbm(panel_id, hld, common_info->hbm.hbm_level_current);
	mutex_unlock(&COMMON_LOCK->hbm_lock);
	return ret;
}

bool display_engine_hist_is_enable(void)
{
	struct display_engine_context *de_context = get_de_context(DISPLAY_ENGINE_PANEL_INNER);

	if (!de_context) {
		LCD_KIT_ERR("de_context is null\n");
		return false;
	}
	LCD_KIT_DEBUG("is_enable:%d\n", de_context->drm_hist.is_enable);
	return de_context->drm_hist.is_enable;
}

bool display_engine_hist_is_enable_change(void)
{
	struct display_engine_context *de_context = get_de_context(DISPLAY_ENGINE_PANEL_INNER);
	bool is_change = false;

	if (!de_context) {
		LCD_KIT_ERR("de_context is null\n");
		return false;
	}
	if (is_change != de_context->drm_hist.is_enable_change) {
		is_change = de_context->drm_hist.is_enable_change;
		de_context->drm_hist.is_enable_change = false;
		LCD_KIT_INFO("is_enable=%d is_enable_change: %d -> %d\n",
			de_context->drm_hist.is_enable, is_change,
			de_context->drm_hist.is_enable_change);
	}
	LCD_KIT_DEBUG("is_enable=%d is_enable_change=%d\n",
		de_context->drm_hist.is_enable, de_context->drm_hist.is_enable_change);
	return is_change;
}

static uint64_t correct_time_based_on_fps(uint32_t real_te_interval, uint64_t time_60_fps)
{
	return time_60_fps * real_te_interval / TE_INTERVAL_60_FPS_US;
}

static uint64_t get_real_te_interval(uint32_t panel_id)
{
	if (disp_info->fps.current_fps == FPS_LEVEL3)
		return TE_INTERVAL_120_FPS_US;
	else if (disp_info->fps.current_fps == FPS_LEVEL2)
		return TE_INTERVAL_90_FPS_US;
	else
		return TE_INTERVAL_60_FPS_US;
}

static uint64_t get_backlight_sync_delay_time_us(struct qcom_panel_info *panel_info,
	struct display_engine_context *de_context, uint32_t panel_id)
{
	uint64_t left_thres_us;
	uint64_t right_thres_us;
	uint64_t te_interval;
	uint64_t diff_from_te_time;
	uint64_t current_frame_time;
	uint64_t delay_us;
	uint32_t real_te_interval;

	if (panel_info->finger_unlock_state_support && !panel_info->finger_unlock_state) {
		LCD_KIT_INFO("in screen off fingerprint unlock\n");
		delay_us = 0;
		return delay_us;
	}
	LCD_KIT_INFO("panel_info->mask_delay.left_time_to_te_us = %lu us,"
		"panel_info->mask_delay.right_time_to_te_us = %lu us,"
		"panel_info->mask_delay.te_interval_us = %lu us\n",
		panel_info->mask_delay.left_time_to_te_us,
		panel_info->mask_delay.right_time_to_te_us, panel_info->mask_delay.te_interval_us);
	real_te_interval = get_real_te_interval(panel_id);
	left_thres_us = correct_time_based_on_fps(real_te_interval,
		panel_info->mask_delay.left_time_to_te_us);
	right_thres_us = correct_time_based_on_fps(real_te_interval,
		panel_info->mask_delay.right_time_to_te_us);
	te_interval = correct_time_based_on_fps(real_te_interval,
		panel_info->mask_delay.te_interval_us);
	if (te_interval == 0) {
		LCD_KIT_ERR("te_interval is 0, used default time\n");
		te_interval = TE_INTERVAL_60_FPS_US;
	}
	diff_from_te_time = ktime_to_us(ktime_get()) -
		ktime_to_us(de_context->brightness.te_timestamp);

	/* when diff time is more than 10 times TE, FP maybe blink. */
	if (diff_from_te_time > real_te_interval) {
		usleep_range(real_te_interval, real_te_interval);
		diff_from_te_time = ktime_to_us(ktime_get()) - ktime_to_us(
			de_context->brightness.te_timestamp);
		LCD_KIT_INFO("delay 1 frame to wait te, te = %d, diff_from_te_time =%ld\n",
			real_te_interval, diff_from_te_time);
	}
	current_frame_time = diff_from_te_time - ((diff_from_te_time / te_interval) * te_interval);
	if (current_frame_time < left_thres_us)
		delay_us = left_thres_us - current_frame_time;
	else if (current_frame_time > right_thres_us)
		delay_us = te_interval - current_frame_time + left_thres_us;
	else
		delay_us = 0;
	LCD_KIT_INFO("backlight_sync left_thres_us = %lu us, right_thres_us = %lu us,"
		"current_frame_time = %lu us, diff_from_te_time = %lu us,"
		"te_interval = %lu us, delay = %lu us\n", left_thres_us, right_thres_us,
		current_frame_time, diff_from_te_time, te_interval, delay_us);

	return delay_us;
}

static void handle_backlight_hbm_and_dc_delay(struct qcom_panel_info *panel_info,
	uint32_t panel_id, bool is_entering)
{
	uint32_t mask_delay_time;
	uint32_t real_te_interval = get_real_te_interval(panel_id);

	if (is_entering) {
		mask_delay_time = correct_time_based_on_fps(real_te_interval,
			panel_info->mask_delay.delay_time_before_fp);
	} else {
		mask_delay_time = correct_time_based_on_fps(real_te_interval,
			panel_info->mask_delay.delay_time_after_fp);
	}
	LCD_KIT_INFO("real_te_interval:%d, delay_time_before_fp:%d,"
		"delay_time_after_fp:%d, mask_delay_time:%d\n",
		real_te_interval, panel_info->mask_delay.delay_time_before_fp,
		panel_info->mask_delay.delay_time_after_fp, mask_delay_time);
	if (mask_delay_time != 0)
		usleep_range(mask_delay_time, mask_delay_time);
}

int display_engine_fp_backlight_set_sync(uint32_t panel_id, u32 level, u32 backlight_type)
{
	struct qcom_panel_info *panel_info = lcm_get_panel_info(panel_id);
	struct display_engine_context *de_context = get_de_context(DISPLAY_ENGINE_PANEL_INNER);
	struct lcd_kit_common_info *lcd_common_info = lcd_kit_get_common_info(panel_id);
	uint64_t fp_delay_time_us = 0;

	if (!de_context) {
		LCD_KIT_ERR("de_context is NULL\n");
		return LCD_KIT_FAIL;
	}

	if (!panel_info || !lcd_common_info) {
		LCD_KIT_ERR("panel_info or lcd_common_info is NULL!\n");
		return LCD_KIT_FAIL;
	}
	if (!de_context->is_inited) {
		LCD_KIT_ERR("de_context is not inited yet!\n");
		return LCD_KIT_FAIL;
	}

	/* Calculate the delay */
	fp_delay_time_us = get_backlight_sync_delay_time_us(panel_info, de_context, panel_id);
	if (fp_delay_time_us > 0)
		usleep_range(fp_delay_time_us, fp_delay_time_us);

	g_fingerprint_backlight_type = backlight_type;
	g_fingerprint_hbm_level = level;

	if (g_fingerprint_backlight_type == BACKLIGHT_LOW_LEVEL &&
		lcd_common_info->process_ddic_vint2_support)
		lcd_kit_get_common_ops()->set_ddic_cmd(panel_id, DSI_CMD_SET_DDIC_VINT2_OFF,
			LCD_KIT_DDIC_CMD_SYNC);

	display_engine_brightness_update_backlight();

	if (g_fingerprint_backlight_type == BACKLIGHT_LOW_LEVEL &&
		lcd_common_info->process_ddic_vint2_support)
		lcd_kit_get_common_ops()->set_ddic_cmd(panel_id, DSI_CMD_SET_DDIC_VINT2_ON,
			LCD_KIT_DDIC_CMD_ASYNC);

	handle_backlight_hbm_and_dc_delay(panel_info, panel_id,
		g_fingerprint_backlight_type == BACKLIGHT_HIGH_LEVEL);
	g_fingerprint_backlight_type_real = g_fingerprint_backlight_type;
	return LCD_KIT_OK;
}

void display_engine_brightness_set_alpha_bypass(bool is_bypass)
{
	struct display_engine_context *de_context = get_de_context(DISPLAY_ENGINE_PANEL_INNER);

	if (!de_context) {
		LCD_KIT_ERR("de_context is NULL\n");
		return;
	}
	LCD_KIT_INFO("brightness.is_alpha_bypass [%u] -> [%u]\n",
		de_context->brightness.is_alpha_bypass, is_bypass);
	de_context->brightness.is_alpha_bypass = is_bypass;
}

bool display_engine_brightness_should_do_fingerprint(void)
{
	int power_mode = display_engine_brightness_get_power_mode();

	display_engine_brightness_print_power_mode(power_mode);
	return (power_mode != SDE_MODE_DPMS_LP1) &&
		(power_mode != SDE_MODE_DPMS_LP2) &&
		(power_mode != SDE_MODE_DPMS_STANDBY) &&
		(power_mode != SDE_MODE_DPMS_SUSPEND);
}

uint32_t display_engine_brightness_get_mipi_level(void)
{
	struct display_engine_context *de_context = get_de_context(DISPLAY_ENGINE_PANEL_INNER);

	if (!de_context) {
		LCD_KIT_ERR("de_context is NULL\n");
		return 0;
	}
	return de_context->brightness.last_level_out;
}

uint32_t display_engine_brightness_get_mode_in_use(void)
{
	struct display_engine_context *de_context = get_de_context(DISPLAY_ENGINE_PANEL_INNER);

	if (!de_context) {
		LCD_KIT_ERR("de_context is NULL\n");
		return 0;
	}
	return de_context->brightness.mode_in_use;
}

static bool is_brightness_need_sync(
	int mode_in_use, int mode_set_by_ioctl, int last_alpha, int alpha)
{
	if (mode_in_use == mode_set_by_ioctl || alpha == last_alpha)
		return false;
	LCD_KIT_DEBUG("last_alpha [%d], alpa [%d]\n", last_alpha, alpha);
	return true;
}

static int get_mapped_alpha_inner_handle_mode(int bl_level,
	struct display_engine_context *de_context)
{
	int out_alpha = ALPHA_DEFAULT;
	unsigned int mode_set_by_ioctl;
	__u16 *to_alpha_lut = NULL;

	mode_set_by_ioctl = de_context->brightness.mode_set_by_ioctl;
	if (mode_set_by_ioctl >= DISPLAY_ENGINE_MODE_MAX) {
		LCD_KIT_ERR("mode_set_by_ioctl out of range, mode_set_by_ioctl = %d\n",
			mode_set_by_ioctl);
		return ALPHA_DEFAULT;
	}
	if (de_context->brightness.lut[mode_set_by_ioctl].mode_id ==
		DISPLAY_ENGINE_BRIGHTNESS_MODE_DEFAULT) {
		return ALPHA_DEFAULT;
	}
	to_alpha_lut = de_context->brightness.lut[mode_set_by_ioctl].to_alpha_lut;
	if (!to_alpha_lut) {
		LCD_KIT_ERR("to_alpha_lut is NULL, mode_set_by_ioctl:%d\n", mode_set_by_ioctl);
		return ALPHA_DEFAULT;
	}
	switch (mode_set_by_ioctl) {
	case DISPLAY_ENGINE_BRIGHTNESS_MODE_DEFAULT:
		out_alpha = ALPHA_DEFAULT;
		break;
	case DISPLAY_ENGINE_BRIGHTNESS_MODE_DC:
	case DISPLAY_ENGINE_BRIGHTNESS_MODE_HIGH_PRECISION:
		out_alpha = to_alpha_lut[bl_level];
		LCD_KIT_DEBUG("brightness.mode_set_by_ioctl[%u], in_level[%u] -> out_alpha[%u]\n",
			mode_set_by_ioctl, bl_level, out_alpha);
		break;
	default:
		LCD_KIT_WARNING("invalid brightness.mode_set_by_ioctl %d\n", mode_set_by_ioctl);
		out_alpha = ALPHA_DEFAULT;
	}
	if (out_alpha <= 0 || out_alpha > ALPHA_DEFAULT) {
		LCD_KIT_ERR("invalid out_alpha, out_alpha:%d\n", out_alpha);
		out_alpha = ALPHA_DEFAULT;
	}
	return out_alpha;
}

static int get_mapped_alpha_inner(int bl_level)
{
	struct display_engine_context *de_context = get_de_context(DISPLAY_ENGINE_PANEL_INNER);

	if (!de_context) {
		LCD_KIT_ERR("de_context is NULL\n");
		return ALPHA_DEFAULT;
	}

	/* bypass alpha at request, e.g. fingerprint */
	if (de_context->brightness.is_alpha_bypass) {
		LCD_KIT_INFO("alpha bypassed...\n");
		return ALPHA_DEFAULT;
	}
	if (!de_context->is_inited) {
		LCD_KIT_WARNING("display engine is not inited yet.\n");
		return ALPHA_DEFAULT;
	}
	if (bl_level < 0 || bl_level >= BRIGHTNESS_LUT_LENGTH) {
		LCD_KIT_ERR("invalid bl_level %d\n", bl_level);
		return ALPHA_DEFAULT;
	}

	return get_mapped_alpha_inner_handle_mode(bl_level, de_context);
}

static bool handle_brightness_lut(
	int in_level, struct display_engine_context *de_context, __u16 *out_level, int mode_in_use)
{
	__u16 *to_dbv_lut = NULL;

	if (de_context->brightness.lut[mode_in_use].mode_id ==
		DISPLAY_ENGINE_BRIGHTNESS_MODE_DEFAULT) {
		LCD_KIT_DEBUG("invalid brightness lut, mode_in_use = %d\n", mode_in_use);
		return false;
	}
	to_dbv_lut = de_context->brightness.lut[mode_in_use].to_dbv_lut;
	if (!to_dbv_lut) {
		LCD_KIT_ERR("to_dbv_lut is NULL, mode_in_use = %d\n", mode_in_use);
		return false;
	}
	*out_level = to_dbv_lut[in_level];
	LCD_KIT_DEBUG("brightness.mode_in_use [%u], in_level [%u] -> out_level [%u]\n",
		mode_in_use, in_level, *out_level);
	return true;
}

static int get_mapped_level_handle_mode(int in_level, struct display_engine_context *de_context)
{
	__u16 out_level = in_level;
	unsigned int mode_in_use;

	mode_in_use = de_context->brightness.mode_in_use;
	if (mode_in_use >= DISPLAY_ENGINE_MODE_MAX) {
		LCD_KIT_ERR("mode_in_use out of range, mode_in_use = %d\n", mode_in_use);
		goto error_out;
	}
	if (in_level == 0) {
		de_context->brightness.mode_in_use = DISPLAY_ENGINE_BRIGHTNESS_MODE_DEFAULT;
		LCD_KIT_DEBUG("in_level is 0, need sync later, mdoe = %d\n", mode_in_use);
		goto error_out;
	}

	switch (mode_in_use) {
	case DISPLAY_ENGINE_BRIGHTNESS_MODE_DEFAULT:
		/* mode_in_use is set to DEFAULT when level is 0, to trigger a sync at screen on */
		if (is_brightness_need_sync(
			de_context->brightness.mode_in_use,
			de_context->brightness.mode_set_by_ioctl,
			ALPHA_DEFAULT, get_mapped_alpha_inner(in_level))) {
			/*
			 * Need to sync: since mode_in_use is still DEFAULT, it means
			 * brightness_handle_sync is not processed yet. To avoid flickering, we have
			 * to use the last level out, which is 0 in the screen on case.
			 */
			LCD_KIT_INFO("in default mode need sync, use last level.\n");
			out_level = de_context->brightness.last_level_out;
		} else {
			/*
			 * No need to sync: use mode_set_by_ioctl directly.
			 * brightness_handle_sync will update mode_in_use.
			 */
			if (!handle_brightness_lut(in_level, de_context, &out_level,
				de_context->brightness.mode_set_by_ioctl))
				goto error_out;
		}
		break;
	case DISPLAY_ENGINE_BRIGHTNESS_MODE_DC:
	case DISPLAY_ENGINE_BRIGHTNESS_MODE_HIGH_PRECISION:
		/* Normal brightness update goes here */
		if (!handle_brightness_lut(in_level, de_context, &out_level, mode_in_use))
			goto error_out;
		break;
	default:
		LCD_KIT_ERR("invalid brightness.mode_in_use %d\n", mode_in_use);
		goto error_out;
	}
	de_context->brightness.last_level_out = out_level;
	return out_level;

error_out:
	de_context->brightness.last_level_out = in_level;
	return in_level;
}

int display_engine_brightness_get_mapped_level(int in_level, uint32_t panel_id)
{
	struct display_engine_context *de_context = get_de_context(DISPLAY_ENGINE_PANEL_INNER);

	/* Keep fingerprint on top, unless the priority of new code is higher than fingerprint */
	if (g_fingerprint_backlight_type == BACKLIGHT_HIGH_LEVEL)
		return g_fingerprint_hbm_level;

	if (panel_id != PRIMARY_PANEL) {
		LCD_KIT_DEBUG("panel_id is not PRIMARY_PANEL\n");
		return in_level;
	}

	if (!de_context) {
		LCD_KIT_ERR("de_context is NULL\n");
		return in_level;
	}

	de_context->brightness.last_level_in = in_level;
	if (in_level < 0 || in_level >= BRIGHTNESS_LUT_LENGTH) {
		LCD_KIT_ERR("invalid in_level %d\n", in_level);
		goto error_out;
	}
	if (!de_context->is_inited) {
		LCD_KIT_WARNING("display engine is not inited yet.\n");
		goto error_out;
	}
	if (de_context->brightness.mode_set_by_ioctl == DISPLAY_ENGINE_BRIGHTNESS_MODE_DEFAULT &&
		de_context->brightness.mode_in_use == DISPLAY_ENGINE_BRIGHTNESS_MODE_DEFAULT) {
		LCD_KIT_DEBUG("default brightness mode.\n");
		goto error_out;
	}

	return get_mapped_level_handle_mode(in_level, de_context);

error_out:
	de_context->brightness.last_level_out = in_level;
	return in_level;
}

int display_engine_brightness_get_mapped_alpha(void)
{
	struct display_engine_context *de_context = get_de_context(DISPLAY_ENGINE_PANEL_INNER);
	int alpha;
	int power_mode = display_engine_brightness_get_power_mode();

	if (!de_context) {
		LCD_KIT_ERR("de_context is NULL\n");
		return ALPHA_DEFAULT;
	}
	de_context->brightness.is_alpha_updated_at_mode_change =
		de_context->brightness.mode_in_use != de_context->brightness.mode_set_by_ioctl;
	if (power_mode != SDE_MODE_DPMS_ON && power_mode != SDE_MODE_DPMS_OFF) {
		LCD_KIT_DEBUG("not in power on/off state, return default alpha\n");
		de_context->brightness.last_alpha = de_context->brightness.current_alpha;
		de_context->brightness.current_alpha = ALPHA_DEFAULT;
		return ALPHA_DEFAULT;
	}
	alpha = get_mapped_alpha_inner(de_context->brightness.last_level_in);
	de_context->brightness.last_alpha = de_context->brightness.current_alpha;
	de_context->brightness.current_alpha = alpha;
	return alpha;
}

/* IO implementation functions: */
/* DRM features: */
static int display_engine_drm_hist_get_enable(uint32_t panel_id,
	struct display_engine_drm_hist *hist)
{
	struct display_engine_context *de_context = get_de_context(panel_id);

	if (!de_context) {
		LCD_KIT_ERR("de_context is null, panel_id=%u\n", panel_id);
		return LCD_KIT_FAIL;
	}

	hist->enable = de_context->drm_hist.is_enable ? 1 : 0;
	LCD_KIT_INFO("panel_id=%u is_enable=%u hist->enable=%u\n",
		panel_id, de_context->drm_hist.is_enable, hist->enable);
	return LCD_KIT_OK;
}

static int display_engine_drm_hist_set_enable(uint32_t panel_id,
	const struct display_engine_drm_hist *hist)
{
	struct display_engine_context *de_context = get_de_context(panel_id);
	bool is_enable = (hist->enable == 1);

	if (!de_context) {
		LCD_KIT_ERR("de_context is null, panel_id=%u\n", panel_id);
		return LCD_KIT_FAIL;
	}

	LCD_KIT_INFO("panel_id=%u is_enable=%u hist->enable=%u\n", is_enable, hist->enable);
	if (de_context->drm_hist.is_enable != is_enable) {
		de_context->drm_hist.is_enable = is_enable;
		de_context->drm_hist.is_enable_change = true;
		LCD_KIT_INFO("panel_id=%u is_enable=%d is_enable_change=%d\n",
			panel_id,
			de_context->drm_hist.is_enable,
			de_context->drm_hist.is_enable_change);
	}
	return LCD_KIT_OK;
}

/* Brightness features */
static int display_engine_brightness_set_mode(uint32_t panel_id, uint32_t mode)
{
	struct display_engine_context *de_context = get_de_context(panel_id);

	if (!de_context) {
		LCD_KIT_ERR("de_context is NULL\n");
		return ALPHA_DEFAULT;
	}

	LCD_KIT_INFO("brightness.mode_set_by_ioctl [%d] -> [%d]\n",
		de_context->brightness.mode_set_by_ioctl, mode);

	/*
	 * Sync between mode_set_by_ioctl and mode_in_use will be handle by
	 * display_engine_brightness_handle_mode_change(void) in sde_crtc_atomic_flush
	 */
	de_context->brightness.mode_set_by_ioctl = mode;
	return LCD_KIT_OK;
}

static int display_engine_brightness_set_fingerprint_backlight(uint32_t panel_id,
	struct display_engine_fingerprint_backlight fingerprint_backlight)
{
	struct display_engine_context *de_context = get_de_context(panel_id);

	if (!de_context) {
		LCD_KIT_ERR("de_context is NULL\n");
		return ALPHA_DEFAULT;
	}
	de_context->brightness.fingerprint_backlight = fingerprint_backlight;
	LCD_KIT_INFO("[effect] scene:%d hbm_level:%d current_level:%d\n",
		de_context->brightness.fingerprint_backlight.scene_info,
		de_context->brightness.fingerprint_backlight.hbm_level,
		de_context->brightness.fingerprint_backlight.current_level);
	return LCD_KIT_OK;
}

static void display_engine_brightness_print_lut(struct display_engine_brightness_lut *lut)
{
	LCD_KIT_INFO("mode_id [%u]", lut->mode_id);
	LCD_KIT_INFO("to_dbv_lut[0 - 4]: [%u, %u, %u, %u, %u], [last]: [%u]",
		lut->to_dbv_lut[0], lut->to_dbv_lut[1],
		lut->to_dbv_lut[2], lut->to_dbv_lut[3],
		lut->to_dbv_lut[4], lut->to_dbv_lut[BRIGHTNESS_LUT_LENGTH - 1]);
	LCD_KIT_INFO("to_alpha_lut[0 - 4]: [%u, %u, %u, %u, %u], [last]: [%u]",
		lut->to_alpha_lut[0], lut->to_alpha_lut[1],
		lut->to_alpha_lut[2], lut->to_alpha_lut[3],
		lut->to_alpha_lut[4], lut->to_alpha_lut[BRIGHTNESS_LUT_LENGTH - 1]);
}

static int display_engine_brightness_check_lut_inner(unsigned short *lut, int length)
{
	/* 1 indicates that check from the second value of the array */
	int lut_index = 1;

	/* Check if lut map contains zero */
	for (; lut_index < length; lut_index++) {
		if (lut[lut_index] == 0) {
			LCD_KIT_ERR("lut[%d] is 0\n", lut_index);
			break;
		}
	}

	if (lut_index != length)
		return LCD_KIT_FAIL;
	else
		return LCD_KIT_OK;
}

static int display_engine_brightness_check_lut(struct display_engine_brightness_lut *lut)
{
	int ret;

	ret = display_engine_brightness_check_lut_inner(lut->to_dbv_lut, BRIGHTNESS_LUT_LENGTH);
	if (ret != LCD_KIT_OK) {
		LCD_KIT_ERR("error occurred when checking lut->to_dbv_lut\n");
		return LCD_KIT_FAIL;
	}

	ret = display_engine_brightness_check_lut_inner(lut->to_alpha_lut, BRIGHTNESS_LUT_LENGTH);
	if (ret != LCD_KIT_OK) {
		LCD_KIT_ERR("error occurred when checking lut->to_alpha_lut\n");
		return LCD_KIT_FAIL;
	}

	return LCD_KIT_OK;
}

/* brightness init */
static int display_engine_brightness_init_lut(uint32_t panel_id, __u32 mode_id)
{
	struct display_engine_context *de_context = get_de_context(DISPLAY_ENGINE_PANEL_INNER);

	if (panel_id != PRIMARY_PANEL) {
		LCD_KIT_INFO("not primary panel, skip lut init\n");
		return LCD_KIT_OK;
	}
	if (!de_context) {
		LCD_KIT_ERR("de_context is null\n");
		return LCD_KIT_FAIL;
	}
	if (de_context->brightness.lut[mode_id].to_dbv_lut &&
		de_context->brightness.lut[mode_id].to_alpha_lut) {
		LCD_KIT_WARNING("Repeated initialization, mode_id = %d\n", mode_id);
		return LCD_KIT_OK;
	}
	de_context->brightness.lut[mode_id].to_dbv_lut =
		kcalloc(BRIGHTNESS_LUT_LENGTH, sizeof(__u16), GFP_KERNEL);
	if (!de_context->brightness.lut[mode_id].to_dbv_lut) {
		LCD_KIT_ERR("to_dbv_lut malloc fail\n");
		goto error_out_after_alloc;
	}
	de_context->brightness.lut[mode_id].to_alpha_lut =
		kcalloc(BRIGHTNESS_LUT_LENGTH, sizeof(__u16), GFP_KERNEL);
	if (!de_context->brightness.lut[mode_id].to_alpha_lut) {
		LCD_KIT_ERR("to_alpha_lut malloc fail\n");
		goto error_out_after_alloc;
	}
	/*
	 * in DISPLAY_ENGINE_BRIGHTNESS_MODE_DEFAULT, brightness will not be mapped.
	 * Both luts should NOT be used in this situation, since they are likely not set yet.
	 */
	de_context->brightness.lut[mode_id].mode_id = DISPLAY_ENGINE_BRIGHTNESS_MODE_DEFAULT;
	return LCD_KIT_OK;

error_out_after_alloc:
	kfree(de_context->brightness.lut[mode_id].to_dbv_lut);
	kfree(de_context->brightness.lut[mode_id].to_alpha_lut);
	de_context->brightness.lut[mode_id].to_dbv_lut = NULL;
	de_context->brightness.lut[mode_id].to_alpha_lut = NULL;
	return LCD_KIT_FAIL;
}

static int display_engine_brightness_set_lut(uint32_t panel_id,
	struct display_engine_brightness_lut *input_lut)
{
	int ret = LCD_KIT_OK;
	struct display_engine_brightness_lut *target_lut = NULL;
	struct display_engine_context *de_context = get_de_context(panel_id);

	if (!de_context) {
		LCD_KIT_ERR("de_context is null\n");
		return LCD_KIT_FAIL;
	}
	if (!input_lut) {
		LCD_KIT_ERR("input_lut is NULL\n");
		return LCD_KIT_FAIL;
	}
	if (input_lut->mode_id >= DISPLAY_ENGINE_MODE_MAX) {
		LCD_KIT_ERR("mode_id out of range, mode_id_max = %d\n, mode_id = %d\n",
			DISPLAY_ENGINE_MODE_MAX, input_lut->mode_id);
		return LCD_KIT_FAIL;
	}

	/* init brightness */
	if (display_engine_brightness_init_lut(panel_id, input_lut->mode_id) != LCD_KIT_OK) {
		LCD_KIT_ERR("brightness_lut_init failed, exit.\n");
		return LCD_KIT_FAIL;
	}

	target_lut = &de_context->brightness.lut[input_lut->mode_id];
	if (!target_lut->to_dbv_lut || !target_lut->to_alpha_lut) {
		LCD_KIT_ERR("to_dbv_lut or to_alpha_lut is NULL\n");
		return LCD_KIT_FAIL;
	}
	ret = copy_from_user(target_lut->to_dbv_lut, input_lut->to_dbv_lut,
		BRIGHTNESS_LUT_LENGTH * sizeof(__u16));
	if (ret != 0) {
		LCD_KIT_ERR("[effect] copy dbv lut failed, ret = %d\n", ret);
		return LCD_KIT_FAIL;
	}
	ret = copy_from_user(target_lut->to_alpha_lut, input_lut->to_alpha_lut,
		BRIGHTNESS_LUT_LENGTH * sizeof(__u16));
	if (ret != 0) {
		LCD_KIT_ERR("[effect] copy alpha lut failed, ret = %d\n", ret);
		return LCD_KIT_FAIL;
	}
	target_lut->mode_id = input_lut->mode_id;
	display_engine_brightness_print_lut(target_lut);

	ret = display_engine_brightness_check_lut(target_lut);
	if (ret != 0) {
		LCD_KIT_ERR("[effect] invalid lut, use mode default!\n", ret);
		target_lut->mode_id = DISPLAY_ENGINE_BRIGHTNESS_MODE_DEFAULT;
		return LCD_KIT_FAIL;
	}
	return ret;
}

static int display_engine_brightness_init_lhbm_alpha_lut(uint32_t panel_id)
{
	struct display_engine_context *de_context = get_de_context(panel_id);

	if (panel_id != PRIMARY_PANEL) {
		LCD_KIT_INFO("not primary panel, skip lut init\n");
		return LCD_KIT_OK;
	}
	if (!de_context) {
		LCD_KIT_ERR("de_context is null\n");
		return LCD_KIT_FAIL;
	}
	if (de_context->brightness.alpha_map.lum_lut) {
		LCD_KIT_WARNING("Repeated initialization alpha_map\n");
		return LCD_KIT_OK;
	}
	de_context->brightness.alpha_map.lum_lut =
		kvzalloc(LHBM_ALPHA_LUT_LENGTH * sizeof(__u16), GFP_KERNEL);
	if (!de_context->brightness.alpha_map.lum_lut) {
		LCD_KIT_ERR("lum_lut malloc fail\n");
		goto error_out_after_alloc;
	}
	return LCD_KIT_OK;

error_out_after_alloc:
	kvfree(de_context->brightness.alpha_map.lum_lut);
	de_context->brightness.alpha_map.lum_lut = NULL;
	return LCD_KIT_FAIL;
}

static void display_engine_brightness_set_alpha_lut_default(
	struct display_engine_alpha_map *input_alpha_map)
{
	int alpha_map_index = 0;

	for (; alpha_map_index < LHBM_ALPHA_LUT_LENGTH; alpha_map_index++)
		input_alpha_map->lum_lut[alpha_map_index] = LCD_KIT_ALPHA_DEFAULT;
}

static int display_engine_brightness_set_alpha_map(uint32_t panel_id,
	struct display_engine_alpha_map *input_alpha_map)
{
	int ret = LCD_KIT_OK;
	struct display_engine_alpha_map *target_alpha_map = NULL;
	struct display_engine_context *de_context = get_de_context(panel_id);

	if (!de_context) {
		LCD_KIT_ERR("de_context is null\n");
		return LCD_KIT_FAIL;
	}
	if (!input_alpha_map) {
		LCD_KIT_ERR("input_alpha_map is NULL\n");
		return LCD_KIT_FAIL;
	}

	/* init brightness */
	if (display_engine_brightness_init_lhbm_alpha_lut(panel_id) != LCD_KIT_OK) {
		LCD_KIT_ERR("display_engine_alpha_map_init failed, exit.\n");
		return LCD_KIT_FAIL;
	}

	target_alpha_map = &de_context->brightness.alpha_map;
	if (!target_alpha_map->lum_lut) {
		LCD_KIT_ERR("target_alpha_map lum_lut is NULL\n");
		return LCD_KIT_FAIL;
	}
	ret = copy_from_user(target_alpha_map->lum_lut, input_alpha_map->lum_lut,
		LHBM_ALPHA_LUT_LENGTH * sizeof(__u16));
	if (ret != 0) {
		LCD_KIT_ERR("[effect] copy input_alpha_map->lum_lut failed, ret = %d\n", ret);
		return LCD_KIT_FAIL;
	}
	ret = display_engine_brightness_check_lut_inner(target_alpha_map->lum_lut,
		LHBM_ALPHA_LUT_LENGTH);
	if (ret != LCD_KIT_OK) {
		LCD_KIT_ERR("check alpha map failed, use default alpha value ret = %d\n", ret);
		display_engine_brightness_set_alpha_lut_default(target_alpha_map);
	}
	LCD_KIT_INFO("target_alpha_map[0 - 4]: [%u, %u, %u, %u, %u], [last]: [%u]",
		target_alpha_map->lum_lut[0], target_alpha_map->lum_lut[1],
		target_alpha_map->lum_lut[2], target_alpha_map->lum_lut[3],
		target_alpha_map->lum_lut[4],
		target_alpha_map->lum_lut[LHBM_ALPHA_LUT_LENGTH - 1]);
	return ret;
}

void display_engine_brightness_handle_vblank(ktime_t te_timestamp)
{
	struct qcom_panel_info *panel_info = lcm_get_panel_info(PRIMARY_PANEL);
	struct display_engine_context *de_context = get_de_context(DISPLAY_ENGINE_PANEL_INNER);

	if (!panel_info) {
		LCD_KIT_ERR("panel_info is NULL\n");
		return;
	}
	if (!de_context) {
		LCD_KIT_ERR("de_context is NULL\n");
		return;
	}
	de_context->brightness.te_timestamp_last = de_context->brightness.te_timestamp;
	de_context->brightness.te_timestamp = te_timestamp;
	if (de_context->is_inited)
		queue_work(panel_info->vblank_workqueue, &panel_info->vblank_work);
}

static void brightness_handle_no_sync(struct qcom_panel_info *panel_info,
	struct display_engine_context *de_context)
{
	if (de_context->brightness.mode_in_use != de_context->brightness.mode_set_by_ioctl)
		LCD_KIT_INFO("no need to update, alpha [%d]->[%d].\n",
			de_context->brightness.last_alpha,
			de_context->brightness.current_alpha);

	de_context->brightness.is_alpha_updated_at_mode_change = false;
	de_context->brightness.mode_in_use = de_context->brightness.mode_set_by_ioctl;
	if (de_context->brightness.last_level_out == 0) {
		LCD_KIT_INFO("screen on update backlight\n");
		queue_work(panel_info->brightness_workqueue, &panel_info->backlight_update_work);
	}
}

static void brightness_handle_sync(struct qcom_panel_info *panel_info,
	struct display_engine_context *de_context)
{
	uint64_t sync_delay_time_us = 0;
	struct dsi_panel *panel = NULL;
	uint32_t panel_id;

	/* reset is_alpha_updated_at_mode_change for next sync */
	de_context->brightness.is_alpha_updated_at_mode_change = false;
	LCD_KIT_INFO("mode changed [%u] -> [%u], need sync.\n",
		de_context->brightness.mode_in_use, de_context->brightness.mode_set_by_ioctl);

	/* Delay atomic flush if needed, make sure backlight and alpha take effect in one frame */
	sync_delay_time_us = get_backlight_sync_delay_time_us(
		panel_info, de_context, PRIMARY_PANEL);
	if (sync_delay_time_us > 0)
		usleep_range(sync_delay_time_us, sync_delay_time_us);

	/* Sync both mode since alpha is ready, and backlight is about to update */
	de_context->brightness.mode_in_use = de_context->brightness.mode_set_by_ioctl;

	queue_work(panel_info->brightness_workqueue, &panel_info->backlight_update_work);

	panel = panel_info->display->panel;
	if (!panel) {
		LCD_KIT_ERR("panel is NULL!\n");
		return;
	}
	panel_id = lcd_kit_get_current_panel_id(panel);

	/* mask delay for DC mode */
	handle_backlight_hbm_and_dc_delay(panel_info, panel_id,
		de_context->brightness.mode_set_by_ioctl == DISPLAY_ENGINE_BRIGHTNESS_MODE_DC);
	LCD_KIT_INFO("sync complete, sync_delay_time_us [%lu]", sync_delay_time_us);
}

void display_engine_brightness_handle_mode_change(void)
{
	struct qcom_panel_info *panel_info = lcm_get_panel_info(PRIMARY_PANEL);
	struct display_engine_context *de_context = get_de_context(DISPLAY_ENGINE_PANEL_INNER);

	if (!de_context) {
		LCD_KIT_ERR("de_context is null\n");
		return;
	}
	if (!panel_info) {
		LCD_KIT_ERR("panel_info is null\n");
		return;
	}
	if (!de_context->is_inited) {
		LCD_KIT_ERR("de_context is not inited yet!\n");
		return;
	}
	if (display_engine_brightness_get_backlight() == 0) {
		LCD_KIT_DEBUG("backlight is not updated yet.\n");
		return;
	}
	if (!de_context->brightness.is_alpha_updated_at_mode_change) {
		LCD_KIT_DEBUG("alpha is not updated yet.\n");
		return;
	}
	if (!is_brightness_need_sync(
		de_context->brightness.mode_in_use, de_context->brightness.mode_set_by_ioctl,
		de_context->brightness.last_alpha, de_context->brightness.current_alpha)) {
		brightness_handle_no_sync(panel_info, de_context);
		return;
	}
	/* need sync */
	brightness_handle_sync(panel_info, de_context);
}

bool display_engine_brightness_is_fingerprint_hbm_enabled(void)
{
	return g_fingerprint_backlight_type == BACKLIGHT_HIGH_LEVEL ||
		g_fingerprint_backlight_type_real == BACKLIGHT_HIGH_LEVEL;
}

/* Panel information: */
static int display_engine_panel_info_get(uint32_t panel_id,
	struct display_engine_panel_info *info)
{
	uint32_t lcd_panel_id = display_engine_panel_id_to_lcdkit(panel_id);
	struct qcom_panel_info *panel_info = lcm_get_panel_info(lcd_panel_id);
	struct lcd_kit_common_info *lcd_common_info = lcd_kit_get_common_info(lcd_panel_id);

	if (!panel_info || !panel_info->display || !lcd_common_info) {
		LCD_KIT_ERR("NULL point: panel_info=%p common_info=%p!\n", panel_info,
			lcd_common_info);
		return LCD_KIT_FAIL;
	}

	info->width = panel_info->display->modes[0].timing.h_active;
	info->height = panel_info->display->modes[0].timing.v_active;
	info->max_luminance = panel_info->maxluminance;
	info->min_luminance = panel_info->minluminance;
	info->max_backlight = panel_info->bl_max;
	info->min_backlight = panel_info->bl_min;
	info->sn_code_length = (panel_info->sn_code_length > SN_CODE_LEN_MAX) ?
		SN_CODE_LEN_MAX : panel_info->sn_code_length;
	if (info->sn_code_length > 0) {
		char sn_text[SN_TEXT_SIZE];
		uint32_t i;
		memcpy(info->sn_code, panel_info->sn_code, info->sn_code_length);
		for (i = 0; i < info->sn_code_length; i++)
			sprintf(sn_text + (i << 1), "%02x", info->sn_code[i]);
		for (i <<= 1; i < SN_CODE_LEN_MAX; i++)
			sn_text[i] = '\0';
		LCD_KIT_INFO("sn[%u]=\'%s\'\n", info->sn_code_length, sn_text);
	} else {
		memset(info->sn_code, 0, sizeof(info->sn_code));
		LCD_KIT_INFO("sn is empty\n");
	}
	strncpy(info->panel_name, lcd_common_info->panel_name, PANEL_NAME_LEN - 1);
	info->local_hbm_support = display_engine_local_hbm_get_support();
#ifdef LCD_FACTORY_MODE
	info->is_factory = true;
#else
	info->is_factory = false;
#endif
	LCD_KIT_INFO("panel_id=%u res(w=%u,h=%u) lum(min=%u,max=%u) bl(min=%u,max=%u) name=%s\n",
		panel_id, info->width, info->height, info->max_luminance, info->min_luminance,
		info->max_backlight, info->min_backlight, info->panel_name);
	return LCD_KIT_OK;
}

static uint32_t calculate_acc_value(uint32_t type, uint32_t region, struct timeval cur_time)
{
	uint64_t delta = 0;
	uint64_t level = g_de_common_context.aging_data.mipi_level;
	uint32_t fps = g_de_common_context.aging_data.fps_index;
	struct timeval last_time = g_de_common_context.aging_data.last_time[type][region][fps];

	do_gettimeofday(&cur_time);
	switch (type) {
	case DE_MIPI_DBV_ACC:
		break;
	case DE_ORI_DBV_ACC:
		level = g_de_common_context.aging_data.orig_level;
		break;
	case DE_SCREEN_ON_TIME:
		level = (level > 0) ? 1 : 0;
		break;
	case DE_HBM_ACC:
		level = (level >= g_de_common_context.aging_data.min_hbm_dbv) ? level : 0;
		break;
	case DE_HBM_ON_TIME:
		level = (level < g_de_common_context.aging_data.min_hbm_dbv) ? 0 : 1;
		break;
	default:
		return 0;
	}
	delta = level * ((cur_time.tv_sec - last_time.tv_sec) * 1000 +
		(cur_time.tv_usec - last_time.tv_usec) / 1000);
	LCD_KIT_DEBUG("DE_FC acc_value delta %u, type %u, level %u, fps %u, region %u\n",
		delta, type, level, fps, region);
	return (uint32_t)delta;
}

static void update_acc_value(void)
{
	struct timeval cur_time;
	unsigned int type;
	unsigned int region;
	unsigned int fps = g_de_common_context.aging_data.fps_index;

	do_gettimeofday(&cur_time);
	for (region = 0; region < DE_REGION_NUM; region++) {
		if (!g_de_common_context.aging_data.is_display_region_enable[region])
			continue;
		for (type = 0; type < DE_STATISTICS_TYPE_MAX; type++) {
			g_de_common_context.aging_data.data[type][region][fps] +=
				calculate_acc_value(type, region, cur_time);
			g_de_common_context.aging_data.last_time[type][region][fps] = cur_time;
		}
	}
}

static void update_last_time_value(void)
{
	unsigned int type = 0;
	unsigned int region = 0;
	unsigned int fps = 0;
	struct timeval cur_time;

	do_gettimeofday(&cur_time);
	for (type = 0; type < DE_STATISTICS_TYPE_MAX; type++) {
		for (region = 0; region < DE_REGION_NUM; region++) {
			for (fps = 0; fps < DE_FPS_MAX; fps++)
				g_de_common_context.aging_data.last_time[type][region][fps] =
					cur_time;
		}
	}
}

static uint32_t get_fps_index(uint32_t panel_id)
{
	if (disp_info->fps.current_fps == FPS_LEVEL3)
		return DE_FPS_LEVEL3_INDEX;
	else if (disp_info->fps.current_fps == FPS_LEVEL2)
		return DE_FPS_LEVEL2_INDEX;
	else
		return DE_FPS_LEVEL1_INDEX;
}

static void display_engine_foldable_info_print(uint32_t fps_index, uint32_t region,
	struct display_engine_foldable_info *param)
{
	LCD_KIT_INFO("DE_FC fps_index:%u,region:%u,mipi %u,ori %u,screen_on %u,hbm %u,hbm_on %u",
		fps_index, region, param->data[DE_MIPI_DBV_ACC][region][fps_index],
		param->data[DE_ORI_DBV_ACC][region][fps_index],
		param->data[DE_SCREEN_ON_TIME][region][fps_index],
		param->data[DE_HBM_ACC][region][fps_index],
		param->data[DE_HBM_ON_TIME][region][fps_index]);
}

static int display_engine_foldable_info_get(uint32_t panel_id,
	struct display_engine_foldable_info *param)
{
	struct display_engine_context *de_context = get_de_context(panel_id);
	uint32_t fps_index;
	uint32_t region;

	if (!g_de_common_context.aging_data.support)
		return LCD_KIT_OK;
	if (!de_context) {
		LCD_KIT_ERR("de_context is null, panel_id=%u\n", panel_id);
		return LCD_KIT_FAIL;
	}
	if (!g_de_common_context.aging_data.is_inited) {
		LCD_KIT_INFO("not init yet");
		return LCD_KIT_OK;
	}
	mutex_lock(&g_de_common_context.aging_data.lock);
	update_acc_value();
	memcpy(param->data, g_de_common_context.aging_data.data,
		sizeof(param->data));
	memset(g_de_common_context.aging_data.data, 0,
		sizeof(g_de_common_context.aging_data.data));
	update_last_time_value();
	for (fps_index = 0; fps_index < DE_FPS_MAX; fps_index++) {
		for (region = 0; region < DE_REGION_NUM; region++)
			display_engine_foldable_info_print(fps_index, region, param);
	}

	mutex_unlock(&g_de_common_context.aging_data.lock);
	return LCD_KIT_OK;
}

static int display_engine_foldable_dbi_roi_set(uint32_t panel_id,
	struct display_engine_roi_param roi_param)
{
	int ret = LCD_KIT_OK;
	struct lcd_kit_adapt_ops *adapt_ops = NULL;

	adapt_ops = lcd_kit_get_adapt_ops();
	if (!adapt_ops->mipi_tx) {
		LCD_KIT_ERR("DE_FC adapt_ops->mipi_tx is NULL\n");
		return LCD_KIT_FAIL;
	}
	/* Minimun deburn-in ofs1[8]. */
	common_info->ddic_bic.roi_cmds.cmds[1].payload[3] =
		((((roi_param.bottom >> 3) >> 8) & 0xF) << 4) |
		((((roi_param.top >> 3) >> 8) & 0xF) << 5) |
		((((roi_param.right >> 4) >> 8) & 0xF) << 6) |
		((((roi_param.left >> 3) >> 8) & 0xF) << 7);
	/* ofs1[0:7] */
	common_info->ddic_bic.roi_cmds.cmds[1].payload[4] = (roi_param.left >> 3) & 0xFF;
	/* There are two ic for one panel, right need >> 4 not 3. */
	common_info->ddic_bic.roi_cmds.cmds[1].payload[5] = (roi_param.right >> 4) & 0xFF;
	common_info->ddic_bic.roi_cmds.cmds[1].payload[6] = (roi_param.top >> 3) & 0xFF;
	common_info->ddic_bic.roi_cmds.cmds[1].payload[7] = (roi_param.bottom >> 3) & 0xFF;
	ret = adapt_ops->mipi_tx(
		lcm_get_panel_info(display_engine_panel_id_to_lcdkit(panel_id))->display,
		&common_info->ddic_bic.roi_cmds);
	if (ret != LCD_KIT_OK)
		LCD_KIT_ERR("DE_FC mipi_tx failed, returen %d\n", ret);
	return ret;
}

static int display_engine_foldable_dbi_weight_set(uint32_t panel_id,
	uint16_t dbi_weight[DE_COLOR_INDEX_COUNT][DDIC_COMPENSATION_LUT_COUNT])
{
	int color;
	int ret = LCD_KIT_OK;
	struct lcd_kit_adapt_ops *adapt_ops = NULL;
	uint32_t lut_index;
	/* 8 is max weight length */
	char weight_tmp[8] = {};
	/* 256 is max length to print log about dbi_weights */
	char log_tmp[256] = {};

	adapt_ops = lcd_kit_get_adapt_ops();
	if (!adapt_ops->mipi_tx) {
		LCD_KIT_ERR("DE_FC adapt_ops->mipi_tx is NULL\n");
		return LCD_KIT_FAIL;
	}
	for (color = 0; color < DE_COLOR_INDEX_COUNT; color++) {
		for (lut_index = 0; lut_index < DDIC_COMPENSATION_LUT_COUNT / 2; lut_index++) {
			uint32_t index = lut_index << 1;
			uint32_t pl_idx = (lut_index * DE_COLOR_INDEX_COUNT) + 1;

			common_info->ddic_bic.weight_cmds.cmds[color + 1].payload[pl_idx] =
				((dbi_weight[color][index] >> 8) & 0xF) |
				((dbi_weight[color][index + 1] >> 4) & 0xF0);
			common_info->ddic_bic.weight_cmds.cmds[color + 1].payload[pl_idx + 1] =
				dbi_weight[color][index] & 0xFF;
			common_info->ddic_bic.weight_cmds.cmds[color + 1].payload[pl_idx + 2] =
				dbi_weight[color][index + 1] & 0xFF;
		}
	}
	ret = adapt_ops->mipi_tx(
		lcm_get_panel_info(display_engine_panel_id_to_lcdkit(panel_id))->display,
		&common_info->ddic_bic.weight_cmds);
	if (ret != LCD_KIT_OK)
		LCD_KIT_ERR("DE_FC mipi_tx failed, returen %d\n", ret);
	for (color = 0; color < DE_COLOR_INDEX_COUNT; color++) {
		strcat(log_tmp, "{ ");
		for (lut_index = 0; lut_index < DDIC_COMPENSATION_LUT_COUNT; lut_index++) {
			sprintf(weight_tmp, "%d ", dbi_weight[color][lut_index]);
			strcat(log_tmp, weight_tmp);
		}
		strcat(log_tmp, "}");
	}
	LCD_KIT_INFO("DE_FC {R G B} %s\n", log_tmp);
	return ret;
}

void display_engine_panel_on(uint32_t panel_id)
{
	if (g_de_common_context.aging_data.support) {
		g_de_common_context.aging_data.last_dbi_set_ret =
			display_engine_foldable_dbi_weight_set(panel_id,
			g_de_common_context.compensation_data.dbi_weight);
		display_engine_foldable_dbi_roi_set(panel_id,
			g_de_common_context.compensation_data.roi_param);
		LCD_KIT_INFO("DE_FC panel on rect top:%u, bottom:%u, left:%u, right:%u\n",
			g_de_common_context.compensation_data.roi_param.top,
			g_de_common_context.compensation_data.roi_param.bottom,
			g_de_common_context.compensation_data.roi_param.left,
			g_de_common_context.compensation_data.roi_param.right);
	}
}

static int display_engine_foldable_info_set(uint32_t panel_id,
	struct display_engine_foldable_info *param)
{
	uint16_t dbi_weight[DE_COLOR_INDEX_COUNT][DDIC_COMPENSATION_LUT_COUNT];
	int color;
	int ret = LCD_KIT_OK;

	if (!g_de_common_context.aging_data.support)
		return ret;
	if (!g_de_common_context.aging_data.is_inited) {
		LCD_KIT_INFO("DE_FC not init yet");
		return ret;
	}
	mutex_lock(&g_de_common_context.aging_data.lock);
	update_acc_value();
	update_last_time_value();
	LCD_KIT_INFO("DE_FC region_enable primary:%u, slave:%u, folding:%u\n",
		param->is_region_enable[DE_REGION_PRIMARY],
		param->is_region_enable[DE_REGION_SLAVE],
		param->is_region_enable[DE_REGION_FOLDING]);
	memcpy(g_de_common_context.aging_data.is_display_region_enable,
		param->is_region_enable, sizeof(g_de_common_context.aging_data.is_display_region_enable));
	mutex_unlock(&g_de_common_context.aging_data.lock);
	if (param->lut_count != DDIC_COMPENSATION_LUT_COUNT) {
		LCD_KIT_INFO("DE_FC lut_count %u\n", param->lut_count);
		return LCD_KIT_OK;
	}
	for (color = 0; color < DE_COLOR_INDEX_COUNT; color++) {
		ret = copy_from_user(dbi_weight[color], param->lut[color],
			param->lut_count * sizeof(uint16_t));
		if (ret != 0) {
			LCD_KIT_ERR("DE_FC copy weight, ret = %d\n", ret);
			return LCD_KIT_FAIL;
		}
	}
	if (memcmp(&g_de_common_context.compensation_data.dbi_weight, &dbi_weight,
		sizeof(dbi_weight)) != 0 ||
		g_de_common_context.aging_data.last_dbi_set_ret != LCD_KIT_OK) {
		memcpy(g_de_common_context.compensation_data.dbi_weight,
			dbi_weight, sizeof(g_de_common_context.compensation_data.dbi_weight));
		g_de_common_context.aging_data.last_dbi_set_ret =
			display_engine_foldable_dbi_weight_set(panel_id, dbi_weight);
	}

	if (memcmp(&g_de_common_context.compensation_data.roi_param, &param->roi_param,
		sizeof(struct display_engine_roi_param)) != 0) {
		LCD_KIT_INFO("DE_FC rect top:%u, bottom:%u, left:%u, right:%u\n",
			param->roi_param.top, param->roi_param.bottom,
			param->roi_param.left, param->roi_param.right);
		display_engine_foldable_dbi_roi_set(panel_id, param->roi_param);
		g_de_common_context.compensation_data.roi_param = param->roi_param;
	}
	return ret;
}

/* IO interfaces: */
int display_engine_get_param(struct drm_device *dev, void *data, struct drm_file *file_priv)
{
	struct display_engine_param *param = (struct display_engine_param *)data;
	bool is_fail = false;

	if (!param) {
		LCD_KIT_ERR("param is null\n");
		return LCD_KIT_FAIL;
	}

	if (param->modules & DISPLAY_ENGINE_DRM_HIST_ENABLE) {
		if (display_engine_drm_hist_get_enable(param->panel_id, &param->drm_hist)) {
			LCD_KIT_WARNING("display_engine_drm_hist_enable() failed!\n");
			is_fail = true;
		}
	}
	if (param->modules & DISPLAY_ENGINE_PANEL_INFO) {
		if (display_engine_panel_info_get(param->panel_id, &param->panel_info)) {
			LCD_KIT_WARNING("display_engine_panel_info_get() failed!\n");
			is_fail = true;
		}
	}
	if (param->modules & DISPLAY_ENGINE_FOLDABLE_INFO) {
		if (display_engine_foldable_info_get(param->panel_id, &param->foldable_info)) {
			LCD_KIT_WARNING("display_engine_foldable_info_get() failed!\n");
			is_fail = true;
		}
	}
	return is_fail ? LCD_KIT_FAIL : LCD_KIT_OK;
}

/* init of display engine kernel */
/* workqueue init */
static int display_engine_workqueue_init(uint32_t panel_id)
{
	struct qcom_panel_info *panel_info = lcm_get_panel_info(panel_id);

	if (!panel_info) {
		LCD_KIT_ERR("panel_info is NULL!\n");
		return LCD_KIT_FAIL;
	}

	panel_info->brightness_workqueue =
		create_singlethread_workqueue("display_engine_brightness");
	panel_info->vblank_workqueue =
		create_singlethread_workqueue("display_engine_vblank");
	if (!panel_info->brightness_workqueue || !panel_info->vblank_workqueue) {
		LCD_KIT_ERR("workqueue is NULL!\n");
		return LCD_KIT_FAIL;
	}

	INIT_WORK(&panel_info->backlight_work,
		display_engine_brightness_handle_fingerprint_hbm_work);
	INIT_WORK(&panel_info->vblank_work, display_engine_brightness_handle_vblank_work);
	INIT_WORK(&panel_info->backlight_update_work,
		display_engine_brightness_handle_backlight_update);
	LCD_KIT_INFO("workqueue inited\n");
	return LCD_KIT_OK;
}

/* Common context struct initialization */
static int display_engine_common_context_init(uint32_t panel_id)
{
	g_de_common_context.aging_data.support = common_info->dbv_stat_support;
	g_de_common_context.aging_data.min_hbm_dbv = common_info->min_hbm_dbv;
	if (g_de_common_context.aging_data.support) {
		mutex_init(&g_de_common_context.aging_data.lock);
		g_de_common_context.aging_data.is_inited = true;
		update_last_time_value();
	}
	return LCD_KIT_OK;
}

static int get_grayscale_index(int grayscale, const struct gamma_node_info *gamma_info, int *index)
{
	int i;
	int cnt;

	if (!gamma_info || !index) {
		LCD_KIT_ERR("NULL pointer\n");
		return LCD_KIT_FAIL;
	}
	cnt = gamma_info->node_grayscale.cnt;
	if (grayscale <= gamma_info->node_grayscale.buf[0]) {
		*index = 1;
	} else if (grayscale >= gamma_info->node_grayscale.buf[cnt - 1]) {
		*index = cnt - 1;
	} else {
		for (i = 0; i < cnt; i++) {
			LCD_KIT_DEBUG("grayscale[%d]:%d\n", i, gamma_info->node_grayscale.buf[i]);
			if (grayscale <= gamma_info->node_grayscale.buf[i]) {
				*index = i;
				break;
			}
		}
	}
	return LCD_KIT_OK;
}

static int linear_interpolation_calculation_gamma(
	struct gamma_linear_interpolation_info *gamma_liner_info)
{
	int gamma_value = -1;

	if (!gamma_liner_info) {
		LCD_KIT_ERR("gamma_liner_info is NULL\n");
		return LCD_KIT_FAIL;
	}

	LCD_KIT_DEBUG("grayscale(before:%d, self:%d, after:%d)\n",
		gamma_liner_info->grayscale_before, gamma_liner_info->grayscale,
		gamma_liner_info->grayscale_after);
	if ((gamma_liner_info->grayscale_before - gamma_liner_info->grayscale_after) == 0) {
		gamma_liner_info->gamma_node_value = gamma_liner_info->gamma_node_value_before;
	} else {
		/* Multiply by 100 to avoid loss of precision, plus 50 to complete rounding */
		gamma_value = (100 * ((int)gamma_liner_info->grayscale -
			(int)gamma_liner_info->grayscale_after) *
			((int)gamma_liner_info->gamma_node_value_before -
			(int)gamma_liner_info->gamma_node_value_after) /
			((int)gamma_liner_info->grayscale_before -
			(int)gamma_liner_info->grayscale_after)) +
			(100 * ((int)gamma_liner_info->gamma_node_value_after));
		gamma_liner_info->gamma_node_value = (uint32_t)(gamma_value + 50) / 100;
	}
	LCD_KIT_DEBUG("gamma_node_value(before:%d, self:%d,after:%d), gamma_value:%d\n",
		gamma_liner_info->gamma_node_value_before, gamma_liner_info->gamma_node_value,
		gamma_liner_info->gamma_node_value_after, gamma_value);
	return LCD_KIT_OK;
}

static int set_gamma_liner_info(const struct gamma_node_info *gamma_info,
	const uint32_t *gamma_value_array, size_t array_size,
	struct gamma_linear_interpolation_info *gamma_liner_info, int grayscale)
{
	int index;
	int ret;
	int i;

	if (!gamma_info || !gamma_liner_info || !gamma_value_array) {
		LCD_KIT_ERR("NULL pointer\n");
		return LCD_KIT_FAIL;
	}
	for (i = 0; i < array_size; i++)
		LCD_KIT_DEBUG("gamma_value_array[%d]:%d\n", i, gamma_value_array[i]);
	ret = get_grayscale_index(grayscale, gamma_info, &index);
	if (ret != LCD_KIT_OK) {
		LCD_KIT_ERR("get_grayscale_index error\n");
		return LCD_KIT_FAIL;
	}
	if ((((uint32_t)index) >= array_size) || (index >= gamma_info->node_grayscale.cnt)) {
		LCD_KIT_ERR("index out of range, index:%d, node_gc.cnt:%d, gamma_array size:%d\n",
			index, gamma_info->node_grayscale.cnt, array_size);
		return LCD_KIT_FAIL;
	}
	gamma_liner_info->grayscale_before = gamma_info->node_grayscale.buf[index - 1];
	gamma_liner_info->grayscale = grayscale;
	gamma_liner_info->grayscale_after = gamma_info->node_grayscale.buf[index];
	gamma_liner_info->gamma_node_value_before = gamma_value_array[index - 1];
	gamma_liner_info->gamma_node_value_after = gamma_value_array[index];
	LCD_KIT_DEBUG("index:%d, gamma_node_value_before:%d, gamma_node_value_after:%d\n",
		index, gamma_liner_info->gamma_node_value_before,
		gamma_liner_info->gamma_node_value_after);
	return ret;
}

static int set_color_cmds_value_by_grayscale(const struct gamma_node_info *gamma_info,
	const uint32_t *gamma_value_array, size_t array_size, int *payload, int grayscale)
{
	int ret = LCD_KIT_OK;
	struct gamma_linear_interpolation_info gamma_liner_info;

	if (!payload) {
		LCD_KIT_ERR("payload is NULL\n");
		return LCD_KIT_FAIL;
	}
	ret = set_gamma_liner_info(gamma_info, gamma_value_array, array_size, &gamma_liner_info,
		grayscale);
	if (ret != LCD_KIT_OK) {
		LCD_KIT_ERR("set_gamma_liner_info error\n");
		return LCD_KIT_FAIL;
	}
	linear_interpolation_calculation_gamma(&gamma_liner_info);

	/* High 8 bits correspond payload[0], low 8 bits correspond payload[1] */
	payload[0] = (gamma_liner_info.gamma_node_value >> 8) & 0xff;
	payload[1] = gamma_liner_info.gamma_node_value & 0xff;
	LCD_KIT_DEBUG("payload[1]:%d, payload[2]:%d\n", payload[0], payload[1]);
	return ret;
}

static int set_color_cmds_by_grayscale(uint32_t fps, struct color_cmds_rgb *color_cmds,
	const struct gamma_node_info *gamma_info, int grayscale)
{
	LCD_KIT_DEBUG("grayscale = %d\n", grayscale);
	if (!color_cmds) {
		LCD_KIT_ERR("color_cmds is NULL\n");
		return LCD_KIT_FAIL;
	}
	if (!gamma_info) {
		LCD_KIT_ERR("gamma_info is NULL\n");
		return LCD_KIT_FAIL;
	}
	if (fps == FPS_LEVEL1) {
		if (set_color_cmds_value_by_grayscale(gamma_info, gamma_info->red_60_hz,
			HBM_GAMMA_NODE_SIZE, color_cmds->red_payload, grayscale) ||
			set_color_cmds_value_by_grayscale(gamma_info, gamma_info->green_60_hz,
			HBM_GAMMA_NODE_SIZE, color_cmds->green_payload, grayscale) ||
			set_color_cmds_value_by_grayscale(gamma_info, gamma_info->blue_60_hz,
			HBM_GAMMA_NODE_SIZE, color_cmds->blue_payload, grayscale)) {
			LCD_KIT_ERR("set_color_cmds_value error\n");
			return LCD_KIT_FAIL;
		}
	} else if (fps == FPS_LEVEL2) {
		if (set_color_cmds_value_by_grayscale(gamma_info, gamma_info->red_90_hz,
			HBM_GAMMA_NODE_SIZE, color_cmds->red_payload, grayscale) ||
			set_color_cmds_value_by_grayscale(gamma_info, gamma_info->green_90_hz,
			HBM_GAMMA_NODE_SIZE, color_cmds->green_payload, grayscale) ||
			set_color_cmds_value_by_grayscale(gamma_info, gamma_info->blue_90_hz,
			HBM_GAMMA_NODE_SIZE, color_cmds->blue_payload, grayscale)) {
			LCD_KIT_ERR("set_color_cmds_value error\n");
			return LCD_KIT_FAIL;
		}
	} else if (fps == FPS_LEVEL3) {
		if (set_color_cmds_value_by_grayscale(gamma_info, gamma_info->red_120_hz,
			HBM_GAMMA_NODE_SIZE, color_cmds->red_payload, grayscale) ||
			set_color_cmds_value_by_grayscale(gamma_info, gamma_info->green_120_hz,
			HBM_GAMMA_NODE_SIZE, color_cmds->green_payload, grayscale) ||
			set_color_cmds_value_by_grayscale(gamma_info, gamma_info->blue_120_hz,
			HBM_GAMMA_NODE_SIZE, color_cmds->blue_payload, grayscale)) {
			LCD_KIT_ERR("set_color_cmds_value error\n");
			return LCD_KIT_FAIL;
		}
	} else {
		LCD_KIT_ERR("fps not support, fps:%d\n", fps);
		return LCD_KIT_FAIL;
	}
	return LCD_KIT_OK;
}

static int set_gamma_node_60hz_info(uint32_t panel_id, struct gamma_node_info *gamma_info)
{
	int i;
	int cnt;

	if (!gamma_info) {
		LCD_KIT_ERR("gamma_info is NULL\n");
		return LCD_KIT_FAIL;
	}
	cnt = gamma_info->node_grayscale.cnt;

	/* 2 hbm_gamma_values are converted to 1 gamma_node_value. */
	if ((cnt > HBM_GAMMA_NODE_SIZE) || ((2 * cnt) > HBM_GAMMA_SIZE)) {
		LCD_KIT_ERR("cnt out of range, cnt:%d, GAMMA_NODE_SIZE:%d, GAMMA_SIZE:%d\n",
			cnt, HBM_GAMMA_NODE_SIZE, HBM_GAMMA_SIZE);
		return LCD_KIT_FAIL;
	}

	/* The high 8 bits of a hex number are converted into a dec number multiplied by 256 */
	for (i = 0; i < cnt; i++) {
		gamma_info->red_60_hz[i] = (common_info->hbm.hbm_gamma.red_60_hz[i * 2] * 256) +
			common_info->hbm.hbm_gamma.red_60_hz[(i * 2) + 1];
		LCD_KIT_INFO("gamma_info->red_60_hz[%d]:%d\n", i, gamma_info->red_60_hz[i]);
	}
	for (i = 0; i < cnt; i++) {
		gamma_info->green_60_hz[i] = (common_info->hbm.hbm_gamma.green_60_hz[i * 2] * 256) +
			common_info->hbm.hbm_gamma.green_60_hz[(i * 2) + 1];
		LCD_KIT_INFO("gamma_info->green_60_hz[%d]:%d\n", i, gamma_info->green_60_hz[i]);
	}
	for (i = 0; i < cnt; i++) {
		gamma_info->blue_60_hz[i] = (common_info->hbm.hbm_gamma.blue_60_hz[i * 2] * 256) +
			common_info->hbm.hbm_gamma.blue_60_hz[(i * 2) + 1];
		LCD_KIT_INFO("gamma_info->blue_60_hz[%d]:%d\n", i, gamma_info->blue_60_hz[i]);
	}
	return LCD_KIT_OK;
}

static int set_gamma_node_90hz_info(uint32_t panel_id, struct gamma_node_info *gamma_info)
{
	int i;
	int cnt;

	if (!gamma_info) {
		LCD_KIT_ERR("gamma_info is NULL\n");
		return LCD_KIT_FAIL;
	}
	cnt = gamma_info->node_grayscale.cnt;

	/* 2 hbm_gamma_values are converted to 1 gamma_node_value. */
	if ((cnt > HBM_GAMMA_NODE_SIZE) || ((2 * cnt) > HBM_GAMMA_SIZE)) {
		LCD_KIT_ERR("cnt out of range, cnt:%d, GAMMA_NODE_SIZE:%d, GAMMA_SIZE:%d\n",
			cnt, HBM_GAMMA_NODE_SIZE, HBM_GAMMA_SIZE);
		return LCD_KIT_FAIL;
	}

	/* The high 8 bits of a hex number are converted into a dec number multiplied by 256 */
	for (i = 0; i < cnt; i++) {
		gamma_info->red_90_hz[i] = (common_info->hbm.hbm_gamma.red_90_hz[i * 2] * 256) +
			common_info->hbm.hbm_gamma.red_90_hz[(i * 2) + 1];
		LCD_KIT_INFO("gamma_info->red_90_hz[%d]:%d\n", i, gamma_info->red_90_hz[i]);
	}
	for (i = 0; i < cnt; i++) {
		gamma_info->green_90_hz[i] = (common_info->hbm.hbm_gamma.green_90_hz[i * 2] * 256) +
			common_info->hbm.hbm_gamma.green_90_hz[(i * 2) + 1];
		LCD_KIT_INFO("gamma_info->green_90_hz[%d]:%d\n", i, gamma_info->green_90_hz[i]);
	}
	for (i = 0; i < cnt; i++) {
		gamma_info->blue_90_hz[i] = (common_info->hbm.hbm_gamma.blue_90_hz[i * 2] * 256) +
			common_info->hbm.hbm_gamma.blue_90_hz[(i * 2) + 1];
		LCD_KIT_INFO("gamma_info->blue_90_hz[%d]:%d\n", i, gamma_info->blue_90_hz[i]);
	}
	return LCD_KIT_OK;
}

static int set_gamma_node_120hz_info(uint32_t panel_id, struct gamma_node_info *gamma_info)
{
	int i;
	int cnt;

	if (!gamma_info) {
		LCD_KIT_ERR("gamma_info is NULL\n");
		return LCD_KIT_FAIL;
	}
	cnt = gamma_info->node_grayscale.cnt;

	/* 2 hbm_gamma_values are converted to 1 gamma_node_value. */
	if ((cnt > HBM_GAMMA_NODE_SIZE) || ((2 * cnt) > HBM_GAMMA_SIZE)) {
		LCD_KIT_ERR("cnt out of range, cnt:%d, GAMMA_NODE_SIZE:%d, GAMMA_SIZE:%d\n",
			cnt, HBM_GAMMA_NODE_SIZE, HBM_GAMMA_SIZE);
		return LCD_KIT_FAIL;
	}

	/* The high 8 bits of a hex number are converted into a dec number multiplied by 256 */
	for (i = 0; i < cnt; i++) {
		gamma_info->red_120_hz[i] = (common_info->hbm.hbm_gamma.red_120_hz[i * 2] * 256) +
			common_info->hbm.hbm_gamma.red_120_hz[(i * 2) + 1];
		LCD_KIT_INFO("gamma_info->red_120_hz[%d]:%d\n", i, gamma_info->red_120_hz[i]);
	}
	for (i = 0; i < cnt; i++) {
		gamma_info->green_120_hz[i] =
			(common_info->hbm.hbm_gamma.green_120_hz[i * 2] * 256) +
			common_info->hbm.hbm_gamma.green_120_hz[(i * 2) + 1];
		LCD_KIT_INFO("gamma_info->green_120_hz[%d]:%d\n", i, gamma_info->green_120_hz[i]);
	}
	for (i = 0; i < cnt; i++) {
		gamma_info->blue_120_hz[i] = (common_info->hbm.hbm_gamma.blue_120_hz[i * 2] * 256) +
			common_info->hbm.hbm_gamma.blue_120_hz[(i * 2) + 1];
		LCD_KIT_INFO("gamma_info->blue_120_hz[%d]:%d\n", i, gamma_info->blue_120_hz[i]);
	}
	return LCD_KIT_OK;
}

static void print_hbm_gamma(uint8_t *hbm_gamma, size_t rgb_size, int frame_rate)
{
	int i;

	LCD_KIT_INFO("hbm %dhz gamma\n", frame_rate);
	/* if more than 4 values, print 4 values */
	if (rgb_size > 4) {
		/* 0:the first of the array,1:the second,-2:the penultimate,-1:the last */
		LCD_KIT_INFO("hbm_gamma[0] = 0x%02X, [1] = 0x%02X, [%d] = 0x%02X, [%d] = 0x%02X\n",
			hbm_gamma[0], hbm_gamma[1], rgb_size - 2, hbm_gamma[rgb_size - 2],
			rgb_size - 1, hbm_gamma[rgb_size - 1]);
	} else {
		for (i = 0; i < rgb_size; i++)
			LCD_KIT_INFO("hbm 120 gamma %d = 0x%02X\n", i, hbm_gamma[i]);
	}
}

static int set_hbm_gamma_60hz_to_common_info(uint32_t panel_id, uint8_t *hbm_gamma, size_t rgb_size)
{
	int i;

	if (!hbm_gamma) {
		LCD_KIT_ERR("hbm_gamma is NULL\n");
		return LCD_KIT_FAIL;
	}

	if ((GAMMA_INDEX_BLUE_HIGH + HBM_GAMMA_SIZE) > rgb_size) {
		LCD_KIT_ERR("hbm_gamma hbm_gamma index out of range\n");
		return LCD_KIT_FAIL;
	}

	/* The 2 values correspond to each other */
	for (i = 0; i < (HBM_GAMMA_SIZE / 2); i++) {
		common_info->hbm.hbm_gamma.red_60_hz[i * 2] =
			hbm_gamma[GAMMA_INDEX_RED_HIGH + (i * 2)];
		common_info->hbm.hbm_gamma.red_60_hz[(i * 2) + 1] =
			hbm_gamma[GAMMA_INDEX_RED_LOW + (i * 2)];
		common_info->hbm.hbm_gamma.green_60_hz[i * 2] =
			hbm_gamma[GAMMA_INDEX_GREEN_HIGH + (i * 2)];
		common_info->hbm.hbm_gamma.green_60_hz[(i * 2) + 1] =
			hbm_gamma[GAMMA_INDEX_GREEN_LOW + (i * 2)];
		common_info->hbm.hbm_gamma.blue_60_hz[i * 2] =
			hbm_gamma[GAMMA_INDEX_BLUE_HIGH + (i * 2)];
		common_info->hbm.hbm_gamma.blue_60_hz[(i * 2) + 1] =
			hbm_gamma[GAMMA_INDEX_BLUE_LOW + (i * 2)];
	}

	print_hbm_gamma(hbm_gamma, rgb_size, FPS_LEVEL1);
	return LCD_KIT_OK;
}

static int set_hbm_gamma_90hz_to_common_info(uint32_t panel_id, uint8_t *hbm_gamma, size_t rgb_size)
{
	int i;

	if (!hbm_gamma) {
		LCD_KIT_ERR("hbm_gamma is NULL\n");
		return LCD_KIT_FAIL;
	}
	if ((GAMMA_INDEX_BLUE_HIGH + HBM_GAMMA_SIZE) > rgb_size) {
		LCD_KIT_ERR("hbm_gamma index out of range\n");
		return LCD_KIT_FAIL;
	}

	// The 2 values correspond to each other
	for (i = 0; i < (HBM_GAMMA_SIZE / 2); i++) {
		common_info->hbm.hbm_gamma.red_90_hz[i * 2] =
			hbm_gamma[GAMMA_INDEX_RED_HIGH + (i * 2)];
		common_info->hbm.hbm_gamma.red_90_hz[(i * 2) + 1] =
			hbm_gamma[GAMMA_INDEX_RED_LOW + (i * 2)];
		common_info->hbm.hbm_gamma.green_90_hz[i * 2] =
			hbm_gamma[GAMMA_INDEX_GREEN_HIGH + (i * 2)];
		common_info->hbm.hbm_gamma.green_90_hz[(i * 2) + 1] =
			hbm_gamma[GAMMA_INDEX_GREEN_LOW + (i * 2)];
		common_info->hbm.hbm_gamma.blue_90_hz[i * 2] =
			hbm_gamma[GAMMA_INDEX_BLUE_HIGH + (i * 2)];
		common_info->hbm.hbm_gamma.blue_90_hz[(i * 2) + 1] =
			hbm_gamma[GAMMA_INDEX_BLUE_LOW + (i * 2)];
	}

	print_hbm_gamma(hbm_gamma, rgb_size, FPS_LEVEL2);
	return LCD_KIT_OK;
}

static int set_hbm_gamma_120hz_to_common_info(uint32_t panel_id, uint8_t *hbm_gamma,
	size_t rgb_size)
{
	int i;

	if (!hbm_gamma) {
		LCD_KIT_ERR("hbm_gamma is NULL\n");
		return LCD_KIT_FAIL;
	}
	if ((GAMMA_INDEX_BLUE_HIGH + HBM_GAMMA_SIZE) > rgb_size) {
		LCD_KIT_ERR("hbm_gamma index out of range\n");
		return LCD_KIT_FAIL;
	}

	// The 2 values correspond to each other
	for (i = 0; i < (HBM_GAMMA_SIZE / 2); i++) {
		common_info->hbm.hbm_gamma.red_120_hz[i * 2] =
			hbm_gamma[GAMMA_INDEX_RED_HIGH + (i * 2)];
		common_info->hbm.hbm_gamma.red_120_hz[(i * 2) + 1] =
			hbm_gamma[GAMMA_INDEX_RED_LOW + (i * 2)];
		common_info->hbm.hbm_gamma.green_120_hz[i * 2] =
			hbm_gamma[GAMMA_INDEX_GREEN_HIGH + (i * 2)];
		common_info->hbm.hbm_gamma.green_120_hz[(i * 2) + 1] =
			hbm_gamma[GAMMA_INDEX_GREEN_LOW + (i * 2)];
		common_info->hbm.hbm_gamma.blue_120_hz[i * 2] =
			hbm_gamma[GAMMA_INDEX_BLUE_HIGH + (i * 2)];
		common_info->hbm.hbm_gamma.blue_120_hz[(i * 2) + 1] =
			hbm_gamma[GAMMA_INDEX_BLUE_LOW + (i * 2)];
	}

	print_hbm_gamma(hbm_gamma, rgb_size, FPS_LEVEL3);
	return LCD_KIT_OK;
}

/* Entry of display engine init */
void display_engine_init(uint32_t panel_id)
{
	struct display_engine_context *de_context =
		get_de_context(display_engine_panel_id_to_de(panel_id));

	LCD_KIT_INFO("+\n");
	if (!de_context) {
		LCD_KIT_ERR("de_context is null\n");
		return;
	}
	/* init workqueue */
	if (display_engine_workqueue_init(panel_id) != LCD_KIT_OK) {
		LCD_KIT_ERR("workqueue_init failed, exit.\n");
		return;
	}
	de_context->is_inited = true;
	/* Initialize display_engine_common_context g_de_common_context */
	if (display_engine_common_context_init(panel_id) != LCD_KIT_OK) {
		LCD_KIT_ERR("common_context init failed, exit.\n");
		return;
	}
	LCD_KIT_INFO("-\n");
}

void display_engine_compensation_set_dbv(uint32_t oir_level, uint32_t mipi_level, uint32_t panel_id)
{
	if (!g_de_common_context.aging_data.support)
		return;
	if (!g_de_common_context.aging_data.is_inited) {
		LCD_KIT_INFO("DE_FC not init yet");
		return;
	}
	/* aging_data.lock is just lock for aging_data updating */
	mutex_lock(&g_de_common_context.aging_data.lock);
	if (g_de_common_context.aging_data.mipi_level == 0) {
		update_last_time_value();
		g_de_common_context.aging_data.mipi_level = mipi_level;
		g_de_common_context.aging_data.orig_level = oir_level;
		mutex_unlock(&g_de_common_context.aging_data.lock);
		return;
	}
	update_acc_value();
	update_last_time_value();
	g_de_common_context.aging_data.mipi_level = mipi_level;
	g_de_common_context.aging_data.orig_level = oir_level;
	mutex_unlock(&g_de_common_context.aging_data.lock);
}

void display_engine_compensation_set_fps(uint32_t panel_id)
{
	unsigned int fps_index;

	if (!g_de_common_context.aging_data.support)
		return;
	if (!g_de_common_context.aging_data.is_inited) {
		LCD_KIT_INFO("DE_FC not init yet");
		return;
	}
	fps_index = get_fps_index(panel_id);
	if (g_de_common_context.aging_data.fps_index == fps_index)
		return;
	mutex_lock(&g_de_common_context.aging_data.lock);
	update_acc_value();
	update_last_time_value();
	LCD_KIT_INFO("DE_FC fps_index update:[%u]->[%u]",
		g_de_common_context.aging_data.fps_index, fps_index);
	g_de_common_context.aging_data.fps_index = fps_index;
	mutex_unlock(&g_de_common_context.aging_data.lock);
}

static int display_engine_gamma_code_print(uint32_t panel_id)
{
	/* circle_color_cmds.cmd_cnt's maximum value is 3 */
	if (common_info->hbm.circle_color_cmds.cmd_cnt != 3) {
		LCD_KIT_ERR("circle_color_cmds.cmd_cnt does not match\n");
		return LCD_KIT_FAIL;
	}

	if ((int)common_info->hbm.circle_color_cmds.cmds[color_cmd_index].dlen <
		CMDS_COLOR_MAX) {
		LCD_KIT_ERR("array index out of range, cmds[1].dlen:%d\n",
			common_info->hbm.circle_color_cmds.cmds[color_cmd_index].dlen);
		return LCD_KIT_FAIL;
	}
	LCD_KIT_DEBUG("circle_color_cmds.cmds[1].payload[9]=%d\n",
		common_info->hbm.circle_color_cmds.cmds[color_cmd_index].payload[CMDS_RED_HIGH]);
	LCD_KIT_DEBUG("circle_color_cmds.cmds[1].payload[10]=%d\n",
		common_info->hbm.circle_color_cmds.cmds[color_cmd_index].payload[CMDS_RED_LOW]);
	LCD_KIT_DEBUG("circle_color_cmds.cmds[1].payload[11]=%d\n",
		common_info->hbm.circle_color_cmds.cmds[color_cmd_index].payload[CMDS_GREEN_HIGH]);
	LCD_KIT_DEBUG("circle_color_cmds.cmds[1].payload[12]=%d\n",
		common_info->hbm.circle_color_cmds.cmds[color_cmd_index].payload[CMDS_GREEN_LOW]);
	LCD_KIT_DEBUG("circle_color_cmds.cmds[1].payload[13]=%d\n",
		common_info->hbm.circle_color_cmds.cmds[color_cmd_index].payload[CMDS_BLUE_HIGH]);
	LCD_KIT_DEBUG("circle_color_cmds.cmds[1].payload[14]=%d\n",
		common_info->hbm.circle_color_cmds.cmds[color_cmd_index].payload[CMDS_BLUE_LOW]);
	return LCD_KIT_OK;
}

static int display_engine_set_em_configure(uint32_t panel_id, void *hld, uint32_t fps)
{
	int ret;
	struct lcd_kit_adapt_ops *adapt_ops = NULL;

	adapt_ops = lcd_kit_get_adapt_ops();
	if (!adapt_ops || !adapt_ops->mipi_tx) {
		LCD_KIT_ERR("can not register adapt_ops!\n");
		return LCD_KIT_FAIL;
	}
	if (fps == FPS_LEVEL1) {
		ret = adapt_ops->mipi_tx(hld, &(common_info->hbm.hbm_em_configure_60hz_cmds));
	} else if (fps == FPS_LEVEL2) {
		ret = adapt_ops->mipi_tx(hld, &(common_info->hbm.hbm_em_configure_90hz_cmds));
	} else if (fps == FPS_LEVEL3) {
		ret = adapt_ops->mipi_tx(hld, &(common_info->hbm.hbm_em_configure_120hz_cmds));
	} else {
		LCD_KIT_ERR("fps not support, fps:%d\n", fps);
		return LCD_KIT_FAIL;
	}
	if (ret != LCD_KIT_OK)
		LCD_KIT_ERR("mipi_tx failed! fps = %d, ret = %d\n", fps, ret);

	return ret;
}

static int display_engine_copy_color_cmds_to_common_info(uint32_t panel_id,
	struct color_cmds_rgb *color_cmds)
{
	if (!color_cmds) {
		LCD_KIT_ERR("color_cmds is NULL\n");
		return LCD_KIT_FAIL;
	}

	/* circle_color_cmds.cmd_cnt's maximum value is 3 */
	if (common_info->hbm.circle_color_cmds.cmd_cnt != 3) {
		LCD_KIT_ERR("circle_color_cmds.cmd_cnt does not match\n");
		return LCD_KIT_FAIL;
	}

	if ((int)common_info->hbm.circle_color_cmds.cmds[color_cmd_index].dlen <
		CMDS_COLOR_MAX) {
		LCD_KIT_ERR("array index out of range, cmds[1].dlen:%d\n",
			common_info->hbm.circle_color_cmds.cmds[color_cmd_index].dlen);
		return LCD_KIT_FAIL;
	}

	common_info->hbm.circle_color_cmds.cmds[color_cmd_index].payload[CMDS_RED_HIGH] =
		color_cmds->red_payload[0];
	common_info->hbm.circle_color_cmds.cmds[color_cmd_index].payload[CMDS_RED_LOW] =
		color_cmds->red_payload[1];
	common_info->hbm.circle_color_cmds.cmds[color_cmd_index].payload[CMDS_GREEN_HIGH] =
		color_cmds->green_payload[0];
	common_info->hbm.circle_color_cmds.cmds[color_cmd_index].payload[CMDS_GREEN_LOW] =
		color_cmds->green_payload[1];
	common_info->hbm.circle_color_cmds.cmds[color_cmd_index].payload[CMDS_BLUE_HIGH] =
		color_cmds->blue_payload[0];
	common_info->hbm.circle_color_cmds.cmds[color_cmd_index].payload[CMDS_BLUE_LOW] =
		color_cmds->blue_payload[1];

	return LCD_KIT_OK;
}

static int display_engine_local_hbm_set_color_by_grayscale(uint32_t panel_id, void *hld,
	uint32_t fps, int grayscale)
{
	int ret;
	struct color_cmds_rgb color_cmds;
	struct lcd_kit_adapt_ops *adapt_ops = NULL;

	if (common_info->hbm.circle_color_cmds.cmds == NULL) {
		LCD_KIT_ERR("circle_color_cmds.cmds is NULL\n");
		return LCD_KIT_FAIL;
	}

	LCD_KIT_INFO("grayscale = %d\n", grayscale);
	ret = set_color_cmds_by_grayscale(fps, &color_cmds, &common_info->hbm.gamma_info,
		grayscale);
	if (ret != LCD_KIT_OK) {
		LCD_KIT_ERR("set_color_cmds error\n");
		return LCD_KIT_FAIL;
	}

	ret = display_engine_set_em_configure(panel_id, hld, fps);
	if (ret != LCD_KIT_OK) {
		LCD_KIT_ERR("display_engine_set_em_configure error\n");
		return LCD_KIT_FAIL;
	}
	ret = display_engine_copy_color_cmds_to_common_info(panel_id, &color_cmds);
	if (ret != LCD_KIT_OK) {
		LCD_KIT_ERR("display_engine_copy_color_cmds_to_common_info error\n");
		return LCD_KIT_FAIL;
	}
	ret = display_engine_gamma_code_print(panel_id);
	if (ret != LCD_KIT_OK) {
		LCD_KIT_ERR("display_engine_gamma_code_print error\n");
		return LCD_KIT_FAIL;
	}
	adapt_ops = lcd_kit_get_adapt_ops();
	if (!adapt_ops || !adapt_ops->mipi_tx) {
		LCD_KIT_ERR("can not register adapt_ops!\n");
		return LCD_KIT_FAIL;
	}

	ret = adapt_ops->mipi_tx(hld, &(common_info->hbm.circle_color_cmds));
	if (ret != LCD_KIT_OK)
		LCD_KIT_ERR("mipi_tx failed! ret = %d\n", ret);
	return ret;
}

static int display_engine_local_hbm_set_dbv(uint32_t panel_id, void *hld, uint32_t dbv)
{
	int ret;
	struct lcd_kit_adapt_ops *adapt_ops = NULL;

	if (!hld) {
		LCD_KIT_ERR("hld is NULL\n");
		return LCD_KIT_FAIL;
	}
	LCD_KIT_DEBUG("set dbv, dbv value:%d\n", dbv);
	if (common_info->hbm.hbm_dbv_cmds.cmds == NULL) {
		LCD_KIT_ERR("hbm_dbv_cmds.cmds is NULL\n");
		return LCD_KIT_FAIL;
	}

	/* hbm_dbv_cmds.cmd_cnt's maximum value is 2 */
	if (common_info->hbm.hbm_dbv_cmds.cmd_cnt != 2) {
		LCD_KIT_ERR("hbm_dbv_cmds.cmd_cnt does not match\n");
		return LCD_KIT_FAIL;
	}

	/* 25 is the 16th of the payload of dbv */
	if ((int)common_info->hbm.hbm_dbv_cmds.cmds[1].dlen <= 25) {
		LCD_KIT_ERR("array index out of range, cmds[1].dlen:%d\n",
			common_info->hbm.hbm_dbv_cmds.cmds[1].dlen);
		return LCD_KIT_FAIL;
	}

	/* 24 and 25 are dbv payloads */
	common_info->hbm.hbm_dbv_cmds.cmds[1].payload[24] = (dbv >> 8) & 0x0f;
	common_info->hbm.hbm_dbv_cmds.cmds[1].payload[25] = dbv & 0xff;
	adapt_ops = lcd_kit_get_adapt_ops();
	if (!adapt_ops || !adapt_ops->mipi_tx) {
		LCD_KIT_ERR("can not register adapt_ops!\n");
		return LCD_KIT_FAIL;
	}
	ret = adapt_ops->mipi_tx(hld, &common_info->hbm.hbm_dbv_cmds);
	if (ret != LCD_KIT_OK)
		LCD_KIT_ERR("mipi_tx failed! ret = %d\n", ret);
	return ret;
}

static int display_engine_local_hbm_update_alpha(uint32_t panel_id, void *hld, uint32_t alpha)
{
	int payload_num;
	int cmd_cnt;
	struct lcd_kit_dsi_panel_cmds *circle_cmds = &common_info->hbm.enter_circle_cmds;

	if (circle_cmds->cmds == NULL) {
		LCD_KIT_ERR("circle_cmds->cmds is NULL\n");
		return LCD_KIT_FAIL;
	}
	cmd_cnt = circle_cmds->cmd_cnt;
	if (cmd_cnt <= 0 || cmd_cnt > cmd_cnt_max) {
		LCD_KIT_ERR("circle_cmds's cmd_cnt is invalid, cmd_cnt:%d\n", cmd_cnt);
		return LCD_KIT_FAIL;
	}
	payload_num = (int)circle_cmds->cmds[cmd_cnt - 1].dlen;
	if (payload_num < payload_num_min || payload_num > payload_num_max) {
		LCD_KIT_ERR("circle_cmds->cmds's payload_num is invalid, payload_num:%d\n",
			payload_num);
		return LCD_KIT_FAIL;
	}
	LCD_KIT_DEBUG("cmd_cnt = %d, payload_num = %d", cmd_cnt, payload_num);
	/* Sets the upper eight bits of the alpha. */
	if (common_info->ddic_alpha.alpha_with_enable_flag == 0)
		circle_cmds->cmds[cmd_cnt - 1].payload[payload_num - 3] = (alpha >> 8) & 0x0f;
	else
		circle_cmds->cmds[cmd_cnt - 1].payload[payload_num - 3] =
			((alpha >> 8) & 0x0f) | 0x10;

	/* Sets the lower eight bits of the alpha. */
	circle_cmds->cmds[cmd_cnt - 1].payload[payload_num - 2] = alpha & 0xff;

	return LCD_KIT_OK;
}

static int display_engine_local_hbm_set_circle(uint32_t panel_id, void *hld, bool is_on,
	uint32_t alpha)
{
	int ret;
	struct lcd_kit_adapt_ops *adapt_ops = NULL;

	if (!hld) {
		LCD_KIT_ERR("hld is null\n");
		return LCD_KIT_FAIL;
	}
	if (is_on)
		display_engine_local_hbm_update_alpha(panel_id, hld, alpha);

	adapt_ops = lcd_kit_get_adapt_ops();
	if (!adapt_ops || !adapt_ops->mipi_tx) {
		LCD_KIT_ERR("can not register adapt_ops!\n");
		return LCD_KIT_FAIL;
	}
	if (!is_on) {
		LCD_KIT_DEBUG("circle exit\n");
		if (common_info->hbm.exit_circle_cmds.cmds == NULL) {
			LCD_KIT_ERR("exit_circle_cmds.cmds is NULL\n");
			return LCD_KIT_FAIL;
		}
		ret = adapt_ops->mipi_tx(hld, &common_info->hbm.exit_circle_cmds);
	} else {
		if (common_info->hbm.enter_circle_cmds.cmds == NULL) {
			LCD_KIT_ERR("exit_circle_cmds.cmds is NULL\n");
			return LCD_KIT_FAIL;
		}
		LCD_KIT_DEBUG("circle enter\n");
		ret = adapt_ops->mipi_tx(hld, &common_info->hbm.enter_circle_cmds);
	}
	return ret;
}

int display_engine_enter_ddic_alpha(uint32_t panel_id, void *hld, uint32_t alpha)
{
	int ret;
	int payload_num;
	int cmd_cnt;
	struct lcd_kit_adapt_ops *adapt_ops = NULL;
	struct lcd_kit_dsi_panel_cmds *enter_alpha_cmds = &common_info->ddic_alpha.enter_alpha_cmds;

	if (enter_alpha_cmds->cmds == NULL) {
		LCD_KIT_ERR("enter_alpha_cmds->cmds is NULL\n");
		return LCD_KIT_FAIL;
	}
	cmd_cnt = enter_alpha_cmds->cmd_cnt;
	if (cmd_cnt <= 0 || cmd_cnt > cmd_cnt_max) {
		LCD_KIT_ERR("enter_alpha_cmds's cmd_cnt is invalid, cmd_cnt:%d\n", cmd_cnt);
		return LCD_KIT_FAIL;
	}
	payload_num = (int)enter_alpha_cmds->cmds[cmd_cnt - 1].dlen;
	if (payload_num < payload_num_min || payload_num > payload_num_max) {
		LCD_KIT_ERR("enter_alpha_cmds->cmds's payload_num is invalid, payload_num:%d\n",
			payload_num);
		return LCD_KIT_FAIL;
	}
	LCD_KIT_DEBUG("cmd_cnt = %d, payload_num = %d, alpha_with_enable_flag = %d\n",
		cmd_cnt, payload_num, common_info->ddic_alpha.alpha_with_enable_flag);

	/* Sets the upper eight bits of the alpha. */
	if (common_info->ddic_alpha.alpha_with_enable_flag == 0)
		enter_alpha_cmds->cmds[cmd_cnt - 1].payload[payload_num - 2] = (alpha >> 8) & 0x0f;
	else
		enter_alpha_cmds->cmds[cmd_cnt - 1].payload[payload_num - 2] =
			((alpha >> 8) & 0x0f) | 0x10;

	/* Sets the lower eight bits of the alpha. */
	enter_alpha_cmds->cmds[cmd_cnt - 1].payload[payload_num - 1] = alpha & 0xff;
	adapt_ops = lcd_kit_get_adapt_ops();
	if (!adapt_ops || !adapt_ops->mipi_tx) {
		LCD_KIT_ERR("can not register adapt_ops!\n");
		return LCD_KIT_FAIL;
	}
	ret = adapt_ops->mipi_tx(hld, enter_alpha_cmds);
	if (ret != LCD_KIT_OK)
		LCD_KIT_ERR("adapt_ops->mipi_tx error\n");

	return ret;
}

static int display_engine_exit_ddic_alpha(uint32_t panel_id, void *hld, uint32_t alpha)
{
	int ret;
	struct lcd_kit_adapt_ops *adapt_ops = NULL;

	LCD_KIT_DEBUG("exit ddic alpha\n");
	if (common_info->ddic_alpha.exit_alpha_cmds.cmds == NULL) {
		LCD_KIT_ERR("exit_alpha_cmds.cmds is NULL\n");
		return LCD_KIT_FAIL;
	}
	adapt_ops = lcd_kit_get_adapt_ops();
	if (!adapt_ops || !adapt_ops->mipi_tx) {
		LCD_KIT_ERR("can not register adapt_ops!\n");
		return LCD_KIT_FAIL;
	}
	ret = adapt_ops->mipi_tx(hld, &common_info->ddic_alpha.exit_alpha_cmds);
	if (ret != LCD_KIT_OK)
		LCD_KIT_ERR("adapt_ops->mipi_tx error\n");
	return ret;
}

static u32 display_engine_get_alpha_support(uint32_t panel_id)
{
	return common_info->ddic_alpha.alpha_support;
}

uint32_t display_engine_local_hbm_get_support(void)
{
	uint32_t panel_id = DISPLAY_ENGINE_PANEL_INNER;

	return common_info->hbm.local_hbm_support;
}

void display_engine_hbm_gamma_read(uint32_t panel_id, void *hld)
{
	/* read rgb gamma, hbm_gamma structure: */
	/* [red_high, red_low, green_high, green_low, blue_high, blue_low] */
	/* read 60 hz gamma, number 3 corresponds to RGB */
	const size_t rgb_size = HBM_GAMMA_SIZE * 3;
#ifdef CONFIG_QCOM_MODULE_KO
	uint8_t hbm_gamma[HBM_GAMMA_SIZE * 3];
#else
	uint8_t hbm_gamma[rgb_size];
#endif
	struct lcd_kit_adapt_ops *adapt_ops = NULL;
	static bool is_gamma_read;

	if (is_gamma_read) {
		LCD_KIT_INFO("read gamma repeatedly\n");
		return;
	}

	if (!display_engine_local_hbm_get_support()) {
		LCD_KIT_ERR("local_hbm not support\n");
		return;
	}

	LCD_KIT_INFO("hbm gamma read\n");
	/* init check_flag */
	common_info->hbm.hbm_gamma.check_flag = 0;

	adapt_ops = lcd_kit_get_adapt_ops();
	if (!adapt_ops || !adapt_ops->mipi_rx) {
		LCD_KIT_ERR("can not register adapt_ops!\n");
		return;
	}

	if (adapt_ops->mipi_rx(hld, hbm_gamma, rgb_size - 1,
		&common_info->hbm.hbm_60_hz_gamma_read_cmds)) {
		LCD_KIT_ERR("hbm 60 HZ gamma mipi_rx failed!\n");
		return;
	}
	if (set_hbm_gamma_60hz_to_common_info(panel_id, hbm_gamma, rgb_size)) {
		LCD_KIT_ERR("set_hbm_gamma_60hz_to_common_info error\n");
		return;
	}
	set_gamma_node_60hz_info(panel_id, &common_info->hbm.gamma_info);

	// read 90 hz gamma
	if (adapt_ops->mipi_rx(hld, hbm_gamma, rgb_size - 1,
		&common_info->hbm.hbm_90_hz_gamma_read_cmds)) {
		LCD_KIT_ERR("hbm 90 HZ gamma mipi_rx failed!\n");
		return;
	}
	if (set_hbm_gamma_90hz_to_common_info(panel_id, hbm_gamma, rgb_size)) {
		LCD_KIT_ERR("set_hbm_gamma_90hz_to_common_info error\n");
		return;
	}
	set_gamma_node_90hz_info(panel_id, &common_info->hbm.gamma_info);

	// read 120 hz gamma
	if (adapt_ops->mipi_rx(hld, hbm_gamma, rgb_size - 1,
		&common_info->hbm.hbm_120_hz_gamma_read_cmds)) {
		LCD_KIT_ERR("hbm 120 HZ gamma mipi_rx failed!\n");
		return;
	}
	if (set_hbm_gamma_120hz_to_common_info(panel_id, hbm_gamma, rgb_size)) {
		LCD_KIT_ERR("set_hbm_gamma_120hz_to_common_info error\n");
		return;
	}
	set_gamma_node_120hz_info(panel_id, &common_info->hbm.gamma_info);
	is_gamma_read = true;
}

static void display_engine_local_hbm_set_circle_alpha_dbv_test(void *hld,
	struct display_engine_param *param)
{
	uint32_t panel_id = param->panel_id;
	int grayscale = param->local_hbm.circle_grayscale;
	uint32_t alpha = param->local_hbm.test.alpha;
	int ret;

	if (!hld) {
		LCD_KIT_ERR("hld is null\n");
		return;
	}
	LCD_KIT_DEBUG("alpha = %d\n", alpha);
	if (!param->local_hbm.test.isLocalHbmOn) {
		LCD_KIT_DEBUG("alpha circle exit\n");
		display_engine_local_hbm_set_circle(panel_id, hld, false, LCD_KIT_ALPHA_DEFAULT);
		display_engine_exit_ddic_alpha(panel_id, hld, LCD_KIT_ALPHA_DEFAULT);
		display_engine_local_hbm_set_dbv(panel_id, hld,
			display_engine_brightness_get_mipi_level());
	} else {
		if (grayscale < 0) {
			LCD_KIT_DEBUG("grayscale is negative, reinitialized to 0\n");
			grayscale = 0;
		}
		ret = display_engine_local_hbm_set_color_by_grayscale(panel_id, hld,
			disp_info->fps.current_fps, grayscale);
		if (ret != LCD_KIT_OK) {
			LCD_KIT_ERR("display_engine_local_hbm_color_set_by_grayscale error\n");
			return;
		}

		if (alpha > LCD_KIT_ALPHA_DEFAULT) {
			LCD_KIT_INFO("alpha exceeds the maximum. Use the default value\n");
			alpha = LCD_KIT_ALPHA_DEFAULT;
		}

		LCD_KIT_DEBUG("alpha circle enter\n");
		display_engine_local_hbm_set_dbv(panel_id, hld, param->local_hbm.test.dbv);
		display_engine_enter_ddic_alpha(panel_id, hld, alpha);
		display_engine_local_hbm_set_circle(panel_id, hld, true, alpha);
	}
}

static int display_engine_local_hbm_set_test(struct display_engine_param *param)
{
	struct qcom_panel_info *panel_info = lcm_get_panel_info(param->panel_id);
	int ret = LCD_KIT_OK;

	if (!display_engine_local_hbm_get_support()) {
		LCD_KIT_ERR("Local HBM is not supported\n");
		return LCD_KIT_FAIL;
	}

	if (!panel_info) {
		LCD_KIT_ERR("panel_info is NULL\n");
		return LCD_KIT_FAIL;
	}

	display_engine_local_hbm_set_circle_alpha_dbv_test(panel_info->display, param);
	return ret;
}

static void display_engine_set_local_hbm_param(struct display_engine_param *param, bool *is_fail)
{
	if (param->modules & DISPLAY_ENGINE_LOCAL_HBM) {
		if (display_engine_local_hbm_set(param->local_hbm.circle_grayscale)) {
			LCD_KIT_WARNING("display_engine_local_hbm_set() failed!\n");
			*is_fail = true;
		}
	}
	if (param->modules & DISPLAY_ENGINE_LOCAL_HBM_TEST) {
		if (display_engine_local_hbm_set_test(param)) {
			LCD_KIT_WARNING("display_engine_local_hbm_set_test() failed!\n");
			*is_fail = true;
		}
	}
	if (param->modules & DISPLAY_ENGINE_LOCAL_HBM_ALPHA_MAP) {
		if (display_engine_brightness_set_alpha_map(param->panel_id, &param->alpha_map)) {
			LCD_KIT_WARNING("display_engine_brightness_set_alpha_map() failed!\n");
			*is_fail = true;
		}
	}
}

int display_engine_set_param(struct drm_device *dev, void *data, struct drm_file *file_priv)
{
	struct display_engine_param *param = (struct display_engine_param *)data;
	bool is_fail = false;

	if (!param) {
		LCD_KIT_ERR("param is null\n");
		return LCD_KIT_FAIL;
	}

	if (param->modules & DISPLAY_ENGINE_DRM_HIST_ENABLE) {
		if (display_engine_drm_hist_set_enable(param->panel_id, &param->drm_hist)) {
			LCD_KIT_WARNING("display_engine_drm_hist_enable() failed!\n");
			is_fail = true;
		}
	}
	if (param->modules & DISPLAY_ENGINE_BRIGHTNESS_LUT) {
		if (display_engine_brightness_set_lut(param->panel_id, &param->brightness_lut)) {
			LCD_KIT_WARNING("display_engine_brightness_lut_set_lut() failed!\n");
			is_fail = true;
		}
	}
	if (param->modules & DISPLAY_ENGINE_BRIGHTNESS_MODE) {
		if (display_engine_brightness_set_mode(param->panel_id, param->brightness_mode)) {
			LCD_KIT_WARNING("display_engine_brightness_set_mode() failed!\n");
			is_fail = true;
		}
	}
	if (param->modules & DISPLAY_ENGINE_FINGERPRINT_BACKLIGHT) {
		if (display_engine_brightness_set_fingerprint_backlight(param->panel_id,
			param->fingerprint_backlight)) {
			LCD_KIT_WARNING(
				"display_engine_brightness_set_fingerprint_backlight() failed!\n");
			is_fail = true;
		}
	}
	if (param->modules & DISPLAY_ENGINE_FOLDABLE_INFO) {
		if (display_engine_foldable_info_set(param->panel_id, &param->foldable_info)) {
			LCD_KIT_WARNING("display_engine_foldable_info_set() failed!\n");
			is_fail = true;
		}
	}
	display_engine_set_local_hbm_param(param, &is_fail);

	return is_fail ? LCD_KIT_FAIL : LCD_KIT_OK;
}

static int display_engine_alpha_set_inner(uint32_t panel_id, void *hld, uint32_t alpha,
	int force_alpha_enable)
{
	int ret = LCD_KIT_OK;
	static uint32_t last_alpha;

	LCD_KIT_DEBUG("alpha:%d\n", alpha);
	if (display_engine_get_alpha_support(panel_id) == 0) {
		LCD_KIT_DEBUG("alpha_support's value is 0\n");
		return ret;
	}
	if (last_alpha == alpha && force_alpha_enable == NO_FORCING) {
		LCD_KIT_DEBUG("alpha repeated setting\n");
		return ret;
	}
	if (alpha == 0) {
		alpha = LCD_KIT_ALPHA_DEFAULT;
		LCD_KIT_WARNING("Reinitialize the alpha when the alpha is 0\n");
	}
	if (force_alpha_enable == NO_FORCING) {
		if (alpha != LCD_KIT_ALPHA_DEFAULT)
			ret = display_engine_enter_ddic_alpha(panel_id, hld, alpha);
		else
			ret = display_engine_exit_ddic_alpha(panel_id, hld, alpha);
	} else if (force_alpha_enable == FORCE_DISABLE) {
		ret = display_engine_exit_ddic_alpha(panel_id, hld, alpha);
	} else if (force_alpha_enable == FORCE_ENABLE) {
		ret = display_engine_enter_ddic_alpha(panel_id, hld, alpha);
	} else {
		LCD_KIT_ERR("Invalid force_set_alpha_value, force_alpha_enable = %d\n",
			force_alpha_enable);
		return LCD_KIT_FAIL;
	}

	last_alpha = alpha;
	return ret;
}

static void display_engine_local_hbm_alpha_log_print(struct display_engine_context *de_context,
	uint32_t dbv_threshold)
{
	struct de_context_brightness *brightness = &de_context->brightness;

	// 0 indicates the minimum dbv, and 4095 indicates the maximum dbv.
	if (dbv_threshold == 0)
		LCD_KIT_INFO("alpha map alpha_map[%d]:%d, alpha_map[%d]:%d, alpha_map[%d]:%d\n",
			dbv_threshold, brightness->alpha_map.lum_lut[dbv_threshold],
			dbv_threshold + 1, brightness->alpha_map.lum_lut[dbv_threshold + 1],
			dbv_threshold + 2, brightness->alpha_map.lum_lut[dbv_threshold + 2]);
	else if (dbv_threshold == 4095)
		LCD_KIT_INFO("map[0]:%d,map[1]:%d,map[2]:%d,map[%d]:%d,map[%d]:%d,map[%d]:%d\n",
			brightness->alpha_map.lum_lut[0], brightness->alpha_map.lum_lut[1],
			brightness->alpha_map.lum_lut[2], dbv_threshold - 2,
			brightness->alpha_map.lum_lut[dbv_threshold - 2], dbv_threshold - 1,
			brightness->alpha_map.lum_lut[dbv_threshold - 1], dbv_threshold,
			brightness->alpha_map.lum_lut[dbv_threshold]);
	else
		LCD_KIT_INFO("map[0]:%d,map[1]:%d,map[2]:%d,map[%d]:%d,map[%d]:%d,map[%d]:%d\n",
			brightness->alpha_map.lum_lut[0], brightness->alpha_map.lum_lut[1],
			brightness->alpha_map.lum_lut[2], dbv_threshold - 1,
			brightness->alpha_map.lum_lut[dbv_threshold - 1], dbv_threshold,
			brightness->alpha_map.lum_lut[dbv_threshold], dbv_threshold + 1,
			brightness->alpha_map.lum_lut[dbv_threshold + 1]);
}

static void display_engine_local_hbm_get_dbv_threshold(
	struct display_engine_context *de_context, uint32_t *threshold)
{
	static bool is_first_getting = true;
	uint32_t i = 0;

	if (!is_first_getting)
		return;
	for (i = 0; i < LHBM_ALPHA_LUT_LENGTH; i++) {
		if (de_context->brightness.alpha_map.lum_lut[i] == LCD_KIT_ALPHA_DEFAULT) {
			*threshold = i;
			is_first_getting = false;
			break;
		}
	}
}

static void display_engine_local_hbm_set_circle_alpha_dbv(uint32_t panel_id, void *hld)
{
	static uint32_t dbv_threshold = (uint32_t)(-1);
	uint32_t dbv;
	uint16_t alpha;
	struct display_engine_context *de_context = get_de_context(panel_id);

	if (!hld) {
		LCD_KIT_ERR("hld is NULL\n");
		return;
	}
	if (!de_context) {
		LCD_KIT_ERR("de_context is NULL\n");
		return;
	}

	if (!de_context->brightness.alpha_map.lum_lut) {
		LCD_KIT_INFO("de_context->brightness.alpha_map.lum_lut is NULL\n");
		return;
	}

	display_engine_local_hbm_get_dbv_threshold(de_context, &dbv_threshold);
	// 4095 indicates the maximum dbv.
	if (dbv_threshold > 4095) {
		LCD_KIT_ERR("invalid dbv_threshold, dbv_threshold:%d\n", dbv_threshold);
		return;
	}
	dbv = display_engine_brightness_get_mipi_level();

	LCD_KIT_INFO("dbv:%d, dbv_threshold:%d\n", dbv, dbv_threshold);
	display_engine_local_hbm_alpha_log_print(de_context, dbv_threshold);
	if (dbv >= dbv_threshold) {
		alpha = LCD_KIT_ALPHA_DEFAULT;
	} else {
		if (dbv >= LHBM_ALPHA_LUT_LENGTH) {
			LCD_KIT_ERR("The dbv value cannot be mapped to alpha\n");
			alpha = LCD_KIT_ALPHA_DEFAULT;
		} else {
			LCD_KIT_INFO("alpha:%d\n", de_context->brightness.alpha_map.lum_lut[dbv]);
			alpha = de_context->brightness.alpha_map.lum_lut[dbv];
		}
		dbv = dbv_threshold;
	}

	display_engine_local_hbm_set_dbv(panel_id, hld, dbv);
	display_engine_alpha_set_inner(panel_id, hld, alpha, FORCE_ENABLE);
	display_engine_local_hbm_set_circle(panel_id, hld, true, alpha);
}

static void display_engine_local_hbm_exit_circle_alpha_dbv(uint32_t panel_id, void *hld)
{
	display_engine_local_hbm_set_circle(panel_id, hld, false, LCD_KIT_ALPHA_DEFAULT);
	display_engine_alpha_set_inner(panel_id, hld, LCD_KIT_ALPHA_DEFAULT, FORCE_DISABLE);
	display_engine_local_hbm_set_dbv(panel_id, hld, display_engine_brightness_get_mipi_level());
}

static int display_engine_get_power_mode(void)
{
	struct display_engine_context *de_context = get_de_context(DISPLAY_ENGINE_PANEL_INNER);

	if (!de_context) {
		LCD_KIT_ERR("de_context is null\n");
		return LCD_KIT_FAIL;
	}
	return de_context->brightness.power_mode;
}

static bool display_engine_local_hbm_is_repeat(int grayscale)
{
	struct display_engine_context *de_context = get_de_context(DISPLAY_ENGINE_PANEL_INNER);

	if (!de_context) {
		LCD_KIT_ERR("de_context is null\n");
		return false;
	}
	return de_context->brightness.local_hbm.last_grayscale == grayscale;
}

static int display_engine_local_hbm_set_inner(int grayscale)
{
	int ret = LCD_KIT_OK;
	uint32_t panel_id = DISPLAY_ENGINE_PANEL_INNER;
	struct qcom_panel_info *panel_info = lcm_get_panel_info(panel_id);

	if (display_engine_local_hbm_is_repeat(grayscale)) {
		LCD_KIT_INFO("display_engine_local_hbm_is_repeat\n");
		return LCD_KIT_OK;
	}
	if (!panel_info) {
		LCD_KIT_ERR("panel_info is NULL\n");
		return LCD_KIT_FAIL;
	}

	LCD_KIT_INFO("LHBM set inner, grayscale:%d\n", grayscale);
	if (display_engine_get_alpha_support(panel_id) == 0) {
		LCD_KIT_ERR("local_hbm_support is 0\n");
		return LCD_KIT_FAIL;
	}
	if (grayscale > 0) {
		ret = display_engine_local_hbm_set_color_by_grayscale(
			panel_id, panel_info->display, disp_info->fps.current_fps, grayscale);
		if (ret != LCD_KIT_OK) {
			LCD_KIT_ERR("display_engine_local_hbm_color_set_by_grayscale error\n");
			return LCD_KIT_FAIL;
		}
		display_engine_local_hbm_set_circle_alpha_dbv(panel_id, panel_info->display);
	} else {
		display_engine_local_hbm_exit_circle_alpha_dbv(panel_id, panel_info->display);
	}
	if (grayscale <= 0)
		g_fingerprint_backlight_type_real = BACKLIGHT_LOW_LEVEL;
	else
		g_fingerprint_backlight_type_real = BACKLIGHT_HIGH_LEVEL;

	return LCD_KIT_OK;
}

void display_engine_handle_power_mode_enter(int mode)
{
	struct display_engine_context *de_context = get_de_context(DISPLAY_ENGINE_PANEL_INNER);

	if (!de_context) {
		LCD_KIT_ERR("de_context is null\n");
		return;
	}
	if (!display_engine_local_hbm_get_support()) {
		LCD_KIT_ERR("Local HBM is not supported\n");
		return;
	}

	LCD_KIT_DEBUG("handle power mode enter, mode:%d\n", mode);
	if (mode != SDE_MODE_DPMS_ON) {
		de_context->brightness.local_hbm.current_grayscale = 0;
		display_engine_local_hbm_set_inner(0);
		de_context->brightness.local_hbm.last_grayscale = 0;
		de_context->brightness.local_hbm.need_set_later = false;
	}
}

void display_engine_handle_power_mode_exit(int mode)
{
	struct display_engine_context *de_context = get_de_context(DISPLAY_ENGINE_PANEL_INNER);

	if (!de_context) {
		LCD_KIT_ERR("de_context is null\n");
		return;
	}
	if (!display_engine_local_hbm_get_support()) {
		LCD_KIT_ERR("Local HBM is not supported\n");
		return;
	}

	LCD_KIT_DEBUG("handle power mode exit, mode:%d\n", mode);
	de_context->brightness.power_mode = mode;
	if (mode == SDE_MODE_DPMS_ON && de_context->brightness.local_hbm.need_set_later) {
		display_engine_local_hbm_set_inner(
			de_context->brightness.local_hbm.current_grayscale);
		de_context->brightness.local_hbm.last_grayscale =
			de_context->brightness.local_hbm.current_grayscale;
		de_context->brightness.local_hbm.need_set_later = false;
	}
}

int display_engine_local_hbm_set(int grayscale)
{
	uint32_t panel_id = DISPLAY_ENGINE_PANEL_INNER;
	struct display_engine_context *de_context = get_de_context(panel_id);
	struct lcd_kit_disp_info *d_info = lcd_kit_get_disp_info(panel_id);
	u32 power_mode;

	if (!display_engine_local_hbm_get_support()) {
		LCD_KIT_ERR("Local HBM is not supported\n");
		return LCD_KIT_FAIL;
	}
	if (!de_context || !d_info) {
		LCD_KIT_ERR("de_context || d_info is null\n");
		return LCD_KIT_FAIL;
	}

	mutex_lock(&d_info->power_mode_lock);
	power_mode = display_engine_get_power_mode();
	LCD_KIT_INFO("grayscale = %d, fps = %d, power_mode = %d\n", grayscale,
		disp_info->fps.current_fps, power_mode);
	de_context->brightness.local_hbm.current_grayscale = grayscale;
	if (power_mode == 0) {
		display_engine_local_hbm_set_inner(grayscale);
		de_context->brightness.local_hbm.last_grayscale = grayscale;
		de_context->brightness.local_hbm.need_set_later = false;
	} else {
		de_context->brightness.local_hbm.need_set_later = true;
	}

	mutex_unlock(&d_info->power_mode_lock);
	return LCD_KIT_OK;
}
