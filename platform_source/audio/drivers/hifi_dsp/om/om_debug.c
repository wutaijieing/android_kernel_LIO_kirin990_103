/*
 * om_debug.c
 *
 * debug for socdsp
 *
 * Copyright (c) 2013-2020 Huawei Technologies Co., Ltd.
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

#include <linux/proc_fs.h>
#include <linux/kthread.h>
#include <linux/semaphore.h>
#include <linux/syscalls.h>
#include <linux/slab.h>
#include <linux/time.h>
#include <linux/rtc.h>
#include <platform_include/basicplatform/linux/rdr_pub.h>

#include "audio_log.h"
#include "audio_hifi.h"
#include "dsp_misc.h"
#include "drv_mailbox_msg.h"
#include "platform_include/basicplatform/linux/ipc_rproc.h"
#include "dsp_om.h"
#include "platform_base_addr_info.h"

/*lint -e773 */
#define HI_DECLARE_SEMAPHORE(name) \
	struct semaphore name = __SEMAPHORE_INITIALIZER(name, 0)
/*lint +e773 */
HI_DECLARE_SEMAPHORE(socdsp_log_sema);

#define SOCDSP_DUMP_FILE_NAME_MAX_LEN   256
#define MAX_LEVEL_STR_LEN               32
#define UNCONFIRM_ADDR                  0
#define ROOT_UID                        0
#define SYSTEM_GID                      1000
#define SOCDSP_OM_DIR_LIMIT             0750
#define SOCDSP_OM_FILE_LIMIT            0640
#define SOCDSP_SEC_MAX_NUM              100

#define HIFI_LOG_PATH_LEN 128

#define FILE_NAME_DUMP_DSP_LOG          "hifi.log"
#define FILE_NAME_DUMP_DSP_BIN          "hifi.bin"
#define FILE_NAME_DUMP_DSP_PANIC_LOG    "hifi_panic.log"
#define FILE_NAME_DUMP_DSP_PANIC_BIN    "hifi_panic.bin"

#define SOCDSP_DEBUG_PATH                  "hifidebug"
#define SOCDSP_DEBUG_LEVEL_PROC_FILE       "debuglevel"
#define SOCDSP_DEBUG_DSPDUMPLOG_PROC_FILE  "dspdumplog"
#define SOCDSP_DEBUG_FAULTINJECT_PROC_FILE "dspfaultinject"
#define SOCDSP_DEBUG_MISCPROC_PROC_FILE    "miscproc"
#define FAULT_INJECT_CMD_STR_MAX_LEN       200
#define MISC_PROC_OPTION_LEN               256
#define SOCDSP_TIME_STAMP_1S               32768
#define SOCDSP_DUMPLOG_TIMESPAN            (10 * SOCDSP_TIME_STAMP_1S)

#ifdef DYNAMIC_LOAD_HIFIIMG
#define CMD_TO_LPM3_POWEROFF_SOCDSP  ((0 << 24) | (16 << 16) | (3 << 8) | (1 << 0))
#define SOCDSP_IMAGE_PATH            "/data/hifi.img"
#define SOCDSP_IMAGE_MAX_SIZE        0xB00000

#ifndef CONFIG_AUDIO_COMMON_IMAGE
#ifdef SOC_ACPU_SECRAM_BASE_ADDR
#define SOCDSP_OCRAM_BASE_ADDR       (SOC_ACPU_SECRAM_BASE_ADDR) /* this macro just define on 990 */
#else
#define SOCDSP_OCRAM_BASE_ADDR       0xE8000000
#endif
#else
#define SOCDSP_OCRAM_BASE_ADDR       0
#endif

#define SOCDSP_OCRAM_PHY_BEGIN_ADDR  (SOCDSP_OCRAM_BASE_ADDR)
#define SOCDSP_OCRAM_PHY_END_ADDR    (SOCDSP_OCRAM_PHY_BEGIN_ADDR + SOCDSP_OCRAM_SIZE - 1)
#define SOCDSP_OCRAM_SIZE            0x40000

#define SOCDSP_TCM_BASE_ADDR         0xE8058000
#define SOCDSP_RUN_DDR_REMAP_BASE    0xC0000000

#define SOCDSP_TCM_PHY_BEGIN_ADDR    (SOCDSP_TCM_BASE_ADDR)
#define SOCDSP_TCM_PHY_END_ADDR      (SOCDSP_TCM_PHY_BEGIN_ADDR + SOCDSP_TCM_SIZE - 1)
#define SOCDSP_TCM_SIZE              (DSP_RUN_ITCM_SIZE + DSP_RUN_DTCM_SIZE)

#endif

extern void *memcpy64(void *dst, const void *src, unsigned int len);
extern void *memcpy128(void *dst, const void *src, unsigned int len);

enum drv_socdsp_image_sec_load {
	DRV_SOCDSP_IMAGE_SEC_LOAD_STATIC,
	DRV_SOCDSP_IMAGE_SEC_LOAD_DYNAMIC,
	DRV_SOCDSP_IMAGE_SEC_UNLOAD,
	DRV_SOCDSP_IMAGE_SEC_UNINIT,
	DRV_SOCDSP_IMAGE_SEC_LOAD_BUTT
};

enum drv_socdsp_image_sec_type {
	DRV_SOCDSP_IMAGE_SEC_TYPE_CODE,
	DRV_SOCDSP_IMAGE_SEC_TYPE_DATA,
	DRV_SOCDSP_IMAGE_SEC_TYPE_BSS,
	DRV_SOCDSP_IMAGE_SEC_TYPE_BUTT
};

enum dump_dsp_type {
	DUMP_DSP_LOG,
	DUMP_DSP_BIN
};

enum dsp_error_type {
	DSP_NORMAL,
	DSP_PANIC,
	DSP_LOG_BUF_FULL
};

struct socdsp_dump_info {
	enum dsp_error_type error_type;
	enum dump_dsp_type dump_type;
	char *file_name;
	char *data_addr;
	unsigned int data_len;
};

enum audio_hook_msg_type {
	AUDIO_HOOK_MSG_START,
	AUDIO_HOOK_MSG_STOP,
	AUDIO_HOOK_MSG_DATA_TRANS,
	AUDIO_HOOK_MSG_BUTT
};

struct image_partition_table {
	unsigned long phy_addr_start;
	unsigned long phy_addr_end;
	unsigned int size;
	unsigned long remap_addr;
};

struct socdsp_data_hook_ctrl_info {
	char file_path[SOCDSP_DUMP_FILE_NAME_MAX_LEN];
	unsigned int hook_data_enable;
	unsigned char *priv_data_base;
};

struct debug_level_com {
	char level_char;
	unsigned int level_num;
};

struct socdsp_om_send_hook_to_ap {
	unsigned int msg_type;
	unsigned int data_pos;
	unsigned int data_addr;
	unsigned int data_length;
};

struct om_priv_debug {
	unsigned int pre_dsp_dump_timestamp;
	unsigned int pre_exception_no;
	unsigned int *dsp_exception_no;
	unsigned int *dsp_panic_mark;
	unsigned int *dsp_log_cur_addr;
	char *dsp_log_addr;
	char *dsp_bin_addr;
	char cur_dump_time[SOCDSP_DUMP_FILE_NAME_MAX_LEN];
	bool first_dump_log;
	bool force_dump_log;
	enum dsp_error_type dsp_error_type;

	bool reset_system;
	unsigned int dsp_loaded_sign;
	unsigned int *dsp_debug_kill_addr;
	unsigned int *dsp_stack_addr;

	struct drv_fama_config *dsp_fama_config;
	struct task_struct *kdumpdsp_task;
	struct semaphore dsp_dump_sema;
	struct socdsp_om_work_info hook_wq;
	bool is_inited;
};

static struct om_priv_debug g_priv_debug;

