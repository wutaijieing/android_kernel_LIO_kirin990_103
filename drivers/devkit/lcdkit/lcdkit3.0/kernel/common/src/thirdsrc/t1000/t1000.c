/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
 * Description: for t1000 driver ic
 */
/*
 * t1000.c
 *
 * t1000 driver for eink panel
 *
 * Copyright (c) 2019-2019 Huawei Technologies Co., Ltd.
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

#include <linux/module.h>
#include <linux/delay.h>
#include <linux/spi/spi.h>
#include <linux/i2c.h>
#include <linux/of.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/interrupt.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/of_regulator.h>
#include <linux/regulator/machine.h>
#include <asm/uaccess.h>
#include <linux/fb.h>
#include "t1000.h"
#include "lcd_kit_common.h"
#include "lcd_kit_panel.h"
#include <securec.h>
#include <chipset_common/hwpower/common_module/power_cmdline.h>
#include "dpu_dpe_utils.h"
#include "hisi_fb.h"

#define MEM_MAX_SIZE (1024 * 1024)
#define GPIO_FASTBOOT_DELAY                     10
#define GPIO_3V3_EN_DELAY                       500
#define GPIO_READY_DELAY                        10
#define GPIO_WAKEUP_DELAY                       1
#define ENTER_SLEEP_MODE_DELAY                  100
#define REG_ENABLE_FASTBOOT_DELAY               10
#define REG_ENABLE_3V3_EN_DELAY                 600
#define REG_DISABLE_WAKEUP_DELAY                500
#define GPIO_READY                              227
#define WRITE_MAX_LENGTH                        (8 * 1024)
#define WAITING_READY_COUNT                     100
#define WAIT_T1000_SUSPEND                      60
#define GPIO_RESET_LOW_DELAY                    2
#define GPIO_3V3_EN_HIGH_DELAY                  2
#define GPIO_TEST_CFG_HIGH_DELAY                1
#define GPIO_TEST_CFG_LOW_DELAY                 2
#define POWER_ON_WORK_DELAY                     1500
#define T100_VOLTAGE                            1234
#define T100_SET_MODE_NUM                       2
#define T1000_DECIMAL                           10
#define GPIO_POWER_TIME_DELAY                   3
#define T1000_OFF_RESPOND_NUM                   150
#define T1000_READY_PIN_TIMES                   3
#define T1000_ESD_MAX_DET_TIME                  10
#define T1000_NO_SUSPEND_RET                    1
#define FAST_READ_MODE_MAIN                     3
#define FAST_READ_MODE_SUB                      0

struct work_struct eink_suspend_work;
static bool is_suspend = false;
static int esd_happened = 0;
static void t1000_fastboot_gpio_set(int enable);
static struct delayed_work t1000_esd_delay_work;
static struct delayed_work t1000_lp_delay_work;
static struct delayed_work t1000_ref_mode_delay_work;
static unsigned int is_recovery_mode;
static bool is_pen_suspend = true;
static unsigned long spi_write_time = 0;
static unsigned int ready_pull_up_times = 0;
static unsigned int t1000_upgrade_status = 0;
static unsigned int t1000_waveform_status = 0;
static unsigned int t1000_low_power_status = 0;
static unsigned int first_mode = FAST_READ_MODE_MAIN;
static unsigned int first_sub_mode = FAST_READ_MODE_SUB;

static int t1000_power_off(void);
static int t1000_power_on(void);

static void t1000_esd_enable(struct hisi_fb_data_type *hisifd, int enable)
{
	struct hisi_panel_info *pinfo = NULL;

	if (hisifd == NULL) {
		LCD_KIT_ERR("hisifd is null\n");
		return;
	}
	pinfo = &(hisifd->panel_info);
	if (pinfo == NULL) {
		LCD_KIT_ERR("pinfo is null!\n");
		return;
	}
	if (common_info->esd.support)
		pinfo->esd_enable = enable;
	LCD_KIT_DEBUG("pinfo->esd_enable = %d\n", pinfo->esd_enable);
}

static struct t1000_data *get_t1000_data(void)
{
	struct t1000_data *data = NULL;
	struct i2c_client *client = NULL;
	if (!t1000dev) {
		LCD_KIT_ERR("t1000dev is null\n");
		return data;
	}
	client = to_i2c_client(t1000dev);
	if (!client) {
		LCD_KIT_ERR("client is null\n");
		return data;
	}
	data = i2c_get_clientdata(client);
	if (!data) {
		LCD_KIT_ERR("data is null\n");
		return data;
	}
	return data;
}

static bool t1000_get_esd_ckeck_status(int num, struct hisi_fb_data_type *hisifd)
{
	struct hisi_panel_info *pinfo = NULL;
	struct t1000_data *data = NULL;
	unsigned int wakeup_status = 0;

	data = get_t1000_data();
	if (!data) {
		LCD_KIT_ERR("data is null\n");
		return false;
	}

	if (hisifd == NULL) {
		LCD_KIT_ERR("hisifd is null\n");
		return false;
	}
	pinfo = &(hisifd->panel_info);
	if (pinfo == NULL) {
		LCD_KIT_ERR("pinfo is null!\n");
		return false;
	}
	wakeup_status = gpio_get_value(data->gpio_wakeup);
	LCD_KIT_DEBUG("pinfo->esd_enable = %d\n", pinfo->esd_enable);
	if (num == T1000_ESD_MAX_DET_TIME && !t1000_low_power_status && t1000_upgrade_status == 0 &&
		t1000_waveform_status == 1 && pinfo->esd_enable && wakeup_status == 0)
		return true;

	return false;
}

static int t1000_send(struct i2c_client *client, u8 *buf, size_t size)
{
	int ret;
	unsigned char *data = NULL;
	int i = 0;
	struct t1000_data *t_data = NULL;

	t_data = i2c_get_clientdata(client);
	if (!t_data) {
		LCD_KIT_ERR("data is null\n");
		return 0;
	}

	if (size < 0 || size > MEM_MAX_SIZE) {
		LCD_KIT_INFO("invalid length\n");
		return -ENOMEM;
	}
	data = kmalloc(size, GFP_KERNEL);
	if (data == NULL) {
		LCD_KIT_INFO("can not allocate memory\n");
		return -ENOMEM;
	}

	ret = memcpy_s(data, size, buf, size);
	if (ret != 0) {
		LCD_KIT_INFO("memcpy_s error\n");
		kfree(data);
		return ret;
	}

	while (!t_data->disp_ready && (i < WAITING_READY_COUNT)) {
		msleep(GPIO_READY_DELAY);
		i++;
	}
	if (i == WAITING_READY_COUNT)
		LCD_KIT_INFO("ready timeout, skip\n");
	ret = i2c_master_send(client, data, size);
	if (size != ret) {
	LCD_KIT_INFO("i2c master send error ret %d\n", ret);
		kfree(data);
		return ret;
	}

	kfree(data);
	return 0;
}

static int t1000_recv(struct i2c_client *client, u8 reg, u8 *buf, size_t size)
{
	int ret;
	unsigned char *data = NULL;
	int i = 0;
	struct t1000_data *t_data = NULL;

	t_data = i2c_get_clientdata(client);
	if (!t_data) {
		LCD_KIT_ERR("data is null\n");
		return 0;
	}
	while (!t_data->disp_ready && (i < WAITING_READY_COUNT)) {
		msleep(GPIO_READY_DELAY);
		i++;
	}
	if (i == WAITING_READY_COUNT)
		LCD_KIT_INFO("ready timeout, skip\n");

	ret = i2c_master_send(client, &reg, 1);
	if (ret != 1) {
		LCD_KIT_INFO("t1000 I2C send failed!!, Addr = 0x%x\n", reg);
		return ret;
	}

	if (size < 0 || size > MEM_MAX_SIZE) {
		LCD_KIT_INFO("invalid length\n");
		return -ENOMEM;
	}
	LCD_KIT_INFO("begin recive\n");
	data = kmalloc(size, GFP_KERNEL);
	ret = i2c_master_recv(client, data, size);
	if (size != ret) {
		LCD_KIT_INFO("t1000 I2C read failed!!\n");
		kfree(data);
		return ret;
	}
	ret = memcpy_s(buf, size, data, size);
	if (ret != 0) {
		LCD_KIT_INFO("memcpy_s error\n");
		kfree(data);
		return ret;
	}

	kfree(data);
	return 0;
}

static ssize_t t1000_set_mode(struct device *dev, int main_mipi_mode, int sub_mipi_mode)
{
	int ret;
	struct i2c_client *client = to_i2c_client(dev);
	struct t1000_command test_cmd = {
		.reg = T1000_REG_CMD,
		.command_code = UCMD_SET_MIPI_MODE,
		.args = {
			.mipi_mode.mode = main_mipi_mode,
			.mipi_mode.sub_mode = sub_mipi_mode,
		},
	};
	LCD_KIT_INFO("mode %d submode %d\n", test_cmd.args.mipi_mode.mode, test_cmd.args.mipi_mode.sub_mode);
	ret = t1000_send(client, (u8 *)&test_cmd, sizeof(test_cmd));
	if (ret)
		return -ret;
	else
		return LCD_KIT_OK;
}

static int t1000_get_mode(struct device *dev)
{
	int mipimode;
	struct i2c_client *client = to_i2c_client(dev);

	struct t1000_command cmd_get_mode = {
		.reg = T1000_REG_CMD,
		.command_code = UCMD_GET_MIPI_MODE,
	};

	// Todo: need show sleep correctly
	if (t1000_send(client, (u8 *)&cmd_get_mode, sizeof(cmd_get_mode))
		|| t1000_recv(client, T1000_REG_DATA, (u8 *)&mipimode, 1)) {
		mipimode = -1;
		LCD_KIT_INFO("error\n");
	} else {
		mipimode = mipimode & 0xFF;
		LCD_KIT_INFO("read mode = %d\n", mipimode);
	}

	return mipimode;
}

static int t1000_enter_sleep_mode(struct device *dev)
{
	int ret;
	struct i2c_client *client = to_i2c_client(dev);

	struct t1000_command cmd_set_mode = {
		.reg = T1000_REG_CMD,
		.command_code = UCMD_SET_MIPI_MODE,
		.args = {
			.mipi_mode.mode = T1000_SLEEP_MODE,
			.mipi_mode.sub_mode = 0,
		},
	};

	LCD_KIT_INFO("\n");
	ret = t1000_send(client, (u8 *)&cmd_set_mode, sizeof(cmd_set_mode));
	LCD_KIT_INFO("ret =  %d!\n", ret);

	return ret;
}
static void t1000_change_fastboot_status(void)
{
	int ret;
	struct t1000_data *data = NULL;
	struct i2c_client *client = NULL;

	LCD_KIT_INFO("esd_happened %d t1000_waveform_status %d t1000_upgrade_status %d!\n",
		esd_happened, t1000_waveform_status, t1000_upgrade_status);
	if (t1000_waveform_status)
		return;
	if (!t1000dev) {
		LCD_KIT_ERR("t1000dev is null\n");
		return;
	}
	client = to_i2c_client(t1000dev);
	if (!client) {
		LCD_KIT_ERR("client is null\n");
		return;
	}
	data = i2c_get_clientdata(client);
	if (!data) {
		LCD_KIT_ERR("data is null\n");
		return;
	}
	mutex_lock(&data->ioctl_mutex);
	t1000_fastboot_gpio_set(0);
	struct t1000_command cmd_set_mode = {
		.reg = T1000_REG_CMD,
		.command_code = 0x73,
		.args = {
			.fboot_direction = 0,
		},
	};
	t1000_upgrade_status = 0;
	ret = t1000_send(client, (u8 *)&cmd_set_mode, sizeof(cmd_set_mode));
	if (ret < 0) {
		t1000_waveform_status = 0;
		esd_happened = 0;
		LCD_KIT_ERR("t1000_send error ret =  %d\n", ret);
		LCD_KIT_INFO("esd_happened %d t1000_waveform_status %d t1000_upgrade_status %d!\n",
			esd_happened, t1000_waveform_status, t1000_upgrade_status);
		mutex_unlock(&data->ioctl_mutex);
		return;
	}
	mutex_unlock(&data->ioctl_mutex);
	lcd_kit_delay(5000, LCD_KIT_WAIT_MS, true);
	esd_happened = 0;
	t1000_waveform_status = 1;
	LCD_KIT_INFO("esd_happened %d t1000_waveform_status %d t1000_upgrade_status %d!\n",
		esd_happened, t1000_waveform_status, t1000_upgrade_status);
	return;
}

static void t1000_change_fastboot_gpio_status(struct work_struct *work)
{
	t1000_change_fastboot_status();
}

static irqreturn_t t1000_ready_irq_handler(int irq, void *_data)
{
	struct t1000_data *data = _data;
	data->disp_ready = gpio_get_value(data->gpio_ready);
	LCD_KIT_INFO("data->disp_ready %d\n", data->disp_ready);
	if ((ready_pull_up_times < T1000_READY_PIN_TIMES) && data->disp_ready) {
		ready_pull_up_times++;
		LCD_KIT_INFO("ready_pull_up_times %d\n", ready_pull_up_times);
	} else if ((ready_pull_up_times == 0) && (data->disp_ready == 0)) {
		ready_pull_up_times++;
		LCD_KIT_INFO("ready_pull_up_times %d\n", ready_pull_up_times);
	}
	return IRQ_HANDLED;
}

static void t1000_parse_dt(struct t1000_data *data)
{
	struct device_node *node = data->dev->of_node;
	int gpio_ready_irq;
	int ret;

	data->gpio_ready = of_get_named_gpio(node, "gpio_ready", 0);
	ready_pull_up_times = T1000_READY_PIN_TIMES;

	if (!gpio_is_valid(data->gpio_ready)) {
		LCD_KIT_INFO("gpio_ready not found\n");
	} else {
		LCD_KIT_INFO("config gpio_ready pin #%d\n", data->gpio_ready);
		ret = devm_gpio_request_one(data->dev, data->gpio_ready,
			GPIOF_IN, "t1000-gpio_ready");
		LCD_KIT_INFO("t1000-gpio_ready request ret = %d\n", ret);

		data->disp_ready = gpio_get_value(data->gpio_ready);

		LCD_KIT_INFO("data->disp_ready %d\n", data->disp_ready);

		gpio_ready_irq = gpio_to_irq(data->gpio_ready);
		if (gpio_ready_irq < 0) {
			LCD_KIT_INFO("get gpio_ready irq failed\n");
			return;
		}

		if (devm_request_threaded_irq(data->dev, gpio_ready_irq,
			NULL, t1000_ready_irq_handler,
			IRQF_ONESHOT | IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
			"t1000-gpio_ready", data))
				LCD_KIT_INFO("request ready irq failed\n");
		else
				LCD_KIT_INFO("config gpio_ready irq ok\n");
	}

	data->gpio_wakeup = of_get_named_gpio(node, "gpio_wakeup", 0);
	if (!gpio_is_valid(data->gpio_wakeup)) {
		LCD_KIT_INFO("gpio_wakeup not found\n");
	} else {
		LCD_KIT_INFO("config gpio_wakeup pin #%d\n", data->gpio_wakeup);
		ret = devm_gpio_request_one(data->dev, data->gpio_wakeup,
			GPIOF_OUT_INIT_LOW, "t1000-wakeup");
		LCD_KIT_INFO("t1000-wakeup request ret = %d\n", ret);
	}

	data->gpio_wake_source = of_get_named_gpio(node, "gpio_wake_source", 0);
	if (!gpio_is_valid(data->gpio_wake_source)) {
		LCD_KIT_INFO("gpio_wake_source not found\n");
	} else {
		LCD_KIT_INFO("config gpio_wake_source pin #%d\n", data->gpio_wake_source);
		ret = devm_gpio_request_one(data->dev, data->gpio_wake_source,
			GPIOF_OUT_INIT_LOW, "t1000-wake_source");
		LCD_KIT_INFO("t1000-wake_source request ret = %d\n", ret);
	}

	data->gpio_reset = of_get_named_gpio(node, "gpio_reset", 0);
	if (!gpio_is_valid(data->gpio_reset)) {
		LCD_KIT_INFO("gpio_reset not found\n");
	} else {
		LCD_KIT_INFO("set gpio_reset %d\n", data->gpio_reset);
		ret = devm_gpio_request_one(data->dev, data->gpio_reset,
			GPIOF_OUT_INIT_HIGH, "t1000-reset");
		LCD_KIT_INFO("t1000-reset request ret = %d\n", ret);
	}

	data->gpio_3v3_en = of_get_named_gpio(node, "gpio_3v3_en", 0);
	if (!gpio_is_valid(data->gpio_3v3_en)) {
		LCD_KIT_INFO("gpio_3v3_en not found\n");
	} else {
		LCD_KIT_INFO("set gpio_3v3_en %d\n", data->gpio_3v3_en);
		ret = devm_gpio_request_one(data->dev, data->gpio_3v3_en,
			GPIOF_OUT_INIT_HIGH, "t1000-gpio_3v3_en");
		LCD_KIT_INFO("t1000-gpio_3v3_en request ret = %d\n", ret);
	}

	data->gpio_fastboot = of_get_named_gpio(node, "gpio_fastboot", 0);
	if (!gpio_is_valid(data->gpio_fastboot)) {
		LCD_KIT_INFO("gpio_fastboot not found\n");
	} else {
		LCD_KIT_INFO("set gpio_fastboot %d\n", data->gpio_fastboot);
		ret = devm_gpio_request_one(data->dev, data->gpio_fastboot,
				GPIOF_DIR_IN, "t1000-gpio_fastboot");
		LCD_KIT_INFO("t1000-gpio_fastboot request ret = %d\n", ret);
		schedule_delayed_work(&t1000_esd_delay_work, msecs_to_jiffies(10));
	}

	gpio_set_value(data->gpio_fastboot, 1);

	data->gpio_test_cfg = of_get_named_gpio(node, "gpio_test_cfg1", 0);
	if (!gpio_is_valid(data->gpio_test_cfg)) {
		LCD_KIT_INFO("gpio_test_cfg1 not found\n");
	} else {
		LCD_KIT_INFO("set gpio_test_cfg1 %d\n", data->gpio_test_cfg);
		ret = devm_gpio_request_one(data->dev, data->gpio_test_cfg,
			GPIOF_OUT_INIT_LOW, "t1000-gpio_test_cfg1");
		LCD_KIT_INFO("t1000-gpio_test_cfg1 request ret = %d\n", ret);
	}
}

int t1000_get_spi_hrdy_status(void);

static int g_predispready = 0;
static int t1000_ioctl_get_display_ready(void *arg)
{
	int value;
	struct t1000_data *data = dev_get_drvdata(t1000dev);

	value = data->disp_ready;

	if (copy_to_user((void *)arg, &value, sizeof(value)) != 0) {
		LCD_KIT_INFO("copy_to_user fail");
		return -1;
	}
	g_predispready = value;

	return 0;
}

static int t1000_ioctl_get_hw_spi_ready(void *arg)
{
	struct t1000_data *data = dev_get_drvdata(t1000dev);

	if (copy_to_user((void *)arg, &data->hw_spi_ready, sizeof(data->hw_spi_ready)) != 0) {
		LCD_KIT_INFO("copy_to_user fail");
		return -1;
	}

	return 0;
}

static int t1000_ioctl_set_power_onoff(void *arg)
{
	int status = 0;
	int ipoweronff = 0;
	struct t1000_data *data = dev_get_drvdata(t1000dev);

	if (copy_from_user(&ipoweronff, arg, sizeof(ipoweronff)) != 0) {
		LCD_KIT_INFO("copy_from_user fail");
		return -1;
	}
	data->power_state = ipoweronff ? POWERON : POWEROFF;
	LCD_KIT_INFO("copy_from_user->poweronff [%d]", ipoweronff);

	if (data->power_state == POWERON)
		gpio_set_value(data->gpio_fastboot, 0);

	gpio_set_value(data->gpio_3v3_en, ipoweronff);

	if (data->power_state == POWEROFF)
		gpio_set_value(data->gpio_fastboot, 1);

	return status;
}

static int t1000_ioctl_set_sleep_wakeup(void *arg)
{
	int status = 0;
	int sleepwakeup = 0;

	struct t1000_data *data = dev_get_drvdata(t1000dev);

	if (copy_from_user(&sleepwakeup, arg, sizeof(sleepwakeup)) != 0) {
		LCD_KIT_INFO("copy_from_user fail");
		return -1;
	}

	LCD_KIT_INFO("copy_from_user->sleepwakeup [%d]", sleepwakeup);

	if (sleepwakeup == 1) {
		gpio_set_value(data->gpio_wakeup, 1);
		msleep(GPIO_WAKEUP_DELAY);

		if (t1000_enter_sleep_mode(t1000dev)) {
			LCD_KIT_INFO("t1000_enter_sleep_mode failed\n");
			return -1;
		}
		data->power_state = SLEEP;
	} else {
		gpio_set_value(data->gpio_wakeup, 0);
		data->power_state = ACTIVE;
	}

	return status;
}

static int t1000_ioctl_get_power_state(void *arg)
{
	int value;
	struct t1000_data *data = dev_get_drvdata(t1000dev);

	value = data->power_state;

	if (copy_to_user((void *)arg, &value, sizeof(value)) != 0) {
		LCD_KIT_INFO("copy_to_user fail\n");
		return -1;
	}

	return 0;
}

static int t1000_ioctl_set_wakeupsource(void *arg)
{
	int status = 0;
	int iwakeupsource = 0;

	struct t1000_data *data = dev_get_drvdata(t1000dev);

	if (copy_from_user(&iwakeupsource, arg, sizeof(iwakeupsource)) != 0) {
		LCD_KIT_INFO("copy_from_user fail\n");
		return -1;
	}

	gpio_set_value(data->gpio_wake_source, iwakeupsource);

	LCD_KIT_INFO("copy_from_user->iWakeupsource [%d]\n", iwakeupsource);

	return status;
}

static void t1000_get_first_mode(struct i2c_msg rdwr_pa)
{
	struct t1000_command *mode_command = NULL;

	if (rdwr_pa.len != sizeof(struct t1000_command)) {
		LCD_KIT_DEBUG("len %d %d\n", rdwr_pa.len, sizeof(struct t1000_command));
		return;
	}
	mode_command = (struct t1000_command*)(rdwr_pa.buf);
	if (mode_command) {
		if (mode_command->command_code != UCMD_SET_MIPI_MODE) {
			LCD_KIT_INFO("invalid mode cmd %d \n", mode_command->command_code);
			return;
		}
		first_mode = mode_command->args.mipi_mode.mode;
		first_sub_mode =  mode_command->args.mipi_mode.sub_mode;
		LCD_KIT_INFO("mode %d sub_mode %d\n", first_mode, first_sub_mode);
	}
}

static noinline int t1000_ioctl_rdwr(void *arg)
{
	struct i2c_rdwr_ioctl_data rdwr_arg;
	struct i2c_msg *rdwr_pa = NULL;
	u8 __user **data_ptrs;
	int i, j, res = 0;
	struct t1000_data *data = dev_get_drvdata(t1000dev);
	struct i2c_client *client = data->client;

	if (ready_pull_up_times < T1000_READY_PIN_TIMES) {
		LCD_KIT_INFO("t1000 is poweroff,not ready return. ready_times %d\n", ready_pull_up_times);
		return -T1000_OFF_RESPOND_NUM;
	}
	if (copy_from_user(&rdwr_arg, (struct i2c_rdwr_ioctl_data __user *)arg, sizeof(rdwr_arg)))
		return -EFAULT;
	if (rdwr_arg.nmsgs > I2C_RDWR_IOCTL_MAX_MSGS)
		return -EINVAL;
	rdwr_pa = memdup_user(rdwr_arg.msgs, rdwr_arg.nmsgs * sizeof(struct i2c_msg));
	if (IS_ERR(rdwr_pa)) {
		return PTR_ERR(rdwr_pa);
	}
	data_ptrs = kmalloc(rdwr_arg.nmsgs * sizeof(u8 __user *), GFP_KERNEL);
	if (data_ptrs == NULL) {
		kfree(rdwr_pa);
		return -ENOMEM;
	}

	for (i = 0; i < rdwr_arg.nmsgs; i++) {
		if (rdwr_pa[i].len > 8192) {
			res = -EINVAL;
			break;
		}
		data_ptrs[i] = (u8 __user *)rdwr_pa[i].buf;
		rdwr_pa[i].buf = memdup_user(data_ptrs[i], rdwr_pa[i].len);
		if (IS_ERR(rdwr_pa[i].buf)) {
			res = PTR_ERR(rdwr_pa[i].buf);
			break;
		}
		if (rdwr_pa[i].flags & I2C_M_RECV_LEN) {
			if (!(rdwr_pa[i].flags & I2C_M_RD) || rdwr_pa[i].buf[0] < 1 ||
				rdwr_pa[i].len < rdwr_pa[i].buf[0] + I2C_SMBUS_BLOCK_MAX) {
				i++;
				res = -EINVAL;
				break;
			}
			rdwr_pa[i].len = rdwr_pa[i].buf[0];
		}
		t1000_get_first_mode(rdwr_pa[i]);
	}

	if (ready_pull_up_times < T1000_READY_PIN_TIMES) {
		LCD_KIT_INFO("t1000 is poweroff,not ready return. ready_times %d\n", ready_pull_up_times);
		res = -T1000_OFF_RESPOND_NUM;
	}

	if (res < 0) {
		for (j = 0; j < i; ++j) {
			kfree(rdwr_pa[j].buf);
		}
		kfree(data_ptrs);
		kfree(rdwr_pa);
		return res;
	}

	res = i2c_transfer(client->adapter, rdwr_pa, rdwr_arg.nmsgs);
	if (res < 0)
		LCD_KIT_ERR("i2c_transfer err res %d\n", res);
	while (i-- > 0) {
		if (res >= 0 && (rdwr_pa[i].flags & I2C_M_RD)) {
			if (copy_to_user(data_ptrs[i], rdwr_pa[i].buf, rdwr_pa[i].len))
				res = -EFAULT;
		}
		kfree(rdwr_pa[i].buf);
	}
	kfree(data_ptrs);
	kfree(rdwr_pa);
	return res;
}

static void t1000_fast_power_on_finished(struct work_struct *work)
{
	int ret;
	int i = 0;

	struct t1000_data *data = NULL;
	data = get_t1000_data();
	if (!data) {
		LCD_KIT_ERR("data is null\n");
		return;
	}
	LCD_KIT_INFO("ready_pull_up_times %d\n", ready_pull_up_times);
	if (ready_pull_up_times == T1000_READY_PIN_TIMES)
		return;

	LCD_KIT_INFO("mode %d submode %d\n", first_mode, first_sub_mode);
	while (!data->disp_ready && (i < WAITING_READY_COUNT)) {
		msleep(GPIO_READY_DELAY);
		i++;
	}
	if (i == WAITING_READY_COUNT)
		LCD_KIT_INFO("ready timeout, skip\n");
	lcd_kit_delay(10, LCD_KIT_WAIT_MS, true);
	if (ready_pull_up_times == T1000_READY_PIN_TIMES) {
		LCD_KIT_INFO("t1000 ready ready_pull_up_times %d\n", ready_pull_up_times);
		return;
	}
	mutex_lock(&data->ioctl_mutex);
	ret = t1000_set_mode(t1000dev, first_mode, first_sub_mode);
	mutex_unlock(&data->ioctl_mutex);
	if (ret)
		LCD_KIT_ERR("t1000_set_mode failed\n");

	ready_pull_up_times = T1000_READY_PIN_TIMES;

	LCD_KIT_INFO("finished!\n");

	return;
}

static int t1000_ioctl_gpio_ctrl(const void *arg)
{
	int status = 0;
	struct t1000_gpio_config  gpio_config;
	struct t1000_data *data = NULL;

	if (esd_happened) {
		LCD_KIT_INFO("esd_happened %d\n", esd_happened);
		return status;
	}
	data = get_t1000_data();
	if (!data) {
		LCD_KIT_ERR("data is null\n");
		return 0;
	}

	if (copy_from_user(&gpio_config, arg, sizeof(gpio_config)) != 0) {
		LCD_KIT_INFO("copy_from_user fail\n");
		return -1;
	}

	gpio_set_value(gpio_config.gpio, gpio_config.value);

	if (gpio_config.gpio == data->gpio_reset) {
		if (gpio_config.value)
			schedule_delayed_work(&t1000_esd_delay_work, msecs_to_jiffies(3000));
		else
			t1000_waveform_status = 0;
	}

	LCD_KIT_INFO("gpio value [%d][%d]\n", gpio_config.gpio, gpio_config.value);

	return status;
}

static int t1000_ioctrl_update_pen_status(const void *arg)
{
	int status = LCD_KIT_OK;
	bool pen_status = false;
	struct lcd_kit_panel_ops *panel_ops = NULL;

	panel_ops = lcd_kit_panel_get_ops();
	if (!panel_ops || !panel_ops->eink_resume)
		return LCD_KIT_FAIL;
	if (copy_from_user(&pen_status, arg, sizeof(pen_status)) != 0) {
		LCD_KIT_INFO("copy_from_user fail\n");
		return LCD_KIT_FAIL;
	}

	LCD_KIT_INFO("copy_from_user->pen_status [%d]", pen_status);

	if (pen_status) {
		is_pen_suspend = false;
		if (is_suspend)
			panel_ops->eink_resume();
	} else {
		is_pen_suspend = true;
	}
	return status;
}

static int t1000_get_ready_times(void *arg)
{
	if (copy_to_user((void *)arg, &ready_pull_up_times, sizeof(ready_pull_up_times)) != 0) {
		LCD_KIT_INFO("copy_to_user fail\n");
		return -1;
	}
	return 0;
}


static int t1000_ioctrl_set_upgrade_status(void *arg)
{
	int status = 0;

	if (copy_from_user(&status, arg, sizeof(status)) != 0) {
		LCD_KIT_INFO("copy_from_user fail\n");
		return -1;
	}

	LCD_KIT_INFO("status from %d to %d\n", t1000_upgrade_status, status);

	if (status == 0 && t1000_upgrade_status == 1) {
		schedule_delayed_work(&t1000_esd_delay_work, msecs_to_jiffies(3000));
	} else {
		t1000_upgrade_status = status;
		t1000_waveform_status = 0;
	}
	return status;
}

static long t1000_file_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int ret;
	struct t1000_data *data = dev_get_drvdata(t1000dev);

	mutex_lock(&data->ioctl_mutex);
	switch (cmd) {
	case T1000_WAIT_DISPLAY_READY:
		ret = t1000_ioctl_get_display_ready((void *)arg);
		break;
	case T1000_WAIT_HW_SPI_READY:
		ret = t1000_ioctl_get_hw_spi_ready((void *)arg);
		break;
	case T1000_SET_POWER_ONOFF:
		ret = t1000_ioctl_set_power_onoff((void *)arg);
		break;
	case T1000_SET_SLEEP_WAKEUP:
		ret = t1000_ioctl_set_sleep_wakeup((void *)arg);
		break;
	case T1000_GET_POWER_STATE:
		ret = t1000_ioctl_get_power_state((void *)arg);
		break;
	case T1000_SET_WAKEUP_SOURCE:
		ret = t1000_ioctl_set_wakeupsource((void *)arg);
		break;
	case T1000_I2C_RDWR:
		ret = t1000_ioctl_rdwr((void *)arg);
		break;
	case T1000_GPIO_CTRL:
		ret = t1000_ioctl_gpio_ctrl((void *)arg);
		break;
	case T1000_SEND_PEN_STATUS:
		ret = t1000_ioctrl_update_pen_status((void *)arg);
		break;
	case T1000_READ_POWERON_TIMING:
		ret = t1000_get_ready_times((void *)arg);
		break;
	case T1000_SET_UPGRADE_STATUS:
		ret = t1000_ioctrl_set_upgrade_status((void *)arg);
		break;
	default:
		ret = -1;
		break;
	}
	mutex_unlock(&data->ioctl_mutex);

	return ret;
}

static int t1000_file_open(struct inode *inode, struct file *file)
{
	LCD_KIT_INFO("t1000_file_open\n");
	return 0;
}

static int t1000_file_release(struct inode *ignored, struct file *file)
{
	LCD_KIT_INFO("t1000_file_release\n");
	return 0;
}

static const struct file_operations g_t1000_fops = {
	.owner =	THIS_MODULE,
	.unlocked_ioctl = t1000_file_ioctl,
	.compat_ioctl	 = t1000_file_ioctl,
	.open =		t1000_file_open,
	.release =	t1000_file_release,
};

static ssize_t t1000_eink_info_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct t1000_data *data = NULL;
	data = dev_get_drvdata(t1000dev);
	if (!data)
		return LCD_KIT_FAIL;

	return snprintf(buf, T1000_EINK_INFO_LEN, "%s\n", data->eink_info);
}

static ssize_t t1000_eink_info_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	ssize_t ret;
	struct t1000_data *data = NULL;

	data = dev_get_drvdata(t1000dev);
	if (!data)
		return count;

	if (count > T1000_EINK_INFO_LEN) {
		LCD_KIT_ERR("err lenth\n");
		return count;
	}
	ret = memcpy_s(data->eink_info, count, buf, count);
	if (ret != 0) {
		LCD_KIT_ERR("memcpy_s error\n");
		return count;
	}
	LCD_KIT_INFO("eink_info: %s\n", data->eink_info);
	return count;
}

static DEVICE_ATTR(eink_info, S_IRUGO | S_IWUSR, t1000_eink_info_show, t1000_eink_info_store);

static struct attribute *t1000_attributes[] = {
	&dev_attr_eink_info.attr,
	NULL
};
static const struct attribute_group g_t1000_attr_group = {
	.attrs = t1000_attributes,
};

void t1000_spi_async_time(void)
{
	struct lcd_kit_panel_ops *panel_ops = NULL;

	panel_ops = lcd_kit_panel_get_ops();
	if (!panel_ops || !panel_ops->eink_resume)
		return;
	spi_write_time = jiffies;
	if (is_suspend) {
		panel_ops->eink_resume();
		LCD_KIT_INFO("t1000 is_suspend=%d\n", is_suspend);
	}
	return;
}

static int t1000_judgement_suspend(void)
{
	unsigned long timeout = spi_write_time + WAIT_T1000_SUSPEND * HZ;

	if (!is_pen_suspend || time_before(jiffies, timeout) || is_recovery_mode || power_cmdline_is_powerdown_charging_mode())
		return T1000_NO_SUSPEND_RET;
	else
		return LCD_KIT_OK;
}

static int t1000_suspend_entity(void)
{
	int i = 0;
	struct t1000_data *data = NULL;
	struct i2c_client *client = NULL;
	struct hisi_fb_data_type *hisifd = hisifd_list[PRIMARY_PANEL_IDX];

	if (!t1000dev) {
		LCD_KIT_ERR("t1000dev is null\n");
		return LCD_KIT_FAIL;
	}
	client = to_i2c_client(t1000dev);
	if (!client) {
		LCD_KIT_ERR("client is null\n");
		return LCD_KIT_FAIL;
	}
	data = i2c_get_clientdata(client);
	if (!data) {
		LCD_KIT_ERR("data is null\n");
		return LCD_KIT_FAIL;
	}
	if (data->power_state == POWEROFF) {
		LCD_KIT_ERR("t1000 is power off, return\n");
		return LCD_KIT_FAIL;
	}
	t1000_esd_enable(hisifd, 0);
	gpio_set_value(data->gpio_wakeup, 1);
	gpio_set_value(data->gpio_wake_source, 0);
	if (t1000_enter_sleep_mode(data->dev))
		LCD_KIT_ERR("t1000 enter sleep mode failed\n");
	while (data->disp_ready && (i < WAITING_READY_COUNT)) {
		msleep(GPIO_READY_DELAY);
		i++;
	}
	if (i == WAITING_READY_COUNT)
		LCD_KIT_INFO("ready timeout, skip\n");
	data->power_state = SLEEP;
	return LCD_KIT_OK;
}

static void t1000_suspend(struct work_struct *work)
{
	struct dpufb_vsync *vsync_ctrl = NULL;
	struct dpu_fb_data_type *dpufd = dpufd_list[PRIMARY_PANEL_IDX];

	if (!dpufd) {
		LCD_KIT_ERR("dpufd is null\n");
		return;
	}
	vsync_ctrl = &(dpufd->vsync_ctrl);
	if (!vsync_ctrl) {
		LCD_KIT_ERR("vsync_ctrl is null\n");
		return;
	}
	mutex_lock(&(vsync_ctrl->vsync_lock));
	if ((vsync_ctrl->vsync_mipi_expire_count != 0) || (vsync_ctrl->vsync_mipi_enabled == 0)) {
		LCD_KIT_ERR("vsync_mipi_expire_count  vsync_mipi_enabled Non-prefetch\n");
		goto OUT;
	}
	if (is_suspend) {
		LCD_KIT_ERR("is_suspend true\n");
		goto OUT;
	}
	if (t1000_suspend_entity())
		goto OUT;
	disable_ldi(dpufd);
	LCD_KIT_INFO("suspend successed\n");
	is_suspend = true;
	t1000_low_power_status = 1;
	vsync_ctrl->vsync_mipi_enabled = 0;

OUT:
	mutex_unlock(&(vsync_ctrl->vsync_lock));
	return;
}

static void t1000_suspend_async(void)
{
	schedule_work(&eink_suspend_work);
}

static void t1000_change_low_power_status(struct work_struct *work)
{
	t1000_low_power_status = 0;
}

static void t1000_resume(void)
{
	struct t1000_data *data = NULL;
	struct i2c_client *client = NULL;
	struct hisi_fb_data_type *hisifd = hisifd_list[PRIMARY_PANEL_IDX];

	if (!is_suspend)
		return;
	if (!t1000dev) {
		LCD_KIT_ERR("t1000dev is null\n");
		return;
	}
	client = to_i2c_client(t1000dev);
	if (!client) {
		LCD_KIT_ERR("client is null\n");
		return;
	}
	data = i2c_get_clientdata(client);
	if (!data) {
		LCD_KIT_ERR("data is null\n");
		return;
	}
	gpio_set_value(data->gpio_wakeup, 0);
	is_suspend = false;
	data->power_state = ACTIVE;
	LCD_KIT_INFO("resume successed\n");
	t1000_esd_enable(hisifd, 1);
	schedule_delayed_work(&t1000_lp_delay_work, msecs_to_jiffies(1000));
	return;
}

static int t1000_esd_handle(struct hisi_fb_data_type *hisifd)
{
	int i = 0;
	int times = 0;
	struct t1000_data *data = NULL;
	char tmp[T1000_ESD_MAX_DET_TIME + 1] = { 0 };

	LCD_KIT_INFO("esd_happened %d t1000_waveform_status %d t1000_upgrade_status %d!\n",
		esd_happened, t1000_waveform_status, t1000_upgrade_status);
	if (t1000_waveform_status == 0) {
		return 0;
	}
	if (esd_happened) {
		t1000_power_off();
		t1000_power_on();
		return 0;
	}

	if (t1000_low_power_status || t1000_upgrade_status)
		return 0;
	data = get_t1000_data();
	if (!data) {
		LCD_KIT_ERR("data is null\n");
		return 0;
	}
	for (i = 0; i < T1000_ESD_MAX_DET_TIME; i++) {
		if (t1000_get_esd_ckeck_status(times, hisifd))
			return 0;
		tmp[i] = gpio_get_value(data->gpio_fastboot);
		LCD_KIT_INFO("value is %d\n", tmp[i]);
		if (tmp[i] != tmp[0])
			break;
		times++;
		mdelay(20);
	}
	LCD_KIT_INFO("times %d t1000_low_power_status %d esd_happened %d t1000_upgrade_status %d t1000_waveform_status %d\n",
		times, t1000_low_power_status, esd_happened, t1000_upgrade_status, t1000_waveform_status);
	if (t1000_get_esd_ckeck_status(times, hisifd)) {
		LCD_KIT_ERR("esd err\n");
		esd_happened = 1;
#if defined CONFIG_HUAWEI_DSM
		dsm_client_record(lcd_dclient, "t1000 esd check err. tiems = %d", times);
		dsm_client_notify(lcd_dclient, DSM_LCD_ESD_STATUS_ERROR_NO);
#endif
		return -1;
	}
	return 0;
}

static void t1000_fastboot_gpio_set(int enable)
{
	int ret;
	struct t1000_data *data = NULL;

	data = get_t1000_data();
	if (!data) {
		LCD_KIT_ERR("data is null\n");
		return;
	}
	gpio_free(data->gpio_fastboot);
	LCD_KIT_INFO("set gpio_fastboot %d\n", data->gpio_fastboot);
	if (enable) {
		ret = devm_gpio_request_one(data->dev, data->gpio_fastboot,
			GPIOF_OUT_INIT_HIGH, "t1000-gpio_fastboot");
		LCD_KIT_INFO("t1000-gpio_fastboot out request err ret = %d\n", ret);
	} else {
		ret = devm_gpio_request_one(data->dev, data->gpio_fastboot,
			GPIOF_DIR_IN, "t1000-gpio_fastboot");
		LCD_KIT_INFO("t1000-gpio_fastboot in request err ret = %d\n", ret);
	}
}

static int t1000_power_off(void)
{
	int i = 0;
	struct t1000_data *data = NULL;
	struct i2c_client *client = NULL;
	if (!t1000dev) {
		LCD_KIT_ERR("t1000dev is null\n");
		return 0;
	}
	client = to_i2c_client(t1000dev);
	if (!client) {
		LCD_KIT_ERR("client is null\n");
		return 0;
	}
	data = i2c_get_clientdata(client);
	if (!data) {
		LCD_KIT_ERR("data is null\n");
		return 0;
	}
	LCD_KIT_INFO("\n");
	gpio_set_value(data->gpio_wakeup, 1);
	if (t1000_enter_sleep_mode(data->dev))
		LCD_KIT_ERR("set MIPI_SLEEP failed\n");

	while (data->disp_ready && (i < WAITING_READY_COUNT)) {
		if (esd_happened)
			break;
		msleep(GPIO_READY_DELAY);
		i++;
	}
	if (i == WAITING_READY_COUNT)
		LCD_KIT_INFO("ready timeout, skip\n");
	lcd_kit_delay(ENTER_SLEEP_MODE_DELAY, LCD_KIT_WAIT_MS, true);
	/* 2nd: poweroff */
	gpio_set_value(data->gpio_3v3_en, 0);
	lcd_kit_delay(REG_ENABLE_FASTBOOT_DELAY, LCD_KIT_WAIT_MS, true);
	gpio_set_value(data->gpio_reset, 0);

	/* fanzhiloudian  */
	gpio_set_value(data->gpio_fastboot, 0);
	gpio_set_value(data->gpio_wakeup, 0);
	gpio_set_value(data->gpio_test_cfg, 0);
	gpio_set_value(data->gpio_wake_source, 0);
	data->power_state = POWEROFF;
	ready_pull_up_times = 0;
	LCD_KIT_INFO("power off success\n");
	t1000_waveform_status = 0;
	return 0;
}

