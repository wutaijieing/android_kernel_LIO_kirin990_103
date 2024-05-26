/*
 * Copyright (C) Huawei Tech. Co. Ltd. 2017-2019. All rights reserved.
 * Description: dev drvier to communicate with sensorhub swing app
 * Create: 2017.12.05
 */
#include <linux/version.h>
#include <linux/init.h>
#include <linux/notifier.h>
#include <linux/uaccess.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/err.h>
#include <linux/time.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/kfifo.h>
#include <linux/debugfs.h>
#include <linux/io.h>
#include <linux/ion.h>
#include <linux/platform_drivers/mm_ion.h>
#include <linux/mm_iommu.h>
#include <linux/of.h>
#include <linux/dma-buf.h>
#include <linux/dma-mapping.h>
#include <linux/syscalls.h>
#include <linux/miscdevice.h>
#include <linux/completion.h>
#include "inputhub_api/inputhub_api.h"
#include "common/common.h"
#include "shmem/shmem.h"
#include <securec.h>
#include <platform_include/smart/linux/base/ap/protocol.h>
#include "mm_lb.h"
#include "igs_dbg_dev.h"
#include "igs_ion.h"

#define swing_dbg_log_info(msg...) pr_info("[I/SWING]" msg)
#define swing_dbg_log_err(msg...) pr_err("[E/SWING]" msg)
#define swing_dbg_log_warn(msg...) pr_warn("[W/SWING]" msg)

#ifdef __LLT_UT__
#define STATIC
#else
#define STATIC static
#endif

struct swing_dbg_read_data_t {
	u32 recv_len;
	void *p_recv;
};

typedef int (*swing_dbg_ioctl_ops_f)(struct file *file, unsigned int cmd, unsigned long arg);
typedef int (*swing_dbg_resp_ops_f)(const struct pkt_subcmd_resp *p_resp);

struct swing_dbg_ioctl_ops {
	unsigned int cmd;
	swing_dbg_ioctl_ops_f ops;
};

struct swing_dbg_resp_ops {
	unsigned int tag;
	unsigned int cmd;
	unsigned int resp_len;
	swing_dbg_resp_ops_f ops;
};

STATIC int ioctl_step(struct file *file, unsigned int cmd, unsigned long arg);
STATIC int ioctl_resume(struct file *file, unsigned int cmd, unsigned long arg);
STATIC int ioctl_stall(struct file *file, unsigned int cmd, unsigned long arg);
STATIC int ioctl_load_model(struct file *file, unsigned int cmd, unsigned long arg);
STATIC int ioctl_unload_model(struct file *file, unsigned int cmd, unsigned long arg);
STATIC int ioctl_run_model(struct file *file, unsigned int cmd, unsigned long arg);
STATIC int ioctl_get_info(struct file *file, unsigned int cmd, unsigned long arg);
STATIC int ioctl_list_models(struct file *file, unsigned int cmd, unsigned long arg);
STATIC int ioctl_debug_set_bkpt(struct file *file, unsigned int cmd, unsigned long arg);
STATIC int ioctl_debug_rm_bkpt(struct file *file, unsigned int cmd, unsigned long arg);
STATIC int ioctl_debug_read(struct file *file, unsigned int cmd, unsigned long arg);
STATIC int ioctl_debug_write(struct file *file, unsigned int cmd, unsigned long arg);
STATIC int ioctl_debug_show_bkpts(struct file *file, unsigned int cmd, unsigned long arg);
STATIC int ioctl_debug_dump(struct file *file, unsigned int cmd, unsigned long arg);
STATIC int ioctl_debug_dbg_model(struct file *file, unsigned int cmd, unsigned long arg);
STATIC int ioctl_debug_list_tasks(struct file *file, unsigned int cmd, unsigned long arg);
STATIC int ioctl_run_blas(struct file *file, unsigned int cmd, unsigned long arg);
STATIC int ioctl_get_phys(struct file *file, unsigned int cmd, unsigned long arg);
STATIC int ioctl_igs_sensor_test(struct file *file, unsigned int cmd, unsigned long arg);

#ifdef CONFIG_CONTEXTHUB_IGS_20
STATIC int ioctl_config_profile(struct file *file, unsigned int cmd, unsigned long arg);
STATIC int ioctl_fake_suspend(struct file *file, unsigned int cmd, unsigned long arg);
STATIC int ioctl_fake_resume(struct file *file, unsigned int cmd, unsigned long arg);
#endif

STATIC int resp_load_model(const struct pkt_subcmd_resp *p_resp);
STATIC int resp_unload_model(const struct pkt_subcmd_resp *p_resp);
STATIC int resp_run_model(const struct pkt_subcmd_resp *p_resp);
STATIC int resp_list_models(const struct pkt_subcmd_resp *p_resp);
STATIC int resp_complete(const struct pkt_subcmd_resp *p_resp);
STATIC int resp_common(const struct pkt_subcmd_resp *p_resp);
STATIC int resp_read(const struct pkt_subcmd_resp *p_resp);
STATIC int resp_show_bkpts(const struct pkt_subcmd_resp *p_resp);
STATIC int resp_list_tasks(const struct pkt_subcmd_resp *p_resp);
STATIC int resp_dbg_isr(const struct pkt_subcmd_resp *p_resp);
STATIC int resp_get_info(const struct pkt_subcmd_resp *p_resp);
STATIC int resp_igs_sensor_test(const struct pkt_subcmd_resp *p_resp);


STATIC struct swing_dbg_ioctl_ops g_swing_dbg_ioctl_ops[] = {
	{ SWING_DBG_IOCTL_SINGLE_STEP, ioctl_step },
	{ SWING_DBG_IOCTL_RESUME, ioctl_resume },
	{ SWING_DBG_IOCTL_SUSPEND, ioctl_stall },
	{ SWING_DBG_IOCTL_LOAD_MODEL, ioctl_load_model },
	{ SWING_DBG_IOCTL_UNLOAD_MODEL, ioctl_unload_model },
	{ SWING_DBG_IOCTL_RUN_MODEL, ioctl_run_model },
	{ SWING_DBG_IOCTL_GET_INFO, ioctl_get_info },
	{ SWING_DBG_IOCTL_LIST_MODELS, ioctl_list_models },

	{ SWING_DBG_IOCTL_SET_BKPT, ioctl_debug_set_bkpt },
	{ SWING_DBG_IOCTL_RM_BKPT, ioctl_debug_rm_bkpt },
	{ SWING_DBG_IOCTL_READ, ioctl_debug_read },
	{ SWING_DBG_IOCTL_WRITE, ioctl_debug_write },
	{ SWING_DBG_IOCTL_SHOW_BKPTS, ioctl_debug_show_bkpts },
	{ SWING_DBG_IOCTL_DUMP, ioctl_debug_dump },
	{ SWING_DBG_IOCTL_DBG_MODEL, ioctl_debug_dbg_model },
	{ SWING_DBG_IOCTL_LIST_TASKS, ioctl_debug_list_tasks },
	{ SWING_DBG_IOCTL_RUN_BLAS, ioctl_run_blas },
	{ SWING_DBG_IOCTL_GET_PHYS, ioctl_get_phys },
	{ SWING_DBG_IOCTL_IGS_SENSOR_TEST, ioctl_igs_sensor_test },
#ifdef CONFIG_CONTEXTHUB_IGS_20
	{ SWING_DBG_IOCTL_PROFILE_CONFIG, ioctl_config_profile },
	{ SWING_DBG_IOCTL_FAKE_SUSPEND, ioctl_fake_suspend },
	{ SWING_DBG_IOCTL_FAKE_RESUME, ioctl_fake_resume },
#endif
};

STATIC struct swing_dbg_resp_ops g_swing_dbg_resp_ops[] = {
	{ TAG_IGS, SUB_CMD_SWING_LOAD_MODEL, sizeof(struct swing_dbg_load_resp_t), resp_load_model},
	{ TAG_IGS, SUB_CMD_SWING_UNLOAD_MODEL, sizeof(struct swing_dbg_unload_resp_t), resp_unload_model},
	{ TAG_IGS, SUB_CMD_SWING_RUN_MODEL, sizeof(struct swing_dbg_run_resp_t), resp_run_model},
	{ TAG_IGS, SUB_CMD_SWING_GET_INFO, sizeof(struct swing_dbg_get_info_resp_t), resp_get_info},
	{ TAG_IGS, SUB_CMD_SWING_LIST_MODELS, sizeof(struct swing_dbg_get_list_resp_t), resp_list_models},
	{ TAG_IGS, SUB_CMD_SWING_SCREEN_OFF, 0, resp_complete},
	{ TAG_IGS, SUB_CMD_SWING_SYSCACHE_DISABLE, 0, resp_complete},

	{ TAG_SWING_DBG, SUB_CMD_SWING_DBG_SET_BKPT, sizeof(struct swing_dbg_comm_resp_t), resp_common},
	{ TAG_SWING_DBG, SUB_CMD_SWING_DBG_RM_BKPT, sizeof(struct swing_dbg_comm_resp_t), resp_common},
	{ TAG_SWING_DBG, SUB_CMD_SWING_DBG_SINGLE_STEP, sizeof(struct swing_dbg_comm_resp_t), resp_common},
	{ TAG_SWING_DBG, SUB_CMD_SWING_DBG_DUMP, sizeof(struct swing_dbg_comm_resp_t), resp_common},
	{ TAG_SWING_DBG, SUB_CMD_SWING_DBG_RESUME, sizeof(struct swing_dbg_comm_resp_t), resp_common},
	{ TAG_SWING_DBG, SUB_CMD_SWING_DBG_PROFILE_CONFIG, sizeof(struct swing_dbg_comm_resp_t), resp_common},
	{ TAG_SWING_DBG, SUB_CMD_SWING_DBG_RUN_BLAS, sizeof(struct swing_dbg_comm_resp_t), resp_common},
	{ TAG_SWING_DBG, SUB_CMD_SWING_DBG_WRITE, sizeof(struct swing_dbg_comm_resp_t), resp_common},
	{ TAG_SWING_DBG, SUB_CMD_SWING_DBG_READ, sizeof(struct swing_dbg_read_resp_t), resp_read},
	{ TAG_SWING_DBG, SUB_CMD_SWING_DBG_SHOW_BKPTS, sizeof(struct swing_dbg_show_bkpts_resp_t), resp_show_bkpts},
	{ TAG_SWING_DBG, SUB_CMD_SWING_DBG_LIST_TASKS, sizeof(struct swing_dbg_tasks_list_resp_t), resp_list_tasks},
	{ TAG_SWING_DBG, SUB_CMD_SWING_DBG_DBG_ISR, sizeof(struct swing_dbg_dbg_isr_t), resp_dbg_isr},
	{ TAG_SWING_DBG, SUB_CMD_SWING_DBG_PROFILE_PMU_DATA, sizeof(struct swing_dbg_pmu_data), NULL},
	{ TAG_SWING_DBG, SUB_CMD_SWING_DBG_SENSOR_TEST, sizeof(union igs_sensor_test_report_union), resp_igs_sensor_test},
};

