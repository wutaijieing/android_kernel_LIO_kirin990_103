/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2020. All rights reserved.
 * Description: common func for device implement.
 * Create: 2019-11-05
 */
#include "device_common.h"
#include <securec.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/of.h>
#include <linux/slab.h>
#include <platform_include/basicplatform/linux/nve/nve_ap_kernel_interface.h>
#include <platform_include/smart/linux/iomcu_ipc.h>
#include <platform_include/smart/linux/iomcu_log.h>
#include <platform_include/smart/linux/iomcu_pm.h>
#include <platform_include/smart/linux/iomcu_dump.h>
#include "inputhub_wrapper/inputhub_wrapper.h"

#define DETECT_CHIP_ID_LENGTH 4
#define MAX_CHIP_ID_LENGTH 4
#define TWO_BYTES_REGISTER_ADDRESS 2
#define BUFFER_SIZE 50
#define MAX_CHIP_ID_VALUE_LEN 10

struct device_bus_info {
	int bus_num;
	int address;
	int register_add;
	uint8_t tag;
};

struct chip_id_info {
	u32 value[MAX_CHIP_ID_VALUE_LEN];
	int len;
	u32 mask;
};

static int get_combo_prop_rx_info(struct device_node *dn, struct detect_word *p_det_wd)
{
	struct property *prop = NULL;

	/* combo_rx_mask */
	prop = of_find_property(dn, "combo_rx_mask", NULL);
	if (!prop) {
		ctxhub_err("%s: get combo_rx_mask err\n", __func__);
		return RET_FAIL;
	}
	p_det_wd->rx_len = (uint32_t)prop->length;
	if (p_det_wd->rx_len > sizeof(p_det_wd->rx_msk)) {
		ctxhub_err("%s: get rx_len %u too big\n", __func__,
			p_det_wd->rx_len);
		return RET_FAIL;
	}

	if (p_det_wd->rx_len == 0) {
		ctxhub_err("[%s], rx_len is 0\n", __func__);
		return RET_FAIL;
	}

	if (of_property_read_u8_array(dn, "combo_rx_mask", p_det_wd->rx_msk,
					  prop->length) < 0) {
		ctxhub_err("[%s], read combo_rx_mask failed!\n", __func__);
		return RET_FAIL;
	}

	/* combo_rx_exp */
	prop = of_find_property(dn, "combo_rx_exp", NULL);
	if (!prop) {
		ctxhub_err("%s: get combo_rx_exp err\n", __func__);
		return RET_FAIL;
	}

	if ((uint32_t)prop->length > sizeof(p_det_wd->rx_exp) ||
		((uint32_t)prop->length % p_det_wd->rx_len)) {
		ctxhub_err("%s: rx_exp_len %u not available\n",
			__func__, prop->length);
		return RET_FAIL;
	}

	p_det_wd->exp_n = (uint32_t)prop->length / p_det_wd->rx_len;
	if (of_property_read_u8_array(dn, "combo_rx_exp", p_det_wd->rx_exp,
					  prop->length) < 0) {
		ctxhub_err("[%s], read combo_rx_exp failed!\n", __func__);
		return RET_FAIL;
	}
	return RET_SUCC;
}

static int get_combo_prop(struct device_node *dn, struct detect_word *p_det_wd)
{
	int ret;
	const char *bus_type = NULL;
	u32 temp;
	struct property *prop = NULL;

	/* combo_bus_type */
	ret = of_property_read_string(dn, "combo_bus_type", &bus_type);
	if (ret) {
		ctxhub_err("%s: get bus_type err\n", __func__);
		return ret;
	}

	if (get_combo_bus_tag(bus_type, &p_det_wd->cfg.bus_type)) {
		ctxhub_err("%s: bus_type(%s) err\n", __func__, bus_type);
		return RET_FAIL;
	}

	/* combo_bus_num */
	ret = of_property_read_u32(dn, "combo_bus_num", &temp);
	if (ret) {
		ctxhub_err("%s: get combo_data err\n", __func__);
		return ret;
	}
	p_det_wd->cfg.bus_num = (uint8_t)temp;

	/* combo_data */
	ret = of_property_read_u32(dn, "combo_data", &p_det_wd->cfg.data);
	if (ret) {
		ctxhub_err("%s: get combo_data err\n", __func__);
		return ret;
	}

	/* combo_tx */
	prop = of_find_property(dn, "combo_tx", NULL);
	if (!prop) {
		ctxhub_err("%s: get combo_tx err\n", __func__);
		return RET_FAIL;
	}
	p_det_wd->tx_len = (uint32_t)prop->length;
	if (p_det_wd->tx_len > sizeof(p_det_wd->tx)) {
		ctxhub_err("%s: get combo_tx_len %u too big\n",
			__func__, p_det_wd->tx_len);
		return RET_FAIL;
	}

	if (of_property_read_u8_array(dn, "combo_tx", p_det_wd->tx,
					prop->length) < 0) {
		ctxhub_err("[%s], read combo_tx failed!\n", __func__);
		return RET_FAIL;
	}

	ret = get_combo_prop_rx_info(dn, p_det_wd);
	return ret;
}

