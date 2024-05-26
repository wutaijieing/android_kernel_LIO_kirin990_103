/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
 * Description: tof detect source file
 */

#include "tof_detect.h"

#include <linux/gpio.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/err.h>
#include <huawei_platform/log/hw_log.h>

#include "securec.h"

#include "sensor_config.h"

#ifndef HWLOG_TAG
#define HWLOG_TAG tof_sensor
HWLOG_REGIST();
#endif

#define MAX_TOF_NUM 2
#define GPIO_STAT_HIGH 1
#define MAX_TOF_NUM 2
#define TOF1_VI5300_COMPATIBLE_PROP "vi5300,tof1"
#define TOF1_VL53L3_COMPATIBLE_PROP "vl53l3,tof1"

static struct tof_dts_data g_vi5300_platform_data = {};
static int32_t g_tof_vi5300_num = 0;
static struct tof_platform_data_ext g_tof_data[MAX_TOF_NUM] = {};

static void read_tof_info_from_dts(enum sensor_detect_list s_id, pkt_sys_dynload_req_t *dyn_req,
	struct device_node *dn, int32_t *tof_sensor_flag, const char *tof_chip_name)
{
	uint32_t temp = 0;
	char *tof_compatible_prop = NULL;

	if (!dn || !tof_sensor_flag || !dyn_req) {
		hwlog_err("[%s-%d]:dn or tof_sensor_flag, dyn_req is null ptr\n",
			__func__, __LINE__);
		return;
	}

	if (s_id == TOF1) {
		tof_compatible_prop = tof_chip_name;
	} else {
		hwlog_err("[%s-%d]:tof sensor list id:%d, is wrong\n", __func__, __LINE__, s_id);
		return;
	}

	read_chip_info(dn, s_id);
	read_tof_chip_info(s_id);
	if (!strncmp(get_sensor_chip_info_address(s_id), tof_compatible_prop,
		strlen(tof_compatible_prop) + 1)) {
		*tof_sensor_flag = 1;
		hwlog_info("[%s-%d]:%s i2c_address success\n",
			__func__, __LINE__, tof_compatible_prop);
	}

	if (of_property_read_u32(dn, "file_id", &temp))
		hwlog_err("[%s-%d]:read tof file_id fail\n", __func__, __LINE__);
	else
		dyn_req->file_list[dyn_req->file_count] = (uint16_t)temp;

	dyn_req->file_count++;

	if (of_property_read_u32(dn, "sensor_list_info_id", &temp))
		hwlog_err("[%s-%d]:read tof sensor_list_info_id fail\n", __func__, __LINE__);
	else
		add_sensor_list_info_id((uint16_t)temp);
	read_sensorlist_info(dn, s_id);
}

static int32_t tof_dts_info_parse(struct tof_dts_data *data, struct device_node *dn)
{
	int32_t ret;
	int32_t temp;

	if (!dn || !data) {
		hwlog_err("[%s-%d]:dn or data is null ptr\n", __func__, __LINE__);
		return -1;
	}
	ret = of_property_read_u32(dn, "chip_id_register", &data->chip_id_reg);
	if (ret != 0) {
		hwlog_err("[%s-%d]:of get property chip_id_register fail, ret:0x%x\n",
			__func__, __LINE__, ret);
		return -1;
	}
	ret = of_property_read_u32(dn, "chip_id_value", &data->chip_id_val);
	if (ret != 0) {
		hwlog_err("[%s-%d]:of get property chip_id_value fail, ret:0x%x\n",
			__func__, __LINE__, ret);
		return -1;
	}
	ret = of_property_read_u32(dn, "bus_number", &data->bus_num);
	if (ret != 0) {
		hwlog_err("[%s-%d]:of get property bus_number fail, ret:0x%x\n",
			__func__, __LINE__, ret);
		return -1;
	}
	ret = of_property_read_u32(dn, "reg", &data->dev_addr);
	if (ret != 0) {
		hwlog_err("[%s-%d]:of get property reg fail, ret:0x%x\n",
			__func__, __LINE__, ret);
		return -1;
	}
	temp = of_get_named_gpio(dn, "xshut-gpio", 0);
	if (temp < 0) {
		hwlog_err("[%s-%d]:read gpio_xshut fail\n", __func__, __LINE__);
		return -1;
	}
	data->gpio_xshut = (GPIO_NUM_TYPE)temp;
	temp = of_get_named_gpio(dn, "irq-gpio", 0);
	if (temp < 0) {
		hwlog_err("[%s-%d]:read gpio_irq fail\n", __func__, __LINE__);
		return -1;
	}
	data->gpio_irq = (GPIO_NUM_TYPE)temp;
	hwlog_info("[%s-%d]:bus_num:0x%x, reg:0x%x, chip_id_reg:0x%x, chip_id_val:0x%x, \
		gpio_xshut:%d, gpio_irq:%d\n",
		__func__, __LINE__, data->bus_num, data->dev_addr, data->chip_id_reg,
		data->chip_id_val, data->gpio_xshut, data->gpio_irq);
	return 0;
}

