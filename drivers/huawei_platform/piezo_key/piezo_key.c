/*
 * piezo_key.c
 *
 * piezo key driver
 *
 * Copyright (c) 2021-2021 Huawei Technologies Co., Ltd.
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

#include "piezo_key.h"
#include <asm/irq.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/uaccess.h>
#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/of_device.h>
#include <linux/of_gpio.h>
#include <linux/firmware.h>
#include <linux/sysfs.h>
#include <linux/device.h>
#include <huawei_platform/log/hw_log.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <huawei_platform/devdetect/hw_dev_dec.h>
#include "securec.h"

#define FIRMWARE_NAME "PiezoKey.bin"
#define DTS_SHORT_PRESS_KEYCODE "shortpress_keycode"
#define DTS_LONG_PRESS_KEYCODE "longpress_keycode"
#define SEND_CMD_BUFFER_LEN 100
#define READ_MAX_BUFF_LEN 4100
#define PIEZO_KEY_MAJOR 138
#define FIRMWARE_VERSION_INDEX 9
#define SEND_CMD_HEADER_LEN 3
#define UPDATE_RETRY_TIME 3
#define ACK_BYTE_INDEX 2
#define ACK_RET_SIZE 1
#define DEVICE_INFO_LEN 16
#define NOISE_TIME 1100
#define WAVE_TIME 1000
#define CRC16_POLYMIAL 0x1021
#define CRC_BIT17_MASK 0x10000
#define CRC_BIT9_MASK 0x100
#define CRC16_RESULT_MASK 0xFFFFu
#define SELF_CHECK_PARAM_NUM 5

#define FIRST_PACKAGE_WAIT_TIME 500
#define LAST_PACKAGE_WAIT_TIME 2500
#define NORMAL_PACKAGE_WAIT_TIME 15

#define START_TEST 1
#define STOP_TEST 0

enum piezo_key_update_status {
	PIEZO_KEY_UPDATE_FAILED,
	PIEZO_KEY_UPDATE_SUCCESS,
	PIEZO_KEY_UPDATE_PROCESSING,
	PIEZO_KEY_UPDATE_INIT_STATUS,
};

enum piezo_key_report_event {
	PIEZO_KEY_NO_EVENT,
	PIEZO_KEY_SHORT_PRESS,
	PIEZO_KEY_LONG_PRESS,
};

#ifdef HWLOG_TAG
#undef HWLOG_TAG
#endif
#define HWLOG_TAG piezo_key
HWLOG_REGIST();

struct piezo_key_data {
	struct i2c_client *client;
	uint8_t update_status;
	uint8_t i2c_status;
	int32_t event_gpio;
	int32_t event_irq;
	int32_t short_press_keycode;
	int32_t long_press_keycode;
	struct input_dev *input_dev;
	struct class *piezo_key_class;
	struct work_struct work;
};

struct piezo_key_data *g_ft_data;

static int32_t piezo_key_readwrite(struct i2c_client *client,
				   uint8_t *write_buff, uint16_t write_len,
				   uint8_t *read_buff, uint16_t read_len)
{
	struct i2c_msg transfer_msg[2]; // 2 up to two messages
	int32_t ret;
	int32_t index = 0;

	if (write_len) {
		transfer_msg[index].addr  = client->addr;
		transfer_msg[index].flags = 0;
		transfer_msg[index].len = write_len;
		transfer_msg[index].buf = write_buff;
		index++;
	}

	if (read_len) {
		transfer_msg[index].addr  = client->addr;
		transfer_msg[index].flags = I2C_M_RD;
		transfer_msg[index].len = read_len;
		transfer_msg[index].buf = read_buff;
		index++;
	}

	ret = i2c_transfer(client->adapter, transfer_msg, index);
	if (ret < 0)
		return ret;

	if (ret != index)
		return -EIO;

	return 0;
}

static int32_t piezo_key_send_cmd(struct i2c_client *client, uint8_t *cmd_buf,
				  uint16_t len, uint16_t wait_ms)
{
	int32_t ret;
	uint8_t resp_buff[HEADER_LEN + ACK_RET_SIZE] = {0};
	ret = piezo_key_readwrite(client, cmd_buf, len, NULL, 0);
	if (ret != 0) {
		hwlog_err("send cmd error, ret: %d", ret);
		return ret;
	}

	mdelay(wait_ms);
	ret = piezo_key_readwrite(client, cmd_buf, 1,
				   resp_buff, HEADER_LEN + ACK_RET_SIZE);
	if (ret != 0) {
		hwlog_err("read command ack error, ret: %d", ret);
		return ret;
	}

	return (int32_t)resp_buff[ACK_BYTE_INDEX];
}

static uint16_t update_crc16(uint16_t crc_in, uint8_t byte)
{
	uint32_t crc = crc_in;
	uint32_t in = byte | CRC_BIT9_MASK;

	do {
		crc <<= 1;
		in <<= 1;
		if (in & CRC_BIT9_MASK)
			++crc;
		if (crc & CRC_BIT17_MASK)
			crc ^= CRC16_POLYMIAL;
	} while (!(in & CRC_BIT17_MASK));

	return crc & CRC16_RESULT_MASK;
}

static uint16_t copy_and_calc_crc16(const uint8_t *src, uint32_t size, uint8_t *dest)
{
	uint32_t crc = 0;
	const uint8_t *data_end = src + size;
	uint16_t index = 0;
	while (src < data_end) {
		dest[index] = *src;
		index++;
		crc = update_crc16(crc, *src++);
	}

	crc = update_crc16(crc, 0);
	crc = update_crc16(crc, 0);

	return crc & CRC16_RESULT_MASK;
}

static int32_t update_firmware_package(struct i2c_client *client, const struct firmware *fw)
{
	int32_t ret = -1;
	int32_t i, j;
	uint16_t wait_time;
	struct update_command *update_cmd =
		(struct update_command *)kmalloc(sizeof(struct update_command), GFP_KERNEL);
	if (!update_cmd) {
		hwlog_err("update_cmd mallock error");
		return -ENOMEM;
	}

	memset_s(update_cmd, sizeof(struct update_command), 0, sizeof(struct update_command));
	/* distribute in multiple packages to send */
	update_cmd->cmd_id = COMMAND_UPDATE_PACKAGE_SEND;
	update_cmd->len = sizeof(struct update_command) - 1;
	update_cmd->package_num_count = fw->size / UPDATE_PACKAGE_LEN;
	for (i = 0; i < update_cmd->package_num_count; i++) {
		update_cmd->package_num = i;
		update_cmd->crc = copy_and_calc_crc16(&fw->data[i * UPDATE_PACKAGE_LEN],
						      UPDATE_PACKAGE_LEN, update_cmd->data);

		if (i == 0) {
			wait_time = FIRST_PACKAGE_WAIT_TIME;
		} else if (i == update_cmd->package_num_count - 1) {
			wait_time = LAST_PACKAGE_WAIT_TIME;
		} else {
			wait_time = NORMAL_PACKAGE_WAIT_TIME;
		}

		for (j = 0; j < UPDATE_RETRY_TIME; j++) {
			ret = piezo_key_send_cmd(client, (uint8_t *)update_cmd,
						  sizeof(struct update_command), wait_time);
			if (ret == 0) {
				break;
			} else {
				hwlog_err("Package %d time %d process failed. ret %d",
					  i, j, ret);
			}
		}
		if (ret != 0) {
			hwlog_err("Package %d process failed", i);
			kfree(update_cmd);
			return ret;
		}
	}
	kfree(update_cmd);
	return ret;
}