static struct socdsp_dump_info g_dsp_dump_info[DUMP_INDEX_MAX] = {
	{ DSP_NORMAL, DUMP_DSP_LOG, FILE_NAME_DUMP_DSP_LOG, UNCONFIRM_ADDR,
		(DRV_DSP_UART_TO_MEM_SIZE - DRV_DSP_UART_TO_MEM_RESERVE_SIZE) },
	{ DSP_NORMAL, DUMP_DSP_BIN, FILE_NAME_DUMP_DSP_BIN, UNCONFIRM_ADDR,
		DSP_DUMP_BIN_SIZE },
	{ DSP_PANIC, DUMP_DSP_LOG, FILE_NAME_DUMP_DSP_PANIC_LOG, UNCONFIRM_ADDR,
		(DRV_DSP_UART_TO_MEM_SIZE - DRV_DSP_UART_TO_MEM_RESERVE_SIZE) },
	{ DSP_PANIC, DUMP_DSP_BIN, FILE_NAME_DUMP_DSP_PANIC_BIN, UNCONFIRM_ADDR,
		DSP_DUMP_BIN_SIZE },
};

#ifdef ENABLE_HIFI_DEBUG
static struct socdsp_data_hook_ctrl_info g_dsp_data_hook;
static struct proc_dir_entry *g_socdsp_debug_dir;
static const char *g_useage = "Useage:\necho \"test_case param1 param2 ...\" > dspfaultinject\n"
	"test_case maybe:\n"
	"read_mem addr\n"
	"write_mem addr value\n"
	"endless_loop\n"
	"overload [type(1:RT 2:Nomal 3:Low else:default)]\n"
	"auto_reset\n"
	"exit\n"
	"dyn_mem_leak [size count type(1:DDR 2:TCM)]\n"
	"dyn_mem_overlap [type(1:DDR 2:TCM)]\n"
	"stack_overflow [type(1:RT 2:Nomal 3:Low else:default)]\n"
	"isr_ipc\n"
	"performance_leak [count]\n"
	"mailbox_overlap\n"
	"msg_leak\n"
	"msg_deliver_err\n"
	"isr_malloc\n"
	"power_off\n"
	"power_on\n"
	"watchdog_timeout\n"
	"ulpp_ddr\n"
	"mailbox_full\n"
	"repeat_start [count]\n"
	"stop_dma\n"
	"performance_check [count]\n"
	"param_test\n"
	"timer_start [timer_count]\n"
	"test_timer [str(dump_num dump_node dump_blk dump_blk normal_oneshot normal_loop normal_clear high_oneshot high_loop high_clear callback_release)]\n"
	"test_volte_encrypt_result\n"
	"usbvoice_stop_audio_micin\n"
	"usbvoice_stop_audio_spkout\n"
	"usbvoice_start_audio_micin\n"
	"usbvoice_start_audio_spkout\n"
	"data_statistic\n"
	"offload_effect_switch [str(on off)]\n"
	"get_pd_config\n"
	"set_volume left_volume right_volume\n"
	"insert_falling\n"
	"set_prefetch\n"
	"fpga_timer_test\n"
	"fpga_ipc_test\n"
	"fpga_wdt_test\n"
	"fpga_dmmu_test\n"
	"fpga_secure_hifi_test\n"
	"fpga_secure_hifi_sec_clk_test\n"
	"fpga_secure_hifi_part_test\n"
	"fpga_secure_hifi_auto_test\n"
	"fpga_secure_hifi_ddr_test\n"
	"fpga_secure_dma_test\n"
	"fpga_secure_hdmi_test\n"
	"fpga_cfg_test\n"
	"fpga_cache_test\n"
	"fpga_l3_cache_test\n"
	"fpga_syscache_test\n"
	"fpga_sio_test\n"
	"fpga_tiny_test\n"
	"fpga_isr_test\n"
	"fpga_addrmonitor_test\n"
	"fpga_stress_test\n"
	"fpga_hifi3z_test\n"
	"fpga_fll_test\n"
	"dfs_test\n"
	"dfc_test\n"
	"hifi_freq_test\n"
	"smartpa_bypass_disable\n"
	"smartpa_bypass_enable\n"
	"hifi_log_to_hids_switch\n"
	"voice_memo_mute_cmd\n"
	"voice_memo_audio2voicetx_cmd\n"
	"audiomix2voicetx\n"
	"check_mem\n"
	"wakeup_om_switch\n"
	"check_icache_mem\n"
	"check_dcache_mem\n"
	"voice_set_mode_vol\n"
	"voice_set_mode_x\n"
	"wakeup_lp\n"
	"hook_wakeup_lp_data\n"
	"hook_data_to_ap\n"
	"stack_info\n"
	"voice_gu_rx_suspend\n"
	"dump_effects_cost\n"
	"set_pa_alg_bypass\n"
	"alp_proxy_log_switch\n"
	"set_srceen_status\n"
	"dyn_mem_check_consume\n"
	"dyn_mem_unfree_dump\n"
	"dsp_freq_test\n"
	"audio_mtp\n"
	"\n";
#endif

#ifdef DYNAMIC_LOAD_HIFIIMG
static struct image_partition_table g_addr_remap_tables[] = {
	{ SOCDSP_RUN_DDR_REMAP_BASE, SOCDSP_RUN_DDR_REMAP_BASE + DSP_RUN_SIZE,
		DSP_RUN_SIZE, DSP_RUN_LOCATION },
	{ SOCDSP_TCM_PHY_BEGIN_ADDR, SOCDSP_TCM_PHY_END_ADDR, SOCDSP_TCM_SIZE,
		DSP_IMAGE_TCMBAK_LOCATION },
	{ SOCDSP_OCRAM_PHY_BEGIN_ADDR, SOCDSP_OCRAM_PHY_END_ADDR, SOCDSP_OCRAM_SIZE,
		DSP_IMAGE_OCRAMBAK_LOCATION }
};

#ifdef CONFIG_AUDIO_COMMON_IMAGE
static void bind_image_partition_addr(void)
{
	g_addr_remap_tables[0].remap_addr += get_platform_mem_base_addr(PLT_HIFI_MEM);
	g_addr_remap_tables[1].remap_addr += get_platform_mem_base_addr(PLT_HIFI_MEM);
	g_addr_remap_tables[2].remap_addr += get_platform_mem_base_addr(PLT_HIFI_MEM);
	g_addr_remap_tables[2].phy_addr_start += get_platform_mem_base_addr(PLT_SECRAM_MEM);
	g_addr_remap_tables[2].phy_addr_end += get_platform_mem_base_addr(PLT_SECRAM_MEM);
}
#endif
#endif

static void get_time_stamp(char *buf, unsigned int len)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,4,0))
	struct timespec64 tv;
#else
	struct timeval tv;
#endif

	struct rtc_time tm;

	memset(&tv, 0, sizeof(tv));
	memset(&tm, 0, sizeof(tm));

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,4,0))
	ktime_get_real_ts64(&tv);
#else
	do_gettimeofday(&tv);
#endif
	tv.tv_sec -= (long)sys_tz.tz_minuteswest * 60;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,10,0))
	rtc_time64_to_tm(tv.tv_sec, &tm);
#else
	rtc_time_to_tm(tv.tv_sec, &tm);
#endif

	snprintf(buf, len, "%04d%02d%02d%02d%02d%02d",
		tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
		tm.tm_hour, tm.tm_min, tm.tm_sec);
}

static int socdsp_chown(const char *path, uid_t user, gid_t group)
{
	int ret;
	mm_segment_t old_fs;

	if (!path)
		return -EPERM;

	old_fs = get_fs();
	set_fs(KERNEL_DS); /*lint !e501 */

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 19, 0))
	ret = (int)ksys_chown((const char __user *)path, user, group);
#else
	ret = (int)sys_chown((const char __user *)path, user, group);
#endif
	if (ret != 0)
		loge("chown: %s uid: %d gid: %d failed error %d\n",
			path, user, group, ret);

	set_fs(old_fs);

	return ret;
}

