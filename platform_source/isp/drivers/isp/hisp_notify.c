/*
 * HiStar Remote Processor driver
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/mod_devicetable.h>
#include <linux/amba/bus.h>
#include <linux/sizes.h>
#include <linux/dma-mapping.h>
#include <linux/remoteproc.h>
#include <linux/delay.h>
#include <linux/kfifo.h>
#include <linux/pm_wakeup.h>
#include <linux/mutex.h>
#include <linux/regulator/consumer.h>
#include <linux/scatterlist.h>
#include <linux/clk.h>
#include <linux/sched/rt.h>
#include <linux/kthread.h>
#include <global_ddr_map.h>
#include <isp_ddr_map.h>
#include <asm/cacheflush.h>
#include <linux/firmware.h>
#include <linux/iommu.h>
#include <linux/mm_iommu.h>
#include <linux/crc32.h>
#include <linux/gpio.h>
#include <ipc_rproc.h>
#include <linux/of_gpio.h>
#include <linux/pinctrl/consumer.h>
#include "hisp_internel.h"

enum gpio_type {
	I2C1_SCL    = 0,
	I2C2_SCL    = 1,
	I2C3_SCL    = 2,
	I2C2_AO_SCL = 3,
	I2C_GPIO_MAX,
};

#define GPIO_HIGH        (1)
#define GPIO_LOW         (0)
#define I2C_CLK_COUNT    (9)

enum hisp_notify_messages {
	NTF_I2C_DEADLOCK        = 0xFFFFFE00U,
	NTF_FAIL                = 0xFFFFFEEEU,
	NTF_SUCC                = 0xFFFFFEEFU,
};

struct hisp_notify {
	struct device *dev;
	unsigned int enable;
	unsigned int a7_ap_mbox;
	unsigned int ap_a7_mbox;
	struct mutex notify_mutex;
	struct notifier_block nb;
	struct rproc *rproc;
	unsigned int gpio_num;
	struct gpio gpios[I2C_GPIO_MAX];
};

static struct hisp_notify hisp_notify_dev;

static int hisp_i2c_deadlock_pwm(unsigned int type)
{
	int ret;
	unsigned int i;
	struct pinctrl *pcl = NULL;
	struct hisp_notify *dev_rep = &hisp_notify_dev;

	if (type >= dev_rep->gpio_num)  {
		pr_err("%s: invalid type.%d\n", __func__, type);
		return -1;
	}

	ret = gpio_request(dev_rep->gpios[type].gpio, NULL);
	if (ret < 0) {
		pr_err("%s :gpio_request failed.%d\n", __func__, ret);
		return ret;
	}

	ret = gpio_direction_output(dev_rep->gpios[type].gpio, GPIO_HIGH);
	if (ret < 0) {
		pr_err("%s :gpio_direction_output failed.%d\n", __func__, ret);
		return ret;
	}

	for (i = 0; i < I2C_CLK_COUNT; i++) {
		gpio_set_value(dev_rep->gpios[type].gpio, GPIO_LOW);
		udelay(1);
		gpio_set_value(dev_rep->gpios[type].gpio, GPIO_HIGH);
		udelay(1);
	}

	gpio_free(dev_rep->gpios[type].gpio);

	return 0;
}

static int hisp_i2c_iomux_scl(unsigned int type)
{
	return hisp_smc_i2c_deadlock(type);
}

static int hisp_i2c_deadlock_escape(unsigned int type)
{
	int ret;
	unsigned int i;
	struct hisp_notify *dev_rep = &hisp_notify_dev;

	ret = hisp_i2c_deadlock_pwm(type);
	if (ret < 0) {
		pr_err("%s: hisp_i2c_deadlock_escape_pwm failed.%d", __func__, ret);
		return ret;
	}

	ret = hisp_i2c_iomux_scl(type);
	if (ret < 0)
		pr_err("%s: hisp_smc_i2c_deadlock failed.%d", __func__, ret);

	return ret;
}

static void hisp_i2c_deadlock_ack(unsigned int ack_type)
{
	int ret;
	static unsigned int ack_array[8] = {0};
	struct hisp_notify *dev_rep = &hisp_notify_dev;

	pr_info("%s: ack.0x%x\n", __func__, ack_type);

	ack_array[0] = ack_type;
	ret = RPROC_ASYNC_SEND(dev_rep->ap_a7_mbox, ack_array,
		sizeof(ack_array) / sizeof(ack_array[0]));
	if (ret < 0)
		pr_err("%s : RPROC_ASYNC_SEND fail.%d\n", __func__, ret);
}

static void hisp_i2c_deadlock_handle(unsigned int type)
{
	int ret;
	unsigned int ack_type = NTF_SUCC;

	ret = hisp_i2c_deadlock_escape(type);
	if(ret < 0) {
		pr_err("%s :hisp_i2c_deadlock_escape failed.%d\n", __func__, ret);
		ack_type = NTF_FAIL;
	}

	hisp_i2c_deadlock_ack(ack_type);
}

static int hisp_notify_callback(struct notifier_block *this,
	unsigned long len, void *data)
{
	unsigned int i;
	mbox_msg_t *msg = (mbox_msg_t *)data;
	mbox_msg_t rx_buf[MBOX_CHAN_DATA_SIZE];
	struct hisp_notify *dev = container_of(this, struct hisp_notify, nb);

	if ((msg == NULL) || (len < MBOX_CHAN_DATA_SIZE)) {
		pr_err("%s: len.%d\n", __func__, len);
		return -EINVAL;
	}

	mutex_lock(&dev->notify_mutex);
	for (i = 0; i < MBOX_CHAN_DATA_SIZE; i++) {
		rx_buf[i] = msg[i];
		pr_info("%s: index.%d, rx buf.0x%x\n", __func__, i, rx_buf[i]);
	}

	switch (rx_buf[0]) {
	case NTF_I2C_DEADLOCK:/*lint !e650 */
		hisp_i2c_deadlock_handle(rx_buf[1]);
		break;

	default:
		pr_err("%s: invalid type.0x%x\n", __func__, rx_buf[0]);
		break;
	}
	mutex_unlock(&dev->notify_mutex);

	return NOTIFY_DONE;
}

