/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020. All rights reserved.
 * Description: plat func file
 * Create:2019.09.22
 */

#include "plat_func.h"

#include <securec.h>

#include <linux/io.h>
#include "platform_include/smart/linux/iomcu_boot.h"
#include "platform_include/smart/linux/iomcu_pm.h"
#include "iomcu_dmd.h"
#include "iomcu_logbuff.h"
#include "device_manager.h"


int __weak send_cmd_from_kernel(void)
{
	return 0;
}

int __weak send_cmd_from_kernel_response(void)
{
	return 0;
}

static struct config_on_ddr *g_config_on_ddr;

struct config_on_ddr *get_config_on_ddr(void)
{
	return g_config_on_ddr;
}

/*lint -e446 */
static int init_config_on_ddr(void)
{
	if (!g_config_on_ddr) {
		g_config_on_ddr = (struct config_on_ddr *)ioremap_wc(IOMCU_CONFIG_START,
								     IOMCU_CONFIG_SIZE);
		if (!g_config_on_ddr) {
			ctxhub_err("[%s] ioremap (%x) failed!\n", __func__, IOMCU_CONFIG_START);
			return -1;
		}
	}

	if (memset_s(g_config_on_ddr, IOMCU_CONFIG_SIZE, 0, sizeof(struct config_on_ddr)) != EOK) {
		ctxhub_err("[%s] memset fail!\n", __func__);
		return -1;
	}

	g_config_on_ddr->logbuff_cb_backup.mutex = 0;
	g_config_on_ddr->log_level = INFO_LEVEL;

	return 0;
}
/*lint +e446 */

void inputhub_init_before_boot(void)
{
	(void)init_config_on_ddr();
	(void)sensorhub_io_driver_init();
#ifdef CONFIG_HUAWEI_DSM
	dmd_init();
#endif
	device_detect_init();
}

static int g_is_sensor_mcu_mode;
void inputhub_init_after_boot(void)
{
	g_is_sensor_mcu_mode = 1;
}

int get_sensor_mcu_mode(void)
{
	return g_is_sensor_mcu_mode;
}
EXPORT_SYMBOL(get_sensor_mcu_mode); //lint !e546 !e580

static char *g_obj_tag_str[] = {
	[TAG_ACCEL] = "TAG_ACCEL",
	[TAG_GYRO] = "TAG_GYRO",
	[TAG_MAG] = "TAG_MAG",
	[TAG_ALS] = "TAG_ALS",
	[TAG_PS] = "TAG_PS",
	[TAG_LINEAR_ACCEL] = "TAG_LINEAR_ACCEL",
	[TAG_GRAVITY] = "TAG_GRAVITY",
	[TAG_ORIENTATION] = "TAG_ORIENTATION",
	[TAG_ROTATION_VECTORS] = "TAG_ROTATION_VECTORS",
	[TAG_PRESSURE] = "TAG_PRESSURE",
	[TAG_HALL] = "TAG_HALL",
	[TAG_MAG_UNCALIBRATED] = "TAG_MAG_UNCALIBRATED",
	[TAG_GAME_RV] = "TAG_GAME_RV",
	[TAG_GYRO_UNCALIBRATED] = "TAG_GYRO_UNCALIBRATED",
	[TAG_SIGNIFICANT_MOTION] = "TAG_SIGNIFICANT_MOTION",
	[TAG_STEP_DETECTOR] = "TAG_STEP_DETECTOR",
	[TAG_STEP_COUNTER] = "TAG_STEP_COUNTER",
	[TAG_GEOMAGNETIC_RV] = "TAG_GEOMAGNETIC_RV",
	[TAG_HANDPRESS] = "TAG_HANDPRESS",
	[TAG_CAP_PROX] = "TAG_CAP_PROX",
	[TAG_FINGERSENSE] = "TAG_FINGERSENSE",
	[TAG_PHONECALL] = "TAG_PHONECALL",
	[TAG_CONNECTIVITY] = "TAG_CONNECTIVITY",
	[TAG_CA] = "TAG_CA",
	[TAG_OIS] = "TAG_OIS",
	[TAG_THP] = "TAG_THP",
	[TAG_TP] = "TAG_TP",
	[TAG_SPI] = "TAG_SPI",
	[TAG_I2C] = "TAG_I2C",
	[TAG_RGBLIGHT] = "TAG_RGBLIGHT",
	[TAG_BUTTONLIGHT] = "TAG_BUTTONLIGHT",
	[TAG_BACKLIGHT] = "TAG_BACKLIGHT",
	[TAG_VIBRATOR] = "TAG_VIBRATOR",
	[TAG_SYS] = "TAG_SYS",
	[TAG_LOG] = "TAG_LOG",
	[TAG_MOTION] = "TAG_MOTION",
	[TAG_LOG_BUFF] = "TAG_LOG_BUFF",
	[TAG_PDR] = "TAG_PDR",
	[TAG_AR] = "TAG_AR",
	[TAG_FP] = "TAG_FP",
	[TAG_KEY] = "TAG_KEY",
	[TAG_TILT_DETECTOR] = "TAG_TILT_DETECTOR",
	[TAG_FAULT] = "TAG_FAULT",
	[TAG_MAGN_BRACKET] = "TAG_MAGN_BRACKET",
	[TAG_RPC] = "TAG_RPC",
	[TAG_AGT] = "TAG_AGT",
	[TAG_COLOR] = "TAG_COLOR",
	[TAG_FP_UD] = "TAG_FP_UD",
	[TAG_ACCEL_UNCALIBRATED] = "TAG_ACCEL_UNCALIBRATED",
	[TAG_END] = "TAG_END",
};