struct swing_dbg_priv_t {
	struct device *self; /* self device. */
	struct completion read_wait; /* Used to synchronize user space read profile */
	struct completion swing_dbg_wait;
	struct mutex read_mutex; /* Used to protect Memory pool */
	struct mutex swing_dbg_mutex; /* Used to ioctl */
	struct kfifo read_kfifo; /* kfifo for read */
#ifdef CONFIG_CONTEXTHUB_IGS_20
	struct wakeup_source* swing_dbg_wklock;
	bool is_fake_suspend;
#endif
	struct swing_dbg_read_resp_t read_resp;
	struct swing_dbg_show_bkpts_resp_t show_bkpts_resp;
	struct swing_dbg_tasks_list_resp_t tasks_list_resp;
	struct swing_dbg_get_info_resp_t model_info;
	struct swing_dbg_get_list_resp_t models_list;
	struct swing_dbg_load_resp_t load_resp;
	struct swing_dbg_unload_resp_t unload_resp;
	struct swing_dbg_run_resp_t run_resp;
	union igs_sensor_test_report_union igs_sensor_test_resp;
	struct swing_dbg_comm_resp_t comm_resp;
	int ref_cnt;
};

#define SWING_DBG_IOCTL_WAIT_TMOUT      (5000) /* ms */
#define SWING_MAX_SHMEM_LENGTH_BYTE     (16 * 1024)
#define SWING_DBG_READ_CACHE_COUNT      (5)
#define SWING_DBG_SYSCAHCE_ID           (8)
#define SWING_DBG_DMA_MASK              (0xFFFFFFFFFFFFFFFF)
#define SWING_DBG_RESET_NOTIFY          (0xFFFF)

static struct swing_dbg_priv_t g_swing_dbg_priv = {0};
static unsigned long long g_swing_dbg_dmamask = SWING_DBG_DMA_MASK;

STATIC inline unsigned int swing_dbg_get_ioctl_ops_num(void)
{
	return (sizeof(g_swing_dbg_ioctl_ops) / sizeof(struct swing_dbg_ioctl_ops));
}

STATIC inline unsigned int swing_dbg_get_resp_ops_num(void)
{
	return (sizeof(g_swing_dbg_resp_ops) / sizeof(struct swing_dbg_resp_ops));
}

STATIC void swing_dbg_wait_init(struct completion *p_wait)
{
	if (p_wait == NULL) {
		swing_dbg_log_err("%s: wait NULL\n", __func__);
		return;
	}
	init_completion(p_wait);
}

STATIC int swing_dbg_wait_completion(struct completion *p_wait, unsigned int tm_out)
{
	if (p_wait == NULL) {
		swing_dbg_log_err("%s: wait NULL\n", __func__);
		return -EFAULT;
	}

	swing_dbg_log_info("%s: waitting\n", __func__);
	if (tm_out != 0) {
		if (!wait_for_completion_interruptible_timeout(p_wait, msecs_to_jiffies(tm_out))) {
			swing_dbg_log_warn("%s: wait timeout\n", __func__);
			return -ETIMEOUT;
		}
	} else {
		if (wait_for_completion_interruptible(p_wait)) {
			swing_dbg_log_warn("%s: wait interrupted\n", __func__);
			return -EFAULT;
		}
	}

	return 0;
}

STATIC void swing_dbg_complete(struct completion *p_wait)
{
	if (p_wait == NULL) {
		swing_dbg_log_err("%s: wait NULL\n", __func__);
		return;
	}

	complete(p_wait);
}

STATIC int ioctl_load_model(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct swing_dbg_load_model_param_t load = { {0} };
	int ret;

	if (arg == 0) {
		swing_dbg_log_err("[%s] arg NULL.\n", __func__);
		return -EFAULT;
	}

	swing_dbg_log_info("%s enter.....\n", __func__);
	if (copy_from_user((void *)&load, (void *)(((uintptr_t)arg)), sizeof(struct swing_dbg_load_model_param_t))) {
		swing_dbg_log_err("[%s]copy_from_user error\n", __func__);
		return -EFAULT;
	}

	if (load.load.blk_cnt > MAX_SWING_DBG_MODEL_BLKS) {
		swing_dbg_log_err("[%s]too much blocks.\n", __func__);
		return -EFAULT;
	}

	ret = send_cmd_from_kernel(TAG_IGS, CMD_CMN_CONFIG_REQ, SUB_CMD_SWING_LOAD_MODEL,
				   (char *)&load.load, sizeof(struct swing_dbg_load_model_t));
	if (ret != 0)
		return -EFAULT;

	ret = swing_dbg_wait_completion(&g_swing_dbg_priv.swing_dbg_wait, SWING_DBG_IOCTL_WAIT_TMOUT);
	if (ret != 0)
		return ret;

	swing_dbg_log_info("load ret: 0x%x\n", g_swing_dbg_priv.load_resp.ret_code);

	ret = memcpy_s((void *)(&load.load_resp), sizeof(struct swing_dbg_load_resp_t),
		       (void *)(&g_swing_dbg_priv.load_resp), sizeof(struct swing_dbg_load_resp_t));
	if (ret != EOK)
		swing_dbg_log_err("%s memcpy buffer fail, ret[%d]\n", __func__, ret);

	if (copy_to_user((void *)(((uintptr_t)arg)), &load, sizeof(struct swing_dbg_load_model_param_t))) {
		swing_dbg_log_err("[%s]copy_to_user error\n", __func__);
		return -EFAULT;
	}

	return 0;
}

STATIC int ioctl_unload_model(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct swing_dbg_unload_model_param_t unload = { {0} };
	int ret;

	if (arg == 0) {
		swing_dbg_log_err("[%s] arg NULL.\n", __func__);
		return -EFAULT;
	}

	if (copy_from_user((void *)&unload, (void *)(((uintptr_t)arg)),\
		sizeof(struct swing_dbg_unload_model_param_t)) != 0) {
		swing_dbg_log_err("[%s]copy_from_user error\n", __func__);
		return -EFAULT;
	}

	ret = send_cmd_from_kernel(TAG_IGS, CMD_CMN_CONFIG_REQ, SUB_CMD_SWING_UNLOAD_MODEL,
				   (char *)&unload.unload, sizeof(struct swing_dbg_unload_model_t));
	if (ret != 0)
		return -EFAULT;

	ret = swing_dbg_wait_completion(&g_swing_dbg_priv.swing_dbg_wait, SWING_DBG_IOCTL_WAIT_TMOUT);
	if (ret != 0)
		return ret;

	swing_dbg_log_info("unload ret: 0x%x\n", g_swing_dbg_priv.unload_resp.ret_code);

	ret = memcpy_s((void *)(&unload.unload_resp), sizeof(struct swing_dbg_unload_resp_t),
		       (void *)(&g_swing_dbg_priv.unload_resp), sizeof(struct swing_dbg_unload_resp_t));
	if (ret != EOK)
		swing_dbg_log_err("%s memcpy buffer fail, ret[%d]\n", __func__, ret);

	if (copy_to_user((void *)(((uintptr_t)arg)), &unload, sizeof(struct swing_dbg_unload_model_param_t))) {
		swing_dbg_log_err("[%s]copy_to_user error\n", __func__);
		return -EFAULT;
	}

	return 0;
}

STATIC int ioctl_run_model(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct swing_dbg_run_model_param_t run = { {0} };
	int ret;

	if (arg == 0) {
		swing_dbg_log_err("[%s] arg NULL.\n", __func__);
		return -EFAULT;
	}

	if (copy_from_user((void *)&run, (void *)(((uintptr_t)arg)), sizeof(struct swing_dbg_run_model_param_t))) {
		swing_dbg_log_err("[%s]copy_from_user error\n", __func__);
		return -EFAULT;
	}

	if ((run.run.input_num > MAX_SWING_DBG_INPUT_NUM) || (run.run.output_num > MAX_SWING_DBG_OUTPUT_NUM))
		return -EFAULT;

	ret = send_cmd_from_kernel(TAG_IGS, CMD_CMN_CONFIG_REQ,
				   SUB_CMD_SWING_RUN_MODEL, (char *)&run.run, sizeof(struct swing_dbg_run_model_t));
	if (ret != 0)
		return -EFAULT;

	ret = swing_dbg_wait_completion(&g_swing_dbg_priv.swing_dbg_wait, SWING_DBG_IOCTL_WAIT_TMOUT);
	if (ret != 0)
		return ret;

	swing_dbg_log_info("run ret: 0x%x\n", g_swing_dbg_priv.run_resp.ret_code);

	ret = memcpy_s((void *)(&run.run_resp), sizeof(struct swing_dbg_run_resp_t),
		       (void *)(&g_swing_dbg_priv.run_resp), sizeof(struct swing_dbg_run_resp_t));
	if (ret != EOK)
		swing_dbg_log_err("%s memcpy buffer fail, ret[%d]\n", __func__, ret);

	if (copy_to_user((void *)(((uintptr_t)arg)), &run, sizeof(struct swing_dbg_run_model_param_t))) {
		swing_dbg_log_err("[%s]copy_to_user error\n", __func__);
		return -EFAULT;
	}

	return 0;
}

