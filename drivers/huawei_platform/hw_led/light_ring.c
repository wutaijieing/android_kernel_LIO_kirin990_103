/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
 */
#include <linux/completion.h>
#include <linux/delay.h>
#include <linux/kthread.h>
#include <linux/leds.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/timer.h>
#include <huawei_platform/log/hw_log.h>
#include <securec.h>

#include "light_ring.h"

#ifdef HWLOG_TAG
#undef HWLOG_TAG
#endif
#define HWLOG_TAG light_ring
HWLOG_REGIST();

#define LIGHT_RING_DEBUG 1
#define LIGHT_RING_NODE_NAME "light_ring"

enum TypeId {
	TYPE_LOOP = 0x3f3f3f00,
	TYPE_FRAMES_NUM,
	TYPE_DATA,
	TYPE_DELAY,
	TYPE_END
};
#define I2C_RETRIES 3
#define I2C_RETRY_DELAY 1
#define VALUE_HEX 16
#define BYTE_SUM 8
#define MAX_TYPE_SIZE 4
#define MAX_TYPE_LENGTH 4
#define TYPE_LOOP_LENGTH 1
#define TYPE_FRAMES_NUM_LENGTH 4
#define TYPE_DELAY_LENGTH 4

static DEFINE_MUTEX(g_light_ring_lock);
static DECLARE_COMPLETION(g_frames_complete);

static struct task_struct *g_fresh_frames;
light_ring_ext_func g_light_ring_ext_func_handle = {0};

light_ring_ext_func *light_ring_ext_func_get(void)
{
	return &g_light_ring_ext_func_handle;
}

static uint32_t u8_stream_to_u32(const uint8_t *data, int32_t length)
{
	int32_t i;
	uint32_t result = 0;

	for (i = 0; i < length; i++)
		result |= data[i] << (BYTE_SUM * i);
	return result;
}

static bool get_check_tlv(const char *buf, int32_t *offset, int32_t type, int32_t length)
{
	int32_t tmp = *offset;
	int32_t t, l;

	t = u8_stream_to_u32(buf + tmp, sizeof(uint32_t));
	tmp += sizeof(uint32_t);
	l = u8_stream_to_u32(buf + tmp, sizeof(uint32_t));
	tmp += sizeof(uint32_t);
	*offset = tmp;
	if ((t != type) || (l != length)) {
		hwlog_err("%s, buf invalid: %x != %x, %d != %d", __func__, type, t, length, l);
		return false;
	}
	return true;
}

static int32_t frames_parse_update(struct light_ring_data *data, int32_t offset)
{
	int32_t i;
	int32_t delay = 0;
	uint8_t *frames_reading_buf = data->frames_buf[data->frames_reading_index];

	for (i = 0; i < data->frames_num; i++) {
		mutex_lock(&g_light_ring_lock);
		if (data->frames_update) {
			hwlog_info("%s: frames update, interrupt", __func__);
			mutex_unlock(&g_light_ring_lock);
			return -EINVAL;
		}
		mutex_unlock(&g_light_ring_lock);

		// get and check TYPE_DATA, fresh frame
		if (!get_check_tlv(frames_reading_buf, &offset, TYPE_DATA, LED_NUM))
			return -EINVAL;
		if (memcpy_s(data->led_color_data, LED_NUM, frames_reading_buf + offset,
			LED_NUM) != EOK) {
			hwlog_err("%s:memcpy_s led_color_data error", __func__);
			return -EINVAL;
		}
		offset += LED_NUM;
		if (!g_light_ring_ext_func_handle.chip_update)
			return -EINVAL;
		(void)((g_light_ring_ext_func_handle.chip_update)(data));

		// get and check TYPE_DELAY, delay
		if (!get_check_tlv(frames_reading_buf, &offset, TYPE_DELAY, TYPE_DELAY_LENGTH))
			return -EINVAL;
		delay = u8_stream_to_u32(frames_reading_buf + offset, sizeof(uint32_t));
		offset += TYPE_DELAY_LENGTH;
		hwlog_info("%s: fresh frame%d done, delay=%d", __func__, i, delay);
		msleep(delay);
	}
	return 0;
}

