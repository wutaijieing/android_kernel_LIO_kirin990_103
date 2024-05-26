/*
 * Adp Acm function driver Test
 *
 * Copyright (c) 2015-2020 Huawei Technologies Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * either version 2 of that License or (at your option) any later version.
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/freezer.h>
#include <linux/skbuff.h>
#include <asm/cacheflush.h>

#include <platform_include/basicplatform/linux/usb/bsp_acm.h>
#include <platform_include/basicplatform/linux/usb/drv_udi.h>
#include <platform_include/basicplatform/linux/usb/drv_acm.h>
#include <securec.h>

#include "usb_vendor.h"

#define adp_acm_log(_dev_id, format, arg...)	\
	pr_info("[ADP_ACM_TEST][DEV %d][%s]" format, _dev_id, __func__, ##arg)

#define adp_acm_fail(_dev_id, format, arg...)	\
	pr_err("[ADP_ACM_TEST][DEV %d][%s][failed]" format, _dev_id,	\
			__func__, ##arg)

#define NAME_LEN 32
#define MAX_MULTI_COUNT 100000
#define MAX_XFER_LEN 65536
#define WRITE_BUFS_LEN MAX_XFER_LEN
#define WRITE_BUFS_NUM 64
#define ACM_REQ_NUM_MAX 16
#define ACM_REQ_NUM_MIN 8
#define ACM_IOCTL_ASYNC_LIMIT 100
#define ACM_CTRL_BUF_SIZE 0x400
#define ACM_AT_BUF_SIZE 0x400
#define ACM_GPS_BUF_SIZE 0x2000
#define ACM_MODEM_BUF_SIZE 0x600
#define ACM_DIAG_BUF_SIZE 0x2000
#define ACM_OM_BUF_SIZE 0x4000
#define ACM_SKYTONE_BUF_SIZE 0x400


unsigned int xfer_len_array[] = { 1, 2, 20, 64, 65, 512, 65500, MAX_XFER_LEN };

struct adp_acm_device {
	unsigned int dev_id;
	void *handler;

	unsigned int read_req_num;
	unsigned int read_buf_size;
	ACM_READ_DONE_CB_T read_done_cb;
	atomic_t read_done_cb_count;
	wait_queue_head_t read_wait;
	struct task_struct *read_thread;
	char read_thread_name[NAME_LEN];
	bool async_read;

	unsigned long total_read_len;
	unsigned long total_write_len;

	char *write_bufs[WRITE_BUFS_NUM];
	spinlock_t write_bufs_lock;
	ACM_WRITE_DONE_CB_T write_done_cb;

	bool loop_back;
};

#define adp_acm_read_done_cb(_name)			\
	static void read_done_cb_ ##_name(void)	\
	{					\
		read_done_cb(_name);		\
	}

#define read_done_cb_name(_name) (read_done_cb_ ##_name)

static void read_done_cb(u32 dev_id);

adp_acm_read_done_cb(UDI_USB_ACM_CTRL);
adp_acm_read_done_cb(UDI_USB_ACM_AT);
adp_acm_read_done_cb(UDI_USB_ACM_SHELL);
adp_acm_read_done_cb(UDI_USB_ACM_LTE_DIAG);
adp_acm_read_done_cb(UDI_USB_ACM_OM);
adp_acm_read_done_cb(UDI_USB_ACM_MODEM);
adp_acm_read_done_cb(UDI_USB_ACM_GPS);
adp_acm_read_done_cb(UDI_USB_ACM_3G_GPS);
adp_acm_read_done_cb(UDI_USB_ACM_3G_PCVOICE);
adp_acm_read_done_cb(UDI_USB_ACM_PCVOICE);
adp_acm_read_done_cb(UDI_USB_ACM_SKYTONE);

#define adp_acm_write_done_cb(_name)						\
	static void write_done_cb_ ##_name(char *vir_addr,		\
				char *phy_addr, int size)		\
	{								\
		write_done_cb(_name, vir_addr, phy_addr, size);		\
	}

#define write_done_cb_name(_name) (write_done_cb_ ##_name)

static void write_done_cb(u32 dev_id, char *vir_addr, char *phy_addr, int size);

adp_acm_write_done_cb(UDI_USB_ACM_CTRL);
adp_acm_write_done_cb(UDI_USB_ACM_AT);
adp_acm_write_done_cb(UDI_USB_ACM_SHELL);
adp_acm_write_done_cb(UDI_USB_ACM_LTE_DIAG);
adp_acm_write_done_cb(UDI_USB_ACM_OM);
adp_acm_write_done_cb(UDI_USB_ACM_MODEM);
adp_acm_write_done_cb(UDI_USB_ACM_GPS);
adp_acm_write_done_cb(UDI_USB_ACM_3G_GPS);
adp_acm_write_done_cb(UDI_USB_ACM_3G_PCVOICE);
adp_acm_write_done_cb(UDI_USB_ACM_PCVOICE);
adp_acm_write_done_cb(UDI_USB_ACM_SKYTONE);

struct adp_acm_device adp_acm_devices[UDI_USB_ACM_MAX] = {
	[UDI_USB_ACM_CTRL] = {
		.read_req_num = ACM_REQ_NUM_MAX,
		.read_buf_size = ACM_CTRL_BUF_SIZE,
		.read_done_cb_count = ATOMIC_INIT(0),
		.read_done_cb = read_done_cb_name(UDI_USB_ACM_CTRL),
		.write_done_cb = write_done_cb_name(UDI_USB_ACM_CTRL),
	},
	[UDI_USB_ACM_AT] = {
		.read_req_num = ACM_REQ_NUM_MAX,
		.read_buf_size = ACM_AT_BUF_SIZE,
		.read_done_cb_count = ATOMIC_INIT(0),
		.read_done_cb = read_done_cb_name(UDI_USB_ACM_AT),
		.write_done_cb = write_done_cb_name(UDI_USB_ACM_AT),
	},
	[UDI_USB_ACM_SHELL] = {
		.read_done_cb_count = ATOMIC_INIT(0),
		.read_done_cb = read_done_cb_name(UDI_USB_ACM_SHELL),
		.write_done_cb = write_done_cb_name(UDI_USB_ACM_SHELL),
	},
	[UDI_USB_ACM_LTE_DIAG] = {
		.read_req_num = ACM_REQ_NUM_MIN,
		.read_buf_size = ACM_DIAG_BUF_SIZE,
		.read_done_cb_count = ATOMIC_INIT(0),
		.read_done_cb = read_done_cb_name(UDI_USB_ACM_LTE_DIAG),
		.write_done_cb = write_done_cb_name(UDI_USB_ACM_LTE_DIAG),
	},
	[UDI_USB_ACM_OM] = {
		.read_req_num = ACM_REQ_NUM_MAX,
		.read_buf_size = ACM_OM_BUF_SIZE,
		.read_done_cb_count = ATOMIC_INIT(0),
		.read_done_cb = read_done_cb_name(UDI_USB_ACM_OM),
		.write_done_cb = write_done_cb_name(UDI_USB_ACM_OM),
	},
	[UDI_USB_ACM_MODEM] = {
		.read_req_num = ACM_REQ_NUM_MAX,
		.read_buf_size = ACM_MODEM_BUF_SIZE,
		.read_done_cb_count = ATOMIC_INIT(0),
		.read_done_cb = read_done_cb_name(UDI_USB_ACM_MODEM),
		.write_done_cb = write_done_cb_name(UDI_USB_ACM_MODEM),
	},
	[UDI_USB_ACM_GPS] = {
		.read_req_num = ACM_REQ_NUM_MIN,
		.read_buf_size = ACM_GPS_BUF_SIZE,
		.read_done_cb_count = ATOMIC_INIT(0),
		.read_done_cb = read_done_cb_name(UDI_USB_ACM_GPS),
		.write_done_cb = write_done_cb_name(UDI_USB_ACM_GPS),
	},
	[UDI_USB_ACM_3G_GPS] = {
		.read_done_cb_count = ATOMIC_INIT(0),
		.read_done_cb = read_done_cb_name(UDI_USB_ACM_3G_GPS),
		.write_done_cb = write_done_cb_name(UDI_USB_ACM_3G_GPS),
	},
	[UDI_USB_ACM_3G_PCVOICE] = {
		.read_done_cb_count = ATOMIC_INIT(0),
		.read_done_cb = read_done_cb_name(UDI_USB_ACM_3G_PCVOICE),
		.write_done_cb = write_done_cb_name(UDI_USB_ACM_3G_PCVOICE),
	},
	[UDI_USB_ACM_PCVOICE] = {
		.read_done_cb_count = ATOMIC_INIT(0),
		.read_done_cb = read_done_cb_name(UDI_USB_ACM_PCVOICE),
		.write_done_cb = write_done_cb_name(UDI_USB_ACM_PCVOICE),
	},
	[UDI_USB_ACM_SKYTONE] = {
		.read_req_num = ACM_REQ_NUM_MAX,
		.read_buf_size = ACM_SKYTONE_BUF_SIZE,
		.read_done_cb_count = ATOMIC_INIT(0),
		.read_done_cb = read_done_cb_name(UDI_USB_ACM_SKYTONE),
		.write_done_cb = write_done_cb_name(UDI_USB_ACM_SKYTONE),
	},
};

static int init_write_bufs(struct adp_acm_device *dev)
{
	unsigned int i, j;
	char *buf = NULL;

	spin_lock_init(&dev->write_bufs_lock);

	for (i = 0; i < WRITE_BUFS_NUM; i++)
		dev->write_bufs[i] = NULL;

	for (i = 0; i < WRITE_BUFS_NUM; i++) {
		dev->write_bufs[i] = kzalloc(WRITE_BUFS_LEN, GFP_KERNEL);
		if (dev->write_bufs[i] == NULL) {
			adp_acm_fail(dev->dev_id, "kmalloc write buf %u failed\n", i);
			return -1;
		}
		buf = dev->write_bufs[i];

		for (j = 0; j < WRITE_BUFS_LEN; j++)
			buf[j] = (char)(j & 0xff);
	}
	return 0;
}

static void free_write_bufs(struct adp_acm_device *dev)
{
	int i;

	for (i = 0; i < WRITE_BUFS_NUM; i++) {
		kfree(dev->write_bufs[i]);
		dev->write_bufs[i] = NULL;
	}
}

/*
 * these two functions never called in interrupt context,
 * so we can operate spinlock without "irqsave".
 */
