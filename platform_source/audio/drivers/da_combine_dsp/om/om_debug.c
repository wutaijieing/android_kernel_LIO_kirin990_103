/*
 * om_debug.c
 *
 * debug for da_combine codec
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

#include "om_debug.h"

#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/proc_fs.h>
#include <linux/rtc.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <platform_include/basicplatform/linux/rdr_pub.h>
#include <linux/platform_drivers/da_combine/da_combine_utils.h>
#include <linux/platform_drivers/da_combine_dsp/da_combine_dsp_misc.h>
#include <linux/platform_drivers/da_combine/da_combine_dsp_regs.h>
#include <linux/platform_drivers/da_combine/da_combine_mad.h>
#include <da_combine_dsp_interface.h>
#include <rdr_audio_codec.h>
#include <rdr_audio_adapter.h>
#include <linux/io.h>

#include "audio_log.h"
#include "download_image.h"
#include "dsp_utils.h"
#include "audio_file.h"
#include "om_hook.h"
#include "om.h"

#define LOG_TAG "om_debug"

#define CODECHIFI_TOTAL_LOG_PATH "/data/log/codechifi_total_logs/"
#define CODECHIFI_TOTAL_LOG_FILE "codechifi_log.bin"
#define WAKEUP_DUMP_DIR "/data/log/wakeup_pcm/"
#define WAKEUP_DUMP_FILE_NAME "codec_pcm.bin"
#define DA_COMBINEDEBUG_PROC_FILE "codecdebug"
#define OM_DA_COMBINE_DUMP_IRAM_NAME "iram.bin"
#define OM_DA_COMBINE_DUMP_DRAM_NAME "dram.bin"
#define DA_COMBINEDEBUG_PATH "codecdbg"
#define WAKEUP_PCM_BLOCK_SIZE 2880
#define WAKEUP_PCM_DATA_SIZE 63360
#define WAKEUP_PCM_SAMPLE_RATE 16000
#define WAKEUP_PCM_FORMAT 2
#define WAKEUP_PCM_CHANNELS 1
#define WAKEUP_NAME_MAX_LEN 50
#define WAKEUP_MAX_PCM_SIZE 0x80000000 /* 2G */
#define TOTAL_LOG_FILE_MAX_SIZE 0x400000 /* 4M */
#define TOTAL_LOG_DIR_MAX_SIZE 0x6400000 /* 100M */
#define FILE_MODE_RW 0660
#define MAX_STR_LEN 64
#define MAX_PARAM_SIZE 2
#define DUMP_DIR_LEN 128
#define MEM_CHECK_MAGIC_NUM1 0x5a5a5a5a
#define MEM_CHECK_MAGIC_NUM2 0xa5a5a5a5
#define SEXAGESIMAL_TIME 60
#define BACKGROUND_DATA_NUM 12
#define DATA_REPEAT_TIMES 2

#define BIN_2_GRAY(x) (unsigned int)(((x) >> 1) ^ (x))
#define WIDTH_4_BYTE 4

#define DEBUG_INFO_STR "0:dump log\n" \
	"1:check memory normally\n"    \
	"2:check memory by march ic\n" \
	"3:dump all ram\n"             \
	"100:exception pointer (addr val)\n" \
	"102:soft irq\n"               \
	"103:call exit\n"              \
	"111:pa alg bypass(1:bypass 0:not bypass)\n" \
	"115:data statistic\n"         \
	"116:wakeup om switch(1:on 0:off)\n" \
	"117:check ocram\n"            \
	"118:check tcm\n"              \
	"119:check icache\n"           \
	"120:check dcache\n"           \
	"121:open total log dump\n"    \
	"122:close total log dump\n"   \
	"200:exit\n"

enum DA_COMBINE_DSP_DEBUG {
	DA_COMBINE_DSP_DUMP_LOG,
	DA_COMBINE_DSP_CHECK_MEM_NORMALLY,
	DA_COMBINE_DSP_CHECK_MEM_BY_MARCH_IC,
	DA_COMBINE_DSP_DUMP_ALL_RAM,

	/* the follows add for dsp debug */
	DA_COMBINE_DSP_EXCEPTION_POINTER_INJECT = 100,
	DA_COMBINE_DSP_SOFT_IRQ_INJECT = 102,
	DA_COMBINE_DSP_CALL_EXIT_INJECT,
	DA_COMBINE_DSP_SMARTPA_ALG_BYPASS = 111,
	DA_COMBINE_DSP_DATA_STATISTIC = 115,
	DA_COMBINE_DSP_WAKEUP_OM_SWITCH,
	DA_COMBINE_DSP_CHECK_OCRAM,
	DA_COMBINE_DSP_CHECK_TCM,
	DA_COMBINE_DSP_CHECK_ICACHE,
	DA_COMBINE_DSP_CHECK_DCACHE,
	DA_COMBINE_DSP_OPEN_TOTALLOG_DUMP,
	DA_COMBINE_DSP_CLOSE_TOTALLOG_DUMP,
	DA_COMBINE_DEBUG_EXIT = 200,
};

struct da_combine_dsp_debug {
	struct mutex debug_store_mutex;
	struct mutex wakeup_hook_stop_mutex;
	struct wakeup_source *log_save_lock;
	struct wakeup_source *wakeup_hook_lock;
	struct task_struct *kwakeup_hook_task;
	struct da_combine_dsp_config dsp_config;
	enum wakeup_hook_pcm_status wakeup_hook_status;
	unsigned long long mad_time;
	unsigned long long vad_time;
	unsigned long long suspend_time;
	unsigned long long pre_hook_time;
	unsigned long long cur_hook_time;
	unsigned int wakeup_pcm_hook_addr;
	bool is_wakeup_hooking;
	bool is_save_dsp_total_log;
};

struct reg_rw_struct {
	unsigned int reg;
	unsigned int val;
};

struct dsp_debug_msg {
	unsigned short msg_id;
	unsigned short debug_type;
	unsigned int param_size;
	unsigned int param[MAX_PARAM_SIZE];
};

static struct da_combine_dsp_debug *priv_debug;
static struct proc_dir_entry *da_combine_debug_dir;

static inline unsigned int pcm_ms_to_bytes(unsigned int time_ms)
{
	return (WAKEUP_PCM_SAMPLE_RATE * WAKEUP_PCM_FORMAT *
		WAKEUP_PCM_CHANNELS * (time_ms) / 1000);
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,4,0))
static inline long long timespec_to_ms(struct timespec64 tv)
{
	return 1000 * tv.tv_sec + tv.tv_nsec / 1000 / 1000;
}
#else
static inline long long timeval_to_ms(struct timeval tv)
{
	return 1000 * tv.tv_sec + tv.tv_usec / 1000;
}
#endif

