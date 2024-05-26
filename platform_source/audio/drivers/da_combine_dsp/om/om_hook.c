/*
 * om_hook.c
 *
 * hook module for da_combine codec
 *
 * Copyright (c) 2015 Huawei Technologies Co., Ltd.
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

#include "om_hook.h"

#include <linux/err.h>
#include <linux/errno.h>
#include <linux/kthread.h>
#include <linux/semaphore.h>
#include <linux/sched/rt.h>
#include <linux/pm_wakeup.h>
#include <linux/fs.h>
#include <linux/syscalls.h>
#include <linux/kernel.h>
#include <linux/dma-mapping.h>
#include <linux/clk.h>
#include <linux/rtc.h>
#include <linux/time.h>
#include <linux/stat.h>
#include <linux/version.h>
#include <linux/types.h>
#if (KERNEL_VERSION(4, 14, 0) <= LINUX_VERSION_CODE)
#include <uapi/linux/sched/types.h>
#endif
#include <linux/platform_drivers/da_combine/da_combine_v3_dsp_regs.h>
#include <platform_include/basicplatform/linux/rdr_pub.h>
#ifdef ENABLE_DA_COMBINE_HIFI_DEBUG
#include <linux/platform_drivers/da_combine/da_combine_mad.h>
#endif
#include <da_combine_dsp_interface.h>

#include "audio_log.h"
#include "codec_bus.h"
#include "da_combine_dsp_misc.h"
#include "download_image.h"
#include "audio_file.h"
#include "dsp_misc.h"
#include "dsp_utils.h"
#include "om_beta.h"
#ifdef ENABLE_DA_COMBINE_HIFI_DEBUG
#include "om_debug.h"
#endif

/*lint -e655 -e838 -e730*/
#define LOG_TAG "om_hook"

#define HOOK_PATH_VISUAL_TOOL "/data/log/codec_dsp/visual_tool/"
#define HOOK_PATH_DEFAULT "/data/log/codec_dsp/default/"
#define HOOK_RECORD_FILE "da_combine_dsp_hook.data"
#define VERIFY_FRAME_HEAD_MAGIC_NUM 0x5a5a5a5a
#define VERIFY_FRAME_BODY_MAGIC_NUM 0xa5a5a5a5
#define LEFT_DMA_ADDR (CODEC_DSP_OM_DMA_BUFFER_ADDR)
#define LEFT_DMA_SIZE (CODEC_DSP_OM_DMA_BUFFER_SIZE / 2)
#define LEFT_DMA_CFG_ADDR (CODEC_DSP_OM_DMA_CONFIG_ADDR)
#define LEFT_DMA_CFG_SIZE (CODEC_DSP_OM_DMA_CONFIG_SIZE / 2)
#define RIGHT_DMA_ADDR (LEFT_DMA_ADDR + LEFT_DMA_SIZE)
#define RIGHT_DMA_SIZE (LEFT_DMA_SIZE)
#define RIGHT_DMA_CFG_ADDR (LEFT_DMA_CFG_ADDR + LEFT_DMA_CFG_SIZE)
#define RIGHT_DMA_CFG_SIZE (LEFT_DMA_CFG_SIZE)
#define HOOK_AP_DMA_INTERVAL 20 /* ms */
#define HOOK_DSP_DMA_INTERVAL 5 /* ms */
#define HOOK_AP_DSP_DMA_TIMES ((HOOK_AP_DMA_INTERVAL) / (HOOK_DSP_DMA_INTERVAL))
#define HOOK_RECORD_MAX_LENGTH 200
#define HOOK_FILE_NAME_MAX_LENGTH 100
#define HOOK_VISUAL_TOOL_MAX_FILE_SIZE (192 * 1024)
#define HOOK_MAX_FILE_SIZE 0x40000000 /* 1G */
#define HOOK_MAX_DIR_SIZE 0x100000000 /* 4G */
#define HOOK_DEFAULT_SUBDIR_CNT 20
#define HOOK_MAX_SUBDIR_CNT 5000
#define PRINT_MAX_CNT 3
#define HOOK_THREAD_PRIO 20
#define ROOT_UID 0
#define SYSTEM_GID 1000
#define MAX_PARA_SIZE 4096
#define MAGIC_NUM_LOCATION 1
#define INFO_MSG_LOCATION 2

#ifdef CONFIG_AUDIO_COMMON_IMAGE
#undef SOC_ACPU_SLIMBUS_BASE_ADDR
#define SOC_ACPU_SLIMBUS_BASE_ADDR 0
#endif

enum {
	VERIFY_DEFAULT,
	VERIFY_START,
	VERIFY_ADJUSTING,
	VERIFY_ADJUSTED,
	VERIFY_END,
};

enum {
	PCM_SWAP_BUFF_A = 0,
	PCM_SWAP_BUFF_B,
	PCM_SWAP_BUFF_CNT,
};

enum {
	HOOK_LEFT = 0,
	HOOK_RIGHT,
	HOOK_CNT,
};

enum hook_status {
	HOOK_VALID = 0,
	HOOK_POS_UNSUPPORT,
	HOOK_CONFIG_ERR,
	HOOK_BANDWIDTH_LIMIT,
	HOOK_DUPLICATE_POS,
	HOOK_IF_BUSY,
	HOOK_MULTI_SCENE,
	HOOK_STATUS_BUTT,
};

struct hook_pos_name {
	enum hook_pos pos;
	char *name;
};

struct linux_dirent {
	unsigned long d_ino;
	unsigned long d_off;
	unsigned short d_reclen;
	char d_name[1];
};

struct data_cfg {
	unsigned int sample_rate;
	unsigned short resolution;
	unsigned short channels;
};

struct pos_info {
	enum hook_pos pos;
	struct data_cfg config;
	unsigned int size;
	enum hook_status hook_status;
};

struct pos_infos {
	struct list_head node;
	struct pos_info info;
};

struct data_flow {
	struct list_head node;
	void *addr;
};

struct hook_dma_cfg {
	unsigned int port; /* port address */
	unsigned int config; /* dma config number */
	unsigned short channel; /* dma channel number */
};

struct hook_runtime {
	void __iomem *lli_cfg[PCM_SWAP_BUFF_CNT]; /* must be aligned to 32byte */
	void *lli_cfg_phy[PCM_SWAP_BUFF_CNT]; /* lli cfg, physical addr */
	void __iomem *buffer[PCM_SWAP_BUFF_CNT]; /* dma buffer */
	void *buffer_phy[PCM_SWAP_BUFF_CNT]; /* dma buffer, physical addr */
	struct semaphore hook_proc_sema;
	struct semaphore hook_stop_sema;
	struct wakeup_source *wake_lock;
	struct task_struct *kthread;
	struct pos_infos info_list;
	struct data_flow data_list;
	unsigned int size; /* dma buffer size */
	unsigned short channel; /* dma channel number */
	unsigned int kthread_should_stop;
	unsigned int verify_state;
	unsigned int hook_id;
	unsigned int idle_id;
	unsigned int hook_file_size;
	unsigned int hook_file_cnt;
	bool parsed;
};

struct da_combine_hook_priv {
	struct clk *asp_subsys_clk;
	struct hook_runtime runtime[HOOK_CNT];
	char hook_path[HOOK_PATH_MAX_LENGTH];
	bool standby;
	bool started;
	bool is_eng;
	bool should_deactive;
	bool is_dspif_hooking;
	unsigned short bandwidth;
	unsigned short sponsor;
	unsigned int dir_count;
	unsigned int codec_type;
	unsigned short open_file_failed_cnt;
	enum bustype_select bus_sel;
};

struct hook_dma_addr {
	unsigned int dma_addr;
	unsigned int dma_size;
	unsigned int dma_config_addr;
	unsigned int dma_config_size;
};

static const struct data_cfg bus_data_cfg[HOOK_CNT] = {
	{ .sample_rate = 192000, .resolution = 4, .channels = 1 },  /* left */
	{ .sample_rate = 192000, .resolution = 4, .channels = 1 },  /* right */
};

#ifdef ENABLE_DA_COMBINE_HIFI_DEBUG
static struct hook_dma_addr hook_dma_addr_cfg[HOOK_CNT] = {
	{ .dma_addr = LEFT_DMA_ADDR, .dma_size = LEFT_DMA_SIZE,
		.dma_config_addr = LEFT_DMA_CFG_ADDR,
		.dma_config_size = LEFT_DMA_CFG_SIZE },
	{ .dma_addr = RIGHT_DMA_ADDR, .dma_size = RIGHT_DMA_SIZE,
		.dma_config_addr = LEFT_DMA_CFG_ADDR,
		.dma_config_size = RIGHT_DMA_CFG_SIZE },
};

