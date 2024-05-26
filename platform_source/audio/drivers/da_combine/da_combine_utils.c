/*
 * da_combine_utils.c
 *
 * da_combine_utils codec driver
 *
 * Copyright (c) 2014-2020 Huawei Technologies CO., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/platform_drivers/da_combine/da_combine_utils.h>

#include <linux/version.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/proc_fs.h>
#include <linux/vmalloc.h>
#include <linux/platform_drivers/da_combine/da_combine_resmgr.h>
#ifdef CONFIG_DFX_DEBUG_FS
#include <linux/debugfs.h>
#endif

#include "audio_log.h"
#include "asoc_adapter.h"
#ifdef CONFIG_SND_SOC_DA_COMBINE_V5
#include <linux/platform_drivers/da_combine/da_combine_v5.h>
#else
#define LOG_TAG "DA_combine_utils"
#endif


#ifdef CONFIG_DFX_DEBUG_FS
#define DA_COMBINE_DBG_PAGES 9
#define DA_COMBINE_DBG_SIZE_PAGE 0x1000

struct da_combine_ins {
	char ins;
	u32 reg;
	u32 val;
};

struct da_combine_ins g_codec_ins;
static struct snd_soc_component *g_snd_cdc;
static struct dentry *g_debug_dir;
static struct hi_cdc_ctrl *g_cdc_ctrl;
static struct da_combine_resmgr *g_resmgr;
spinlock_t da_combine_ins_lock;
#endif
static struct utils_config *g_utils_config;
static unsigned int g_cdc_type = DA_COMBINE_CODEC_TYPE_BUTT;
static struct snd_soc_component *g_snd_codec;

int da_combine_update_bits(struct snd_soc_component *codec, unsigned int reg,
	unsigned int mask, unsigned int value)
{
	int change;
	unsigned int old;
	unsigned int new;

	if (codec == NULL) {
		AUDIO_LOGE("parameter is null");
		return -EINVAL;
	}
	old = snd_soc_component_read32(codec, reg);
	new = (old & ~mask) | (value & mask);
	change = (old != new);
	if (change != 0)
		snd_soc_component_write_adapter(codec, reg, new);

	return change;
}

#ifdef CONFIG_DFX_DEBUG_FS

#ifdef ENABLE_DA_COMBINE_CODEC_DEBUG
static ssize_t da_combine_rr_read(struct file *file, char __user *user_buf,
	size_t count, loff_t *ppos)
{
	int len;
	char *kernel_buf = NULL;
	ssize_t byte_read;
	struct da_combine_ins m_codec_ins = {0};

	spin_lock(&da_combine_ins_lock);
	m_codec_ins = g_codec_ins;
	spin_unlock(&da_combine_ins_lock);

	if (user_buf == NULL) {
		AUDIO_LOGE("input error: user buf is NULL");
		return -EINVAL;
	}

	kernel_buf = kzalloc(DA_COMBINE_DBG_SIZE_PAGE, GFP_KERNEL);
	if (kernel_buf == NULL) {
		AUDIO_LOGE("kernel buf is null");
		return -ENOMEM;
	}

	switch (m_codec_ins.ins) {
	case 'r':
		snprintf(kernel_buf, DA_COMBINE_DBG_SIZE_PAGE, " %#08x: 0x%x\n",
			m_codec_ins.reg, snd_soc_component_read32(g_snd_cdc, m_codec_ins.reg));
		break;
	case 'w':
		snprintf(kernel_buf, DA_COMBINE_DBG_SIZE_PAGE, "write ok.\n");
		break;
	case 'n':
	default:
		snprintf(kernel_buf, DA_COMBINE_DBG_SIZE_PAGE, "error parameter.\n");
		break;
	}

	len = strlen(kernel_buf);
	snprintf(kernel_buf + len, DA_COMBINE_DBG_SIZE_PAGE - len, "<cat end>\n");
	len = strlen(kernel_buf);
	snprintf(kernel_buf + len, DA_COMBINE_DBG_SIZE_PAGE - len, "\n");

	byte_read = simple_read_from_buffer(user_buf, count, ppos, kernel_buf, strlen(kernel_buf));

	kfree(kernel_buf);
	kernel_buf = NULL;

	return byte_read;
}

static ssize_t write_register(const char *kernel_buf, const char __user *user_buf)
{
	int num;
	ssize_t ret = 0;
	struct da_combine_ins m_codec_ins = {0};

	switch (kernel_buf[0]) {
	case 'w':
		m_codec_ins.ins = 'w';
		num = sscanf(kernel_buf, "w 0x%x 0x%x", &m_codec_ins.reg, &m_codec_ins.val);
		if (num != 2) {
			AUDIO_LOGE("wrong parameter for cmd w");
			g_codec_ins.ins = 'n';
			ret = -EINVAL;
			break;
		}

		snd_soc_component_write_adapter(g_snd_cdc, m_codec_ins.reg, m_codec_ins.val);
		break;
	case 'r':
		m_codec_ins.ins = 'r';
		num = sscanf(kernel_buf, "r 0x%x", &m_codec_ins.reg);
		if (num != 1) {
			AUDIO_LOGE("wrong parameter for cmd r");
			ret = -EINVAL;
			break;
		}
		break;
	default:
		m_codec_ins.ins = 'n';
		AUDIO_LOGE("unknown cmd");
		ret = -EINVAL;
		break;
	}

	spin_lock(&da_combine_ins_lock);
	g_codec_ins = m_codec_ins;
	spin_unlock(&da_combine_ins_lock);

	return ret;
}

static ssize_t da_combine_rr_write(struct file *file, const char __user *user_buf,
	size_t count, loff_t *ppos)
{
	char *kernel_buf = NULL;
	ssize_t ret;

	if (user_buf == NULL) {
		AUDIO_LOGE("input error: user buf is NULL");
		return -EINVAL;
	}

	kernel_buf = kzalloc(DA_COMBINE_DBG_SIZE_PAGE, GFP_KERNEL);
	if (kernel_buf == NULL) {
		AUDIO_LOGE("kernel buf is null");
		return -EFAULT;
	}

	ret = simple_write_to_buffer(kernel_buf, DA_COMBINE_DBG_SIZE_PAGE,
		ppos, user_buf, count);
	if (ret != count) {
		kfree(kernel_buf);
		AUDIO_LOGE("Input param buf read error, return value: %zd", ret);
		return -EINVAL;
	}

	ret = write_register(kernel_buf, user_buf);

	kfree(kernel_buf);
	kernel_buf = NULL;

	return ret;
}

static const struct file_operations da_combine_rr_fops = {
	.read = da_combine_rr_read,
	.write = da_combine_rr_write,
};
#endif

void da_combine_dump_debug_info(void)
{
	hi_cdcctrl_dump(g_cdc_ctrl);
	da_combine_resmgr_dump(g_resmgr);
}

static ssize_t da_combine_dump_write(struct file *file,
	const char __user *user_buf, size_t count, loff_t *ppos)
{
	da_combine_dump_debug_info();

	return count;
}

static const struct file_operations da_combine_dump_fops = {
	.write = da_combine_dump_write,
};
#endif

int da_combine_utils_init(struct snd_soc_component *codec, struct hi_cdc_ctrl *cdc_ctrl,
	const struct utils_config *config, struct da_combine_resmgr *resmgr,
	unsigned int codec_type)
{
	g_utils_config = kzalloc(sizeof(*g_utils_config), GFP_KERNEL);
	if (g_utils_config == NULL) {
		AUDIO_LOGE("Failed to kzalloc utils config");
		g_snd_codec = NULL;
		return -EFAULT;
	}
	memcpy(g_utils_config, config, sizeof(*g_utils_config));

#ifdef CONFIG_DFX_DEBUG_FS
	g_debug_dir = debugfs_create_dir("da_combine_v2", NULL);
	if (g_debug_dir == NULL) {
		AUDIO_LOGE("anc_hs: Failed to create debugfs dir");
		kfree(g_utils_config);
		g_utils_config = NULL;
		return 0;
	}
#ifdef ENABLE_DA_COMBINE_CODEC_DEBUG
	if (!debugfs_create_file("rr", 0640, g_debug_dir, NULL, &da_combine_rr_fops))
		AUDIO_LOGE("failed to create rr debugfs file");
#endif

	if (!debugfs_create_file("dump", 0640, g_debug_dir, NULL, &da_combine_dump_fops))
		AUDIO_LOGE("failed to create dump debugfs file");

	spin_lock_init(&da_combine_ins_lock);
	g_snd_cdc = codec;
	g_cdc_ctrl = cdc_ctrl;
	g_resmgr = resmgr;
#endif
	g_cdc_type = codec_type;
	g_snd_codec = codec;

	return 0;
}

void da_combine_utils_deinit(void)
{
	if (g_utils_config != NULL) {
		kfree(g_utils_config);
		g_utils_config = NULL;
	}

	g_snd_codec = NULL;
}

#ifdef CONFIG_PLATFORM_DIEID
int da_combine_codec_get_dieid(char *dieid, unsigned int len)
{
#ifdef CONFIG_SND_SOC_DA_COMBINE_V5
	if (g_cdc_type == DA_COMBINE_CODEC_TYPE_V5)
		return da_combine_v5_codec_get_dieid(dieid, len);
#endif
	return -1;
}
#endif

unsigned int da_combine_utils_reg_read(unsigned int reg)
{
	if (g_snd_codec == NULL) {
		AUDIO_LOGE("parameter is NULL");
		return 0;
	}

	return snd_soc_component_read32(g_snd_codec, reg);
}

