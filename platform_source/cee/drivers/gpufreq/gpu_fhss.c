/*
 * gpufreq.c
 *
 * Frequency Hopped Spread Spectrum for gpu freq
 *
 * Copyright (c) 2017-2020 Huawei Technologies Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#include <linux/errno.h>
#include <linux/module.h>
#include <linux/devfreq.h>
#include <linux/math64.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/pm.h>
#include <linux/mutex.h>
#include <linux/list.h>
#include <linux/of.h>
#include <securec.h>

#include "gpu_fhss.h"
#include <platform_include/basicplatform/linux/ipc_rproc.h>

#define KHZ		1000
#define LOCAL_BUF_MAX		128

#ifdef CONFIG_GPU_FHSS

#define GPU_OPP_EN_IPC_CMD		0x00030310
#define GPU_OPP_DIS_IPC_CMD		0x00030313
#define PROPER_NAME	"fhss"
#define ELEMS_TYPE	unsigned int
#define ELEMS_SIZE	sizeof(ELEMS_TYPE)
#define GPU_FHSS_NOTIFY_RETRY_TIMES	3

struct fhss_map {
	ELEMS_TYPE device_id;
	ELEMS_TYPE freq; /* KHz */
};

static struct fhss_map *g_fhss_map;
static int g_fhss_size;

static int find_fhss_offset(unsigned int device_id)
{
	int i;

	if (g_fhss_map == NULL || g_fhss_size == 0)
		return -EINVAL;

	for (i = 0; i < g_fhss_size; i++) {
		if (device_id == g_fhss_map[i].device_id)
			return i;
	}

	return -EINVAL;
}

/* freq is KHz */
static unsigned int get_freq_by_offset(int offset)
{
	return g_fhss_map[offset].freq;
}

int send_setting_to_lpmcu(u32 cmd, u32 data)
{
	int err;
	rproc_msg_t ack_buf[2] = {0};
	rproc_msg_t tx_buf[2] = {0};

	tx_buf[0] = cmd;
	tx_buf[1] = data;

	err = RPROC_SYNC_SEND(IPC_ACPU_LPM3_MBX_2,
			      tx_buf, sizeof(tx_buf) / sizeof(rproc_msg_t),
			      ack_buf, sizeof(ack_buf) / sizeof(rproc_msg_t));
	if (err != 0 || ack_buf[0] != tx_buf[0])
		return -EINVAL;

	return 0;
}

int hisi_send_fhss_notify(u32 cmd, u32 freq_mhz)
{
	int retry = GPU_FHSS_NOTIFY_RETRY_TIMES;
	int ret = -EINVAL;

	if (cmd != GPU_OPP_EN_IPC_CMD && cmd != GPU_OPP_DIS_IPC_CMD)
		return -EINVAL;

	while (retry > 0) {
		ret = send_setting_to_lpmcu(cmd, freq_mhz);
		if (ret == 0)
			break;
		retry--;
	}

	if (retry == 0)
		pr_err("%s: Send CMD to lpmcu failed, ret = %d\n",
		       __func__, ret);

	return ret;
}

int cmd_parse(char *para_cmd, char *argv[], int max_args)
{
	int para_id = 0;

	while (*para_cmd != '\0') {
		if (para_id >= max_args)
			break;
		while (*para_cmd == ' ')
			para_cmd++;

		if (*para_cmd == '\0')
			break;

		argv[para_id] = para_cmd;
		para_id++;

		while ((*para_cmd != ' ') && (*para_cmd != '\0'))
			para_cmd++;

		if (*para_cmd == '\0')
			break;

		*para_cmd = '\0';
		para_cmd++;
	}

	return para_id;
}