static struct hook_dma_cfg slim_dma_cfg[HOOK_CNT] = {
	{ .port = (SOC_ACPU_SLIMBUS_BASE_ADDR + 0x1180),
		.config = 0x43322067, .channel = 14 },  /* left */
	{ .port = (SOC_ACPU_SLIMBUS_BASE_ADDR + 0x11c0),
		.config = 0x43322077, .channel = 15 },  /* right */
};

#ifdef CONFIG_PLATFORM_SOUNDWIRE
static const struct hook_dma_cfg soundwire_dma_cfg[HOOK_CNT] = {
	{ .port = (SOC_ACPU_SoundWire_BASE_ADDR + 0x400),
		.config = 0x43322047, .channel = 14 },
};
#endif

#ifdef CONFIG_AUDIO_COMMON_IMAGE
static void bind_dma_addr(void)
{
	int i;
	uint32_t phy_base = get_platform_mem_base_addr(PLT_HIFI_MEM);

	for (i = 0; i < HOOK_CNT; i++) {
		hook_dma_addr_cfg[i].dma_addr += phy_base;
		hook_dma_addr_cfg[i].dma_config_addr += phy_base;
	}
}

static void bind_dma_cfg(void)
{
	int i;
	uint32_t slim_base = get_platform_mem_base_addr(PLT_SLIMBUS_MEM);

	for (i = 0; i < HOOK_CNT; i++)
		slim_dma_cfg[i].port += slim_base;
}
#endif
#endif

static const struct hook_pos_name pos_name[] = {
	{ HOOK_POS_IF0,              "IF0" },
	{ HOOK_POS_IF1,              "IF1" },
	{ HOOK_POS_IF2,              "IF2" },
	{ HOOK_POS_IF3,              "IF3" },
	{ HOOK_POS_IF4,              "IF4" },
	{ HOOK_POS_IF5,              "IF5" },
	{ HOOK_POS_IF6,              "IF6" },
	{ HOOK_POS_IF7,              "IF7" },
	{ HOOK_POS_IF8,              "IF8" },
	{ HOOK_POS_ANC_CORE,         "ANC_CORE" },
	{ HOOK_POS_LOG,              "LOG" },
	{ HOOK_POS_ANC_PCM_RX_VOICE, "ANC_PCM_RX_VOICE" },
	{ HOOK_POS_ANC_PCM_REF,      "ANC_PCM_REF" },
	{ HOOK_POS_ANC_PCM_ERR,      "ANC_PCM_ERR" },
	{ HOOK_POS_ANC_PCM_ANTI,     "ANC_PCM_ANTI" },
	{ HOOK_POS_ANC_BETA_CSINFO,  "ANC_BETA_CSINFO" },
	{ HOOK_POS_ANC_PCM_TX_VOICE, "ANC_PCM_TX_VOICE" },
	{ HOOK_POS_ANC_ALG_INDICATE, "ANC_ALG_INDICATE" },
	{ HOOK_POS_ANC_COEF,         "ANC_COEF" },
	{ HOOK_POS_PA_EFFECTIN,      "PA_EFFECTIN" },
	{ HOOK_POS_PA_EFFECTOUT,     "PA_EFFECTOUT" },
	{ HOOK_POS_WAKEUP_MICIN,     "WAKEUP_MICIN" },
	{ HOOK_POS_WAKEUP_EFFECTOUT, "WAKEUP_EFFECTOUT" },
	{ HOOK_POS_PA1_I,            "PA1_I" },
	{ HOOK_POS_PA1_V,            "PA1_V" },
	{ HOOK_POS_PA2_I,            "PA2_I" },
	{ HOOK_POS_PA2_V,            "PA2_V" },
	{ HOOK_POS_ULTRASONIC,       "ULTRASONIC" },
};

static struct da_combine_hook_priv *hook_priv;

static unsigned int get_idle_buffer_id(const struct hook_runtime *runtime)
{
	unsigned int used_dma_addr;
	unsigned int src_addr_a;

	src_addr_a =
		(unsigned int)((uintptr_t)runtime->buffer_phy[PCM_SWAP_BUFF_A]);

	used_dma_addr = asp_dma_get_des(runtime->channel);
	if (used_dma_addr == src_addr_a)
		return PCM_SWAP_BUFF_B;
	else
		return PCM_SWAP_BUFF_A;
}

static void deactivate_codec_bus(void)
{
	int ret = 0;
	struct scene_param params;
	const struct data_cfg *data_cfg = &bus_data_cfg[HOOK_LEFT];

	params.channels = 1;
	params.rate = data_cfg->sample_rate;
	params.bits_per_sample = 32;
	params.priority = NORMAL_PRIORITY;
	params.ports[0] = PORT_TYPE_U9;

	if (hook_priv->should_deactive) {
		ret = codec_bus_deactivate("DEBUG", &params);
		if (ret)
			AUDIO_LOGW("deactivate debug return ret %d", ret);
	}

	clk_disable_unprepare(hook_priv->asp_subsys_clk);
}

/* stop hook dma and release hook data buffer */
static void stop_hook(unsigned int hook_id)
{
	int ret;
	struct hook_runtime *runtime = NULL;
	struct pos_infos *pos_infos = NULL;
	struct da_combine_hook_priv *priv = hook_priv;

	runtime = &priv->runtime[hook_id];

	runtime->hook_file_cnt  = 0;
	runtime->hook_file_size = 0;

	asp_dma_stop(runtime->channel);

	ret = down_interruptible(&runtime->hook_stop_sema);
	if (ret == -ETIME)
		AUDIO_LOGE("down interruptible error -ETIME");

	iounmap(runtime->buffer[PCM_SWAP_BUFF_A]);
	iounmap(runtime->lli_cfg[PCM_SWAP_BUFF_A]);

	while (!list_empty(&runtime->info_list.node)) {
		pos_infos = list_entry(runtime->info_list.node.next,
			struct pos_infos, node);
		list_del(&pos_infos->node);
		kfree(pos_infos);
	}

	priv->started = false;
	priv->standby = true;

	up(&runtime->hook_stop_sema);
}

#ifdef ENABLE_DA_COMBINE_HIFI_DEBUG
static int recreat_dir(void)
{
	struct da_combine_hook_priv *priv = hook_priv;

	if (audio_remove_dir(priv->hook_path, sizeof(priv->hook_path)) < 0)
		return -EFAULT;

	if (audio_create_dir(priv->hook_path, sizeof(priv->hook_path)))
		return -EFAULT;

	return 0;
}

static int create_hook_dir(void)
{
	char dir[HOOK_PATH_MAX_LENGTH] = {0};
	int dir_count;
	int ret = 0;
	long long dir_size;
	struct da_combine_hook_priv *priv = hook_priv;

	if (audio_create_dir(priv->hook_path, sizeof(priv->hook_path))) {
		ret = -EFAULT;
		goto out;
	}

	if (priv->sponsor != OM_SPONSOR_DEFAULT) {
		ret = recreat_dir();
		goto out;
	}

	dir_size = audio_get_dir_size(priv->hook_path, sizeof(priv->hook_path));
	if (dir_size < 0) {
		ret = dir_size;
		goto out;
	}

	AUDIO_LOGI("dir size:%lld", dir_size);

	dir_count = audio_get_dir_count(priv->hook_path, sizeof(priv->hook_path));
	if (dir_count < 0) {
		ret = dir_count;
		goto out;
	}

	AUDIO_LOGI("dir count:%d, max dir count:%u", dir_count, priv->dir_count);

	if (dir_size > HOOK_MAX_DIR_SIZE ||
		(unsigned int)dir_count > priv->dir_count) {
		AUDIO_LOGI("rm dirs, dir size:%lld, dir count:%d, max dir count:%u",
			dir_size, dir_count, priv->dir_count);

		ret = recreat_dir();
		if (ret != 0)
			goto out;
	}

	/* generate dir name by system time */
	audio_append_dir_systime(dir, priv->hook_path, sizeof(priv->hook_path));
	if (audio_create_dir(dir, sizeof(dir))) {
		ret = -EFAULT;
		goto out;
	}

	strncpy(priv->hook_path, dir, sizeof(priv->hook_path) - 1);

out:
	return ret;
}

static void set_hook_path_by_sponsor(void)
{
	struct da_combine_hook_priv *priv = hook_priv;

	switch (priv->sponsor) {
	case OM_SPONSOR_BETA:
		strncpy(priv->hook_path, HOOK_PATH_BETA_CLUB,
			sizeof(priv->hook_path));
		break;
	case OM_SPONSOR_TOOL:
		strncpy(priv->hook_path, HOOK_PATH_VISUAL_TOOL,
			sizeof(priv->hook_path));
		break;
	default:
		strncpy(priv->hook_path, HOOK_PATH_DEFAULT,
			sizeof(priv->hook_path));
		break;
	}
}