static int check_mem_ascendingly(unsigned int start_addr,
	unsigned int temp_size, unsigned int mem_size, const unsigned int *write_data)
{
	int ret = 0;
	unsigned int off_set;
	unsigned int read_data;
	unsigned int off_set_gray = 0;

	/*
	 * Step1 ascend even set write background number,
	 * the odd address write the contrary number
	 */
	for (off_set = 0; off_set < temp_size; off_set++) {
		off_set_gray = BIN_2_GRAY(off_set) * WIDTH_4_BYTE;
		if (off_set_gray > mem_size - WIDTH_4_BYTE)
			continue;

		da_combine_dsp_write_reg((start_addr + off_set_gray),
			write_data[off_set % 2]);
	}

	/*
	 * Step2 ascend even set read DB and write DB,
	 * odd set read DB and write DB
	 */
	for (off_set = 0; off_set < temp_size; off_set++) {
		off_set_gray = BIN_2_GRAY(off_set) * WIDTH_4_BYTE;
		if (off_set_gray > mem_size - WIDTH_4_BYTE)
			continue;

		read_data = da_combine_dsp_read_reg(start_addr + off_set_gray);
		if (write_data[off_set % 2] != read_data) {
			/* read wrong information */
			AUDIO_LOGE("Step2 Addr:0x%pK, value:0x%x, should be 0x%x",
				(void *)(uintptr_t)(start_addr + off_set_gray),
				read_data, write_data[off_set % 2]);
			ret++;
			mdelay(1);
		}
		da_combine_dsp_write_reg((start_addr + off_set_gray),
			write_data[(off_set + 1) % 2]);
	}

	/*
	 * Step3 ascend even set read iDB and write DB,
	 * odd set read DB and write iDB
	 */
	for (off_set = 0; off_set < temp_size; off_set++) {
		off_set_gray = BIN_2_GRAY(off_set) * WIDTH_4_BYTE;
		if (off_set_gray > mem_size - WIDTH_4_BYTE)
			continue;

		read_data = da_combine_dsp_read_reg(start_addr + off_set_gray);
		if (write_data[(off_set + 1) % 2] != read_data) {
			/* record wrong information */
			AUDIO_LOGE("Step3 Addr:0x%pK, value:0x%x, should be 0x%x",
				(void *)(uintptr_t)(start_addr + off_set_gray),
				read_data, write_data[(off_set + 1) % 2]);
			ret++;
			mdelay(1);
		}

		da_combine_dsp_write_reg((start_addr + off_set_gray),
			write_data[off_set % 2]);
	}

	return ret;
}

static int check_mem_descendingly(unsigned int start_addr,
	unsigned int temp_size, unsigned int mem_size, const unsigned int *write_data)
{
	int ret = 0;
	unsigned int off_set;
	unsigned int read_data;
	unsigned int off_set_gray = 0;

	/*
	 * from Ascending to Descending, the last space need to
	 * write to MEM by cache and invalid.
	 * step4 the Descending even set read DB and write iDB,
	 * the odd set read iDB and write DB
	 */
	for (off_set = temp_size; off_set > 0; off_set--) {
		off_set_gray = BIN_2_GRAY(off_set) * WIDTH_4_BYTE;
		if (off_set_gray > mem_size - WIDTH_4_BYTE)
			continue;

		read_data = da_combine_dsp_read_reg(start_addr + off_set_gray);
		if (write_data[off_set % 2] != read_data) {
			/* record wrong information */
			AUDIO_LOGE("Step4 Addr:0x%pK, value:0x%x, should be 0x%x",
				(void *)(uintptr_t)(start_addr + off_set_gray),
				read_data, write_data[off_set % 2]);
			ret++;
			mdelay(1);
		}

		da_combine_dsp_write_reg((start_addr + off_set_gray),
			write_data[(off_set + 1) % 2]);
	}

	/*
	 * step5 descending even set read iDB and write DB,
	 * the odd set read DB and write iDB
	 */
	for (off_set = temp_size; off_set > 0; off_set--) {
		off_set_gray = BIN_2_GRAY(off_set) * WIDTH_4_BYTE;
		if (off_set_gray > mem_size - WIDTH_4_BYTE)
			continue;

		read_data = da_combine_dsp_read_reg(start_addr + off_set_gray);
		if (write_data[(off_set + 1) % 2] != read_data) {
			/* record wrong information */
			AUDIO_LOGE("Step5 Addr:0x%pK, value:0x%x, should be 0x%x",
				(void *)(uintptr_t)(start_addr + off_set_gray),
				read_data, write_data[(off_set + 1) % 2]);
			ret++;
			mdelay(1);
		}

		da_combine_dsp_write_reg((start_addr + off_set_gray),
			write_data[off_set % 2]);
	}

	/* Step6 descending even set read iDB and the odd set read DB */
	for (off_set = temp_size; off_set > 0; off_set--) {
		off_set_gray = BIN_2_GRAY(off_set) * WIDTH_4_BYTE;
		if (off_set_gray > mem_size - WIDTH_4_BYTE)
			continue;

		read_data = da_combine_dsp_read_reg(start_addr + off_set_gray);
		if (write_data[off_set % 2] != read_data) {
			/* record wrong information */
			AUDIO_LOGE("Step6 Addr:0x%pK, value:0x%x, should be 0x%x",
				(void *)(uintptr_t)(start_addr + off_set_gray),
				read_data, write_data[off_set % 2]);
			ret++;
			mdelay(1);
		}
	}

	return ret;
}

static int marchic_check(unsigned int addr, unsigned int len)
{
	int ret = 0;
	unsigned int i;
	unsigned int count;
	unsigned int temp_size = 0;
	unsigned int data_num = BACKGROUND_DATA_NUM;
	const unsigned int *write_data = NULL;
	const unsigned int background_data[BACKGROUND_DATA_NUM][DATA_REPEAT_TIMES] = {
		{0x55555555, 0xAAAAAAAA},
		{0xAAAAAAAA, 0x55555555},
		{0x00000000, 0xFFFFFFFF},
		{0xFFFFFFFF, 0x00000000},
		{0x33333333, 0xCCCCCCCC},
		{0xCCCCCCCC, 0x33333333},
		{0x0F0F0F0F, 0xF0F0F0F0},
		{0xF0F0F0F0, 0x0F0F0F0F},
		{0x00FF00FF, 0xFF00FF00},
		{0xFF00FF00, 0x00FF00FF},
		{0x0000FFFF, 0xFFFF0000},
		{0xFFFF0000, 0x0000FFFF}
	};

	AUDIO_LOGI("teststart, addr is 0x%x, len is 0x%x", addr, len);

	if (addr == 0) {
		AUDIO_LOGE("addr is null");
		return -EINVAL;
	}

	/* calculate the closest space size of 2 Power to the power */
	for (count = 0; temp_size < len; count++)
		temp_size = (1 << count);

	temp_size = temp_size / WIDTH_4_BYTE;
	AUDIO_LOGI("temp size: 0x%x\n", temp_size);

	for (i = 0; i < data_num; i++) {
		write_data = &background_data[i][0];
		/* Ascending test */
		ret = check_mem_ascendingly(addr, temp_size, len, write_data);
		if (ret != 0) {
			AUDIO_LOGE("ascend check error");
			return ret;
		}
		/* Descending test */
		ret = check_mem_descendingly(addr, temp_size, len, write_data);
		if (ret != 0) {
			AUDIO_LOGE("descend check error");
			return ret;
		}
	}

	AUDIO_LOGI("marchIC exit");

	return ret;
}

