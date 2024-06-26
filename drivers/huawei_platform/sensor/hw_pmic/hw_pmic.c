/*
 * hw_pmic.c
 *
 * debug for pmic sensor
 *
 * Copyright (c) 2019 Huawei Technologies Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */
#include <linux/i2c.h>
#include <hw_pmic.h>
#ifdef CONFIG_HUAWEI_HW_DEV_DCT
#include <huawei_platform/devdetect/hw_dev_dec.h>
#endif
#include <huawei_platform/log/hw_log.h>
#include <huawei_platform/sensor/hw_comm_pmic.h>
#include "gp_buck.h"

#define HWLOG_TAG huawei_pmic
HWLOG_REGIST();

#define GPIO_IRQ_REGISTER_SUCCESS 1
#define MAX_I2C_EXCEP_TIME 3
#define MAX_PMIC_ONCHIP    3

struct hw_pmic_ctrl_t *hw_pmic_ctrl;
struct dsm_client *hw_pmic_sen_client;
int pmic_dsm_notify_limit;
static struct pmic_ctrl_vote_t pmic_ctrl_vote[MAX_PMIC_ONCHIP][VOUT_BOOST_EN + 1];
DEFINE_HW_PMIC_MUTEX(vote_cfg);

#ifdef CONFIG_HUAWEI_DSM
struct dsm_dev dsm_hw_pmic = {
	.name = "dsm_hw_pmic",
	.device_name = NULL,
	.ic_name = NULL,
	.module_name = NULL,
	.fops = NULL,
	.buff_size = 256, // max pmic exception info size
};
#endif

#ifndef	CONFIG_USE_CAMERA3_ARCH
struct kernel_pmic_ctrl_t * kernel_get_pmic_ctrl(void)
{
	return NULL;
}
#endif

static int hw_pmic_i2c_read(struct hw_pmic_i2c_client *client,
	u8 reg, u8 *data)
{
	int rc;
	struct i2c_msg msgs[2];

	if (!client || !data || !client->client || !client->client->adapter)
		return -EFAULT;

	msgs[0].addr = client->client->addr;
	msgs[0].flags = 0;
	msgs[0].len = 1;
	msgs[0].buf = &reg;

	msgs[1].addr = client->client->addr;
	msgs[1].flags = I2C_M_RD;
	msgs[1].len = 1;
	msgs[1].buf = data;

	rc = i2c_transfer(client->client->adapter, msgs, 2);
	if (rc < 0) {
		hwlog_err("%s transfer error, reg=0x%x, data=0x%x, addr=0x%x\n",
			__func__, reg, *data, msgs[0].addr);
#ifdef CONFIG_HUAWEI_DSM
		if (!dsm_client_ocuppy(hw_pmic_sen_client) &&
			(pmic_dsm_notify_limit < MAX_I2C_EXCEP_TIME)) {
			dsm_client_record(hw_pmic_sen_client,
				"pmic read fail! reg=0x%x, rc=%d\n", reg, rc);
			dsm_client_notify(hw_pmic_sen_client,
				DSM_CAMPMIC_I2C_ERROR_NO);
			hwlog_err("[I/DSM] dsm_client_notify\n");
			pmic_dsm_notify_limit++;
		}
#endif
	} else {
		hwlog_debug("%s reg=0x%x, data=0x%x\n", __func__, reg, *data);
	}

	return rc;
}