static int32_t check_firmware_version_is_equal(struct i2c_client *client, const struct firmware *fw)
{
	int32_t ret, i;
	uint8_t cmd_id;
	uint8_t resp_buff[HEADER_LEN + VERSION_LEN] = {0};
	cmd_id = COMMAND_GET_VERSION;
	ret = piezo_key_readwrite(client, &cmd_id, 1, resp_buff, HEADER_LEN + VERSION_LEN);
	if (ret != 0) {
		hwlog_err("get version error!");
		return 0;
	}

	for (i = 0; i < VERSION_LEN; i++) {
		if (resp_buff[HEADER_LEN + i] != fw->data[FIRMWARE_VERSION_INDEX + i])
			break;
	}
	if (i == VERSION_LEN) {
		hwlog_info("Already newest version %u.%u.%u.%u",
			   resp_buff[HEADER_LEN + 0], resp_buff[HEADER_LEN + 1],
			   resp_buff[HEADER_LEN + 2], resp_buff[HEADER_LEN + 3]); // 2 3 version offset
		return 1;
	}
	return 0;
}

static int32_t jump_app_or_boot_func(struct i2c_client *client, enum jump_area_t area)
{
	int32_t ret;
	uint8_t jump_boot_cmd[] = {0x00, 0x03, 0x00, 0x01};
	uint8_t jump_app_cmd[] = {0x00, 0x03, 0x00, 0x02};
	uint8_t cmd_id = COMMAND_GET_VERSION;
	uint8_t resp_buff[HEADER_LEN + VERSION_LEN] = {0};
	uint8_t i;
	for (i = 0; i < UPDATE_RETRY_TIME; i++) {
		if (area == BOOTLOADER) {
			ret = piezo_key_readwrite(client, jump_boot_cmd, sizeof(jump_boot_cmd),
						   NULL, 0);
			mdelay(100); // 100ms wait Bootloader init
			ret = piezo_key_readwrite(client, &cmd_id, 1,
						   resp_buff, HEADER_LEN + VERSION_LEN);
			if (ret != 0 || resp_buff[HEADER_LEN] != 0) { // 0 express Bootloader
				hwlog_err("Jump Bootloader Failed. Retry %u", i);
				continue;
			} else {
				break;
			}
		} else if (area == APPLICATION) {
			ret = piezo_key_readwrite(client, jump_app_cmd, sizeof(jump_app_cmd),
						   NULL, 0);
			mdelay(500); // 500ms wait app init
			ret = piezo_key_readwrite(client, &cmd_id, 1,
						   resp_buff, HEADER_LEN + VERSION_LEN);
			if (ret != 0 || resp_buff[HEADER_LEN] != 1) { // 1 express App
				hwlog_err("Jump App Failed. Retry %u", i);
				continue;
			} else {
				break;
			}
		}
	}
	if (i == UPDATE_RETRY_TIME)
		return (area == BOOTLOADER) ? FT_JUMP_BOOT_FAILED : FT_JUMP_APP_FAILED;

	return FT_SUCCESS;
}

static int32_t load_program_func(struct i2c_client *client,
				 const struct firmware *fw, int32_t check_flag)
{
	int32_t ret;
	int32_t jump_app_ret;
	struct piezo_key_data *ts = i2c_get_clientdata(client);
	if (!ts)
		return -EINVAL;

	ts->update_status = PIEZO_KEY_UPDATE_PROCESSING;
	if (check_flag && check_firmware_version_is_equal(client, fw)) {
		ts->update_status = PIEZO_KEY_UPDATE_SUCCESS;
		return 0;
	}

	ret = jump_app_or_boot_func(client, BOOTLOADER);
	if (ret != 0)
		goto exit;

	ret = update_firmware_package(client, fw);
	if (ret != 0)
		hwlog_err("update_firmware_package failed, ret: %d", ret);

	// jump back to APP at the end whether success or not 
	jump_app_ret = jump_app_or_boot_func(client, APPLICATION);
exit:
	ts->update_status = (ret == 0) && (jump_app_ret == 0);
	return (ret == 0) ? jump_app_ret : ret;
}