STATIC int ioctl_debug_set_bkpt(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct swing_dbg_bkpt_param_t bkpt = { {0} };
	int ret;

	if (arg == 0) {
		swing_dbg_log_err("[%s] arg NULL.\n", __func__);
		return -EFAULT;
	}

	if (copy_from_user((void *)&bkpt, (void *)(((uintptr_t)arg)), sizeof(struct swing_dbg_bkpt_param_t))) {
		swing_dbg_log_err("[%s]copy_from_user error\n", __func__);
		return -EFAULT;
	}

	ret = send_cmd_from_kernel(TAG_SWING_DBG, CMD_CMN_CONFIG_REQ,
				   SUB_CMD_SWING_DBG_SET_BKPT, (char *)(&bkpt.bkpt), sizeof(struct swing_dbg_bkpt_t));
	if (ret != 0)
		return -EFAULT;

	ret = swing_dbg_wait_completion(&g_swing_dbg_priv.swing_dbg_wait, SWING_DBG_IOCTL_WAIT_TMOUT);
	if (ret != 0)
		return ret;

	ret = memcpy_s((void *)(&bkpt.bkpt_resp), sizeof(struct swing_dbg_comm_resp_t),
		       (void *)(&g_swing_dbg_priv.comm_resp), sizeof(struct swing_dbg_comm_resp_t));
	if (ret != EOK)
		swing_dbg_log_err("%s memcpy buffer fail, ret[%d]\n", __func__, ret);

	if (copy_to_user((void *)(((uintptr_t)arg)), &bkpt, sizeof(struct swing_dbg_bkpt_param_t))) {
		swing_dbg_log_err("%s failed to copy to user\n", __func__);
		return -EFAULT;
	}

	return 0;
}

STATIC int ioctl_debug_rm_bkpt(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct swing_dbg_bkpt_param_t bkpt = { {0} };
	int ret;

	if (arg == 0) {
		swing_dbg_log_err("[%s] arg NULL.\n", __func__);
		return -EFAULT;
	}

	if (copy_from_user((void *)&bkpt, (void *)(((uintptr_t)arg)), sizeof(struct swing_dbg_bkpt_param_t))) {
		swing_dbg_log_err("[%s]copy_from_user error\n", __func__);
		return -EFAULT;
	}

	ret = send_cmd_from_kernel(TAG_SWING_DBG, CMD_CMN_CONFIG_REQ, SUB_CMD_SWING_DBG_RM_BKPT,
				   (char *)(&bkpt.bkpt), sizeof(struct swing_dbg_bkpt_t));
	if (ret != 0)
		return -EFAULT;

	ret = swing_dbg_wait_completion(&g_swing_dbg_priv.swing_dbg_wait, SWING_DBG_IOCTL_WAIT_TMOUT);
	if (ret != 0)
		return ret;

	ret = memcpy_s((void *)(&bkpt.bkpt_resp), sizeof(struct swing_dbg_comm_resp_t),
		       (void *)(&g_swing_dbg_priv.comm_resp), sizeof(struct swing_dbg_comm_resp_t));
	if (ret != EOK)
		swing_dbg_log_err("%s memcpy buffer fail, ret[%d]\n", __func__, ret);

	if (copy_to_user((void *)(((uintptr_t)arg)), &bkpt, sizeof(struct swing_dbg_bkpt_param_t))) {
		swing_dbg_log_err("%s failed to copy to user\n", __func__);
		return -EFAULT;
	}

	return 0;
}

STATIC int ioctl_debug_read(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct swing_dbg_read_param_t read = { {0} };
	int ret;

	if (arg == 0) {
		swing_dbg_log_err("[%s] arg NULL.\n", __func__);
		return -EFAULT;
	}

	if (copy_from_user((void *)&read, (void *)((uintptr_t)arg), sizeof(struct swing_dbg_read_param_t))) {
		swing_dbg_log_err("[%s]copy_from_user error\n", __func__);
		return -EFAULT;
	}

	ret = send_cmd_from_kernel(TAG_SWING_DBG, CMD_CMN_CONFIG_REQ,
				   SUB_CMD_SWING_DBG_READ, (char *)(&read.read), sizeof(struct swing_dbg_read_t));
	if (ret != 0)
		return -EFAULT;

	ret = swing_dbg_wait_completion(&g_swing_dbg_priv.swing_dbg_wait, SWING_DBG_IOCTL_WAIT_TMOUT);
	if (ret != 0)
		return ret;

	swing_dbg_log_info("read ret: 0x%x\n", g_swing_dbg_priv.read_resp.ret_code);

	ret = memcpy_s((void *)(&read.read_resp), sizeof(struct swing_dbg_read_resp_t),
		       (void *)(&g_swing_dbg_priv.read_resp), sizeof(struct swing_dbg_read_resp_t));
	if (ret != EOK)
		swing_dbg_log_err("%s memcpy buffer fail, ret[%d]\n", __func__, ret);

	if (copy_to_user((void *)(((uintptr_t)arg)), &read, sizeof(struct swing_dbg_read_param_t))) {
		swing_dbg_log_err("%s failed to copy to user\n", __func__);
		return -EFAULT;
	}

	return 0;
}

STATIC int ioctl_debug_write(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct swing_dbg_write_param_t write = { {0} };
	int ret;

	if (arg == 0) {
		swing_dbg_log_err("[%s] arg NULL.\n", __func__);
		return -EFAULT;
	}

	if (copy_from_user((void *)&write, (void *)(((uintptr_t)arg)), sizeof(struct swing_dbg_write_param_t))) {
		swing_dbg_log_err("[%s]copy_from_user error\n", __func__);
		return -EFAULT;
	}

	ret = send_cmd_from_kernel(TAG_SWING_DBG, CMD_CMN_CONFIG_REQ,
				   SUB_CMD_SWING_DBG_WRITE, (char *)(&write.write), sizeof(struct swing_dbg_write_t));
	if (ret != 0)
		return -EFAULT;

	swing_dbg_log_info("write ret: 0x%x\n", g_swing_dbg_priv.comm_resp.ret_code);

	ret = swing_dbg_wait_completion(&g_swing_dbg_priv.swing_dbg_wait, SWING_DBG_IOCTL_WAIT_TMOUT);
	if (ret != 0)
		return ret;

	ret = memcpy_s((void *)(&write.write_resp), sizeof(struct swing_dbg_comm_resp_t),
		       (void *)(&g_swing_dbg_priv.comm_resp), sizeof(struct swing_dbg_comm_resp_t));
	if (ret != EOK)
		swing_dbg_log_err("%s memcpy buffer fail, ret[%d]\n", __func__, ret);

	if (copy_to_user((void *)(((uintptr_t)arg)), &write, sizeof(struct swing_dbg_write_param_t))) {
		swing_dbg_log_err("%s failed to copy to user\n", __func__);
		return -EFAULT;
	}

	return 0;
}

STATIC int ioctl_debug_show_bkpts(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct swing_dbg_show_bkpts_param_t show_bkpts = { {0} };

	int ret;

	if (arg == 0) {
		swing_dbg_log_err("[%s] arg NULL.\n", __func__);
		return -EFAULT;
	}

	if (copy_from_user((void *)&show_bkpts, (void *)(((uintptr_t)arg)),
		sizeof(struct swing_dbg_show_bkpts_param_t)) != 0) {
		swing_dbg_log_err("[%s]copy_from_user error\n", __func__);
		return -EFAULT;
	}
	ret = send_cmd_from_kernel(TAG_SWING_DBG, CMD_CMN_CONFIG_REQ, SUB_CMD_SWING_DBG_SHOW_BKPTS, NULL, 0);
	if (ret != 0)
		return -EFAULT;

	ret = swing_dbg_wait_completion(&g_swing_dbg_priv.swing_dbg_wait, SWING_DBG_IOCTL_WAIT_TMOUT);
	if (ret != 0)
		return ret;

	swing_dbg_log_info("bkpt num: %d\n", g_swing_dbg_priv.show_bkpts_resp.bkpts_num);

	ret = memcpy_s((void *)(&show_bkpts.show_bkpts_resp), sizeof(struct swing_dbg_show_bkpts_resp_t),
		       (void *)(&g_swing_dbg_priv.show_bkpts_resp), sizeof(struct swing_dbg_show_bkpts_resp_t));
	if (ret != EOK)
		swing_dbg_log_err("%s memcpy buffer fail, ret[%d]\n", __func__, ret);

	if (copy_to_user((void *)(((uintptr_t)arg)), &show_bkpts, sizeof(struct swing_dbg_show_bkpts_param_t))) {
		swing_dbg_log_err("%s failed to copy to user\n", __func__);
		return -EFAULT;
	}
	return 0;
}

STATIC int ioctl_debug_dump(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct swing_dbg_dump_param_t dump = { {0} };
	int ret;

	if (arg == 0) {
		swing_dbg_log_err("[%s] arg NULL.\n", __func__);
		return -EFAULT;
	}

	if (copy_from_user((void *)&dump, (void *)(((uintptr_t)arg)), sizeof(struct swing_dbg_dump_param_t))) {
		swing_dbg_log_err("[%s]copy_from_user error\n", __func__);
		return -EFAULT;
	}

	ret = send_cmd_from_kernel(TAG_SWING_DBG, CMD_CMN_CONFIG_REQ, SUB_CMD_SWING_DBG_DUMP,
				   (char *)(&dump.dump), sizeof(struct swing_dbg_dump_t));
	if (ret != 0)
		return -EFAULT;

	ret = swing_dbg_wait_completion(&g_swing_dbg_priv.swing_dbg_wait, SWING_DBG_IOCTL_WAIT_TMOUT);
	if (ret != 0)
		return ret;

	swing_dbg_log_info("dump ret: 0x%x\n", g_swing_dbg_priv.comm_resp.ret_code);

	ret = memcpy_s((void *)(&dump.dump_resp), sizeof(struct swing_dbg_comm_resp_t),
		       (void *)(&g_swing_dbg_priv.comm_resp), sizeof(struct swing_dbg_comm_resp_t));
	if (ret != EOK)
		swing_dbg_log_err("%s memcpy buffer fail, ret[%d]\n", __func__, ret);

	if (copy_to_user((void *)(((uintptr_t)arg)), &dump, sizeof(struct swing_dbg_dump_param_t))) {
		swing_dbg_log_err("%s failed to copy to user\n", __func__);
		return -EFAULT;
	}

	return 0;
}

