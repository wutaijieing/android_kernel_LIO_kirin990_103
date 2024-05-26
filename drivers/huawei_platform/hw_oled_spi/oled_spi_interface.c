// SPDX-License-Identifier: GPL-2.0
/*
 * oled_spi_interface.c
 *
 * driver for oled spi interface
 *
 * Copyright (c) 2022-2022 Huawei Technologies Co., Ltd.
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
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/mm.h>
#include <linux/vmalloc.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/fb.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/uaccess.h>
#include <linux/semaphore.h>
#include <linux/interrupt.h>
#include <linux/timer.h>
#include <linux/jiffies.h>
#include <linux/kthread.h>
#include <linux/platform_device.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/dma-mapping.h>
#include <linux/regulator/consumer.h>
#include <linux/leds.h>
#include <linux/uaccess.h>
#include <linux/timekeeping.h>
#include <huawei_platform/log/hw_log.h>
#include "oled_spi_display.h"

#ifdef CONFIG_OF
#include <linux/of_address.h>
#include <linux/of.h>
#endif

#define HWLOG_TAG oled_spi
HWLOG_REGIST();

#define SPIDEV_MAJOR      150 /* SPI major dev no. */
#define N_SPI_MINORS      32 /* SPI minor dev no. */

static DECLARE_BITMAP(minors, N_SPI_MINORS);
static struct v7r2_oled g_oled_dev;

static LIST_HEAD(device_list);
static DEFINE_MUTEX(device_list_lock);

static int oled_spi_transfer(u8 *tx_buf, unsigned int len)
{
	struct spi_message msg;
	struct spi_device *sdev = g_oled_dev.spidev_data->spi;
	struct spi_transfer xfer = {
		.tx_buf = tx_buf,
		.len    = len,
	};
	int rc = 0;

	spi_message_init(&msg);
	spi_message_add_tail(&xfer, &msg);
	rc = spi_sync(sdev, &msg);

	return rc;
}

static s32 __init oled_spidev_probe(struct spi_device *spi)
{
	struct oled_spidev_data *spidev;
	s32 status;
	u32 minor;
	u32 tmp;
	int retval = 0;
	u8 save;

	spidev = kzalloc(sizeof(*spidev), GFP_KERNEL);
	if (!spidev)
		return -ENOMEM;
	g_oled_dev.spidev_data = spidev;

	sema_init(&spidev->sem_cmd, OLED_SPI_SEM_FULL);
	spidev->spi = spi;
	spin_lock_init(&spidev->spi_lock);
	mutex_init(&spidev->buf_lock);
	INIT_LIST_HEAD(&spidev->device_entry);
	mutex_lock(&device_list_lock);
	minor = find_first_zero_bit(minors, N_SPI_MINORS);
	if (minor < N_SPI_MINORS) {
		struct device *dev;

		spidev->devt = MKDEV(SPIDEV_MAJOR, minor);
		dev = device_create(g_oled_dev.oled_class, &spi->dev, spidev->devt,
				spidev, "oled%d.%d",
				spi->master->bus_num, spi->chip_select);
		status = IS_ERR(dev) ? PTR_ERR(dev) : 0;
	} else {
		dev_dbg(&spi->dev, "no minor number available!\n");
		status = -ENODEV;
	}
	if (status == 0) {
		set_bit(minor, minors);
		list_add(&spidev->device_entry, &device_list);
	}
	mutex_unlock(&device_list_lock);

	if (status == 0) {
		spi_set_drvdata(spi, spidev);
		hwlog_err("%s status = 0\n", __func__);
	} else {
		kfree(spidev);
		hwlog_err("%s kfree spidev\n", __func__);
		return status;
	}

	save = spi->mode;
	tmp = SPI_MODE_3;

	spi->mode = (u16)tmp;
	spi->bits_per_word = BITS_8;
	spi->chip_select = 0;
	spi->max_speed_hz = SPI_MAX_HZ;

	retval = spi_setup(spi);
	if (retval < 0) {
		spi->mode = save;
		kfree(spidev);
		hwlog_err("%s :set up failed,now spi mode %02x\n",
			__func__, spi->mode);
		return -1;
	}

	hwlog_info("oled_spidev_probe is over!\n");
	return status;
}