static int t1000_power_on(void)
{
	struct t1000_data *data = NULL;
	struct i2c_client *client = NULL;
	if (!t1000dev) {
		LCD_KIT_ERR("t1000dev is null\n");
		return 0;
	}
	client = to_i2c_client(t1000dev);
	if (!client) {
		LCD_KIT_ERR("client is null\n");
		return 0;
	}
	data = i2c_get_clientdata(client);
	if (!data) {
		LCD_KIT_ERR("data is null\n");
		return 0;
	}
	ready_pull_up_times = 0;
	LCD_KIT_INFO("esd_happened %d!\n", esd_happened);
	t1000_fastboot_gpio_set(1);
	if (esd_happened)
		gpio_set_value(data->gpio_fastboot, 1);
	else
		gpio_set_value(data->gpio_fastboot, 0);
	lcd_kit_delay(GPIO_POWER_TIME_DELAY, LCD_KIT_WAIT_MS, true);
	gpio_set_value(data->gpio_3v3_en, 1);
	lcd_kit_delay(GPIO_POWER_TIME_DELAY, LCD_KIT_WAIT_MS, true);
	gpio_set_value(data->gpio_test_cfg, 1);
	lcd_kit_delay(GPIO_POWER_TIME_DELAY, LCD_KIT_WAIT_MS, true);
	gpio_set_value(data->gpio_test_cfg, 0);
	lcd_kit_delay(GPIO_POWER_TIME_DELAY, LCD_KIT_WAIT_MS, true);
	gpio_set_value(data->gpio_reset, 1);
	LCD_KIT_INFO("power on success\n");
	data->power_state = POWERON;
	schedule_delayed_work(&t1000_esd_delay_work, msecs_to_jiffies(3000));
	schedule_delayed_work(&t1000_ref_mode_delay_work, msecs_to_jiffies(POWER_ON_WORK_DELAY));
	return 0;
}