static char *get_write_buf(struct adp_acm_device *dev)
{
	int i;
	char *buf = NULL;

	spin_lock(&dev->write_bufs_lock);

	for (i = 0; i < WRITE_BUFS_NUM; i++) {
		if (dev->write_bufs[i] != NULL) {
			buf = dev->write_bufs[i];
			dev->write_bufs[i] = NULL;
			break;
		}
	}

	spin_unlock(&dev->write_bufs_lock);

	return buf;
}

static void ret_write_buf(struct adp_acm_device *dev, char *buf)
{
	int i;

	spin_lock(&dev->write_bufs_lock);

	for (i = 0; i < WRITE_BUFS_NUM; i++) {
		if (dev->write_bufs[i] == NULL) {
			dev->write_bufs[i] = buf;
			break;
		}
	}

	spin_unlock(&dev->write_bufs_lock);

	if (i >= WRITE_BUFS_NUM) {
		adp_acm_fail(dev->dev_id, "ret write_buf failed\n");
		dump_stack();
	}
}

static int write_async(void *handler, char *buf, unsigned int size)
{
	ACM_WR_ASYNC_INFO rw_info;

	rw_info.u32Size = size;
	rw_info.pVirAddr = buf;
	rw_info.pPhyAddr = (char *)(uintptr_t)virt_to_phys(rw_info.pVirAddr);
	__dma_map_area(rw_info.pVirAddr, size, DMA_TO_DEVICE);

	return bsp_acm_ioctl(handler, ACM_IOCTL_WRITE_ASYNC, &rw_info);
}

