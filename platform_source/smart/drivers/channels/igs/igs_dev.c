/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2017-2021. All rights reserved.
 * Description: dev drvier to communicate with sensorhub igs app
 * Create: 2017-12-05
 */

#include "igs_dev.h"

#include <securec.h>
#include <platform_include/smart/linux/base/ap/protocol.h>

#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/notifier.h>
#include <linux/uaccess.h>
#include <linux/module.h>
#include <linux/err.h>
#include <linux/time.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/kfifo.h>
#include <linux/debugfs.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/syscalls.h>
#include <linux/completion.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/miscdevice.h>
#include <linux/of_address.h>
#include <linux/dma-buf.h>
#include <linux/dma-mapping.h>
#include <linux/platform_drivers/mm_ion.h>
#ifdef CONFIG_CONTEXTHUB_IGS_20
#include <linux/device.h>
#include <linux/pm_wakeup.h>
#include <linux/fb.h>
#endif
#ifdef CONFIG_INPUTHUB_30
#include <platform_include/smart/linux/iomcu_boot.h>
#endif
#include "mm_lb.h"
#include "inputhub_api/inputhub_api.h"
#include "common/common.h"
#include "shmem/shmem.h"
#include <platform_include/basicplatform/linux/dfx_bbox_diaginfo.h>
#include "bbox_diaginfo_id_def.h"
#include "igs_ion.h"


#define IGS_DEV_COMPAT_STR "hisilicon,igs-dev"

#define igs_log_info(msg...) pr_info("[I/IGS]" msg)
#define igs_log_err(msg...) pr_err("[E/IGS]" msg)
#define igs_log_warn(msg...) pr_warn("[W/IGS]" msg)

#ifdef __LLT_UT__
#define STATIC
#else
#define STATIC static
#endif

struct igs_read_data_t {
	unsigned int recv_len;
	void *p_recv;
};

typedef int (*igs_ioctl_ops_f)(struct file *file, unsigned int cmd, unsigned long arg);
typedef int (*igs_resp_ops_f)(struct pkt_subcmd_resp *p_resp);

struct igs_ioctl_ops {
	unsigned int cmd;
	igs_ioctl_ops_f ops;
};

struct igs_resp_ops {
	unsigned int cmd;
	unsigned int resp_len;
	igs_resp_ops_f ops;
};

STATIC int ioctl_fusion_en(struct file *file, unsigned int cmd, unsigned long arg);
STATIC int ioctl_fusion_set(struct file *file, unsigned int cmd, unsigned long arg);
STATIC int ioctl_set_ap_input(struct file *file, unsigned int cmd, unsigned long arg);
#ifdef CONFIG_CONTEXTHUB_IGS_20
STATIC int ioctl_fusion_gets(struct file *file, unsigned int cmd, unsigned long arg);
STATIC int ioctl_fusion_sets(struct file *file, unsigned int cmd, unsigned long arg);
STATIC int ioctl_ion_attach(struct file *file, unsigned int cmd, unsigned long arg);
STATIC int ioctl_ion_detach(struct file *file, unsigned int cmd, unsigned long arg);
#endif

STATIC int resp_fusion_en(struct pkt_subcmd_resp *p_resp);
STATIC int resp_fusion_set(struct pkt_subcmd_resp *p_resp);
STATIC int resp_fusion_upload(struct pkt_subcmd_resp *p_resp);
#ifdef CONFIG_CONTEXTHUB_IGS_20
STATIC int resp_fusion_gets(struct pkt_subcmd_resp *p_resp);
STATIC int resp_fusion_sets(struct pkt_subcmd_resp *p_resp);
STATIC int resp_syscache_disable(struct pkt_subcmd_resp *p_resp);
STATIC int resp_get_wakeup_lock(struct pkt_subcmd_resp *p_resp);
STATIC int resp_release_wakeup_lock(struct pkt_subcmd_resp *p_resp);
STATIC int resp_fusion_sets(struct pkt_subcmd_resp *p_resp);
STATIC int resp_fusion_sets(struct pkt_subcmd_resp *p_resp);
#endif

STATIC struct igs_ioctl_ops g_igs_ioctl_ops[] = {
	{IGS_IOCTL_IGS_OPEN, NULL},
	{IGS_IOCTL_IGS_CLOSE, NULL},
	{IGS_IOCTL_FUSION_EN, ioctl_fusion_en},
	{IGS_IOCTL_FUSION_SET, ioctl_fusion_set},
	{IGS_IOCTL_SET_AP_INPUT, ioctl_set_ap_input},
#ifdef CONFIG_CONTEXTHUB_IGS_20
	{IGS_IOCTL_FUSION_SETS, ioctl_fusion_sets},
	{IGS_IOCTL_FUSION_GETS, ioctl_fusion_gets},
	{IGS_IOCTL_ION_REF_ATTACH, ioctl_ion_attach},
	{IGS_IOCTL_ION_REF_DETACH, ioctl_ion_detach},
#endif
};

STATIC struct igs_resp_ops g_igs_resp_ops[] = {
	{SUB_CMD_IGS_FUSION_EN, sizeof(struct igs_fusion_en_resp_t), resp_fusion_en},
	{SUB_CMD_IGS_FUSION_SET, sizeof(struct igs_fusion_set_resp_t), resp_fusion_set},
	{SUB_CMD_IGS_FUSION_UPLOAD, 0, resp_fusion_upload},
#ifdef CONFIG_CONTEXTHUB_IGS_20
	{SUB_CMD_IGS_FUSION_SETS, sizeof(struct igs_fusion_sets_resp_t), resp_fusion_sets},
	{SUB_CMD_IGS_FUSION_GETS, 0, resp_fusion_gets},
	{SUB_CMD_SWING_SYSCACHE_ENABLE, 0, NULL},
	{SUB_CMD_SWING_SYSCACHE_DISABLE, 0, resp_syscache_disable},
	{SUB_CMD_SWING_SCREEN_ON, 0, NULL},
	{SUB_CMD_SWING_SCREEN_OFF, 0, resp_release_wakeup_lock},
	{SUB_CMD_SWING_GET_WAKEUP_LOCK, 0, resp_get_wakeup_lock},
	{SUB_CMD_SWING_RELEASE_WAKEUP_LOCK, 0, resp_release_wakeup_lock},
#endif
};

struct igs_priv_t {
	struct device *self;            /* self device. */
	struct completion igs_wait;
	struct completion read_wait;
	struct mutex read_mutex;        /* Used to protect ops on read */
	struct mutex igs_mutex;         /* Used to protect ops on ref_cnt/sh_recover_flg */
	struct mutex ioctl_mutex;       /* Used to protect ops on ioctl */
	struct mutex sr_mutex;          /* Used to protect sr */
	struct kfifo read_kfifo;
	struct igs_fusion_en_resp_t en_resp;
	struct igs_fusion_set_resp_t set_resp;
	int ref_cnt;
	int sh_recover_flag;
#ifdef CONFIG_CONTEXTHUB_IGS_20
	struct igs_fusion_gets_resp_t gets_resp;
	struct igs_fusion_sets_resp_t sets_resp;
	struct wakeup_source *igs_wklock;
	struct mutex wklock_mutex;       /* Used to protect igs_wklock op */
	struct list_head ion_buf_list;
#endif
	int send_ipc_to_shb;
};

#ifdef CONFIG_CONTEXTHUB_IGS_20
struct igs_ion_node {
	struct list_head list;
	struct igs_ion_info_t ion_info;
};
#endif

struct igs_dmd_log_t {
	unsigned int dmd_id;
	unsigned int dmd_para_num;
	const char *dmd_msg;
};

#define IGS_READ_CACHE_COUNT          5
#define IGS_SYSCAHCE_ID               8
#define IGS_IOCTL_WAIT_TMOUT          5000 /* ms */
#define IGS_RESET_NOTIFY              0xFFFF
#define IGS_DMD_CASE_DETAIL           0xFF
#define IGS_DMD_INFO_NUM_MAX          5
#define IGS_DEV_MAX_UPLOAD_LEN        0x1000
#define IGS_DEV_MAX_DATA_LEN          16000
#define IGS_DMA_MASK                  0xFFFFFFFFFFFFFFFF
#define IGS_SEND_SUSPEND_TO_SHB       1
#define IGS_SEND_RESUME_TO_SHB        2
#define IGS_SCREEN_OFF                0x5A
#define IGS_SCREEN_ON                 0x0

static unsigned long long g_igs_dmamask = IGS_DMA_MASK;