static int32_t clear_mcu_log(struct i2c_client *client)
{
	uint8_t cmd_buff[SEND_CMD_BUFFER_LEN] = {0};
	int32_t ret;
	struct command_data *send_cmd_struct = (struct command_data *)cmd_buff;

	send_cmd_struct->cmd_id = COMMAND_CLEAR_LOG;
	send_cmd_struct->len = HEADER_LEN + 1;
	send_cmd_struct->data[0] = 2; // 2 clear all log
	ret = piezo_key_send_cmd(client, cmd_buff, SEND_CMD_HEADER_LEN + 1, 100); // 100 waitms
	if (ret != 0)
		hwlog_err("clear log ret : %d", ret);

	return ret;
}

static int32_t get_mcu_log_and_print(struct i2c_client *client)
{
	uint8_t cmd_id = COMMAND_GET_LOG;
	int32_t ret, i;
	struct log_data *log_list = NULL;
	uint8_t *get_log_buff;
	get_log_buff = kmalloc(HEADER_LEN + sizeof(struct log_data), GFP_KERNEL);
	if (!get_log_buff) {
		hwlog_err("Get log malloc error");
		return -ENOMEM;
	}
	(void)memset_s(get_log_buff, HEADER_LEN + sizeof(struct log_data),
		       0, HEADER_LEN + sizeof(struct log_data));
	ret = piezo_key_readwrite(client, &cmd_id, 1,
				  get_log_buff, HEADER_LEN + sizeof(struct log_data));
	if (ret != 0) {
		hwlog_err("Get log error, piezo_key_readwrite ret : %d", ret);
		kfree(get_log_buff);
		return -EIO;
	}

	log_list = (struct log_data *)&get_log_buff[HEADER_LEN];
	if (log_list->log_num != 0) {
		hwlog_err("Log num: %u\n", log_list->log_num);
		for (i = 0; i < log_list->log_num; i++)
			hwlog_err("mcu log: %u 0x%08X %u %u %u %u\n",
				  log_list->log[i].log_tick, log_list->log[i].log_id,
				  log_list->log[i].para1, log_list->log[i].para2,
				  log_list->log[i].para3, log_list->log[i].para4);

		clear_mcu_log(client);
	}

	kfree(get_log_buff);
	return 0;
}

static int32_t self_vib_func(struct i2c_client *client, int32_t vib_time)
{
	int32_t ret, stop_ret;
	int32_t i;
	uint8_t cmd_buff[SEND_CMD_BUFFER_LEN] = {0};
	struct command_data *send_cmd_struct = (struct command_data *)cmd_buff;

	send_cmd_struct->cmd_id = COMMAND_CONTROL_VIB_TEST;
	send_cmd_struct->len = HEADER_LEN + 1;
	send_cmd_struct->data[0] = START_TEST;
	ret = piezo_key_send_cmd(client, cmd_buff, SEND_CMD_HEADER_LEN + 1, 30); // 30 waitms
	if (ret != 0) {
		hwlog_err("start self vib failed, ret : %d", ret);
		return ret;
	}

	for (i = 0; i < vib_time; i++) {
		send_cmd_struct->cmd_id = COMMAND_BIBRATE_TEST;
		send_cmd_struct->len = HEADER_LEN;
		ret = piezo_key_send_cmd(client, cmd_buff,
					  SEND_CMD_HEADER_LEN, 100); // 100 waitms
		if (ret != 0) {
			hwlog_err("self vib process failed, ret : %d", ret);
			break;
		}
	}

	send_cmd_struct->cmd_id = COMMAND_CONTROL_VIB_TEST;
	send_cmd_struct->len = HEADER_LEN + 1;
	send_cmd_struct->data[0] = STOP_TEST;
	stop_ret = piezo_key_send_cmd(client, cmd_buff,
				       SEND_CMD_HEADER_LEN + 1, 30); // 30 waitms
	if (stop_ret != 0)
		hwlog_err("stop self vib failed, ret : %d", stop_ret);

	return (ret || stop_ret);
}

static int32_t self_check_func(struct i2c_client *client)
{
	uint8_t cmd_buff[SEND_CMD_BUFFER_LEN] = {0};
	int32_t ret;
	struct command_data *send_cmd_struct = (struct command_data *)cmd_buff;

	send_cmd_struct->cmd_id = COMMAND_NOISE_TEST;
	send_cmd_struct->len = HEADER_LEN;
	ret = piezo_key_send_cmd(client, cmd_buff, SEND_CMD_HEADER_LEN, NOISE_TIME);
	if (ret != 0) {
		hwlog_err("noise test failed, ret : %d", ret);
		get_mcu_log_and_print(client);
		return ret;
	}

	send_cmd_struct->cmd_id = COMMAND_VOLTAGE_WAVE_TEST;
	send_cmd_struct->len = HEADER_LEN;
	ret = piezo_key_send_cmd(client, cmd_buff, SEND_CMD_HEADER_LEN, WAVE_TIME);
	if (ret != 0) {
		hwlog_err("wave test failed, ret : %d", ret);
		get_mcu_log_and_print(client);
	}

	return ret;
}