static struct lcd_kit_panel_ops g_t1000EinkOps = {
	.eink_power_on = t1000_power_on,
	.eink_power_off = t1000_power_off,
	.eink_suspend = t1000_suspend_async,
	.eink_resume = t1000_resume,
	.eink_spi_async_time = t1000_spi_async_time,
	.lcd_esd_check = t1000_esd_handle,
	.eink_judgement_suspend = t1000_judgement_suspend,
};

static int t1000_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct t1000_data *data = NULL;
	int ret;
	int error;

	LCD_KIT_INFO("\n");
	data = devm_kzalloc(&client->dev, sizeof(*data), GFP_KERNEL);
	if (!data) {
		LCD_KIT_INFO("Couldn't allocate memory\n");
		return -ENOMEM;
	}

	data->client = client;
	data->dev = &client->dev;
	data->bpower_alwayson = true;

	t1000dev = &client->dev;

	i2c_set_clientdata(client, data);
	mutex_init(&data->ioctl_mutex);
	mutex_init(&data->pm_mutex);

	INIT_DELAYED_WORK(&t1000_esd_delay_work, t1000_change_fastboot_gpio_status);
	t1000_parse_dt(data);

	sysfs_create_group(&client->dev.kobj, &g_t1000_attr_group);

	atomic_set(&data->pm_cnt, 1);
	data->rdev_enabled = 1;
	data->autopm_enable = 1;
	data->power_state = POWERON;
	data->mipimode = t1000_get_mode(data->dev);
	if (data->mipimode == -1) {
		LCD_KIT_ERR("t1000_probe i2c error ret\n");
#if defined CONFIG_HUAWEI_DSM
		dsm_client_record(lcd_dclient, "t1000_recv err ERRNO %d", ret);
		dsm_client_notify(lcd_dclient, DSM_LCD_BIAS_I2C_ERROR_NO);
#endif
		return -1;
	}

	error = register_chrdev(T1000CDEV_MAJOR, t1000_chrname, &g_t1000_fops);
	if (error < 0) {
		LCD_KIT_INFO("t1000_probe register_chrdev: unable to get major number %d\n", 0);
		return 0;
	}

	t1000_cdev.major = T1000CDEV_MAJOR;

	LCD_KIT_INFO("t1000_cdev.major %d\n", t1000_cdev.major);

	t1000_cdev.cls = class_create(THIS_MODULE, t1000_chrname);
	if (IS_ERR(t1000_cdev.cls)) {
		LCD_KIT_INFO("t1000cdev class_create error!\n");
		ret = PTR_ERR(t1000_cdev.cls);
		goto err_unregister;
	}

	t1000_cdev.dev = device_create(t1000_cdev.cls, NULL, MKDEV(T1000CDEV_MAJOR, 0), NULL, t1000_chrname);
	if (IS_ERR(t1000_cdev.dev)) {
		LCD_KIT_INFO("t1000cdev device_create error!\n");
		ret = PTR_ERR(t1000_cdev.dev);
		goto err_class;
	}

	ret = lcd_kit_panel_ops_register(&g_t1000EinkOps);
	if (ret) {
		LCD_KIT_ERR("register failed\n");
		return LCD_KIT_FAIL;
	}

	INIT_WORK(&eink_suspend_work, t1000_suspend);
	INIT_DELAYED_WORK(&t1000_lp_delay_work, t1000_change_low_power_status);
	INIT_DELAYED_WORK(&t1000_ref_mode_delay_work, t1000_fast_power_on_finished);