static int get_hook_buffer_addr(struct hook_runtime *runtime,
	const struct hook_dma_addr *dma_addr_cfg)
{
	runtime->buffer_phy[PCM_SWAP_BUFF_A] =
		(void *)(uintptr_t)(dma_addr_cfg->dma_addr);
	runtime->buffer_phy[PCM_SWAP_BUFF_B] =
		(void *)(uintptr_t)(dma_addr_cfg->dma_addr + runtime->size);
	runtime->lli_cfg_phy[PCM_SWAP_BUFF_A] =
		(void *)(uintptr_t)(dma_addr_cfg->dma_config_addr);
	runtime->lli_cfg_phy[PCM_SWAP_BUFF_B] =
		(void *)(uintptr_t)(dma_addr_cfg->dma_config_addr +
		sizeof(struct dma_lli_cfg));

	runtime->buffer[PCM_SWAP_BUFF_A] =
		ioremap_wc(dma_addr_cfg->dma_addr, dma_addr_cfg->dma_size);//lint !e446
	if (runtime->buffer[PCM_SWAP_BUFF_A] == NULL) {
		AUDIO_LOGE("ioremap fail");
		return -ENOMEM;
	}

	runtime->buffer[PCM_SWAP_BUFF_B] =
		(void __iomem *)((char *)runtime->buffer[PCM_SWAP_BUFF_A] +
		runtime->size);

	runtime->lli_cfg[PCM_SWAP_BUFF_A] =
		ioremap_wc(dma_addr_cfg->dma_config_addr, dma_addr_cfg->dma_config_size);//lint !e446
	if (runtime->lli_cfg[PCM_SWAP_BUFF_A] == NULL) {
		AUDIO_LOGE("ioremap fail");
		iounmap(runtime->buffer[PCM_SWAP_BUFF_A]);
		runtime->buffer[PCM_SWAP_BUFF_A] = NULL;
		runtime->buffer[PCM_SWAP_BUFF_B] = NULL;
		return -ENOMEM;
	}

	runtime->lli_cfg[PCM_SWAP_BUFF_B] =
		(void __iomem *)((char *)runtime->lli_cfg[PCM_SWAP_BUFF_A] +
		sizeof(struct dma_lli_cfg));

	return 0;
}

static int alloc_hook_buffer_fama(unsigned int hook_id)
{
	int ret;
	struct hook_runtime *runtime = NULL;
	struct da_combine_hook_priv *priv = hook_priv;

	runtime = &priv->runtime[hook_id];

	if (hook_id == HOOK_LEFT)
		ret = get_hook_buffer_addr(runtime, &hook_dma_addr_cfg[HOOK_LEFT]);
	else
		ret = get_hook_buffer_addr(runtime, &hook_dma_addr_cfg[HOOK_RIGHT]);

	if (ret != 0)
		return -EFAULT;

	/* must use ioremap_wc to remap */
	memset((void *)runtime->buffer[PCM_SWAP_BUFF_A], 0, LEFT_DMA_SIZE);
	memset((void *)runtime->lli_cfg[PCM_SWAP_BUFF_A], 0, LEFT_DMA_CFG_SIZE);

	return 0;
}

static unsigned int get_data_size(const struct data_cfg *config)
{
	uint32_t data_size;

	data_size = config->sample_rate * config->resolution *
		config->channels * HOOK_AP_DMA_INTERVAL / 1000;

	return data_size;
}

static int get_dma_config(unsigned int hook_id)
{
	struct da_combine_hook_priv *priv = hook_priv;
	struct dma_lli_cfg *lli_cfg_a = NULL;
	struct dma_lli_cfg *lli_cfg_b = NULL;
	struct hook_runtime *runtime = NULL;
	const struct data_cfg *data_cfg = NULL;
	const struct hook_dma_cfg *dma_cfg = NULL;

	runtime = &priv->runtime[hook_id];
	data_cfg = &bus_data_cfg[hook_id];
	dma_cfg = &slim_dma_cfg[hook_id];
#ifdef CONFIG_PLATFORM_SOUNDWIRE
	if (priv->bus_sel == BUSTYPE_SELECT_SOUNDWIRE)
		dma_cfg = &soundwire_dma_cfg[0];
#endif

	runtime->size = get_data_size(data_cfg);

	if (alloc_hook_buffer_fama(hook_id) != 0) {
		AUDIO_LOGE("alloc hook buffer fail");
		return -EFAULT;
	}

	runtime->channel = dma_cfg->channel;
	runtime->verify_state = VERIFY_DEFAULT;
	runtime->parsed = false;

	lli_cfg_a = (struct dma_lli_cfg *)runtime->lli_cfg[PCM_SWAP_BUFF_A];
	lli_cfg_b = (struct dma_lli_cfg *)runtime->lli_cfg[PCM_SWAP_BUFF_B];

	lli_cfg_a->a_count = runtime->size;
	lli_cfg_a->src_addr = dma_cfg->port;
	lli_cfg_a->des_addr =
		(unsigned int)((uintptr_t)runtime->buffer_phy[PCM_SWAP_BUFF_A]);
	lli_cfg_a->config = dma_cfg->config;
	lli_cfg_a->lli = ASP_DMA_LLI_LINK(runtime->lli_cfg_phy[PCM_SWAP_BUFF_B]);

	lli_cfg_b->a_count = runtime->size;
	lli_cfg_b->src_addr = dma_cfg->port;
	lli_cfg_b->des_addr =
		(unsigned int)((uintptr_t)runtime->buffer_phy[PCM_SWAP_BUFF_B]);
	lli_cfg_b->config = dma_cfg->config;
	lli_cfg_b->lli = ASP_DMA_LLI_LINK(runtime->lli_cfg_phy[PCM_SWAP_BUFF_A]);

	return 0;
}

static void dump_dma_config(const struct hook_runtime *runtime)
{
	const struct dma_lli_cfg *lli_cfg_a = NULL;
	const struct dma_lli_cfg *lli_cfg_b = NULL;

	if (runtime == NULL)
		return;

	AUDIO_LOGI("lli cfg:%pK, buffer:%pK",
		runtime->lli_cfg[PCM_SWAP_BUFF_A],
		runtime->buffer[PCM_SWAP_BUFF_A]);

	AUDIO_LOGI("lli cfg phy:%pK, buffer phy:%pK",
		runtime->lli_cfg_phy[PCM_SWAP_BUFF_A],
		runtime->buffer_phy[PCM_SWAP_BUFF_A]);

	lli_cfg_a = (struct dma_lli_cfg *)runtime->lli_cfg[PCM_SWAP_BUFF_A];
	lli_cfg_b = (struct dma_lli_cfg *)runtime->lli_cfg[PCM_SWAP_BUFF_B];
	if (!lli_cfg_a || !lli_cfg_b)
		return;

	AUDIO_LOGI("A::a count:0x%x, src addr:0x%pK, dest addr:0x%pK",
		lli_cfg_a->a_count, (void *)(uintptr_t)(lli_cfg_a->src_addr),
		(void *)(uintptr_t)(lli_cfg_a->des_addr));
	AUDIO_LOGI("config:0x%x, lli:0x%x", lli_cfg_a->config, lli_cfg_a->lli);

	AUDIO_LOGI("B::a count:0x%x, src addr:0x%pK, dest addr:0x%pK",
		lli_cfg_b->a_count, (void *)(uintptr_t)(lli_cfg_b->src_addr),
		(void *)(uintptr_t)(lli_cfg_b->des_addr));
	AUDIO_LOGI("config:0x%x, lli:0x%x", lli_cfg_b->config, lli_cfg_b->lli);

	AUDIO_LOGI("dma size:%d, dma channel:%d",
		runtime->size, runtime->channel);

	AUDIO_LOGI("parsed:%d, verify state:%d",
		runtime->parsed, runtime->verify_state);
}

static int left_dma_irq_handler(unsigned short int_type,
	unsigned long para, unsigned int dma_channel)
{
	struct da_combine_hook_priv *priv = hook_priv;
	struct hook_runtime *runtime = NULL;

	UNUSED_PARAMETER(dma_channel);

	if ((int_type != ASP_DMA_INT_TYPE_TC1) &&
		(int_type != ASP_DMA_INT_TYPE_TC2)) {
		AUDIO_LOGE("int type err:%d", int_type);
		return -EINVAL;
	}

	runtime = &priv->runtime[HOOK_LEFT];
	runtime->idle_id = get_idle_buffer_id(runtime);
	up(&runtime->hook_proc_sema);

	return 0;
}