static ssize_t fhss_store(struct device *dev,
			  struct device_attribute *attr,
			  const char *buf,
			  size_t count)
{
	unsigned int device_id, case_id;
	unsigned long freq_khz;
	char local_buf[LOCAL_BUF_MAX] = {0};
	char *argv[2] = {0};
	int argc, ret;
	int offset;

	if (count >= sizeof(local_buf))
		return -ENOMEM;

	ret = strncpy_s(local_buf, LOCAL_BUF_MAX, buf,
			min_t(size_t, sizeof(local_buf) - 1, count));
	if (ret != EOK) {
		dev_err(dev, "failed to copy string\n");
		return ret;
	}

	argc = cmd_parse(local_buf, argv, ARRAY_SIZE(argv));
	if (argc != 2) {
		dev_err(dev, "error, only surport two para\n");
		return -EINVAL;
	}

	ret = sscanf_s(argv[0], "%u", &device_id);
	if (ret != 1) {
		dev_err(dev, "parse device id error %s\n", argv[0]);
		return -EINVAL;
	}

	ret = sscanf_s(argv[1], "%u", &case_id);
	if (ret != 1) {
		dev_err(dev, "parse case id error %s\n", argv[1]);
		return -EINVAL;
	}

	offset = find_fhss_offset(device_id);
	if (offset < 0) {
		dev_err(dev, "%s:device id%d not exist\n", __func__, device_id);
		return -EINVAL;
	}
	freq_khz = get_freq_by_offset(offset);

	if (case_id == 0) {
		ret = dev_pm_opp_enable(dev, freq_khz * KHZ);
		ret += hisi_send_fhss_notify(GPU_OPP_EN_IPC_CMD, freq_khz / KHZ);
	} else {
		ret = dev_pm_opp_disable(dev, freq_khz * KHZ);
		ret += hisi_send_fhss_notify(GPU_OPP_DIS_IPC_CMD, freq_khz / KHZ);
	}

	if (ret != 0) {
		dev_err(dev, "enable or disable opp fail\n");
		return ret;
	}

	return count;
}

static ssize_t fhss_show(struct device *dev,
			 struct device_attribute *attr,
			 char *buf)
{
	int ret;
	int count;
	int i;

	ret = snprintf_s(buf, PAGE_SIZE, PAGE_SIZE - 1, "deviceID  freq(KHz)\n");
	if (ret < 0)
		return 0;

	count = ret;
	if ((unsigned int)count >= PAGE_SIZE)
		goto err_ret;

	for (i = 0; i < g_fhss_size; i++) {
		ret = snprintf_s(buf + count, (PAGE_SIZE - count),
				 (PAGE_SIZE - count - 1),
				 "%d <--> %d\n", g_fhss_map[i].device_id,
				 g_fhss_map[i].freq);
		if (ret >= ((int)PAGE_SIZE - count) || ret < 0)
			break;

		count += ret;
		if ((unsigned int)count >= PAGE_SIZE)
			goto err_ret;
	}

err_ret:
	return count;
}

static DEVICE_ATTR(fhss, 0640, fhss_show, fhss_store);

static struct attribute *dev_entries[] = {
	&dev_attr_fhss.attr,
	NULL,
};

static struct attribute_group dev_attr_group = {
	.name = "fhss",
	.attrs = dev_entries,
};

int fhss_init(struct device *dev)
{
	struct device_node *np = NULL;
	int count;
	int ret;

	np = dev->of_node;
	if (np == NULL) {
		dev_err(dev, "%s: %s have not device node\n",
			__func__, dev_name(dev));
		return -EINVAL;
	}

	count = of_property_count_elems_of_size(np, PROPER_NAME, ELEMS_SIZE);
	if (count <= 0) {
		dev_info(dev, "%s: unsurport fhss\n", __func__);
		return 0;
	}
	if ((count % 2) != 0) {
		dev_err(dev, "%s: fhss format error\n", __func__);
		return -EINVAL;
	}

	g_fhss_map = devm_kzalloc(dev, count * ELEMS_SIZE, GFP_KERNEL);
	if (g_fhss_map == NULL) {
		dev_err(dev, "%s: fhss alloc memory fail\n", __func__);
		return -ENOMEM;
	}
	g_fhss_size = count / 2;

	ret = of_property_read_u32_array(np, PROPER_NAME,
					 (u32 *)g_fhss_map, count);
	if (ret != 0) {
		dev_err(dev, "%s: read fhss fail\n", __func__);
		return -EINVAL;
	}

	ret = sysfs_create_group(&(dev->kobj), &dev_attr_group);
	if (ret != 0)
		dev_err(dev, "%s: sysfs create opp group err %d\n",
			__func__, ret);

	return ret;
}

#endif
MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("gpu freq fhss");
MODULE_AUTHOR("Huawei Technologies Co., Ltd.");