STATIC int ioctl_igs_sensor_test(struct file *file, unsigned int cmd, unsigned long arg)
{
	int ret;
	igs_dbg_sensor_test_t test_param = {0};
	swing_dbg_log_warn("ioctl_igs_sensor_test enter\n");

	if (arg == 0) {
		swing_dbg_log_err("ioctl_igs_sensor_test: arg NULL...\n");
		return -EFAULT;
	}

	if (copy_from_user((void *)&test_param, (void *)((uintptr_t)arg), sizeof(igs_dbg_sensor_test_t))) {
		swing_dbg_log_err("ioctl_igs_sensor_test: copy_from_user failed\n");
		return -EFAULT;
	}
	swing_dbg_log_info("ioctl_igs_sensor_test target:%d, cmd:%d\n", test_param.target, test_param.cmd);

	ret = send_cmd_from_kernel(TAG_SWING_DBG, CMD_CMN_CONFIG_REQ, SUB_CMD_SWING_DBG_SENSOR_TEST,
		(char *)(&test_param), sizeof(igs_dbg_sensor_test_t));
	if (ret != 0)
		return -EFAULT;

	ret = swing_dbg_wait_completion(&g_swing_dbg_priv.swing_dbg_wait, SWING_DBG_IOCTL_WAIT_TMOUT);
	if (ret != 0) {
		swing_dbg_log_err("ioctl_igs_sensor_test wait timeout\n");
		return ret;
	}

	ret = memcpy_s(&test_param.report.data, sizeof(union igs_sensor_test_report_union),
		&g_swing_dbg_priv.igs_sensor_test_resp, sizeof(union igs_sensor_test_report_union));
	if (ret != EOK) {
		swing_dbg_log_err("%s memcpy_s failed...\n", __func__);
		return -EFAULT;
	}
	swing_dbg_log_info("ioctl_igs_sensor_test report data[0]:0x%x\n", test_param.report.data[0]);

	if (copy_to_user((void *)(((uintptr_t)arg)), &test_param, sizeof(igs_dbg_sensor_test_t))) {
		swing_dbg_log_err("%s failed to copy to user\n", __func__);
		return -EFAULT;
	}

	return 0;
}

STATIC int ioctl_debug_dbg_model(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct swing_dbg_dbg_model_param_t dm = { {0} };
	int ret;

	if (arg == 0) {
		swing_dbg_log_err("[%s] arg NULL.\n", __func__);
		return -EFAULT;
	}

	if (copy_from_user((void *)&dm, (void *)(((uintptr_t)arg)), sizeof(struct swing_dbg_dbg_model_param_t))) {
		swing_dbg_log_err("[%s]copy_from_user error\n", __func__);
		return -EFAULT;
	}

	ret = send_cmd_from_kernel(TAG_SWING_DBG, CMD_CMN_CONFIG_REQ, SUB_CMD_SWING_DBG_MODEL, (char *)(&dm.dbg_model),
				   sizeof(struct swing_dbg_dbg_model_t));
	if (ret != 0)
		return -EFAULT;

	ret = swing_dbg_wait_completion(&g_swing_dbg_priv.swing_dbg_wait, SWING_DBG_IOCTL_WAIT_TMOUT);
	if (ret != 0)
		return ret;

	swing_dbg_log_info("dbg ret: 0x%x\n", g_swing_dbg_priv.comm_resp.ret_code);

	ret = memcpy_s((void *)(&dm.dbg_model), sizeof(struct swing_dbg_comm_resp_t),
		       (void *)(&g_swing_dbg_priv.comm_resp), sizeof(struct swing_dbg_comm_resp_t));
	if (ret != EOK)
		swing_dbg_log_err("[%s] memcpy_s fail.\n", __func__);

	if (copy_to_user((void *)(((uintptr_t)arg)), &dm, sizeof(struct swing_dbg_dbg_model_param_t))) {
		swing_dbg_log_err("%s failed to copy to user\n", __func__);
		return -EFAULT;
	}

	return 0;
}

STATIC int ioctl_debug_list_tasks(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct swing_dbg_list_tasks_param_t lt = { {0} };
	int ret;

	if (arg == 0) {
		swing_dbg_log_err("[%s] arg NULL.\n", __func__);
		return -EFAULT;
	}

	if (copy_from_user((void *)&lt, (void *)((uintptr_t)arg), sizeof(struct swing_dbg_list_tasks_param_t))) {
		swing_dbg_log_err("[%s]copy_from_user error\n", __func__);
		return -EFAULT;
	}

	ret = send_cmd_from_kernel(TAG_SWING_DBG, CMD_CMN_CONFIG_REQ, SUB_CMD_SWING_DBG_LIST_TASKS,
				   (char *)(&lt.list_tasks), sizeof(struct swing_dbg_list_tasks_t));
	if (ret != 0)
		return -EFAULT;

	ret = swing_dbg_wait_completion(&g_swing_dbg_priv.swing_dbg_wait, SWING_DBG_IOCTL_WAIT_TMOUT);
	if (ret != 0)
		return ret;

	ret = memcpy_s((void *)(&lt.list_tasks_resp), sizeof(struct swing_dbg_tasks_list_resp_t),
		       (void *)(&g_swing_dbg_priv.tasks_list_resp), sizeof(struct swing_dbg_tasks_list_resp_t));
	if (ret != EOK)
		swing_dbg_log_err("%s memcpy buffer fail, ret[%d]\n", __func__, ret);

	if (copy_to_user((void *)(((uintptr_t)arg)), &lt, sizeof(struct swing_dbg_list_tasks_param_t))) {
		swing_dbg_log_err("[%s]copy_to_user error\n", __func__);
		return -EFAULT;
	}

	return 0;
}

STATIC int swing_dbg_profile_receive(const struct pkt_header *head)
{
	const struct swing_dbg_pmu_data *pmu_data = NULL;
	struct swing_dbg_read_data_t read_data = {0};
	int ret;

	if (head == NULL) {
		swing_dbg_log_err("[PROFILE]%s input head is null\n", __func__);
		return -EFAULT;
	}
	pmu_data = &(((struct swing_dbg_shmem_sh *)head)->pmu_data);
	swing_dbg_log_info("[PROFILE]%s : tag[%d], cmd[%d], length[%d], bkpt=%llx\n", __func__,
			   head->tag, head->cmd, head->length, pmu_data->bkpt);
	mutex_lock(&g_swing_dbg_priv.read_mutex);
	if (kfifo_avail(&g_swing_dbg_priv.read_kfifo) < sizeof(struct swing_dbg_read_data_t)) {
		swing_dbg_log_err("[PROFILE]%s read_kfifo is full, drop upload data.\n", __func__);
		goto err_reply_req;
	}

	read_data.recv_len = head->length + sizeof(u32);
	read_data.p_recv = kzalloc(read_data.recv_len, GFP_ATOMIC);
	if (read_data.p_recv == NULL) {
		swing_dbg_log_err("[PROFILE]Failed to alloc memory to save profile data...\n");
		goto err_reply_req;
	}

	*((u32 *)(read_data.p_recv)) = SWING_DBG_READ_PROF;
	ret = memcpy_s(read_data.p_recv + sizeof(u32), read_data.recv_len - sizeof(u32),
		       pmu_data, sizeof(struct swing_dbg_pmu_data));
	if (ret != 0) {
		swing_dbg_log_err("[PROFILE]%s Copy IOMCU return buffer failed, length = %u, errno = %d\n",
				  __func__, (u32)(read_data.recv_len - sizeof(u32)), ret);
		goto err_free_recv;
	}

	if (kfifo_in(&g_swing_dbg_priv.read_kfifo, (unsigned char *)&read_data,\
		sizeof(struct swing_dbg_read_data_t)) == 0) {
		swing_dbg_log_err("[PROFILE]%s: kfifo_in failed.\n", __func__);
		goto err_free_recv;
	}

	mutex_unlock(&g_swing_dbg_priv.read_mutex);
	ret = send_cmd_from_kernel(TAG_SWING_DBG, CMD_CMN_CONFIG_REQ, SUB_CMD_SWING_DBG_PROFILE_PMU_DATA, NULL, 0);
	if (ret != 0)
		swing_dbg_log_err("[PROFILE]%s: shmem_send failed\n", __func__);

	swing_dbg_log_info("[PROFILE]%s wakeup\n", __func__);
	swing_dbg_complete(&g_swing_dbg_priv.read_wait);
	return ret;

err_free_recv:
	if (read_data.p_recv != NULL)
		kfree(read_data.p_recv);

err_reply_req:
	mutex_unlock(&g_swing_dbg_priv.read_mutex);

	ret = send_cmd_from_kernel(TAG_SWING_DBG, CMD_CMN_CONFIG_REQ, SUB_CMD_SWING_DBG_PROFILE_PMU_DATA, NULL, 0);
	if (ret != 0)
		swing_dbg_log_err("%s: shmem_send failed\n", __func__);

	return ret;
}

STATIC ssize_t swing_dbg_read(struct file *file, char __user *buf, size_t count, loff_t *pos)
{
	struct swing_dbg_read_data_t read_data = {0};
	ssize_t error = 0;
	u32 length;
	int ret = 0;

	swing_dbg_log_info("[%s]\n", __func__);
	if (buf == NULL || count == 0) {
		swing_dbg_log_err("%s: param error.\n", __func__);
		goto done;
	}

	ret = swing_dbg_wait_completion(&g_swing_dbg_priv.read_wait, 0);
	if (ret != 0) {
		swing_dbg_log_err("%s: wait_event_interruptible failed.\n", __func__);
		goto done;
	}

	mutex_lock(&g_swing_dbg_priv.read_mutex);
	if (kfifo_len(&g_swing_dbg_priv.read_kfifo) < sizeof(struct swing_dbg_read_data_t)) {
		mutex_unlock(&g_swing_dbg_priv.read_mutex);
		swing_dbg_log_err("%s: read data failed.\n", __func__);
		goto done;
	}

	if (kfifo_out(&g_swing_dbg_priv.read_kfifo,\
		(unsigned char *)&read_data, sizeof(struct swing_dbg_read_data_t)) == 0) {
		mutex_unlock(&g_swing_dbg_priv.read_mutex);
		swing_dbg_log_err("%s: kfifo out failed.\n", __func__);
		goto done;
	}

	if (read_data.p_recv == NULL) {
		mutex_unlock(&g_swing_dbg_priv.read_mutex);
		swing_dbg_log_err("%s: kfifo out is NULL.\n", __func__);
		goto done;
	}

	swing_dbg_log_info("[%s] copy cnt %d.\n", __func__, (int)count);
	if (count < read_data.recv_len) {
		length = count;
		swing_dbg_log_err("%s user buffer is too small\n", __func__);
	} else {
		length = read_data.recv_len;
	}

	error = length;
	/* copy to user */
	if (copy_to_user(buf, read_data.p_recv, length)) {
		swing_dbg_log_err("%s failed to copy to user\n", __func__);
		error = 0;
	}

	mutex_unlock(&g_swing_dbg_priv.read_mutex);

done:
	if (read_data.p_recv != NULL) {
		/* prof buffer is static allocated, don't free it. */
		kfree(read_data.p_recv);
	}

	return error;
}