static int socdsp_create_dir(char *path)
{
	int fd = -1;
	mm_segment_t old_fs;

	if (!path) {
		loge("path %pK is invailed\n", path);
		return -EPERM;
	}

	old_fs = get_fs();
	set_fs(KERNEL_DS); /*lint !e501 */

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 19, 0))
	fd = ksys_access(path, 0);
#else
	fd = sys_access(path, 0);
#endif
	if (fd != 0) {
		logi("need create dir %s\n", path);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 19, 0))
		fd = ksys_mkdir(path, SOCDSP_OM_DIR_LIMIT);
#else
		fd = sys_mkdir(path, SOCDSP_OM_DIR_LIMIT);
#endif
		if (fd < 0) {
			set_fs(old_fs);
			loge("create dir %s fail, ret: %d\n", path, fd);
			return fd;
		}

		logi("create dir %s successed, fd: %d\n", path, fd);

		/* log dir limit root-system */
		if (socdsp_chown(path, ROOT_UID, SYSTEM_GID))
			loge("chown %s failed\n", path);
	}

	set_fs(old_fs);

	return 0;
}

static int socdsp_om_create_log_dir(const char *path, int size)
{
	char cur_path[SOCDSP_DUMP_FILE_NAME_MAX_LEN];
	int index = 0;

	UNUSED_PARAMETER(size);
	if (!path || (strlen(path) + 1) > SOCDSP_DUMP_FILE_NAME_MAX_LEN) {
		loge("path %pK is invailed\n", path);
		return -EPERM;
	}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 19, 0))
	if (ksys_access(path, 0) == 0)
#else
	if (sys_access(path, 0) == 0)
#endif
		return 0;

	memset(cur_path, 0, sizeof(cur_path));

	if (*path != '/')
		return -EPERM;

	cur_path[index] = *path;
	index++;
	path++;

	while (*path != '\0') {
		if (*path == '/') {
			if (socdsp_create_dir(cur_path)) {
				loge("create dir %s failed\n", cur_path);
				return -EPERM;
			}
		}
		cur_path[index] = *path;
		index++;
		path++;
	}

	return 0;
}

static void dump_write_file_head(enum dump_dsp_index index, struct file *fp)
{
	char tmp_buf[64];
	unsigned long tmp_len;
	struct rtc_time cur_tm;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,10,0))
	struct timespec64 now;
#else
	struct timespec now;
#endif
	unsigned int err_no = 0xFFFFFFFF;

	const char *is_panic = "i'm panic\n";
	const char *is_exception = "i'm exception\n";
	const char *not_panic = "i'm ok\n";

	/* write file head */
	if (g_dsp_dump_info[index].dump_type == DUMP_DSP_LOG) {
		/* write dump log time */
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,10,0))
		ktime_get_coarse_ts64(&now);
		rtc_time64_to_tm(now.tv_sec, &cur_tm);
#else
		now = current_kernel_time();
		rtc_time_to_tm(now.tv_sec, &cur_tm);
#endif

		memset(tmp_buf, 0, sizeof(tmp_buf));
		tmp_len = snprintf(tmp_buf, sizeof(tmp_buf), "%04d-%02d-%02d %02d:%02d:%02d.\n",
			cur_tm.tm_year + 1900, cur_tm.tm_mon + 1,
			cur_tm.tm_mday, cur_tm.tm_hour,
			cur_tm.tm_min, cur_tm.tm_sec);
		vfs_write(fp, tmp_buf, tmp_len, &fp->f_pos);

		/* write exception no */
		memset(tmp_buf, 0, sizeof(tmp_buf));
		err_no = (unsigned int)(*(g_priv_debug.dsp_exception_no));
		if (err_no != 0xFFFFFFFF)
			tmp_len = snprintf(tmp_buf, sizeof(tmp_buf), "the exception no: %u\n", err_no);
		else
			tmp_len = snprintf(tmp_buf, sizeof(tmp_buf), "%s", "socdsp is fine, just dump log\n");

		vfs_write(fp, tmp_buf, tmp_len, &fp->f_pos);

		/* write error type */
		if (*g_priv_debug.dsp_panic_mark == 0xdeadbeaf)
			vfs_write(fp, is_panic, strlen(is_panic), &fp->f_pos);
		else if (*g_priv_debug.dsp_panic_mark == 0xbeafdead)
			vfs_write(fp, is_exception, strlen(is_exception), &fp->f_pos);
		else
			vfs_write(fp, not_panic, strlen(not_panic), &fp->f_pos);
	}
}

static int dump_write_file_body(enum dump_dsp_index index, struct file *fp)
{
	int write_size;
	char *data_addr = NULL;
	unsigned int data_len = g_dsp_dump_info[index].data_len;

	g_dsp_dump_info[NORMAL_LOG].data_addr =
		g_priv_debug.dsp_log_addr + DRV_DSP_UART_TO_MEM_RESERVE_SIZE;
	g_dsp_dump_info[PANIC_LOG].data_addr =
		g_priv_debug.dsp_log_addr + DRV_DSP_UART_TO_MEM_RESERVE_SIZE;

	if (!g_dsp_dump_info[index].data_addr) {
		loge("dsp log ioremap fail\n");
		return -EPERM;
	}

	data_addr = g_dsp_dump_info[index].data_addr;

	write_size = vfs_write(fp, data_addr, data_len, &fp->f_pos);
	if (write_size < 0)
		loge("write file fail\n");

	logi("write file size: %d\n", write_size);

	return 0;
}

void socdsp_dump_dsp(enum dump_dsp_index index, const char *log_path)
{
	int ret;
	mm_segment_t fs;
	struct file *fp = NULL;
	unsigned int file_flag = O_RDWR;
	struct kstat file_stat;
	char path_name[SOCDSP_DUMP_FILE_NAME_MAX_LEN];

	IN_FUNCTION;

	if (down_interruptible(&g_priv_debug.dsp_dump_sema) < 0) {
		loge("acquire the semaphore error\n");
		return;
	}

	dsp_get_log_signal();
	fs = get_fs();
	set_fs(KERNEL_DS); /*lint !e501 */
	ret = socdsp_om_create_log_dir(log_path, SOCDSP_DUMP_FILE_NAME_MAX_LEN);
	if (ret != 0)
		goto END;

	memset(path_name, 0, SOCDSP_DUMP_FILE_NAME_MAX_LEN);
	snprintf(path_name, SOCDSP_DUMP_FILE_NAME_MAX_LEN, "%s%s",
		log_path, g_dsp_dump_info[index].file_name);
	ret = vfs_stat(path_name, &file_stat);
	if (ret < 0) {
		logi("there isn't a dsp log file:%s, and need to create\n", path_name);
		file_flag |= O_CREAT;
	}
	fp = filp_open(path_name, file_flag, SOCDSP_OM_FILE_LIMIT);
	if (IS_ERR(fp)) {
		loge("open file fail: %s\n", path_name);
		fp = NULL;
		goto END;
	}

	vfs_llseek(fp, 0, SEEK_SET); /* write from file start */
	dump_write_file_head(index, fp);
	ret = dump_write_file_body(index, fp);
	if (ret != 0)
		goto END;

	ret = socdsp_chown(path_name, ROOT_UID, SYSTEM_GID);
	if (ret != 0)
		loge("chown %s failed!\n", path_name);
END:
	if (fp)
		filp_close(fp, 0);
	set_fs(fs);
	dsp_release_log_signal();
	up(&g_priv_debug.dsp_dump_sema);
	OUT_FUNCTION;
}