static int32_t piezo_key_open(struct inode *inode, struct file *file)
{
	hwlog_debug("piezo_key_open");
	file->private_data = g_ft_data;
	return 0;
}

static int32_t piezo_key_close(struct inode *inode, struct file *file)
{
	hwlog_debug("piezo_key_close");
	return 0;
}

static long piezo_key_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	int32_t ret_val, ret, param;
	uint8_t cmd_buff[SEND_CMD_BUFFER_LEN] = {0};
	struct piezo_key_data *ts = filp->private_data;
	uint8_t *resp_buff = NULL;

	if (!ts)
		return -EINVAL;

	resp_buff = kmalloc(READ_MAX_BUFF_LEN, GFP_KERNEL);
	if (!resp_buff)
		return -ENOMEM;

	memset_s(resp_buff, READ_MAX_BUFF_LEN, 0, READ_MAX_BUFF_LEN);
	switch (cmd) {
	case GET_VERSION_CMD:
		cmd_buff[0] = COMMAND_GET_VERSION;
		ret = piezo_key_readwrite(ts->client, cmd_buff, 1,
					   resp_buff, HEADER_LEN + VERSION_LEN);
		ret_val = copy_to_user((void *)arg, &resp_buff[HEADER_LEN],
				       sizeof(struct version_struct));
		break;
	case SELF_CHECK_CMD:
		ret = self_check_func(ts->client);
		ret_val = put_user(ret, (int32_t __user *)arg);
		break;
	case SELF_VIB_CMD:
		get_user(param, (int32_t __user *)arg);
		ret = self_vib_func(ts->client, param);
		ret_val = put_user(ret, (int32_t __user *)arg);
		break;
	case GET_LOG_CMD:
		cmd_buff[0] = COMMAND_GET_LOG;
		ret = piezo_key_readwrite(ts->client, cmd_buff, 1,
					   resp_buff, HEADER_LEN + sizeof(struct log_data));
		ret_val = copy_to_user((void *)arg, &resp_buff[HEADER_LEN],
				       sizeof(struct log_data));
		break;
	case CLEAR_LOG_CMD:
		ret = clear_mcu_log(ts->client);
		ret_val = put_user(ret, (int32_t __user *)arg);
		break;
	default:
		hwlog_err("unsupport cmd\n");
		ret_val = -EINVAL;
		break;
	}
	kfree(resp_buff);
	return ret_val;
}

static const struct file_operations piezo_key_fops = {
	.owner = THIS_MODULE,
	.open = piezo_key_open,
	.unlocked_ioctl = piezo_key_ioctl,
	.release = piezo_key_close,
};

static int32_t piezo_key_check_chip(struct i2c_client *client)
{
	uint8_t get_chip_id_cmd = COMMAND_GET_CHIPID;
	uint8_t resp_buff[HEADER_LEN + 1] = {0};
	int32_t ret;
	struct piezo_key_data *ts = i2c_get_clientdata(client);
	if (!ts)
		return -EINVAL;

	ret = piezo_key_readwrite(client, &get_chip_id_cmd, 1, resp_buff, HEADER_LEN + 1);
	if (ret != 0) { // check return value
		ts->i2c_status = 0;
		hwlog_err("Failed to read chipid. ret: %d", ret);
		return ret;
	}
	if (resp_buff[HEADER_LEN] != 0x15) { // check chipid
		ts->i2c_status = 0;
		hwlog_err("Check chipid error, chipid: %u", resp_buff[HEADER_LEN]);
		return -EIO;
	}
	ts->i2c_status = 1;
	return ret;
}

static void piezo_key_firmware_cb(const struct firmware *fw, void *ctx)
{
	int32_t ret;
	struct piezo_key_data *ts = (struct piezo_key_data *)ctx;
	if (!ts) {
		hwlog_err("piezo_key_firmware_cb ctx NULL!");
		return;
	}

	if (fw) {
		ret = load_program_func(ts->client, fw, 1);
		if (ret != 0)
			hwlog_err("load_program_func error. ret: %d", ret);
	} else {
		hwlog_err("piezo key firmware load failed");
	}

	release_firmware(fw);
}

static ssize_t piezo_key_get_version_show(struct device *dev,
					  struct device_attribute *attr, char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	uint8_t cmd_id = COMMAND_GET_VERSION;
	uint8_t version_buf[HEADER_LEN + VERSION_LEN] = {0};
	int32_t ret;
	ret = piezo_key_readwrite(client, &cmd_id, 1, version_buf, HEADER_LEN + VERSION_LEN);
	if (ret != 0) {
		hwlog_err("Failed to read mcu version. ret: %d", ret);
		return -EIO;
	}

	return sprintf_s(buf, PAGE_SIZE, "%u.%u.%u.%u\n",
			 version_buf[HEADER_LEN],
			 version_buf[HEADER_LEN + 1],
			 version_buf[HEADER_LEN + 2], // 2 version offset
			 version_buf[HEADER_LEN + 3]); // 3 version offset
}

static ssize_t piezo_key_get_update_status_show(struct device *dev,
						struct device_attribute *attr, char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct piezo_key_data *ts = i2c_get_clientdata(client);
	if (!ts) {
		hwlog_err("piezo_key_get_update_status_show piezo_key_data NULL!");
		return -EINVAL;
	}

	return sprintf_s(buf, PAGE_SIZE, "%d\n", ts->update_status);
}