static int loopback_write(struct adp_acm_device *adp_acm_dev,
				const char *data, u32 size)
{
	char *buf = NULL;
	u32 dev_id = adp_acm_dev->dev_id;
	void *handler = adp_acm_dev->handler;
	int ret;
	u32 copy_size = min_t(u32, size, WRITE_BUFS_LEN);

	/* get a buf used for send data */
	buf = get_write_buf(adp_acm_dev);
	if (buf == NULL) {
		adp_acm_fail(dev_id, "get write buf failed\n");
		return -ENOMEM;
	}

	/* copy data to buf */
	memcpy(buf, data, copy_size);

	/* send */
	ret = write_async(handler, buf, size);
	if (ret) {
		adp_acm_fail(dev_id, "loopback write_async failed %d\n", ret);
		ret_write_buf(adp_acm_dev, buf);
		return ret;
	}

	adp_acm_log(dev_id, "loopback write_async done\n");

	return 0;
}

static void write_done_cb(u32 dev_id, char *vir_addr, char *phy_addr, int size)
{
	if (size < 0)
		adp_acm_fail(dev_id, "async write complete %d\n", size);
	else
		adp_acm_log(dev_id, "async write complete %d\n", size);

	ret_write_buf(&adp_acm_devices[dev_id], vir_addr);
}

static void read_thread(struct adp_acm_device *adp_acm_dev)
{
	u32 dev_id = adp_acm_dev->dev_id;
	void *handler = adp_acm_dev->handler;
	ACM_WR_ASYNC_INFO rw_info = {0};
	int cb_count;
	int ret;

	while (1) {
		cb_count = atomic_read(&adp_acm_dev->read_done_cb_count);
		adp_acm_log(dev_id, "cb_count %d\n", cb_count);
		if (cb_count <= 0)
			break;

		if (adp_acm_dev->handler == NULL)
			break;

		/* get read buffer */
		ret = bsp_acm_ioctl(handler, ACM_IOCTL_GET_RD_BUFF, &rw_info);
		if (ret) {
			adp_acm_fail(dev_id, "get read buf failed\n");
		} else {
			/* get read buf successed */

			adp_acm_log(dev_id, "buf %pK, size %u\n",
				rw_info.pVirAddr, rw_info.u32Size);
			adp_acm_dev->total_read_len += rw_info.u32Size;

			/* loop back */
			if (adp_acm_dev->loop_back)
				loopback_write(adp_acm_dev,
					rw_info.pVirAddr, rw_info.u32Size);

			/* return read buffer */
			ret = bsp_acm_ioctl(handler,
					ACM_IOCTL_RETURN_BUFF, &rw_info);
			if (ret)
				adp_acm_fail(dev_id, "return read buf failed\n");
		}
		atomic_dec(&adp_acm_dev->read_done_cb_count);
	}
}