static int hw_pmic_i2c_write(struct hw_pmic_i2c_client *client,
	u8 reg, u8 data)
{
	int rc;
	u8 buf[2];
	struct i2c_msg msg;

	hwlog_debug("%s reg=0x%x, data=0x%x\n", __func__, reg, data);
	if (!client || !client->client || !client->client->adapter)
		return -EFAULT;
	buf[0] = reg;
	buf[1] = data;
	msg.addr = client->client->addr;
	msg.flags = 0;
	msg.len = 2;
	msg.buf = buf;

	rc = i2c_transfer(client->client->adapter, &msg, 1);
	if (rc < 0) {
		hwlog_err("%s transfer error, reg=0x%x, data=0x%x, addr=0x%x",
			__func__, reg, data, msg.addr);
#ifdef CONFIG_HUAWEI_DSM
		if (!dsm_client_ocuppy(hw_pmic_sen_client) &&
			(pmic_dsm_notify_limit < MAX_I2C_EXCEP_TIME)) {
			dsm_client_record(hw_pmic_sen_client,
				"pmic write fail! reg=0x%x, rc=%d\n", reg, rc);
			dsm_client_notify(hw_pmic_sen_client,
				DSM_CAMPMIC_I2C_ERROR_NO);
			hwlog_err("[I/DSM] dsm_client_notify\n");
			pmic_dsm_notify_limit++;
		}
#endif
	}

	return rc;
}

struct hw_pmic_i2c_fn_t hw_pmic_i2c_func_tbl = {
	.i2c_read = hw_pmic_i2c_read,
	.i2c_write = hw_pmic_i2c_write,
};

void hw_set_pmic_ctrl(struct hw_pmic_ctrl_t *pmic_ctrl)
{
	hw_pmic_ctrl = pmic_ctrl;
}

struct hw_pmic_ctrl_t *hw_get_pmic_ctrl(void)
{
	return hw_pmic_ctrl;
}

int hw_main_pmic_ctrl_cfg(u8 pmic_seq, u32 vol, int state)
{
	struct kernel_pmic_ctrl_t *pmic_main_ctrl = NULL;

	pmic_main_ctrl = kernel_get_pmic_ctrl();
	if (!pmic_main_ctrl || !pmic_main_ctrl->func_tbl ||
		!pmic_main_ctrl->func_tbl->pmic_seq_config) {
		hwlog_err("%s, main pmic not support\n", __func__);
		return -EFAULT;
	}

	pmic_main_ctrl->func_tbl->pmic_seq_config(pmic_main_ctrl,
		pmic_seq, vol, state);
	return 0;
}

static void buck_max_volt_calc(enum pmic_power_req_src_t pmic_power_src,
	struct hw_comm_pmic_cfg_t *pmic_cfg)
{
	pmic_seq_index_t seq;
	u8 num;
	u32 seq_volt = 0;
	u32 i;

	seq = pmic_cfg->pmic_power_type;
	num = pmic_cfg->pmic_num;
	for (i = 0; i < MAX_RMIC_REQ; i++) {
		hwlog_debug("volt = %u\n",pmic_ctrl_vote[num][seq].pmic_volt[i]);
		if (seq_volt <= pmic_ctrl_vote[num][seq].pmic_volt[i]) {
			seq_volt = pmic_ctrl_vote[num][seq].pmic_volt[i];
			hwlog_debug("%s higher vol = %u\n", __func__, seq_volt);
		}
	}
	pmic_cfg->pmic_power_voltage = seq_volt;
	hwlog_info("calc vote volt = %u\n", pmic_cfg->pmic_power_voltage);
}

// define a function to claim different vote algo
static void pmic_volt_vote_set(enum pmic_power_req_src_t src,
	struct hw_comm_pmic_cfg_t *pmic_cfg)
{
	pmic_seq_index_t seq = pmic_cfg->pmic_power_type;
	u8 num = pmic_cfg->pmic_num;

	if (pmic_ctrl_vote[num][seq].power_state == 0)
		return;

	if ((pmic_ctrl_vote[num][seq].power_state &
		(~((u8)PMIC_VOTE_ON << (u8)src))) == 0) {
		hwlog_info("only this src need power, no need vote");
		return;
	}
	if (seq == VOUT_BUCK_1)
		buck_max_volt_calc(src, pmic_cfg);
}

static void pmic_power_vote_set(enum pmic_power_req_src_t src,
	struct hw_comm_pmic_cfg_t *pmic_cfg)
{
	pmic_seq_index_t seq = pmic_cfg->pmic_power_type;
	u8 num = pmic_cfg->pmic_num;