static struct igs_dmd_log_t dmd_log[] = {
	{IGS_DMD_FDUL_PW_ON,          3, "FDUL PowerOn Failed: step %u, register 0x %x, value 0x %x."},
	{IGS_DMD_FDUL_PW_OFF,         3, "FDUL PowerOff Failed: step %u, register 0x %x, value 0x %x."},
	{IGS_DMD_HWTS_PW_ON,          4, "HWTS PowerOn Failed: step %u, register 0x %x, value 0x %x %x."},
	{IGS_DMD_HWTS_PW_OFF,         4, "HWTS PowerOff Failed: step %u, register 0x %x, value 0x %x %x."},
	{IGS_DMD_AIC_PW_ON,           3, "TinyCore PowerOn Failed: step %u, register 0x %x, value 0x %x."},
	{IGS_DMD_AIC_PW_OFF,          3, "TinyCore PowerOff Failed: step %u, register 0x %x, value 0x %x."},
	{IGS_DMD_CAM_PW_ON,           2, "Igs cam pw on timeout: cam_state: %d, ao_cam_status: %d."},
	{IGS_DMD_CAM_PW_OFF,          2, "Igs cam pw off timeout: cam_state: %d, ao_cam_status: %d."},
	{IGS_DMD_CAM_IR_PW,           3, "Igs IR pw fail: onoff: %d, ir_status: %d, pw_err_type: %d."},
	{IGS_DMD_CAM_TIMEOUT,         0, "Camera timeout."},
	{IGS_DMD_SLEEP_FUSION,        3, "Fusions open when suspend: num %u, bitmap 0x %x %x."},
	{IGS_DMD_NPU_PLL_RETRY,       3, "NPU_PLL_RETRY: PLL_ID %d Total %d Retry %d."},
};

static struct igs_priv_t g_igs_priv = { 0 };

static unsigned int *g_igs_avs_data;
static unsigned int *g_igs_pa_margin;
static unsigned int *g_igs_svfd_data;
static unsigned int *g_igs_svfd_para;
static unsigned int *g_prof_margin;
static unsigned int *g_level2_chip;

static int g_avs_data_num;
static int g_pa_margin_num;
static int g_svfd_data_num;
static int g_svfd_para_num;
static int g_prof_num;

STATIC bool get_npu_data_ptr(void)
{
#ifdef CONFIG_INPUTHUB_30
	struct plat_cfg_str *smplat_scfg = get_plat_cfg_str();

	if (smplat_scfg != NULL) {
		g_igs_avs_data = smplat_scfg->npu_data.avs_data;
		g_igs_pa_margin = smplat_scfg->npu_data.pa_margin;
		g_igs_svfd_data = smplat_scfg->npu_data.svfd_data;
		g_igs_svfd_para = smplat_scfg->npu_data.svfd_para;
		g_prof_margin = smplat_scfg->npu_data.prof_margin;
		g_level2_chip = &(smplat_scfg->npu_data.level2_chip);
		g_avs_data_num = NPU_AVS_DATA_NUN;
		g_pa_margin_num = PA_MARGIN_NUM;
		g_svfd_data_num = NPU_SVFD_DATA_NUN;
		g_svfd_para_num = NPU_SVFD_PARA_NUN;
		g_prof_num = PROFILE_NUM;
		return true;
	}
	igs_log_err("get npu data ptr: g_smplat_scfg NULL\n");
	return false;
#else
	return false;
#endif
}

STATIC inline unsigned int igs_dev_get_ioctl_ops_num(void)
{
	return (sizeof(g_igs_ioctl_ops) / sizeof(struct igs_ioctl_ops));
}

STATIC inline unsigned int igs_dev_get_resp_ops_num(void)
{
	return (sizeof(g_igs_resp_ops) / sizeof(struct igs_resp_ops));
}

STATIC void igs_dev_wait_init(struct completion *p_wait)
{
	if (p_wait == NULL) {
		igs_log_err("igs dev wait init: wait NULL\n");
		return;
	}

	init_completion(p_wait);
}

STATIC int igs_dev_wait_completion(struct completion *p_wait, unsigned int tm_out)
{
	if (p_wait == NULL) {
		igs_log_err("igs dev wait completion: wait NULL\n");
		return -EFAULT;
	}

	igs_log_info("igs dev wait completion: waitting\n");
	if (tm_out != 0) {
		if (!wait_for_completion_interruptible_timeout(
		    p_wait, msecs_to_jiffies(tm_out))) {
			igs_log_warn("igs dev wait completion: wait timeout\n");
			return -ETIMEOUT;
		}
	} else {
		if (wait_for_completion_interruptible(p_wait)) {
			igs_log_warn("igs dev wait completion: wait interrupted.\n");
			return -EFAULT;
		}
	}

	return 0;
}

STATIC void igs_dev_complete(struct completion *p_wait)
{
	if (p_wait == NULL) {
		igs_log_err("igs dev complete: wait NULL\n");
		return;
	}

	complete(p_wait);
}

STATIC int ioctl_fusion_en(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct igs_fusion_en_param_t fe = { {0} };
	int ret;

	if (arg == 0) {
		igs_log_err("[%s] arg NULL.\n", __func__);
		return -EFAULT;
	}

	if (copy_from_user((void *)&fe, (void *)((uintptr_t)arg), sizeof(struct igs_fusion_en_param_t))) {
		igs_log_err("[%s]copy_from_user error\n", __func__);
		return -EFAULT;
	}

	if (send_cmd_from_kernel(TAG_IGS, CMD_CMN_CONFIG_REQ, SUB_CMD_IGS_FUSION_EN,
				 (char *)(&fe.en), sizeof(struct igs_fusion_en_t))) {
		igs_log_err("[%s]send cmd error\n", __func__);
		return -EFAULT;
	}

	ret = igs_dev_wait_completion(&g_igs_priv.igs_wait, IGS_IOCTL_WAIT_TMOUT);
	if (ret)
		return ret;

	ret = memcpy_s((void *)(&fe.en_resp), sizeof(struct igs_fusion_en_resp_t),
		       (void *)(&g_igs_priv.en_resp), sizeof(struct igs_fusion_en_resp_t));
	if (ret != EOK)
		pr_err("%s memcpy buffer fail, ret[%d]\n", __func__, ret);

	if (copy_to_user((char *)((uintptr_t)arg), &fe, sizeof(struct igs_fusion_en_param_t))) {
		igs_log_err("%s failed to copy to user\n", __func__);
		return -EFAULT;
	}
	return 0;
}

STATIC int ioctl_fusion_set(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct igs_fusion_set_param_t fs = { {0} };
	int ret;

	if (arg == 0) {
		igs_log_err("[%s] arg NULL.\n", __func__);
		return -EFAULT;
	}

	if (copy_from_user((void *)&fs, (void *)((uintptr_t)arg), sizeof(struct igs_fusion_set_param_t))) {
		igs_log_err("[%s]copy_from_user error\n", __func__);
		return -EFAULT;
	}

	if (send_cmd_from_kernel(TAG_IGS, CMD_CMN_CONFIG_REQ, SUB_CMD_IGS_FUSION_SET,
				 (char *)(&fs.set), sizeof(struct igs_fusion_set_t))) {
		igs_log_err("[%s]send cmd error\n", __func__);
		return -EFAULT;
	}

	ret = igs_dev_wait_completion(&g_igs_priv.igs_wait, IGS_IOCTL_WAIT_TMOUT);
	if (ret)
		return ret;

	ret = memcpy_s((void *)(&fs.set_resp), sizeof(struct igs_fusion_set_resp_t),
		       (void *)(&g_igs_priv.set_resp), sizeof(struct igs_fusion_set_resp_t));
	if (ret != EOK)
		pr_err("%s memcpy buffer fail, ret[%d]\n", __func__, ret);

	if (copy_to_user((void *)((uintptr_t)arg), &fs, sizeof(struct igs_fusion_set_param_t))) {
		igs_log_err("%s failed to copy to user\n", __func__);
		return -EFAULT;
	}

	return 0;
}

STATIC int ioctl_set_ap_input(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct igs_fusion_set_param_t fs = { {0} };
	struct igs_set_ap_input_param_t ap_input = { {0} };
	dma_addr_t phys_addr;
	int ret;

	if (arg == 0) {
		igs_log_err("[%s] arg NULL.\n", __func__);
		return -EFAULT;
	}

	if (copy_from_user((void *)&ap_input, (void *)((uintptr_t)arg), sizeof(ap_input))) {
		igs_log_err("[%s]copy_from_user error\n", __func__);
		return -EFAULT;
	}

	if (igs_get_ion_phys(ap_input.input.fd, (dma_addr_t *)(&phys_addr), g_igs_priv.self) < 0) {
		igs_log_err("igs_ioctl_get_phys: get_ion_phys failed\n");
		return -EFAULT;
	}

	fs.set.fusion_id = ap_input.input.fusion_id;
	fs.set.data_type = ap_input.input.data_type;
	fs.set.data_addr = (u32)phys_addr;
	fs.set.data_len = ap_input.input.data_len;

	if (send_cmd_from_kernel(TAG_IGS, CMD_CMN_CONFIG_REQ, SUB_CMD_IGS_FUSION_SET,
				 (char *)(&fs.set), sizeof(fs.set))) {
		igs_log_err("[%s]send cmd error\n", __func__);
		return -EFAULT;
	}

	ret = igs_dev_wait_completion(&g_igs_priv.igs_wait, IGS_IOCTL_WAIT_TMOUT);
	if (ret) {
		igs_log_err("[%s]wait timeout\n", __func__);
		return ret;
	}

	ret = memcpy_s((void *)(&fs.set_resp), sizeof(fs.set_resp),
		       (void *)(&g_igs_priv.set_resp), sizeof(g_igs_priv.set_resp));
	if (ret != EOK) {
		pr_err("%s memcpy resp buffer fail, ret[%d]\n", __func__, ret);
		return -EFAULT;
	}

	ap_input.input_resp.ret_code = fs.set_resp.ret_code;
	ap_input.input_resp.fusion_id = fs.set_resp.fusion_id;
	if (copy_to_user((void *)((uintptr_t)arg), &ap_input, sizeof(ap_input))) {
		igs_log_err("%s failed to copy to user\n", __func__);
		return -EFAULT;
	}

	return 0;
}

