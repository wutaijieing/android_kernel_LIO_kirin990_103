/*
* lcd_kit_spidev.c
*
* lcdkit parse function for lcdkit
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

#include <linux/module.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/list.h>
#include <linux/errno.h>
#include <linux/mutex.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/spi/spi.h>
#include <linux/uaccess.h>
#include <linux/interrupt.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <securec.h>
#include "lcd_kit_spidev.h"
#include "lcd_kit_common.h"
#if defined(CONFIG_LCD_KIT_HISI)
#include "lcd_kit_panel.h"
#endif

#define SPIDEV_MAJOR                    153
#define N_SPI_MINORS                    32
#define MAX_DEV_ATTR_NUM                10
#define EINK_SEND_BUFF_SIZE             256
#define EINK_SEND_MAX_TRANS_NUM         630
#define LCD_KIT_SPI_NAME                "lcd_kit_spi"

static DECLARE_BITMAP(minors, N_SPI_MINORS);
static int spidev_create_chrdev(void);
static void spidev_destory_chrdev(void);

struct ser_req {
	u8    ref_on;
	u8    command;
	u8    ref_off;
	u16   scratch;
	struct spi_message  msg;
	struct spi_transfer xfer[EINK_SEND_MAX_TRANS_NUM];
	/*
	* DMA (thus cache coherency maintenance) requires the
	* transfer buffers to live in their own cache lines.
	*/
	__be16 sample ____cacheline_aligned;
};

struct spidev_data {
	dev_t                   devt;
	spinlock_t              spi_lock;
	struct spi_device       *spi;
	struct list_head        device_entry;

	/* TX/RX buffers are NULL unless this device is open (users > 0) */
	struct mutex            buf_lock;
	unsigned                users;
	u8                      *tx_buffer;
	u8                      *rx_buffer;
	u32                     speed_hz;
};

static LIST_HEAD(device_list);
static unsigned tx_bufsiz = 160001; /* max size for t1000 write per 5ms */
static unsigned rx_bufsiz = 1024; /* max size for t1000 read per times */
static int cs_gpio;

module_param(tx_bufsiz, uint, S_IRUGO);
module_param(rx_bufsiz, uint, S_IRUGO);
MODULE_PARM_DESC(bufsiz, "data tx bytes in biggest supported SPI message");
MODULE_PARM_DESC(bufsiz, "data rx bytes in biggest supported SPI message");

static ssize_t spidev_sync(struct spidev_data *spidev, struct spi_message *message)
{
	int status;
	struct spi_device *spi = NULL;

	spin_lock_irq(&spidev->spi_lock);
	spi = spidev->spi;
	spin_unlock_irq(&spidev->spi_lock);

	if (spi == NULL)
		status = -ESHUTDOWN;
	else
		status = spi_sync(spi, message);

	if (status == 0)
		status = (int)message->actual_length;

	return status;
}

static ssize_t spidev_sync_write(struct spidev_data *spidev, size_t len)
{
	struct ser_req *req;
	int i = 0;
	ssize_t status;
	int size = sizeof(struct ser_req);

	req = kzalloc(size, GFP_KERNEL);
	if (!req)
		return -ENOMEM;
	spi_message_init(&req->msg);

	/* the max value of len / EINK_SEND_BUFF_SIZE is 625 so it is safe */
	for (i = 0; i < (int)(len / EINK_SEND_BUFF_SIZE); i++) {
		req->xfer[i].tx_buf = spidev->tx_buffer + i * EINK_SEND_BUFF_SIZE;
		req->xfer[i].len = EINK_SEND_BUFF_SIZE;
		spi_message_add_tail(&req->xfer[i], &req->msg);
	}
	if (len % EINK_SEND_BUFF_SIZE) {
		req->xfer[i].tx_buf = spidev->tx_buffer + i * EINK_SEND_BUFF_SIZE;
		req->xfer[i].len = len % EINK_SEND_BUFF_SIZE;
		spi_message_add_tail(&req->xfer[i], &req->msg);
	}

	status = spidev_sync(spidev, &req->msg);
	kfree(req);
	return status;
}