static int right_dma_irq_handler(unsigned short int_type,
	unsigned long para, unsigned int dma_channel)
{
	struct da_combine_hook_priv *priv = hook_priv;
	struct hook_runtime *runtime = NULL;

	UNUSED_PARAMETER(dma_channel);

	if ((int_type != ASP_DMA_INT_TYPE_TC1) &&
		(int_type != ASP_DMA_INT_TYPE_TC2)) {
		AUDIO_LOGE("int type err:%d", int_type);
		return -EINVAL;
	}

	runtime = &priv->runtime[HOOK_RIGHT];
	runtime->idle_id = get_idle_buffer_id(runtime);
	up(&runtime->hook_proc_sema);

	return 0;
}

static int start_hook_dma(unsigned int hook_id)
{
	struct hook_runtime *runtime = NULL;
	struct da_combine_hook_priv *priv = hook_priv;
	callback_t callback = NULL;

	runtime = &priv->runtime[hook_id];

	runtime->hook_file_cnt  = 0;
	runtime->hook_file_size = 0;

	if (get_dma_config(hook_id))
		return -EINVAL;

	dump_dma_config(runtime);

	if (hook_id == HOOK_LEFT)
		callback = left_dma_irq_handler;
	else
		callback = right_dma_irq_handler;

	asp_dma_config(runtime->channel,
		runtime->lli_cfg[PCM_SWAP_BUFF_A], callback, 0);
	asp_dma_start(runtime->channel,
		runtime->lli_cfg[PCM_SWAP_BUFF_A]);

	priv->started = true;
	priv->standby = false;

	return 0;
}

static int prepare_hook_file(void)
{
	int ret;

	set_hook_path_by_sponsor();

	ret = create_hook_dir();
	if (ret != 0) {
		AUDIO_LOGE("create hook dir failed, ret:%d", ret);
		return -EBUSY;
	}

	return 0;
}

static int hook_slimbus_callback(const void *params)
{
	struct da_combine_hook_priv *priv = hook_priv;

	if (priv == NULL) {
		AUDIO_LOGE("priv is null");
		return -EINVAL;
	}

	UNUSED_PARAMETER(params);

	priv->should_deactive = false;
	da_combine_stop_dsp_hook();

	return 0;
}

static int active_codec_bus(void)
{
	int ret;
	struct scene_param params;
	struct da_combine_hook_priv *priv = hook_priv;
	const struct data_cfg *data_cfg = &bus_data_cfg[HOOK_LEFT];

	ret = clk_prepare_enable(priv->asp_subsys_clk);
	if (ret) {
		AUDIO_LOGE("clk prepare enable failed");
		return -EBUSY;
	}

	memset(&params, 0, sizeof(params));
	params.rate = data_cfg->sample_rate;
	params.ports[0] = PORT_TYPE_U9;
	if (priv->bandwidth == OM_BANDWIDTH_12288) {
		params.channels = 2;
		params.ports[1] = PORT_TYPE_U10;
	} else {
		params.channels = 1;
	}
	params.bits_per_sample = 32;
	params.priority = NORMAL_PRIORITY;
	params.callback = hook_slimbus_callback;
	priv->should_deactive = true;

	ret = codec_bus_activate("DEBUG", &params);
	if (ret) {
		AUDIO_LOGE("activate failded, ret: %d", ret);
		return ret;
	}

	/* clk must at codec to avoid frequency offset */
	if (codec_bus_get_framer_type() != FRAMER_CODEC) {
		AUDIO_LOGE("can not start om");
		return -EBUSY;
	}

	return 0;
}

static int start_hook_route(void)
{
	int ret;
	struct da_combine_hook_priv *priv = hook_priv;

	if (priv == NULL) {
		AUDIO_LOGE("priv is null");
		return -EINVAL;
	}

	if (!priv->is_eng) {
		AUDIO_LOGE("current verison is not eng");
		return -EPERM;
	}

	if (priv->started || !priv->standby) {
		AUDIO_LOGE("hook is started or standby");
		return -EBUSY;
	}

	ret = prepare_hook_file();
	if (ret != 0) {
		AUDIO_LOGE("prepare hook file failed, ret:%d", ret);
		return ret;
	}

	ret = active_codec_bus();
	if (ret != 0) {
		AUDIO_LOGE("active failed, ret:%d", ret);
		return ret;
	}

	priv->open_file_failed_cnt = 0;

	ret = start_hook_dma(HOOK_LEFT);
	if (ret != 0)
		goto exit;

	AUDIO_LOGI("left hook started");

	if (priv->bandwidth == OM_BANDWIDTH_12288) {
		ret = start_hook_dma(HOOK_RIGHT);
		if (ret != 0) {
			stop_hook(HOOK_LEFT);
			goto exit;
		}

		AUDIO_LOGI("right hook started");
	}

	return 0;

exit:
	deactivate_codec_bus();
	priv->sponsor = OM_SPONSOR_DEFAULT;

	return ret;
}

bool is_scene_conflict(void)
{
	union da_combine_high_freq_status high_status;
	union da_combine_low_freq_status low_status;

	high_status.val = da_combine_get_high_freq_status();
	low_status.val = da_combine_get_low_freq_status();

	if (high_status.pa && low_status.wake_up) {
		AUDIO_LOGW("pa & wake up exist, can not hook");
		return true;
	}

	return false;
}

static int check_hook_para(const struct da_combine_param_io_buf *param)
{
	const struct om_start_hook_msg *start_hook_msg = NULL;

	if (!param || !param->buf_in) {
		AUDIO_LOGE("null param");
		return -EINVAL;
	}

	if (param->buf_size_in == 0 || param->buf_size_in > MAX_PARA_SIZE) {
		AUDIO_LOGE("err param size:%d", param->buf_size_in);
		return -EINVAL;
	}

	start_hook_msg = (struct om_start_hook_msg *)param->buf_in;

	if (param->buf_size_in < (sizeof(*start_hook_msg) +
		start_hook_msg->para_size)) {
		AUDIO_LOGE("input size:%u invalid", param->buf_size_in);
		return -EINVAL;
	}

	return 0;
}

static int check_dsp_if(const void *hook_para, unsigned int size)
{
	unsigned int count;
	unsigned int i;
	unsigned int dspif = 0;
	const struct om_hook_para *para = NULL;

	para = (struct om_hook_para *)hook_para;
	/* calc pos count will hook */
	count = size / sizeof(*para);

	for (i = 0; i < count; i++) {
		dspif = para[i].pos & HOOK_POS_IF0;
		dspif += para[i].pos & HOOK_POS_IF1;
		dspif += para[i].pos & HOOK_POS_IF2;
		dspif += para[i].pos & HOOK_POS_IF3;
		dspif += para[i].pos & HOOK_POS_IF4;
		dspif += para[i].pos & HOOK_POS_IF5;
		dspif += para[i].pos & HOOK_POS_IF6;
		dspif += para[i].pos & HOOK_POS_IF7;
		dspif += para[i].pos & HOOK_POS_IF8;
	}

	AUDIO_LOGI("dsp if data exist flag:%d", dspif);
	if (!dspif)
		return 0;

	return -1;
}

static int check_dsp_status(const void *hook_para, unsigned int size)
{
	unsigned int have_dsp_scene;
	union da_combine_low_freq_status low_scene;
	union da_combine_high_freq_status hi_scene;

	if (hook_para == NULL) {
		AUDIO_LOGE("hook para is null");
		return -EINVAL;
	}

	low_scene.val = da_combine_get_low_freq_status();
	hi_scene.val = da_combine_get_high_freq_status();
	if (hi_scene.ir_learn || hi_scene.ir_trans) {
		AUDIO_LOGW("dsp have ir scene, can not hook if data, high:0x%x",
			hi_scene.val);
		return -EINVAL;
	}

	if (check_dsp_if(hook_para, size) == 0)
		return 0;

	have_dsp_scene = hi_scene.pa;
	have_dsp_scene += hi_scene.anc;
	have_dsp_scene += hi_scene.anc_test;
	have_dsp_scene += hi_scene.anc_debug;
	have_dsp_scene += hi_scene.mad_test;
	have_dsp_scene += low_scene.wake_up;

	if (have_dsp_scene) {
		AUDIO_LOGW("dsp have scene, can not hook if data");
		AUDIO_LOGW("high:0x%x, low:0x%x", hi_scene.val, low_scene.val);

		return -EINVAL;
	}

	hook_priv->is_dspif_hooking = true;

	return 0;
}