static void dump_dsp_show_info(unsigned int *socdsp_info_addr)
{
	uintptr_t socdsp_stack_addr;
	unsigned int i;

	socdsp_stack_addr = *(uintptr_t *)(socdsp_info_addr + 4);

	logi("panic addr: 0x%pK, cur pc: 0x%pK, pre pc: 0x%pK, cause: 0x%x\n",
		(void *)(uintptr_t)(*socdsp_info_addr),
		(void *)(uintptr_t)(*(socdsp_info_addr + 1)),
		(void *)(uintptr_t)(*(socdsp_info_addr + 2)),
		*(unsigned int *)(uintptr_t)(socdsp_info_addr + 3));
	for (i = 0; i < (DRV_DSP_STACK_TO_MEM_SIZE / 2) / sizeof(int) / 4; i += 4)
		logi("0x%pK: 0x%pK 0x%pK 0x%pK 0x%pK\n",
			(void *)(socdsp_stack_addr + i * 4),
			(void *)(uintptr_t)(*(socdsp_info_addr + i)),
			(void *)(uintptr_t)(*(socdsp_info_addr + i + 1)),
			(void *)(uintptr_t)(*(socdsp_info_addr + i + 2)),
			(void *)(uintptr_t)(*(socdsp_info_addr + i + 3)));
}

static int socdsp_dump_thread(void *p)
{
	unsigned int exception_no;
	unsigned int time_now;
	unsigned int time_diff;
	unsigned int *socdsp_info_addr = NULL;
	char log_path[HIFI_LOG_PATH_LEN] = {0};

	snprintf(log_path, sizeof(log_path) - 1, "%s%s", PATH_ROOT, "running_trace/hifi_log/");

	IN_FUNCTION;

	while (!kthread_should_stop()) {
		if (down_interruptible(&socdsp_log_sema) != 0)
			loge("socdsp dump dsp thread wake up err\n");
#ifdef CONFIG_DFX_BB
		if (bbox_check_edition() != EDITION_INTERNAL_BETA) {
			loge("not beta, do not dump socdsp\n");
			continue;
		}
#endif
		time_now = om_get_dsp_timestamp();
		time_diff = time_now - g_priv_debug.pre_dsp_dump_timestamp;
		g_priv_debug.pre_dsp_dump_timestamp = time_now;
		socdsp_info_addr = g_priv_debug.dsp_stack_addr;
		exception_no = *(unsigned int *)(socdsp_info_addr + 3);

		logi("errno: %x pre errno: %x is first: %d is force: %d time diff: %d ms\n",
			exception_no, g_priv_debug.pre_exception_no, g_priv_debug.first_dump_log,
			g_priv_debug.force_dump_log, (time_diff * 1000) / SOCDSP_TIME_STAMP_1S);

		get_time_stamp(g_priv_debug.cur_dump_time, SOCDSP_DUMP_FILE_NAME_MAX_LEN);
		if ((exception_no < 40) && (exception_no != g_priv_debug.pre_exception_no)) {
			dump_dsp_show_info(socdsp_info_addr);
			socdsp_dump_dsp(PANIC_LOG, log_path);
			socdsp_dump_dsp(PANIC_BIN, log_path);
			g_priv_debug.pre_exception_no = exception_no;
		} else if (g_priv_debug.first_dump_log || g_priv_debug.force_dump_log ||
				(time_diff > SOCDSP_DUMPLOG_TIMESPAN)) {
			socdsp_dump_dsp(NORMAL_LOG, log_path);
			/* needn't dump bin when socdsp log buffer full */
			if (g_priv_debug.dsp_error_type != DSP_LOG_BUF_FULL)
				socdsp_dump_dsp(NORMAL_BIN, log_path);
			g_priv_debug.first_dump_log = false;
		}
	}
	OUT_FUNCTION;

	return 0;
}

void socdsp_dump_panic_log(void)
{
	if (!is_dsp_img_loaded())
		return;

	up(&socdsp_log_sema);
}

int socdsp_dump(uintptr_t arg)
{
	unsigned int err_type = 0;

	if (!arg) {
		loge("arg is null\n");
		return -EPERM;
	}

	if (try_copy_from_user(&err_type, (void __user *)arg, sizeof(err_type))) {
		loge("copy from user fail, don't dump log\n");
		return -EPERM;
	}
	g_priv_debug.dsp_error_type = err_type;
	g_priv_debug.force_dump_log = true;
	up(&socdsp_log_sema);

	return 0;
}

#ifdef ENABLE_HIFI_DEBUG
static void socdsp_om_data_hook_start(void)
{
	int ret;
	mm_segment_t fs;
	struct rtc_time cur_tm;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,10,0))
	struct timespec64 now;
#else
	struct timespec now;
#endif
	fs = get_fs();
	set_fs(KERNEL_DS); /*lint !e501 */
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,10,0))
	ktime_get_coarse_ts64(&now);
	rtc_time64_to_tm(now.tv_sec, &cur_tm);
#else
	now = current_kernel_time();
	rtc_time_to_tm(now.tv_sec, &cur_tm);
#endif

	memset(g_dsp_data_hook.file_path, 0, sizeof(g_dsp_data_hook.file_path));
	snprintf(g_dsp_data_hook.file_path, sizeof(g_dsp_data_hook.file_path),
		"%s%s%02d-%02d-%02d-%02d/", PATH_ROOT, "om_data/", cur_tm.tm_mday,
		cur_tm.tm_hour, cur_tm.tm_min, cur_tm.tm_sec);

	loge("ctreat path: %s\n", g_dsp_data_hook.file_path);

	ret = socdsp_om_create_log_dir(g_dsp_data_hook.file_path, SOCDSP_DUMP_FILE_NAME_MAX_LEN);
	if (ret != 0)
		loge("ctreat dir fail\n");
	set_fs(fs);
}

static void socdsp_om_data_hook_proc(struct socdsp_om_send_hook_to_ap *info)
{
	char path_name[SOCDSP_DUMP_FILE_NAME_MAX_LEN] = {0};
	mm_segment_t fs;
	struct kstat file_stat;
	struct file *fp = NULL;
	unsigned int file_flag = O_RDWR | O_APPEND;
	unsigned char *priv_data_addr = NULL;
	int ret;

	fs = get_fs();
	set_fs(KERNEL_DS); /*lint !e501 */

	snprintf(path_name, SOCDSP_DUMP_FILE_NAME_MAX_LEN, "%s%u.dat",
		g_dsp_data_hook.file_path, info->data_pos);

	ret = vfs_stat(path_name, &file_stat);
	if (ret < 0) {
		logi("there isn't a dsp data file: %s, and need to create\n", path_name);
		file_flag |= O_CREAT;
	}

	fp = filp_open(path_name, file_flag, SOCDSP_OM_FILE_LIMIT);
	if (IS_ERR(fp)) {
		loge("open file fail: %s\n", path_name);
		set_fs(fs);
		return;
	}

	priv_data_addr = g_dsp_data_hook.priv_data_base + (info->data_addr - DSP_UNSEC_BASE_ADDR);
	ret = vfs_write(fp, (const char *)priv_data_addr, info->data_length, &fp->f_pos);
	if (ret < 0)
		loge("write file fail\n");

	if (socdsp_chown(path_name, ROOT_UID, SYSTEM_GID))
		loge("chown %s failed\n", path_name);

	filp_close(fp, 0);

	set_fs(fs);
}

static void socdsp_om_data_hook(struct work_struct *work)
{
	struct socdsp_om_work *om_work = NULL;
	unsigned char data[MAIL_LEN_MAX];
	struct socdsp_om_send_hook_to_ap hook_data_info;

	while (!list_empty(&g_priv_debug.hook_wq.ctl.list)) {
		memset(data, 0, sizeof(unsigned char) * MAIL_LEN_MAX);
		spin_lock_bh(&g_priv_debug.hook_wq.ctl.lock);
		om_work = list_entry(g_priv_debug.hook_wq.ctl.list.next, struct socdsp_om_work, om_node);

		memcpy(data, om_work->data, om_work->data_len);

		list_del(&om_work->om_node);
		kfree(om_work);
		spin_unlock_bh(&g_priv_debug.hook_wq.ctl.lock);

		memcpy(&hook_data_info, data, sizeof(hook_data_info));

		switch (hook_data_info.msg_type) {
		case AUDIO_HOOK_MSG_START:
			g_dsp_data_hook.hook_data_enable = 1;
			socdsp_om_data_hook_start();
			break;
		case AUDIO_HOOK_MSG_DATA_TRANS:
			if (g_dsp_data_hook.hook_data_enable)
				socdsp_om_data_hook_proc(&hook_data_info);
			break;
		case AUDIO_HOOK_MSG_STOP:
			g_dsp_data_hook.hook_data_enable = 0;
			break;
		default:
			loge("type %u, not support\n", hook_data_info.msg_type);
		}
	}
}