static ssize_t spidev_sync_read(struct spidev_data *spidev, size_t len)
{
	struct spi_transfer t = {
		.rx_buf = spidev->rx_buffer,
		.len = len,
		.speed_hz = spidev->speed_hz,
	};
	struct spi_message m;
	spi_message_init(&m);
	spi_message_add_tail(&t, &m);
	return spidev_sync(spidev, &m);
}

static ssize_t spidev_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
	struct spidev_data *spidev = NULL;
	ssize_t status = 0;
	unsigned char cs_change;
	unsigned long missing;
	ssize_t i;

	if (count > rx_bufsiz) {
		LCD_KIT_ERR("err count %d\n", count);
		return -EMSGSIZE;
	}

	spidev = filp->private_data;

	missing = copy_from_user(&cs_change, buf, 1);
	if (missing != 0) {
		LCD_KIT_ERR("spidev_read copy_from_user err\n");
		return -EFAULT;
	}

	gpio_set_value(cs_gpio, 0);

	status = spidev_sync_read(spidev, count);

	if (cs_change == 0)
		gpio_set_value(cs_gpio, 1);

	if (status < 0)
		return -EFAULT;

	for (i = 0; i < status; i++)
		LCD_KIT_DEBUG("buffer[%d] = 0x%x\n", i, spidev->rx_buffer[i]);

	if (status > 0) {
		missing = copy_to_user(buf, spidev->rx_buffer, status);
		if (missing == (unsigned long)status)
			status = -EFAULT;
		else
			status = status - (long)missing;
	}

	return status;
}

static ssize_t spidev_write(struct file *filp, const char __user *buf,
		size_t count, loff_t *f_pos)
{
	struct spidev_data *spidev = NULL;
	ssize_t status = 0;
	unsigned long missing;
	unsigned char cs_change;
#if defined(CONFIG_LCD_KIT_HISI)
	struct lcd_kit_panel_ops *panel_ops = NULL;
#endif

	if (count > tx_bufsiz) {
		LCD_KIT_ERR("err count %d\n", count);
		return -EMSGSIZE;
	}

	spidev = filp->private_data;
	missing = copy_from_user(&cs_change, buf, 1);
	if (missing != 0) {
		LCD_KIT_ERR("copy_from_user missing\n");
		return -EFAULT;
	}
	missing = copy_from_user(spidev->tx_buffer, buf + 1, count - 1);
	if (missing == 0){
		gpio_set_value(cs_gpio, 0);
		status = spidev_sync_write(spidev, count - 1);
	} else {
		status = -EFAULT;
	}
	if (cs_change == 0)
		gpio_set_value(cs_gpio, 1);
#if defined(CONFIG_LCD_KIT_HISI)
	panel_ops = lcd_kit_panel_get_ops();
	if (panel_ops && panel_ops->eink_spi_async_time)
	    panel_ops->eink_spi_async_time();
#endif
	return status;
}

static int spidev_open(struct inode *inode, struct file *filp)
{
	struct spidev_data *spidev = NULL;
	int status = -ENXIO;

	list_for_each_entry(spidev, &device_list, device_entry) {
		if (spidev->devt == inode->i_rdev) {
			status = 0;
			break;
		}
	}

	if (status) {
		LCD_KIT_ERR("spidev: nothing for minor %d\n", iminor(inode));
		goto err_find_dev;
	}

	if (!spidev->tx_buffer) {
		spidev->tx_buffer = kmalloc(tx_bufsiz, GFP_KERNEL);
		if (!spidev->tx_buffer) {
			LCD_KIT_ERR("kmalloc tx_buff error");
			status = -ENOMEM;
			goto err_find_dev;
		}
	}

	if (!spidev->rx_buffer) {
		spidev->rx_buffer = kmalloc(rx_bufsiz, GFP_KERNEL);
		if (!spidev->rx_buffer) {
			LCD_KIT_ERR("kmalloc rx_buff error");
			status = -ENOMEM;
			goto err_alloc_rx_buf;
		}
	}

	filp->private_data = spidev;
	nonseekable_open(inode, filp);

	return 0;

err_alloc_rx_buf:
	kfree(spidev->tx_buffer);
	spidev->tx_buffer = NULL;
err_find_dev:

	return status;
}