	switch (src) {
	case MAIN_CAM_PMIC_REQ:
		if (pmic_cfg->pmic_power_state == true) {
			pmic_ctrl_vote[num][seq].power_state |= (u8)PMIC_VOTE_ON << (u8)src;
			pmic_ctrl_vote[num][seq].power_cnt[src]++;
			pmic_ctrl_vote[num][seq].pmic_volt[src] =
				pmic_cfg->pmic_power_voltage;
		} else {
			pmic_ctrl_vote[num][seq].power_cnt[src]--;
		}
		if (pmic_ctrl_vote[num][seq].power_cnt[src] < 0) {
			pmic_ctrl_vote[num][seq].power_cnt[src] = 0;
			hwlog_err("err power vote cnt\n");
		}
		if (pmic_ctrl_vote[num][seq].power_cnt[src] == 0) {
			pmic_ctrl_vote[num][seq].power_state &=
				~((u8)PMIC_VOTE_ON << (u8)src);
			pmic_ctrl_vote[num][seq].pmic_volt[src] = 0;
		}
		break;
	default:
		if (pmic_cfg->pmic_power_state == true) {
			pmic_ctrl_vote[num][seq].power_state |= (u8)PMIC_VOTE_ON << (u8)src;
			pmic_ctrl_vote[num][seq].pmic_volt[src] =
				pmic_cfg->pmic_power_voltage;
		} else {
			pmic_ctrl_vote[num][seq].power_state &=
				~((u8)PMIC_VOTE_ON << (u8)src);
			pmic_ctrl_vote[num][seq].pmic_volt[src] = 0;
		}
		break;
	}

	if (pmic_ctrl_vote[num][seq].power_state > 0) {
		pmic_cfg->pmic_power_state = PMIC_VOTE_ON;
	} else {
		pmic_cfg->pmic_power_state = PMIC_VOTE_OFF;
		pmic_ctrl_vote[num][seq].pmic_volt[src] = 0;
	}
}

int hw_pmic_power_cfg(enum pmic_power_req_src_t pmic_power_src,
	struct hw_comm_pmic_cfg_t *comm_pmic_cfg)
{
	struct hw_pmic_ctrl_t *pmic_ctrl = NULL;
	int ret;

	if (!comm_pmic_cfg) {
		hwlog_err("%s, pmic vol cfg para null\n", __func__);
		return -EFAULT;
	}
	if ((pmic_power_src >= MAX_RMIC_REQ) ||
		(comm_pmic_cfg->pmic_power_type > VOUT_BOOST_EN) ||
		(comm_pmic_cfg->pmic_num >= MAX_PMIC_ONCHIP)) {
		hwlog_err("%s, err para, %d, %d\n", __func__,
			pmic_power_src, comm_pmic_cfg->pmic_power_type);
		return -EFAULT;
	}
	mutex_lock(&pmic_mut_vote_cfg);
	hwlog_info("need num-%d, seq-%d, type-%d, volt-%u, on-%d, state-%u\n",
		comm_pmic_cfg->pmic_num, pmic_power_src,
		comm_pmic_cfg->pmic_power_type,
		comm_pmic_cfg->pmic_power_voltage,
		comm_pmic_cfg->pmic_power_state,
		pmic_ctrl_vote[comm_pmic_cfg->pmic_num][comm_pmic_cfg->pmic_power_type].power_state);

	pmic_power_vote_set(pmic_power_src, comm_pmic_cfg);
	pmic_volt_vote_set(pmic_power_src, comm_pmic_cfg);
	hwlog_info("set num-%d, src-%d, type-%d, vol-%u, on-%d, state-%u\n",
		comm_pmic_cfg->pmic_num, pmic_power_src,
		comm_pmic_cfg->pmic_power_type,
		comm_pmic_cfg->pmic_power_voltage,
		comm_pmic_cfg->pmic_power_state,
		pmic_ctrl_vote[comm_pmic_cfg->pmic_num][comm_pmic_cfg->pmic_power_type].power_state);