static void check_memory_by_march_ic(void)
{
	int ret;

	if (da_combine_request_pll_resource(HIGH_FREQ_SCENE_MEM_CHECK)) {
		AUDIO_LOGI("request pll fail");
		return;
	}

	AUDIO_LOGI("check ocram begin");
	ret = marchic_check(priv_debug->dsp_config.ocram_start_addr,
		priv_debug->dsp_config.ocram_size);
	if (ret)
		AUDIO_LOGE("dsp marchic check ocram fail");
	AUDIO_LOGI("check ocram end");

	AUDIO_LOGI("check itcm begin");
	ret = marchic_check(priv_debug->dsp_config.itcm_start_addr,
		priv_debug->dsp_config.itcm_size);
	if (ret)
		AUDIO_LOGE("dsp marchic check itcm fail");
	AUDIO_LOGI("check itcm end");

	AUDIO_LOGI("check dtcm begin");
	ret = marchic_check(priv_debug->dsp_config.dtcm_start_addr,
		priv_debug->dsp_config.dtcm_size);
	if (ret)
		AUDIO_LOGE("dsp marchic check dtcm fail");
	AUDIO_LOGI("check dtcm end");

	da_combine_release_pll_resource(HIGH_FREQ_SCENE_MEM_CHECK);
}

static int normally_check(const unsigned int start_addr,
	const unsigned int len)
{
	int ret = 0;
	unsigned int i;
	unsigned int cur_addr = start_addr;
	unsigned int original_val;

	if (start_addr == 0 || len == 0) {
		AUDIO_LOGE("start addr or len:%d is illegal", len);
		return -EINVAL;
	}

	for (i = 0; i < len; i += sizeof(unsigned int)) {
		original_val = da_combine_dsp_read_reg(cur_addr);

		da_combine_dsp_write_reg(cur_addr, MEM_CHECK_MAGIC_NUM1);
		if (da_combine_dsp_read_reg(cur_addr) != MEM_CHECK_MAGIC_NUM1) {
			AUDIO_LOGE("dsp memory check fail");
			AUDIO_LOGE("addr:0x%pK, val:0x%x(shoule be 0x5a)",
				(void *)(uintptr_t)cur_addr,
				da_combine_dsp_read_reg(cur_addr));
			ret = -EINVAL;
		}

		da_combine_dsp_write_reg(cur_addr, MEM_CHECK_MAGIC_NUM2);
		if (da_combine_dsp_read_reg(cur_addr) != MEM_CHECK_MAGIC_NUM2) {
			AUDIO_LOGE("dsp memory check fail");
			AUDIO_LOGE("addr:0x%pK, val:0x%x(shoule be 0xa5)",
				(void *)(uintptr_t)cur_addr,
				da_combine_dsp_read_reg(cur_addr));
			ret = -EINVAL;
		}

		da_combine_dsp_write_reg(cur_addr, original_val);
		cur_addr += sizeof(unsigned int);
	}

	return ret;
}

static void check_memory_normally(void)
{
	if (da_combine_request_pll_resource(HIGH_FREQ_SCENE_MEM_CHECK)) {
		AUDIO_LOGI("request pll fail");
		return;
	}

	AUDIO_LOGI("check ocram begin");
	if (normally_check(priv_debug->dsp_config.ocram_start_addr,
		priv_debug->dsp_config.ocram_size))
		AUDIO_LOGE("dsp ocram check fail");
	AUDIO_LOGI("check ocram end");

	AUDIO_LOGI("check itcm begin");
	if (normally_check(priv_debug->dsp_config.itcm_start_addr,
		priv_debug->dsp_config.itcm_size))
		AUDIO_LOGE("dsp itcm check fail");
	AUDIO_LOGI("check itcm end");

	AUDIO_LOGI("check dtcm begin");
	if (normally_check(priv_debug->dsp_config.dtcm_start_addr,
		priv_debug->dsp_config.dtcm_size))
		AUDIO_LOGE("dsp dtcm check fail");
	AUDIO_LOGI("check dtcm end");

	da_combine_release_pll_resource(HIGH_FREQ_SCENE_MEM_CHECK);
}

static unsigned int get_pcm_size_by_time(void)
{
	unsigned int interval_ms;
	unsigned int size;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,4,0))
	struct timespec64 tv;

	ktime_get_real_ts64(&tv);
	priv_debug->cur_hook_time = timespec_to_ms(tv);
#else
	struct timeval tv;

	do_gettimeofday(&tv);
	priv_debug->cur_hook_time = timeval_to_ms(tv);
#endif

	if (priv_debug->pre_hook_time != 0 &&
		priv_debug->pre_hook_time < priv_debug->cur_hook_time) {
		interval_ms =
			priv_debug->cur_hook_time - priv_debug->pre_hook_time;
		size = pcm_ms_to_bytes(interval_ms);
	} else {
		size = WAKEUP_PCM_BLOCK_SIZE;
	}

	priv_debug->pre_hook_time = priv_debug->cur_hook_time;

	return size;
}

static int check_wakeup_hook_para(const struct da_combine_param_io_buf *param)
{
	if (param == NULL) {
		AUDIO_LOGE("input param is null");
		return -EINVAL;
	}
	if (param->buf_in == NULL || param->buf_size_in == 0) {
		AUDIO_LOGE("input buf_in or buf_size_in[%u] error",
			param->buf_size_in);
		return -EINVAL;
	}
	if (param->buf_size_in != sizeof(struct wakeup_hook_msg)) {
		AUDIO_LOGE("input size:%u invalid", param->buf_size_in);
		return -EINVAL;
	}

	return 0;
}