#ifdef CONFIG_CONTEXTHUB_IGS_20
STATIC int ioctl_fusion_gets(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct igs_fusion_gets_param_t fs_gets = { {0} };
	int ret;

	if (arg == 0) {
		igs_log_err("[%s] arg NULL.\n", __func__);
		return -EFAULT;
	}

	if (copy_from_user((void *)&fs_gets, (void *)((uintptr_t)arg), sizeof(struct igs_fusion_gets_param_t))) {
		igs_log_err("[%s]copy_from_user error\n", __func__);
		return -EFAULT;
	}

	if (send_cmd_from_kernel(TAG_IGS, CMD_CMN_CONFIG_REQ, SUB_CMD_IGS_FUSION_GETS,
				 (char *)(&fs_gets.gets_param), sizeof(struct igs_fusion_gets_t))) {
		igs_log_err("[%s]send cmd error\n", __func__);
		return -EFAULT;
	}

	ret = igs_dev_wait_completion(&g_igs_priv.igs_wait, IGS_IOCTL_WAIT_TMOUT);
	if (ret != 0)
		goto do_ret;

	fs_gets.gets_resp.ret_code = g_igs_priv.gets_resp.ret_code;
	fs_gets.gets_resp.fusion_id = g_igs_priv.gets_resp.fusion_id;
	fs_gets.gets_resp.data_type = g_igs_priv.gets_resp.data_type;
	fs_gets.gets_resp.data_len = g_igs_priv.gets_resp.data_len;
	fs_gets.gets_resp.data_addr = fs_gets.gets_param.data_addr;

	if (fs_gets.gets_resp.data_addr == 0 || g_igs_priv.gets_resp.data_addr == 0) {
		igs_log_err("%s: addr NULL\n", __func__);
		ret = -EFAULT;
		goto do_ret;
	}
	/* copy get data to get buffer. */
	if (copy_to_user((void *)((uintptr_t)fs_gets.gets_resp.data_addr),
			(void *)((uintptr_t)g_igs_priv.gets_resp.data_addr), g_igs_priv.gets_resp.data_len)) {
		igs_log_err("%s failed to copy to user\n", __func__);
		ret = -EFAULT;
		goto do_ret;
	}

	if (copy_to_user((void *)((uintptr_t)arg), &fs_gets, sizeof(struct igs_fusion_gets_param_t))) {
		igs_log_err("%s failed to copy to user\n", __func__);
		ret = -EFAULT;
		goto do_ret;
	}

do_ret:
	/* try to free buffer, alloc in get_resp. */
	if ((void *)((uintptr_t)g_igs_priv.gets_resp.data_addr) != NULL) {
		kfree((void *)((uintptr_t)g_igs_priv.gets_resp.data_addr));
		g_igs_priv.gets_resp.data_addr = 0;
	}
	return ret;
}

STATIC int ioctl_fusion_sets(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct igs_fusion_sets_param_t fss = { {0} };
	struct igs_fusion_sets_t *p_fss = NULL;
	void *packet_buf = NULL;
	struct write_info wr;
	int ret;

	if (arg == 0) {
		igs_log_err("[%s] arg NULL.\n", __func__);
		return -EFAULT;
	}

	if (copy_from_user((void *)&fss, (void *)((uintptr_t)arg), sizeof(struct igs_fusion_sets_param_t))) {
		igs_log_err("[%s]copy_from_user error\n", __func__);
		return -EFAULT;
	}

	if (fss.sets.data_len == 0 || fss.sets.data_addr == 0 || fss.sets.data_len >= IGS_DEV_MAX_DATA_LEN) {
		igs_log_err("[%s]para error\n", __func__);
		return -EFAULT;
	}

	packet_buf = kzalloc(sizeof(struct igs_fusion_sets_t) + fss.sets.data_len + sizeof(u32), GFP_ATOMIC);
	if (packet_buf == NULL) {
		igs_log_err("[%s]all fail\n", __func__);
		return -EFAULT;
	}

	((u32 *)packet_buf)[0] = SUB_CMD_IGS_FUSION_SETS;
	p_fss = (struct igs_fusion_sets_t *)((char *)packet_buf + sizeof(u32));
	ret = memcpy_s((void *)p_fss, sizeof(struct igs_fusion_sets_t) + fss.sets.data_len,
		       (void *)(&fss), sizeof(struct igs_fusion_sets_t));
	if (ret != EOK) {
		igs_log_err("%s memcpy buffer fail, ret[%d]\n", __func__, ret);
		goto ERR;
	}

	igs_log_info("[%s] copy data ++\n", __func__);
	if (copy_from_user((void *)(p_fss + 1), (void *)(uintptr_t)(p_fss->data_addr), fss.sets.data_len)) {
		igs_log_err("[%s]copy_from_user error\n", __func__);
		ret = -EFAULT;
		goto ERR;
	}
	igs_log_info("[%s] copy data --\n", __func__);

	wr.tag = TAG_IGS;
	wr.cmd = CMD_CMN_CONFIG_REQ;
	wr.wr_buf = packet_buf;
	wr.wr_len = (int)(sizeof(struct igs_fusion_sets_t) + fss.sets.data_len + sizeof(u32));
#ifdef CONFIG_INPUTHUB_30
	if (write_customize_cmd(&wr, NULL, false) != 0) {
		igs_log_err("[%s]send cmd error\n", __func__);
		ret = -EFAULT;
		goto ERR;
	}
#else
	if (shmem_send(wr.tag, wr.wr_buf, wr.wr_len) != 0) {
		igs_log_err("[%s]send cmd error\n", __func__);
		ret = -EFAULT;
		goto ERR;
	}
#endif
	ret = igs_dev_wait_completion(&g_igs_priv.igs_wait, IGS_IOCTL_WAIT_TMOUT);
	if (ret)
		goto ERR;
	ret = memcpy_s((void *)(&fss.sets_resp), sizeof(struct igs_fusion_sets_resp_t),
		       (void *)(&g_igs_priv.sets_resp), sizeof(struct igs_fusion_sets_resp_t));
	if (ret != EOK) {
		igs_log_err("%s memcpy buffer fail, ret[%d]\n", __func__, ret);
		goto ERR;
	}

	if (copy_to_user((void *)((uintptr_t)arg), &fss, sizeof(struct igs_fusion_sets_param_t))) {
		igs_log_err("%s failed to copy to user\n", __func__);
		ret = -EFAULT;
		goto ERR;
	}
	kfree(packet_buf);
	return 0;
ERR:
	if (packet_buf != NULL)
		kfree(packet_buf);
	return ret;
}

STATIC int ioctl_ion_attach(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct igs_ion_info_t ion_info;
	struct igs_ion_node *node = NULL;
	struct sg_table *table = NULL;
	enum SEC_SVC type = 0;
	struct dma_buf *buf = NULL;
	int ret;

	if (arg == 0) {
		igs_log_err("[%s]: arg NULL...\n", __func__);
		return -EFAULT;
	}

	if (copy_from_user((void *)&ion_info, (void *)((uintptr_t)arg), sizeof(struct igs_ion_info_t))) {
		igs_log_err("[%s]: copy_from_user failed\n", __func__);
		return -EFAULT;
	}

	if (ion_info.fd < 0) {
		igs_log_err("[%s] : invalid fd...\n", __func__);
		return -EFAULT;
	}

	buf = dma_buf_get(ion_info.fd);
	if (IS_ERR_OR_NULL(buf)) {
		igs_log_err("[%s] Failed : dma_buf_get\n", __func__);
		return -EFAULT;
	}

	ret = secmem_get_buffer(ion_info.fd, &table, &ion_info.id, &type);
	if (ret != 0) {
		igs_log_err("[%s] : secmem_get_buffer failed.\n", __func__);
		dma_buf_put(buf);
		return -EFAULT;
	}

	if (copy_to_user((void *)((uintptr_t)arg), &ion_info, sizeof(struct igs_ion_info_t))) {
		igs_log_err("%s failed to copy to user\n", __func__);
		dma_buf_put(buf);
		return -EFAULT;
	}

	node = kzalloc(sizeof(struct igs_ion_node), GFP_ATOMIC);
	if (node == NULL) {
		igs_log_err("[%s] : Failed to alloc memory for ion node...\n", __func__);
		dma_buf_put(buf);
		return -EFAULT;
	}

	node->ion_info.fd = ion_info.fd;
	node->ion_info.id = ion_info.id;
	node->ion_info.buf = buf;
	list_add(&node->list, &g_igs_priv.ion_buf_list);
	igs_log_info("[%s]....done, %x, %llx\n", __func__, ion_info.fd, ion_info.id);

	return 0;
}