static void tof_chip_power_on(struct tof_dts_data *data)
{
	int32_t ret = gpio_direction_output(data->gpio_xshut, GPIO_STAT_HIGH);
	if (ret != 0) {
		hwlog_err("[%s-%d]:gpio_direction_output fail, gpio_xshut:%d, ret:0x%x\n",
			__func__, __LINE__, data->gpio_xshut, ret);
		return;
	}
	hwlog_info("[%s-%d]:tof bus_num:%u, dev_addr:0x%x, gpio_xshut:%d chip power on\n",
		__func__, __LINE__, data->bus_num, data->dev_addr, data->gpio_xshut);
}

static int32_t tof_sensor_detect_ext(struct tof_dts_data *platform_data,
	struct sensor_combo_cfg *cfg, struct device_node *dn,
	struct sensor_detect_manager *sm, int32_t index)
{
	int32_t ret;
	struct sensor_combo_cfg combo_cfg = {};
	struct tof_dts_data dts_data = {};

	ret = tof_dts_info_parse(&dts_data, dn);
	if (ret != 0) {
		hwlog_err("[%s-%d]:tof_dts_info_parse fail\n", __func__, __LINE__);
		return -1;
	}
	// pull up chip voltage gpio
	tof_chip_power_on(&dts_data);

	if (memcpy_s(platform_data, sizeof(struct tof_dts_data), &dts_data,
		sizeof(struct tof_dts_data)) != EOK)
		hwlog_err("[%s-%d]:memcpy_s fail\n", __func__, __LINE__);

	ret = _device_detect(dn, index, &combo_cfg);
	hwlog_info("[%s-%d]:device_detect_ex:%s _device_detect ret:0x%x\n",
		__func__, __LINE__, sm[index].sensor_name_str, ret);
	if (!ret) {
		if (memcpy_s((void *)cfg, sizeof(struct sensor_combo_cfg), (void *)&combo_cfg,
			sizeof(struct sensor_combo_cfg)) != EOK) {
			hwlog_err("[%s-%d]:memcpy_s sensor_combo_cfg fail\n", __func__, __LINE__);
			ret = -1;
		}
		hwlog_info("[%s-%d]:device_detect_ex:%s, memcpy_s data, cfg->bus_num:0x%x, \
			cfg->i2c_addr:0x%x, ret:0x%x\n", __func__, __LINE__,
			sm[index].sensor_name_str, combo_cfg.bus_num, combo_cfg.i2c_address, ret);
	}
	return ret;
}

void read_tof_data_from_dts_ext(enum sensor_detect_list s_id, pkt_sys_dynload_req_t *dyn_req,
	struct device_node *dn, int32_t *tof_sensor_flag)
{
	read_tof_info_from_dts(s_id, dyn_req, dn, tof_sensor_flag, TOF1_VI5300_COMPATIBLE_PROP);
	read_tof_info_from_dts(s_id, dyn_req, dn, tof_sensor_flag, TOF1_VL53L3_COMPATIBLE_PROP);
}

int32_t tof_sensor_detect(struct device_node *dn, struct sensor_detect_manager *sm, int32_t index)
{
	int32_t ret;
	char *chip_info = NULL;

	if (!dn || !sm) {
		hwlog_err("[%s-%d]:dn or sm is null ptr\n", __func__, __LINE__);
		return -1;
	}

	if (index == TOF) {
		ret = tof_sensor_detect_ext(&g_tof_data[0].vi5300,
			&g_tof_data[0].cfg, dn, sm, index);
		if (ret != 0)
			hwlog_err("[%s-%d]:tof vi5300 index:%d fail, ret:0x%x\n",
				__func__, __LINE__, index, ret);
		ret = tof_sensor_detect_ext(&g_tof_data[0].vl53l3,
			&g_tof_data[0].cfg, dn, sm, index);
		if (ret != 0)
			hwlog_err("[%s-%d]:tof vl53l3 index:%d fail, ret:0x%x\n",
				__func__, __LINE__, index, ret);
	}

	if (index == TOF1) {
		ret = tof_sensor_detect_ext(&g_tof_data[1].vi5300,
			&g_tof_data[1].cfg, dn, sm, index);
		if (ret != 0)
			hwlog_err("[%s-%d]:tof vi5300 index:%d fail, ret:0x%x\n",
				__func__, __LINE__, index, ret);
		ret = tof_sensor_detect_ext(&g_tof_data[1].vl53l3,
			&g_tof_data[1].cfg, dn, sm, index);
		if (ret != 0)
			hwlog_err("[%s-%d]:tof vl53l3 index:%d fail, ret:0x%x\n",
				__func__, __LINE__, index, ret);
	}
	return ret;
}