static int spidev_release(struct inode *inode, struct file *filp)
{
	struct spidev_data *spidev = NULL;
	int dofree;

	spidev = filp->private_data;
	filp->private_data = NULL;

	if (spidev->tx_buffer) {
		kfree(spidev->tx_buffer);
		spidev->tx_buffer = NULL;
	}

	if (spidev->rx_buffer) {
		kfree(spidev->rx_buffer);
		spidev->rx_buffer = NULL;
	}

	spin_lock_irq(&spidev->spi_lock);
	if (spidev->spi)
		spidev->speed_hz = spidev->spi->max_speed_hz;

	dofree = (spidev->spi == NULL);
	spin_unlock_irq(&spidev->spi_lock);

	if (dofree)
		kfree(spidev);
	return 0;
}

static const struct file_operations spidev_fops = {
	.owner = THIS_MODULE,
	.write = spidev_write,
	.read = spidev_read,
	.open = spidev_open,
	.release = spidev_release,
};

static struct class *spidev_class;

static struct lcd_kit_spi_hrdy spi_hrdy;
static ssize_t spi_hrdy_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int count;
	spi_hrdy.status = gpio_get_value(spi_hrdy.gpio);
	count = sprintf_s(buf, sizeof(int), "%d\n", spi_hrdy.status);

	return count;
}
static DEVICE_ATTR(hrdy, S_IRUGO, spi_hrdy_show, NULL);

static struct device_attribute *spi_attribute[] = {
	&dev_attr_hrdy,
};

static void spidev_create_attribute(struct device *dev)
{
	int size, idx;
	size = ARRAY_SIZE(spi_attribute);
	if (size > MAX_DEV_ATTR_NUM) {
		LCD_KIT_ERR("size invalid\n");
		return;
	}
	for (idx = 0; idx < size; idx++)
		device_create_file(dev, spi_attribute[idx]);
}

int get_spi_hrdy_status(void)
{
	int ret;
	ret = spi_hrdy.status;
	return ret;
}

static irqreturn_t spi_hrdy_irq_handler(int irq, void *_data)
{
	spi_hrdy.status = gpio_get_value(spi_hrdy.gpio);
	return IRQ_HANDLED;
}

static int spidev_prase_dt(struct device *dev)
{
	struct device_node *node = dev->of_node;
	int ret;

	LCD_KIT_INFO("spidev_prase_dt enter\n");
	cs_gpio = of_get_named_gpio(node, "cs_gpio", 0);
	if (!gpio_is_valid(cs_gpio)) {
		LCD_KIT_ERR("cs_gpio node not found\n");
		return -EFAULT;
	} else {
		ret = gpio_request(cs_gpio, "lcd_kit_spi_cs_gpio");
		if (ret) {
			LCD_KIT_ERR("cs_gpio request failed\n");
			return -EFAULT;
		}
		gpio_direction_output(cs_gpio, 1);
		gpio_set_value(cs_gpio, 1);
		LCD_KIT_INFO("cs-gpio = %d default high\n", cs_gpio);
	}

	spi_hrdy.gpio = of_get_named_gpio(node, "gpio_hrdy", 0);

	if (!gpio_is_valid(spi_hrdy.gpio)) {
		LCD_KIT_ERR("gpio_hrdy not found\n");
	} else {
		LCD_KIT_INFO("config gpio_hrdy pin %d\n", spi_hrdy.gpio);
		devm_gpio_request_one(dev, spi_hrdy.gpio, GPIOF_IN, "lcd_kit_spi_hrdy");

		spi_hrdy.status = gpio_get_value(spi_hrdy.gpio);

		LCD_KIT_INFO("spi_hrdy.status %d\n", spi_hrdy.status);
		ret = gpio_to_irq(spi_hrdy.gpio);
		if (ret < 0) {
			LCD_KIT_ERR("get gpio_hrdy irq failed\n");
			return -EFAULT;
		}

		spi_hrdy.irq = ret;
		if (devm_request_threaded_irq(dev, spi_hrdy.irq, NULL, spi_hrdy_irq_handler,
			IRQF_ONESHOT | IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING, "lcd_kit_spi_hrdy", NULL))
			LCD_KIT_ERR("request hrdy irq failed\n");
		else
			LCD_KIT_INFO("config hrdy irq ok\n");
	}
	return 0;
}