int socdsp_get_dmesg(uintptr_t arg)
{
	int ret;
	unsigned int len;
	struct misc_io_dump_buf_param dump_info;
	void __user *dump_info_user_buf = NULL;

	len = (unsigned int)(*g_priv_debug.dsp_log_cur_addr);
	if (len > DRV_DSP_UART_TO_MEM_SIZE) {
		loge("len is: %u larger than: %d, don't dump log\n", len, DRV_DSP_UART_TO_MEM_SIZE);
		return 0;
	}

	if (try_copy_from_user(&dump_info, (void __user *)arg, sizeof(dump_info))) {
		loge("copy from user fail, don't dump log\n");
		return 0;
	}

	if (dump_info.buf_size == 0) {
		loge("input buf size is zero, don't dump log\n");
		return 0;
	}

	if (len > dump_info.buf_size) {
		logw("input buf size smaller, input buf size: %u, log size: %u, contiue to dump using smaller size\n",
			dump_info.buf_size, len);
		len = dump_info.buf_size;
	}
	dump_info_user_buf = INT_TO_ADDR(dump_info.user_buf_l, dump_info.user_buf_h);
	if (!dump_info_user_buf) {
		loge("input dump buff addr is null\n");
		return 0;
	}

	g_dsp_dump_info[NORMAL_LOG].data_addr = g_priv_debug.dsp_log_addr;
	logi("get msg: len: %d from: %pK to: %pK\n",
		len, g_dsp_dump_info[NORMAL_LOG].data_addr, dump_info_user_buf);

	ret = try_copy_to_user(dump_info_user_buf, g_dsp_dump_info[NORMAL_LOG].data_addr, len);
	if (ret != 0) {
		loge("copy to user fail: %d\n", ret);
		len -= ret;
	}
	if (dump_info.clear) {
		*g_priv_debug.dsp_log_cur_addr = DRV_DSP_UART_TO_MEM_RESERVE_SIZE;
		if (g_dsp_dump_info[NORMAL_LOG].data_len > DRV_DSP_UART_TO_MEM_SIZE) {
			loge("dsp dump info normal log data len is larger than drv dsp uart to mem size\n");
			len = 0;
		} else {
			memset(g_dsp_dump_info[NORMAL_LOG].data_addr, 0, g_dsp_dump_info[NORMAL_LOG].data_len);
		}
	}

	return (int)len;
}

static struct debug_level_com s_debug_level_com[PRINT_LEVEL_MAX] = {
	{'d', PRINT_LEVEL_DEBUG}, {'i', PRINT_LEVEL_INFO},
	{'w', PRINT_LEVEL_WARN},  {'e', PRINT_LEVEL_ERROR}
};

static unsigned int socdsp_get_debug_level_num(char level_char)
{
	unsigned int i;
	unsigned int len = ARRAY_SIZE(s_debug_level_com);

	for (i = 0; i < len; i++) {
		if (level_char == s_debug_level_com[i].level_char)
			return s_debug_level_com[i].level_num;
	}

	return PRINT_LEVEL_INFO;
}

static char socdsp_get_debug_level_char(char level_num)
{
	unsigned int i;
	unsigned int len = ARRAY_SIZE(s_debug_level_com);

	for (i = 0; i < len; i++) {
		if (level_num == s_debug_level_com[i].level_num)
			return s_debug_level_com[i].level_char;
	}

	return 'i'; /* info */
}

static ssize_t socdsp_debug_level_show(struct file *file, char __user *buf,
	size_t size, loff_t *data)
{
	char level_str[MAX_LEVEL_STR_LEN] = {0};
	unsigned int level = om_get_print_level();

	if (!buf) {
		loge("input param buf is invalid\n");
		return -EINVAL;
	}

	snprintf(level_str, MAX_LEVEL_STR_LEN, "debug level: %c.\n", socdsp_get_debug_level_char(level));

	return simple_read_from_buffer(buf, size, data, level_str, strlen(level_str));
}

static ssize_t socdsp_debug_level_store(struct file *file, const char __user *buf,
	size_t size, loff_t *data)
{
	ssize_t ret;
	char level_str[MAX_LEVEL_STR_LEN] = {0};
	loff_t pos = 0;
	unsigned int level;

	if (!buf) {
		loge("input param buf is invalid\n");
		return -EINVAL;
	}

	ret = simple_write_to_buffer(level_str, MAX_LEVEL_STR_LEN - 1, &pos, buf, size);
	if (ret != size) {
		loge("input param buf read error, return value: %zd\n", ret);
		return -EINVAL;
	}

	if (!strchr("diwe", level_str[0])) {
		loge("input param buf is error, valid: d i w e: %s\n", level_str);
		return -EINVAL;
	}

	if (level_str[1] != '\n') {
		loge("input param buf is error, last char is not \\n\n");
		return -EINVAL;
	}

	level = socdsp_get_debug_level_num(level_str[0]);

	om_set_print_level(level);

	return size;
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,10,0))
static const struct proc_ops socdsp_debug_proc_ops = {
	.proc_read  = socdsp_debug_level_show,
	.proc_write = socdsp_debug_level_store,
};
#else
static const struct file_operations socdsp_debug_proc_ops = {
	.owner = THIS_MODULE,
	.read  = socdsp_debug_level_show,
	.write = socdsp_debug_level_store,
};
#endif

static int socdsp_send_str_todsp(const char *cmd_str, size_t size)
{
	int ret;
	unsigned int msg_len;
	struct socdsp_str_cmd *pcmd = NULL;

	msg_len = sizeof(*pcmd) + size + 1; /* 1 for last \0 */

	pcmd = kmalloc(msg_len, GFP_ATOMIC);
	if (!pcmd) {
		loge("cmd malloc is null\n");
		return -ENOMEM;
	}

	memset(pcmd, 0, msg_len); /*lint !e419 */

	pcmd->msg_id = ID_AP_AUDIO_STR_CMD;
	pcmd->str_len = size;
	strncpy(pcmd->str, cmd_str, size); /*lint !e419 */

	ret = (int)mailbox_send_msg(MAILBOX_MAILCODE_ACPU_TO_HIFI_MISC, pcmd, msg_len);

	kfree(pcmd);

	return ret;
}

static ssize_t socdsp_fault_inject_show(struct file *file, char __user *buf,
	size_t size, loff_t *data)
{
	if (!buf) {
		loge("input param buf is invalid\n");
		return -EINVAL;
	}

	return simple_read_from_buffer(buf, size, data, g_useage, strlen(g_useage));
}

static ssize_t socdsp_fault_inject_store(struct file *file, const char __user *buf,
	size_t size, loff_t *data)
{
	int ret;
	ssize_t len;
	loff_t pos = 0;
	char cmd_str[FAULT_INJECT_CMD_STR_MAX_LEN];

	if (!buf) {
		loge("param buf is null\n");
		return -EINVAL;
	}

	if ((size > FAULT_INJECT_CMD_STR_MAX_LEN) || (size == 0)) {
		loge("input size %zd is invalid\n", size);
		return -EINVAL;
	}

	memset(cmd_str, 0, sizeof(cmd_str));
	len = simple_write_to_buffer(cmd_str, (FAULT_INJECT_CMD_STR_MAX_LEN - 1), &pos, buf, size);
	if (len != size) {
		loge("write to buffer fail: %zd\n", len);
		return -EINVAL;
	}

	ret = socdsp_send_str_todsp(cmd_str, size);
	if (ret != 0) {
		loge("msg: %s send to socdsp fail: %d\n", cmd_str, ret);
		return ret;
	}

	return size;
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,10,0))
static const struct proc_ops socdsp_dspfaultinject_proc_ops = {
	.proc_read  = socdsp_fault_inject_show,
	.proc_write = socdsp_fault_inject_store,
};
#else
static const struct file_operations socdsp_dspfaultinject_proc_ops = {
	.owner = THIS_MODULE,
	.read  = socdsp_fault_inject_show,
	.write = socdsp_fault_inject_store,
};
#endif