static int check_hook_context(const struct da_combine_param_io_buf *param)
{
	int ret;

	const struct om_hook_para *hook_para = NULL;
	const struct om_start_hook_msg *start_hook_msg = NULL;

	if (is_scene_conflict()) {
		AUDIO_LOGW("hook disable");
		return -EINVAL;
	}

	if (da_combine_check_i2s2_clk()) {
		AUDIO_LOGE("s2 is enable, om hook can't start");
		return -EINVAL;
	}

	start_hook_msg = (struct om_start_hook_msg *)param->buf_in;
	hook_para = (struct om_hook_para *)((char *)start_hook_msg +
		sizeof(*start_hook_msg));

	ret = check_dsp_status(hook_para, start_hook_msg->para_size);
	if (ret != 0)
		AUDIO_LOGE("check current scene fail, ret: %d", ret);

	return ret;
}

static int set_dsp_hook_dir_count(unsigned int count)
{
	struct da_combine_hook_priv *priv = hook_priv;

	if (priv == NULL) {
		AUDIO_LOGE("priv is null");
		return -EINVAL;
	}

	if (priv->started || !priv->standby) {
		AUDIO_LOGE("om is running, forbid set dir count:%d", count);
		return -EBUSY;
	}

	if (count > HOOK_MAX_SUBDIR_CNT)
		priv->dir_count = HOOK_MAX_SUBDIR_CNT;
	else
		priv->dir_count = count;

	AUDIO_LOGI("set om dir count:%d", priv->dir_count);

	return 0;
}
#endif

static int check_param_and_scene(const struct da_combine_param_io_buf *param,
	unsigned int in_size)
{
	int ret;
	union da_combine_high_freq_status high_status;

	ret = da_combine_check_msg_para(param, in_size);
	if (ret != 0)
		return ret;

	if (param->buf_size_in != in_size) {
		AUDIO_LOGE("err param size:%d!", param->buf_size_in);
		return -EINVAL;
	}

	high_status.val = da_combine_get_high_freq_status();
	if (high_status.om_hook) {
		AUDIO_LOGW("om hook is running");
		return -EINVAL;
	}

	return 0;
}

static const char *get_pos_name(enum hook_pos pos)
{
	unsigned int i;
	unsigned int array_size;

	array_size = ARRAY_SIZE(pos_name);
	for (i = 0; i < array_size; i++) {
		if (pos_name[i].pos == pos)
			return pos_name[i].name;
	}

	return "UNKNOW";
}

static void dump_dsp_data_for_tool(enum hook_pos pos, const void *data,
	unsigned int size, unsigned int hook_id)
{
	char path[HOOK_PATH_MAX_LENGTH + HOOK_FILE_NAME_MAX_LENGTH] = {0};
	char new_path[HOOK_PATH_MAX_LENGTH + HOOK_FILE_NAME_MAX_LENGTH] = {0};
	struct da_combine_hook_priv *priv = hook_priv;
	struct hook_runtime *runtime = NULL;

	if (pos == HOOK_POS_LOG)
		return;

	if (hook_id == HOOK_LEFT) {
		runtime = &priv->runtime[HOOK_LEFT];
		snprintf(path, sizeof(path) - 1, "%s%s_L_WRITING.data",
			priv->hook_path, get_pos_name(pos));
		snprintf(new_path, sizeof(new_path) - 1, "%s%s_L_%d.data",
			priv->hook_path, get_pos_name(pos),
			runtime->hook_file_cnt);
	} else {
		runtime = &priv->runtime[HOOK_RIGHT];
		snprintf(path, sizeof(path) - 1, "%s%s_R_WRITING.data",
			priv->hook_path, get_pos_name(pos));
		snprintf(new_path, sizeof(new_path) - 1, "%s%s_R_%d.data",
			priv->hook_path, get_pos_name(pos),
			runtime->hook_file_cnt);
	}

	da_combine_dsp_dump_to_file(data, size, path);

	runtime->hook_file_size += size;

	if (runtime->hook_file_size >= HOOK_VISUAL_TOOL_MAX_FILE_SIZE) {
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4,19,0))
		(void)ksys_rename(path, new_path);
#else
		(void)sys_rename(path, new_path);
#endif
		runtime->hook_file_cnt++;
		runtime->hook_file_size = 0;
	}
}

static void dump_dsp_data(enum hook_pos pos, const void *data,
	unsigned int size, unsigned int hook_id)
{
	char path[HOOK_PATH_MAX_LENGTH + HOOK_FILE_NAME_MAX_LENGTH] = {0};
	struct da_combine_hook_priv *priv = hook_priv;

	if (pos == HOOK_POS_LOG)
		return;

	if (priv->sponsor == OM_SPONSOR_BETA) {
		da_combine_anc_beta_generate_path(pos, priv->hook_path,
			path, sizeof(path));
	} else {
		if (hook_id == HOOK_LEFT)
			snprintf(path, sizeof(path), "%s%s_L.data",
				priv->hook_path, get_pos_name(pos));
		else
			snprintf(path, sizeof(path), "%s%s_R.data",
				priv->hook_path, get_pos_name(pos));
	}
	da_combine_dsp_dump_to_file(data, size, path);
}

static void parse_dsp_data(const void *data, const struct hook_runtime *runtime)
{
	struct da_combine_hook_priv *priv = hook_priv;
	const struct pos_infos *pos_infos = NULL;
	const unsigned int *buffer = (unsigned int *)data;
	const unsigned int *dsp_buffer = NULL;
	unsigned int data_size = 0;
	unsigned int i;

	if (*buffer == VERIFY_FRAME_HEAD_MAGIC_NUM) {
		AUDIO_LOGW("throw away verify data");
		return;
	}

	/*
	 * data arrange:
	 * |dsp buffer1|dsp buffer2|dsp buffer3|dsp buffer4|
	 */
	for (i = 0; i < HOOK_AP_DSP_DMA_TIMES; i++) {
		dsp_buffer = buffer;

		list_for_each_entry(pos_infos, &runtime->info_list.node, node) {
			data_size = pos_infos->info.size;

			if (((char *)dsp_buffer + data_size) >
				((char *)data + runtime->size)) {
				AUDIO_LOGE("om buffer will overflow, forbid dump data");
				return;
			}

			if (priv->sponsor == OM_SPONSOR_TOOL)
				dump_dsp_data_for_tool(pos_infos->info.pos,
					dsp_buffer, data_size,
					runtime->hook_id);
			else
				dump_dsp_data(pos_infos->info.pos, dsp_buffer,
						data_size, runtime->hook_id);

			dsp_buffer += data_size / sizeof(unsigned int);
		}

		buffer += (runtime->size / HOOK_AP_DSP_DMA_TIMES) /
			sizeof(unsigned int);
	}
}

static int parse_pos_info(const void *data, struct hook_runtime *runtime)
{
	struct da_combine_hook_priv *priv = hook_priv;
	const struct pos_info *pos_info = NULL;
	struct pos_infos *pos_infos = NULL;
	unsigned int parsed_size = 0;
	const unsigned int *buffer = (unsigned int *)data;
	char log[HOOK_RECORD_MAX_LENGTH] = {0};
	char path[HOOK_PATH_MAX_LENGTH + HOOK_FILE_NAME_MAX_LENGTH] = {0};

	/*
	 * pos info arrange:
	 * |5a5a5a5a|a5a5a5a5|......|a5a5a5a5|
	 * |  head  |  body  | info |  body  |
	 */
	if (buffer[MAGIC_NUM_LOCATION] != VERIFY_FRAME_BODY_MAGIC_NUM) {
		AUDIO_LOGE("err pos info:%d", buffer[1]);
		return -EINVAL;
	}

	pos_info = (struct pos_info *) (&buffer[INFO_MSG_LOCATION]);
	while ((*(unsigned int *)pos_info != VERIFY_FRAME_BODY_MAGIC_NUM) &&
		(parsed_size <= runtime->size)) {
		if (pos_info->hook_status != HOOK_VALID ||
			(pos_info->size > (runtime->size / HOOK_AP_DSP_DMA_TIMES))) {
			AUDIO_LOGE("invalid hook pos:0x%x, size:%d, reason:%d",
				pos_info->pos, pos_info->size,
				pos_info->hook_status);
			snprintf(log, sizeof(log) - 1,
				"invalid hook pos:0x%x, size:%d, reason:%d\n",
				pos_info->pos, pos_info->size,
				pos_info->hook_status);
			snprintf(path, sizeof(path) - 1,
				"%s%s", priv->hook_path, HOOK_RECORD_FILE);
			da_combine_dsp_dump_to_file(log, strlen(log), path);
			pos_info++;
			parsed_size += sizeof(struct pos_info);
			break;
		}

		pos_infos = kzalloc(sizeof(struct pos_infos), GFP_KERNEL);
		if (pos_infos == NULL) {
			AUDIO_LOGE("malloc failed");
			break;
		}

		memcpy(&pos_infos->info, pos_info, sizeof(struct pos_info));

		AUDIO_LOGI("pos:0x%x, size:%d, state:%d, rate:%d",
			pos_infos->info.pos, pos_infos->info.size,
			pos_infos->info.hook_status,
			pos_infos->info.config.sample_rate);
		AUDIO_LOGI("resolution:%d, channels:%d",
			pos_infos->info.config.resolution,
			pos_infos->info.config.channels);

		list_add_tail(&pos_infos->node, &runtime->info_list.node);

		pos_info++;

		parsed_size += sizeof(struct pos_info);
	}

	return 0;
}