static int32_t fresh_frames_thread(void *data)
{
	int32_t offset;
	struct light_ring_data *lr_data = (struct light_ring_data *)data;
	uint8_t *frames_reading_buf = NULL;

	while (!kthread_should_stop()) {
		wait_for_completion(&g_frames_complete);
		hwlog_info("%s: begin to fresh frames", __func__);

		offset = 0;
		mutex_lock(&g_light_ring_lock);
		lr_data->frames_update = false;
		lr_data->frames_reading_index = lr_data->frames_idle_index++;
		frames_reading_buf = lr_data->frames_buf[lr_data->frames_reading_index];
		if (lr_data->frames_idle_index >= FRAMES_BUF_NUM)
			lr_data->frames_idle_index = 0;
		mutex_unlock(&g_light_ring_lock);

		// get and check TYPE_LOOP
		if (!get_check_tlv(frames_reading_buf, &offset, TYPE_LOOP, TYPE_LOOP_LENGTH))
			goto err;
		lr_data->frames_loop = frames_reading_buf[offset++] == 0 ? false : true;
		hwlog_info("%s: lr_data->frames_loop=%d", __func__, lr_data->frames_loop);

		// get and check TYPE_FRAMES_NUM
		if (!get_check_tlv(frames_reading_buf, &offset, TYPE_FRAMES_NUM,
			TYPE_FRAMES_NUM_LENGTH))
			goto err;
		lr_data->frames_num = u8_stream_to_u32(frames_reading_buf + offset, sizeof(uint32_t));
		offset += TYPE_FRAMES_NUM_LENGTH;
		hwlog_info("%s: lr_data->frames_num=%d", __func__, lr_data->frames_num);

		// fresh frames
		do {
			if (frames_parse_update(lr_data, offset) != 0)
				break;
		} while (lr_data->frames_loop);
err:
		(void)memset_s(frames_reading_buf, sizeof(FRAMES_BUF_LEN), 0,
			sizeof(FRAMES_BUF_LEN));
		hwlog_info("%s: fresh frames done", __func__);
	}
	return 0;
}

#ifdef LIGHT_RING_DEBUG
static ssize_t leds_color_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int32_t ret;
	struct i2c_client *i2c = to_i2c_client(dev);
	struct light_ring_data *data = i2c_get_clientdata(i2c);

	ret = memcpy_s(buf, PAGE_SIZE, data->led_color_data, LED_NUM);
	if (ret != EOK) {
		hwlog_err("%s:memcpy_s error[%d]", __func__, ret);
		return 0;
	}
	hwlog_info("%s: cat %s", __func__, buf);
	return LED_NUM;
}

/*
 * Usage: Write 24 hexadecimal numbers to /sys/class/leds/light_ring/leds_color
 * E.g: echo -e -n "\xff\x00\x00\x00\xff\x00\x00\x00\xff\xff\xff\xff\x34 \
 *                 \x9a\x5c\x9a\x34\x5c\x34\x5c\x9a\x5c\x34 \
 *                 \x9a" > /sys/class/leds/light_ring/leds_color
 */
static ssize_t leds_color_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t len)
{
	int32_t i;
	struct i2c_client *i2c = to_i2c_client(dev);
	struct light_ring_data *data = i2c_get_clientdata(i2c);

	hwlog_info("%s: echo buf=%s, len=%u", __func__, buf, len);
	if (len > LED_NUM)
		return -EINVAL;
	for (i = 0; i < LED_NUM; i++)
		data->led_color_data[i] = (uint8_t)buf[i];
	if (!g_light_ring_ext_func_handle.chip_update)
		return -EINVAL;
	return ((g_light_ring_ext_func_handle.chip_update)(data) < 0) ? -EINVAL : len;
}

static ssize_t leds_brightness_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int32_t ret;
	struct i2c_client *i2c = to_i2c_client(dev);
	struct light_ring_data *data = i2c_get_clientdata(i2c);

	ret = memcpy_s(buf, PAGE_SIZE, data->led_brightness_data, LED_NUM);
	if (ret != EOK) {
		hwlog_err("%s:memcpy_s error[%d]", __func__, ret);
		return 0;
	}
	hwlog_info("%s: cat %s", __func__, buf);
	return LED_NUM;
}

/*
 * Usage: Write 24 hexadecimal numbers to /sys/class/leds/light_ring/leds_brightness
 * E.g: echo -e -n "\xff\x00\x00\x00\xff\x00\x00\x00\xff\xff\xff\xff\x34 \
 *                 \x9a\x5c\x9a\x34\x5c\x34\x5c\x9a\x5c\x34 \
 *                 \x9a" > /sys/class/leds/light_ring/leds_brightness
 */