static int check_chip_id(const uint32_t *expect_chip_id, int len,
			uint32_t chip_id_mask, const char *device_name,
			const uint8_t *detected_chip_id, int detected_chip_id_len)
{
	int i;
	int ret;
	uint32_t chip_id;

	ret = memcpy_s(&chip_id, sizeof(uint32_t), detected_chip_id,
			detected_chip_id_len);
	if (ret != EOK) {
		ctxhub_err("[%s], memcpy failed!\n", __func__);
		return RET_FAIL;
	}

	for (i = 0; i < len; i++) {
		if (chip_id == expect_chip_id[i]) {
			ctxhub_info("%s:i2c detect suc!chip_value:0x%x\n",
				device_name, chip_id);
			return RET_SUCC;
		} else if (chip_id_mask != 0) {
			if ((chip_id & chip_id_mask) == expect_chip_id[i]) {
				ctxhub_info("%s:i2c detect suc!chip_value_with_mask:0x%x\n",
					device_name, chip_id & chip_id_mask);
				return RET_SUCC;
			}
		}
	}
	ctxhub_info("%s:i2c detect fail,chip_value:0x%x,len:%d\n",
		 device_name, chip_id, len);
	return RET_FAIL;
}

static int get_chip_id_info(struct device_node *dn, const char *device_name, struct chip_id_info *ci)
{
	struct property *prop = NULL;
	u32 temp;
	int length;
	int i;

	// param check outside
	prop = of_find_property(dn, "chip_id_value", NULL);
	if (!prop || !prop->value) {
		ctxhub_err("[%s], prop or prop value is null\n", __func__);
		return RET_FAIL;
	}

	length = prop->length / sizeof(unsigned int);
	if (length > MAX_CHIP_ID_VALUE_LEN) {
		ctxhub_err("[%s], chip id value len %d from dts bigger than array len %d\n",
			__func__, length, MAX_CHIP_ID_VALUE_LEN);
		return RET_FAIL;
	}

	ci->len = length;
	if (of_property_read_u32_array(dn, "chip_id_value", ci->value, length)) {
		ctxhub_err("%s:read chip_id_value from dts fail len=%d\n",
			device_name, length);
		return RET_FAIL;
	}

	for (i = 0; i < length; i++)
		ctxhub_info("%s, chip id value %u\n", __func__, ci->value[i]);

	if (of_property_read_u32(dn, "chip_id_mask", &temp)) {
		ctxhub_info("%s:no chip_id_mask, use chip id value directly\n",
			 __func__);
	} else {
		ci->mask = temp;
		ctxhub_info("%s:read succ chip_id_mask:0x%x\n", __func__, temp);
	}

	return RET_SUCC;
}

static int get_bus_info(struct device_node *dn, const char *device_name,
			struct device_bus_info *bi)
{
	const char *bus_type = NULL;

	if (!bi)
		return RET_FAIL;

	if (of_property_read_u32(dn, "bus_number", &bi->bus_num) ||
		of_property_read_u32(dn, "reg", &bi->address) ||
		of_property_read_u32(dn, "chip_id_register", &bi->register_add)) {
		ctxhub_err("%s:read i2c bus info from dts fail\n",
			device_name);
		return RET_FAIL;
	}