static int read_thread_fn(void *data)
{
	struct adp_acm_device *dev = data;

	pr_info("%s: read_thread begaining\n", dev->read_thread_name);
	set_freezable();
	do {
		if (dev->async_read != 0)
			read_thread(data);

		wait_event_freezable(dev->read_wait,
			((atomic_read(&dev->read_done_cb_count) != 0) ||
			(kthread_should_stop())));
		adp_acm_log(dev->dev_id, "cb_count %d\n",
				atomic_read(&dev->read_done_cb_count));
	} while (!kthread_should_stop()
			|| atomic_read(&dev->read_done_cb_count));

	pr_info("%s: read_thread exiting\n", dev->read_thread_name);
	return 0;
}

static long start_read_thread(u32 dev_id)
{
	struct adp_acm_device *dev = &adp_acm_devices[dev_id];

	init_waitqueue_head(&dev->read_wait);
	adp_acm_log(dev_id, "init read_wait done\n");

	scnprintf(dev->read_thread_name, NAME_LEN,
		"read_thread_%u", dev_id);
	adp_acm_log(dev_id, "read_thread: %s\n", dev->read_thread_name);

	dev->read_thread = kthread_run(read_thread_fn,
				dev, dev->read_thread_name);
	if (IS_ERR(dev->read_thread)) {
		adp_acm_fail(dev_id, "start read thread failed\n");
		return PTR_ERR(dev->read_thread);
	}

	return 0;
}

static int stop_read_thread(u32 dev_id)
{
	struct adp_acm_device *dev = &adp_acm_devices[dev_id];
	int ret;

	ret = kthread_stop(dev->read_thread);
	wake_up(&dev->read_wait);

	return ret;
}

static void read_done_cb(u32 dev_id)
{
	struct adp_acm_device *adp_acm_dev = &adp_acm_devices[dev_id];
	int cb_count;

	cb_count = atomic_inc_return(&adp_acm_dev->read_done_cb_count);
	adp_acm_log(dev_id, "cb_count %d\n", cb_count);

	if (adp_acm_dev->async_read != 0)
		wake_up(&adp_acm_dev->read_wait);
	else
		read_thread(adp_acm_dev);
}

static void event_cb(ACM_EVT_E evt)
{
	u32 dev_id = UDI_USB_ACM_MAX; /* stub!!! */

	adp_acm_log(dev_id, "evt: %d\n", evt);
}

int adp_acm_test_disconnect_modem(void)
{
	bsp_usb_status_change(USB_MODEM_DEVICE_DISABLE);
	return 0;
}

int adp_acm_test_connect_modem(void)
{
	bsp_usb_status_change(USB_MODEM_ENUM_DONE);
	return 0;
}

static int bsp_acm_realloc_buf(void *handler,
		struct adp_acm_device *adp_acm_dev, u32 dev_id)
{
	int ret = 0;

	if (adp_acm_dev->read_req_num && adp_acm_dev->read_buf_size) {
		ACM_READ_BUFF_INFO buf_info = {0};

		buf_info.u32BuffNum = adp_acm_dev->read_req_num;
		buf_info.u32BuffSize = adp_acm_dev->read_buf_size;
		ret = bsp_acm_ioctl(handler, ACM_IOCTL_RELLOC_READ_BUFF, &buf_info);
		if (ret) {
			adp_acm_fail(dev_id, "realloc buf failed\n");
			return ret;
		}
		adp_acm_log(dev_id, "realloc buf done, num %u, size %x\n",
				buf_info.u32BuffNum, buf_info.u32BuffSize);
	};
	return ret;
}

static int bsp_acm_cb_set(void *handler,
		struct adp_acm_device *adp_acm_dev, u32 dev_id)
{
	int ret;

	/* set read cb */
	ret = bsp_acm_ioctl(handler, ACM_IOCTL_SET_READ_CB,
					adp_acm_dev->read_done_cb);
	if (ret) {
		adp_acm_fail(dev_id, "set read cb failed\n");
		return ret;
	}
	adp_acm_log(dev_id, "set read cb done %pK\n", adp_acm_dev->read_done_cb);

	/* set write cb */
	ret = bsp_acm_ioctl(handler, ACM_IOCTL_SET_WRITE_CB,
					adp_acm_dev->write_done_cb);
	if (ret) {
		adp_acm_fail(dev_id, "set write cb failed\n");
		return ret;
	}
	adp_acm_log(dev_id, "set write cb done %pK\n", adp_acm_dev->write_done_cb);

	/* set do_copy */
	ret = bsp_acm_ioctl(handler, ACM_IOCTL_WRITE_DO_COPY, 0);
	if (ret) {
		adp_acm_fail(dev_id, "set do_copy failed\n");
		return ret;
	}

