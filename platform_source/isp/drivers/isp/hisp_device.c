/*
 * HiStar Remote Processor driver
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/mod_devicetable.h>
#include <linux/amba/bus.h>
#include <linux/sizes.h>
#include <linux/dma-mapping.h>
#include <linux/remoteproc.h>
#include <linux/delay.h>
#include <linux/kfifo.h>
#include <linux/pm_wakeup.h>
#include <linux/mutex.h>
#include <linux/regulator/consumer.h>
#include <linux/scatterlist.h>
#include <linux/clk.h>
#include <linux/sched/rt.h>
#include <linux/kthread.h>
#include <global_ddr_map.h>
#include <isp_ddr_map.h>
#include <asm/cacheflush.h>
#include <linux/firmware.h>
#include <linux/iommu.h>
#include <linux/mm_iommu.h>
#include <linux/crc32.h>
#include <linux/ion.h>
#include <linux/platform_drivers/mm_ion.h>
#include <linux/spinlock.h>
#include "isp_ddr_map.h"
#include <linux/dma-buf.h>
#include <linux/version.h>
#include <linux/syscalls.h>
#ifdef CONFIG_VENDOR_EFUSE
#include <platform_include/see/efuse_driver.h>
#endif
#include <platform_include/isp/linux/hisp_remoteproc.h>
#include <platform_include/isp/linux/hisp_mempool.h>
#include <linux/sched/types.h>
#include <securec.h>
#include <asm/memory.h>
#include <hisp_internel.h>
#ifdef CONFIG_CAMERA_USE_HISP230
#include "soc_spec_info.h"
#endif
#ifdef CONFIG_HUAWEI_DSM
#include <dsm/dsm_pub.h>
#endif
#ifdef DEBUG_HISP
#include "hisp_pcie.h"
#endif
#ifdef  CONFIG_RPMSG_HISP
#include <platform_include/isp/linux/hisp_rpmsg.h>
#endif
#define LOGON_DBG                    (1 << 0)
#define LOGON_INF                    (1 << 1)
#define LOGON_ERR                    (1 << 2)
#define LOGON_WAR                    (1 << 3)

static unsigned int g_perf_data;
static unsigned int hisplog_mask = LOGON_INF | LOGON_ERR;

#define HISP_LOG_SIZE                128

#define TIMEOUT_IS_FPGA_BOARD        10000
#define TIMEOUT_IS_NOT_FPGA_BOARD    5000
#define BOARDID_SIZE                 4
#define DTS_COMP_LOGIC_NAME          "hisilicon,isplogic"

#define INVALID_BOARDID              0xFFFFFFFF

#define hisp_min(a, b) (((a) < (b)) ? (a) : (b))

enum isp_isplogic_type_info_e {
	ISP_FPGA_EXCLUDE    = 0x0,
	ISP_FPGA,
	ISP_UDP,
	ISP_FPGA_EXC,
	ISP_MAXTYPE
};

enum hisp_power_state {
	HISP_ISP_POWER_OFF      = 0,
	HISP_ISP_POWER_ON       = 1,
	HISP_ISP_POWER_FAILE    = 2,
	HISP_ISP_POWER_CLEAN    = 3,
};

enum phy_csi_type {
	PHY_ID_TYPE,
	CSI_ID_TYPE,
	PHY_CSI_MAX_TYPE,
};

struct phy_csi_info_t {
	struct hisp_phy_info_t phy_info;
	unsigned int csi_idx;
};

struct rproc_camera {
	struct clk *aclk;
	struct clk *aclk_dss;
	struct clk *pclk_dss;
	/* pinctrl */
	struct pinctrl *isp_pinctrl;
	struct pinctrl_state *pinctrl_def;
	struct pinctrl_state *pinctrl_idle;
};

struct hisp_boot_device {
	bool pwronisp_wake;
	unsigned int isp_efuse_flag;
	unsigned int boardid;
	unsigned int reg_num;
	unsigned int isppd_adb_flag;
	unsigned int isp_wdt_flag;
	unsigned int pwronisp_nice;
	unsigned int pwronisp_thread_status;
	unsigned int use_logb_flag;
	unsigned int hisp_power_state;
	unsigned int chip_flag;
	int bypass_pwr_updn;
	int probe_finished;
	int last_boot_state;
	int ispcpu_status;
	unsigned int phy_csi_num[PHY_CSI_MAX_TYPE];
	void __iomem *reg[ISP_MAX_REGTYPE];
	enum hisp_rproc_case_attr case_type;
	struct platform_device *isp_pdev;
#ifdef CONFIG_HUAWEI_DSM
	struct dsm_client *hisp_dsm_client;
#endif
	struct resource *r[ISP_MAX_REGTYPE];
	struct wakeup_source *ispcpu_wakelock;
	struct wakeup_source *jpeg_wakelock;
	struct task_struct *pwronisp;
	struct hisp_shared_para *share_para;
	struct isp_plat_cfg tmp_plat_cfg;
	struct rproc_camera isp_data;
	struct mutex ispcpu_mutex;
	struct mutex jpeg_mutex;
	struct mutex sharedbuf_mutex;
	struct mutex hisp_power_mutex;
	struct mutex rproc_state_mutex;
	struct completion rpmsg_boot_comp;
	wait_queue_head_t pwronisp_wait;
	atomic_t rproc_enable_status;
};

struct hisp_boot_device hisp_boot_dev;

static void hisplog(unsigned int on, const char *tag,
			const char *func, const char *fmt, ...)
{
	va_list args;
	char pbuf[HISP_LOG_SIZE] = {0};
	int size;

	if ((hisplog_mask & on) == 0 || fmt == NULL)
		return;

	va_start(args, fmt);
	size = vsnprintf_s(pbuf, HISP_LOG_SIZE, HISP_LOG_SIZE - 1, fmt, args);
	va_end(args);

	if ((size < 0) || (size > HISP_LOG_SIZE - 1))
		return;

	pbuf[size] = '\0';
	pr_info("Hisp rprocDrv [%s][%s] %s", tag, func, pbuf);
}