	if (of_property_read_string(dn, "bus_type", &bus_type) == 0) {
		/* get failed will use TAG_I2C */
		if (get_combo_bus_tag(bus_type, &bi->tag) != RET_SUCC)
			ctxhub_err("[%s], get bus tag failed\n", __func__);
	}
	return RET_SUCC;
}

static int detect_i2c_device(struct device_node *dn, char *device_name)
{
	int ret;
	struct device_bus_info bi;
	struct chip_id_info ci;
	uint8_t detected_chip_id[DETECT_CHIP_ID_LENGTH] = { 0 };
	uint32_t register_add_len;
	uint32_t rx_len;

	bi.tag = TAG_I2C;
	ret = get_bus_info(dn, device_name, &bi);
	if (ret != RET_SUCC)
		return RET_FAIL;

	ci.mask = 0; // init value must be 0
	(void)memset_s(ci.value, MAX_CHIP_ID_VALUE_LEN, 0, MAX_CHIP_ID_VALUE_LEN);
	ret = get_chip_id_info(dn, device_name, &ci);
	if (ret != RET_SUCC)
		return RET_FAIL;

	if (bi.tag == TAG_I3C) {
		ret = mcu_i3c_rw((uint8_t)bi.bus_num, (uint8_t)bi.address,
				 (uint8_t *)&bi.register_add, 1, detected_chip_id, 1);
	} else {
		register_add_len = 1;
		rx_len = 1;
		if ((unsigned int)bi.register_add & 0xFF00) {
			register_add_len = TWO_BYTES_REGISTER_ADDRESS;
			rx_len = MAX_CHIP_ID_LENGTH;
		}

		ret = mcu_i2c_rw((uint8_t)bi.bus_num, (uint8_t)bi.address,
				 (uint8_t *)&bi.register_add, register_add_len,
				 detected_chip_id, rx_len);
	}

	if (ret) {
		ctxhub_err("%s:%s:send i2c read cmd to mcu fail, ret=%d\n",
			__func__, device_name, ret);
		return RET_FAIL;
	}

	ret = check_chip_id(ci.value, ci.len, ci.mask, device_name, detected_chip_id,
				DETECT_CHIP_ID_LENGTH);
	return ret;
}

static int device_detect_combo_bus_type_process(struct device_node *dn,
						const struct device_manager_node *device,
					struct detect_word *det_wd)
{
	int ret;
	u32 n;
	u32 i;
	u8 r_buf[MAX_TX_RX_LEN];
	int rx_exp_p;

	ret = get_combo_prop(dn, det_wd);
	if (ret != 0) {
		ctxhub_err("%s:get_combo_prop fail\n", __func__);
		return ret;
	}

	ctxhub_info("%s: combo detect bus type %d; num %d; data %d;"
		"txlen %d; tx[0] 0x%x; rxLen %d; rxmsk[0] 0x%x;"
		"n %d; rxexp[0] 0x%x\n",
		__func__, det_wd->cfg.bus_type, det_wd->cfg.bus_num,
		det_wd->cfg.data, det_wd->tx_len, det_wd->tx[0],
		det_wd->rx_len, det_wd->rx_msk[0], det_wd->exp_n,
		det_wd->rx_exp[0]);

	ret = combo_bus_trans(&det_wd->cfg, det_wd->tx,
				  det_wd->tx_len, r_buf, det_wd->rx_len);
	ctxhub_info("combo_bus_trans ret is %d; rx 0x%x;\n", ret, r_buf[0]);

	if (ret >= 0) { /* success */
		ret = RET_FAIL; /* fail first */
		/* check expect value */
		for (n = 0; n < det_wd->exp_n; n++) {
			for (i = 0; i < det_wd->rx_len;) {
				rx_exp_p = n * det_wd->rx_len + i;
				/* check value */
				if ((r_buf[i] & det_wd->rx_msk[i]) !=
					det_wd->rx_exp[rx_exp_p])
					break;
				i++;
			}
			/* get the success device */
			if (i == det_wd->rx_len) {
				ret = 0;
				ctxhub_info("%s: %s detect succ;\n",
					 __func__, device->device_name_str);
				break;
			}
		}
	} else {
		ctxhub_err("%s, combo bus trans send failed!\n", __func__);
	}

	return ret;
}