static int hisp_notify_getdts(struct hisp_notify *dev_ntf)
{
	int ret;
	unsigned int i;
	struct device *dev = dev_ntf->dev;
	struct device_node *np = dev->of_node;

	pr_info("[%s] +\n", __func__);
	if (np == NULL) {
		pr_err("[%s] Failed : np.NULL\n", __func__);
		return -ENODEV;
	}

	ret = of_property_read_u32(np, "use-notify-flag", &dev_ntf->enable);
	if ((ret < 0) || (dev_ntf->enable != 1)) {
		pr_err("[%s] Failed: ret.0x%x\n", __func__, ret);
		dev_ntf->enable = 0;
		return -1;
	}

	dev_ntf->gpio_num = of_gpio_count(np);
	if (dev_ntf->gpio_num < 0) {
		pr_err("%s failed: gpio_num.%d\n", __func__, dev_ntf->gpio_num);
		return -1;
	}

	for (i = 0; i < dev_ntf->gpio_num; i++)
		dev_ntf->gpios[i].gpio = of_get_gpio(np, i);

	pr_info("%s :enable.%d, gpio_num.%d\n", __func__, dev_ntf->enable, dev_ntf->gpio_num);

	return 0;
}

static int hisp_notify_init(struct hisp_notify *dev_rep)
{
	int ret;

	dev_rep->ap_a7_mbox = IPC_ACPU_ISP_MBX_2;
	dev_rep->a7_ap_mbox = IPC_ISP_ACPU_MBX_2;
	mutex_init(&dev_rep->notify_mutex);

	dev_rep->nb.notifier_call = hisp_notify_callback;
	ret = RPROC_MONITOR_REGISTER(dev_rep->a7_ap_mbox,
		&dev_rep->nb);/*lint !e64 */
	if (ret != 0) {
		pr_err("%s failed : RPROC_MONITOR_REGISTER.%d\n", __func__, ret);
		return -1;
	}

	return 0;
}

static int hisp_notify_deinit(void)
{
	int ret;
	struct hisp_notify *dev_rep = &hisp_notify_dev;

	mutex_destroy(&dev_rep->notify_mutex);
	ret = RPROC_MONITOR_UNREGISTER(dev_rep->a7_ap_mbox, &dev_rep->nb);
	if (ret != 0)
		pr_err("Failed :RPROC_MONITOR_UNREGISTER.%d\n", ret);

	return 0;
}

void hisp_notify_probe(struct platform_device *pdev)
{
	struct hisp_notify *dev_rep = &hisp_notify_dev;
	int ret;

	pr_alert("[%s] +\n", __func__);

	dev_rep->dev = &pdev->dev;

	ret = hisp_notify_getdts(dev_rep);
	if (ret < 0) {
		pr_err("[%s] : hisp notify not support.%d\n", __func__, ret);
		return;
	}

	ret = hisp_notify_init(dev_rep);
	if (ret < 0) {
		pr_err("[%s] : hisp_notify_init failed.%d\n", __func__, ret);
		return;
	}

	pr_alert("[%s] -\n", __func__);
}

void hisp_notify_remove(void)
{
	hisp_notify_deinit();
}