	/* set event cb */
	ret = bsp_acm_ioctl(handler, ACM_IOCTL_SET_EVT_CB, event_cb);
	if (ret) {
		adp_acm_fail(dev_id, "set event cb failed\n");
		return ret;
	}
	return ret;
}


int adp_acm_test_open(u32 dev_id)
{
	void *handler = NULL;
	int ret;
	long status;
	struct adp_acm_device *adp_acm_dev = NULL;

	if (dev_id >= UDI_USB_ACM_MAX) {
		adp_acm_fail(dev_id, "illegal dev_id\n");
		return -EINVAL;
	}

	if (dev_id == UDI_USB_ACM_MODEM) {
		adp_acm_fail(dev_id, "modem port!\n");
		return -EPERM;
	}

	adp_acm_dev = &adp_acm_devices[dev_id];

	if (adp_acm_dev->handler != NULL) {
		adp_acm_fail(dev_id, "already opend\n");
		return -EBUSY;
	}

	handler = bsp_acm_open(dev_id);
	if (IS_ERR_OR_NULL(handler)) {
		adp_acm_fail(dev_id, "open failed\n");
		adp_acm_dev->handler = NULL;
		return -ENOENT;
	}

	adp_acm_dev->dev_id = dev_id;
	adp_acm_dev->handler = handler;
	adp_acm_dev->total_read_len = 0;
	adp_acm_dev->total_write_len = 0;

	/* realloc buf */
	ret = bsp_acm_realloc_buf(handler, adp_acm_dev, dev_id);
	if (ret) {
		bsp_acm_close(handler);
		return ret;
	}
	/* acm cb set */
	ret = bsp_acm_cb_set(handler, adp_acm_dev, dev_id);
	if (ret) {
		bsp_acm_close(handler);
		return ret;
	}

	/* prepare data template */
	ret = init_write_bufs(adp_acm_dev);
	if (ret) {
		adp_acm_fail(dev_id, "init_write_infos failed\n");
		free_write_bufs(adp_acm_dev);
		return ret;
	}

	/* prepare read */
	status = start_read_thread(dev_id);
	if (status != 0)
		pr_err("[%s] start read thread err\n", __func__);

	return 0;
}

int adp_acm_test_close(u32 dev_id)
{
	void *handler = NULL;
	struct adp_acm_device *adp_acm_dev = NULL;
	int ret;

	if (dev_id >= UDI_USB_ACM_MAX) {
		adp_acm_fail(dev_id, "illegal dev_id\n");
		return -EINVAL;
	}

	if (dev_id == UDI_USB_ACM_MODEM) {
		adp_acm_fail(dev_id, "modem port!\n");
		return -EINVAL;
	}

	adp_acm_dev = &adp_acm_devices[dev_id];

	handler = adp_acm_dev->handler;
	if (handler == NULL) {
		adp_acm_fail(dev_id, "not opened\n");
		return -ENOENT;
	}

	/* stop read */
	stop_read_thread(dev_id);

	ret = bsp_acm_close(handler);

	adp_acm_dev->handler = NULL;

	/* free data template */
	free_write_bufs(adp_acm_dev);

	adp_acm_log(dev_id, "total_read_len %lu\n", adp_acm_dev->total_read_len);
	adp_acm_log(dev_id, "total_write_len %lu\n", adp_acm_dev->total_write_len);

	return ret;
}


int adp_acm_test_write(u32 dev_id, unsigned int size, unsigned int count)
{
	struct adp_acm_device *adp_acm_dev = NULL;
	void *handler = NULL;
	char *buf = NULL;
	s32 ret;
	unsigned int i;

	if (dev_id >= UDI_USB_ACM_MAX) {
		adp_acm_fail(dev_id, "illegal dev_id\n");
		return -EINVAL;
	}

	if (dev_id == UDI_USB_ACM_MODEM) {
		adp_acm_fail(dev_id, "modem port!\n");
		return -EPERM;
	}

	if (size > WRITE_BUFS_LEN) {
		adp_acm_fail(dev_id, "size large 0x%x\n", size);
		return -EINVAL;
	}

	if (count > WRITE_BUFS_NUM) {
		adp_acm_fail(dev_id, "count large 0x%x\n", count);
		return -EINVAL;
	}

	adp_acm_dev = &adp_acm_devices[dev_id];

	handler = adp_acm_dev->handler;
	if (handler == NULL) {
		adp_acm_fail(dev_id, "not opened\n");
		return -ENODEV;
	}

	for (i = 0; i < count; i++) {
		buf = get_write_buf(adp_acm_dev);
		if (buf == NULL) {
			adp_acm_fail(dev_id, "get write buf failed\n");
			return -ENOMEM;
		}

		ret = write_async(handler, buf, size);
		if (ret) {
			adp_acm_fail(dev_id, "async write failed %d\n", ret);
			ret_write_buf(adp_acm_dev, buf);
			return ret;
		}
		adp_acm_log(dev_id, "async write done %u\n", i);
	}

	return ret;
}