static ssize_t leds_brightness_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t len)
{
	uint32_t i;
	struct i2c_client *i2c = to_i2c_client(dev);
	struct light_ring_data *data = i2c_get_clientdata(i2c);

	hwlog_info("%s: echo buf=%s, len=%u", __func__, buf, len);
	if (len > LED_NUM)
		return -EINVAL;
	for (i = 0; i < LED_NUM; i++)
		data->led_brightness_data[i] = (uint8_t)buf[i];
	if (!g_light_ring_ext_func_handle.chip_update)
		return -EINVAL;
	return ((g_light_ring_ext_func_handle.chip_update)(data) < 0) ? -EINVAL : len;
}
#endif

/*
 * Usage: Write frame TLV to /sys/class/leds/light_ring/frames
 * TLV format: TYPE_LOOP       : T1(uint8_t * 4) L1(uint8_t * 4) V1(uint8_t * L1)
 *             TYPE_FRAMES_NUM : T2(uint8_t * 4) L2(uint8_t * 4) V2(uint8_t * L2)
 *             TYPE_DATA       : T3(uint8_t * 4) L3(uint8_t * 4) V3(uint8_t * L3)
 *             TYPE_DELAY      : T4(uint8_t * 4) L4(uint8_t * 4) V4(uint8_t * L3) ...
 */
static ssize_t frames_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t len)
{
	int32_t ret;
	struct i2c_client *i2c = to_i2c_client(dev);
	struct light_ring_data *data = i2c_get_clientdata(i2c);
	uint8_t *frames_writing_buf = data->frames_buf[data->frames_idle_index];

	hwlog_info("%s: echo buf=%s, len=%u", __func__, buf, len);
	(void)memset_s(frames_writing_buf, sizeof(FRAMES_BUF_LEN), 0, sizeof(FRAMES_BUF_LEN));
	ret = memcpy_s(frames_writing_buf, FRAMES_BUF_LEN, buf, len);
	if (ret != EOK) {
		hwlog_err("%s:memcpy_s error, ret[%d]", __func__, ret);
		return -EINVAL;
	}
	mutex_lock(&g_light_ring_lock);
	data->frames_update = true;
	data->frames_buf[data->frames_idle_index] = frames_writing_buf;
	mutex_unlock(&g_light_ring_lock);
	complete(&g_frames_complete);
	hwlog_info("%s: done", __func__);
	return len;
}

#ifdef LIGHT_RING_DEBUG
static DEVICE_ATTR(leds_brightness, S_IWUSR | S_IRUGO, leds_brightness_show,
	leds_brightness_store);
static DEVICE_ATTR(leds_color, S_IWUSR | S_IRUGO, leds_color_show, leds_color_store);
#endif
static DEVICE_ATTR(frames, S_IWUSR | S_IRUGO, NULL, frames_store);

static struct attribute *light_ring_attributes[] = {
	&dev_attr_leds_brightness.attr,
	&dev_attr_leds_color.attr,
	&dev_attr_frames.attr,
	NULL,
};

static struct attribute_group light_ring_attrs = {
	.attrs = light_ring_attributes,
};

// returning negative errno else a data byte received from the device
int32_t light_ring_i2c_read(const struct i2c_client *i2c, unsigned char reg_addr)
{
	int32_t ret;
	uint8_t cnt = 0;

	if (!i2c) {
		hwlog_err("%s:invalid args", __func__);
		return -EINVAL;
	}
	while (cnt < I2C_RETRIES) {
		ret = i2c_smbus_read_byte_data(i2c, reg_addr);
		if (ret < 0)
			hwlog_err("%s: i2c_read cnt = %u, ret = %d", __func__, cnt, ret);
		else
			break;
		cnt++;
		msleep(I2C_RETRY_DELAY);
	}
	return ret;
}