static int device_detect(struct device_node *dn, struct device_manager_node *device,
			 struct sensor_combo_cfg *p_succ_ret)
{
	int ret = RET_FAIL; /* fail first */
	struct detect_word det_wd;
	struct property *prop = of_find_property(dn, "combo_bus_type", NULL);

	(void)memset_s(&det_wd, sizeof(det_wd), 0, sizeof(det_wd));

	if (prop) {
		ret = device_detect_combo_bus_type_process(dn, device, &det_wd);
	} else {
		ctxhub_info("%s: [%s] donot find combo prop\n", __func__,
			 device->device_name_str);
		ret = detect_i2c_device(dn, device->device_name_str);
		if (!ret) {
			u32 i2c_bus_num = 0;
			u32 i2c_address = 0;
			u32 register_add = 0;

			if (of_property_read_u32(dn, "bus_number",
						 &i2c_bus_num) || of_property_read_u32(dn,
				"reg", &i2c_address) ||
				of_property_read_u32(dn, "chip_id_register",
							 &register_add)) {
				ctxhub_err("%s:read i2c bus_number or bus address or chip_id_register from dts fail\n",
					device->device_name_str);
				return RET_FAIL;
			}
			det_wd.cfg.bus_type = TAG_I2C;
			det_wd.cfg.bus_num = (uint8_t)i2c_bus_num;
			det_wd.cfg.i2c_address = (uint8_t)i2c_address;
		}
	}

	if (ret == 0)
		*p_succ_ret = det_wd.cfg;

	return ret;
}

void read_chip_info(struct device_node *dn, struct device_manager_node *node)
{
	char *chip_info = NULL;
	int ret;

	if (!dn || !node) {
		ctxhub_err("%s, input pointer is NULL\n", __func__);
		return;
	}

	ret = of_property_read_string(dn, "compatible",
					(const char **)&chip_info);
	if (ret) {
		ctxhub_err("%s:read tag:%d info fail\n", __func__, node->tag);
		return;
	} else {
		ret = strncpy_s(node->device_chip_info, MAX_CHIP_INFO_LEN, chip_info,
				strlen(chip_info));
		if (ret != EOK) {
			ctxhub_err("[%s], strcpy device chip info err\n",
				__func__);
			return;
		}
	}
	ctxhub_info("get chip info from dts success. device name=%s\n",
		 node->device_chip_info);
}

static void read_sensorlist_fifo_param(struct device_node *dn,
				       struct sensorlist_info *sensor_info,
				       int tag)
{
	u32 temp;

	if (of_property_read_u32(dn, "minDelay", &temp) == 0) {
		sensor_info->min_delay = temp;
		ctxhub_info("device tag %d get minDelay %d\n", tag, temp);
	} else {
		sensor_info->min_delay = -1;
	}

	if (of_property_read_u32(dn, "fifoReservedEventCount", &temp) == 0) {
		sensor_info->fifo_reserved_event_count = temp;
		ctxhub_info("device tag %d get fifoReservedEventCount %d\n",
			 tag, temp);
	} else {
		sensor_info->fifo_reserved_event_count = (u32)-1;
	}

	if (of_property_read_u32(dn, "fifoMaxEventCount", &temp) == 0) {
		sensor_info->fifo_max_event_count = temp;
		ctxhub_info("device tag %d get fifoMaxEventCount %d\n",
			 tag, temp);
	} else {
		sensor_info->fifo_max_event_count = (u32)-1;
	}

	if (of_property_read_u32(dn, "maxDelay", &temp) == 0) {
		sensor_info->max_delay = temp;
		ctxhub_info("device tag %d get maxDelay %d\n", tag, temp);
	} else {
		sensor_info->max_delay = -1;
	}

	if (of_property_read_u32(dn, "flags", &temp) == 0) {
		sensor_info->flags = temp;
		ctxhub_info("device tag %d get flags %d\n", tag, temp);
	} else {
		sensor_info->flags = (u32)-1;
	}
}