#define hisp_debug(fmt, ...) \
	hisplog(LOGON_DBG, "D", __func__, fmt, ##__VA_ARGS__)
#define hisp_info(fmt, ...) \
	hisplog(LOGON_INF, "I", __func__, fmt, ##__VA_ARGS__)
#define hisp_err(fmt, ...) \
	hisplog(LOGON_ERR, "E", __func__, fmt, ##__VA_ARGS__)

static int hisp_phy_csi_check(struct hisp_phy_info_t *phy_info,
				  unsigned int csi_index)
{
	struct hisp_boot_device *dev = &hisp_boot_dev;

	if (phy_info == NULL) {
		pr_err("[%s] phy_info is NULL!\n", __func__);
		return -EINVAL;
	}

	if (phy_info->phy_id >= dev->phy_csi_num[PHY_ID_TYPE]) {
		pr_err("[%s] wrong para: phy_id = %u!\n",
			__func__, phy_info->phy_id);
		return -EINVAL;
	}

	if (phy_info->phy_mode >= HISP_PHY_MODE_MAX) {
		pr_err("[%s] wrong para: phy_mode = %u!\n",
			__func__, phy_info->phy_mode);
		return -EINVAL;
	}

	if (phy_info->phy_freq_mode >= HISP_PHY_FREQ_MODE_MAX) {
		pr_err("[%s] wrong para: phy_freq_mode = %u!\n",
			__func__, phy_info->phy_freq_mode);
		return -EINVAL;
	}

	if (phy_info->phy_work_mode >= HISP_PHY_WORK_MODE_MAX) {
		pr_err("[%s] wrong para: phy_work_mode = %u!\n",
			__func__, phy_info->phy_work_mode);
		return -EINVAL;
	}

	if (csi_index >= dev->phy_csi_num[CSI_ID_TYPE]) {
		pr_err("[%s] wrong para: csi_index = %u!\n",
			__func__, csi_index);
		return -EINVAL;
	}
	return 0;
}

int hisp_phy_csi_connect(struct hisp_phy_info_t *phy_info,
				  unsigned int csi_index)
{
	int ret = 0;
	u64 sharepa = 0;
	struct phy_csi_info_t *shareva = NULL;
	struct phy_csi_info_t phy_csi_info;

	ret = hisp_phy_csi_check(phy_info, csi_index);
	if (ret < 0) {
		pr_err("[%s] fail: hisp_phy_csi_check.%d!\n", __func__, ret);
		return ret;
	}

	ret = memcpy_s(&phy_csi_info.phy_info, sizeof(struct hisp_phy_info_t),
		       phy_info, sizeof(struct hisp_phy_info_t));
	if (ret != 0) {
		pr_err("[%s] memcpy_s phy info fail.%d!\n", __func__, ret);
		return ret;
	}

	phy_csi_info.csi_idx = csi_index;
	shareva = hisp_get_param_info_va();
	if (shareva == NULL) {
		pr_err("[%s] hisp_phy_csi_info_vaddr is NULL!\n", __func__);
		return -EINVAL;
	}

	ret = memcpy_s((void *)(shareva), sizeof(struct phy_csi_info_t),
		       (const void *)(&phy_csi_info),
		       sizeof(struct phy_csi_info_t));
	if (ret != 0) {
		pr_err("[%s] memcpy_s phy csi info fail.%d!\n", __func__, ret);
		return ret;
	}

	sharepa = hisp_get_param_info_pa();
	if (sharepa == 0) {
		pr_err("[%s] hisp_phy_csi_info_paddr is NULL!\n", __func__);
		return -EINVAL;
	}

	ret = hisp_smc_phy_csi_connect(sharepa);
	if (ret != 0) {
		pr_err("[%s] hisp_smc_phy_csi_connect fail ret.%d!\n",
				__func__, ret);
		return ret;
	}

	pr_info("[%s] csi_index = %d\n", __func__, csi_index);
	pr_info("[%s] is master sensor = %d, phy_id = %d\n",
		__func__, phy_info->is_master_sensor, phy_info->phy_id);
	pr_info("[%s] phy_mode = %d,phy_freq_mode = %d\n",
		__func__, phy_info->phy_mode, phy_info->phy_freq_mode);
	pr_info("[%s] phy_freq = %d,phy_work_mode = %d\n",
		__func__, phy_info->phy_freq, phy_info->phy_work_mode);
#ifdef DEBUG_HISP
	pr_info("[%s] : phy_csi_connect_type.%d, csia.0x%08x, b.0x%08x, c.0x%08x, e.0x%08x\n",
		__func__, shareva->phy_info.is_master_sensor,
		shareva->phy_info.phy_id, shareva->phy_info.phy_mode,
		shareva->phy_info.phy_freq_mode, shareva->phy_info.phy_freq);
#endif
	return 0;
}
EXPORT_SYMBOL(hisp_phy_csi_connect); /*lint !e508*/

int hisp_phy_csi_disconnect(void)
{
	int ret = 0;

	pr_info("[%s] +++ \n", __func__);

	ret = hisp_smc_phy_csi_disconnect();
	if (ret != 0) {
		pr_err("[%s] fail ret.%d!\n", __func__, ret);
		return ret;
	}

	return 0;
}
EXPORT_SYMBOL(hisp_phy_csi_disconnect); /*lint !e508*/

int hisp_csi_mode_sel(unsigned int mode)
{
	int ret = 0;
	if (mode != 0 && mode != 1){
		pr_err("[%s] : mode 0x%x must be 0 or 1!\n", __func__, mode);
		return -EINVAL;
	}
	pr_info("[%s] : hicsi mode.0x%x\n", __func__, mode);

	ret = hisp_smc_csi_mode_sel(mode);
	if (ret != 0) {
		pr_err("[%s] fail ret.%d!\n", __func__, ret);
		return ret;
	}

	return 0;
}
EXPORT_SYMBOL(hisp_csi_mode_sel); /*lint !e508*/

int hisp_i2c_pad_sel(void)
{
	int ret = 0;

	pr_info("[%s] : enter\n", __func__);

	ret = hisp_smc_i2c_pad_type();
	if (ret != 0) {
		pr_err("[%s] fail ret.%d!\n", __func__, ret);
		return ret;
	}

	return 0;
}
EXPORT_SYMBOL(hisp_i2c_pad_sel); /*lint !e508*/

void hisp_lock_sharedbuf(void)
{
	struct hisp_boot_device *dev = &hisp_boot_dev;

	mutex_lock(&dev->sharedbuf_mutex);
}

void hisp_unlock_sharedbuf(void)
{
	struct hisp_boot_device *dev = &hisp_boot_dev;

	mutex_unlock(&dev->sharedbuf_mutex);
}

void hisp_rproc_state_lock(void)
{
	struct hisp_boot_device *dev = &hisp_boot_dev;

	mutex_lock(&dev->rproc_state_mutex);
}

void hisp_rproc_state_unlock(void)
{
	struct hisp_boot_device *dev = &hisp_boot_dev;

	mutex_unlock(&dev->rproc_state_mutex);
}

int hisp_ispcpu_is_powerup(void)
{
	struct hisp_boot_device *dev = &hisp_boot_dev;

	return dev->ispcpu_status;
}

int hisp_log_beta_mode(void)
{
	struct hisp_boot_device *dev = &hisp_boot_dev;

	return dev->use_logb_flag;
}

struct device *hisp_get_device(void)
{
	struct hisp_boot_device *dev = &hisp_boot_dev;

	if (dev->isp_pdev == NULL) {
		pr_err("[%s] Failed: isp_pdev is NULL!\n", __func__);
		return NULL;
	}

	return &dev->isp_pdev->dev;
}

struct hisp_shared_para *hisp_share_get_para(void)
{
	struct hisp_boot_device *dev = &hisp_boot_dev;

	pr_debug("%s: enter.\n", __func__);
	if (dev->share_para != NULL)
		return dev->share_para;

	pr_debug("%s: failed.\n", __func__);
	return NULL;
}

static int hisp_get_ispcpu_logmode(void)
{
	struct device_node *np = NULL;
	int ret = 0, ispcpu_logmode = 0;

	np = of_find_compatible_node(NULL, NULL, "hisilicon,prktimer");
	if (np == NULL) {
		pr_err("%s: no device node 'prktimer'!\n", __func__);
		return -ENXIO;
	}

	ret = of_property_read_u32(np, "fpga_flag", &ispcpu_logmode);/*lint !e64*/
	if (ret) {
		pr_err("%s: failed to get fpga_flag resource.\n", __func__);
		return -ENXIO;
	}

	return ispcpu_logmode;
}

unsigned int hisp_wait_excflag_timeout(unsigned int flag, unsigned int time)
{
	struct hisp_shared_para *param = NULL;
	unsigned int timeout = time;

	pr_debug("[%s] +\n", __func__);
	param = hisp_share_get_para();
	if (param == NULL) {
		pr_err("[%s] param is NULL!\n", __func__);
		return 0;
	}

	if (timeout == 0) {
		pr_err("[%s] err : timeout.%d\n", __func__, timeout);
		return 0;
	}

	do {
		if ((param->exc_flag & flag) == flag)
			break;
		timeout--;
		mdelay(10);
	} while (timeout > 0);

	pr_debug("[%s] exc_flag.0x%x != flag.0x%x, timeout.%d\n",
			__func__, param->exc_flag, flag, timeout);
	pr_debug("[%s] -\n", __func__);
	return timeout;
}

u32 hisp_share_get_exc_flag(void)
{
	struct hisp_shared_para *share_para = NULL;
	u32 exc_flag = 0x0;

	hisp_lock_sharedbuf();
	share_para = hisp_share_get_para();
	if (share_para == NULL) {
		hisp_err("Failed : hisp_share_get_para.%pK\n", share_para);
		hisp_unlock_sharedbuf();
		return ISP_MAX_NUM_MAGIC;
	}
	exc_flag = share_para->exc_flag;
	hisp_unlock_sharedbuf();
	return exc_flag;
}

static int hisp_share_set_plat_parameters(void)
{
	struct hisp_boot_device *dev = &hisp_boot_dev;
	struct hisp_shared_para *param = NULL;

	hisp_lock_sharedbuf();
	param = hisp_share_get_para();
	if (param == NULL) {
		hisp_err("Failed : hisp_share_get_para.%pK\n", param);
		hisp_unlock_sharedbuf();
		return -EINVAL;
	}

	param->plat_cfg.perf_power      = g_perf_data;
	param->plat_cfg.platform_id     = (unsigned int)hw_is_fpga_board();
	param->plat_cfg.isp_local_timer = dev->tmp_plat_cfg.isp_local_timer;
	param->rdr_enable_type         |= ISPCPU_RDR_USE_APCTRL; /*lint !e648*/
	param->logx_switch             |= ISPCPU_LOG_USE_APCTRL;
	param->exc_flag                 = 0x0;
	if (hisp_get_ispcpu_logmode())
		param->logx_switch         |= ISPCPU_LOG_TIMESTAMP_FPGAMOD;

	hisp_info("platform_id = %d, isp_local_timer = %d\n",
		param->plat_cfg.platform_id, param->plat_cfg.isp_local_timer);
	hisp_info("perf_power = %d, logx_switch.0x%x\n",
		param->plat_cfg.perf_power, param->logx_switch);
	hisp_unlock_sharedbuf();

	hisploglevel_update();
	return 0;
}

int hisp_share_set_para(void)
{
	struct hisp_boot_device *dev = &hisp_boot_dev;
	struct hisp_shared_para *share_para = NULL;
	int ret, i;

	ret = hisp_share_set_plat_parameters();
	if (ret) {
		pr_err("%s: hisp_share_set_plat_parameters failed.\n", __func__);
		return ret;
	}

	hisp_lock_sharedbuf();
	share_para = hisp_share_get_para();
	if (share_para == NULL) {
		pr_err("%s:hisp_share_get_para failed.\n", __func__);
		hisp_unlock_sharedbuf();
		return -EINVAL;
	}

	share_para->bootware_paddr = hisp_get_ispcpu_remap_pa();

	if (hisp_rdr_addr_get())
		share_para->rdr_enable = 1;

	share_para->rdr_enable_type |= ISPCPU_DEFAULT_RDR_LEVEL;
	for (i = 0; i < IRQ_NUM; i++)
		share_para->irq[i] = 0;

	share_para->chip_flag = dev->chip_flag;
	pr_info("%s: platform_id = 0x%x, timer = 0x%x\n",
			__func__, share_para->plat_cfg.platform_id,
			share_para->plat_cfg.isp_local_timer);
	pr_info("%s: rdr_enable = %d, rdr_enable_type = %d\n",
			__func__, share_para->rdr_enable,
			share_para->rdr_enable_type);
	pr_info("%s: chip_flag = %u", __func__, share_para->chip_flag);
	hisp_unlock_sharedbuf();

	return ret;
}

void hisp_share_update_clk_value(int type, unsigned int value)
{
	struct hisp_shared_para *share_para = NULL;

	if ((type >= ISP_CLK_MAX) || (type < 0)) {
		pr_err("%s:type error.%d\n", __func__, type);
		return;
	}

	hisp_lock_sharedbuf();
	share_para = hisp_share_get_para();
	if (share_para == NULL) {
		pr_err("%s:hisp_share_get_para failed.\n", __func__);
		hisp_unlock_sharedbuf();
		return;
	}
	share_para->clk_value[type] = value;
	pr_debug("%s: type.%d = %u\n", __func__, type, value);
	hisp_unlock_sharedbuf();
}

static void hisp_share_clk_value_init(void)
{
	struct hisp_shared_para *share_para = NULL;
	int ret = 0;

	hisp_lock_sharedbuf();
	share_para = hisp_share_get_para();
	if (share_para == NULL) {
		pr_err("%s:hisp_share_get_para failed.\n", __func__);
		hisp_unlock_sharedbuf();
		return;
	}
	ret = memset_s(share_para->clk_value, sizeof(share_para->clk_value),
			0, sizeof(share_para->clk_value));
	if (ret != 0)
		pr_err("%s:memset_s share_para->clk_value to 0 failed.ret.%d\n",
				__func__, ret);
	hisp_unlock_sharedbuf();
}

static void hisp_efuse_deskew(void)
{
#ifdef CONFIG_VENDOR_EFUSE
	int ret = 0;
#endif
	unsigned char efuse = 0xFF;
	struct hisp_shared_para *share_para = NULL;
	struct hisp_boot_device *hisp_dev = &hisp_boot_dev;

	hisp_info("+\n");
	if (hisp_dev->probe_finished != 1) {
		hisp_err("isp_rproc_probe failed\n");
		return;
	}

	if (hisp_dev->isp_efuse_flag == 0) {
		hisp_err("isp_efuse_flag.%d\n", hisp_dev->isp_efuse_flag);
		return;
	}

#ifdef CONFIG_VENDOR_EFUSE
	ret = get_efuse_deskew_value(&efuse, 1, 1000);
	if (ret < 0)
		pr_err("[%s] Failed: ret.%d\n", __func__, ret);

	pr_info("[%s] : efuse.%d\n", __func__, ret);
#endif

	hisp_lock_sharedbuf();
	share_para = hisp_share_get_para();
	if (share_para == NULL) {
		pr_err("%s: hisp_share_get_para failed.\n", __func__);
		hisp_unlock_sharedbuf();
		return;
	}
	share_para->isp_efuse = efuse;
	hisp_unlock_sharedbuf();

	hisp_info("-\n");
}

int hisp_nsec_boot_mode(void)
{
	struct hisp_boot_device *dev = &hisp_boot_dev;

	return ((dev->case_type == NONSEC_CASE) ? 1 : 0);
}

int hisp_sec_boot_mode(void)
{
	struct hisp_boot_device *dev = &hisp_boot_dev;

	return ((dev->case_type == SEC_CASE) ? 1 : 0);
}

int hisp_get_last_boot_state(void)
{
	struct hisp_boot_device *dev = &hisp_boot_dev;

	return dev->last_boot_state;
}

void hisp_set_last_boot_state(int state)
{
	struct hisp_boot_device *dev = &hisp_boot_dev;

	dev->last_boot_state = state;
}

int hisp_set_boot_mode(enum hisp_rproc_case_attr type)
{
	struct hisp_boot_device *dev = &hisp_boot_dev;
	int ret;

	if (type >= INVAL_CASE) {
		pr_err("%s: invalid case, type = %u\n", __func__, type);
		return -EINVAL;
	}

	if (atomic_read(&dev->rproc_enable_status) > 0) {
		pr_err("[%s] hisp_rproc had been enabled\n", __func__);
		pr_err("[%s] rproc_enable_status.0x%x\n", __func__,
				atomic_read(&dev->rproc_enable_status));
		return -ENODEV;
	}

	dev->case_type = type;
	dev->share_para = hisp_get_ispcpu_shrmem_va();
	if (dev->share_para == NULL) {
		pr_err("%s: share mem is NULL\n", __func__);
		return -ENOMEM;
	}

	ret = memset_s(dev->share_para, SZ_4K, 0, SZ_4K);
	if (ret != 0)
		pr_err("%s: memset_s isp_share_para to 0 fail.%d!\n",
			__func__, ret);

	if (hisplogcat_sync() < 0)
		pr_err("[%s] Failed: hisplogcat_sync\n", __func__);

	pr_info("%s.%d: type.%u, rporc.%pK\n", __func__,
			__LINE__, type, platform_get_drvdata(dev->isp_pdev));

	return 0;
}

enum hisp_rproc_case_attr hisp_get_boot_mode(void)
{
	struct hisp_boot_device *dev = &hisp_boot_dev;

	return dev->case_type;
}

int hisp_set_clk_rate(unsigned int type, unsigned int rate)
{
	struct hisp_boot_device *dev = &hisp_boot_dev;
	int ret = -EINVAL;

	if (dev->probe_finished != 1) {
		hisp_err("isp_rproc_probe failed\n");
		return -EPERM;
	}

	if (!hisp_dvfs_enable()) {
		pr_err("[%s] Failed : hisp_dvfs_enable\n", __func__);
		return -EINVAL;
	}

	switch (dev->case_type) {
	case NONSEC_CASE:
		ret = hisp_pwr_nsec_setclkrate(type, rate);
		if (ret < 0)
			pr_err("[%s] Failed : hisp_pwr_nsec_setclkrate.%d\n",
				__func__, ret);
		break;
	case SEC_CASE:
		ret = hisp_pwr_sec_setclkrate(type, rate);
		if (ret < 0)
			pr_err("[%s] Failed : hisp_pwr_nsec_setclkrate.%d\n",
				__func__, ret);
		break;
	default:
		pr_err("[%s] Unsupported case_type.%d\n",
				__func__, dev->case_type);
		return -EINVAL;
	}

	return ret;
}

unsigned long hisp_get_clk_rate(unsigned int type)
{
	struct hisp_boot_device *dev = &hisp_boot_dev;

	if (dev->probe_finished != 1) {
		hisp_err("isp_rproc_probe failed\n");
		return 0;
	}

	if (!hisp_rproc_enable_status()) {
		pr_err("[%s] ispcpu not start!\n", __func__);
		return 0;
	}

	return hisp_pwr_getclkrate(type);
}

int hisp_dev_setpinctl(struct pinctrl *isp_pinctrl,
	struct pinctrl_state *pinctrl_def, struct pinctrl_state *pinctrl_idle)
{
	struct hisp_boot_device *hisp_dev = &hisp_boot_dev;

	if (hw_is_fpga_board()) {
		hisp_err("fpga board, no need to init pinctrl.\n");
		return 0;
	}

	hisp_dev->isp_data.isp_pinctrl = isp_pinctrl;
	hisp_dev->isp_data.pinctrl_def = pinctrl_def;
	hisp_dev->isp_data.pinctrl_idle = pinctrl_idle;

	return 0;
}

int hisp_dev_setclkdepend(struct clk *aclk_dss, struct clk *pclk_dss)
{
	struct hisp_boot_device *hisp_dev = &hisp_boot_dev;

	if (hw_is_fpga_board()) {
		hisp_err("fpga board, no need to init clkdepend.\n");
		return 0;
	}

	hisp_dev->isp_data.aclk_dss = aclk_dss;
	hisp_dev->isp_data.pclk_dss = pclk_dss;

	return 0;
}

u64 hisp_get_ispcpu_shrmem_pa(void)
{
	if (hisp_nsec_boot_mode())
		return hisp_get_ispcpu_shrmem_nsecpa();

	return hisp_get_ispcpu_shrmem_secpa();
}

void *hisp_get_ispcpu_shrmem_va(void)
{
	if (hisp_nsec_boot_mode())
		return hisp_get_ispcpu_shrmem_nsecva();

	return hisp_get_ispcpu_shrmem_secva();
}

u64 hisp_get_ispcpu_remap_pa(void)
{
	if (hisp_nsec_boot_mode())
		return hisp_nsec_get_remap_pa();

	return hisp_sec_get_remap_pa();
}

static void hisp_set_ispcpu_idle(void)
{
	pr_debug("[%s] +\n", __func__);
	if (hisp_wait_excflag_timeout(ISP_CPU_POWER_DOWN, 10) == 0x0)
		hisp_send_fiq2ispcpu();
	pr_info("[%s] timeout.%d!\n", __func__,
		hisp_wait_excflag_timeout(ISP_CPU_POWER_DOWN, 300));
	pr_debug("[%s] -\n", __func__);
}

static int hisp_get_ispcpu_idle_stat(void)
{
	struct hisp_boot_device *dev = &hisp_boot_dev;
	int ret = 0;

	hisp_set_ispcpu_idle();

	if (dev->isppd_adb_flag) {
		ret = hisp_smc_get_ispcpu_idle();
		if (ret != 0)
			pr_alert("[%s] Failed : ispcpu not in idle.%d!\n",
					__func__, ret);
	}

	return ret;
}

int hisp_get_ispcpu_cfginfo(void)
{
#ifdef DEBUG_HISP
	int ret = 0;
	struct hisp_boot_device *dev = &hisp_boot_dev;

	if (dev->isp_wdt_flag)
		hisp_send_fiq2ispcpu();

	ret = hisp_mntn_dumpregs();
	if (ret < 0) {
		pr_err("[%s] Failed : ret.%d\n", __func__, ret);
		return ret;
	}
#endif
	return 0;
}

static void hisp_device_disable_prepare(void)
{
	int ret = -1;
	struct hisp_boot_device *dev = &hisp_boot_dev;
	struct rproc *rproc = platform_get_drvdata(dev->isp_pdev);

	ret = hisp_mntn_dumpregs();
	if (ret < 0)
		hisp_err("Failed : hisp_mntn_dumpregs.%d\n", ret);
	if (hisp_get_ispcpu_idle_stat() < 0) {
		if (dev->isppd_adb_flag)
			hisp_nsec_boot_dump(rproc, DUMP_ISP_BOOT_SIZE);
	}
	hisplogcat_stop();
}

static int hisp_device_disable(void)
{
	struct hisp_boot_device *dev = &hisp_boot_dev;
	int ret;

	hisp_info("+\n");
	if (dev->ispcpu_status == 0) {
		pr_err("%s: isp already off\n", __func__);
		return 0;
	}
	switch (dev->case_type) {
	case SEC_CASE:
		hisp_device_disable_prepare();
		ret = hisp_sec_device_disable();
		if (ret < 0)
			hisp_err("Failed : hisp_sec_device_disable.%d\n", ret);
		break;

	case NONSEC_CASE:
		hisp_device_disable_prepare();
		ret = hisp_nsec_device_disable();
		if (ret < 0)
			hisp_err("Failed:hisp_nsec_device_disable.%d\n", ret);
		break;

	default:
		hisp_err("Failed : not supported type.%d\n", dev->case_type);
		return -EINVAL;
	}
	dev->ispcpu_status = 0;
	if (hisplogcat_sync() < 0)
		pr_err("[%s] Failed: hisplogcat_sync\n", __func__);
	hisplog_clear_info();
	if (ret != 0)
		hisp_err("ispcpu power down fail.%d, case_type.%d\n",
				ret, dev->case_type);
	mutex_lock(&dev->ispcpu_mutex);
	if (dev->ispcpu_wakelock->active) {
		__pm_relax(dev->ispcpu_wakelock);
		hisp_info("ispcpu power up wake unlock.\n");
	}
	mutex_unlock(&dev->ispcpu_mutex);/*lint !e456 */
	hisp_info("-\n");

	return ret;
}

static int hisp_device_enable(void)
{
#define TIMEOUT_ISPLOG_START (10)
	struct hisp_boot_device *dev = &hisp_boot_dev;
	struct rproc *rproc = platform_get_drvdata(dev->isp_pdev);
	int ret = -ENOMEM, timeout = 0;

	hisp_info("+\n");
	dev->last_boot_state = 0;
	mutex_lock(&dev->ispcpu_mutex);
	if (!dev->ispcpu_wakelock->active) {
		__pm_stay_awake(dev->ispcpu_wakelock);
		hisp_info("ispcpu power up wake lock.\n");
	}
	mutex_unlock(&dev->ispcpu_mutex);/*lint !e456 */
	timeout = TIMEOUT_ISPLOG_START;
	do {
		ret = hisplogcat_start();
		if (ret < 0)
			hisp_err("Failed:hisplogcat_start.%d,timeout.%d\n",
					ret, timeout);
	} while (ret < 0 && timeout-- > 0);

	switch (dev->case_type) {
	case SEC_CASE:
		ret = hisp_sec_device_enable();
		if (ret < 0)
			hisp_err("Failed : secisp_enable.%d\n", ret);
		break;
	case NONSEC_CASE:
		ret = hisp_nsec_device_enable(rproc);
		if (ret < 0)
			hisp_err("Failed : nonsec_isp_enable.%d\n", ret);
		break;
	default:
		hisp_err("Failed : not supported type.%d, ret.%d\n",
			dev->case_type, ret);
		break;
	}
	dev->ispcpu_status = 1;
	hisp_info("-\n");

	if (ret != 0) {
		hisp_err("ispcpu power up fail.%d, case_type.%d\n",
				ret, dev->case_type);
		mutex_lock(&dev->ispcpu_mutex);
		if (dev->ispcpu_wakelock->active) {
			__pm_relax(dev->ispcpu_wakelock);
			hisp_info("ispcpu power up wake unlock.\n");
		}
		mutex_unlock(&dev->ispcpu_mutex);/*lint !e456 */
	}
	return ret;
}

static int hisp_power_on_work(void *data)
{
	struct hisp_boot_device *dev = &hisp_boot_dev;
	struct cpumask cpu_mask;
	int ret;

	hisp_info("+\n");
	set_user_nice(current, (int)(-1 * (dev->pwronisp_nice)));

	get_slow_cpus(&cpu_mask);
	cpumask_clear_cpu(0, &cpu_mask);

	if (sched_setaffinity(current->pid, &cpu_mask) < 0)
		hisp_err("Couldn't set affinity to cpu\n");

	ret = hisp_sec_pwron_prepare();
	if (ret != 0) {
		hisp_err("hisp_sec_pwron_prepare fail.%d\n", ret);
		return ret;
	}

	hisp_info("-\n");
	while (!kthread_should_stop()) {
		ret = wait_event_interruptible(dev->pwronisp_wait,
			dev->pwronisp_wake);
		if (ret < 0) {
			hisp_info("fail to wait for pwronisp_wake.%ld\n", ret);
			continue;
		}

		ret = hisp_device_enable();
		if (ret != 0)
			hisp_err("hisp_device_enable fail.%d\n", ret);

		dev->pwronisp_wake = 0;
	}

	return 0;
}

int hisp_wait_rpmsg_completion(struct rproc *rproc)
{
	struct hisp_boot_device *hisp_dev = &hisp_boot_dev;
	unsigned int timeout = hw_is_fpga_board() ?
		TIMEOUT_IS_FPGA_BOARD : TIMEOUT_IS_NOT_FPGA_BOARD;
	unsigned int ret;

	ret = wait_for_completion_timeout(&hisp_dev->rpmsg_boot_comp,
		msecs_to_jiffies(timeout));
	if (ret == 0) {
		pr_err("Failed : wait_for_completion_timeout\n");
		return -ETIME;
	}

	hisp_rproc_set_rpmsg_ready(rproc);

	return 0;
}

int hisp_device_start(void)
{
	struct hisp_boot_device *hisp_dev = &hisp_boot_dev;
	int ret;

	hisp_dev->pwronisp_wake = 1;
	wake_up(&hisp_dev->pwronisp_wait);

	/* set shared parameters for rproc*/
	ret = hisp_share_set_para();
	if (ret != 0) {
		hisp_err("Failed to set bootware parameters...: %d\n", ret);
		return ret;
	}

	ret = hisp_nsec_set_pgd();
	if (ret != 0) {
		hisp_err("Failed : hisp_nsec_set_pgd.%d\n", ret);
		return ret;
	}

	hisp_efuse_deskew();
	hisp_share_clk_value_init();

	return 0;
}
EXPORT_SYMBOL(hisp_device_start);

int hisp_device_stop(void)
{
	int ret;

	ret = hisp_device_disable();
	if (ret != 0) {
		hisp_err("Failed : hisp_device_disable.%d\n", ret);
		return ret;
	}

	return 0;
}
EXPORT_SYMBOL(hisp_device_stop);

int hisp_device_attach(struct rproc *rproc)
{
	int ret;

	if (hisp_nsec_bin_mode() && (rproc->state == RPROC_DETACHED)) {
		ret = hisp_loadbin_segments(rproc);
		if (ret != 0) {
			hisp_err("hisp_loadbin_segments fail.%d\n", ret);
			return ret;
		}
	}

	return hisp_device_start();
}
EXPORT_SYMBOL(hisp_device_attach);

int hisp_rproc_enable_status(void)
{
	struct hisp_boot_device *hisp_dev = &hisp_boot_dev;

	return atomic_read(&hisp_dev->rproc_enable_status);
}

static int hisp_rproc_down_check(void)
{
	int err = 0;
	struct hisp_boot_device *hisp_dev = &hisp_boot_dev;

	if (hisp_dev->probe_finished != 1) {
		hisp_err("isp_rproc_probe failed\n");
		return -EPERM;
	}

	err = hisp_bypass_power_updn();
	if (err != 0) {/*lint !e838 */
		pr_err("[%s] hisp_bypass_power_updn.0x%x\n", __func__, err);
		return -ENODEV;
	}

	if (atomic_read(&hisp_dev->rproc_enable_status) == 0) {
		pr_err("[%s] Failed : rproc_enable_status.0\n",	__func__);
		return -ENODEV;
	}
	mutex_lock(&hisp_dev->hisp_power_mutex);
	if (hisp_dev->hisp_power_state == HISP_ISP_POWER_OFF) {
		pr_err("[%s] hisp_rproc disable no power.0x%x\n",
			__func__, hisp_dev->hisp_power_state);
		mutex_unlock(&hisp_dev->hisp_power_mutex);
		return -ENODEV;
	}
	mutex_unlock(&hisp_dev->hisp_power_mutex);

	return 0;
}

int hisp_rproc_disable(void)
{
	struct hisp_boot_device *hisp_dev = &hisp_boot_dev;
	struct rproc *rproc = platform_get_drvdata(hisp_dev->isp_pdev);
	int err = 0;

	hisp_info("+\n");

#ifdef DEBUG_HISP
	err = hisp_check_pcie_stat();
	if (err == ISP_PCIE_NREADY)
		return err;
#endif

	err = hisp_rproc_down_check();
	if (err < 0) {
		pr_err("[%s] Failed:hisp_rproc_down_check.%d\n", __func__, err);
		return err;
	}

	mutex_lock(&hisp_dev->hisp_power_mutex);
	hisp_dev->hisp_power_state = HISP_ISP_POWER_OFF;
	mutex_unlock(&hisp_dev->hisp_power_mutex);

	hisp_rproc_state_lock();
	rproc_shutdown(rproc);
	hisp_rproc_state_unlock();

	hisp_dynamic_mem_pool_clean();
	hisp_rproc_clr_domain(rproc);
	hisp_dev->share_para = NULL;

	if (atomic_read(&hisp_dev->rproc_enable_status) == 1)
		atomic_set(&hisp_dev->rproc_enable_status, 0);

	hisp_info("-\n");
	return err;
}
EXPORT_SYMBOL(hisp_rproc_disable);

static int hisp_rproc_up_check(void)
{
	int err;
	struct hisp_boot_device *hisp_dev = &hisp_boot_dev;

	if (hisp_dev->probe_finished != 1) {
		hisp_err("isp_rproc_probe failed\n");
		return -EPERM;
	}

	err = hisp_bypass_power_updn();
	if (err != 0) {/*lint !e838 */
		pr_err("[%s] hisp_bypass_power_updn.0x%x\n", __func__, err);
		return -ENODEV;
	}

	pr_info("%s.%d: case type.%u\n", __func__, __LINE__,
			hisp_get_boot_mode());

	if (atomic_read(&hisp_dev->rproc_enable_status) > 0) {
		pr_err("[%s] hisp_rproc had been enabled\n", __func__);
		pr_err("[%s] rproc_enable_status.0x%x\n", __func__,
			atomic_read(&hisp_dev->rproc_enable_status));
		return -ENODEV;
	}

	return 0;
}

static int hisp_wakeup_pwronisp_kthread(void)
{
	struct hisp_boot_device *hisp_dev = &hisp_boot_dev;

	if (hisp_dev->pwronisp_thread_status == 1)
		return 0;

	if (hisp_dev->pwronisp == NULL) {
		hisp_err("Failed : pwronisp is NULL\n");
		return -ENODEV;
	}

	wake_up_process(hisp_dev->pwronisp);
	hisp_dev->pwronisp_thread_status = 1;

	return 0;
}

static int hisp_rproc_boot_prepare(struct rproc *rproc)
{
	struct hisp_boot_device *hisp_dev = &hisp_boot_dev;
	struct iommu_domain *domain = NULL;
	int ret;

	if (hisp_sec_boot_mode()) {
		ret = hisp_request_sec_rsctable(rproc);
		if (ret < 0) {
			pr_err("[%s] Failed : hisp_request_sec_rsctable.%d\n",
					__func__, ret);
			return ret;
		}

		rproc->state = RPROC_DETACHED;
	} else if (hisp_nsec_bin_mode()) {
		domain = iommu_get_domain_for_dev(&hisp_dev->isp_pdev->dev);
		ret = hisp_rproc_set_domain(rproc, domain);
		if (ret != 0)
			return -EINVAL;

		ret = hisp_request_nsec_rsctable(rproc);
		if (ret < 0) {
			pr_err("[%s] Failed : hisp_request_nsec_rsctable.%d\n",
					__func__, ret);
			return ret;
		}

		rproc->state = RPROC_DETACHED;
	} else {
		domain = iommu_get_domain_for_dev(&hisp_dev->isp_pdev->dev);
		ret = hisp_rproc_set_domain(rproc, domain);
		if (ret != 0)
			return -EINVAL;

		rproc->state = RPROC_OFFLINE;
	}

	return 0;
}

static int hisp_rproc_boot_config(void)
{
	struct hisp_boot_device *hisp_dev = &hisp_boot_dev;
	struct rproc *rproc = platform_get_drvdata(hisp_dev->isp_pdev);
	int err = 0;

	hisp_info("+\n");

	err = hisp_rproc_up_check();
	if (err < 0) {
		pr_err("[%s] Failed:hisp_rproc_up_check.%d\n", __func__, err);
		return err;
	}

	atomic_set(&hisp_dev->rproc_enable_status, 1);

	err = hisp_wakeup_binload_kthread();
	if (err != 0) {
		hisp_err("Failed : hisp_wakeup_binload_kthread.0x%x\n", err);
		atomic_set(&hisp_dev->rproc_enable_status, 0);
		return -ENODEV;
	}

	err = hisp_wakeup_pwronisp_kthread();
	if (err != 0) {
		hisp_err("Failed : hisp_wakeup_pwronisp_kthread.0x%x\n", err);
		atomic_set(&hisp_dev->rproc_enable_status, 0);
		return -ENODEV;
	}

	err = hisp_rproc_boot_prepare(rproc);
	if (err < 0) {
		atomic_set(&hisp_dev->rproc_enable_status, 0);
		return err;
	}

	err = hisp_loadbin_debug_elf(rproc);
	if (err < 0) {
		atomic_set(&hisp_dev->rproc_enable_status, 0);
		return err;
	}

	reinit_completion(&hisp_dev->rpmsg_boot_comp);

	/* clean up the invalid exception entries */
	if (!list_empty(&rproc->rvdevs)) {
		hisp_err("Failed : enable exception will disable...\n");
		atomic_set(&hisp_dev->rproc_enable_status, 2);
		mutex_lock(&hisp_dev->hisp_power_mutex);
		hisp_dev->hisp_power_state = HISP_ISP_POWER_CLEAN;
		mutex_unlock(&hisp_dev->hisp_power_mutex);
		hisp_rproc_disable();
	}

	return 0;
}

int hisp_rproc_enable(void)
{
	struct hisp_boot_device *hisp_dev = &hisp_boot_dev;
	struct rproc *rproc = platform_get_drvdata(hisp_dev->isp_pdev);
	int err = 0;

#ifdef DEBUG_HISP
	err = hisp_check_pcie_stat();
	if (err == ISP_PCIE_NREADY)
		return err;
#endif

	err = hisp_rproc_boot_config();
	if (err < 0) {
		hisp_err("[%s]Failed:hisp_rproc_boot_config.%d\n", __func__, err);
		return err;
	}

	hisp_info("rproc_enable...\n");

	err = rproc_boot(rproc);
	if (err) {
		hisp_err("Failed : rproc_boot.%d\n", err);
		goto enable_err;
	}

#ifndef CONFIG_HISP_VQUEUE_DMAALLOC_DEBUG
	err = hisp_rproc_vqmem_init(rproc);
	if (err) {
		hisp_err("Failed : hisp_rproc_vqmem_init.%d\n", err);
		goto enable_err;
	}
#endif
	complete(&hisp_dev->rpmsg_boot_comp);
	mutex_lock(&hisp_dev->hisp_power_mutex);
	hisp_dev->hisp_power_state = HISP_ISP_POWER_ON;
	mutex_unlock(&hisp_dev->hisp_power_mutex);

	hisp_info("-\n");
	return 0;

enable_err:
	mutex_lock(&hisp_dev->hisp_power_mutex);
	hisp_dev->hisp_power_state = HISP_ISP_POWER_FAILE;
	mutex_unlock(&hisp_dev->hisp_power_mutex);
	hisp_rproc_disable();

	return err;
}
EXPORT_SYMBOL(hisp_rproc_enable);

int hisp_jpeg_powerup(void)
{
	struct hisp_boot_device *dev = &hisp_boot_dev;
	int ret = 0;

	if (dev->probe_finished != 1) {
		hisp_err("isp_rproc_probe failed\n");
		return -EPERM;
	}

	mutex_lock(&dev->jpeg_mutex);
	if (!dev->jpeg_wakelock->active) {
		__pm_stay_awake(dev->jpeg_wakelock);
		hisp_info("jpeg power up wake lock.\n");
	}
	mutex_unlock(&dev->jpeg_mutex);/*lint !e456 */
	switch (dev->case_type) {
	case NONSEC_CASE:
		ret = hisp_nsec_jpeg_powerup();
		break;
	case SEC_CASE:
		ret = hisp_sec_jpeg_powerup();
		break;
	default:
		ret = -EINVAL;
		break;
	}
	if (ret != 0) {
		hisp_err("Failed : jpeg power up fail.%d, case_type.%d\n",
				ret, dev->case_type);
		mutex_lock(&dev->jpeg_mutex);
		if (dev->jpeg_wakelock->active) {
			__pm_relax(dev->jpeg_wakelock);
			hisp_info("jpeg power up wake unlock.\n");
		}
		mutex_unlock(&dev->jpeg_mutex);/*lint !e456 */
	}
	return ret;
}
EXPORT_SYMBOL(hisp_jpeg_powerup);

int hisp_jpeg_powerdn(void)
{
	struct hisp_boot_device *dev = &hisp_boot_dev;
	int ret = 0;

	if (dev->probe_finished != 1) {
		hisp_err("isp_rproc_probe failed\n");
		return -EPERM;
	}

	switch (dev->case_type) {
	case NONSEC_CASE:
		ret = hisp_nsec_jpeg_powerdn();
		break;
	case SEC_CASE:
		ret = hisp_sec_jpeg_powerdn();
		break;
	default:
		ret = -EINVAL;
		break;
	}
	mutex_lock(&dev->jpeg_mutex);
	if (dev->jpeg_wakelock->active) {
		__pm_relax(dev->jpeg_wakelock);
		hisp_info("jpeg power up wake unlock.\n");
	}
	mutex_unlock(&dev->jpeg_mutex);/*lint !e456 */
	if (ret != 0)
		hisp_err("Failed : jpeg power down fail.%d, case_type.%d\n",
				ret, dev->case_type);
	return ret;
}
EXPORT_SYMBOL(hisp_jpeg_powerdn);

int hisp_dev_select_def(void)
{
	struct hisp_boot_device *hisp_dev = &hisp_boot_dev;
	int ret;

	hisp_info("+\n");
	if (hisp_dev->probe_finished != 1) {
		hisp_err("isp_rproc_probe failed\n");
		return -EPERM;
	}

	if (hw_is_fpga_board() == 0) {
		ret = pinctrl_select_state(hisp_dev->isp_data.isp_pinctrl,
					hisp_dev->isp_data.pinctrl_def);
		if (ret != 0) {
			hisp_err("Failed : not set pins to default state.\n");
			return ret;
		}
	}

	hisp_info("-\n");
	return 0;
}
EXPORT_SYMBOL(hisp_dev_select_def);

int hisp_dev_select_idle(void)
{
	struct hisp_boot_device *hisp_dev = &hisp_boot_dev;
	int ret;

	hisp_info("+\n");
	if (hisp_dev->probe_finished != 1) {
		hisp_err("isp_rproc_probe failed\n");
		return -EPERM;
	}

	if (hw_is_fpga_board() == 0) {
		ret = pinctrl_select_state(hisp_dev->isp_data.isp_pinctrl,
					hisp_dev->isp_data.pinctrl_idle);
		if (ret != 0) {
			hisp_err("Failed : not set pins to ilde state.\n");
			return ret;
		}
	}

	hisp_info("-\n");
	return 0;
}
EXPORT_SYMBOL(hisp_dev_select_idle);

int hisp_bypass_power_updn(void)
{
	struct hisp_boot_device *dev = &hisp_boot_dev;

	return dev->bypass_pwr_updn;
}

void __iomem *hisp_dev_get_regaddr(unsigned int type)
{
	struct hisp_boot_device *dev = &hisp_boot_dev;

	if (type >= hisp_min(ISP_MAX_REGTYPE, dev->reg_num)) {
		pr_err("[%s] unsupported type.0x%x\n", __func__, type);
		return NULL;
	}

	return dev->reg[type] ? dev->reg[type]:NULL; /*lint !e661 !e662 */
}

static int hisp_pwron_task_init(struct hisp_boot_device *hisp_dev)
{
	if (hisp_dev == NULL) {
		pr_err("[%s] Failed : hisp_dev.%pK\n", __func__, hisp_dev);
		return -ENOMEM;
	}

	hisp_dev->pwronisp = kthread_create(hisp_power_on_work,
				NULL, "pwronisp");
	if (IS_ERR(hisp_dev->pwronisp)) {
		pr_err("[%s] Failed : kthread_create.%ld\n",
			__func__, PTR_ERR(hisp_dev->pwronisp));
		return -EINVAL;
	}

	return 0;
}

static void hisp_pwron_task_deinit(struct hisp_boot_device *hisp_dev)
{
	if (hisp_dev == NULL) {
		pr_err("[%s] Failed : hisp_dev.%pK\n", __func__, hisp_dev);
		return;
	}

	if (hisp_dev->pwronisp != NULL) {
		kthread_stop(hisp_dev->pwronisp);
		hisp_dev->pwronisp = NULL;
	}
}

static unsigned int hisp_get_isplogic(void)
{
	struct device_node *np = NULL;
	char *name = NULL;
	unsigned int isplogic = 1;
	int ret = 0;

	name = DTS_COMP_LOGIC_NAME;

	np = of_find_compatible_node(NULL, NULL, name);
	if (np == NULL) {
		pr_err("%s: of_find_compatible_node failed, %s\n",
				__func__, name);
		return ISP_UDP;
	}

	ret = of_property_read_u32(np, "vendor,isplogic",
			(unsigned int *)(&isplogic));
	if (ret < 0) {
		pr_err("[%s] Failed: isplogic of_property_read_u32.%d\n",
				__func__, ret);
		return ISP_FPGA_EXC;
	}

	pr_info("[%s] isplogic.0x%x\n", __func__, isplogic);
	return isplogic;
}

static unsigned int hisp_get_boardid(struct hisp_boot_device *dev)
{
	unsigned int boardid[BOARDID_SIZE] = {0};
	struct device_node *root = NULL;
	unsigned int stat10 = 0;
	unsigned int isplogic_type = 0;
	int ret = 0;
	const char *str = "ISP/Camera Power Up and Down May be Bypassed";

	dev->bypass_pwr_updn = 0;
	root = of_find_node_by_path("/");
	if (root == NULL) /*lint !e838 */
		goto err_get_bid;

	ret = of_property_read_u32_array(root, "hisi,boardid",
				boardid, BOARDID_SIZE);
	if (ret < 0)
		goto err_get_bid;

	pr_info("Board ID.(%x %x %x %x)\n",
			boardid[0], boardid[1], boardid[2], boardid[3]);

	isplogic_type = hisp_get_isplogic();
	switch (isplogic_type) {
	case ISP_FPGA_EXCLUDE:
		pr_err(" %s isplogic state.%d\n", str, isplogic_type);
		dev->bypass_pwr_updn = 1;
		break;
	case ISP_FPGA:
#ifdef DEBUG_HISP
		ret = hisp_check_pcie_stat();
		if (ret != ISP_NORMAL_MODE)
			break;
#endif
		if (dev->reg[ISP_PCTRL] == NULL) {/*lint !e747  */
			pr_err("Failed : pctrl_base ioremap.\n");
			break;
		}

		stat10 = readl(dev->reg[ISP_PCTRL] +
				ISP_PCTRL_PERI_STAT_ADDR);/*lint !e732 !e529 */
		pr_alert("ISP/Camera pctrl_stat10.0x%x\n", stat10);
		if ((stat10 & (ISP_PCTRL_PERI_FLAG)) == 0) {
			pr_err("%s, pctrl_stat10.0x%x\n", str, stat10);
			dev->bypass_pwr_updn = 1;
		}
		break;
	case ISP_UDP:
	case ISP_FPGA_EXC:
		break;
	default:
		pr_err("ERROR: isplogic state.%d\n", isplogic_type);
		break;
	}
	return boardid[0];

err_get_bid:
	pr_err("Failed : find_node.%pK, property_read.%d\n", root, ret);
	return INVALID_BOARDID;/*lint !e570 */
}

static int hisp_dev_plat_getdts(struct device *pdev,
			struct hisp_boot_device *dev)
{
	struct device_node *np = pdev->of_node;
	unsigned int platform_info;
	int ret;

	ret = of_property_read_u32(np, "isp_local_timer", &platform_info);
	if (ret < 0) {
		pr_err("[%s] Failed: isp_local_timer.0x%x ret.%d\n",
			__func__, platform_info, ret);
		return -EINVAL;
	}
	dev->tmp_plat_cfg.isp_local_timer = platform_info;
	pr_info("[%s] isp_local_timer = %d\n", __func__, platform_info);

	return 0;
}


static int hisp_dev_flag_getdts(struct device_node *np,
			struct hisp_boot_device *dev)
{
	int ret;

	ret = of_property_read_u32(np, "isppd-adb-flag", &dev->isppd_adb_flag);
	if (ret < 0) {
		pr_err("Failed: adb-flag.0x%x ret.%d\n",
			dev->isppd_adb_flag, ret);
		return -EINVAL;
	}
	pr_info("isppd_adb_flag.%d\n", dev->isppd_adb_flag);

	ret = of_property_read_u32(np, "isp-efuse-flag", &dev->isp_efuse_flag);
	if (ret < 0) {
		pr_err("Failed: efuse-flag.%d ret.%d\n", dev->isp_efuse_flag, ret);
		return -EINVAL;
	}
	pr_info("isp_efuse_flag.%d\n", dev->isp_efuse_flag);

	ret = of_property_read_u32(np, "logb-flag", &dev->use_logb_flag);
	if (ret < 0) {
		pr_err("Failed: logb-flag.0x%x ret.%d\n",
			dev->use_logb_flag, ret);
		return -EINVAL;
	}
	pr_info("use_logb_flag.%d\n", dev->use_logb_flag);

	ret = of_property_read_u32(np, "isp-wdt-flag", &dev->isp_wdt_flag);
	if (ret < 0) {
		pr_err("[%s] Failed: isp-wdt-flag.ret.%d\n", __func__, ret);
		return -EINVAL;
	}
	pr_info("isp-wdt-flag.0x%x\n", dev->isp_wdt_flag);

	ret = of_property_read_u32(np, "pwronisp-nice", &dev->pwronisp_nice);
	if (ret < 0) {
		pr_err("[%s] Failed: pwronisp-nice.%d\n", __func__, ret);
		return -EINVAL;
	}
	pr_info("pwronisp-nice.0x%x of_property_read_u32.%d\n",
			dev->pwronisp_nice, ret);

	ret = of_property_read_u32(np, "chip-flag", &dev->chip_flag);
	if (ret < 0) {
		dev->chip_flag = 0;
		pr_info("[%s]: not supported board flag\n", __func__);
	}

	return 0;
}

static int hisp_dev_regaddr_getdts(struct platform_device *pdev,
		struct hisp_boot_device *rproc_dev)
{
	struct device *dev = NULL;
	struct device_node *np = NULL;
	int ret = 0;
	unsigned int i;
	unsigned int min_num;

	if (pdev == NULL)
		return -ENXIO;

	dev = &pdev->dev;
	if (dev == NULL) {
		pr_err("[%s] Failed : No device\n", __func__);
		return -ENXIO;
	}

	np = dev->of_node;
	if (np == NULL) {
		pr_err("[%s] Failed : No dt node\n", __func__);
		return -ENXIO;
	}

	ret = of_property_read_u32(np, "reg-num",
		(unsigned int *)(&rproc_dev->reg_num));
	if (ret < 0) {
		pr_err("[%s] Failed: reg-num,ret.%d\n", __func__, ret);
		return -EINVAL;
	}

	min_num = hisp_min(ISP_MAX_REGTYPE, rproc_dev->reg_num);

	for (i = 0; i < min_num; i++) {
		rproc_dev->r[i] = platform_get_resource(pdev,
					IORESOURCE_MEM, i);
		if (rproc_dev->r[i] == NULL) {
			pr_err("[%s] Failed : platform_get_resource.%pK\n",
					__func__, rproc_dev->r[i]);
			goto error;
		}
		if ((rproc_dev->r[i]->start == 0) ||
			(resource_size(rproc_dev->r[i]) == 0)) {
			rproc_dev->reg[i] = NULL;
			continue;
		}
		rproc_dev->reg[i] = ioremap(rproc_dev->r[i]->start,
			resource_size(rproc_dev->r[i]));//lint !e442
		if (rproc_dev->reg[i] == NULL) {
			pr_err("[%s] resource.%d ioremap fail\n", __func__, i);
			goto error;
		}
	}

	return 0;

error:
	for (; i < min_num; i--) {//lint !e442
		if (rproc_dev->reg[i] != NULL) {
			iounmap(rproc_dev->reg[i]);
			rproc_dev->reg[i] = NULL;
		}
	}
	return -ENOMEM;
}

static void hisp_dev_regaddr_putdts(struct hisp_boot_device *rproc_dev)
{
	unsigned int i;
	unsigned int min_num;

	pr_info("+\n");
	min_num = hisp_min(ISP_MAX_REGTYPE, rproc_dev->reg_num);
	for (i = 0; i < min_num; i++) {
		if (rproc_dev->reg[i]) {
			iounmap(rproc_dev->reg[i]);
			rproc_dev->reg[i] = NULL;
		}
	}
	pr_info("-\n");
}

static int hisp_phy_csi_getdts(struct device *pdev,
		struct hisp_boot_device *dev)
{
	struct device_node *np = pdev->of_node;
	int ret = 0;

	if (np == NULL) {
		pr_err("[%s] Failed : np.%pK\n", __func__, np);
		return -ENODEV;
	}

	ret = of_property_read_u32_array(np, "phy-csi-num",
		dev->phy_csi_num, PHY_CSI_MAX_TYPE);
	if (ret < 0)
		pr_err("[%s] Failed: phy_csi_max get from dtsi.%d\n",
				__func__, ret);

	return 0;
}

static int hisp_dev_getdts(struct device *pdev,
			struct hisp_boot_device *dev)
{
	struct device_node *np = pdev->of_node;
	int ret;

	if (np == NULL) {
		pr_err("Failed : No dt node\n");
		return -EINVAL;
	}

	dev->boardid = hisp_get_boardid(dev);
	if (dev->boardid == INVALID_BOARDID)/*lint !e650 */
		return -EINVAL;


	ret = hisp_dev_plat_getdts(pdev, dev);
	if (ret < 0) {
		pr_err("Failed : hisp_dev_plat_getdt ret.%d\n", ret);
		return ret;
	}

	ret = hisp_dev_flag_getdts(np, dev);
	if (ret < 0) {
		pr_err("Failed : hisp_dev_flag_getdts ret.%d\n", ret);
		return ret;
	}

	ret = hisp_phy_csi_getdts(pdev, dev);
	if (ret < 0) {
		pr_err("[%s] Failed: hisp_phy_csi_getdts.%d\n", __func__, ret);
		return ret;
	}

	return 0;
}

static int hisp_device_init(struct platform_device *pdev,
	struct hisp_boot_device *hisp_dev)
{
	int ret = 0;

	if ((hisp_dev == NULL) || (pdev == NULL)) {
		pr_err("[%s] Failed : hisp_dev.%pK or pdev.%pK\n",
				__func__, hisp_dev, pdev);
		return -ENOMEM;
	}

	hisp_dev->probe_finished  = 0;
	hisp_dev->isp_pdev = pdev;
	hisp_dev->pwronisp_wake = 0;
	hisp_dev->pwronisp_thread_status = 0;
	mutex_init(&hisp_dev->sharedbuf_mutex);
	mutex_init(&hisp_dev->jpeg_mutex);
	mutex_init(&hisp_dev->ispcpu_mutex);
	mutex_init(&hisp_dev->hisp_power_mutex);
	mutex_init(&hisp_dev->rproc_state_mutex);

	hisp_dev->jpeg_wakelock = wakeup_source_register(&pdev->dev, "jpeg_wakelock");
	if (!hisp_dev->jpeg_wakelock) {
		hisp_err("Failed: jpeg_wakelock register fail\n");
		ret = -ENOMEM;
		goto jpeg_wakelock_fail;
	}

	hisp_dev->ispcpu_wakelock = wakeup_source_register(&pdev->dev, "ispcpu_wakelock");
	if (!hisp_dev->ispcpu_wakelock) {
		hisp_err("Failed: ispcpu_wakelock register fail\n");
		ret = -ENOMEM;
		goto ispcpu_wakelock_fail;
	}
	init_waitqueue_head(&hisp_dev->pwronisp_wait);
	init_completion(&hisp_dev->rpmsg_boot_comp);
	mutex_lock(&hisp_dev->hisp_power_mutex);
	hisp_dev->hisp_power_state = HISP_ISP_POWER_OFF;
	mutex_unlock(&hisp_dev->hisp_power_mutex);
	atomic_set(&hisp_dev->rproc_enable_status, 0);

	ret = hisp_pwron_task_init(hisp_dev);
	if (ret < 0) {
		hisp_err("Failed: hisp_pwron_task_init.%d\n", ret);
		goto hisp_pwron_task_init_fail;
	}

	return 0;

hisp_pwron_task_init_fail:
	wakeup_source_unregister(hisp_dev->ispcpu_wakelock);
	hisp_dev->ispcpu_wakelock = NULL;

ispcpu_wakelock_fail:
	wakeup_source_unregister(hisp_dev->jpeg_wakelock);
	hisp_dev->jpeg_wakelock = NULL;

jpeg_wakelock_fail:
	mutex_destroy(&hisp_dev->rproc_state_mutex);
	mutex_destroy(&hisp_dev->hisp_power_mutex);
	mutex_destroy(&hisp_dev->ispcpu_mutex);
	mutex_destroy(&hisp_dev->jpeg_mutex);
	mutex_destroy(&hisp_dev->sharedbuf_mutex);
	hisp_dev->isp_pdev = NULL;

	return ret;
}
static void hisp_device_deinit(void)
{
	struct hisp_boot_device *hisp_dev = &hisp_boot_dev;

	hisp_pwron_task_deinit(hisp_dev);

	wakeup_source_unregister(hisp_dev->ispcpu_wakelock);
	hisp_dev->ispcpu_wakelock = NULL;
	wakeup_source_unregister(hisp_dev->jpeg_wakelock);
	hisp_dev->jpeg_wakelock = NULL;
	mutex_destroy(&hisp_dev->rproc_state_mutex);
	mutex_destroy(&hisp_dev->hisp_power_mutex);
	mutex_destroy(&hisp_dev->ispcpu_mutex);
	mutex_destroy(&hisp_dev->jpeg_mutex);
	mutex_destroy(&hisp_dev->sharedbuf_mutex);

	hisp_dev->pwronisp_thread_status = 0;
	hisp_dev->probe_finished  = 0;
	hisp_dev->isp_pdev = NULL;
}

static int hisp_dev_probe_prev(struct platform_device *pdev)
{
	struct hisp_boot_device *hisp_dev = &hisp_boot_dev;
	int ret;

	ret = hisp_device_init(pdev, hisp_dev);
	if (ret < 0)
		return ret;

	ret = hisp_dev_regaddr_getdts(pdev, hisp_dev);
	if (ret < 0) {
		pr_err("[%s] Failed : hisp_regaddr_init.%d\n", __func__, ret);
		goto hisp_regaddr_init_fail;
	}

	ret = hisp_rdr_init();
	if (ret != 0) {
		pr_err("%s: rdr_isp_init failed.%d", __func__, ret);
		goto rdr_isp_init_fail;
	}

	ret = hisp_pwr_probe(pdev);
	if (ret < 0) {
		hisp_err("Failed : hisp_pwr_probe.%d\n", ret);
		goto dtsi_read_fail;
	}

	ret = hisp_sec_probe(pdev);
	if (ret < 0) {
		hisp_err("Failed : hisp_sec_probe.%d\n", ret);
		goto hisp_pwr_probe_fail;
	}

	ret = hisp_nsec_probe(pdev);
	if (ret < 0) {
		hisp_err("Failed : hisp_nsec_probe.%d\n", ret);
		goto hisp_sec_probe_fail;
	}

	return 0;

hisp_sec_probe_fail:
	(void)hisp_sec_remove(pdev);

hisp_pwr_probe_fail:
	hisp_pwr_remove(pdev);

dtsi_read_fail:
	hisp_rdr_exit();

rdr_isp_init_fail:
	hisp_dev_regaddr_putdts(hisp_dev);

hisp_regaddr_init_fail:
	hisp_device_deinit();

	return -ENODEV;
}

static void hisp_dev_probe_clr(struct platform_device *pdev)
{
	struct hisp_boot_device *hisp_dev = &hisp_boot_dev;

	(void)hisp_sec_remove(pdev);

	hisp_pwr_remove(pdev);

	hisp_rdr_exit();

	hisp_dev_regaddr_putdts(hisp_dev);

	hisp_device_deinit();
}

#ifdef CONFIG_HUAWEI_DSM
struct dsm_client_ops hisp_dsm_ops = {
	.poll_state = NULL,
	.dump_func = NULL,
};

struct dsm_dev hisp_dsm_dev = {
	.name = "dsm_isp",
	.device_name = NULL,
	.ic_name = NULL,
	.module_name = NULL,
	.fops = &hisp_dsm_ops,
	.buff_size = 256,
};
#endif

static int hisp_dsm_register(void)
{
#ifdef CONFIG_HUAWEI_DSM
	struct hisp_boot_device *dev = &hisp_boot_dev;

	dev->hisp_dsm_client = dsm_register_client(&hisp_dsm_dev);
	if (dev->hisp_dsm_client == NULL) {
		pr_err("Failed : dsm_register_client fail\n");
		return -EFAULT;
	}
#endif
	return 0;
}

static void hisp_dsm_unregister(void)
{
#ifdef CONFIG_HUAWEI_DSM
	struct hisp_boot_device *dev = &hisp_boot_dev;

	if (dev->hisp_dsm_client != NULL) {
		dsm_unregister_client(dev->hisp_dsm_client, &hisp_dsm_dev);
		dev->hisp_dsm_client = NULL;
	}
#endif
}

int hisp_device_probe(struct platform_device *pdev)
{
	struct hisp_boot_device *hisp_dev = &hisp_boot_dev;
	struct device *dev = &pdev->dev;
	int ret;

	hisp_info("+\n");

	ret = hisp_dev_probe_prev(pdev);
	if (ret < 0) {
		hisp_err("Failed : hisp_rproc_probe_prev.%d\n", ret);
		return ret;
	}

#ifdef DEBUG_HISP
	ret = hisp_check_pcie_stat();
	if (ret == ISP_PCIE_NREADY)
		return ret;
#endif

	ret = hisp_dev_getdts(&pdev->dev, hisp_dev);
	if (ret < 0) {
		hisp_err("hisp_dev_getdts : ret. %d\n", ret);
		goto hisp_nsec_probe_fail;
	}

	ret = hisp_rpmsg_rdr_init();
	if (ret < 0) {
		dev_err(dev, "Fail :hisp_rpmsg_rdr_init: %d\n", ret);
		goto hisp_rpmsg_rdr_init_fail;
	}

	ret = hisp_dsm_register();
	if (ret < 0) {
		dev_err(dev, "Fail :hisp_dsm_register: %d\n", ret);
		goto hisp_rpmsg_rdr_init_fail;
	}

	hisp_dev->probe_finished = 1;
	hisp_info("-\n");
	return 0;

hisp_rpmsg_rdr_init_fail:
	(void)hisp_rpmsg_rdr_deinit();

hisp_nsec_probe_fail:
	(void)hisp_nsec_remove(pdev);

	hisp_dev_probe_clr(pdev);

	return ret;
}

int hisp_device_remove(struct platform_device *pdev)
{
	struct hisp_boot_device *hisp_dev = &hisp_boot_dev;

	hisp_info("+\n");
	hisp_dsm_unregister();
	(void)hisp_rpmsg_rdr_deinit();

	(void)hisp_pwr_remove(pdev);
	(void)hisp_nsec_remove(pdev);
	(void)hisp_sec_remove(pdev);

	hisp_rdr_exit();
	hisp_dev_regaddr_putdts(hisp_dev);

	hisp_device_deinit();

	hisp_info("-\n");
	return 0;
}

#ifdef DEBUG_HISP
static int hisp_dev_set_power_updn(int bypass)
{
	struct hisp_boot_device *dev = &hisp_boot_dev;

	if (bypass != 0 && bypass != 1) {/*lint !e774 */
		hisp_err("Failed : bypass.%x\n", bypass);
		return -EINVAL;
	}

	dev->bypass_pwr_updn = bypass;

	return 0;
}

ssize_t hisp_power_show(struct device *pdev,
		struct device_attribute *attr, char *buf)
{
	char *s = buf;

	s += snprintf_s(s, NONSEC_MAX_SIZE, NONSEC_MAX_SIZE-1,
			"Bypass Power Up and Down : %s\n",
			(hisp_bypass_power_updn() ? "enable" : "disable"));

	return (s - buf);
}

ssize_t hisp_power_store(struct device *pdev,
		struct device_attribute *attr, const char *buf,
		size_t count)
{
	char *p = NULL;
	size_t len;

	p = memchr(buf, '\n', count);
	if (p == NULL) {
		pr_err("[%s] Failed p.%pK\n", __func__, p);
		return -EINVAL;
	}

	len = (size_t)(p - buf);
	if (len == 0)
		return -EINVAL;

	if (!strncmp(buf, "enable", len))
		hisp_dev_set_power_updn(1);
	else if (!strncmp(buf, "disable", len))
		hisp_dev_set_power_updn(0);
	else
		pr_err("<Usage> echo enable/disable > isppower\n");

	return count;/*lint !e713 */
}

#endif