// returning negative errno else zero on success
int32_t light_ring_i2c_write_byte(const struct i2c_client *i2c, unsigned char reg_addr,
	unsigned char reg_data)
{
	int32_t ret;
	uint8_t cnt = 0;

	if (!i2c) {
		hwlog_err("%s:invalid args", __func__);
		return -EINVAL;
	}
	while (cnt < I2C_RETRIES) {
		ret = i2c_smbus_write_byte_data(i2c, reg_addr, reg_data);
		if (ret != 0)
			hwlog_err("%s: i2c_write failed, i2c = 0x%x, i2c_name = %s, \
reg_addr = 0x%x, reg_data = 0x%x, cnt = %u, ret = 0x%x",
				__func__, i2c->addr, i2c->name, reg_addr, reg_data, cnt, ret);
		else
			break;
		cnt++;
		msleep(I2C_RETRY_DELAY);
	}
	return ret;
}

// returning negative errno else the number of bytes written
int32_t light_ring_i2c_write_bytes(const struct i2c_client *i2c, unsigned char reg_addr,
	unsigned char *buf, uint32_t len)
{
	int32_t ret;
	unsigned char *data = NULL;

	if (!i2c || !buf || len == 0) {
		hwlog_err("%s:invalid args", __func__);
		return -EINVAL;
	}
	data = kmalloc(len + 1, GFP_KERNEL);
	if (!data)
		return -ENOMEM;
	data[0] = reg_addr;
	ret = memcpy_s(&data[1], len, buf, len);
	if (ret != EOK) {
		hwlog_err("%s:memcpy_s error, ret[%d]", __func__, ret);
		kfree(data);
		return -EINVAL;
	}

	ret = i2c_master_send(i2c, data, len + 1);
	if (ret < 0)
		hwlog_err("%s:i2c master send failed, i2c = 0x%x, i2c_name = %s, \
reg_addr = 0x%x, ret =0x%x", __func__, i2c->addr, i2c->name, reg_addr, ret);
	kfree(data);
	return ret;
}

int32_t light_ring_init_data(struct i2c_client *i2c, struct light_ring_data *data)
{
	int32_t i;

	if (!i2c || !data) {
		hwlog_err("%s:invalid args", __func__);
		return -EINVAL;
	}
	for (i = 0; i < FRAMES_BUF_NUM; i++) {
		data->frames_buf[i] = vmalloc(FRAMES_BUF_LEN);
		if (!data->frames_buf[i])
			return -ENOMEM;
		(void)memset_s(data->frames_buf[i], sizeof(FRAMES_BUF_LEN), 0,
			sizeof(FRAMES_BUF_LEN));
	}
	data->frames_idle_index = 0;
	data->i2c = i2c;
	data->cdev.name = LIGHT_RING_NODE_NAME;
	return 0;
}

void light_ring_deinit_data(struct light_ring_data *data)
{
	int32_t i;

	if (!data) {
		hwlog_err("%s:invalid args", __func__);
		return;
	}
	for (i = 0; i < FRAMES_BUF_NUM; i++) {
		if (data->frames_buf[i]) {
			vfree(data->frames_buf[i]);
			data->frames_buf[i] = NULL;
		}
	}
}

int32_t light_ring_register(struct light_ring_data *data)
{
	if (!data) {
		hwlog_err("%s:invalid args", __func__);
		return -EINVAL;
	}
	if (led_classdev_register(&data->i2c->dev, &data->cdev) != 0) {
		hwlog_err("%s: unable to register light ring", __func__);
		goto err_clsdev_register;
	}
	if (sysfs_create_group(&data->cdev.dev->kobj, &light_ring_attrs) != 0) {
		hwlog_err("%s: error creating light ring class dev", __func__);
		goto err_sysfs_create;
	}
	dev_set_drvdata(data->cdev.dev, data);

	g_fresh_frames = kthread_run(fresh_frames_thread, data, "fresh_frames");
	if (!g_fresh_frames) {
		hwlog_err("%s: fail to create the thread that fresh frames", __func__);
		goto err_kthread_run;
	}
	return 0;

err_kthread_run:
	sysfs_remove_group(&data->cdev.dev->kobj, &light_ring_attrs);
err_sysfs_create:
	led_classdev_unregister(&data->cdev);
err_clsdev_register:
	return -EINVAL;
}

void light_ring_unregister(struct light_ring_data *data)
{
	if (!data) {
		hwlog_err("%s:invalid args", __func__);
		return;
	}
	sysfs_remove_group(&data->cdev.dev->kobj, &light_ring_attrs);
	led_classdev_unregister(&data->cdev);
	if (!g_fresh_frames) {
		hwlog_err("%s: g_fresh_frames is invalid", __func__);
		return;
	}
	kthread_stop(g_fresh_frames);
}