static ssize_t piezo_key_get_i2c_status_show(struct device *dev,
					     struct device_attribute *attr, char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct piezo_key_data *ts = i2c_get_clientdata(client);
	if (!ts) {
		hwlog_err("piezo_key_get_i2c_status_show piezo_key_data NULL!");
		return -EINVAL;
	}

	if (ts->update_status == PIEZO_KEY_UPDATE_PROCESSING) {
		return sprintf_s(buf, PAGE_SIZE, "%d\n", ts->i2c_status);
	}
	return sprintf_s(buf, PAGE_SIZE, "%d\n", (piezo_key_check_chip(client) == 0));
}

static ssize_t piezo_key_get_device_info_show(struct device *dev,
					      struct device_attribute *attr, char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	uint8_t cmd_id = COMMAND_GET_DEVICE_INFO;
	uint8_t get_device_info_buf[HEADER_LEN + DEVICE_INFO_LEN] = {0};
	int32_t ret;
	ret = piezo_key_readwrite(client, &cmd_id, 1,
				   get_device_info_buf, HEADER_LEN + DEVICE_INFO_LEN);
	if (ret != 0) {
		hwlog_err("Failed to read device info. ret: %d", ret);
		return -EIO;
	}
	return sprintf_s(buf, PAGE_SIZE, "%s,%u.%u.%u.%u,%u\n",
			 get_device_info_buf[HEADER_LEN] == 0 ? "HC32F460" : "Error",
			 get_device_info_buf[HEADER_LEN + 1], // 1 version offset
			 get_device_info_buf[HEADER_LEN + 2], // 2 version offset
			 get_device_info_buf[HEADER_LEN + 3], // 3 version offset
			 get_device_info_buf[HEADER_LEN + 4], // 4 version offset
			 get_device_info_buf[HEADER_LEN + 5]); // 5 boardversion offset
}

static ssize_t piezo_key_trig_update_store(struct device *dev, struct device_attribute *attr,
					   const char *buf, size_t count)
{
	struct i2c_client *client = to_i2c_client(dev);
	int32_t ret;
	const struct firmware *fw = NULL;

	ret = request_firmware(&fw, FIRMWARE_NAME, &client->dev);
	if (ret)  {
		hwlog_err("Load Firmware Bin Failed");
		return -ENOENT;
	}

	ret = load_program_func(client, fw, 0);
	if (ret != 0)
		hwlog_err("load_program_func error. ret: %d", ret);

	release_firmware(fw);
	return count;
}

static ssize_t piezo_key_get_mcu_log_show(struct device *dev,
					   struct device_attribute *attr, char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	uint8_t cmd_id = COMMAND_GET_LOG;
	int32_t ret, i, return_size;
	struct log_data *log_list = NULL;
	uint8_t *get_log_buff;
	get_log_buff = kmalloc(HEADER_LEN + sizeof(struct log_data), GFP_KERNEL);
	if (!get_log_buff)
		return -ENOMEM;

	memset_s(get_log_buff, HEADER_LEN + sizeof(struct log_data),
		 0, HEADER_LEN + sizeof(struct log_data));
	ret = piezo_key_readwrite(client, &cmd_id, 1,
				   get_log_buff, HEADER_LEN + sizeof(struct log_data));
	if (ret != 0) {
		hwlog_err("Get log error, piezo_key_readwrite ret : %d", ret);
		kfree(get_log_buff);
		return -EIO;
	}

	return_size = 0;
	log_list = (struct log_data *)&get_log_buff[HEADER_LEN];
	return_size += sprintf_s(buf, PAGE_SIZE, "Log num: %u\n", log_list->log_num);
	if (log_list->log_num != 0) {
		for (i = 0; i < log_list->log_num; i++)
			return_size += sprintf_s(buf + return_size, PAGE_SIZE - return_size,
						 "log: %u 0x%08X %u %u %u %u\n",
						 log_list->log[i].log_tick,
						 log_list->log[i].log_id,
						 log_list->log[i].para1,
						 log_list->log[i].para2,
						 log_list->log[i].para3,
						 log_list->log[i].para4);

		clear_mcu_log(client);
	}
	kfree(get_log_buff);
	return return_size;
}

static ssize_t piezo_key_set_thresh_store(struct device *dev, struct device_attribute *attr,
					  const char *buf, size_t count)
{
	struct i2c_client *client = to_i2c_client(dev);
	uint16_t thresh_params[SELF_CHECK_PARAM_NUM] = {0};
	uint8_t cmd_buff[SEND_CMD_BUFFER_LEN] = {0};
	int32_t ret;
	struct command_data *send_cmd_struct = (struct command_data *)cmd_buff;

	ret = sscanf_s(buf, "%u,%u,%u,%u,%u", &thresh_params[0], &thresh_params[1],
		       &thresh_params[2], &thresh_params[3], &thresh_params[4]); // 2 3 4 offset
	if (ret != SELF_CHECK_PARAM_NUM) {
		hwlog_err("thresh input error, ret: %d", ret);
		return -EINVAL;
	}
	send_cmd_struct->cmd_id = COMMAND_SET_SELF_CHECK_PARAM;
	send_cmd_struct->len = HEADER_LEN + SELF_CHECK_PARAM_NUM * sizeof(uint16_t);
	ret = memcpy_s(send_cmd_struct->data, SEND_CMD_BUFFER_LEN - SEND_CMD_HEADER_LEN,
		       thresh_params, SELF_CHECK_PARAM_NUM * sizeof(uint16_t));
	if (ret != 0) {
		hwlog_err("copy data to cmd struct failed, ret : %d", ret);
		return ret;
	}

	ret = piezo_key_send_cmd(client, cmd_buff,
				 SEND_CMD_HEADER_LEN + SELF_CHECK_PARAM_NUM * sizeof(uint16_t),
				 200); // 200 waitms
	if (ret != 0) {
		hwlog_err("set self check thresh failed, ret : %d", ret);
		return -EIO;
	}
	return count;
}