int dump_wakeup_pcm(const char *filepath)
{
	char dump_ram_path[DUMP_DIR_LEN] = {0};
	unsigned int addr;
	unsigned int size;
	unsigned int length;
	unsigned int last_size;

	if (priv_debug == NULL)
		return -EINVAL;

	snprintf(dump_ram_path, sizeof(dump_ram_path) - 1, "%s%s",
		filepath, WAKEUP_DUMP_FILE_NAME);

	addr = priv_debug->wakeup_pcm_hook_addr;
	size = get_pcm_size_by_time();
	if (addr > (priv_debug->dsp_config.ocram_start_addr + WAKEUP_PCM_DATA_SIZE) ||
		size > WAKEUP_PCM_DATA_SIZE) {
		AUDIO_LOGE("dump error, dump addr:0x%pK, dump size:%u",
			(void *)(uintptr_t)addr, size);
		return -EINVAL;
	}

	priv_debug->wakeup_pcm_hook_addr += size;

	length = priv_debug->wakeup_pcm_hook_addr - priv_debug->dsp_config.ocram_start_addr;
	if (length >= WAKEUP_PCM_DATA_SIZE) {
		last_size = size - (length - WAKEUP_PCM_DATA_SIZE);
		da_combine_save_log_file(dump_ram_path, DUMP_TYPE_WAKEUP_PCM, addr, last_size);

		/* update remainder data pos */
		addr = priv_debug->dsp_config.ocram_start_addr;
		size = size - last_size;
		priv_debug->wakeup_pcm_hook_addr = addr + size;
		AUDIO_LOGI("buffer read revert");
	}

	return da_combine_save_log_file(dump_ram_path, DUMP_TYPE_WAKEUP_PCM, addr, size);
}

static int wakeup_hook_pcm_thread(void *p)
{
	unsigned int type;
	char pcm_path[WAKEUP_NAME_MAX_LEN + 1] = {0};

	IN_FUNCTION;
	priv_debug->is_wakeup_hooking = true;

	type = bbox_check_edition();
	AUDIO_LOGI("edition type is: %d", type);
	if (type != EDITION_INTERNAL_BETA && type != EDITION_OVERSEA_BETA) {
		AUDIO_LOGE("not beta, do not dump hifi");
		goto exit;
	}

	if (audio_create_rootpath(pcm_path, WAKEUP_DUMP_DIR, sizeof(pcm_path))) {
		AUDIO_LOGE("create log path failed, do not dump");
		goto exit;
	}

	mdelay(100);

	priv_debug->wakeup_pcm_hook_addr = priv_debug->dsp_config.ocram_start_addr;
	while (!kthread_should_stop()) {
		if (da_combine_get_wtdog_state()) {
			AUDIO_LOGE("watchdog come");
			break;
		}
		if (da_combine_get_dsp_poweron_state() != HIFI_STATE_INIT) {
			AUDIO_LOGE("dsp init unfinished");
			break;
		}
		if (priv_debug->wakeup_hook_status == WAKEUP_HOOK_PCM_STOP)
			break;

		if (dump_wakeup_pcm(pcm_path)) {
			AUDIO_LOGE("dump data failed");
			break;
		}

		mdelay(90);
	}

exit:
	priv_debug->is_wakeup_hooking = false;
	OUT_FUNCTION;

	return 0;
}

static void wakeup_hook_thread_stop(void)
{
	mutex_lock(&priv_debug->wakeup_hook_stop_mutex);
	if (priv_debug->is_wakeup_hooking &&
		priv_debug->kwakeup_hook_task != NULL) {
		kthread_stop(priv_debug->kwakeup_hook_task);
		priv_debug->kwakeup_hook_task = NULL;
	}
	mutex_unlock(&priv_debug->wakeup_hook_stop_mutex);
}

static int rename_total_log_file(const char *path_name, unsigned int size)
{
	int ret;
	char log_file_with_time[DUMP_DIR_LEN] = {0};
	struct rtc_time tm;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,4,0))
	struct timespec64 tv;
#else
	struct timeval tv;
#endif
	memset(&tm, 0, sizeof(tm));
	memset(&tv, 0, sizeof(tv));

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,4,0))
	ktime_get_real_ts64(&tv);
#else
	do_gettimeofday(&tv);
#endif
	tv.tv_sec -= (long)sys_tz.tz_minuteswest * SEXAGESIMAL_TIME;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,10,0))
	rtc_time64_to_tm(tv.tv_sec, &tm);
#else
	rtc_time_to_tm(tv.tv_sec, &tm);
#endif
	snprintf(log_file_with_time, sizeof(log_file_with_time),
		"%s%04d%02d%02d%02d%02d%02d_%s",
		CODECHIFI_TOTAL_LOG_PATH,
		tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
		tm.tm_hour, tm.tm_min, tm.tm_sec,
		CODECHIFI_TOTAL_LOG_FILE);
	ret = audio_rename_dir(path_name, size,
		log_file_with_time, sizeof(log_file_with_time));
	if (ret != 0) {
		AUDIO_LOGE("rename path name fail");
		return -EINVAL;
	}

	return 0;
}

static void debug_dump_all_ram(void)
{
	int ret;
	char path[DUMP_DIR_LEN] = {0};
	char *dirname = kzalloc((unsigned long)DUMP_DIR_LEN, GFP_ATOMIC);

	if (dirname == NULL) {
		AUDIO_LOGE("dirname alloc fail");
		return;
	}
	snprintf(path, sizeof(path) - 1, "%s%s", PATH_ROOT, "codecdump/");
	if (audio_create_rootpath(dirname, path, DUMP_DIR_LEN - 1)) {
		AUDIO_LOGE("create log dir failed");
		goto end;
	}

	memset(path, 0, sizeof(path));
	snprintf(path, sizeof(path) - 1, "%s%s%s",
		dirname, OM_DA_COMBINE_DUMP_RAM_LOG_PATH,
		OM_DA_COMBINE_DUMP_OCRAM_NAME);

	ret = da_combine_save_log_file(path, DUMP_TYPE_WHOLE_OCRAM,
		priv_debug->dsp_config.ocram_start_addr,
		priv_debug->dsp_config.ocram_size);
	if (ret != 0) {
		AUDIO_LOGE("dump ocram failed");
		goto end;
	}

	memset(path, 0, sizeof(path));
	snprintf(path, sizeof(path) - 1, "%s%s%s",
		dirname, OM_DA_COMBINE_DUMP_RAM_LOG_PATH,
		OM_DA_COMBINE_DUMP_IRAM_NAME);
	ret = da_combine_save_log_file(path, DUMP_TYPE_WHOLE_IRAM,
		priv_debug->dsp_config.itcm_start_addr,
		priv_debug->dsp_config.itcm_size);
	if (ret != 0) {
		AUDIO_LOGE("dump itcm failed");
		goto end;
	}

	memset(path, 0, sizeof(path));
	snprintf(path, sizeof(path) - 1, "%s%s%s",
		dirname, OM_DA_COMBINE_DUMP_RAM_LOG_PATH,
		OM_DA_COMBINE_DUMP_DRAM_NAME);
	ret = da_combine_save_log_file(path, DUMP_TYPE_WHOLE_DRAM,
		priv_debug->dsp_config.dtcm_start_addr,
		priv_debug->dsp_config.dtcm_size);
	if (ret != 0) {
		AUDIO_LOGE("dump dtcm failed");
		goto end;
	}

end:
	kfree(dirname);
}