#ifdef DYNAMIC_LOAD_HIFIIMG
static void *memcpy_aligned(void *_dst, const void *_src, unsigned int len)
{
	unsigned char *dst = _dst;
	const unsigned char *src = _src;
	unsigned int length = len;
	unsigned int cpy_len;

	if (((uintptr_t)dst % 16 == 0) && ((uintptr_t)src % 16 == 0) && (length >= 16)) {
		cpy_len = length & 0xFFFFFFF0;
		memcpy128(dst, src, cpy_len);
		length = length % 16;
		dst = dst + cpy_len;
		src = src + cpy_len;

		if (length == 0)
			return _dst;
	}

	if (((uintptr_t)dst % 8 == 0) && ((uintptr_t)src % 8 == 0) && (length >= 8)) {
		cpy_len = length & 0xFFFFFFF8;
		memcpy64(dst, src, cpy_len);
		length = length % 8;
		dst = dst + cpy_len;
		src = src + cpy_len;
		if (length == 0)
			return _dst;
	}

	if (((uintptr_t)dst % 4 == 0) && ((uintptr_t)src % 4 == 0)) {
		cpy_len = length >> 2;
		while (cpy_len-- > 0) {
			*(unsigned int *)dst = *(unsigned int *)src;
			dst += 4;
			src += 4;
		}
		length = length % 4;
		if (length == 0)
			return _dst;
	}

	while (length-- > 0)
		*dst++ = *src++;

	return _dst;
}

static int notify_lpm3_power_off_hifi(void)
{
	int ret;
	unsigned int msg = CMD_TO_LPM3_POWEROFF_SOCDSP;

	ret = RPROC_ASYNC_SEND(IPC_ACPU_LPM3_MBX_5, &msg, 1);
	if (ret != 0)
		loge("send message to lpm3 fail\n");

	return ret;
}

static int get_img_length(int fd, unsigned int *img_size)
{
	struct drv_socdsp_image_head *img_head = NULL;
	unsigned int read_size;

	img_head = vzalloc(sizeof(*img_head) + 1);
	if (!img_head) {
		loge("alloc img buffer fail\n");
		return -EPERM;
	}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4,19,0))
	read_size = ksys_read((unsigned int)fd, (char *)img_head, sizeof(*img_head));
#else
	read_size = sys_read((unsigned int)fd, (char *)img_head, sizeof(*img_head));
#endif
	if (read_size != sizeof(*img_head)) {
		loge("read image head failed, read size: %u\n", read_size);
		vfree(img_head);
		return -EPERM;
	}

	*img_size = img_head->image_size;
	vfree(img_head);

	return 0;
}

static int read_socdsp_img_file(char *img_buf)
{
	int fd = -1;
	unsigned int img_size = 0;
	unsigned int read_size;
	int ret = 0;
	mm_segment_t old_fs;

	if (!img_buf) {
		loge("img buffer is null\n");
		return -EPERM;
	}

	old_fs = get_fs();
	set_fs(KERNEL_DS); /*lint !e501 */

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4,19,0))
	fd = ksys_open(SOCDSP_IMAGE_PATH, O_RDONLY, SOCDSP_OM_FILE_LIMIT);
#else
	fd = sys_open(SOCDSP_IMAGE_PATH, O_RDONLY, SOCDSP_OM_FILE_LIMIT);
#endif
	if (fd < 0) {
		loge("open %s failed, fd: %d\n", SOCDSP_IMAGE_PATH, fd);
		set_fs(old_fs);
		return -EPERM;
	}

	ret = get_img_length(fd, &img_size);
	if (ret != 0)
		goto end;
	if (img_size > SOCDSP_IMAGE_MAX_SIZE) {
		loge("image size error, size: %u\n", img_size);
		ret = -EPERM;
		goto end;
	}
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4,19,0))
	if (ksys_lseek((unsigned int)fd, 0, SEEK_SET)) {
#else
	if (sys_lseek((unsigned int)fd, 0, SEEK_SET)) {
#endif
		loge("lseek fail\n");
		ret = -EPERM;
		goto end;
	}
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4,19,0))
	read_size = ksys_read((unsigned int)fd, (char *)img_buf, img_size);
#else
	read_size = sys_read((unsigned int)fd, (char *)img_buf, img_size);
#endif
	if (read_size != img_size) {
		loge("read total image file failed, read size: %u, image size: %u\n",
			read_size, img_size);
		ret = -EPERM;
		goto end;
	}
end:
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4,19,0))
	ksys_close(fd);
#else
	sys_close(fd);
#endif
	set_fs(old_fs);

	return ret;
}

static int load_image_by_section_check(struct drv_socdsp_image_head *socdsp_img)
{
	if (!socdsp_img) {
		loge("input socdsp img is null\n");
		return -EPERM;
	}

	logi("sections num: %u, image size: %u\n", socdsp_img->sections_num, socdsp_img->image_size);

	if (socdsp_img->sections_num > SOCDSP_SEC_MAX_NUM) {
		loge("socdsp img sections num: %u\n", socdsp_img->sections_num);
		return -EPERM;
	}

	return 0;
}