static ssize_t piezo_key_get_u16_data_to_string(struct device *dev, uint8_t cmd_id,
					    	uint16_t num, char *buf, ssize_t buf_size)
{
	ssize_t return_size = 0;
	int32_t ret, i;
	struct i2c_client *client = to_i2c_client(dev);
	uint8_t *cmd_buf = (uint8_t *)kmalloc(HEADER_LEN + sizeof(uint16_t) * num, GFP_KERNEL);
	uint16_t *raw_data = NULL;
	if (!cmd_buf) {
		hwlog_err("Get mcu data malloc error");
		return -ENOMEM;
	}
	(void)memset_s(cmd_buf, HEADER_LEN + sizeof(uint16_t) * num,
		       0, HEADER_LEN + sizeof(uint16_t) * num);
	ret = piezo_key_readwrite(client, &cmd_id, 1, cmd_buf,
				  HEADER_LEN + sizeof(int16_t) * num);
	if (ret != 0) {
		hwlog_err("Get mcu data error, cmdid: %u, ret : %d", cmd_id, ret);
		kfree(cmd_buf);
		return -EIO;
	}

	raw_data = (uint16_t *)&cmd_buf[HEADER_LEN];
	for (i = 0; i < num; i++) {
		ret = sprintf_s(buf + return_size, buf_size - return_size,
				"%u,", raw_data[i]);
		if (ret < 0) {
			hwlog_err("sprintf_s error ret: %d", ret);
			kfree(cmd_buf);
			return ret;
		}
		return_size += ret;
	}
	ret = sprintf_s(buf + return_size, buf_size - return_size, "\n");
	if (ret < 0) {
		hwlog_err("sprintf_s error ret: %d", ret);
		kfree(cmd_buf);
		return ret;
	}
	return_size += ret;

	kfree(cmd_buf);
	return return_size;
}

static ssize_t piezo_key_get_factory_bigdata_show(struct device *dev,
						  struct device_attribute *attr, char *buf)
{
	return piezo_key_get_u16_data_to_string(dev, COMMAND_GET_FACTORY_BIGDATA,
						18, buf, PAGE_SIZE); // 18 param num
}

static ssize_t piezo_key_get_mcu_rawdata_show(struct device *dev,
					      struct device_attribute *attr, char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	uint8_t cmd_id = COMMAND_GET_MCU_RAWDATA;
	ssize_t return_size = 0;
	int32_t ret, i;
	uint8_t raw_data_buff[HEADER_LEN + sizeof(uint16_t)] = {0};
	uint16_t *raw_data = (uint16_t *)&raw_data_buff[HEADER_LEN];

	for (i = 0; i < 500; i++) { // sample 500 data
		ret = piezo_key_readwrite(client, &cmd_id, 1,
					  raw_data_buff, HEADER_LEN + sizeof(uint16_t));
		if (ret != 0) {
			hwlog_err("Get rawdata error, piezo_key_readwrite ret : %d", ret);
			return -EIO;
		}
		ret = sprintf_s(buf + return_size, PAGE_SIZE - return_size,
				"%u\n",  raw_data[0]);
		if (ret < 0) {
			hwlog_err("sprintf_s error ret: %d", ret);
			return ret;
		}
		return_size += ret;
		mdelay(6); // 6ms interval
	}

	return return_size;
}

static ssize_t piezo_key_get_system_status_show(struct device *dev,
						struct device_attribute *attr, char *buf)
{
	return piezo_key_get_u16_data_to_string(dev, COMMAND_GET_SYSTEM_STATUS,
						2, buf, PAGE_SIZE); // 2 param num
}

static ssize_t piezo_key_get_wave_info_show(struct device *dev,
						struct device_attribute *attr, char *buf)
{
	return piezo_key_get_u16_data_to_string(dev, COMMAND_GET_WAVE_INFO,
						100, buf, PAGE_SIZE); // 100 param num
}

static ssize_t piezo_key_get_target_wave_show(struct device *dev,
						struct device_attribute *attr, char *buf)
{
	return piezo_key_get_u16_data_to_string(dev, COMMAND_GET_TARGET_WAVE,
						500, buf, PAGE_SIZE); // 500 param num
}

static ssize_t piezo_key_get_actual_wave_show(struct device *dev,
						struct device_attribute *attr, char *buf)
{
	return piezo_key_get_u16_data_to_string(dev, COMMAND_GET_ACTUAL_WAVE,
						500, buf, PAGE_SIZE); // 500 param num
}

static ssize_t piezo_key_get_pwm_param_show(struct device *dev,
						struct device_attribute *attr, char *buf)
{
	return piezo_key_get_u16_data_to_string(dev, COMMAND_GET_PWM_PARAM,
						500, buf, PAGE_SIZE); // 500 param num
}