static void signal_tof_detect_init(enum sensor_detect_list s_id,
	struct sensor_detect_manager *sm, uint32_t len, int32_t index)
{
	struct sensor_detect_manager *tof_sm = NULL;

	if (!sm || len <= s_id) {
		hwlog_err("[%s-%d]:sm is null ptr of len:%u is less than TOF:%d\n",
			__func__, __LINE__, len, s_id);
		return;
	}

	tof_sm = sm + s_id;
	tof_sm->spara = (const void *)&g_tof_data[index];
	tof_sm->cfg_data_length = sizeof(g_tof_data[index]);
}

void tof_detect_init(struct sensor_detect_manager *sm, uint32_t len)
{
	signal_tof_detect_init(TOF, sm, len, 0);
	signal_tof_detect_init(TOF1, sm, len, 1);
}

static void tof_vl53l3_ref_calib_data_dump(const struct tof_vl53l3_calibrate_data *data)
{
	hwlog_info("[%s-%d]:customer->global_config_spad_enables_ref_0:%u\n",
		__func__, __LINE__, data->global_config_spad_enables_ref_0);
	hwlog_info("[%s-%d]:customer->global_config_spad_enables_ref_1:%u\n",
		__func__, __LINE__, data->global_config_spad_enables_ref_1);
	hwlog_info("[%s-%d]:customer->global_config_spad_enables_ref_2:%u\n",
		__func__, __LINE__, data->global_config_spad_enables_ref_2);
	hwlog_info("[%s-%d]:customer->global_config_spad_enables_ref_3:%u\n",
		__func__, __LINE__, data->global_config_spad_enables_ref_3);
	hwlog_info("[%s-%d]:customer->global_config_spad_enables_ref_4:%u\n",
		__func__, __LINE__, data->global_config_spad_enables_ref_4);
	hwlog_info("[%s-%d]:customer->global_config_spad_enables_ref_5:%u\n",
		__func__, __LINE__, data->global_config_spad_enables_ref_5);
	hwlog_info("[%s-%d]:customer->global_config_ref_en_start_select:%u\n",
		__func__, __LINE__, data->global_config_ref_en_start_select);
	hwlog_info("[%s-%d]:customer->ref_spad_man_num_requested_ref_spads:%u\n",
		__func__, __LINE__, data->ref_spad_man_num_requested_ref_spads);
	hwlog_info("[%s-%d]:customer->ref_spad_man_ref_location:%u\n",
		__func__, __LINE__, data->ref_spad_man_ref_location);
	hwlog_info("[%s-%d]:customer->ref_spad_char_total_rate_target_mcps:%u\n",
		__func__, __LINE__, data->ref_spad_char_total_rate_target_mcps);
}