static unsigned int get_verify_pos(void *data, unsigned int size)
{
	unsigned int i;

	unsigned int *buffer = (unsigned int *)data;

	for (i = 0; i < size / sizeof(unsigned int); i++)
		if (buffer[i] == VERIFY_FRAME_HEAD_MAGIC_NUM)
			break;

	return i * sizeof(unsigned int);
}

static void verify_data(struct data_flow *data, struct hook_runtime *runtime)
{
	unsigned int pos;
	unsigned int id;

	if (runtime->verify_state == VERIFY_END)
		return;

	if (runtime->verify_state == VERIFY_DEFAULT) {
		pos = get_verify_pos(data->addr, runtime->size);
		if (pos == runtime->size)
			return;

		id = get_idle_buffer_id(runtime);
		if (pos)
			((struct dma_lli_cfg *)(runtime->lli_cfg[id]))->a_count = pos;

		AUDIO_LOGI("verify pos:%d", pos);
		runtime->verify_state = VERIFY_START;
		AUDIO_LOGI("verify start");
	} else if (runtime->verify_state == VERIFY_START) {
		((struct dma_lli_cfg *)(runtime->lli_cfg[PCM_SWAP_BUFF_A]))->a_count =
			runtime->size;
		((struct dma_lli_cfg *)(runtime->lli_cfg[PCM_SWAP_BUFF_B]))->a_count =
			runtime->size;
		runtime->verify_state = VERIFY_ADJUSTING;
		AUDIO_LOGI("verify adjusting");
	} else if (runtime->verify_state == VERIFY_ADJUSTING) {
		runtime->verify_state = VERIFY_ADJUSTED;
		AUDIO_LOGI("verify adjusted");
	} else {
		runtime->verify_state = VERIFY_END;
		AUDIO_LOGI("verify end");
	}
}

static void parse_data(struct hook_runtime *runtime)
{
	struct data_flow *data = NULL;

	if (list_empty(&runtime->data_list.node)) {
		AUDIO_LOGE("data list is empty");
		return;
	}
	data = list_entry(runtime->data_list.node.next, struct data_flow, node);

	verify_data(data, runtime);
	if (runtime->verify_state != VERIFY_END)
		goto free_list_node;

	if (!runtime->parsed) {
		if (!parse_pos_info(data->addr, runtime))
			runtime->parsed = true;
	} else {
		parse_dsp_data(data->addr, runtime);
	}

free_list_node:
	list_del(&data->node);
	kfree(data->addr);
	kfree(data);
}

static void hook_should_stop(void)
{
	enum framer_type framer;

	framer = codec_bus_get_framer_type();
	if ((framer != FRAMER_CODEC) || da_combine_check_i2s2_clk())
		da_combine_stop_dsp_hook();
}

static int add_data_to_list(struct hook_runtime *runtime)
{
	int ret = 0;
	void *buffer = NULL;
	unsigned int id;
	struct data_flow *data = NULL;

	buffer = kzalloc(runtime->size, GFP_KERNEL);
	if (buffer == NULL) {
		AUDIO_LOGE("malloc failed");
		return -ENOMEM;
	}

	data = kzalloc(sizeof(struct data_flow), GFP_KERNEL);
	if (data == NULL) {
		AUDIO_LOGE("malloc failed");
		ret = -ENOMEM;
		goto err_exit;
	}

	id = get_idle_buffer_id(runtime);
	memcpy(buffer, runtime->buffer[id], runtime->size);

	if (runtime->idle_id != get_idle_buffer_id(runtime))
		AUDIO_LOGW("dma buffer is changed");

	data->addr = buffer;

	list_add_tail(&data->node, &runtime->data_list.node);

	return 0;

err_exit:
	kfree(buffer);

	return ret;
}

static int left_data_parse_thread(void *p)
{
	int ret;
	struct da_combine_hook_priv *priv = hook_priv;
	struct hook_runtime *runtime = NULL;

	runtime = &priv->runtime[HOOK_LEFT];

	while (!runtime->kthread_should_stop) {
		ret = down_interruptible(&runtime->hook_proc_sema);
		if (ret == -ETIME)
			AUDIO_LOGE("proc sema down int err -ETIME");

		ret = down_interruptible(&runtime->hook_stop_sema);
		if (ret == -ETIME)
			AUDIO_LOGE("stop sema down int err -ETIME");

		if (!priv->started || priv->standby) {
			up(&runtime->hook_stop_sema);
			AUDIO_LOGE("parse data when hook stopped");
			continue;
		}

		__pm_stay_awake(runtime->wake_lock);
		add_data_to_list(runtime);
		parse_data(runtime);
		if (runtime->idle_id != get_idle_buffer_id(runtime))
			AUDIO_LOGW("dma buffer is changed");

		__pm_relax(runtime->wake_lock);

		up(&runtime->hook_stop_sema);

		hook_should_stop();
	}

	return 0;
}

static int right_data_parse_thread(void *p)
{
	int ret;
	struct da_combine_hook_priv *priv = hook_priv;
	struct hook_runtime *runtime = NULL;

	runtime = &priv->runtime[HOOK_RIGHT];

	while (!runtime->kthread_should_stop) {
		ret = down_interruptible(&runtime->hook_proc_sema);
		if (ret == -ETIME)
			AUDIO_LOGE("proc sema down int err -ETIME");

		ret = down_interruptible(&runtime->hook_stop_sema);
		if (ret == -ETIME)
			AUDIO_LOGE("stop sema down int err -ETIME");

		if (!priv->started || priv->standby) {
			up(&runtime->hook_stop_sema);
			AUDIO_LOGE("parse data when hook stopped");
			continue;
		}

		__pm_stay_awake(runtime->wake_lock);

		add_data_to_list(runtime);
		parse_data(runtime);
		if (runtime->idle_id != get_idle_buffer_id(runtime))
			AUDIO_LOGW("dma buffer is changed");

		__pm_relax(runtime->wake_lock);

		up(&runtime->hook_stop_sema);
	}

	return 0;
}

static int init_para(unsigned int codec_type, enum bustype_select bus_sel)
{
	strncpy(hook_priv->hook_path, HOOK_PATH_DEFAULT,
		sizeof(hook_priv->hook_path));
	hook_priv->sponsor = OM_SPONSOR_DEFAULT;
	hook_priv->bandwidth = OM_BANDWIDTH_6144;
	hook_priv->dir_count = HOOK_DEFAULT_SUBDIR_CNT;
	hook_priv->codec_type = codec_type;
	hook_priv->bus_sel = bus_sel;

#ifdef ENABLE_DA_COMBINE_HIFI_DEBUG
	hook_priv->is_eng = true;
#else
	hook_priv->is_eng = false;
#endif
	hook_priv->started = false;
	hook_priv->standby = true;
	hook_priv->should_deactive = true;

	return 0;
}

static int init_thread(void)
{
	unsigned int i;
	int ret;
	struct sched_param param;

	for (i = 0; i < HOOK_CNT; i++) {
		hook_priv->runtime[i].hook_id = i;

		INIT_LIST_HEAD(&hook_priv->runtime[i].info_list.node);
		INIT_LIST_HEAD(&hook_priv->runtime[i].data_list.node);

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,4,0))
		hook_priv->runtime[i].wake_lock = wakeup_source_register(NULL,
			"da_combine_dsp_hook-resmgr");
#else
		hook_priv->runtime[i].wake_lock = wakeup_source_register("da_combine_dsp_hook-resmgr");