static int load_image_by_section(struct drv_socdsp_image_head *socdsp_img)
{
	unsigned int i;

	if (load_image_by_section_check(socdsp_img) != 0)
		return -EPERM;

	for (i = 0; i < socdsp_img->sections_num; i++) {
		if (socdsp_img->sections[i].type != DRV_SOCDSP_IMAGE_SEC_TYPE_BSS &&
			socdsp_img->sections[i].load_attib != (unsigned char)DRV_SOCDSP_IMAGE_SEC_UNLOAD) {
			unsigned int *iomap_dest_addr = NULL;
			unsigned int index = 0;
			unsigned long remap_dest_addr;

			remap_dest_addr = (unsigned long)socdsp_img->sections[i].des_addr;
#ifndef CONFIG_AUDIO_COMMON_IMAGE
			if (remap_dest_addr >= SOCDSP_OCRAM_BASE_ADDR &&
				remap_dest_addr <= SOCDSP_OCRAM_PHY_END_ADDR) {
#else
			if (remap_dest_addr >= (SOCDSP_OCRAM_PHY_BEGIN_ADDR +
				get_platform_mem_base_addr(PLT_SECRAM_MEM)) &&
				remap_dest_addr <= (SOCDSP_OCRAM_PHY_END_ADDR +
				get_platform_mem_base_addr(PLT_SECRAM_MEM))) {
#endif
				logi("do not load ocram section\n");
				continue;
			} else if (remap_dest_addr >= SOCDSP_TCM_PHY_BEGIN_ADDR &&
				remap_dest_addr <= SOCDSP_TCM_PHY_END_ADDR) {
				index = 1;
			} else if (remap_dest_addr >= SOCDSP_RUN_DDR_REMAP_BASE &&
				remap_dest_addr <= (SOCDSP_RUN_DDR_REMAP_BASE + DSP_RUN_SIZE)) {
				index = 0;
			} else {
				loge("dest addr error, %pK\n", (void *)(uintptr_t)remap_dest_addr);
				return -EPERM;
			}

			remap_dest_addr -= g_addr_remap_tables[index].phy_addr_start - g_addr_remap_tables[index].remap_addr;
			iomap_dest_addr = (unsigned int *)ioremap(remap_dest_addr, socdsp_img->sections[i].size); /*lint !e446 */
			if (!iomap_dest_addr) {
				loge("ioremap failed\n");
				return -EPERM;
			}

			memcpy_aligned((void *)(iomap_dest_addr),
				(void *)((char *)socdsp_img + socdsp_img->sections[i].src_offset),
				socdsp_img->sections[i].size);
			iounmap(iomap_dest_addr);
		}
	}
	loge("load socdsp image done\n");

	return 0;
}

static int load_socdsp_img_by_misc(void)
{
	char *img_buf = NULL;
	char *running_region_addr = NULL;

	loge("load socdsp image now\n");

	if (notify_lpm3_power_off_hifi()) {
		loge("power off socdsp fail\n");
		return -EPERM;
	}

	img_buf = vzalloc(SOCDSP_IMAGE_MAX_SIZE);
	if (!img_buf) {
		loge("alloc img buffer fail\n");
		return -EPERM;
	}

	if (read_socdsp_img_file(img_buf)) {
		loge("read socdsp img fail\n");
		vfree(img_buf);
		return -EPERM;
	}

	/* clear running region */
	running_region_addr = (char *)ioremap_wc(get_phy_addr(DSP_RUN_LOCATION, PLT_HIFI_MEM),
		DSP_RUN_SIZE);
	if (running_region_addr) {
		memset(running_region_addr, 0, DSP_RUN_SIZE);
		iounmap(running_region_addr);
	}

	if (load_image_by_section((struct drv_socdsp_image_head *)img_buf)) {
		loge("load image fail\n");
		vfree(img_buf);
		return -EPERM;
	}

	vfree(img_buf);
	return 0;
}
#endif

static ssize_t socdsp_misc_proc_option_show(struct file *file, char __user *buf,
	size_t size, loff_t *data)
{
	char misc_proc_option[MISC_PROC_OPTION_LEN] = {0};
	char *info_str = "0:disable reset system\n"
		"1:enable reset system\n"
		"2:load socdsp image\n";

	if (!buf) {
		loge("input param buf is invalid\n");
		return -EINVAL;
	}

	snprintf(misc_proc_option, MISC_PROC_OPTION_LEN,
		"%s current reset status: 0-disable 1-enable: %d\n",
		info_str, g_priv_debug.reset_system);

	return simple_read_from_buffer(buf, size, data, misc_proc_option, strlen(misc_proc_option));
}

static ssize_t socdsp_misc_proc_option_store(struct file *file, const char __user *buf,
	size_t size, loff_t *data)
{
	ssize_t ret;
	char misc_proc_option[MISC_PROC_OPTION_LEN] = {0};
	loff_t pos = 0;

	if (!buf) {
		loge("input param buf is invalid\n");
		return -EINVAL;
	}

	ret = simple_write_to_buffer(misc_proc_option, MISC_PROC_OPTION_LEN - 1, &pos, buf, size);
	if (ret != size) {
		loge("input param buf read error, return value:%zd\n", ret);
		return -EINVAL;
	}

	if (!strchr("012", misc_proc_option[0])) {
		loge("input param buf is error, valid: 0 1 2: %s\n", misc_proc_option);
		return -EINVAL;
	}

	if (misc_proc_option[1] != '\n') {
		loge("input param buf is error, last char is not \\n\n");
		return -EINVAL;
	}

	switch (misc_proc_option[0]) {
	case '0':
		g_priv_debug.reset_system = false;
		break;
	case '1':
		g_priv_debug.reset_system = true;
		break;
	case '2':
#ifdef DYNAMIC_LOAD_HIFIIMG
		if (dsp_misc_get_platform_type() == DSP_PLATFORM_FPGA) {
			if (load_socdsp_img_by_misc()) {
				loge("load socdsp image by misc fail\n");
				return -EBUSY;
			}
		}
#endif
		break;
	default:
		loge("input option value error, misc proc option 0: %c\n",
			misc_proc_option[0]);
		break;
	}

	return size;
}

bool om_get_resetting_state(void)
{
	return g_priv_debug.reset_system;
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,10,0))
static const struct proc_ops socdsp_misc_proc_proc_ops = {
	.proc_read  = socdsp_misc_proc_option_show,
	.proc_write = socdsp_misc_proc_option_store,
};
#else
static const struct file_operations socdsp_misc_proc_proc_ops = {
	.owner = THIS_MODULE,
	.read  = socdsp_misc_proc_option_show,
	.write = socdsp_misc_proc_option_store,
};
#endif

static ssize_t socdsp_dump_log_show(struct file *file, char __user *buf,
		size_t size, loff_t *ppos)
{
	ssize_t ret;
	char log_path[HIFI_LOG_PATH_LEN] = {0};

	if (!buf) {
		loge("input param buf is invalid\n");

		return -EINVAL;
	}

	snprintf(log_path, sizeof(log_path) - 1, "%s%s", PATH_ROOT, "running_trace/hifi_log/");
	ret = simple_read_from_buffer(buf, size, ppos,
			log_path, (strlen(log_path) + 1));
	if (ret == (ssize_t)(strlen(log_path) + 1)) {
		g_priv_debug.force_dump_log = true;
		up(&socdsp_log_sema);
	}

	return ret;
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,10,0))
static const struct proc_ops socdsp_dspdumplog_ops = {
	.proc_read  = socdsp_dump_log_show,
};
#else
static const struct file_operations socdsp_dspdumplog_ops = {
	.owner = THIS_MODULE,
	.read  = socdsp_dump_log_show,
};
#endif

static int create_debug_proc_file(void)
{
	struct proc_dir_entry *ent_debuglevel = NULL;
	struct proc_dir_entry *ent_dspdumplog = NULL;
	struct proc_dir_entry *ent_dspfaultinject = NULL;
	struct proc_dir_entry *poc_miscproc_option = NULL;

	/* Creating read/write "status" entry */
	ent_debuglevel = proc_create(SOCDSP_DEBUG_LEVEL_PROC_FILE, 0660,
			g_socdsp_debug_dir, &socdsp_debug_proc_ops);
	if (!ent_debuglevel) {
		loge("create proc file fail\n");
		goto err_create_debuglevel;
	}

	ent_dspdumplog = proc_create(SOCDSP_DEBUG_DSPDUMPLOG_PROC_FILE, 0440,
		g_socdsp_debug_dir, &socdsp_dspdumplog_ops);
	if (!ent_dspdumplog) {
		loge("create proc file fail\n");
		goto err_create_dspdumplog;
	}

	ent_dspfaultinject = proc_create(SOCDSP_DEBUG_FAULTINJECT_PROC_FILE, 0660,
		g_socdsp_debug_dir, &socdsp_dspfaultinject_proc_ops);
	if (!ent_dspfaultinject) {
		loge("create proc file fail\n");
		goto err_create_dspfaultinject;
	}

	poc_miscproc_option = proc_create(SOCDSP_DEBUG_MISCPROC_PROC_FILE, 0660,
		g_socdsp_debug_dir, &socdsp_misc_proc_proc_ops);
	if (!poc_miscproc_option) {
		loge("create proc file fail\n");
		goto err_create_miscproc_option;
	}

	return 0;

err_create_miscproc_option:
	remove_proc_entry(SOCDSP_DEBUG_FAULTINJECT_PROC_FILE, g_socdsp_debug_dir);
err_create_dspfaultinject:
	remove_proc_entry(SOCDSP_DEBUG_DSPDUMPLOG_PROC_FILE, g_socdsp_debug_dir);
err_create_dspdumplog:
	remove_proc_entry(SOCDSP_DEBUG_LEVEL_PROC_FILE, g_socdsp_debug_dir);
err_create_debuglevel:
	return -EPERM;
}