int set_total_log_name(const char *path_name, unsigned int size)
{
	char dir_name[DUMP_DIR_LEN] = {0};

	if (path_name == NULL) {
		AUDIO_LOGE("input path name is null");
		return -EINVAL;
	}

	memcpy(dir_name, CODECHIFI_TOTAL_LOG_PATH, /* unsafe_function_ignore: memcpy */
		sizeof(CODECHIFI_TOTAL_LOG_PATH));
	if (audio_get_dir_size(dir_name, sizeof(dir_name)) >= TOTAL_LOG_DIR_MAX_SIZE) {
		AUDIO_LOGI("save log more than 100M, remove it");
		if (audio_remove_dir(dir_name, sizeof(dir_name)) != 0) {
			AUDIO_LOGE("remove total log dir failed");
			return -ENOENT;
		}
		return 0;
	}

	if (audio_get_dir_size(path_name, size) >= TOTAL_LOG_FILE_MAX_SIZE)
		return rename_total_log_file(path_name, size);

	return 0;
}

static int send_debug_msg(unsigned long cmd,
	const unsigned int *param, unsigned int param_cnt)
{
	struct dsp_debug_msg *msg = NULL;
	int ret;

	if (param == NULL) {
		AUDIO_LOGE("param is null");
		return -EINVAL;
	}

	if (param_cnt > MAX_PARAM_SIZE) {
		AUDIO_LOGE("param_size:%d is larger than MAX_PARAM_SIZE:%d",
			param_cnt, MAX_PARAM_SIZE);
		return -EINVAL;
	}

	msg = kzalloc(sizeof(*msg), GFP_ATOMIC);
	if (msg == NULL) {
		AUDIO_LOGE("msg alloc fail");
		return -ENOMEM;
	}

	msg->msg_id = ID_AP_DSP_DEBUG_MSG;
	msg->debug_type = (unsigned short)cmd;
	msg->param_size = param_cnt;

	memcpy(msg->param, param, param_cnt * sizeof(unsigned int)); /* unsafe_function_ignore: memcpy */

	/* only return ERROR,do not send debug msg */
	ret = da_combine_request_pll_resource(HIGH_FREQ_SCENE_DSP_DEBUG);
	if (ret != 0) {
		AUDIO_LOGE("dsp debug request pll fail ret:%d", ret);
		kfree(msg);
		return -EPERM;
	}

	ret = da_combine_sync_write(msg, sizeof(*msg));
	if (ret != 0)
		AUDIO_LOGE("sync write failed, ret:%d", ret);

	/* do not neet release pll */
	kfree(msg);

	return ret;
}

static int send_debug_msg_to_dsp(char *msg_data, unsigned long msg_id)
{
	unsigned int param_cnt = 0;
	unsigned int param[MAX_PARAM_SIZE] = {0};
	char *sub_msg = NULL;
	unsigned long param_tmp;

	/* msg_data may be null, just send msg_id to dsp */
	while (msg_data != NULL && param_cnt < MAX_PARAM_SIZE) {
		sub_msg = strsep(&msg_data, " ");
		if (kstrtoul(sub_msg, 0, &param_tmp)) {
			AUDIO_LOGE("input para is error");
			return -EINVAL;
		}

		param[param_cnt++] = GET_LOW32(param_tmp);
	}

	if (send_debug_msg(msg_id, param, param_cnt) != 0) {
		AUDIO_LOGE("debug message process error");
		return -EINVAL;
	}

	return 0;
}

static ssize_t check_debug_para(const char __user *buf, size_t size,
	loff_t *data)
{
	if (buf == NULL) {
		AUDIO_LOGE("buf is NULL");
		return -EINVAL;
	}

	if (size > MAX_STR_LEN) {
		AUDIO_LOGE("input size:%zd is larger than MAX_STR_LEN:%d",
			size, MAX_STR_LEN);
		return -EINVAL;
	}

	if (data == NULL) {
		AUDIO_LOGE("data is NULL");
		return -EINVAL;
	}

	return 0;
}

static ssize_t get_debug_store_cmd(const char __user *buf,
	size_t size, loff_t *data, unsigned long *cmd, char **content)
{
	ssize_t ret;
	char *spe_str = NULL;

	ret = simple_write_to_buffer(*content, (MAX_STR_LEN - 1),
		data, buf, size);
	if (ret != size) {
		AUDIO_LOGE("input param buf read error, return value:%zd", ret);
		return -EINVAL;
	}

	spe_str = strsep(content, " ");
	if (spe_str == NULL) {
		AUDIO_LOGE("spe str is null");
		return -EINVAL;
	}

	if (kstrtoul(spe_str, 0, cmd)) {
		AUDIO_LOGE("input para is error");
		return -EINVAL;
	}

	return 0;
}

static void open_the_totallog_saving(char *total_str,
	unsigned long cmd)
{
	priv_debug->is_save_dsp_total_log = true;
	__pm_stay_awake(priv_debug->log_save_lock);
	if (send_debug_msg_to_dsp(total_str, cmd) != 0)
		AUDIO_LOGE("send debug msg to hifi fail");
}

static void close_the_totallog_saving(char *total_str,
	unsigned long cmd)
{
	ssize_t ret;
	char path[DUMP_DIR_LEN] = {0};

	snprintf(path, sizeof(path) - 1, "%s%s",
		CODECHIFI_TOTAL_LOG_PATH, CODECHIFI_TOTAL_LOG_FILE);
	if (set_total_log_name(path, DUMP_DIR_LEN) == 0) {
		ret = da_combine_save_log_file(path, DUMP_TYPE_TOTAL_LOG,
			priv_debug->dsp_config.dump_log_addr,
			priv_debug->dsp_config.dump_log_size);
		if (ret != 0)
			AUDIO_LOGE("dump dsp total log failed");
	}

	priv_debug->is_save_dsp_total_log = false;
	__pm_relax(priv_debug->log_save_lock);
	if (send_debug_msg_to_dsp(total_str, cmd) != 0)
		AUDIO_LOGE("send debug msg to hifi fail");
}