void adp_acm_test_read(u32 dev_id, int async_read)
{
	void *handler = NULL;

	if (dev_id >= UDI_USB_ACM_MAX) {
		adp_acm_fail(dev_id, "illegal dev_id\n");
		return;
	}

	if (dev_id == UDI_USB_ACM_MODEM) {
		adp_acm_fail(dev_id, "modem port!\n");
		return;
	}

	handler = adp_acm_devices[dev_id].handler;
	if (handler == NULL) {
		adp_acm_fail(dev_id, "not opened\n");
		return;
	}

	adp_acm_devices[dev_id].async_read = !!(async_read);
	adp_acm_log(dev_id, "async_read %d\n", adp_acm_devices[dev_id].async_read);
}

void adp_acm_test_loopback(u32 dev_id, int loop_back)
{
	struct adp_acm_device *adp_acm_dev = NULL;

	if (dev_id >= UDI_USB_ACM_MAX) {
		adp_acm_fail(dev_id, "illegal dev_id\n");
		return;
	}

	if (dev_id == UDI_USB_ACM_MODEM) {
		adp_acm_fail(dev_id, "modem port!\n");
		return;
	}

	adp_acm_dev = &adp_acm_devices[dev_id];
	if (adp_acm_dev == NULL)
		return;

	adp_acm_dev->loop_back = !!loop_back;
}

unsigned long adp_acm_test_total_read_len(u32 dev_id)
{
	struct adp_acm_device *adp_acm_dev = NULL;

	if (dev_id >= UDI_USB_ACM_MAX) {
		adp_acm_fail(dev_id, "illegal dev_id\n");
		return 0;
	}

	if (dev_id == UDI_USB_ACM_MODEM) {
		adp_acm_fail(dev_id, "modem port!\n");
		return 0;
	}

	adp_acm_dev = &adp_acm_devices[dev_id];
	if (adp_acm_dev == NULL)
		return 0;

	adp_acm_log(dev_id, "total_read_len %lu\n", adp_acm_dev->total_read_len);
	adp_acm_log(dev_id, "total_write_len %lu\n", adp_acm_dev->total_write_len);

	return adp_acm_dev->total_read_len;
}

enum ACM_MODEM_SIG_TYPE {
	ACM_MODEM_SIG_DSR,
	ACM_MODEM_SIG_DCD,
	ACM_MODEM_SIG_RING,
	ACM_MODEM_SIG_CTS_RECV_EN,
	ACM_MODEM_SIG_CTS_RECV_DIS,
	ACM_MODEM_SIG_WRITE_ALL,
	ACM_MODE_SIG_MAX
};

void *p_modem_handle;
#define SKB_BUF_SIZE 2048
static struct sk_buff *g_loop_skb;
static struct sk_buff *g_write_skb;

int bsp_usb_st_acm_modem_clean_cb(void)
{
	if (bsp_acm_ioctl(p_modem_handle, ACM_IOCTL_SET_READ_CB, NULL)) {
		pr_err("ACM_IOCTL_SET_READ_CB fail\n");
		return -1;
	}

	if (bsp_acm_ioctl(p_modem_handle, ACM_IOCTL_SET_FREE_CB, NULL)) {
		pr_err("ACM_IOCTL_SET_FREE_CB fail\n");
		return -1;
	}

	if (bsp_acm_ioctl(p_modem_handle,
		ACM_MODEM_IOCTL_SET_MSC_READ_CB, NULL)) {
		pr_err("ACM_MODEM_IOCTL_SET_MSC_READ_CB fail\n");
		return -1;
	}

	if (bsp_acm_ioctl(p_modem_handle,
		ACM_MODEM_IOCTL_SET_REL_IND_CB, NULL)) {
		pr_err("ACM_MODEM_IOCTL_SET_REL_IND_CB fail\n");
		return -1;
	}

	return 0;
}

static void modem_write_done_cb(void *send_skb)
{
	dev_kfree_skb((struct sk_buff *)send_skb);
}

int bsp_usb_st_acm_modem_close(void)
{
	int ret;

	if (g_write_skb != NULL) {
		dev_kfree_skb(g_write_skb);
		g_write_skb = NULL;
	}

	if (g_loop_skb != NULL) {
		dev_kfree_skb(g_loop_skb);
		g_loop_skb = NULL;
	}

	ret = bsp_acm_close(p_modem_handle);
	if (ret < 0) {
		pr_err("%s: return %d\n", __func__, ret);
		return -1;
	}
	p_modem_handle = NULL;
	return 0;
}