static DEVICE_ATTR(get_version, S_IRUGO, piezo_key_get_version_show, NULL);
static DEVICE_ATTR(get_update_status, S_IRUGO, piezo_key_get_update_status_show, NULL);
static DEVICE_ATTR(get_i2c_status, S_IRUGO, piezo_key_get_i2c_status_show, NULL);
static DEVICE_ATTR(trig_update, S_IWUSR, NULL, piezo_key_trig_update_store);
static DEVICE_ATTR(get_device_info, S_IRUGO, piezo_key_get_device_info_show, NULL);
static DEVICE_ATTR(get_mcu_log, S_IRUGO, piezo_key_get_mcu_log_show, NULL);
static DEVICE_ATTR(set_thresh, S_IWUSR, NULL, piezo_key_set_thresh_store);
static DEVICE_ATTR(get_factory_bigdata, S_IRUGO, piezo_key_get_factory_bigdata_show, NULL);
static DEVICE_ATTR(get_mcu_rawdata, S_IRUGO, piezo_key_get_mcu_rawdata_show, NULL);
static DEVICE_ATTR(get_system_status, S_IRUGO, piezo_key_get_system_status_show, NULL);
static DEVICE_ATTR(get_wave_info, S_IRUGO, piezo_key_get_wave_info_show, NULL);
static DEVICE_ATTR(get_target_wave, S_IRUGO, piezo_key_get_target_wave_show, NULL);
static DEVICE_ATTR(get_actual_wave, S_IRUGO, piezo_key_get_actual_wave_show, NULL);
static DEVICE_ATTR(get_pwm_param, S_IRUGO, piezo_key_get_pwm_param_show, NULL);

static struct attribute *piezo_key_attributes[] = {
	&dev_attr_get_version.attr,
	&dev_attr_get_update_status.attr,
	&dev_attr_get_i2c_status.attr,
	&dev_attr_trig_update.attr,
	&dev_attr_get_device_info.attr,
	&dev_attr_get_mcu_log.attr,
	&dev_attr_set_thresh.attr,
	&dev_attr_get_factory_bigdata.attr,
	&dev_attr_get_mcu_rawdata.attr,
	&dev_attr_get_system_status.attr,
	&dev_attr_get_wave_info.attr,
	&dev_attr_get_target_wave.attr,
	&dev_attr_get_actual_wave.attr,
	&dev_attr_get_pwm_param.attr,
	NULL,
};

static struct attribute_group piezo_key_attr_group = {
	.attrs = piezo_key_attributes,
};

static void report_piezo_key_event(int32_t keycode)
{
	input_report_key(g_ft_data->input_dev, keycode, 1);
	input_sync(g_ft_data->input_dev);

	input_report_key(g_ft_data->input_dev, keycode, 0);
	input_sync(g_ft_data->input_dev);
}

static void piezo_key_int_work_func(__attribute__((unused)) struct work_struct *work)
{
	int32_t ret;
	uint8_t event_type;

	if (g_ft_data == NULL) {
		hwlog_err("g_ft_data NULL");
		return;
	}
	ret = piezo_key_readwrite(g_ft_data->client, NULL, 0, &event_type, 1);
	if (ret != 0) {
		hwlog_err("read key event error %d", ret);
		return;
	}

	if (event_type == PIEZO_KEY_SHORT_PRESS) {
		report_piezo_key_event(g_ft_data->short_press_keycode);
	} else if (event_type == PIEZO_KEY_LONG_PRESS) {
		report_piezo_key_event(g_ft_data->long_press_keycode);
	} else {
		hwlog_err("unknown keyevent: %u", event_type);
	}
}

static irqreturn_t event_irq_handler(__attribute__((unused)) int32_t irq,
				     __attribute__((unused)) void *dev_id)
{
	schedule_work(&g_ft_data->work);
	return IRQ_HANDLED;
}

static int32_t piezo_key_init_irq(const struct i2c_client *client, struct piezo_key_data *ts)
{
	struct device_node *np = client->dev.of_node;
	int32_t error;

	INIT_WORK(&ts->work, piezo_key_int_work_func);

	ts->event_gpio = of_get_named_gpio(np, "gpio_irq", 0);
	if (!gpio_is_valid(ts->event_gpio)) {
		hwlog_err("irq gpio isn't valid, chk DTS");
		return -EINVAL;
	}

	error = of_property_read_s32(np, DTS_SHORT_PRESS_KEYCODE, &(ts->short_press_keycode));
	if (error != 0) {
		hwlog_err("short press keycode is not valid, chk DTS");
		return error;
	}
	error = of_property_read_s32(np, DTS_LONG_PRESS_KEYCODE, &(ts->long_press_keycode));
	if (error != 0) {
		hwlog_err("long press keycode is not valid, chk DTS");
		return error;
	}

	error = gpio_request((uint32_t)ts->event_gpio, "piezo_key_irq");
	if (error != 0) {
		hwlog_err("gpio request failed, gpio: %d", ts->event_gpio);
		return error;
	}
	ts->event_irq = gpio_to_irq((uint32_t)ts->event_gpio);
	if (ts->event_irq < 0) {
		hwlog_err("gpio_to_irq failed, gpio_num: %d", ts->event_gpio);
		return -EINVAL;
	}
	error = request_irq((uint32_t)ts->event_irq, event_irq_handler,
			    IRQF_TRIGGER_FALLING, "piezo_key_event", ts);
	if (error != 0) {
		hwlog_err("gpio irq register failed");
		return error;
	}
	return 0;
}

static int32_t piezo_key_regist_input_device(struct i2c_client *client, struct piezo_key_data *ts)
{
	int32_t error;

	ts->input_dev = input_allocate_device();
	if (!ts->input_dev) {
		hwlog_err("Failed to allocate struct input_dev!");
		return -ENOMEM;
	}

	ts->input_dev->name = "piezo_key_event";
	ts->input_dev->id.bustype = BUS_I2C;
	ts->input_dev->dev.parent = &client->dev;

	set_bit(EV_KEY, ts->input_dev->evbit);
	set_bit(EV_SYN, ts->input_dev->evbit);
	set_bit(ts->short_press_keycode, ts->input_dev->keybit);
	set_bit(ts->long_press_keycode, ts->input_dev->keybit);

	error = input_register_device(ts->input_dev);
	if (error != 0) {
		hwlog_err("Failed to register input device!");
		input_free_device(ts->input_dev);
		return error;
	}
	return 0;
}