STATIC int ioctl_ion_detach(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct igs_ion_info_t ion_info;
	struct igs_ion_node *cur = NULL;
	struct igs_ion_node *next = NULL;

	if (arg == 0) {
		igs_log_err("[%s]: arg NULL...\n", __func__);
		return -EFAULT;
	}

	if (copy_from_user((void *)&ion_info, (void *)((uintptr_t)arg), sizeof(struct igs_ion_info_t))) {
		igs_log_err("[%s]: copy_from_user failed\n", __func__);
		return -EFAULT;
	}

	list_for_each_entry_safe(cur, next, &g_igs_priv.ion_buf_list, list) {
		if (cur->ion_info.fd == ion_info.fd) {
			igs_log_info("[%s]....done, %x\n", __func__, cur->ion_info.fd);

			dma_buf_put(cur->ion_info.buf);

			list_del(&(cur->list));
			kfree(cur);
			return 0;
		}
	}

	igs_log_info("[%s]....done\n", __func__);

	return 0;
}
#endif

STATIC long igs_dev_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int ret = -ENOTTY;
	unsigned int i;
	unsigned int cmd_num = igs_dev_get_ioctl_ops_num();

	igs_log_info("%s cmd: [0x%x]\n", __func__, cmd);

	mutex_lock(&g_igs_priv.ioctl_mutex);

	if (g_igs_priv.sh_recover_flag == 1) {
		mutex_unlock(&g_igs_priv.ioctl_mutex);
		igs_log_info("%s sensorhub in recover mode\n", __func__);
		return -EBUSY;
	}

	reinit_completion(&g_igs_priv.igs_wait);

	igs_log_info("%s reinit completion\n", __func__);

	for (i = 0; i < cmd_num; i++) {
		if (cmd == g_igs_ioctl_ops[i].cmd)
			break;
	}

	if (i == cmd_num) {
		mutex_unlock(&g_igs_priv.ioctl_mutex);
		igs_log_err("%s unknown cmd : %d\n", __func__, cmd);
		return -ENOTTY;
	}

	if (g_igs_ioctl_ops[i].ops != NULL)
		ret = g_igs_ioctl_ops[i].ops(file, cmd, arg);

	mutex_unlock(&g_igs_priv.ioctl_mutex);

	if (ret != 0)
		igs_log_err("%s err %x\n", __func__, ret);

	return ret;
}

STATIC int resp_fusion_upload(struct pkt_subcmd_resp *p_resp)
{
	struct igs_upload_t *p_upload = NULL;
	struct igs_read_data_t read_data = {0};
	int ret = 0;

	mutex_lock(&g_igs_priv.read_mutex);

	if (kfifo_avail(&g_igs_priv.read_kfifo) < sizeof(struct igs_read_data_t)) {
		igs_log_err("%s read_kfifo is full, drop upload data.\n", __func__);
		ret = -EFAULT;
		goto ret_err;
	}

	p_upload = (struct igs_upload_t *)(p_resp + 1);

	if (p_upload->notify_len > IGS_DEV_MAX_UPLOAD_LEN) {
		igs_log_err("%s upload length err 0x%x.\n", __func__, p_upload->notify_len);
		ret = -EFAULT;
		goto ret_err;
	}

	read_data.recv_len = sizeof(struct igs_upload_t) + p_upload->notify_len;
	read_data.p_recv = kzalloc(read_data.recv_len, GFP_ATOMIC);
	if (read_data.p_recv == NULL) {
		igs_log_err("Failed to alloc memory to save upload resp...\n");
		ret = -EFAULT;
		goto ret_err;
	}

	ret = memcpy_s(read_data.p_recv, read_data.recv_len, p_upload, read_data.recv_len);
	if (ret != 0) {
		igs_log_err("%s memcpy_s failed...\n", __func__);
		ret = -EFAULT;
		goto ret_err;
	}

	if (kfifo_in(&g_igs_priv.read_kfifo, (unsigned char *)&read_data, sizeof(struct igs_read_data_t)) == 0) {
		igs_log_err("%s: kfifo_in failed\n", __func__);
		ret = -EFAULT;
		goto ret_err;
	}

	mutex_unlock(&g_igs_priv.read_mutex);

	igs_log_info("Fusion Upload Received, [0x%x].\n", p_upload->fusion_id);
	igs_dev_complete(&g_igs_priv.read_wait);

	return 0;
ret_err:
	if (read_data.p_recv != NULL)
		kfree(read_data.p_recv);

	mutex_unlock(&g_igs_priv.read_mutex);

	return ret;
}

STATIC int resp_fusion_en(struct pkt_subcmd_resp *p_resp)
{
	struct igs_fusion_en_resp_t *p_en_resp = NULL;
	int ret;

	p_en_resp = (struct igs_fusion_en_resp_t *)(p_resp + 1);
	ret = memcpy_s(&g_igs_priv.en_resp, sizeof(struct igs_fusion_en_resp_t), p_en_resp,
		       sizeof(struct igs_fusion_en_resp_t));
	if (ret != EOK)
		igs_log_err("%s memcpy buffer fail, ret[%d]\n", __func__, ret);

	igs_log_info("Fusion [0x%x] En Resp ret[0x%x] .\n",
		       p_en_resp->fusion_id, p_en_resp->ret_code);
	igs_dev_complete(&g_igs_priv.igs_wait);

	return ret;
}

STATIC int resp_fusion_set(struct pkt_subcmd_resp *p_resp)
{
	struct igs_fusion_set_resp_t *p_set_resp = NULL;
	int ret;

	p_set_resp = (struct igs_fusion_set_resp_t *)(p_resp + 1);
	ret = memcpy_s(&g_igs_priv.set_resp, sizeof(struct igs_fusion_set_resp_t),
		       p_set_resp, sizeof(struct igs_fusion_set_resp_t));
	if (ret != EOK)
		igs_log_err("%s memcpy buffer fail, ret[%d]\n", __func__, ret);

	igs_log_info("Fusion [0x%x] Set Resp ret[0x%x] .\n",
		       p_set_resp->fusion_id, p_set_resp->ret_code);
	igs_dev_complete(&g_igs_priv.igs_wait);

	return ret;
}


#ifdef CONFIG_CONTEXTHUB_IGS_20
STATIC int resp_fusion_gets(struct pkt_subcmd_resp *p_resp)
{
	struct igs_fusion_gets_resp_t *p_gets_resp = NULL;
	int ret = 0;

	p_gets_resp = (struct igs_fusion_gets_resp_t *)(p_resp + 1);
	ret = memcpy_s(&g_igs_priv.gets_resp, sizeof(struct igs_fusion_gets_resp_t),
		       p_gets_resp, sizeof(struct igs_fusion_gets_resp_t));
	if (ret != EOK) {
		igs_log_err("%s memcpy buffer fail, ret[%d]\n", __func__, ret);
		goto ret_err;
	}

	if (g_igs_priv.gets_resp.data_len != 0 && g_igs_priv.gets_resp.data_len < IGS_DEV_MAX_DATA_LEN) {
		g_igs_priv.gets_resp.data_addr = (u64)((uintptr_t)kzalloc(g_igs_priv.gets_resp.data_len,
									    GFP_ATOMIC));
		if ((void *)((uintptr_t)g_igs_priv.gets_resp.data_addr) == NULL) {
			igs_log_err("[%s]kzalloc error\n", __func__);
			ret = -EFAULT;
			goto ret_err;
		}
		ret = memcpy_s((void *)((uintptr_t)g_igs_priv.gets_resp.data_addr), g_igs_priv.gets_resp.data_len,
			       (void *)((u8 *)p_gets_resp + sizeof(struct igs_fusion_gets_resp_t)),
			       g_igs_priv.gets_resp.data_len);
		if (ret != EOK) {
			igs_log_err("%s memcpy buffer fail, ret[%d]\n", __func__, ret);
			goto ret_err;
		}
	}
	igs_log_info("Fusion [0x%x] Gets Resp ret[0x%x], type[%d].\n",
		       p_gets_resp->fusion_id, p_gets_resp->ret_code, p_gets_resp->data_type);

ret_err:
	igs_dev_complete(&g_igs_priv.igs_wait);
	return ret;
}

STATIC int resp_fusion_sets(struct pkt_subcmd_resp *p_resp)
{
	struct igs_fusion_sets_resp_t *p_sets_resp = NULL;
	int ret = 0;

	p_sets_resp = (struct igs_fusion_sets_resp_t *)(p_resp + 1);
	ret = memcpy_s(&g_igs_priv.sets_resp, sizeof(struct igs_fusion_sets_resp_t),
		       p_sets_resp, sizeof(struct igs_fusion_sets_resp_t));
	if (ret != EOK)
		igs_log_err("%s memcpy buffer fail, ret[%d]\n", __func__, ret);

	igs_log_info("Fusion [0x%x] Sets Resp ret[0x%x], type[%d].\n",
		       p_sets_resp->fusion_id, p_sets_resp->ret_code, p_sets_resp->data_type);
	igs_dev_complete(&g_igs_priv.igs_wait);

	return ret;
}