	if (comm_pmic_cfg->pmic_num == 0) {
		hw_main_pmic_ctrl_cfg(comm_pmic_cfg->pmic_power_type,
			comm_pmic_cfg->pmic_power_voltage,
			comm_pmic_cfg->pmic_power_state);
		mutex_unlock(&pmic_mut_vote_cfg);
		return 0;
	} else if (comm_pmic_cfg->pmic_num == 2) { // 2 for gp buck type
		 gp_buck_ctrl_en(comm_pmic_cfg->pmic_power_type,
			comm_pmic_cfg->pmic_power_voltage,
			comm_pmic_cfg->pmic_power_state);
		mutex_unlock(&pmic_mut_vote_cfg);
		return 0;
	}
	pmic_ctrl = hw_get_pmic_ctrl();
	if (!pmic_ctrl || !pmic_ctrl->func_tbl ||
		!pmic_ctrl->func_tbl->pmic_seq_config) {
		hwlog_err("%s, pmic_ctrl is null,just return\n", __func__);
		mutex_unlock(&pmic_mut_vote_cfg);
		return -EFAULT;
	}

	ret = pmic_ctrl->func_tbl->pmic_seq_config(pmic_ctrl,
		comm_pmic_cfg->pmic_power_type,
		comm_pmic_cfg->pmic_power_voltage,
		comm_pmic_cfg->pmic_power_state);
	mutex_unlock(&pmic_mut_vote_cfg);
	return ret;
}

int set_boost_load_enable_cfg(enum pmic_power_req_src_t pmic_power_src, u8 pmic_num, int power_status)
{
	struct kernel_pmic_ctrl_t *pmic_main_ctrl = NULL;
	int ret;

	if (pmic_num != 0)
		return -EFAULT;

	pmic_main_ctrl = kernel_get_pmic_ctrl();
	if (!pmic_main_ctrl || !pmic_main_ctrl->func_tbl ||
		!pmic_main_ctrl->func_tbl->pmic_set_boost_load_enable) {
		hwlog_err("%s, main pmic not support\n", __func__);
		return -EFAULT;
	}
	ret = pmic_main_ctrl->func_tbl->pmic_set_boost_load_enable(pmic_main_ctrl, power_status);
	return ret;
}

int set_force_pwm_mode_cfg(enum pmic_power_req_src_t pmic_power_src, u8 pmic_num, int power_status)
{
	struct kernel_pmic_ctrl_t *pmic_main_ctrl = NULL;
	int ret;

	if (pmic_num != 0)
		return -EFAULT;

	pmic_main_ctrl = kernel_get_pmic_ctrl();
	if (!pmic_main_ctrl || !pmic_main_ctrl->func_tbl ||
		!pmic_main_ctrl->func_tbl->pmic_force_pwm_mode) {
		hwlog_err("%s, main pmic not support\n", __func__);
		return -EFAULT;
	}
	ret = pmic_main_ctrl->func_tbl->pmic_force_pwm_mode(pmic_main_ctrl, power_status);
	return ret;
}

static irqreturn_t hw_pmic_interrupt_handler(int vec, void *info)
{
	struct hw_pmic_ctrl_t *pmic_ctrl = NULL;

	if (!info)
		return IRQ_NONE;

	pmic_ctrl = (struct hw_pmic_ctrl_t *)info;
	if (!pmic_ctrl || !pmic_ctrl->func_tbl ||
		!pmic_ctrl->func_tbl->pmic_check_exception)
		return IRQ_NONE;

	if (pmic_ctrl->func_tbl->pmic_check_exception != NULL)
		pmic_ctrl->func_tbl->pmic_check_exception(pmic_ctrl);

	return IRQ_HANDLED;
}