#endif
		if (hook_priv->runtime[i].wake_lock == NULL) {
			AUDIO_LOGE("request wakeup source failed: %u", i);
			ret = -ENOMEM;
			goto init_err;
		}

		sema_init(&hook_priv->runtime[i].hook_proc_sema, 0);
		sema_init(&hook_priv->runtime[i].hook_stop_sema, 1);

		if (i == HOOK_RIGHT)
			hook_priv->runtime[i].kthread = kthread_create(
				right_data_parse_thread, 0,
				"right_data_parse_thread");
		else
			hook_priv->runtime[i].kthread = kthread_create(
				left_data_parse_thread, 0,
				"left_data_parse_thread");

		if (IS_ERR(hook_priv->runtime[i].kthread)) {
			AUDIO_LOGE("create data parse thread fail:%u", i);
			ret = -EINVAL;
			goto init_err;
		}

		hook_priv->runtime[i].kthread_should_stop = 0;

		/* set high prio */
		memset(&param, 0, sizeof(struct sched_param));
		param.sched_priority = MAX_RT_PRIO - HOOK_THREAD_PRIO;
		ret = sched_setscheduler(hook_priv->runtime[i].kthread,
			SCHED_RR, &param);
		if (ret)
			AUDIO_LOGE("set thread schedule priorty failed:%u", i);
	}

	wake_up_process(hook_priv->runtime[HOOK_LEFT].kthread);
	wake_up_process(hook_priv->runtime[HOOK_RIGHT].kthread);

	return 0;

init_err:
	for (i = 0; i < HOOK_CNT; i++) {
		if (hook_priv->runtime[i].kthread != NULL) {
			kthread_stop(hook_priv->runtime[i].kthread);
			hook_priv->runtime[i].kthread = NULL;
		}
		wakeup_source_unregister(hook_priv->runtime[i].wake_lock);
	}

	return ret;
}

static void deinit_thread(void)
{
	int i;

	for (i = 0; i < HOOK_CNT; i++) {
		if (hook_priv->runtime[i].kthread) {
			hook_priv->runtime[i].kthread_should_stop = 1;
			up(&hook_priv->runtime[i].hook_proc_sema);
			kthread_stop(hook_priv->runtime[i].kthread);
			hook_priv->runtime[i].kthread = NULL;
		}

		wakeup_source_unregister(hook_priv->runtime[i].wake_lock);
	}
}

static bool is_file_valid(const char *buf, const char *path, unsigned int size)
{
	struct da_combine_hook_priv *priv = hook_priv;

	if (priv == NULL) {
		AUDIO_LOGE("hook priv is null");
		return false;
	}

	if (path == NULL || buf == NULL || size == 0) {
		AUDIO_LOGE("para err");
		return false;
	}

	return true;
}

int da_combine_dsp_create_hook_dir(const char *path)
{
	char tmp_path[HOOK_PATH_MAX_LENGTH] = {0};

	if (path == NULL) {
		AUDIO_LOGE("path is null");
		return -EINVAL;
	}

	strncpy(tmp_path, path,  sizeof(tmp_path) - 1);

	if (audio_create_dir(tmp_path, HOOK_PATH_MAX_LENGTH - 1))
		return -EFAULT;

	return 0;
}

void da_combine_dsp_dump_to_file(const char *buf, unsigned int size,
	const char *path)
{
	unsigned int file_flag = O_RDWR;
	mm_segment_t fs;
	struct file *fp = NULL;
	struct kstat file_stat;
	struct da_combine_hook_priv *priv = hook_priv;

	if(!is_file_valid(buf, path, size))
		return;

	fs = get_fs();
	set_fs(KERNEL_DS);

	if (vfs_stat(path, &file_stat) == 0 &&
		file_stat.size > HOOK_MAX_FILE_SIZE) {
		/* delete file */
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4,19,0))
		ksys_unlink(path);
#else
		sys_unlink(path);
#endif

		AUDIO_LOGI("delete too large file");
	}

	if (vfs_stat(path, &file_stat) < 0) {
		if (priv->open_file_failed_cnt < PRINT_MAX_CNT)
			AUDIO_LOGI("create file");

		file_flag |= O_CREAT;
	}

	fp = filp_open(path, file_flag, 0660);
	if (IS_ERR(fp)) {
		if (priv->open_file_failed_cnt < PRINT_MAX_CNT) {
			AUDIO_LOGE("open file fail");
			priv->open_file_failed_cnt++;
		}
		fp = NULL;
		goto END;
	}

	vfs_llseek(fp, 0L, SEEK_END);

	if (vfs_write(fp, buf, size, &fp->f_pos) < 0) /*lint !e613*/
		AUDIO_LOGE("write file fail");

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4,19,0))
	if (ksys_chown((const char __user *)path, ROOT_UID, SYSTEM_GID))
#else
	if (sys_chown((const char __user *)path, ROOT_UID, SYSTEM_GID))
#endif
		AUDIO_LOGE("chown path failed");

END:
	if (fp != NULL)
		filp_close(fp, 0);

	set_fs(fs);
}

void da_combine_stop_hook_route(void)
{
	struct da_combine_hook_priv *priv = hook_priv;

	if (priv == NULL) {
		AUDIO_LOGE("priv is null");
		return;
	}

	if (!priv->started || priv->standby)
		return;

	stop_hook(HOOK_LEFT);
	AUDIO_LOGI("left hook stoped");

	if (priv->bandwidth == OM_BANDWIDTH_12288) {
		stop_hook(HOOK_RIGHT);
		AUDIO_LOGI("right hook stoped");
	}

	deactivate_codec_bus();

	priv->sponsor = OM_SPONSOR_DEFAULT;
}

/* stop hook route and notify dsp */
void da_combine_stop_dsp_hook(void)
{
	int ret;
	struct om_stop_hook_msg stop_hook_msg;
	union da_combine_high_freq_status high_status;

	if (hook_priv == NULL) {
		AUDIO_LOGE("priv is null");
		return;
	}

	high_status.val = da_combine_get_high_freq_status();
	if (high_status.om_hook == 0) {
		AUDIO_LOGW("om hook is not opened");
		return;
	}

	stop_hook_msg.msg_id = ID_AP_DSP_HOOK_STOP;

	da_combine_stop_hook_route();

	ret = da_combine_sync_write(&stop_hook_msg,
		(unsigned int)sizeof(struct om_stop_hook_msg));
	if (ret != 0) {
		AUDIO_LOGE("sync write failed");
		goto end;
	}

	hook_priv->is_dspif_hooking = false;
	AUDIO_LOGI("hook stopped");

end:
	(void)da_combine_release_pll_resource(HIGH_FREQ_SCENE_OM_HOOK);
}

void da_combine_stop_dspif_hook(void)
{
	if (hook_priv == NULL) {
		AUDIO_LOGE("priv is null");
		return;
	}

	if (hook_priv->is_dspif_hooking) {
		AUDIO_LOGW("dsp scene will start, stop hooking dspif data");
		da_combine_stop_dsp_hook();
	}
}

/*
 * this function is called by sync ap msg proc or dsp msg proc and
 * needs to check the scene and param
 */
int da_combine_stop_hook(const struct da_combine_param_io_buf *param)
{
#ifdef ENABLE_DA_COMBINE_HIFI_DEBUG
	union da_combine_high_freq_status high_status;

	high_status.val = da_combine_get_high_freq_status();
	if (high_status.om_hook == 0) {
		AUDIO_LOGW("om hook is not opened");
		return -EINVAL;
	}

	if (!param || !param->buf_in) {
		AUDIO_LOGE("null param");
		da_combine_release_pll_resource(HIGH_FREQ_SCENE_OM_HOOK);
		return -EINVAL;
	}

	if (param->buf_size_in == 0 || param->buf_size_in > MAX_PARA_SIZE ||
		param->buf_size_in < sizeof(struct om_set_bandwidth_msg)) {
		AUDIO_LOGE("err param size:%d", param->buf_size_in);
		da_combine_release_pll_resource(HIGH_FREQ_SCENE_OM_HOOK);
		return -EINVAL;
	}

	da_combine_stop_dsp_hook();
#endif

	return 0;
}