static void read_sensorlist_chip_info(struct device_node *dn,
				      struct sensorlist_info *sensor_info,
				      int tag)
{
	char *chip_info = NULL;
	int ret;

	if (of_property_read_string(dn, "sensorlist_name",
				    (const char **)&chip_info) == 0) {
		ret = strncpy_s(sensor_info->name, MAX_NAME_LEN,
				chip_info, strlen(chip_info));
		if (ret != EOK)
			ctxhub_err("[%s], strcpy sensor name failed!\n",
				__func__);
		ctxhub_info("device chip info name %s\n", chip_info);
		ctxhub_info("device tag %d get name %s\n",
			 tag, sensor_info->name);
	} else {
		sensor_info->name[0] = '\0';
	}

	if (of_property_read_string(dn, "vendor",
					(const char **)&chip_info) == 0) {
		ret = strncpy_s(sensor_info->vendor, MAX_CHIP_INFO_LEN, chip_info,
				strlen(chip_info));
		if (ret != EOK)
			ctxhub_err("[%s], strcpy sensor vendor failed!\n",
				__func__);
		ctxhub_info("device tag %d get vendor %s\n",
			 tag, sensor_info->vendor);
	} else {
		sensor_info->vendor[0] = '\0';
	}
}

void read_sensorlist_info(struct device_node *dn, struct sensorlist_info *sensor_info, int tag)
{
	int temp = 0;

	if (!dn || !sensor_info) {
		ctxhub_err("[%s], input is null\n", __func__);
		return;
	}

	read_sensorlist_chip_info(dn, sensor_info, tag);

	if (of_property_read_u32(dn, "version", &temp) == 0) {
		sensor_info->version = temp;
		ctxhub_info("device tag %d get version %d\n", tag, temp);
	} else {
		sensor_info->version = -1;
	}

	if (of_property_read_u32(dn, "maxRange", &temp) == 0) {
		sensor_info->max_range = temp;
		ctxhub_info("device tag %d get maxRange %d\n", tag, temp);
	} else {
		sensor_info->max_range = -1;
	}

	if (of_property_read_u32(dn, "resolution", &temp) == 0) {
		sensor_info->resolution = temp;
		ctxhub_info("device tag %d get resolution %d\n", tag, temp);
	} else {
		sensor_info->resolution = -1;
	}

	if (of_property_read_u32(dn, "power", &temp) == 0) {
		sensor_info->power = temp;
		ctxhub_info("device tag %d get power %d\n", tag, temp);
	} else {
		sensor_info->power = -1;
	}

	read_sensorlist_fifo_param(dn, sensor_info, tag);
}

int send_parameter_to_mcu(struct device_manager_node *node, int cmd)
{
	int ret;
	pkt_parameter_req_t param_pkt;
	struct pkt_header *hd = (struct pkt_header *)&param_pkt;
	int iom3_state = get_iom3_state();

	if (!node || !node->spara) {
		ctxhub_err("[%s] node or node param is null\n", __func__);
		return RET_FAIL;
	}

	param_pkt.subcmd = cmd;
	ret = memcpy_s(param_pkt.para, sizeof(param_pkt.para), node->spara,
			   node->cfg_data_length);
	if (ret != EOK) {
		ctxhub_err("[%s] memcpy failed!\n", __func__);
		return RET_FAIL;
	}

	ctxhub_info("[%s] g_iom3_state = %d,tag =%d ,cmd =%d\n",
		 __func__, iom3_state, node->tag, cmd);

	ret = inputhub_wrapper_send_cmd(node->tag, CMD_CMN_CONFIG_REQ, &hd[1],
					node->cfg_data_length + SUBCMD_LEN, NULL);
	if (ret != 0) {
		ctxhub_err("send tag %d cfg data to mcu fail,ret=%d\n",
			node->tag, ret);
		return RET_FAIL;
	}

	ctxhub_info("[%s] set %s cfg to mcu success\n", __func__, node->device_name_str);

	return RET_SUCC;
}

__attribute__ ((weak))
int nve_direct_access_interface(struct opt_nve_info_user *user_info)
{
	if (!user_info)
		ctxhub_info("%s: user_info is null\n", __func__);
	ctxhub_info("%s: use weak\n", __func__);
	return RET_SUCC;
}

int read_calibrate_data_from_nv(int nv_number, int nv_size, const char *nv_name,
				struct opt_nve_info_user *user_info)
{
	int ret;
	int nv_name_len;

	/*
	 * If nv_name is null, strlen will coredump.
	 */
	if (!nv_name || !user_info) {
		ctxhub_err("[%s], input pointer is null\n", __func__);
		return RET_FAIL;
	}