static ssize_t debug_proc_show(struct file *file, char __user *buf,
	size_t size, loff_t *data)
{
	ssize_t ret;

	if (buf == NULL) {
		AUDIO_LOGE("input error: buf is NULL");
		return -EINVAL;
	}

	ret = simple_read_from_buffer(buf, size, data, DEBUG_INFO_STR,
		strlen(DEBUG_INFO_STR));
	if (ret != strlen(DEBUG_INFO_STR))
		AUDIO_LOGE("read from buffer err: %zd, size: %zd", ret, size);

	return ret;
}

static void cmd_process(unsigned long cmd, char *total_str)
{
	switch (cmd) {
	/* dump dsp panic stack; logs; regs */
	case DA_COMBINE_DSP_DUMP_LOG:
		da_combine_dsp_dump_no_path();
		break;
	case DA_COMBINE_DSP_CHECK_MEM_NORMALLY:
		check_memory_normally();
		break;
	case DA_COMBINE_DSP_CHECK_MEM_BY_MARCH_IC:
		check_memory_by_march_ic();
		break;
	case DA_COMBINE_DSP_DUMP_ALL_RAM:
		debug_dump_all_ram();
		break;
	/* dsp_debug test */
	case DA_COMBINE_DSP_EXCEPTION_POINTER_INJECT:
	case DA_COMBINE_DSP_SOFT_IRQ_INJECT:
	case DA_COMBINE_DSP_CALL_EXIT_INJECT:
	case DA_COMBINE_DSP_SMARTPA_ALG_BYPASS:
	case DA_COMBINE_DSP_DATA_STATISTIC:
	case DA_COMBINE_DSP_WAKEUP_OM_SWITCH:
	case DA_COMBINE_DSP_CHECK_OCRAM:
	case DA_COMBINE_DSP_CHECK_TCM:
	case DA_COMBINE_DSP_CHECK_ICACHE:
	case DA_COMBINE_DSP_CHECK_DCACHE:
		if (send_debug_msg_to_dsp(total_str, cmd) != 0)
			AUDIO_LOGE("send debug msg to dsp fail");
		break;
	case DA_COMBINE_DSP_OPEN_TOTALLOG_DUMP:
		open_the_totallog_saving(total_str, cmd);
		break;
	case DA_COMBINE_DSP_CLOSE_TOTALLOG_DUMP:
		close_the_totallog_saving(total_str, cmd);
		break;
	case DA_COMBINE_DEBUG_EXIT:
		da_combine_release_pll_resource(HIGH_FREQ_SCENE_DSP_DEBUG);
		break;
	default:
		AUDIO_LOGE("cmd error:%lu", cmd);
		break;
	}
}

static ssize_t debug_proc_store(struct file *file,
	const char __user *buf, size_t size, loff_t *data)
{
	ssize_t ret;
	unsigned long cmd;
	char total_str[MAX_STR_LEN] = {0};
	char *content = total_str;

	if (priv_debug == NULL) {
		AUDIO_LOGE("priv is null");
		return -EINVAL;
	}

	ret = check_debug_para(buf, size, data);
	if (ret != 0)
		return ret;

	ret = get_debug_store_cmd(buf, size, data, &cmd, &content);
	if (ret != 0)
		return ret;

	mutex_lock(&priv_debug->debug_store_mutex);
	cmd_process(cmd, content);
	mutex_unlock(&priv_debug->debug_store_mutex);

	return size;
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,10,0))
static const struct proc_ops debug_proc_ops = {
	.proc_read  = debug_proc_show,
	.proc_write = debug_proc_store,
};
#else
static const struct file_operations debug_proc_ops = {
	.owner = THIS_MODULE,
	.read = debug_proc_show,
	.write = debug_proc_store,
};
#endif

void da_combine_adjust_log_sequence(char *log_buf, const size_t log_size)
{
	char *tmp_log_buf = NULL;
	char *divider_pos = NULL;
	char *divider = "<---------->";
	unsigned int div_pos;

	tmp_log_buf = vzalloc(log_size);
	if (tmp_log_buf == NULL) {
		AUDIO_LOGE("alloc buf failed");
		return;
	}

	memcpy(tmp_log_buf, log_buf, log_size); /* unsafe_function_ignore: memcpy */

	divider_pos = strstr(tmp_log_buf, divider); /* unsafe_function_ignore: strstr */
	if (divider_pos != NULL) {
		div_pos = divider_pos - tmp_log_buf;
		memset(log_buf, 0, log_size); /* unsafe_function_ignore: memset */
		memcpy(log_buf, divider_pos, log_size - div_pos); /* unsafe_function_ignore: memcpy */
		memcpy(log_buf + (log_size - div_pos), tmp_log_buf, div_pos); /* unsafe_function_ignore: memcpy */
	} else {
		AUDIO_LOGW("can not find divider, do not adjust sequence");
	}

	vfree(tmp_log_buf);
}


void da_combine_set_supend_time(void)
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,4,0))
{
	struct timespec64 tv;

	if (priv_debug == NULL) {
		AUDIO_LOGE("priv is null");
		return;
	}

	ktime_get_real_ts64(&tv);
	priv_debug->suspend_time = timespec_to_ms(tv);
}
#else
{
	struct timeval tv;

	if (priv_debug == NULL) {
		AUDIO_LOGE("priv is null");
		return;
	}

	do_gettimeofday(&tv);
	priv_debug->suspend_time = timeval_to_ms(tv);
}
#endif

int da_combine_wakeup_suspend_handle(const void *data)
{
	UNUSED_PARAMETER(data);

	if (priv_debug == NULL) {
		AUDIO_LOGE("priv is null");
		return -EINVAL;
	}

	if (priv_debug->wakeup_hook_status != WAKEUP_HOOK_PCM_START)
		return -EINVAL;

	/* before suspend handle, vad come */
	if (priv_debug->mad_time > priv_debug->suspend_time ||
		priv_debug->mad_time != priv_debug->vad_time)
		return -EINVAL;

	wakeup_hook_thread_stop();

	return 0;
}

void da_combine_wakeup_pcm_hook_start(void)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,4,0))
	struct timespec64 tv;
#else
	struct timeval tv;
#endif

	AUDIO_LOGI("start hook wakeup data");

	if (priv_debug == NULL) {
		AUDIO_LOGE("priv is null");
		return;
	}

	if (priv_debug->wakeup_hook_status != WAKEUP_HOOK_PCM_START)
		return;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,4,0))
	ktime_get_real_ts64(&tv);
	priv_debug->mad_time = timespec_to_ms(tv);