STATIC int resp_syscache_disable(struct pkt_subcmd_resp *p_resp)
{
	int ret = 0;

	igs_log_info("%s: receive shb resume rsp msg, sc_flag = %u.\n", __func__, g_config_on_ddr->sc_flag);
	mutex_lock(&g_igs_priv.sr_mutex);
	if (g_igs_priv.send_ipc_to_shb == IGS_SEND_RESUME_TO_SHB) {
		if (g_config_on_ddr->sc_flag == 1) {
			igs_log_info("release quota.\n");
#ifdef CONFIG_MM_LB
			ret = lb_release_quota(IGS_SYSCAHCE_ID);
			if (ret != 0)
				igs_log_err("%s lb_release_quota fail, ret[%d]\n", __func__, ret);
#endif
		}

		g_igs_priv.send_ipc_to_shb = 0;
	}
	mutex_unlock(&g_igs_priv.sr_mutex);

	return ret;
}

STATIC int resp_get_wakeup_lock(struct pkt_subcmd_resp *p_resp)
{
	// release wake-lock when got screen-off ack.
	mutex_lock(&g_igs_priv.wklock_mutex);
	igs_log_info("%s: get wake msg.\n", __func__);
	if (!g_igs_priv.igs_wklock->active) {
		igs_log_info("%s: wake unlock.\n", __func__);
		__pm_wakeup_event(g_igs_priv.igs_wklock, IGS_IOCTL_WAIT_TMOUT * 2);
	}
	mutex_unlock(&g_igs_priv.wklock_mutex);

	return 0;
}

STATIC int resp_release_wakeup_lock(struct pkt_subcmd_resp *p_resp)
{
	mutex_lock(&g_igs_priv.wklock_mutex);
	// release wake-lock when got screen-off ack.
	igs_log_info("%s: receive msg [%x].\n", __func__, p_resp->subcmd);
	if (g_igs_priv.igs_wklock->active) {
		igs_log_info("%s: wake unlock.\n", __func__);
		__pm_relax(g_igs_priv.igs_wklock);
	}
	mutex_unlock(&g_igs_priv.wklock_mutex);

	return 0;
}
#endif

STATIC int igs_get_resp(const struct pkt_header *head)
{
	struct pkt_subcmd_resp *p_resp = NULL;
	int ret = 0;
	unsigned int i;
	unsigned int cmd_num = igs_dev_get_resp_ops_num();

	p_resp = (struct pkt_subcmd_resp *)(head);

	if (p_resp == NULL) {
		igs_log_err("%s: p_resp is null\n", __func__);
		return -EFAULT;
	}

	if (p_resp->hd.tag != TAG_IGS) {
		igs_log_err("%s: invalid tag [0x%x]\n", __func__, p_resp->hd.tag);
		return -EFAULT;
	}

	igs_log_info("%s: cmd[%d], length[%d], tag[%d], sub_cmd[%d]\n",
		       __func__, p_resp->hd.cmd, p_resp->hd.length, p_resp->hd.tag, p_resp->subcmd);

	for (i = 0; i < cmd_num; i++) {
		if (p_resp->subcmd == g_igs_resp_ops[i].cmd)
			break;
	}

	if (i == cmd_num) {
		igs_log_warn("unhandled cmd: tag[%d], sub_cmd[%d]\n", p_resp->hd.tag, p_resp->subcmd);
		return -EFAULT;
	}

	if (g_igs_resp_ops[i].resp_len != 0) {
		if ((g_igs_resp_ops[i].resp_len + 8) != head->length) {
			igs_log_err("%s: invalid payload length: tag[%d], sub_cmd[%d], length[0x%x]\n",
				      __func__, p_resp->hd.tag, p_resp->subcmd, head->length);
			return -EFAULT;
		}
	}

	if (g_igs_resp_ops[i].ops != NULL)
		ret = g_igs_resp_ops[i].ops(p_resp);

	return ret;
}

STATIC u32 igs_get_dmd_log_index(u32 dmd_id)
{
	u32 index;
	size_t log_arry_size = ARRAY_SIZE(dmd_log);

	for (index = 0; index < log_arry_size; index++) {
		if (dmd_log[index].dmd_id == dmd_id)
			break;
	}
	return index;
}

STATIC int igs_get_shb_dmd_report(const struct pkt_header *head)
{
	int ret;
	u32 index;
	size_t hd_size = sizeof(struct pkt_header);
	size_t req_size = sizeof(pkt_dmd_log_report_req_t);
	size_t log_arry_size = ARRAY_SIZE(dmd_log);

	pkt_dmd_log_report_req_t *req = (pkt_dmd_log_report_req_t *)(head);

	if (req == NULL) {
		igs_log_err("igs get shb dmd report: req is null!!\n");
		return -EFAULT;
	}

	if (req->hd.tag != TAG_IGS) {
		igs_log_err("igs get shb dmd report: tag[0x%x] not match TAG_IGS!!\n", req->hd.tag);
		return -EFAULT;
	}

	if (req->hd.cmd != CMD_LOG_REPORT_REQ) {
		igs_log_err("igs get shb dmd report: cmd[0x%x] not match CMD_LOG_REPORT_REQ!!\n", req->hd.cmd);
		return -EFAULT;
	}

	if (hd_size + req->hd.length != req_size) {
		igs_log_err("igs get shb dmd report: req length check failed!! hd_size[%u], payload size[%u], total[%u]!!\n",
			      (u32)hd_size, req->hd.length, (u32)req_size);
		return -EFAULT;
	}
	igs_log_info("igs get shb dmd report: dmd_id[%u], dmd_case[0x%x], length[%u], tranid[%u], para_num [%u]\n",
		       req->dmd_id, req->dmd_case, req->hd.length, req->hd.tranid, req->resv1);

	if (req->dmd_case != IGS_DMD_CASE_DETAIL) {
		igs_log_err("igs get shb dmd report: dmd_id[%u] err!! dmd_case [0x%x]\n", req->dmd_id,
			      req->dmd_case);
		return -EFAULT;
	}

	index = igs_get_dmd_log_index(req->dmd_id);
	if (index >= log_arry_size) {
		igs_log_err("igs get shb dmd report: dmd_id[%u] not found!!\n", req->dmd_id);
		return -EFAULT;
	}

	if (req->resv1 != dmd_log[index].dmd_para_num) {
		igs_log_err("igs get shb dmd report: resv1[%u] not equal to dmd_para_num[%u]!! req->dmd_id[%u].\n",
			      req->resv1, dmd_log[index].dmd_para_num, req->dmd_id);
		return -EFAULT;
	}

	/* req->resv1: info[] arry size */
	switch (req->resv1) {
	case 0:
		ret = bbox_diaginfo_record(req->dmd_id, NULL, dmd_log[index].dmd_msg);
		break;

	case 1:
		ret = bbox_diaginfo_record(req->dmd_id, NULL, dmd_log[index].dmd_msg, req->info[0]);
		break;

	case 2:
		ret = bbox_diaginfo_record(req->dmd_id, NULL, dmd_log[index].dmd_msg, req->info[0], req->info[1]);
		break;

	case 3:
		ret = bbox_diaginfo_record(req->dmd_id, NULL, dmd_log[index].dmd_msg, req->info[0], req->info[1],
					   req->info[2]);
		break;

	case 4:
		ret = bbox_diaginfo_record(req->dmd_id, NULL, dmd_log[index].dmd_msg, req->info[0], req->info[1],
					   req->info[2], req->info[3]);
		break;

	case 5:
		ret = bbox_diaginfo_record(req->dmd_id, NULL, dmd_log[index].dmd_msg, req->info[0], req->info[1],
					   req->info[2], req->info[3], req->info[4]);
		break;

	default:
		igs_log_err("igs get shb dmd report: info arry size[%u] error!! dmd_id[%u]\n",
			      req->resv1, req->dmd_id);
		return -EFAULT;
	}

	return ret;
}

STATIC void igs_sensorhub_reset_handler(void)
{
	struct igs_read_data_t read_data = {0};
#ifdef CONFIG_MM_LB
	int ret;
#endif

	igs_log_info("enter [%s]\n", __func__);
	mutex_lock(&g_igs_priv.read_mutex);

	if (kfifo_avail(&g_igs_priv.read_kfifo) < sizeof(struct igs_read_data_t)) {
		igs_log_err("%s read_kfifo is full, drop upload data.\n", __func__);
		goto ERR;
	}

	read_data.recv_len = sizeof(u32);
	read_data.p_recv = kzalloc(sizeof(u32), GFP_ATOMIC);
	if (read_data.p_recv == NULL) {
		igs_log_err("Failed to alloc memory for sensorhub reset message...\n");
		goto ERR;
	}

	*(u32 *)read_data.p_recv = IGS_RESET_NOTIFY;

	if (kfifo_in(&g_igs_priv.read_kfifo, (unsigned char *)&read_data, sizeof(struct igs_read_data_t)) == 0) {
		igs_log_err("%s: kfifo_in failed\n", __func__);
		goto ERR;
	}

	mutex_unlock(&g_igs_priv.read_mutex);

	g_igs_priv.sh_recover_flag = 1;

#ifdef CONFIG_CONTEXTHUB_IGS_20
	if (g_igs_priv.send_ipc_to_shb != 0) {
		igs_log_info("%s: sc_flag = %u.\n", __func__, g_config_on_ddr->sc_flag);
		if (g_config_on_ddr->sc_flag == 1) {
#ifdef CONFIG_MM_LB
			ret = lb_release_quota(IGS_SYSCAHCE_ID);
			if (ret != 0)
				igs_log_err("%s: lb_release_quota failed[%d].\n", __func__, ret);
#endif
		}
		g_igs_priv.send_ipc_to_shb = 0;
	}
#endif

	igs_dev_complete(&g_igs_priv.read_wait);
	igs_dev_complete(&g_igs_priv.igs_wait);
	return;

ERR:
	if (read_data.p_recv != NULL)
		kfree(read_data.p_recv);

	mutex_unlock(&g_igs_priv.read_mutex);
}

