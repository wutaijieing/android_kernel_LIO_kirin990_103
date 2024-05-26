

#ifndef __DISPLAY_ENGINE_KERNEL_H__
#define __DISPLAY_ENGINE_KERNEL_H__

#ifndef BIT
#define BIT(nr) (1UL << (nr))
#endif

#define BRIGHTNESS_LUT_LENGTH 10001
#define LHBM_ALPHA_LUT_LENGTH 4096
#define SN_CODE_LEN_MAX 54
#define PANEL_NAME_LEN 128
#define PANEL_VERSION_LEN 32

#if defined(__cplusplus)
extern "C" {
#endif

/* The panel ID used to specify which panel would be configured.
 * And you can use DISPLAY_ENGINE_PANEL_INNER for single panel device.
 */
enum display_engine_panel_id {
	DISPLAY_ENGINE_PANEL_INVALID = -1,
	DISPLAY_ENGINE_PANEL_INNER,
	DISPLAY_ENGINE_PANEL_OUTER,
	DISPLAY_ENGINE_PANEL_COUNT,
};

/* The module ID used to specify the detail operation of IO.
 * Module IDs can be used as single or multiple.
 */
enum display_engine_module_id {
	DISPLAY_ENGINE_DRM_HIST_ENABLE = BIT(0),
	DISPLAY_ENGINE_BRIGHTNESS_LUT = BIT(1),
	DISPLAY_ENGINE_BRIGHTNESS_MODE = BIT(2),
	DISPLAY_ENGINE_PANEL_INFO = BIT(3),
	DISPLAY_ENGINE_FINGERPRINT_BACKLIGHT = BIT(4),
	DISPLAY_ENGINE_FOLDABLE_INFO = BIT(5),
	DISPLAY_ENGINE_LOCAL_HBM = BIT(6),
	DISPLAY_ENGINE_LOCAL_HBM_TEST = BIT(7),
	DISPLAY_ENGINE_LOCAL_HBM_ALPHA_MAP = BIT(8),
};

/* The data structure of display engine IO feature.
 * Names should be started with display_engine_drm_, display_engine_ddic_ or display_engine_panel_,
 * and followed by feature name.
 */
struct display_engine_drm_hist {
	unsigned int enable;
};

enum display_engine_brightness_mode_id {
	DISPLAY_ENGINE_BRIGHTNESS_MODE_DEFAULT = 0,
	DISPLAY_ENGINE_BRIGHTNESS_MODE_DC,
	DISPLAY_ENGINE_BRIGHTNESS_MODE_HIGH_PRECISION,
	DISPLAY_ENGINE_MODE_MAX
};

/* The data structure of dc brightness */
struct display_engine_brightness_lut {
	unsigned int mode_id;

	/* length of both lut: BRIGHTNESS_LUT_LENGTH */
	unsigned short *to_dbv_lut;
	unsigned short *to_alpha_lut;
};

/* The data structure of local hbm test */
struct local_hbm_test_param {
	bool isLocalHbmOn;
	int alpha;
	int dbv;
};

/* The data structure of alpha map */
struct display_engine_alpha_map {
	unsigned short *lum_lut;
};

/* The data structure of local hbm */
struct display_engine_local_hbm_param {
	int circle_grayscale;
	struct local_hbm_test_param test;
};

/* The data structure of panel information IO feature. */
struct display_engine_panel_info {
	unsigned int width;
	unsigned int height;
	unsigned int max_luminance;
	unsigned int min_luminance;
	unsigned int max_backlight;
	unsigned int min_backlight;
	unsigned int sn_code_length;
	unsigned char sn_code[SN_CODE_LEN_MAX];
	char panel_name[PANEL_NAME_LEN];
	int local_hbm_support;
	char panel_version[PANEL_VERSION_LEN];
	bool is_factory;
};

/* The data structure of fingerprint backlight, includes fingerprint up and down */
struct display_engine_fingerprint_backlight {
	unsigned int scene_info;
	unsigned int hbm_level;
	unsigned int current_level;
};

enum display_engine_foldable_REGION {
	DE_REGION_PRIMARY = 0,
	DE_REGION_SLAVE,
	DE_REGION_FOLDING,
	DE_REGION_NUM
};

enum display_engine_statistics_type {
	DE_MIPI_DBV_ACC = 0,
	DE_ORI_DBV_ACC,
	DE_SCREEN_ON_TIME,
	DE_SCREEN_ON_TIME_WITH_HIST_ENABLE,
	DE_HBM_ACC,
	DE_HBM_ON_TIME,
	DE_STATISTICS_TYPE_MAX
};

/* frame rate type ,LEVEL 1,2,3 mean 60Hz,90Hz,120Hz */
enum display_engine_fps_type_index {
	DE_FPS_LEVEL1_INDEX = 0,
	DE_FPS_LEVEL2_INDEX,
	DE_FPS_LEVEL3_INDEX,
	DE_FPS_MAX
};

/* The roi param structure of foldable compensation */
struct display_engine_roi_param {
	unsigned int top;
	unsigned int bottom;
	unsigned int left;
	unsigned int right;
};

/* Kernel RGB index */
enum display_engine_color_index {
	DE_INDEX_INVALID = -1,
	DE_INDEX_R,
	DE_INDEX_G,
	DE_INDEX_B,
	DE_COLOR_INDEX_COUNT
};

/* The data structure of foldable compensation */
struct display_engine_foldable_info {
	bool is_region_enable[DE_REGION_NUM];
	unsigned int data[DE_STATISTICS_TYPE_MAX][DE_REGION_NUM][DE_FPS_MAX];
	struct display_engine_roi_param roi_param;
	unsigned int lut_count;
	unsigned short *lut[DE_COLOR_INDEX_COUNT];
};

/* The data structure of display engine IO interface. */
struct display_engine_param {
	unsigned int panel_id; /* It used for multi-panels */
	unsigned int modules;
	struct display_engine_drm_hist drm_hist;
	struct display_engine_brightness_lut brightness_lut;
	struct display_engine_panel_info panel_info;
	unsigned int brightness_mode;
	struct display_engine_fingerprint_backlight fingerprint_backlight;
	struct display_engine_local_hbm_param local_hbm;
	struct display_engine_alpha_map alpha_map;
	struct display_engine_foldable_info foldable_info;
};

#if defined(__cplusplus)
}
#endif

#endif /* __DISPLAY_ENGINE_KERNEL_H__ */