#else
	do_gettimeofday(&tv);
	priv_debug->mad_time = timeval_to_ms(tv);
#endif
	priv_debug->vad_time = priv_debug->mad_time;
	priv_debug->cur_hook_time = 0;
	priv_debug->pre_hook_time = 0;
	priv_debug->is_wakeup_hooking = false;
	priv_debug->kwakeup_hook_task = kthread_create(
		wakeup_hook_pcm_thread, 0, "da_combinewakeupdump");
	if (IS_ERR(priv_debug->kwakeup_hook_task))
		AUDIO_LOGE("create wakeup hook thread fail");
	else
		wake_up_process(priv_debug->kwakeup_hook_task);
}

void da_combine_wakeup_pcm_hook_stop(void)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,4,0))
	struct timespec64 tv;
#else
	struct timeval tv;
#endif

	if (priv_debug == NULL) {
		AUDIO_LOGE("priv is null");
		return;
	}

	if (priv_debug->wakeup_hook_status != WAKEUP_HOOK_PCM_START)
		return;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,4,0))
	ktime_get_real_ts64(&tv);
	priv_debug->vad_time = timespec_to_ms(tv);
#else
	do_gettimeofday(&tv);
	priv_debug->vad_time = timeval_to_ms(tv);
#endif
	wakeup_hook_thread_stop();
}

int da_combine_set_wakeup_hook(const struct da_combine_param_io_buf *param)
{
	int ret;

	if (priv_debug == NULL) {
		AUDIO_LOGE("priv is null");
		return -EINVAL;
	}

	ret = check_wakeup_hook_para(param);
	if (ret != 0)
		return ret;

	priv_debug->wakeup_hook_status =
		((struct wakeup_hook_msg *)param->buf_in)->status;

	if (audio_create_dir(WAKEUP_DUMP_DIR, sizeof(WAKEUP_DUMP_DIR))) {
		AUDIO_LOGE("create wakeup hook dir failed");
		return -EINVAL;
	}

	if (priv_debug->wakeup_hook_status == WAKEUP_HOOK_PCM_START) {
		__pm_stay_awake(priv_debug->wakeup_hook_lock);
		da_combine_mad_request_irq();
	} else {
		da_combine_mad_free_irq();
		da_combine_hook_pcm_handle();
		__pm_relax(priv_debug->wakeup_hook_lock);
	}

	return 0;
}

ssize_t da_combine_dsp_misc_read(struct file *file, char __user *buf,
	size_t nbytes, loff_t *pos)
{
	struct reg_rw_struct kern_buf;

	if (buf == NULL) {
		AUDIO_LOGE("input error: buf is NULL");
		return -EFAULT;
	}

	if (nbytes != sizeof(kern_buf)) {
		AUDIO_LOGE("nbytes:%zu from user space exceed", nbytes);
		AUDIO_LOGE("sizeof(kern_buf):%zu", sizeof(kern_buf));
		return -EFAULT;
	}

	if (copy_from_user(&kern_buf, buf, nbytes)) {
		AUDIO_LOGE("copy_from_user fail");
		return -EFAULT;
	}

	kern_buf.val = da_combine_dsp_read_reg(kern_buf.reg);
	if (copy_to_user(buf, &kern_buf, nbytes)) {
		AUDIO_LOGE("copy_to_user fail");
		return -EFAULT;
	}

	return (ssize_t)sizeof(kern_buf);
}

ssize_t da_combine_dsp_misc_write(struct file *file,
	const char __user *buf, size_t nbytes, loff_t *pos)
{
	struct reg_rw_struct kern_buf;

	if (buf == NULL) {
		AUDIO_LOGE("input error: buf is NULL");
		return -EFAULT;
	}

	if (nbytes != sizeof(kern_buf)) {
		AUDIO_LOGE("nbytes:%zu from user space exceed", nbytes);
		AUDIO_LOGE("sizeof(kern_buf):%zu", sizeof(kern_buf));
		return -EFAULT;
	}

	if (copy_from_user(&kern_buf, buf, nbytes)) {
		AUDIO_LOGE("copy_from_user fail");
		return -EFAULT;
	}

	da_combine_dsp_write_reg(kern_buf.reg, kern_buf.val);

	return sizeof(kern_buf);
}

int da_combine_dsp_get_dmesg(const void __user *arg)
{
	char *buf = NULL;
	unsigned int dump_addr;
	unsigned int log_size;
	unsigned int copy_fail_len;
	void *dump_info_user_buf = NULL;
	struct da_combine_dump_param_io_buf dump_info;

	if (priv_debug == NULL)
		return -EINVAL;

	dump_addr = priv_debug->dsp_config.dump_log_addr;
	log_size = priv_debug->dsp_config.dump_log_size;
	if (dump_addr == 0 || log_size == 0) {
		AUDIO_LOGE("dump addr or log size error, log size:%u", log_size);
		return -EINVAL;
	}

	if (copy_from_user(&dump_info, arg, sizeof(dump_info))) {
		AUDIO_LOGE("copy_from_user fail, don't dump log");
		return -EFAULT;
	}

	if (dump_info.buf_size != log_size) {
		AUDIO_LOGE("dump buffer size error");
		return -EINVAL;
	}

	dump_info_user_buf = INT_TO_ADDR(dump_info.user_buf_l, dump_info.user_buf_h);
	if (dump_info_user_buf == NULL) {
		AUDIO_LOGE("input dump buff addr is null");
		return -EFAULT;
	}

	buf = vzalloc((unsigned long)log_size);
	if (buf == NULL) {
		AUDIO_LOGE("alloc buf failed");
		return -ENOMEM;
	}

	da_combine_misc_dump_bin(dump_addr, buf, (size_t)log_size);

	copy_fail_len = (unsigned int)copy_to_user(dump_info_user_buf, buf, (unsigned long)log_size);
	if (copy_fail_len != 0)
		AUDIO_LOGE("copy_to_user fail: %u", copy_fail_len);

	vfree(buf);

	if (dump_info.clear) {
		AUDIO_LOGI("log clear");
		if (da_combine_request_low_pll_resource(LOW_FREQ_SCENE_DUMP)) {
			AUDIO_LOGE("clear log request pll failed");
			return -EINVAL;
		}
		da_combine_memset(dump_addr, (size_t)log_size);
		da_combine_release_low_pll_resource(LOW_FREQ_SCENE_DUMP);
	}

	return (int)(log_size - copy_fail_len);
}