STATIC ssize_t swing_dbg_write(struct file *file, const char __user *data,
			       size_t len, loff_t *ppos)
{
	swing_dbg_log_info("%s need to do...\n", __func__);
	return len;
}

STATIC int ioctl_run_blas(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct swing_dbg_run_blas_param_t rb = { {0} };
	int ret;

	if (arg == 0) {
		swing_dbg_log_err("[%s] arg NULL.\n", __func__);
		return -EFAULT;
	}

	if (copy_from_user((void *)&rb, (void *)((uintptr_t)arg), sizeof(struct swing_dbg_run_blas_param_t))) {
		swing_dbg_log_err("[%s]copy_from_user error\n", __func__);
		return -EFAULT;
	}

	ret = send_cmd_from_kernel(TAG_SWING_DBG, CMD_CMN_CONFIG_REQ, SUB_CMD_SWING_DBG_RUN_BLAS,
				   (char *)(&rb.run_blas), sizeof(struct swing_dbg_run_blas_t));
	if (ret != 0)
		return -EFAULT;

	ret = swing_dbg_wait_completion(&g_swing_dbg_priv.swing_dbg_wait, SWING_DBG_IOCTL_WAIT_TMOUT);
	if (ret != 0)
		return ret;

	if (copy_to_user((void *)(((uintptr_t)arg)), &rb, sizeof(struct swing_dbg_run_blas_param_t))) {
		swing_dbg_log_err("[%s]copy_to_user error\n", __func__);
		return -EFAULT;
	}

	return 0;
}

STATIC int ioctl_get_phys(struct file *file, unsigned int cmd, unsigned long arg)
{
	dma_addr_t phys_addr;
	struct swing_addr_info_t addr_info;

	swing_dbg_log_info("swing_dbg_ioctl_get_phys....\n");

	if (arg == 0) {
		swing_dbg_log_err("swing_dbg_ioctl_get_phys: arg NULL...\n");
		return -EFAULT;
	}

	if (copy_from_user((void *)&addr_info, (void *)((uintptr_t)arg), sizeof(struct swing_addr_info_t))) {
		swing_dbg_log_err("swing_dbg_ioctl_get_phys: copy_from_user failed\n");
		return -EFAULT;
	}

	swing_dbg_log_info("swing_dbg_ioctl_get_phys: map_fd = %d\n", addr_info.fd);

	if (igs_get_ion_phys(addr_info.fd, (dma_addr_t *)(&phys_addr), g_swing_dbg_priv.self) < 0) {
		swing_dbg_log_err("igs_get_ion_phys: get_ion_phys failed\n");
		return -EFAULT;
	}

	addr_info.pa = (unsigned long long *)((uintptr_t)phys_addr);

	if (copy_to_user((void *)(((uintptr_t)arg)), &addr_info, sizeof(struct swing_addr_info_t))) {
		swing_dbg_log_err("swing_dbg_ioctl_get_phys: copy_to_user failed\n");
		return -EFAULT;
	}

	return 0;
}

#ifdef CONFIG_CONTEXTHUB_IGS_20
STATIC int ioctl_config_profile(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct swing_dbg_shmem_ap config_data = {0};
	struct write_info winfo = {0};
	int ret;

	swing_dbg_log_info("[PROFILE]%s config profile\n", __func__);
	if (arg == 0) {
		swing_dbg_log_err("[PROFILE]%s arg NULL.\n", __func__);
		return -EFAULT;
	}

	config_data.cmd = SUB_CMD_SWING_DBG_PROFILE_CONFIG;
	if (copy_from_user((void *)&config_data.pmu_cfg, (void *)(((uintptr_t)arg)),
			     sizeof(struct swing_dbg_profile_cfg)) != 0) {
		swing_dbg_log_err("[PROFILE]%s Failed to copy user space buffer\n", __func__);
		return -EFAULT;
	}

	winfo.tag = TAG_SWING_DBG;
	winfo.cmd = CMD_CMN_IPCSHM_REQ;
	winfo.wr_buf = (char *)&config_data;
	winfo.wr_len = (int)sizeof(struct swing_dbg_shmem_ap);
	ret = write_customize_cmd(&winfo, NULL, true);
	if (ret != 0)
		pr_err("[PROFILE]%s write_customize_cmd fail\n", __func__);

	return ret;
}

STATIC int ioctl_fake_suspend(struct file *file, unsigned int cmd, unsigned long arg)
{
	int ret = 0;
#ifdef CONFIG_MM_LB
	bool is_quota_req = false;
#endif
	if (g_swing_dbg_priv.is_fake_suspend) {
		swing_dbg_log_info("swing_dbg_ioctl_fake_suspend...dup ops\n");
		return 0;
	}

	swing_dbg_log_info("swing_dbg_ioctl_fake_suspend...\n");

	/* keep awaken when fake suspend */
	if (!g_swing_dbg_priv.swing_dbg_wklock->active) {
		swing_dbg_log_info("[%s] wake lock.\n", __func__);
		__pm_stay_awake(g_swing_dbg_priv.swing_dbg_wklock);
	}

	ret = send_cmd_from_kernel(TAG_IGS, CMD_CMN_CONFIG_REQ,
				   SUB_CMD_SWING_SCREEN_OFF, NULL, (size_t)0);
	if (ret != 0) {
		swing_dbg_log_err("[%s] send screen off failed [%d]\n", __func__, (int)ret);
		goto ret_err;
	}

	ret = swing_dbg_wait_completion(&g_swing_dbg_priv.swing_dbg_wait, SWING_DBG_IOCTL_WAIT_TMOUT);
	if (ret == -ETIMEOUT) {
		swing_dbg_log_err("%s wait screen off ack timeout\n", __func__);
		goto ret_err;
	}
#ifdef CONFIG_MM_LB
	ret = lb_request_quota(SWING_DBG_SYSCAHCE_ID);
	if (ret != 0) {
		swing_dbg_log_err("[%s] lb_request_quota failed[%d]\n", __func__, (int)ret);
		goto ret_err;
	}

	is_quota_req = true;
#endif
	ret = send_cmd_from_kernel(TAG_IGS, CMD_CMN_CONFIG_REQ, SUB_CMD_SWING_SYSCACHE_ENABLE,
				   NULL, (size_t)0);
	if (ret != 0) {
		swing_dbg_log_err("[%s]send syscache enable failed [%d]\n", __func__, (int)ret);
		goto ret_err;
	}

	g_swing_dbg_priv.is_fake_suspend = true;

	return 0;

ret_err:
#ifdef CONFIG_MM_LB
	if (is_quota_req)
		(void)lb_release_quota(SWING_DBG_SYSCAHCE_ID);
#endif

	if (g_swing_dbg_priv.swing_dbg_wklock->active)
		__pm_relax(g_swing_dbg_priv.swing_dbg_wklock);

	return ret;
}

STATIC int ioctl_fake_resume(struct file *file, unsigned int cmd, unsigned long arg)
{
	int ret = 0;
#ifdef CONFIG_MM_LB
	bool is_quota_release = false;
#endif

	if (!g_swing_dbg_priv.is_fake_suspend) {
		swing_dbg_log_info("swing_dbg_ioctl_fake_suspend...dup ops\n");
		return 0;
	}

	swing_dbg_log_info("swing_dbg_ioctl_fake_suspend...\n");

	ret = send_cmd_from_kernel(TAG_IGS, CMD_CMN_CONFIG_REQ,
				   SUB_CMD_SWING_SYSCACHE_DISABLE, NULL, (size_t)0);
	if (ret != 0) {
		swing_dbg_log_err("[%s]send syscache disable failed [%d]\n", __func__, (int)ret);
		goto ret_err;
	}

	ret = swing_dbg_wait_completion(&g_swing_dbg_priv.swing_dbg_wait, SWING_DBG_IOCTL_WAIT_TMOUT);
	if (ret == -ETIMEOUT) {
		swing_dbg_log_err("%s wait syscache disable ack timeout\n", __func__);
		goto ret_err;
	}
#ifdef CONFIG_MM_LB
	ret = lb_release_quota(SWING_DBG_SYSCAHCE_ID);
	if (ret != 0) {
		swing_dbg_log_err("[%s] lb_request_quota failed[%d]\n", __func__, (int)ret);
		goto ret_err;
	}

	is_quota_release = true;
#endif
	ret = send_cmd_from_kernel(TAG_IGS, CMD_CMN_CONFIG_REQ, SUB_CMD_SWING_SCREEN_ON,
				   NULL, (size_t)0);
	if (ret != 0) {
		swing_dbg_log_err("[%s]send cmd error[%d]\n", __func__, (int)ret);
		goto ret_err;
	}

	g_swing_dbg_priv.is_fake_suspend = false;

	return 0;

ret_err:
#ifdef CONFIG_MM_LB
	if (!is_quota_release)
		(void)lb_release_quota(SWING_DBG_SYSCAHCE_ID);
#endif

	if (g_swing_dbg_priv.swing_dbg_wklock->active)
		__pm_relax(g_swing_dbg_priv.swing_dbg_wklock);

	return ret;
}
#endif

STATIC int ioctl_get_info(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct swing_dbg_get_info_param_t get_info = { {0} };
	int ret;

	if (arg == 0) {
		swing_dbg_log_err("[%s] arg NULL.\n", __func__);
		return -EFAULT;
	}

	if (copy_from_user((void *)&get_info, (void *)((uintptr_t)arg), sizeof(struct swing_dbg_get_info_param_t))) {
		swing_dbg_log_err("[%s]copy_from_user error\n", __func__);
		return -EFAULT;
	}

	ret = send_cmd_from_kernel(TAG_IGS, CMD_CMN_CONFIG_REQ, SUB_CMD_SWING_GET_INFO,
				   (char *)(&get_info.get_info), sizeof(struct swing_dbg_get_info_t));
	if (ret != 0)
		return -EFAULT;

	ret = swing_dbg_wait_completion(&g_swing_dbg_priv.swing_dbg_wait, SWING_DBG_IOCTL_WAIT_TMOUT);
	if (ret != 0)
		return ret;

	ret = memcpy_s((void *)(&get_info.get_info_resp), sizeof(struct swing_dbg_get_info_resp_t),
		       (void *)(&g_swing_dbg_priv.model_info), sizeof(struct swing_dbg_get_info_resp_t));
	if (ret != EOK)
		swing_dbg_log_err("%s memcpy buffer fail, ret[%d]\n", __func__, (int)ret);

	if (copy_to_user((void *)(((uintptr_t)arg)), &get_info, sizeof(struct swing_dbg_get_info_param_t))) {
		swing_dbg_log_err("[%s]copy_to_user error\n", __func__);
		return -EFAULT;
	}

	return 0;
}