	nv_name_len = strlen(nv_name);

	(void)memset_s(user_info, sizeof(struct opt_nve_info_user), 0,
		sizeof(struct opt_nve_info_user));
	user_info->nv_operation = NV_READ_TAG;
	user_info->nv_number = nv_number;
	user_info->valid_size = nv_size;
	ret = strncpy_s(user_info->nv_name, sizeof(user_info->nv_name),
			nv_name, nv_name_len);
	if (ret != EOK) {
		ctxhub_err("[%s], strncpy_s failed\n", __func__);
		return RET_FAIL;
	}

	ret = nve_direct_access_interface(user_info);
	if (ret != 0) {
		ctxhub_err("nve_direct_access_interface read nv %d error %d\n",
			nv_number, ret);
		return RET_FAIL;
	}
	return RET_SUCC;
}

int write_calibrate_data_to_nv(int nv_number, int nv_size,
				   const char *nv_name, const char *temp)
{
	int ret;
	struct opt_nve_info_user local_user_info;
	int nv_name_len;

	/*
	 * If nv_name is null, strlen will coredump.
	 * Temp is null, memcpy_s will return fail.
	 */
	if (!nv_name) {
		ctxhub_err("[%s], input pointer is null\n", __func__);
		return RET_FAIL;
	}

	(void)memset_s(&local_user_info, sizeof(local_user_info), 0,
		sizeof(local_user_info));
	local_user_info.nv_operation = NV_WRITE_TAG;
	local_user_info.nv_number = nv_number;
	local_user_info.valid_size = nv_size;
	nv_name_len = strlen(nv_name);
	ret = strncpy_s(local_user_info.nv_name,
			sizeof(local_user_info.nv_name),
		nv_name, nv_name_len);
	if (ret != EOK) {
		ctxhub_err("[%s], device gyro detect memcpy failed!\n", __func__);
		return RET_FAIL;
	}
	/* copy to nv by pass */
	ret = memcpy_s(local_user_info.nv_data,
			   sizeof(local_user_info.nv_data),
		temp, local_user_info.valid_size);
	if (ret != EOK) {
		ctxhub_err("[%s], device gyro detect memcpy failed!\n", __func__);
		return RET_FAIL;
	}

	ret = nve_direct_access_interface(&local_user_info);
	if (ret != 0) {
		ctxhub_err("nve_direct_access_interface read nv %d error %d\n",
			nv_number, ret);
		return RET_FAIL;
	}
	return RET_SUCC;
}

int send_calibrate_data_to_mcu(int tag, uint32_t subcmd, const void *data,
				   int length, bool is_lock)
{
	int ret;
	pkt_parameter_req_t param_pkt;
	struct pkt_header *hd = (struct pkt_header *)&param_pkt;

	(void)is_lock;
	if (!data) {
		ctxhub_err("[%s] data is null\n", __func__);
		return RET_FAIL;
	}

	param_pkt.subcmd = subcmd;
	ret = memcpy_s(param_pkt.para, sizeof(param_pkt.para), data,
			   length);
	if (ret != EOK) {
		ctxhub_err("[%s] memcpy failed!\n", __func__);
		return RET_FAIL;
	}

	ctxhub_info("[%s] lock is %d\n", __func__, is_lock);

	ret = inputhub_wrapper_send_cmd(tag, CMD_CMN_CONFIG_REQ, &hd[1],
					length + SUBCMD_LEN, NULL);
	if (ret != 0) {
		ctxhub_err("send tag %d calibrate data to mcu fail,ret=%d\n",
			 tag, ret);
		return RET_FAIL;
	}

	ctxhub_info("send tag %d calibrate data to mcu success\n", tag);
	return RET_SUCC;
}

static int detect_disable_sample_task_prop(struct device_node *dn, uint32_t *value)
{
	int ret;

	ret = of_property_read_u32(dn, "disable_sample_task", value);
	if (ret)
		return RET_FAIL;

	return RET_SUCC;
}