static int32_t piezo_key_create_device(struct i2c_client *client, struct piezo_key_data *ts)
{
	int32_t error;
	struct device *piezo_key_dev = NULL;

	error = register_chrdev(PIEZO_KEY_MAJOR, "piezo_key", &piezo_key_fops);
	if (error != 0) {
		hwlog_err("register driver failed, ret %d", error);
		return -ENODEV;
	}
	ts->piezo_key_class = class_create(THIS_MODULE, "piezo_key");
	if (IS_ERR(ts->piezo_key_class)) {
		hwlog_err("create class failed");
		error = -ENODEV;
		goto free_devm;
	}
	piezo_key_dev = device_create(ts->piezo_key_class, NULL, MKDEV(PIEZO_KEY_MAJOR, 0), NULL,
				      "piezo_key");
	if (IS_ERR(piezo_key_dev)) {
		hwlog_err("create device failed");
		error = -ENODEV;
		goto free_class;
	}

	error = sysfs_create_group(&client->dev.kobj, &piezo_key_attr_group);
	if (error != 0) {
		hwlog_err("sysfs_create_group failed, ret %d", error);
		goto free_device;
	}

	return 0;
free_device:
	device_destroy(ts->piezo_key_class, MKDEV(PIEZO_KEY_MAJOR, 0));
free_class:
	class_destroy(ts->piezo_key_class);
free_devm:
	unregister_chrdev(PIEZO_KEY_MAJOR, "piezo_key");
	return error;
}

static int32_t piezo_key_probe(struct i2c_client *client,
			       const struct i2c_device_id *id)
{
	int32_t error;
	struct piezo_key_data *ts;

	ts = devm_kzalloc(&client->dev, sizeof(*ts), GFP_KERNEL);
	if (!ts) {
		hwlog_err("piezo_key devm_kzalloc failed");
		return -ENOMEM;
	}

	g_ft_data = ts;
	ts->client = client;
	i2c_set_clientdata(client, ts);
	ts->update_status = PIEZO_KEY_UPDATE_INIT_STATUS;

	error = piezo_key_check_chip(client);
	if (error != 0) {
		hwlog_err("piezo_key_check_chip failed, ret: %d", error);
		goto err;
	}

	error = piezo_key_init_irq(client, ts);
	if (error != 0) {
		hwlog_err("piezo_key_init_irq failed, ret: %d", error);
		goto err;
	}

	error = piezo_key_regist_input_device(client, ts);
	if (error != 0) {
		hwlog_err("piezo_key_regist_input_device failed, ret: %d", error);
		goto err;
	}

	error = request_firmware_nowait(THIS_MODULE, true, FIRMWARE_NAME, &client->dev,
					GFP_KERNEL, ts, piezo_key_firmware_cb);
	if (error != 0) {
		hwlog_err("failed to invoke firmware loader: %d", error);
		goto err;
	}

	error = piezo_key_create_device(client, ts);
	if (error != 0) {
		hwlog_err("piezo_key_create_device failed, ret: %d", error);
		goto err;
	}

#ifdef CONFIG_HUAWEI_HW_DEV_DCT
	set_hw_dev_flag(DEV_I2C_FORCE_TOUCH);
#endif
	return 0;
err:
	devm_kfree(&client->dev, ts);
	return error;
}

static int32_t piezo_key_remove(struct i2c_client *client)
{
	struct piezo_key_data *ts = (struct piezo_key_data *)i2c_get_clientdata(client);
	if (!ts) {
		hwlog_err("piezo_key_data null!\n");
		return -EINVAL;
	}

	device_destroy(ts->piezo_key_class, MKDEV(PIEZO_KEY_MAJOR, 0));
	sysfs_remove_group(&client->dev.kobj, &piezo_key_attr_group);
	class_destroy(ts->piezo_key_class);
	input_free_device(ts->input_dev);
	unregister_chrdev(PIEZO_KEY_MAJOR, "piezo_key");
	devm_kfree(&client->dev, ts);
	return 0;
}

static int32_t __maybe_unused piezo_key_suspend(struct device *dev)
{
	hwlog_debug("piezo_key_suspend\n");
	return 0;
}

static int32_t __maybe_unused piezo_key_resume(struct device *dev)
{
	hwlog_debug("piezo_key_resume\n");
	return 0;
}

static SIMPLE_DEV_PM_OPS(piezo_key_i2c_pm, piezo_key_suspend, piezo_key_resume);

static const struct i2c_device_id piezo_key_id[] = {
	{ "piezo_key", 0 },
	{}
};
MODULE_DEVICE_TABLE(i2c, piezo_key_id);

#ifdef CONFIG_OF
static const struct of_device_id piezo_key_i2c_id[] = {
	{ .compatible = "hisilicon,piezo-key" },
	{}
};
MODULE_DEVICE_TABLE(of, piezo_key_i2c_id);
#endif

static struct i2c_driver piezo_key_driver = {
	.driver = {
		.name = "piezo_key",
		.of_match_table = piezo_key_i2c_id,
		.pm = &piezo_key_i2c_pm,
	},
	.probe = piezo_key_probe,
	.remove = piezo_key_remove,
	.id_table = piezo_key_id,
};

module_i2c_driver(piezo_key_driver);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("Piezo key I2C bus driver");