STATIC int ioctl_list_models(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct swing_dbg_get_list_param_t lm = { {0} };
	int ret;

	if (arg == 0) {
		swing_dbg_log_err("[%s] arg NULL.\n", __func__);
		return -EFAULT;
	}

	if (copy_from_user((void *)&lm, (void *)(((uintptr_t)arg)), sizeof(struct swing_dbg_get_list_param_t))) {
		swing_dbg_log_err("[%s]copy_from_user error\n", __func__);
		return -EFAULT;
	}
	ret = send_cmd_from_kernel(TAG_IGS, CMD_CMN_CONFIG_REQ,
				   SUB_CMD_SWING_LIST_MODELS, NULL, 0);
	if (ret != 0)
		return -EFAULT;

	ret = swing_dbg_wait_completion(&g_swing_dbg_priv.swing_dbg_wait, SWING_DBG_IOCTL_WAIT_TMOUT);
	if (ret != 0)
		return ret;

	ret = memcpy_s((void *)(&lm.get_list_resp), sizeof(struct swing_dbg_get_info_resp_t),
		       (void *)(&g_swing_dbg_priv.models_list), sizeof(struct swing_dbg_get_info_resp_t));
	if (ret != EOK)
		swing_dbg_log_err("%s , %d memcpy buffer fail, ret[%d]\n", __func__, __LINE__, (int)ret);

	if (copy_to_user((void *)(((uintptr_t)arg)), &lm, sizeof(struct swing_dbg_get_list_param_t))) {
		swing_dbg_log_err("[%s]copy_to_user error\n", __func__);
		return -EFAULT;
	}

	return 0;
}

STATIC int ioctl_step(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct swing_dbg_step_param_t step = { {0} };
	int ret;

	if (arg == 0) {
		swing_dbg_log_err("[%s] arg NULL.\n", __func__);
		return -EFAULT;
	}

	if (copy_from_user((void *)&step, (void *)(((uintptr_t)arg)), sizeof(struct swing_dbg_step_param_t))) {
		swing_dbg_log_err("[%s]copy_from_user error\n", __func__);
		return -EFAULT;
	}

	ret = send_cmd_from_kernel(TAG_SWING_DBG, CMD_CMN_CONFIG_REQ, SUB_CMD_SWING_DBG_SINGLE_STEP, NULL, 0);
	if (ret != 0)
		return -EFAULT;

	ret = swing_dbg_wait_completion(&g_swing_dbg_priv.swing_dbg_wait, SWING_DBG_IOCTL_WAIT_TMOUT);
	if (ret != 0)
		return ret;

	swing_dbg_log_info("step ret: 0x%x\n", g_swing_dbg_priv.comm_resp.ret_code);

	ret = memcpy_s((void *)(&step.step_resp), sizeof(struct swing_dbg_comm_resp_t),
		       (void *)(&g_swing_dbg_priv.comm_resp), sizeof(struct swing_dbg_comm_resp_t));
	if (ret != EOK)
		swing_dbg_log_err("%s memcpy buffer fail, ret[%d]\n", __func__, ret);

	if (copy_to_user((void *)(((uintptr_t)arg)), &step, sizeof(struct swing_dbg_step_param_t))) {
		swing_dbg_log_err("%s failed to copy to user\n", __func__);
		return -EFAULT;
	}

	return 0;
}

STATIC int ioctl_resume(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct swing_dbg_resume_param_t resume = { {0} };
	int ret;

	if (arg == 0) {
		swing_dbg_log_err("[%s] arg NULL.\n", __func__);
		return -EFAULT;
	}

	if (copy_from_user((void *)&resume, (void *)(((uintptr_t)arg)), sizeof(struct swing_dbg_resume_param_t))) {
		swing_dbg_log_err("[%s]copy_from_user error\n", __func__);
		return -EFAULT;
	}

	ret = send_cmd_from_kernel(TAG_SWING_DBG, CMD_CMN_CONFIG_REQ, SUB_CMD_SWING_DBG_RESUME, NULL, 0);
	if (ret != 0)
		return -EFAULT;

	ret = swing_dbg_wait_completion(&g_swing_dbg_priv.swing_dbg_wait, SWING_DBG_IOCTL_WAIT_TMOUT);
	if (ret != 0)
		return ret;

	swing_dbg_log_info("resume ret: 0x%x\n", g_swing_dbg_priv.comm_resp.ret_code);

	ret = memcpy_s((void *)(&resume.resume_resp), sizeof(struct swing_dbg_comm_resp_t),
		       (void *)(&g_swing_dbg_priv.comm_resp), sizeof(struct swing_dbg_comm_resp_t));
	if (ret != EOK)
		swing_dbg_log_err("%s memcpy buffer fail, ret[%d]\n", __func__, ret);

	if (copy_to_user((void *)(((uintptr_t)arg)), &resume, sizeof(struct swing_dbg_resume_param_t))) {
		swing_dbg_log_err("%s failed to copy to user\n", __func__);
		return -EFAULT;
	}

	return 0;
}

STATIC int ioctl_stall(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct swing_dbg_stall_param_t stall = { {0} };
	int ret;

	if (arg == 0) {
		swing_dbg_log_err("[%s] arg NULL.\n", __func__);
		return -EFAULT;
	}

	if (copy_from_user((void *)&stall, (void *)(((uintptr_t)arg)), sizeof(struct swing_dbg_stall_param_t))) {
		swing_dbg_log_err("[%s]copy_from_user error\n", __func__);
		return -EFAULT;
	}

	ret = send_cmd_from_kernel(TAG_SWING_DBG, CMD_CMN_CONFIG_REQ, SUB_CMD_SWING_DBG_SUSPEND, NULL, 0);
	if (ret != 0)
		return -EFAULT;

	ret = swing_dbg_wait_completion(&g_swing_dbg_priv.swing_dbg_wait, SWING_DBG_IOCTL_WAIT_TMOUT);
	if (ret != 0)
		return ret;

	swing_dbg_log_info("resume ret: 0x%x\n", g_swing_dbg_priv.comm_resp.ret_code);

	ret = memcpy_s((void *)(&stall.stall_resp), sizeof(struct swing_dbg_comm_resp_t),
		       (void *)(&g_swing_dbg_priv.comm_resp), sizeof(struct swing_dbg_comm_resp_t));
	if (ret != EOK)
		swing_dbg_log_err("%s memcpy buffer fail, ret[%d]\n", __func__, ret);

	if (copy_to_user((void *)(((uintptr_t)arg)), &stall, sizeof(struct swing_dbg_stall_param_t))) {
		swing_dbg_log_err("%s failed to copy to user\n", __func__);
		return -EFAULT;
	}

	return 0;
}

STATIC long swing_dbg_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	long ret = -ENOTTY;
	unsigned int  i;
	unsigned int  cmd_num = swing_dbg_get_ioctl_ops_num();

	swing_dbg_log_info("%s cmd: [0x%x]\n", __func__, cmd);

	mutex_lock(&g_swing_dbg_priv.swing_dbg_mutex);

	reinit_completion(&g_swing_dbg_priv.swing_dbg_wait);

	swing_dbg_log_info("%s reinit completion\n", __func__);
	for (i = 0; i < cmd_num; i++) {
		if (cmd == g_swing_dbg_ioctl_ops[i].cmd)
			break;
	}

	if (i == cmd_num) {
		mutex_unlock(&g_swing_dbg_priv.swing_dbg_mutex);
		swing_dbg_log_err("%s unknown cmd : %d\n", __func__, cmd);
		return -ENOTTY;
	}

	if (g_swing_dbg_ioctl_ops[i].ops != NULL)
		ret = (long)(g_swing_dbg_ioctl_ops[i].ops(file, cmd, arg));

	mutex_unlock(&g_swing_dbg_priv.swing_dbg_mutex);

	if (ret != 0)
		pr_err("[IGS] %s err %x\n", __func__, (unsigned int)ret);

	return ret;
}

STATIC int resp_read(const struct pkt_subcmd_resp *p_resp)
{
	struct swing_dbg_read_resp_t *p_read_resp = NULL;
	int ret;

	p_read_resp = (struct swing_dbg_read_resp_t *)(p_resp + 1);

	ret = memcpy_s(&g_swing_dbg_priv.read_resp, sizeof(struct swing_dbg_read_resp_t),
		       p_read_resp, sizeof(struct swing_dbg_read_resp_t));
	if (ret != EOK) {
		swing_dbg_log_err("%s memcpy_s failed...\n", __func__);
		return -EFAULT;
	}
	swing_dbg_log_info("Read Resp, module %d, addr 0x%x.\n", p_read_resp->module, p_read_resp->addr);
	swing_dbg_complete(&g_swing_dbg_priv.swing_dbg_wait);

	return 0;
}

STATIC int resp_show_bkpts(const struct pkt_subcmd_resp *p_resp)
{
	struct swing_dbg_show_bkpts_resp_t *p_show_bkpts = NULL;
	int ret;

	p_show_bkpts = (struct swing_dbg_show_bkpts_resp_t *)(p_resp + 1);

	ret = memcpy_s(&g_swing_dbg_priv.show_bkpts_resp, sizeof(struct swing_dbg_show_bkpts_resp_t),
		       p_show_bkpts, sizeof(struct swing_dbg_show_bkpts_resp_t));
	if (ret != EOK) {
		swing_dbg_log_err("%s memcpy_s failed...\n", __func__);
		return -EFAULT;
	}
	swing_dbg_log_info("Show Bkpts Resp, %d.\n", g_swing_dbg_priv.show_bkpts_resp.bkpts_num);
	swing_dbg_complete(&g_swing_dbg_priv.swing_dbg_wait);

	return 0;
}