int sensor_device_detect(struct device_node *dn, struct device_manager_node *device,
			 void (*read_data_from_dts)(struct device_node*, struct device_manager_node*))
{
	int ret;
	struct sensor_combo_cfg cfg;
	struct sensor_combo_cfg *p_cfg = NULL;
	u32 disable;

	if (!dn || !device) {
		ctxhub_err("input dts node is NULL\n");
		return RET_FAIL;
	}

	ret = device_detect(dn, device, &cfg);
	if (ret) {
		device->detect_result = DET_FAIL;
		return RET_FAIL;
	}

	ret = memcpy_s((void *)device->spara, device->cfg_data_length,
			   (void *)&cfg, sizeof(struct sensor_combo_cfg));
	if (ret != EOK) {
		ctxhub_err("[%s], device acc detect memcpy failed!\n", __func__);
		return RET_FAIL;
	}

	/* check disable sensor task */
	p_cfg = (struct sensor_combo_cfg *)device->spara;
	if (!p_cfg) {
		ctxhub_err("[%s], sensor config pointer is NULL\n", __func__);
		return RET_FAIL;
	}

	ret = detect_disable_sample_task_prop(dn, &disable);
	/* get disbale_sample_task property value */
	if (!ret)
		p_cfg->disable_sample_thread = (uint8_t)disable;

	if (read_data_from_dts)
		read_data_from_dts(dn, device);

	return RET_SUCC;
}

#ifdef CONFIG_CONTEXTHUB_WATCH_FACTORY
static int check_process_factory_input(int tag, int subcmd, u8 *send, int send_size,
	u8 *rec, int rec_size)
{
	if (tag != TAG_ACCEL && tag != TAG_CAP_PROX && tag != TAG_GYRO) {
		ctxhub_err("[%s] tag err: %d!\n", __func__, tag);
		return RET_FAIL;
	}

	if (subcmd < SUB_CMD_FACTORY_ACC_CHIP_ID ||
		subcmd > SUB_CMD_FACTORY_CAP_GET_MODE) {
		ctxhub_err("[%s] subcmd err: %d!\n", __func__, subcmd);
		return RET_FAIL;
	}

	if (send_size < 0 || (send == NULL && send_size > 0)) {
		ctxhub_err("[%s] send err: %d!\n", __func__, send_size);
		return RET_FAIL;
	}

	if (rec_size < 0 || (rec == NULL && rec_size > 0)) {
		ctxhub_err("[%s] rec err:  %d!\n", __func__, rec_size);
		return RET_FAIL;
	}
	return RET_SUCC;
}

/*
 * send ipc to process operation and get response data
 */
int bus_type_process_factory(int tag, int subcmd, u8 *send, int send_size,
	u8 *rec, int rec_size)
{
	int ret;
	pkt_parameter_req_t param_pkt;
	struct pkt_header *hd = (struct pkt_header *)&param_pkt;
	struct read_info ri;

	ret = check_process_factory_input(tag, subcmd, send, send_size, rec, rec_size);
	if (ret != RET_SUCC)
		return ret;

	ctxhub_info("[%s] tag %d; subcmd %d;\n",
		__func__, tag, subcmd);

	(void)memset_s(&ri, sizeof(ri), 0, sizeof(ri));

	if (send_size > 0) {
		ret = memcpy_s(param_pkt.para, sizeof(param_pkt.para), send,
				   send_size);
		if (ret != EOK) {
			ctxhub_err("[%s] memcpy failed!\n", __func__);
			return RET_FAIL;
		}
	}

	param_pkt.subcmd = subcmd;
	ret = inputhub_wrapper_send_cmd(tag, CMD_CMN_CONFIG_REQ, &hd[1],
		send_size + SUBCMD_LEN, &ri);
	if (ret != EOK) {
		ctxhub_err("[%s] inputhub_wrapper_send_cmd err: %d\n", __func__, ret);
		return RET_FAIL;
	}

	if (rec_size < ri.data_length)
	{
		ctxhub_err("[%s] rec_size err: %d, %d\n",
			__func__, rec_size, ri.data_length);
		return RET_FAIL;
	}

	ret = memcpy_s(rec, rec_size, ri.data, ri.data_length);
	if (ret != EOK) {
		ctxhub_err("[%s] memcpy_s failed!\n", __func__);
		return RET_FAIL;
	}

	ctxhub_info("[%s] ret: %d; data_length: %d;\n",
		__func__, ri.errno, ri.data_length);

	return RET_SUCC;
}
#endif