int hw_pmic_setup_intr(struct hw_pmic_ctrl_t *pmic_ctrl)
{
	struct device_node *dev_node = NULL;
	struct hw_pmic_info *pmic_info = NULL;
	int rc = -EPERM;

	hwlog_info("%s enter\n", __func__);
	if (!pmic_ctrl || !pmic_ctrl->dev) {
		hwlog_err("%s pmic_ctrl is NULL\n", __func__);
		return rc;
	}
	dev_node = pmic_ctrl->dev->of_node;
	pmic_info = &pmic_ctrl->pmic_info;

	// get intrrupt pin
	rc = of_property_read_u32(dev_node, "hisi,pmic-intr", &pmic_info->intr);
	hwlog_info("%s huawei,pmic-intr %u", __func__, pmic_info->intr);
	if (rc < 0) {
		hwlog_err("%s, failed %d\n", __func__, __LINE__);
		rc = 0; // ignore if pmic chip is not support interrupt mode
		goto irq_req_fail;
	}

	// setup intrrupt request
	rc = gpio_request(pmic_info->intr, "pmic-intr");
	if (rc < 0) {
		hwlog_err("%s failed request exp pin rc = %d\n", __func__, rc);
		goto irq_req_fail;
	}

	rc = gpio_direction_input(pmic_info->intr);
	if (rc < 0) {
		hwlog_err("fail to configure intr as input %d", rc);
		goto direction_failed;
	}

	rc = gpio_to_irq(pmic_info->intr);
	if (rc < 0) {
		hwlog_err("fail to irq");
		goto direction_failed;
	}
	pmic_info->irq = rc;

	rc = request_threaded_irq(pmic_info->irq, NULL,
			hw_pmic_interrupt_handler,
			IRQF_TRIGGER_FALLING | IRQF_ONESHOT,
			"hw_pmic_interrupt",
			(void *)pmic_ctrl);
	if (rc < 0) {
		hwlog_err("allocate int fail result:%d\n", rc);
		goto irq_failed;
	}
	pmic_info->irq_flag = GPIO_IRQ_REGISTER_SUCCESS;
	return 0;

irq_failed:
	free_irq(pmic_info->irq, (void *)pmic_ctrl);
direction_failed:
	gpio_free(pmic_info->intr);
irq_req_fail:
	return rc;
}

void hw_pmic_release_intr(struct hw_pmic_ctrl_t *pmic_ctrl)
{
	struct hw_pmic_info *pmic_info = NULL;

	hwlog_info("%s enter\n", __func__);
	if (!pmic_ctrl) {
		hwlog_err("%s pmic_ctrl is NULL\n", __func__);
		return;
	}
	pmic_info = &pmic_ctrl->pmic_info;
	if (pmic_info->irq_flag != GPIO_IRQ_REGISTER_SUCCESS) {
		hwlog_err("%s pmic_info->irq failed\n", __func__);
		return;
	}
	free_irq(pmic_info->irq, (void *)pmic_ctrl);
	gpio_free(pmic_info->intr);
}

int hw_pmic_gpio_boost_enable(struct hw_pmic_ctrl_t *pmic_ctrl, int state)
{
	int ret = -EPERM;
	struct hw_pmic_info *pmic_info = NULL;

	hwlog_info("%s enter\n", __func__);
	if (!pmic_ctrl)
		return ret;
	pmic_info = &pmic_ctrl->pmic_info;
	if (pmic_info->boost_en_pin == 0) {
		hwlog_info("boost enable pin not controled here\n");
		return ret;
	}

	if (state)
		gpio_set_value(pmic_info->boost_en_pin, 1);
	else
		gpio_set_value(pmic_info->boost_en_pin, 0);
	return 0;
}