int bsp_usb_st_acm_modem_open(void)
{
	if (p_modem_handle != NULL) {
		pr_err("%s: %pK already open\n", __func__, p_modem_handle);
		return -1;
	}
	p_modem_handle = bsp_acm_open(UDI_ACM_MODEM_ID);
	if (IS_ERR(p_modem_handle)) {
		pr_err("%s: %pK\n", __func__, p_modem_handle);
		p_modem_handle = NULL;
		return -1;
	}

	if (g_loop_skb == NULL) {
		g_loop_skb = alloc_skb(SKB_BUF_SIZE, GFP_KERNEL);
		if (g_loop_skb == NULL) {
			(void)bsp_usb_st_acm_modem_close();
			return -ENOMEM;
		}
	}

	if (g_write_skb == NULL) {
		g_write_skb = alloc_skb(SKB_BUF_SIZE, GFP_KERNEL);
		if (g_write_skb == NULL) {
			(void)bsp_usb_st_acm_modem_close();
			return -ENOMEM;
		}
	}

	if (bsp_usb_st_acm_modem_clean_cb()) {
		(void)bsp_usb_st_acm_modem_close();
		pr_err("clean cb fail!\n");
		return -1;
	}

	if (bsp_acm_ioctl(p_modem_handle,
		ACM_IOCTL_SET_FREE_CB, modem_write_done_cb)) {
		(void)bsp_usb_st_acm_modem_close();
		pr_err("acm_modem:ACM_IOCTL_SET_FREE_CB fail\n");
		return -1;
	}

	return 0;
}

int bsp_usb_st_acm_modem_signal(enum ACM_MODEM_SIG_TYPE sig)
{
	int status;
	MODEM_MSC_STRU st_modem_msc = {0};

	switch (sig) {
	case ACM_MODEM_SIG_DSR:
		st_modem_msc.OP_Dsr = 1;
		st_modem_msc.ucDsr = 1;
		break;
	case ACM_MODEM_SIG_DCD:
		st_modem_msc.OP_Dcd = 1;
		st_modem_msc.ucDcd = 1;
		break;
	case ACM_MODEM_SIG_RING:
		st_modem_msc.OP_Ri = 1;
		st_modem_msc.ucRi = 1;
		break;
	case ACM_MODEM_SIG_CTS_RECV_EN:
		st_modem_msc.OP_Cts = 1;
		st_modem_msc.ucCts = 1;
		break;
	case ACM_MODEM_SIG_CTS_RECV_DIS:
		st_modem_msc.OP_Cts = 1;
		st_modem_msc.ucCts = 0;
		break;
	case ACM_MODEM_SIG_WRITE_ALL:
		st_modem_msc.OP_Dsr = 1;
		st_modem_msc.ucDsr = 1;
		st_modem_msc.OP_Dcd = 1;
		st_modem_msc.ucDcd = 1;
		st_modem_msc.OP_Ri = 1;
		st_modem_msc.ucRi = 1;
		st_modem_msc.OP_Cts = 1;
		st_modem_msc.ucCts = 1;
		break;
	default:
		break;
	}

	if (p_modem_handle == NULL)
		return -1;
	status = bsp_acm_ioctl(p_modem_handle, ACM_MODEM_IOCTL_MSC_WRITE_CMD,
			&st_modem_msc);
	if (status < 0) {
		pr_err("acm_modem_signal: bsp_acm_ioctl(MSC_WRITE_CMD)<0\n");
		return status;
	}

	pr_err("Send To PC Successfully!\n");

	return 0;
}

int bsp_usb_st_acm_modem_dsr(void)
{
	return bsp_usb_st_acm_modem_signal(ACM_MODEM_SIG_DSR);
}

int bsp_usb_st_acm_modem_dcd(void)
{
	return bsp_usb_st_acm_modem_signal(ACM_MODEM_SIG_DCD);
}

int bsp_usb_st_acm_modem_ring(void)
{
	return  bsp_usb_st_acm_modem_signal(ACM_MODEM_SIG_RING);
}

int bsp_usb_st_acm_modem_recv_en(void)
{
	return  bsp_usb_st_acm_modem_signal(ACM_MODEM_SIG_CTS_RECV_EN);
}

int bsp_usb_st_acm_modem_recv_dis(void)
{
	return  bsp_usb_st_acm_modem_signal(ACM_MODEM_SIG_CTS_RECV_DIS);
}

int bsp_usb_st_acm_modem_write_signal_all(void)
{
	return  bsp_usb_st_acm_modem_signal(ACM_MODEM_SIG_WRITE_ALL);
}

int bsp_usb_modem_st_write(const char *header, unsigned int len)
{
	struct sk_buff *send_skb = NULL;
	int ret;
	ACM_WR_ASYNC_INFO send_info;
	unsigned int value;

	if (header == NULL || len == 0)
		return -EINVAL;

	if (len > SKB_BUF_SIZE) {
		pr_err("acm[acm_modem] len can't be bigger than SKB_BUF_SIZE\n");
		return -EINVAL;
	}

	send_skb = alloc_skb(SKB_BUF_SIZE, GFP_KERNEL);
	if (send_skb == NULL) {
		pr_err("alloc g_write_skb fail!\n");
		return -ENOMEM;
	}
	value = header[0];
	skb_trim(send_skb, 0);
	memset((void *)send_skb->data, value, len);
	skb_put(send_skb, len);

	send_info.pVirAddr = (char *)send_skb;
	send_info.pPhyAddr = NULL;
	send_info.u32Size = 0;
	send_info.pDrvPriv = NULL;

	if (p_modem_handle == NULL) {
		dev_kfree_skb(send_skb);
		return -1;
	}
	ret = bsp_acm_ioctl(p_modem_handle, ACM_IOCTL_WRITE_ASYNC, &send_info);
	if (ret != 0) {
		pr_err("acm[acm_modem] ACM_IOCTL_WRITE_ASYNC fail %d\n", ret);
		dev_kfree_skb(send_skb);
	}

	return ret;
}