static int spidev_probe(struct spi_device *spi)
{
	int status;
	unsigned long minor;
	struct spidev_data *spidev = NULL;

	status = spidev_create_chrdev();
	if (status)
		return status;
	spidev = kzalloc(sizeof(*spidev), GFP_KERNEL);
	if (!spidev) {
		spidev_destory_chrdev();
		return -ENOMEM;
	}
	spidev->spi = spi;
	spin_lock_init(&spidev->spi_lock);
	mutex_init(&spidev->buf_lock);

	INIT_LIST_HEAD(&spidev->device_entry);

	status = spidev_prase_dt(&spi->dev);
	if (status) {
		kfree(spidev);
		spidev_destory_chrdev();
		return status;
	}
	spidev_create_attribute(&spi->dev);

	minor = find_first_zero_bit(minors, N_SPI_MINORS);
	if (minor < N_SPI_MINORS) {
		struct device *dev;
		spidev->devt = MKDEV(SPIDEV_MAJOR, minor);
		dev = device_create(spidev_class, &spi->dev, spidev->devt,
			spidev, "spidev%d.%d", spi->master->bus_num, spi->chip_select);
		status = PTR_ERR_OR_ZERO(dev);
	} else {
		dev_dbg(&spi->dev, "no minor number available!\n");
		status = -ENODEV;
	}
	if (status == 0) {
		set_bit(minor, minors);
		list_add(&spidev->device_entry, &device_list);
	}
	spidev->speed_hz = spi->max_speed_hz;
	if (status == 0) {
		spi_set_drvdata(spi, spidev);
	} else {
		kfree(spidev);
		spidev_destory_chrdev();
	}
	return status;
}

static int spidev_remove(struct spi_device *spi)
{
	struct spidev_data *spidev = spi_get_drvdata(spi);

	/* make sure ops on existing fds can abort cleanly */
	spin_lock_irq(&spidev->spi_lock);
	spidev->spi = NULL;
	spin_unlock_irq(&spidev->spi_lock);

	/* prevent new opens */
	list_del(&spidev->device_entry);
	device_destroy(spidev_class, spidev->devt);
	clear_bit(MINOR(spidev->devt), minors);
	if (spidev->users == 0)
		kfree(spidev);

	return 0;
}

static const struct of_device_id lcd_kit_spidev_dt_ids[] = {
	{ .compatible = LCD_KIT_SPI_NAME, },
	{ },
};
MODULE_DEVICE_TABLE(of, lcd_kit_spidev_dt_ids);
static struct spi_driver lcd_kit_spidev_driver = {
	.driver = {
		.name = LCD_KIT_SPI_NAME,
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(lcd_kit_spidev_dt_ids),
	},
	.probe = spidev_probe,
	.remove = spidev_remove,
};

static int spidev_create_chrdev(void)
{
	int status;

	BUILD_BUG_ON(N_SPI_MINORS > 256);
	status = register_chrdev(SPIDEV_MAJOR, "spi", &spidev_fops);
	if (status < 0)
		return status;

	spidev_class = class_create(THIS_MODULE, "spidev");
	if (IS_ERR(spidev_class)) {
		return PTR_ERR(spidev_class);
	}

	return status;
}

static void spidev_destory_chrdev(void)
{
	class_destroy(spidev_class);
	unregister_chrdev(SPIDEV_MAJOR, lcd_kit_spidev_driver.driver.name);
}

module_spi_driver(lcd_kit_spidev_driver);

MODULE_AUTHOR("Huawei Technologies Co., Ltd");
MODULE_LICENSE("GPL");