static s32 __exit oled_spidev_remove(struct spi_device *spi)
{
	struct oled_spidev_data *spidev = spi_get_drvdata(spi);

	/* make sure ops on existing fds can abort cleanly */
	spin_lock_irq(&spidev->spi_lock);
	spidev->spi = NULL;
	spi_set_drvdata(spi, NULL);
	spin_unlock_irq(&spidev->spi_lock);

	/* prevent new opens */
	mutex_lock(&device_list_lock);
	list_del(&spidev->device_entry);
	device_destroy(g_oled_dev.oled_class, spidev->devt);
	clear_bit(MINOR(spidev->devt), minors);
	if (spidev->users == 0)
		kfree(spidev);
	mutex_unlock(&device_list_lock);

	return 0;
}

int oled_display_transfer(u8 *tx_buf, unsigned int len)
{
	int ret;
	u8 display_cmd = OLED_SPI_MEMORY_WRITE;
	struct timespec64 tv_before = {0};
	struct timespec64 tv_after = {0};
	int time_use = 0;
	struct spi_device *sdev = g_oled_dev.spidev_data->spi;

	down(&g_oled_dev.spidev_data->sem_cmd);
	/* DCX: 0, cmd mode */
	ret = oled_spi_set_dcx(DCX_CMD_MODE);
	if (ret)
		goto set_up_err;

	ret = oled_spi_transfer(&display_cmd, 1);
	if (ret)
		goto cmd_write_err;

	ret = oled_spi_set_dcx(DCX_DATA_MODE);
	if (ret)
		goto cmd_write_err;

	ktime_get_real_ts64(&tv_before);

	sdev->bits_per_word = BITS_16;
	ret = spi_setup(sdev);
	if (ret)
		goto set_up_err;

	ret = oled_spi_transfer(tx_buf, len);
	if (ret)
		goto cmd_write_err;

	sdev->bits_per_word = BITS_8;
	ret = spi_setup(sdev);
	if (ret)
		goto set_up_err;

	ktime_get_real_ts64(&tv_after);
	time_use = ((tv_after.tv_sec - tv_before.tv_sec) * TIME_NS_UINT +
		(tv_after.tv_nsec - tv_before.tv_nsec)) / TIME_UINT_1000;
	hwlog_info("%s :test 1fps time:%d\n", __func__, time_use);

	up(&g_oled_dev.spidev_data->sem_cmd);
	return 0;
cmd_write_err:
	hwlog_err("%s: spi write cmd error!\n", __func__);
set_up_err:
	hwlog_err("%s :spi set up failed\n", __func__);
	up(&g_oled_dev.spidev_data->sem_cmd);
	return ret;
}

int oled_data_cmd_transfer(u8 *tx_buf, unsigned int len, int mode)
{
	unsigned int ret = 0;

	down(&g_oled_dev.spidev_data->sem_cmd);
	/* DCX: 0, cmd mode */
	ret = oled_spi_set_dcx(mode);
	if (ret)
		goto err_out;

	ret = oled_spi_transfer(tx_buf, len);
	if (ret)
		goto err_out;

	up(&g_oled_dev.spidev_data->sem_cmd);
	return 0;
err_out:
	hwlog_err("%s: spi write cmd error!\n", __func__);
	up(&g_oled_dev.spidev_data->sem_cmd);
	return ret;
}

static const struct of_device_id oled_spi_dts[] = {
	{ .compatible = "raydium,spi_oled" },
};

static struct spi_driver oled_spi_drv = {
	.probe  = oled_spidev_probe,
	.remove = __exit_p(oled_spidev_remove),
	.driver = {
		.name = "hw_oled_spi",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(oled_spi_dts),
	},
};

int oled_spi_init(void)
{
	int ret;

	g_oled_dev.oled_class = class_create(THIS_MODULE, "oled");
	if (IS_ERR(g_oled_dev.oled_class)) {
		hwlog_err("%s:class_create oled class error\n", __func__);
		return -1;
	}

	ret = spi_register_driver(&oled_spi_drv);
	if (ret) {
		hwlog_err("%s:couldn't register SPI Interface\n", __func__);
		goto class_des;
	}

	return 0;
class_des:
	class_destroy(g_oled_dev.oled_class);
	return ret;
}