unsigned int da_combine_read_mlib(unsigned char *arg,
	unsigned int len)
{
	unsigned int addr;
	unsigned int msg_size;
	unsigned int count;
	unsigned int value;

	IN_FUNCTION;

	if (priv_debug == NULL) {
		AUDIO_LOGE("debug priv is null");
		return 0;
	}

	addr = priv_debug->dsp_config.mlib_to_ap_msg_addr;
	msg_size = priv_debug->dsp_config.mlib_to_ap_msg_size;

	if (!addr || (!msg_size)) {
		AUDIO_LOGE("cannot find msg addr or size");
		return 0;
	}

	if (da_combine_dsp_read_reg(addr) != UCOM_PROTECT_WORD) {
		AUDIO_LOGE("mlib test cannot find parameters");
		return 0;
	}

	for (count = 1; count < (msg_size / sizeof(unsigned int)); count++) {
		value = da_combine_dsp_read_reg(addr + count * sizeof(unsigned int));
		if (count * sizeof(unsigned int) >= len) {
			AUDIO_LOGE("input not enough space");
			return 0;
		}

		if (value == UCOM_PROTECT_WORD && count > 0)
			break;

		memcpy(arg + (count - 1) * sizeof(unsigned int), &value, /* unsafe_function_ignore: memcpy */
			sizeof(value));
		AUDIO_LOGI("mlib test para[0x%x]", value);
	}

	OUT_FUNCTION;

	return (count - 1) * sizeof(unsigned int);
}

int da_combine_save_dsp_log(const void *data)
{
	int ret;
	char path[DUMP_DIR_LEN] = {0};

	UNUSED_PARAMETER(data);

	if (priv_debug == NULL) {
		AUDIO_LOGE("debug priv is null");
		return -EINVAL;
	}

	if (priv_debug->is_save_dsp_total_log) {
		AUDIO_LOGI("save hifi total log");

		snprintf(path, sizeof(path) - 1, "%s%s",
			CODECHIFI_TOTAL_LOG_PATH, CODECHIFI_TOTAL_LOG_FILE);
		if (set_total_log_name(path, DUMP_DIR_LEN - 1) != 0) {
			AUDIO_LOGE("set total log file name fail");
			return -EINVAL;
		}

		ret = da_combine_save_log_file(path, DUMP_TYPE_TOTAL_LOG,
			priv_debug->dsp_config.dump_log_addr,
			priv_debug->dsp_config.dump_log_size);
		if (ret != 0) {
			AUDIO_LOGE("dump dsp total log failed");
			return -EINVAL;
		}
	}

	return 0;
}

int da_combine_check_memory(const struct da_combine_param_io_buf *param)
{
	int ret;
	int type;

	ret = da_combine_check_msg_para(param, sizeof(struct fault_inject));
	if (ret != 0)
		return ret;

	type = ((struct fault_inject *)param->buf_in)->fault_type;

	AUDIO_LOGI("check type is %d", type);

	if (type == CHECK_MEMORY_NORMALLY)
		check_memory_normally();
	else
		check_memory_by_march_ic();

	return 0;
}

int da_combine_dsp_debug_init(const struct da_combine_dsp_config *config)
{
	struct proc_dir_entry *ent_debug = NULL;

	priv_debug = kzalloc(sizeof(*priv_debug), GFP_KERNEL);
	if (priv_debug == NULL) {
		AUDIO_LOGE("dsp debug init: failed to kzalloc priv debug");
		return -ENOMEM;
	}

	memcpy(&priv_debug->dsp_config, config, sizeof(*config));

	mutex_init(&priv_debug->debug_store_mutex);
	mutex_init(&priv_debug->wakeup_hook_stop_mutex);

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,4,0))
	priv_debug->log_save_lock = wakeup_source_register(NULL, "da_combine_log_save_lock");
	if (priv_debug->log_save_lock == NULL) {
		AUDIO_LOGE("request wakeup source failed");
		goto wakeup_source_err;
	}
	priv_debug->wakeup_hook_lock = wakeup_source_register(NULL, "da_combine_wakeup_hook_lock");
	if (priv_debug->wakeup_hook_lock == NULL) {
		AUDIO_LOGE("request wakeup source failed");
		goto wakeup_source_err;
	}
#else
	priv_debug->log_save_lock = wakeup_source_register("da_combine_log_save_lock");
	if (priv_debug->log_save_lock == NULL) {
		AUDIO_LOGE("request wakeup source failed");
		goto wakeup_source_err;
	}
	priv_debug->wakeup_hook_lock = wakeup_source_register("da_combine_wakeup_hook_lock");
	if (priv_debug->wakeup_hook_lock == NULL) {
		AUDIO_LOGE("request wakeup source failed");
		goto wakeup_source_err;
	}
#endif

	da_combine_debug_dir = proc_mkdir(DA_COMBINEDEBUG_PATH, NULL);
	if (da_combine_debug_dir == NULL) {
		AUDIO_LOGE("Unable to create debug dir");
		goto exit;
	}

	/* creating read/write "status" entry */
	ent_debug = proc_create(DA_COMBINEDEBUG_PROC_FILE, FILE_MODE_RW,
		da_combine_debug_dir, &debug_proc_ops);
	if (ent_debug == NULL) {
		remove_proc_entry(DA_COMBINEDEBUG_PATH, da_combine_debug_dir);
		AUDIO_LOGE("create proc file fail");
		goto exit;
	}

	priv_debug->is_save_dsp_total_log = false;
	priv_debug->wakeup_hook_status = WAKEUP_HOOK_PCM_STOP;
	priv_debug->is_wakeup_hooking = false;

	return 0;

exit:
	wakeup_hook_thread_stop();
wakeup_source_err:
	wakeup_source_unregister(priv_debug->wakeup_hook_lock);
	wakeup_source_unregister(priv_debug->log_save_lock);
	mutex_destroy(&priv_debug->wakeup_hook_stop_mutex);
	mutex_destroy(&priv_debug->debug_store_mutex);

	kfree(priv_debug);
	priv_debug = NULL;

	return -ENOMEM;
}

void da_combine_dsp_debug_deinit(void)
{
	if (priv_debug == NULL)
		return;

	remove_proc_entry(DA_COMBINEDEBUG_PROC_FILE, da_combine_debug_dir);
	remove_proc_entry(DA_COMBINEDEBUG_PATH, da_combine_debug_dir);

	wakeup_hook_thread_stop();

	wakeup_source_unregister(priv_debug->wakeup_hook_lock);
	wakeup_source_unregister(priv_debug->log_save_lock);
	mutex_destroy(&priv_debug->wakeup_hook_stop_mutex);
	mutex_destroy(&priv_debug->debug_store_mutex);

	kfree(priv_debug);
	priv_debug = NULL;
}