static void remove_debug_proc_file(void)
{
	remove_proc_entry(SOCDSP_DEBUG_LEVEL_PROC_FILE, g_socdsp_debug_dir);
	remove_proc_entry(SOCDSP_DEBUG_DSPDUMPLOG_PROC_FILE, g_socdsp_debug_dir);
	remove_proc_entry(SOCDSP_DEBUG_FAULTINJECT_PROC_FILE, g_socdsp_debug_dir);
	remove_proc_entry(SOCDSP_DEBUG_MISCPROC_PROC_FILE, g_socdsp_debug_dir);
}

static void socdsp_create_procfs(void)
{
	int ret;

	g_socdsp_debug_dir = proc_mkdir(SOCDSP_DEBUG_PATH, NULL);
	if (!g_socdsp_debug_dir) {
		loge("unable to create proc soc dsp debug directory\n");
		return;
	}

	ret = create_debug_proc_file();
	if (ret != 0) {
		loge("unable to create proc soc dsp debug file\n");
		remove_proc_entry(SOCDSP_DEBUG_PATH, NULL);
	}
}

static void socdsp_remove_procfs(void)
{
	remove_debug_proc_file();
	remove_proc_entry(SOCDSP_DEBUG_PATH, NULL);
}

static int init_hook_workqueue(void)
{
	g_priv_debug.hook_wq.ctl.wq =
		create_singlethread_workqueue("hifi_om_work_data_hook");
	if (!g_priv_debug.hook_wq.ctl.wq) {
		loge("workqueue create failed\n");
		return -EPERM;
	}

	INIT_WORK(&g_priv_debug.hook_wq.ctl.work, socdsp_om_data_hook);
	spin_lock_init(&g_priv_debug.hook_wq.ctl.lock);
	INIT_LIST_HEAD(&g_priv_debug.hook_wq.ctl.list);

	return 0;
}

static void deinit_hook_workqueue(void)
{
	if (!g_priv_debug.hook_wq.ctl.wq)
		return;

	flush_workqueue(g_priv_debug.hook_wq.ctl.wq);
	destroy_workqueue(g_priv_debug.hook_wq.ctl.wq);
	g_priv_debug.hook_wq.ctl.wq = NULL;
}

void socdsp_debug_data_handle(enum socdsp_om_work_id work_id,
	const unsigned char *addr, unsigned int len)
{
	struct socdsp_om_work *work = NULL;

	if (!addr || 0 == len || len > MAIL_LEN_MAX) {
		loge("addr is null or len is invaled, len: %u", len);
		return;
	}

	if (!g_priv_debug.is_inited) {
		loge("om debug is not init");
		return;
	}

	work = kzalloc(sizeof(*work) + len, GFP_ATOMIC);
	if (!work) {
		loge("malloc size %zu failed\n", sizeof(*work) + len);
		return;
	}

	memcpy(work->data, addr, len); /*lint !e419 */
	work->data_len = len;

	spin_lock_bh(&g_priv_debug.hook_wq.ctl.lock);
	list_add_tail(&work->om_node, &g_priv_debug.hook_wq.ctl.list);
	spin_unlock_bh(&g_priv_debug.hook_wq.ctl.lock);

	if (!queue_work(g_priv_debug.hook_wq.ctl.wq, &g_priv_debug.hook_wq.ctl.work))
		loge("socdsp debug, this work was already on the queue\n");
}
#endif

static void set_om_data(const unsigned char *unsec_virt_addr)
{
	g_priv_debug.first_dump_log = true;

	g_priv_debug.dsp_panic_mark = (unsigned int *)(unsec_virt_addr +
		(DRV_DSP_PANIC_MARK - DSP_UNSEC_BASE_ADDR));
	g_priv_debug.dsp_bin_addr = (char *)(unsec_virt_addr +
		(DSP_DUMP_BIN_ADDR - DSP_UNSEC_BASE_ADDR));
	g_priv_debug.dsp_exception_no = (unsigned int *)(unsec_virt_addr +
		(DRV_DSP_EXCEPTION_NO - DSP_UNSEC_BASE_ADDR));
	g_priv_debug.dsp_log_cur_addr = (unsigned int *)(unsec_virt_addr +
		(DRV_DSP_UART_TO_MEM_CUR_ADDR - DSP_UNSEC_BASE_ADDR));
	g_priv_debug.dsp_log_addr = (char *)(unsec_virt_addr +
		(DRV_DSP_UART_TO_MEM - DSP_UNSEC_BASE_ADDR));
	memset(g_priv_debug.dsp_log_addr, 0, DRV_DSP_UART_TO_MEM_SIZE);
	*g_priv_debug.dsp_log_cur_addr = DRV_DSP_UART_TO_MEM_RESERVE_SIZE;

	g_priv_debug.dsp_debug_kill_addr = (unsigned int *)(unsec_virt_addr +
		(DRV_DSP_KILLME_ADDR - DSP_UNSEC_BASE_ADDR));
	g_priv_debug.dsp_fama_config = (struct drv_fama_config *)(unsec_virt_addr +
		(DRV_DSP_ODT_FAMA_CONFIG_ADDR - DSP_UNSEC_BASE_ADDR));
	g_priv_debug.dsp_stack_addr = (unsigned int *)(unsec_virt_addr +
		(DRV_DSP_STACK_TO_MEM - DSP_UNSEC_BASE_ADDR));

	*(g_priv_debug.dsp_exception_no) = ~0;
	g_priv_debug.pre_exception_no = ~0;
	g_priv_debug.dsp_fama_config->head_magic = DRV_DSP_SOCP_FAMA_HEAD_MAGIC;
	g_priv_debug.dsp_fama_config->flag = DRV_DSP_FAMA_OFF;
	g_priv_debug.dsp_fama_config->rear_magic = DRV_DSP_SOCP_FAMA_REAR_MAGIC;

	g_dsp_dump_info[NORMAL_BIN].data_addr = g_priv_debug.dsp_bin_addr;
	g_dsp_dump_info[PANIC_BIN].data_addr  = g_priv_debug.dsp_bin_addr;
}

int om_debug_init(unsigned char *unsec_virt_addr)
{
	int ret = 0;

#ifdef DYNAMIC_LOAD_HIFIIMG
#ifdef CONFIG_AUDIO_COMMON_IMAGE
	bind_image_partition_addr();
#endif
#endif
	memset(&g_priv_debug, 0, sizeof(g_priv_debug));

	g_priv_debug.reset_system = false;
	g_priv_debug.is_inited = false;

	set_om_data(unsec_virt_addr);

	sema_init(&g_priv_debug.dsp_dump_sema, 1);

	g_priv_debug.kdumpdsp_task = kthread_create(socdsp_dump_thread, 0, "dspdumplog");
	if (IS_ERR(g_priv_debug.kdumpdsp_task)) {
		loge("creat socdsp dump log thread fail\n");
		return -EPERM;
	} else {
		wake_up_process(g_priv_debug.kdumpdsp_task);
	}

#ifdef ENABLE_HIFI_DEBUG
	ret = init_hook_workqueue();
	if (ret != 0) {
		kthread_stop(g_priv_debug.kdumpdsp_task);
		g_priv_debug.kdumpdsp_task = NULL;
		return ret;
	}

	socdsp_create_procfs();
	memset(&g_dsp_data_hook, 0, sizeof(g_dsp_data_hook));
	g_dsp_data_hook.priv_data_base = unsec_virt_addr;
#endif
	g_priv_debug.is_inited = true;

	return ret;
}

void om_debug_deinit(void)
{
	if (g_priv_debug.is_inited)
		up(&g_priv_debug.dsp_dump_sema);

	if (g_priv_debug.kdumpdsp_task) {
		kthread_stop(g_priv_debug.kdumpdsp_task);
		g_priv_debug.kdumpdsp_task = NULL;
	}

	g_priv_debug.is_inited = false;

#ifdef ENABLE_HIFI_DEBUG
	socdsp_remove_procfs();

	deinit_hook_workqueue();
#endif
}