STATIC int igs_sensorhub_reset_notifier(struct notifier_block *nb, unsigned long action, void *data)
{
	switch (action) {
	case IOM3_RECOVERY_IDLE:
		igs_sensorhub_reset_handler();
		break;
	default:
		break;
	}

	return 0;
}

static struct notifier_block igs_reboot_notify = {
	.notifier_call = igs_sensorhub_reset_notifier,
	.priority = -1,
};

STATIC ssize_t igs_dev_read(struct file *file,
			      char __user *buf, size_t count, loff_t *pos)
{
	struct igs_read_data_t read_data = {0};
	u32 error = 0;
	u32 length;
	int ret = 0;

	igs_log_info("[%s]\n", __func__);
	if (buf == NULL || count == 0)
		goto ERR;

	ret = igs_dev_wait_completion(&g_igs_priv.read_wait, 0);
	if (ret != 0)
		goto ERR;

	mutex_lock(&g_igs_priv.read_mutex);
	if (kfifo_len(&g_igs_priv.read_kfifo) < sizeof(struct igs_read_data_t)) {
		igs_log_err("%s: read data failed.\n", __func__);
		mutex_unlock(&g_igs_priv.read_mutex);
		goto ERR;
	}

	if (kfifo_out(&g_igs_priv.read_kfifo, (unsigned char *)&read_data, sizeof(struct igs_read_data_t)) == 0
		|| !read_data.p_recv) {
		igs_log_err("%s: kfifo out failed.\n", __func__);
		mutex_unlock(&g_igs_priv.read_mutex);
		goto ERR;
	}

	if (count < read_data.recv_len) {
		length = count;
		igs_log_err("%s user buffer is too small\n", __func__);
	} else {
		length = read_data.recv_len;
	}

	igs_log_info("[%s] copy len[0x%x], count[0x%x]\n", __func__, read_data.recv_len, (u32)count);

	error = length;
	/* copy to user */
	if (copy_to_user(buf, read_data.p_recv, length)) {
		igs_log_err("%s failed to copy to user\n", __func__);
		error = 0;
	}

	mutex_unlock(&g_igs_priv.read_mutex);

ERR:
	if (read_data.p_recv != NULL) {
		kfree(read_data.p_recv);
		read_data.p_recv = NULL;
		read_data.recv_len = 0;
	}

	return error;
}

STATIC ssize_t igs_dev_write(struct file *file, const char __user *data,
			       size_t len, loff_t *ppos)
{
	igs_log_info("%s need to do...\n", __func__);
	return len;
}

STATIC int igs_dev_open(struct inode *inode, struct file *file)
{
	int ret = 0;

	igs_log_info("enter %s\n", __func__);
	mutex_lock(&g_igs_priv.igs_mutex);

	if (g_igs_priv.ref_cnt != 0) {
		igs_log_warn("%s duplicate open.\n", __func__);
		mutex_unlock(&g_igs_priv.igs_mutex);
		return -EFAULT;
	}

	if (g_igs_priv.ref_cnt == 0)
		ret = send_cmd_from_kernel(TAG_IGS, CMD_CMN_OPEN_REQ, 0, NULL, (size_t)0);

	file->private_data = &g_igs_priv;

	if (ret == 0)
		g_igs_priv.ref_cnt++;

	g_igs_priv.sh_recover_flag = 0;
	g_igs_priv.send_ipc_to_shb = 0;

	mutex_unlock(&g_igs_priv.igs_mutex);
	return ret;
}

STATIC int igs_dev_flush(struct file *file, fl_owner_t id)
{
	struct read_info rd;
#ifdef CONFIG_CONTEXTHUB_IGS_20
	struct igs_ion_node *cur = NULL;
	struct igs_ion_node *next = NULL;
#endif

	igs_log_info("enter %s\n", __func__);

	mutex_lock(&g_igs_priv.igs_mutex);
	if (g_igs_priv.ref_cnt == 0) {
		igs_log_err("%s: ref cnt is 0.\n", __func__);
		mutex_unlock(&g_igs_priv.igs_mutex);
		return -EFAULT;
	}

	g_igs_priv.ref_cnt--;

	if (g_igs_priv.ref_cnt == 0) {
		(void)memset_s((void *)&rd, sizeof(struct read_info), 0, sizeof(struct read_info));

		send_cmd_from_kernel_response(TAG_IGS, CMD_CMN_CLOSE_REQ, 0, NULL, (size_t)0, &rd);
		igs_log_info("%s: got close resp\n", __func__);
	}

#ifdef CONFIG_CONTEXTHUB_IGS_20
	list_for_each_entry_safe(cur, next, &g_igs_priv.ion_buf_list, list) {
		igs_log_info("%s: detach %x\n", __func__, cur->ion_info.fd);

		dma_buf_put(cur->ion_info.buf);
		list_del(&(cur->list));
		kfree(cur);
	}
#endif
	mutex_unlock(&g_igs_priv.igs_mutex);
	igs_dev_complete(&g_igs_priv.read_wait);

	return 0;
}

static int npu_get_avs_volt(struct device_node *np)
{
	const struct property *prop = NULL;
	int nr, i;
	const char *npu_avs_name = "npu-avs-volt";
	const char *margin_code_name = "pa-code-margin";

	if (g_igs_pa_margin == NULL || g_igs_avs_data == NULL ||
	    g_pa_margin_num == 0 || g_avs_data_num == 0) {
		igs_log_err("%s: invalid g_smplat_scfg\n", __func__);
		return -EINVAL;
	}
	prop = of_find_property(np, npu_avs_name, NULL);
	if (prop == NULL) {
		igs_log_warn("%s: no property:%s\n", __func__, npu_avs_name);
		return 0;
	}
	if (!prop->value) {
		igs_log_err("%s: property:%s value err\n", __func__, npu_avs_name);
		return -ENODATA;
	}

	nr = (int)(prop->length / sizeof(u32));
	if (nr > g_avs_data_num) {
		igs_log_err("%s: property:%s value num err, nr=%d\n", __func__, npu_avs_name, nr);
		return -EINVAL;
	}

	if (of_property_read_u32_array(np, npu_avs_name, g_igs_avs_data, nr)) {
		igs_log_err("%s: property:%s array err\n", __func__, npu_avs_name);
		return -EINVAL;
	}

	for (i = 0; i < nr / 2; i++) { /* 2 int for 1 profile avs data */
		if (i * 2 + 1 < g_avs_data_num)
			igs_log_info("[%s]: npu_avs_volt[%d]=0x%x %x\n", __func__, i,
				       g_igs_avs_data[i * 2 + 1], /* 2 int for 1 profile avs data */
				       g_igs_avs_data[i * 2]);
	}

	/* read margin */
	prop = of_find_property(np, margin_code_name, NULL);
	if (prop == NULL) {
		igs_log_warn("%s: no property:%s\n", __func__, margin_code_name);
		return 0;
	}
	if (!prop->value) {
		igs_log_err("%s: property:%s value err\n", __func__, margin_code_name);
		return -ENODATA;
	}

	nr = (int)(prop->length / sizeof(u32));
	if (nr > g_pa_margin_num) {
		igs_log_err("%s: property:%s value num err, nr=%d\n", __func__, margin_code_name, nr);
		return -EINVAL;
	}

	if (of_property_read_u32_array(np, margin_code_name, g_igs_pa_margin, nr)) {
		igs_log_err("%s: property:%s array err\n", __func__, margin_code_name);
		return -EINVAL;
	}
	igs_log_info("[%s]: pa_margin=0x%x, 0x%x\n", __func__, g_igs_pa_margin[0],
		       g_igs_pa_margin[1]);
	return 0;
}

static int npu_get_profvolt_margin(struct device_node *np)
{
	const struct property *prop = NULL;
	int nr;
	const char *npu_profvolt_margin = "npu-profvolt-margin";

	if (g_prof_margin == NULL || g_prof_num == 0) {
		igs_log_err("%s: invalid g_smplat_scfg\n", __func__);
		return -EINVAL;
	}
	prop = of_find_property(np, npu_profvolt_margin, NULL);
	if (prop == NULL) {
		igs_log_warn("%s: no property:%s\n", __func__, npu_profvolt_margin);
		return 0;
	}
	if (!prop->value) {
		igs_log_err("%s: property:%s value err\n", __func__, npu_profvolt_margin);
		return -ENODATA;
	}

	nr = (int)(prop->length / sizeof(u32));
	if (nr > g_prof_num) {
		igs_log_err("%s: property:%s value num err, nr=%d\n", __func__, npu_profvolt_margin, nr);
		return -EINVAL;
	}

	if (of_property_read_u32_array(np, npu_profvolt_margin, g_prof_margin, nr)) {
		igs_log_err("%s: property:%s array err\n", __func__, npu_profvolt_margin);
		return -EINVAL;
	}

	return 0;
}