STATIC int resp_list_tasks(const struct pkt_subcmd_resp *p_resp)
{
	struct swing_dbg_tasks_list_resp_t *p_tasks_list = NULL;
	int ret;

	p_tasks_list = (struct swing_dbg_tasks_list_resp_t *)(p_resp + 1);

	ret = memcpy_s(&g_swing_dbg_priv.tasks_list_resp, sizeof(struct swing_dbg_tasks_list_resp_t),
		       p_tasks_list, sizeof(struct swing_dbg_tasks_list_resp_t));
	if (ret != EOK) {
		swing_dbg_log_err("%s memcpy_s failed...\n", __func__);
		return -EFAULT;
	}
	swing_dbg_log_info("Get Tasks List Resp, cnt %d, tot %d.\n",
			   p_tasks_list->task_cnt, p_tasks_list->total_task_cnt);
	swing_dbg_complete(&g_swing_dbg_priv.swing_dbg_wait);

	return 0;
}

STATIC int resp_igs_sensor_test(const struct pkt_subcmd_resp *p_resp)
{
	union igs_sensor_test_report_union *p_report = NULL;
	int ret;

	p_report = (union igs_sensor_test_report_union *)(p_resp + 1);

	ret = memcpy_s(&g_swing_dbg_priv.igs_sensor_test_resp, sizeof(union igs_sensor_test_report_union),
		       p_report, sizeof(union igs_sensor_test_report_union));
	if (ret != EOK) {
		swing_dbg_log_err("%s memcpy_s failed...\n", __func__);
		return -EFAULT;
	}
	swing_dbg_log_info("get igs_sensor_test resp\n");
	swing_dbg_complete(&g_swing_dbg_priv.swing_dbg_wait);

	return 0;
}

STATIC int resp_common(const struct pkt_subcmd_resp *p_resp)
{
	struct swing_dbg_comm_resp_t *p_comm = NULL;
	int ret;

	p_comm = (struct swing_dbg_comm_resp_t *)(p_resp + 1);

	swing_dbg_log_info("Cmd [0x%x] Resp.\n", p_resp->subcmd);
	ret = memcpy_s(&g_swing_dbg_priv.comm_resp,
		       sizeof(struct swing_dbg_comm_resp_t), (void *)p_comm, sizeof(struct swing_dbg_comm_resp_t));
	if (ret != EOK)
		swing_dbg_log_err("%s memcpy_s failed...\n", __func__);

	swing_dbg_complete(&g_swing_dbg_priv.swing_dbg_wait);

	return ret;
}

STATIC int swing_dbg_get_resp(const struct pkt_header *head)
{
	struct pkt_subcmd_resp *p_resp = NULL;
	int ret = 0;
	unsigned int i;
	unsigned int cmd_num = swing_dbg_get_resp_ops_num();

	p_resp = (struct pkt_subcmd_resp *)(head);

	if (p_resp == NULL) {
		swing_dbg_log_err("%s: p_resp null.\n", __func__);
		return -EFAULT;
	}

	swing_dbg_log_info("%s : cmd[%d], length[%d], tag[%d], sub_cmd[%d]\n", __func__,
			   p_resp->hd.cmd, p_resp->hd.length, p_resp->hd.tag, p_resp->subcmd);

	for (i = 0; i < cmd_num; i++) {
		if ((p_resp->subcmd == g_swing_dbg_resp_ops[i].cmd) && (p_resp->hd.tag == g_swing_dbg_resp_ops[i].tag))
			break;
	}

	if (i == cmd_num) {
		swing_dbg_log_warn("unhandled cmd: tag[%d], sub_cmd[%d]\n", p_resp->hd.tag, p_resp->subcmd);
		return -EFAULT;
	}

	if (g_swing_dbg_resp_ops[i].resp_len != 0) {
		if ((g_swing_dbg_resp_ops[i].resp_len + 8) != head->length) {
			swing_dbg_log_err("%s: invalid payload length: tag[%d], sub_cmd[%d], length[0x%x]\n",
				      __func__, p_resp->hd.tag, p_resp->subcmd, head->length);
			return -EFAULT;
		}
	}

	if (g_swing_dbg_resp_ops[i].ops != NULL)
		ret = g_swing_dbg_resp_ops[i].ops(p_resp);

	return ret;
}

STATIC int resp_dbg_isr(const struct pkt_subcmd_resp *p_resp)
{
	struct swing_dbg_dbg_isr_t *p_dbg_isr = NULL;
	struct swing_dbg_read_data_t read_data = {0};
	int ret = 0;

	mutex_lock(&g_swing_dbg_priv.read_mutex);

	if (kfifo_avail(&g_swing_dbg_priv.read_kfifo) < sizeof(struct swing_dbg_read_data_t)) {
		swing_dbg_log_err("%s read_kfifo is full, drop upload data.\n", __func__);
		ret = -EFAULT;
		goto ret_err;
	}

	swing_dbg_log_info("Hit Debug Int.\n");
	p_dbg_isr = (struct swing_dbg_dbg_isr_t *)(p_resp + 1);
	read_data.recv_len = sizeof(u32) + sizeof(struct swing_dbg_dbg_isr_t);
	read_data.p_recv = kzalloc(read_data.recv_len, GFP_ATOMIC);
	if (read_data.p_recv == NULL) {
		swing_dbg_log_err("Failed to alloc memory to save upload resp...\n");
		ret = -EFAULT;
		goto ret_err;
	}

	*((u32 *)(read_data.p_recv)) = SWING_DBG_READ_DBG_ISR;

	ret = memcpy_s(read_data.p_recv + sizeof(u32),
		       sizeof(struct swing_dbg_dbg_isr_t), p_dbg_isr, sizeof(struct swing_dbg_dbg_isr_t));
	if (ret != EOK) {
		swing_dbg_log_err("%s memcpy_s failed...\n", __func__);
		ret = -EFAULT;
		goto ret_err;
	}

	if (kfifo_in(&g_swing_dbg_priv.read_kfifo,\
		(unsigned char *)&read_data, sizeof(struct swing_dbg_read_data_t)) == 0) {
		swing_dbg_log_err("%s: kfifo_in failed.\n", __func__);
		ret = -EFAULT;
		goto ret_err;
	}

	mutex_unlock(&g_swing_dbg_priv.read_mutex);

	swing_dbg_log_info("Debug ISR Received, uid %d.\n", p_dbg_isr->uid);
	swing_dbg_complete(&g_swing_dbg_priv.read_wait);

	return 0;

ret_err:

	if (read_data.p_recv != NULL)
		kfree(read_data.p_recv);

	mutex_unlock(&g_swing_dbg_priv.read_mutex);

	return ret;
}

STATIC int resp_load_model(const struct pkt_subcmd_resp *p_resp)
{
	struct swing_dbg_load_resp_t *p_load_resp = NULL;
	int ret;

	p_load_resp = (struct swing_dbg_load_resp_t *)(p_resp + 1);

	swing_dbg_log_info("Load Model Resp, uid 0x%x.\n", p_load_resp->uid);
	ret = memcpy_s(&g_swing_dbg_priv.load_resp,
		       sizeof(struct swing_dbg_load_resp_t), (void *)p_load_resp, sizeof(struct swing_dbg_load_resp_t));
	if (ret != EOK)
		swing_dbg_log_err("%s memcpy_s failed...\n", __func__);

	swing_dbg_complete(&g_swing_dbg_priv.swing_dbg_wait);

	return ret;
}

STATIC int resp_unload_model(const struct pkt_subcmd_resp *p_resp)
{
	struct swing_dbg_unload_resp_t *p_unload_resp = NULL;
	int ret;

	p_unload_resp = (struct swing_dbg_unload_resp_t *)(p_resp + 1);

	swing_dbg_log_info("UnLoad Model Resp, uid 0x%x.\n", p_unload_resp->uid);
	ret = memcpy_s(&g_swing_dbg_priv.unload_resp,
		       sizeof(struct swing_dbg_unload_resp_t), (void *)p_unload_resp, sizeof(struct swing_dbg_unload_resp_t));
	if (ret != EOK)
		swing_dbg_log_err("%s memcpy_s failed...\n", __func__);

	swing_dbg_complete(&g_swing_dbg_priv.swing_dbg_wait);

	return ret;
}

STATIC int resp_run_model(const struct pkt_subcmd_resp *p_resp)
{
	struct swing_dbg_run_resp_t *p_run_resp = NULL;
	int ret;

	p_run_resp = (struct swing_dbg_run_resp_t *)(p_resp + 1);

	swing_dbg_log_info("Run Model Resp, uid 0x%x.\n", p_run_resp->uid);
	ret = memcpy_s(&g_swing_dbg_priv.run_resp,
		       sizeof(struct swing_dbg_run_resp_t), (void *)p_run_resp, sizeof(struct swing_dbg_run_resp_t));
	if (ret != EOK)
		swing_dbg_log_err("%s memcpy_s failed...\n", __func__);

	swing_dbg_complete(&g_swing_dbg_priv.swing_dbg_wait);

	return ret;
}

STATIC int resp_get_info(const struct pkt_subcmd_resp *p_resp)
{
	struct swing_dbg_get_info_resp_t *p_model_info = NULL;
	int ret;

	p_model_info = (struct swing_dbg_get_info_resp_t *)(p_resp + 1);
	ret = memcpy_s(&g_swing_dbg_priv.model_info, sizeof(struct swing_dbg_get_info_resp_t),
		       p_model_info, sizeof(struct swing_dbg_get_info_resp_t));
	if (ret != EOK) {
		swing_dbg_log_err("%s memcpy_s failed...\n", __func__);
		return -EFAULT;
	}
	swing_dbg_log_info("Get Model Info Resp, uid 0x%x.\n", p_model_info->uid);

	swing_dbg_complete(&g_swing_dbg_priv.swing_dbg_wait);

	return ret;
}

STATIC int resp_list_models(const struct pkt_subcmd_resp *p_resp)
{
	struct swing_dbg_get_list_resp_t *p_models_list = NULL;
	int ret;

	p_models_list = (struct swing_dbg_get_list_resp_t *)(p_resp + 1);
	ret = memcpy_s(&g_swing_dbg_priv.models_list, sizeof(struct swing_dbg_get_list_resp_t),
		       p_models_list, sizeof(struct swing_dbg_get_list_resp_t));
	if (ret != EOK) {
		swing_dbg_log_err("%s memcpy_s failed...\n", __func__);
		return -EFAULT;
	}
	swing_dbg_log_info("Get Model List Resp, cnt %d.\n", p_models_list->model_cnt);

	swing_dbg_complete(&g_swing_dbg_priv.swing_dbg_wait);

	return ret;
}