static void tof_vl53l3_xtalk_calib_data_dump(const struct tof_vl53l3_calibrate_data *data)
{
	int i;

	hwlog_info("[%s-%d]:customer->algo_crosstalk_compensation_plane_offset_kcps:%u\n",
		__func__, __LINE__, data->algo_crosstalk_compensation_plane_offset_kcps);
	hwlog_info("[%s-%d]:customer->algo_crosstalk_compensation_x_plane_gradient_kcps:%d\n",
		__func__, __LINE__, data->algo_crosstalk_compensation_x_plane_gradient_kcps);
	hwlog_info("[%s-%d]:customer->algo_crosstalk_compensation_y_plane_gradient_kcps:%d\n",
		__func__, __LINE__, data->algo_crosstalk_compensation_y_plane_gradient_kcps);
	hwlog_info("[%s-%d]:algo_xtalk_cpo_histo_merge_kcps:\n", __func__, __LINE__);
	for (i = 0; i < VL53LX_BIN_REC_SIZE; i++)
		hwlog_info("[%s-%d]:algo_xtalk_cpo_histo_merge_kcps[%d]:%u\n",
			__func__, __LINE__, i, data->algo_xtalk_cpo_histomerge_kcps[i]);
	hwlog_info("xtalk_histo->xtalk_shape->bin_data:\n", __func__, __LINE__);
	for (i = 0; i < VL53LX_XTALK_HISTO_BINS; i++)
		hwlog_info("[%s-%d]:bin_data[%d]:%u\n",
			__func__, __LINE__, i, data->xtalk_shape_bin_data[i]);
	hwlog_info("[%s-%d]:xtalk_histo->xtalk_shape->phasecal_result_reference_phase:%u\n",
		__func__, __LINE__, data->phasecal_result_reference_phase);
	hwlog_info("[%s-%d]:xtalk_histo->xtalk_shape->phasecal_result_vcsel_start:%u\n",
		__func__, __LINE__, data->phasecal_result_vcsel_start);
	hwlog_info("[%s-%d]:xtalk_histo->xtalk_shape->cal_config_vcsel_start:%u\n",
		__func__, __LINE__, data->cal_config_vcsel_start);
	hwlog_info("[%s-%d]:xtalk_histo->xtalk_shape->vcsel_width:%u\n",
		__func__, __LINE__, data->vcsel_width);
	hwlog_info("[%s-%d]:xtalk_histo->xtalk_shape->VL53LX_p_015:%u\n",
		__func__, __LINE__, data->vl53lx_p_015);
	hwlog_info("[%s-%d]:xtalk_histo->xtalk_shape->zero_distance_phase:%u\n",
		__func__, __LINE__, data->zero_distance_phase);
	hwlog_info("[%s-%d]:xtalk_histo->xtalk_hist_removed->cfg_device_state:%u\n",
		__func__, __LINE__, data->cfg_device_state);
	hwlog_info("[%s-%d]:xtalk_histo->xtalk_hist_removed->rd_device_state:%u\n",
		__func__, __LINE__, data->rd_device_state);
	hwlog_info("[%s-%d]:xtalk_histo->xtalk_shape->VL53LX_p_019:%u\n",
		__func__, __LINE__, data->vl53lx_p_019);
	hwlog_info("[%s-%d]:xtalk_histo->xtalk_shape->VL53LX_p_020:%u\n",
		__func__, __LINE__, data->vl53lx_p_020);
	hwlog_info("[%s-%d]:xtalk_histo->xtalk_shape->VL53LX_p_021:%u\n",
		__func__, __LINE__, data->vl53lx_p_021);
	hwlog_info("[%s-%d]:xtalk_histo->xtalk_hist_removed->bin_seq:\n", __func__, __LINE__);
	for (i = 0; i < VL53LX_MAX_BIN_SEQUENCE_LENGTH; i++)
		hwlog_info("[%s-%d]:bin_seq[%d]:%u\n",
			__func__, __LINE__, i, data->xtalk_hist_removed_bin_seq[i]);
	hwlog_info("[%s-%d]:xtalk_histo->xtalk_hist_removed->bin_rep:\n", __func__, __LINE__);
	for (i = 0; i < VL53LX_MAX_BIN_SEQUENCE_LENGTH; i++)
		hwlog_info("[%s-%d]:bin_rep[%d]:%u\n",
			__func__, __LINE__, i, data->xtalk_hist_removed_bin_rep[i]);
	hwlog_info("[%s-%d]:xtalk_check:%u\n", __func__, __LINE__, data->xtalk_check);
}

static void tof_vl53l3_offset_calib_data_dump(const struct tof_vl53l3_calibrate_data *data)
{
	int i;

	hwlog_info("[%s-%d]:xtalk_histo->xtalk_hist_removed->bin_data:\n", __func__, __LINE__);
	for (i = 0; i < VL53LX_HISTOGRAM_BUFFER_SIZE; i++)
		hwlog_info("[%s-%d]:bin_data[%d]:%u\n",
			__func__, __LINE__, i, data->xtalk_hist_removed_bin_data[i]);
	hwlog_info("[%s-%d]:customer->algo_part_to_part_range_offset_mm:%d\n",
		__func__, __LINE__, data->algo_part_to_part_range_offset_mm);
	hwlog_info("[%s-%d]:customer->mm_config_inner_offset_mm:%d\n",
		__func__, __LINE__, data->mm_config_inner_offset_mm);
	hwlog_info("[%s-%d]:customer->mm_config_outer_offset_mm:%d\n",
		__func__, __LINE__, data->mm_config_outer_offset_mm);
	hwlog_info("[%s-%d]:offset_check:%u\n", __func__, __LINE__, data->offset_check);
}

void tof_vl53l3_calib_data_dump(const struct tof_vl53l3_calibrate_data *data,
	int __attribute__((unused)) len)
{
	hwlog_info("[%s-%d]:Calibrate Data Dump:\n", __func__, __LINE__);
	hwlog_info("[%s-%d]:sensor_type:%u\n", __func__, __LINE__, data->sensor_type);
	tof_vl53l3_ref_calib_data_dump(data);
	tof_vl53l3_xtalk_calib_data_dump(data);
	tof_vl53l3_offset_calib_data_dump(data);
}