static int hw_pmic_get_dt_data(struct hw_pmic_ctrl_t *pmic_ctrl)
{
	struct device_node *dev_node = NULL;
	struct hw_pmic_info *pmic_info = NULL;
	static bool gpio_req_status = false;
	int rc = -EPERM;

	hwlog_info("%s enter\n", __func__);
	if (!pmic_ctrl || !pmic_ctrl->dev) {
		hwlog_err("%s pmic_ctrl is NULL\n", __func__);
		return rc;
	}

	dev_node = pmic_ctrl->dev->of_node;
	pmic_info = &pmic_ctrl->pmic_info;

	rc = of_property_read_u32(dev_node, "hisi,slave_address",
		&pmic_info->slave_address);
	hwlog_info("%s slave_address %d\n", __func__, pmic_info->slave_address);
	if (rc < 0) {
		hwlog_err("%s failed %d\n", __func__, __LINE__);
		return rc;
	}

	rc = of_property_read_u32(dev_node, "hw,boost-pin",
		&pmic_info->boost_en_pin);
	if (rc < 0) {
		hwlog_err("%s, get boost enable pin failed\n", __func__);
		pmic_info->boost_en_pin = 0;
		rc = 0;
	} else {
		hwlog_info("enable boost-pin = %d\n", pmic_info->boost_en_pin);
		if (!gpio_req_status) {
			rc = gpio_request(pmic_info->boost_en_pin,
				"hw_pmic_boost_en");
			if (rc < 0) {
				hwlog_err("fail req boost en = %d\n", rc);
				return -EPERM;
			}
			(void)gpio_direction_output(pmic_info->boost_en_pin, 1);
			gpio_req_status = true;
		}
	}

	return rc;
}

int hw_pmic_i2c_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	struct i2c_adapter *adapter = NULL;
	struct hw_pmic_ctrl_t *pmic_ctrl = NULL;
	static bool hw_pmic_enabled;
	int rc;

	pmic_dsm_notify_limit = 0;
	if (hw_pmic_enabled) {
		hwlog_info("pmic has already registerd\n");
		return -EFAULT;
	}
	hwlog_info("%s client name = %s\n", __func__, client->name);

	adapter = client->adapter;
	if (!i2c_check_functionality(adapter, I2C_FUNC_I2C)) {
		hwlog_err("%s i2c_check_functionality failed\n", __func__);
		return -EIO;
	}
	if (!id) {
		hwlog_err("%s id null\n", __func__);
		return -EFAULT;
	}
	pmic_ctrl = (struct hw_pmic_ctrl_t *)(uintptr_t)id->driver_data;
	if (!pmic_ctrl || !pmic_ctrl->pmic_i2c_client || !pmic_ctrl->func_tbl ||
		!pmic_ctrl->func_tbl->pmic_get_dt_data ||
		!pmic_ctrl->func_tbl->pmic_init) {
		hwlog_err("%s pmic_ctrl null\n", __func__);
		return -EFAULT;
	}
	pmic_ctrl->pmic_i2c_client->client = client;
	pmic_ctrl->dev = &client->dev;
	pmic_ctrl->pmic_i2c_client->i2c_func_tbl = &hw_pmic_i2c_func_tbl;

	rc = hw_pmic_get_dt_data(pmic_ctrl);
	if (rc < 0) {
		hwlog_err("%s kernel_pmic_get_dt_data failed\n", __func__);
		return -EFAULT;
	}
	rc = pmic_ctrl->func_tbl->pmic_get_dt_data(pmic_ctrl);
	if (rc < 0) {
		hwlog_err("%s flash_get_dt_data failed\n", __func__);
		return -EFAULT;
	}
	rc = pmic_ctrl->func_tbl->pmic_init(pmic_ctrl);
	if (rc < 0) {
		hwlog_err("%s pmic init failed\n", __func__);
		return -EFAULT;
	}

	hw_set_pmic_ctrl(pmic_ctrl);
#ifdef CONFIG_HUAWEI_DSM
	if (!hw_pmic_sen_client)
		hw_pmic_sen_client = dsm_register_client(&dsm_hw_pmic);
#endif
	hwlog_info("%s hw pmic register success\n", __func__);
	hw_pmic_enabled = true;
	return rc;
}

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("debug for pmic driver");
MODULE_AUTHOR("Huawei Technologies Co., Ltd.");