STATIC int resp_complete(const struct pkt_subcmd_resp *p_resp)
{
	swing_dbg_complete(&g_swing_dbg_priv.swing_dbg_wait);

	return 0;
}

STATIC int swing_dbg_open(struct inode *inode, struct file *file)
{
	int ret;

	swing_dbg_log_info("enter %s.\n", __func__);
	mutex_lock(&g_swing_dbg_priv.swing_dbg_mutex);

	if (g_swing_dbg_priv.ref_cnt != 0) {
		swing_dbg_log_err("%s: duplicate open.\n", __func__);
		mutex_unlock(&g_swing_dbg_priv.swing_dbg_mutex);
		return -EFAULT;
	}

	ret = send_cmd_from_kernel(TAG_SWING_DBG, CMD_CMN_OPEN_REQ, 0, NULL, (size_t)0);
	if (ret != 0) {
		mutex_unlock(&g_swing_dbg_priv.swing_dbg_mutex);
		swing_dbg_log_err("%s: send_cmd_from_kernel fail ret=%d.\n", __func__, ret);
		return -EFAULT;
	}

	file->private_data = &g_swing_dbg_priv;
	g_swing_dbg_priv.ref_cnt++;

	mutex_unlock(&g_swing_dbg_priv.swing_dbg_mutex);

	return 0;
}

STATIC int igs_dbg_flush(struct file *file, fl_owner_t id)
{
	struct read_info rd;
	int ret;

	swing_dbg_log_info("enter %s.\n", __func__);
	mutex_lock(&g_swing_dbg_priv.swing_dbg_mutex);
	if (g_swing_dbg_priv.ref_cnt == 0) {
		swing_dbg_log_err("%s: ref cnt is 0.\n", __func__);
		mutex_unlock(&g_swing_dbg_priv.swing_dbg_mutex);
		return -EFAULT;
	}

	g_swing_dbg_priv.ref_cnt--;
	if (g_swing_dbg_priv.ref_cnt == 0) {
		(void)memset_s((void *)&rd, sizeof(struct read_info), 0, sizeof(struct read_info));
		ret = send_cmd_from_kernel_response(TAG_SWING_DBG, CMD_CMN_CLOSE_REQ, 0, NULL, (size_t)0, &rd);
		if (ret != 0) {
			mutex_unlock(&g_swing_dbg_priv.swing_dbg_mutex);
			swing_dbg_log_err("%s: send_cmd_from_kernel_response fail ret=%d,errno=%d\n",
					  __func__, ret, rd.errno);
			return -EFAULT;
		}
		swing_dbg_log_info("%s: got close resp\n", __func__);
	}

	mutex_unlock(&g_swing_dbg_priv.swing_dbg_mutex);
	swing_dbg_complete(&g_swing_dbg_priv.read_wait);
	return 0;
}

STATIC void swing_dbg_sensorhub_reset_handler(void)
{
	swing_dbg_log_info("%s...\n", __func__);

	swing_dbg_complete(&g_swing_dbg_priv.read_wait);
	swing_dbg_complete(&g_swing_dbg_priv.swing_dbg_wait);
}

STATIC int swing_dbg_sensorhub_reset_notifier(struct notifier_block *nb, unsigned long action, void *data)
{
	switch (action) {
	case IOM3_RECOVERY_IDLE:
		swing_dbg_sensorhub_reset_handler();
		break;
	default:
		break;
	}

	return 0;
}

static struct notifier_block swing_dbg_reboot_notify = {
	.notifier_call = swing_dbg_sensorhub_reset_notifier,
	.priority = -1,
};

static const struct file_operations swing_dbg_fops = {
	.owner             = THIS_MODULE,
	.llseek            = no_llseek,
	.unlocked_ioctl    = swing_dbg_ioctl,
	.open              = swing_dbg_open,
	.flush             = igs_dbg_flush,
	.read              = swing_dbg_read,
	.write             = swing_dbg_write,
};

static struct miscdevice swing_dbg_miscdev = {
	.minor =    MISC_DYNAMIC_MINOR,
	.name =     "swing_dbg_dev",
	.fops =     &swing_dbg_fops,
};

STATIC int __init swing_dbg_init(struct platform_device *pdev)
{
	if (is_sensorhub_disabled()) {
		swing_dbg_log_err("sensorhub disabled....\n");
		return -EFAULT;
	}

	if (g_config_on_ddr->igs_hardware_bypass != 0) {
		swing_dbg_log_warn("%s: swing hardware bypass!\n", __func__);
		return -ENODEV;
	}

	mutex_init(&g_swing_dbg_priv.read_mutex);
	mutex_init(&g_swing_dbg_priv.swing_dbg_mutex);

	swing_dbg_wait_init(&g_swing_dbg_priv.read_wait);
	swing_dbg_wait_init(&g_swing_dbg_priv.swing_dbg_wait);

	if (misc_register(&swing_dbg_miscdev) != 0) {
		swing_dbg_log_err("%s cannot register miscdev.\n", __func__);
		return -EFAULT;
	}

	/* wait for swing_dbg response. */
	if (register_mcu_event_notifier(TAG_SWING_DBG, CMD_CMN_CONFIG_RESP, swing_dbg_get_resp) != 0) {
		swing_dbg_log_err("[%s]: register notifier failed.\n", __func__);
		goto err1;
	}

	// wait for swing response.
	if (register_mcu_event_notifier(TAG_IGS, CMD_CMN_CONFIG_RESP, swing_dbg_get_resp) != 0) {
		swing_dbg_log_err("[%s]: register notifier failed.\n", __func__);
		goto err2;
	}

	/* wait for profile response. */
	if (register_mcu_event_notifier(TAG_SWING_DBG, CMD_DATA_PMU_PROFILE, swing_dbg_profile_receive) != 0) {
		swing_dbg_log_err("[%s]: register notifier failed.\n", __func__);
		goto err3;
	}

	if (register_iom3_recovery_notifier(&swing_dbg_reboot_notify) < 0) {
		swing_dbg_log_err("[%s]register_iom3_recovery_notifier fail\n", __func__);
		goto err4;
	}

	if (kfifo_alloc(&g_swing_dbg_priv.read_kfifo,
			sizeof(struct swing_dbg_read_data_t) * SWING_DBG_READ_CACHE_COUNT, GFP_KERNEL)) {
		swing_dbg_log_err("%s kfifo alloc failed.\n", __func__);
		goto err5;
	}

	g_swing_dbg_priv.ref_cnt = 0;
	g_swing_dbg_priv.self = &(pdev->dev);

#ifdef CONFIG_CONTEXTHUB_IGS_20
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
	g_swing_dbg_priv.swing_dbg_wklock = wakeup_source_register(g_swing_dbg_priv.self, "swing-dbg-wklock");
#else
	g_swing_dbg_priv.swing_dbg_wklock = wakeup_source_register("swing-dbg-wklock");
#endif
	g_swing_dbg_priv.is_fake_suspend = false;
#endif
	pdev->dev.dma_mask = &g_swing_dbg_dmamask;
	pdev->dev.coherent_dma_mask = SWING_DBG_DMA_MASK;

	return 0;

err5:
	/* unregister_iom3_recovery_notifier */
err4:
	/* wait for profile response. */
	unregister_mcu_event_notifier(TAG_SWING_DBG, CMD_DATA_PMU_PROFILE, swing_dbg_profile_receive);
err3:
	//wait for swing response.
	unregister_mcu_event_notifier(TAG_IGS, CMD_CMN_CONFIG_RESP, swing_dbg_get_resp);
err2:
	//wait for swing_dbg response.
	unregister_mcu_event_notifier(TAG_SWING_DBG, CMD_CMN_CONFIG_RESP, swing_dbg_get_resp);
err1:
	misc_deregister(&swing_dbg_miscdev);

	swing_dbg_log_err("%s : init failed.\n", __func__);

	return -EFAULT;
}

STATIC void __exit swing_dbg_exit(void)
{
	swing_dbg_log_info("%s....\n", __func__);

#ifdef CONFIG_CONTEXTHUB_IGS_20
	if (g_swing_dbg_priv.swing_dbg_wklock->active)
		__pm_relax(g_swing_dbg_priv.swing_dbg_wklock);

	wakeup_source_unregister(g_swing_dbg_priv.swing_dbg_wklock);
	g_swing_dbg_priv.swing_dbg_wklock = NULL;
#endif

	kfifo_free(&g_swing_dbg_priv.read_kfifo);

	/* wait for swing_dbg response. */
	unregister_mcu_event_notifier(TAG_SWING_DBG, CMD_CMN_CONFIG_RESP, swing_dbg_get_resp);

	/* wait for swing response. */
	unregister_mcu_event_notifier(TAG_IGS, CMD_CMN_CONFIG_RESP, swing_dbg_get_resp);

	/* wait for profile response. */
	unregister_mcu_event_notifier(TAG_SWING_DBG, CMD_DATA_PMU_PROFILE, swing_dbg_profile_receive);

	misc_deregister(&swing_dbg_miscdev);
}

/* probe() function for platform driver */
STATIC int swing_dbg_probe(struct platform_device *pdev)
{
	if (pdev == NULL) {
		swing_dbg_log_err("%s: pdev is NULL\n",  __func__);
		return -EFAULT;
	}

	swing_dbg_log_info("%s...\n", __func__);

	return swing_dbg_init(pdev);
}

/* remove() function for platform driver */
STATIC int __exit swing_dbg_remove(struct platform_device *pdev)
{
	swing_dbg_log_info("%s...\n", __func__);

	swing_dbg_exit();

	return 0;
}

#define SWING_DBG_DRV_NAME "hisilicon,swing-dbg"

static const struct of_device_id swing_dbg_match_table[] = {
	{ .compatible = SWING_DBG_DRV_NAME, },
	{},
};

static struct platform_driver swing_dbg_platdev = {
	.driver = {
		.name = "swing_dbg_dev",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(swing_dbg_match_table),
	},
	.probe  = swing_dbg_probe,
	.remove = swing_dbg_remove,
};

STATIC int __init swing_dbg_main_init(void)
{
	return platform_driver_register(&swing_dbg_platdev);
}

STATIC void __exit swing_dbg_main_exit(void)
{
	platform_driver_unregister(&swing_dbg_platdev);
}

late_initcall_sync(swing_dbg_main_init);
module_exit(swing_dbg_main_exit);