static int get_npu_svfd_code(struct device_node *np)
{
	const struct property *prop = NULL;
	int nr;
	const char *svfd_code_name = "npu-svfd-code";
	const char *svfd_para_name = "npu-svfd-para";

	if (g_igs_svfd_data == NULL || g_igs_svfd_para == NULL ||
	    g_svfd_data_num == 0 || g_svfd_para_num == 0) {
		igs_log_err("%s: invalid g_smplat_scfg\n", __func__);
		return -EINVAL;
	}

	prop = of_find_property(np, svfd_code_name, NULL);
	if (prop == NULL) {
		igs_log_warn("%s: find property:npu-svfd-code fail\n", __func__);
		return 0;
	}
	if (!prop->value) {
		igs_log_err("%s: property:npu-svfd-code value err\n", __func__);
		return -ENODATA;
	}

	nr = (int)(prop->length / sizeof(u32));
	if (nr > g_svfd_data_num) {
		igs_log_err("%s: property:npu-svfd-code value num err, nr=%d\n", __func__, nr);
		return -EINVAL;
	}

	if (of_property_read_u32_array(np, svfd_code_name, g_igs_svfd_data, nr)) {
		igs_log_err("%s: property:%s array err\n", __func__, svfd_code_name);
		return -EINVAL;
	}

	/* npu_svfd_para */
	prop = of_find_property(np, svfd_para_name, NULL);
	if (prop == NULL) {
		igs_log_warn("%s: find property:npu-svfd-para fail\n", __func__);
		return 0;
	}
	if (!prop->value) {
		igs_log_err("%s: property:npu-svfd-para value err\n", __func__);
		return -ENODATA;
	}

	nr = (int)(prop->length / sizeof(u32));
	if (nr > g_svfd_para_num) {
		igs_log_err("%s: property:npu-svfd-para value num err, nr=%d\n", __func__, nr);
		return -EINVAL;
	}

	if (of_property_read_u32_array(np, svfd_para_name, g_igs_svfd_para, nr)) {
		igs_log_err("%s: property:%s array err\n", __func__, svfd_para_name);
		return -EINVAL;
	}

	return 0;
}

static int get_level2_chip_type(struct device_node *np)
{
	const struct property *prop = NULL;
	size_t nr;
	const char *level2_chip_name = "level2_chip";

	if (g_level2_chip == NULL) {
		igs_log_err("%s: invalid g_smplat_scfg\n", __func__);
		return -EINVAL;
	}

	prop = of_find_property(np, level2_chip_name, NULL);
	if (prop == NULL) {
		igs_log_warn("%s: find property:level2_chip fail\n", __func__);
		return 0;
	}
	if (!prop->value) {
		igs_log_err("%s: property:level2_chip value err\n", __func__);
		return -ENODATA;
	}

	nr = (uint32_t)prop->length / sizeof(u32);
	if (nr > 1) {
		igs_log_err("%s: property:level2_chip value num err, nr=%u\n", __func__, nr);
		return -EINVAL;
	}

	if (of_property_read_u32_array(np, level2_chip_name, g_level2_chip, nr)) {
		igs_log_err("%s: property:%s array err\n", __func__, level2_chip_name);
		return -EINVAL;
	}
	igs_log_info("%s: g_level2_chip:%u\n", __func__, *g_level2_chip);

	return 0;
}

STATIC int get_igs_dev_dts_status(void)
{
	struct device_node *node = NULL;
	int temp = 0;

	node = of_find_compatible_node(NULL, NULL, IGS_DEV_COMPAT_STR);
	if (node == NULL) {
		pr_warn("[%s] : no igs dev..\n", __func__);
		return -ENODEV;
	}

	if (!of_device_is_available(node)) {
		pr_warn("[%s] igs disabled..\n", __func__);
		return -ENODATA;
	}

	if (of_property_read_u32(node, "is_pll_on", &temp))
		igs_log_warn("[%s]:read product_id_value fail\n", __func__);
	else
		g_config_on_ddr->is_pll_on = (u32)temp;

	igs_log_info("[%s] pid 0x%x..\n", __func__, temp);

	pr_info("[%s][enabled]\n", __func__);

	return 0;
}

STATIC void get_npu_dts_status(void)
{
	struct device_node *node = NULL;

	node = of_find_compatible_node(NULL, NULL, IGS_DEV_COMPAT_STR);
	if (node == NULL) {
		pr_warn("[%s] : no igs dev..\n", __func__);
		return;
	}

	if (get_npu_data_ptr()) {
		if (npu_get_avs_volt(node) != 0) {
			igs_log_err("[%s] fail..\n", __func__);
			return;
		}
		if (npu_get_profvolt_margin(node) != 0) {
			igs_log_err("[%s] fail..\n", __func__);
			return;
		}
		if (get_npu_svfd_code(node) != 0) {
			igs_log_err("[%s] fail..\n", __func__);
			return;
		}
		if (get_level2_chip_type(node) != 0) {
			igs_log_err("[%s] fail..\n", __func__);
			return;
		}
	}
	igs_log_info("%s\n", __func__);
}

#ifdef CONFIG_CONTEXTHUB_IGS_20
static int igs_notifier(struct notifier_block *nb, unsigned long action, void *data)
{
	int ret;

	if (!data)
		return NOTIFY_OK;

	switch (action) {
	case FB_EVENT_BLANK: /* change finished */
	{
		struct fb_event *event = data;
		int *blank = event->data;

		if (registered_fb[0] !=
			event->info) { /* only main screen on/off info send to hub */
			igs_log_err("%s, not main screen info, just return\n", __func__);
			return NOTIFY_OK;
		}
		switch (*blank) {
		case FB_BLANK_UNBLANK:
			mutex_lock(&g_igs_priv.wklock_mutex);
			/* screen on */
			igs_log_info("[%s] screen on\n", __func__);
			if (g_igs_priv.igs_wklock->active)
				__pm_relax(g_igs_priv.igs_wklock);
			ret = send_cmd_from_kernel(TAG_IGS, CMD_CMN_CONFIG_REQ,
						   SUB_CMD_SWING_SCREEN_ON, NULL, (size_t)0);
			if (ret != 0)
				igs_log_err("[%s]send cmd error[%d]\n", __func__, ret);

			g_config_on_ddr->screen_sts = IGS_SCREEN_ON;
			mutex_unlock(&g_igs_priv.wklock_mutex);
			break;

		case FB_BLANK_POWERDOWN:
			mutex_lock(&g_igs_priv.wklock_mutex);
			/* screen off, wake-lock, util receive ack from sensorhub */
			igs_log_info("[%s] screen off\n", __func__);
			if (!g_igs_priv.igs_wklock->active) {
				igs_log_info("[%s] wake lock.\n", __func__);
				__pm_wakeup_event(g_igs_priv.igs_wklock, IGS_IOCTL_WAIT_TMOUT * 2);
			}

			ret = send_cmd_from_kernel(TAG_IGS, CMD_CMN_CONFIG_REQ,
						   SUB_CMD_SWING_SCREEN_OFF, NULL, (size_t)0);
			if (ret != 0) {
				igs_log_err("[%s]send cmd error[%d]\n", __func__, ret);
				/* send ipc failed, release wake-lock */
				__pm_relax(g_igs_priv.igs_wklock);
			}

			g_config_on_ddr->screen_sts = IGS_SCREEN_OFF;
			mutex_unlock(&g_igs_priv.wklock_mutex);
			break;

		default:
			igs_log_err("unknown---> lcd unknown in %s\n", __func__);
			break;
		}
		break;
	}
	default:
		break;
	}

	return NOTIFY_OK;
}

static struct notifier_block igs_fb_notify = {
	.notifier_call = igs_notifier,
};
#endif

static const struct file_operations igs_dev_fops = {
	.owner             = THIS_MODULE,
	.llseek            = no_llseek,
	.unlocked_ioctl    = igs_dev_ioctl,
	.open              = igs_dev_open,
	.flush             = igs_dev_flush,
	.read              = igs_dev_read,
	.write             = igs_dev_write,
};

static struct miscdevice igs_miscdev = {
	.minor =    MISC_DYNAMIC_MINOR,
	.name =     "swing_dev",
	.fops =     &igs_dev_fops,
};