void bsp_usb_modem_st_multi_write(unsigned int count,
			const char *header, unsigned int len)
{
	unsigned int i;
	int ret;

	if (count > MAX_MULTI_COUNT)
		return;
	for (i = 0; i < count; i++) {
		ret = bsp_usb_modem_st_write(header, len);
		if (ret) {
			pr_err("acm[acm_modem] %s fail %d\n", __func__, ret);
			break;
		}
	}
}

int bsp_usb_modem_st_set_do_copy(int enable)
{
	int ret;

	if (p_modem_handle == NULL)
		return -1;
	ret = bsp_acm_ioctl(p_modem_handle, ACM_IOCTL_WRITE_DO_COPY, &enable);
	if (ret != 0)
		pr_err("acm[acm_modem] %s fail %d\n", __func__, ret);

	return ret;
}

void modem_loop_free_buff(char *pviraddr)
{
	pr_err("acm[acm_modem] %s OK\n", __func__);
}

static unsigned int write_egain_num;
void modem_loop_recv_cb(void)
{
	ACM_WR_ASYNC_INFO mem_info = {0};
	ACM_WR_ASYNC_INFO send_info = {0};
	struct sk_buff *recv_skb = NULL;
	int ret;

	if (p_modem_handle == NULL)
		return;
	ret = bsp_acm_ioctl(p_modem_handle, ACM_IOCTL_GET_RD_BUFF, &mem_info);
	if (ret < 0)
		pr_err("acm[acm_modem] ACM_IOCTL_GET_RD_BUFF fail [ret=%d]\n", ret);

	if (mem_info.pVirAddr == NULL) {
		pr_err("acm[acm_modem] mem_info.pVirAddr == NULL\n");
		return;
	}

	recv_skb = (struct sk_buff *)mem_info.pVirAddr;

	if (g_loop_skb == NULL) {
		pr_err("acm[acm_modem] firstly do ecall bsp_usb_st_acm_modem_open\n");
		return;
	}

	if (recv_skb->len > g_loop_skb->truesize) {
		dev_kfree_skb(g_loop_skb);
		pr_err("realloc g_loop_skb\n");
		g_loop_skb = alloc_skb(recv_skb->len, GFP_KERNEL);
		if (g_loop_skb == NULL)
			return;
	}

	skb_trim(g_loop_skb, 0);
	memcpy((void *)g_loop_skb->data, (void *)recv_skb->data, recv_skb->len);
	skb_put(g_loop_skb, recv_skb->len);

	ret = bsp_acm_ioctl(p_modem_handle, ACM_IOCTL_RETURN_BUFF, &mem_info);
	if (ret < 0)
		pr_err("acm[acm_modem] ACM_IOCTL_RETURN_BUFF fail [ret=%d]\n", ret);

	send_info.pVirAddr = (char *)g_loop_skb;
	send_info.pPhyAddr = NULL;
	send_info.u32Size = 0;

	ret = bsp_acm_ioctl(p_modem_handle, ACM_IOCTL_WRITE_ASYNC, &send_info);
	if (ret == -EAGAIN) {
		pr_err("acm[acm_modem] ACM_IOCTL_WRITE_ASYNC EAGAIN\n");
		write_egain_num++;
		if (write_egain_num == ACM_IOCTL_ASYNC_LIMIT) {
			ret = bsp_acm_ioctl(p_modem_handle, UDI_IOCTL_SET_READ_CB, NULL);
			if (ret < 0)
				pr_err("acm[acm_modem] UDI_IOCTL_SET_READ_CB fail [ret=%d]\n", ret);
		}
	} else if (ret < 0) {
		pr_err("acm[acm_modem] ACM_IOCTL_WRITE_ASYNC fail [ret=%d]\n", ret);
	} else {
		write_egain_num = 0;
	}
}

int bsp_usb_modem_st_loop(void)
{
	if (bsp_acm_ioctl(p_modem_handle, UDI_IOCTL_SET_READ_CB,
					modem_loop_recv_cb)) {
		pr_err("UDI_IOCTL_SET_READ_CB fail\n");
		return -1;
	}

	if (bsp_acm_ioctl(p_modem_handle, ACM_IOCTL_SET_FREE_CB,
					modem_loop_free_buff)) {
		pr_err("ACM_IOCTL_SET_FREE_CB fail\n");
		return -1;
	}

	return 0;
}