char *get_tag_str(int tag)
{
	if (tag < TAG_BEGIN || tag >= TAG_END) {
		ctxhub_err("[%s] tag %d is error\n", __func__, tag);
		return NULL;
	}
	return g_obj_tag_str[tag] ? g_obj_tag_str[tag] : "TAG_UNKNOWN";
}

/* find first pos */
const char *get_str_begin(const char *cmd_buf)
{
	if (cmd_buf == NULL)
		return NULL;

	while (is_space_ch(*cmd_buf))
		++cmd_buf;

	if (end_of_string(*cmd_buf))
		return NULL;

	return cmd_buf;
}

/* find last pos */
const char *get_str_end(const char *cmd_buf)
{
	if (cmd_buf == NULL)
		return NULL;

	while (!is_space_ch(*cmd_buf) && !end_of_string(*cmd_buf))
		++cmd_buf;

	return cmd_buf;
}

#define ch_is_digit(ch) ('0' <= (ch) && (ch) <= '9')
#define ch_is_hex(ch) (('A' <= (ch) && (ch) <= 'F') || ('a' <= (ch) && (ch) <= 'f'))
#define ch_is_hexdigit(ch) (ch_is_digit(ch) || ch_is_hex(ch))

bool get_arg(const char *str, int *arg)
{
	int val = 0;
	bool neg = false;
	bool check_failed = (!str || !arg);
	bool is_hex = false;

	if (check_failed)
		return false;

	if (*str == '-') {
		++str;
		neg = true;
	}
	is_hex = (*str == '0') && ((*(str + 1) == 'x') || (*(str + 1) == 'X'));
	if (is_hex) {
		str += 2; // strip 0x
		for (; !is_space_ch(*str) && !end_of_string(*str); ++str) {
			if (!ch_is_hexdigit(*str))
				return false;
			val *= 16; // data is HEX
			val = (unsigned int)val | (ch_is_digit(*str) ?
				(*str - '0') : (((*str | 0x20) - 'a') + 10)); // calc value
		}
	} else {
		for (; !is_space_ch(*str) && !end_of_string(*str); ++str) {
			if (!ch_is_digit(*str))
				return false;
			val *= 10; // data is DEC
			val += *str - '0';
		}
	}

	*arg = neg ? -val : val;
	return true;
}