#if defined(CONFIG_LCD_KIT_HISI)
	is_recovery_mode = get_boot_into_recovery_flag();
#endif
	LCD_KIT_INFO("success\n");
	return ret;

err_class:
	class_destroy(t1000_cdev.cls);

err_unregister:
	unregister_chrdev(t1000_cdev.major, t1000_chrname);

	return -1;
}

static int t1000_remove(struct i2c_client *client)
{
	LCD_KIT_INFO("\n");
	return 0;
}

static const struct i2c_device_id t1000_id_table[] = {
	{ "T1000", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, t1000_id_table);

static const struct of_device_id t1000_of_match[] = {
	{ .compatible = "E Ink,T1000", },
	{ }
};

static struct i2c_driver t1000_driver = {
	.driver = {
		.name = "T1000",
		.of_match_table = t1000_of_match,
	},
	.probe = t1000_probe,
	.remove = t1000_remove,
	.id_table = t1000_id_table,
};

static int __init t1000_init(void)
{
	LCD_KIT_INFO("\n");
	return i2c_add_driver(&t1000_driver);
}

static void __exit t1000_exit(void)
{
	LCD_KIT_INFO("\n");
	i2c_del_driver(&t1000_driver);
}

/* For display regulator, it needs to be eariler */
late_initcall(t1000_init);
module_exit(t1000_exit);
MODULE_AUTHOR("ireader");
MODULE_DESCRIPTION("T1000 EPD controller");
MODULE_LICENSE("GPL v2");