int da_combine_start_hook(const struct da_combine_param_io_buf *param)
{
	int ret = 0;
#ifdef ENABLE_DA_COMBINE_HIFI_DEBUG
	const struct om_start_hook_msg *start_hook_msg = NULL;

	if (hook_priv == NULL) {
		AUDIO_LOGE("priv is null");
		return -EINVAL;
	}

	if (check_hook_para(param))
		return -EINVAL;

	if (check_hook_context(param))
		return -EINVAL;

	ret = da_combine_request_pll_resource(HIGH_FREQ_SCENE_OM_HOOK);
	if (ret != 0)
		goto end;

	ret = start_hook_route();
	if (ret != 0) {
		AUDIO_LOGE("start om hook failed");
		da_combine_release_pll_resource(HIGH_FREQ_SCENE_OM_HOOK);
		goto end;
	}

	start_hook_msg = (struct om_start_hook_msg *)param->buf_in;
	ret = da_combine_sync_write(param->buf_in, sizeof(*start_hook_msg) +
		start_hook_msg->para_size);
	if (ret != 0) {
		AUDIO_LOGE("sync write failed");
		da_combine_stop_hook_route();
		da_combine_release_pll_resource(HIGH_FREQ_SCENE_OM_HOOK);
		goto end;
	}

	return ret;

end:
	hook_priv->is_dspif_hooking = false;
#endif

	return ret;
}

int da_combine_set_dsp_hook_bw(unsigned short bandwidth)
{
	struct da_combine_hook_priv *priv = hook_priv;

	if (priv == NULL) {
		AUDIO_LOGE("priv is null");
		return -EINVAL;
	}

	if (bandwidth >= OM_BANDWIDTH_BUTT) {
		AUDIO_LOGE("err om bw:%d", bandwidth);
		return -EINVAL;
	}

	if (priv->started || !priv->standby) {
		AUDIO_LOGE("om is running, forbid set bw:%d", bandwidth);
		return -EBUSY;
	}

	priv->bandwidth = bandwidth;

	AUDIO_LOGI("set om bw:%d", bandwidth);

	return 0;
}

int da_combine_set_dsp_hook_sponsor(unsigned short sponsor)
{
	struct da_combine_hook_priv *priv = hook_priv;

	if (priv == NULL) {
		AUDIO_LOGE("priv is null");
		return -EINVAL;
	}

	if (sponsor >= OM_SPONSOR_BUTT) {
		AUDIO_LOGE("err om sponsor:%d", sponsor);
		return -EINVAL;
	}

	if (priv->started || !priv->standby) {
		AUDIO_LOGE("hook is running, forbid set sponsor:%d", sponsor);
		return -EBUSY;
	}

	priv->sponsor = sponsor;

	AUDIO_LOGI("set om sponsor:%d", sponsor);

	return 0;
}

int da_combine_set_hook_path(const struct da_combine_param_io_buf *param)
{
	int ret;
	struct om_set_hook_path_msg *set_path = NULL;
	struct da_combine_hook_priv *priv = hook_priv;

	if (priv == NULL) {
		AUDIO_LOGE("priv is null");
		return -EINVAL;
	}

	ret = check_param_and_scene(param,
		sizeof(struct om_set_hook_path_msg));
	if (ret != 0)
		return ret;

	set_path = (struct om_set_hook_path_msg *)param->buf_in;
	if (set_path->size > (HOOK_PATH_MAX_LENGTH - 2)) {
		AUDIO_LOGE("err para");
		return -EINVAL;
	}

	if (priv->started || !priv->standby) {
		AUDIO_LOGE("om is running, forbid set path");
		return -EBUSY;
	}

	AUDIO_LOGI("ok");

	return ret;
}

#ifdef ENABLE_DA_COMBINE_HIFI_DEBUG
int da_combine_set_hook_bw(const struct da_combine_param_io_buf *param)
{
	int ret;
	const struct om_set_bandwidth_msg *set_bw = NULL;

	ret = check_param_and_scene(param,
		sizeof(struct om_set_bandwidth_msg));
	if (ret != 0)
		return ret;

	(void)da_combine_request_pll_resource(HIGH_FREQ_SCENE_OM_HOOK);

	set_bw = (struct om_set_bandwidth_msg *)param->buf_in;
	ret = da_combine_set_dsp_hook_bw(set_bw->bw);
	if (ret != 0)
		return ret;

	ret = da_combine_sync_write(param->buf_in, param->buf_size_in);
	if (ret != 0) {
		AUDIO_LOGE("sync write failed");
		return ret;
	}

	da_combine_release_pll_resource(HIGH_FREQ_SCENE_OM_HOOK);

	return ret;
}

int da_combine_set_hook_sponsor(const struct da_combine_param_io_buf *param)
{
	int ret;
	struct om_set_hook_sponsor_msg *set_sponsor = NULL;

	ret = check_param_and_scene(param,
		sizeof(struct om_set_hook_sponsor_msg));
	if (ret != 0)
		return ret;

	set_sponsor = (struct om_set_hook_sponsor_msg *)param->buf_in;
	ret = da_combine_set_dsp_hook_sponsor(set_sponsor->sponsor);

	return ret;
}

int da_combine_set_dir_count(const struct da_combine_param_io_buf *param)
{
	int ret;
	struct om_set_dir_count_msg *set_dir_count = NULL;

	ret = check_param_and_scene(param,
		sizeof(struct om_set_dir_count_msg));
	if (ret != 0)
		return ret;

	set_dir_count = (struct om_set_dir_count_msg *)param->buf_in; /*lint !e826*/
	ret = set_dsp_hook_dir_count(set_dir_count->count);

	return ret;
}

int da_combine_start_mad_test(const struct da_combine_param_io_buf *param)
{
	int ret;

	ret = da_combine_check_msg_para(param, sizeof(struct mad_test_stru));
	if (ret != 0)
		return ret;

	da_combine_stop_dspif_hook();

	ret = da_combine_request_pll_resource(HIGH_FREQ_SCENE_MAD_TEST);
	if (ret != 0)
		return ret;

	ret = da_combine_sync_write(param->buf_in, param->buf_size_in);
	if (ret != 0) {
		AUDIO_LOGE("sync write failed");
		da_combine_release_pll_resource(HIGH_FREQ_SCENE_MAD_TEST);
		return ret;
	}

	return ret;
}

int da_combine_stop_mad_test(const struct da_combine_param_io_buf *param)
{
	int ret;
	union da_combine_high_freq_status high_status;

	ret = da_combine_check_msg_para(param, sizeof(struct mad_test_stru));
	if (ret != 0)
		return ret;

	high_status.val = da_combine_get_high_freq_status();
	if (high_status.mad_test == 0) {
		AUDIO_LOGE("scene mad test is not opened");
		return -EINVAL;
	}

	ret = da_combine_sync_write(param->buf_in, param->buf_size_in);
	if (ret != 0)
		AUDIO_LOGE("sync write failed");

	da_combine_release_pll_resource(HIGH_FREQ_SCENE_MAD_TEST);

	return ret;
}

int da_combine_wakeup_test(const struct da_combine_param_io_buf *param)
{
	int ret;
	union da_combine_low_freq_status low_status;

	ret = da_combine_check_msg_para(param, sizeof(struct mad_test_stru));
	if (ret != 0)
		return ret;

	low_status.val = da_combine_get_low_freq_status();
	if (low_status.wake_up == 0) {
		AUDIO_LOGI("scene wakeup is not opened, status:%d",
			low_status.val);
		return -EINVAL;
	}

	ret = da_combine_sync_write(param->buf_in, param->buf_size_in);
	if (ret != 0)
		AUDIO_LOGE("sync write failed, ret:%d", ret);

	return ret;
}
#endif

int da_combine_dsp_hook_init(struct da_combine_irq *irqmgr,
	unsigned int codec_type, enum bustype_select bus_sel)
{
	int ret;
	struct device *dev = irqmgr->dev;

	AUDIO_LOGI("init begin");
#ifdef ENABLE_DA_COMBINE_HIFI_DEBUG
#ifdef CONFIG_AUDIO_COMMON_IMAGE
	bind_dma_addr();
	bind_dma_cfg();
#endif
#endif

	hook_priv = kzalloc(sizeof(*hook_priv), GFP_KERNEL);
	if (hook_priv == NULL) {
		AUDIO_LOGE("dsp hook malloc failed");
		return -ENOMEM;
	}

	ret = init_para(codec_type, bus_sel);
	if (ret != 0)
		goto err_exit;

	hook_priv->asp_subsys_clk = devm_clk_get(dev, "clk_asp_subsys");
	if (IS_ERR(hook_priv->asp_subsys_clk)) {
		AUDIO_LOGE("asp subsys clk fail");
		ret = -EINVAL;
		goto err_exit;
	}

	ret = init_thread();
	if (ret != 0)
		goto err_exit;

	AUDIO_LOGI("init end");

	return 0;

err_exit:
	kfree(hook_priv);
	hook_priv = NULL;
	AUDIO_LOGE("init failed");

	return ret;
}

void da_combine_dsp_hook_deinit(void)
{
	if (hook_priv == NULL)
		return;

	deinit_thread();

	kfree(hook_priv);
	hook_priv = NULL;
}