STATIC int __init igs_dev_init(struct platform_device *pdev)
{
	int ret = 0;

	if (is_sensorhub_disabled()) {
		igs_log_err("sensorhub disabled....\n");
		return -EFAULT;
	}

	ret = get_igs_dev_dts_status();
	if (ret != 0) {
		igs_log_warn("%s igs is disabled\n", __func__);
		return ret;
	}

	if (g_config_on_ddr->igs_hardware_bypass != 0) {
		igs_log_warn("%s: igs hardware bypass!\n", __func__);
		return -ENODEV;
	}

	ret = misc_register(&igs_miscdev);
	if (ret != 0) {
		igs_log_err("%s cannot register miscdev err=%d\n", __func__, ret);
		return ret;
	}
#ifdef CONFIG_CONTEXTHUB_IGS_20
	ret = fb_register_client(&igs_fb_notify);
	if (ret != 0) {
		igs_log_err("%s: unable to register fb_notifier: %d", __func__, ret);
		misc_deregister(&igs_miscdev);
		return ret;
	}

	INIT_LIST_HEAD(&g_igs_priv.ion_buf_list);
#endif
	mutex_init(&g_igs_priv.read_mutex);
	mutex_init(&g_igs_priv.igs_mutex);
	mutex_init(&g_igs_priv.ioctl_mutex);
	mutex_init(&g_igs_priv.sr_mutex);

	igs_dev_wait_init(&g_igs_priv.igs_wait);
	igs_dev_wait_init(&g_igs_priv.read_wait);
#ifdef CONFIG_CONTEXTHUB_IGS_20
	mutex_init(&g_igs_priv.wklock_mutex);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
	g_igs_priv.igs_wklock = wakeup_source_register(g_igs_priv.self, "igs-wklock");
#else
	g_igs_priv.igs_wklock = wakeup_source_register("igs-wklock");
#endif
	if (!g_igs_priv.igs_wklock) {
		igs_log_err("[%s]wakeup source register failed\n", __func__);
		goto ERR1;
	}
#endif
	ret = register_mcu_event_notifier(TAG_IGS, CMD_CMN_CONFIG_RESP, igs_get_resp);
	if (ret != 0) {
		igs_log_err("[%s]: register CMD_CMN_CONFIG_RESP notifier failed. [%d]\n", __func__, ret);
		goto ERR1;
	}

	ret = register_mcu_event_notifier(TAG_IGS, CMD_LOG_REPORT_REQ, igs_get_shb_dmd_report);
	if (ret != 0) {
		igs_log_err("[%s]: register DMD CMD_LOG_REPORT_REQ notifier failed. [%d]\n", __func__, ret);
		goto ERR2;
	}

	ret = register_iom3_recovery_notifier(&igs_reboot_notify);
	if (ret < 0) {
		igs_log_err("[%s]register_iom3_recovery_notifier fail\n", __func__);
		goto ERR3;
	}

	if (kfifo_alloc(&g_igs_priv.read_kfifo,
			sizeof(struct igs_read_data_t) * IGS_READ_CACHE_COUNT, GFP_KERNEL)) {
		igs_log_err("%s kfifo alloc failed.\n", __func__);
		ret = -ENOMEM;
		goto ERR4;
	}

	g_igs_priv.ref_cnt = 0;
	g_igs_priv.self = &(pdev->dev);
	g_igs_priv.send_ipc_to_shb = 0;

	g_config_on_ddr->screen_sts = IGS_SCREEN_ON;

	pdev->dev.dma_mask = &g_igs_dmamask;
	pdev->dev.coherent_dma_mask = IGS_DMA_MASK;

	return 0;

ERR4:
	/* unregister_iom3_recovery_notifier */
ERR3:
	unregister_mcu_event_notifier(TAG_IGS, CMD_LOG_REPORT_REQ, igs_get_shb_dmd_report);
ERR2:
	unregister_mcu_event_notifier(TAG_IGS, CMD_CMN_CONFIG_RESP, igs_get_resp);
ERR1:
#ifdef CONFIG_CONTEXTHUB_IGS_20
	wakeup_source_unregister(g_igs_priv.igs_wklock);
	g_igs_priv.igs_wklock = NULL;
	fb_unregister_client(&igs_fb_notify);
#endif
	misc_deregister(&igs_miscdev);

	igs_log_err("%s : init failed....\n", __func__);

	return ret;
}

STATIC void __exit igs_dev_exit(void)
{
	igs_log_info("%s : enter....\n", __func__);

	kfifo_free(&g_igs_priv.read_kfifo);

	unregister_mcu_event_notifier(TAG_IGS, CMD_CMN_CONFIG_RESP, igs_get_resp);
	unregister_mcu_event_notifier(TAG_IGS, CMD_LOG_REPORT_REQ, igs_get_shb_dmd_report);

#ifdef CONFIG_CONTEXTHUB_IGS_20
	if (g_igs_priv.igs_wklock->active)
		__pm_relax(g_igs_priv.igs_wklock);

	wakeup_source_unregister(g_igs_priv.igs_wklock);
	g_igs_priv.igs_wklock = NULL;
	fb_unregister_client(&igs_fb_notify);
#endif
	misc_deregister(&igs_miscdev);
}

/* probe() function for platform driver */
static int igs_probe(struct platform_device *pdev)
{
	if (pdev == NULL) {
		igs_log_err("igs probe: pdev is NULL\n");
		return -EFAULT;
	}

	igs_log_info("igs probe...\n");

	return igs_dev_init(pdev);
}

/* remove() function for platform driver */
static int __exit igs_remove(struct platform_device *pdev)
{
	igs_log_info("igs remove...\n");

	igs_dev_exit();

	return 0;
}

/*
 * suspend
 */
static int igs_pm_suspend(struct device *dev)
{
	int ret = 0;

#ifndef CONFIG_CONTEXTHUB_IGS_20
	/* just return, if igs20 is off. */
	return 0;
#endif

	mutex_lock(&g_igs_priv.igs_mutex);

	if (g_igs_priv.ref_cnt == 0) {
		igs_log_info("[%s] igs is not open.\n", __func__);
		mutex_unlock(&g_igs_priv.igs_mutex);
		return 0;
	}

	mutex_lock(&g_igs_priv.sr_mutex);
	if (g_igs_priv.send_ipc_to_shb != 0) {
		igs_log_err("[%s]not send suspend to shb. g_igs_priv.send_ipc_to_shb:[%d]\n",
			      __func__, g_igs_priv.send_ipc_to_shb);
		goto suspend_err;
	}

	igs_log_info("[%s]sc_flag = %u.\n", __func__, g_config_on_ddr->sc_flag);
	if (g_config_on_ddr->sc_flag == 1) {
#ifdef CONFIG_MM_LB
		ret = lb_request_quota(IGS_SYSCAHCE_ID);
		if (ret != 0) {
			igs_log_err("[%s]lb_request_quota failed[%d]\n", __func__, ret);
			goto suspend_err;
		}
#endif
	}

	g_igs_priv.send_ipc_to_shb = IGS_SEND_SUSPEND_TO_SHB;

	ret = send_cmd_from_kernel(TAG_IGS, CMD_CMN_CONFIG_REQ, SUB_CMD_SWING_SYSCACHE_ENABLE,
				   NULL, (size_t)0);
	if (ret != 0) {
		igs_log_err("[%s]send cmd error[%d]\n", __func__, ret);
		goto suspend_err;
	}

suspend_err:
	mutex_unlock(&g_igs_priv.sr_mutex);
	mutex_unlock(&g_igs_priv.igs_mutex);
	return ret;
}

/*
 * resume
 */
static int igs_pm_resume(struct device *dev)
{
	int ret = 0;

#ifndef CONFIG_CONTEXTHUB_IGS_20
	/* just return, if igs20 is off. */
	return 0;
#endif

	mutex_lock(&g_igs_priv.igs_mutex);

	if (g_igs_priv.ref_cnt == 0) {
		igs_log_info("[%s] igs is not open.\n", __func__);
		mutex_unlock(&g_igs_priv.igs_mutex);
		return 0;
	}

	mutex_lock(&g_igs_priv.sr_mutex);

	if (g_igs_priv.send_ipc_to_shb != IGS_SEND_SUSPEND_TO_SHB)
		igs_log_info("[%s] shb condition not match, send_ipc_to_shb: %d\n", __func__,
			       g_igs_priv.send_ipc_to_shb);

	if (g_igs_priv.send_ipc_to_shb == 0) {
		igs_log_info("[%s]not send resume to shb: g_igs_priv.send_ipc_to_shb %d\n", __func__,
			       g_igs_priv.send_ipc_to_shb);
		goto end;
	}

	ret = send_cmd_from_kernel(TAG_IGS, CMD_CMN_CONFIG_REQ, SUB_CMD_SWING_SYSCACHE_DISABLE,
				   NULL, (size_t)0);
	if (ret != 0) {
		igs_log_err("[%s]send cmd error[%d]\n", __func__, ret);
		goto end;
	}

	g_igs_priv.send_ipc_to_shb = IGS_SEND_RESUME_TO_SHB;

end:
	mutex_unlock(&g_igs_priv.sr_mutex);
	mutex_unlock(&g_igs_priv.igs_mutex);

	return ret;
}

const struct dev_pm_ops igs_pm_ops = {
	.suspend = igs_pm_suspend,
	.resume  = igs_pm_resume,
};

static const struct of_device_id igs_match_table[] = {
	{ .compatible = IGS_DEV_COMPAT_STR, },
	{},
};

static struct platform_driver igs_platdev = {
	.driver = {
		.name = "igs_dev",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(igs_match_table),
		.pm = &igs_pm_ops,
	},
	.probe  = igs_probe,
	.remove = igs_remove,
};

static int __init igs_main_init(void)
{
	get_npu_dts_status();
	return platform_driver_register(&igs_platdev);
}

static void __exit igs_main_exit(void)
{
	platform_driver_unregister(&igs_platdev);
}

late_initcall_sync(igs_main_init);
module_exit(igs_main_exit);
