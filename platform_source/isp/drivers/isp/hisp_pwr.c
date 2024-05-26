/*
 *  driver, hisp_pwr.c
 *
 * Copyright (c) 2013 ISP Technologies CO., Ltd.
 *
 */

#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/mutex.h>
#include <linux/clk.h>
#include <linux/regulator/consumer.h>
#ifdef CONFIG_LOAD_IMAGE
#include <platform_include/see/load_sec_image.h>
#endif
#include <platform_include/basicplatform/linux/partition/partition_ap_kernel.h>
#include <platform_include/isp/linux/hisp_remoteproc.h>
#include <teek_client_id.h>
#include <platform_include/basicplatform/linux/partition/partition_macro.h>
#include <global_ddr_map.h>
#include "hisp_internel.h"
#include <isp_ddr_map.h>
#include <securec.h>
#include "platform_include/basicplatform/linux/peri_volt_poll.h"

#ifdef CONFIG_CAMERA_USE_HISP230
#include "soc_spec_info.h"
#endif

#ifdef DEBUG_HISP
#include <linux/platform_drivers/mm_svm.h>
#include "hisp_pcie.h"
#endif

#ifdef CONFIG_ES_ISP_LOW_FREQ
#define CLOCKVALUE                          "clock-value-low"
#else
#define CLOCKVALUE                          "clock-value"
#endif
#define CLOCKVALUELOW                       "clock-value-low"

#define ISP_CFG_SIZE                        0x1000
#define ISP_LOAD_PARTITION                  "isp_firmware"
#define HZ_TO_MHZ_DIV                       1000000
#define DEVICE_PATH                         "/dev/block/bootdevice/by-name/"

#define ISPCPU_REMAP_ENABLE                 (1 << 31)
#define ISP_SUB_CTRL_ISP_A7_CTRL_0          (0x40)
#define ISP_SUB_CTRL_CANARY_ADDR            (0x6FC)

#define QOS_MAX_NUM                         64
#define QOS_NSECREG                         0U

#ifdef CONFIG_KERNEL_CAMERA_USE_HISP120
#define ISP_020010_MODULE_CGR_TOP           0x020010
#define ISP_BASE_ADDR_CVDR_SRT              0x2E000
#define ISP_BASE_ADDR_CVDR_RT               0x22000
#define ISP_SUB_CTRL_BASE_ADDR              0x183000
#define SMMU_CLK_ENABLE                     0x400000

#define CVDR_RT_CVDR_CFG_REG                0x0
#define CVDR_RT_CVDR_WR_QOS_CFG_REG         0xC
#define CVDR_RT_CVDR_RD_QOS_CFG_REG         0x10
#define CVDR_SRT_CVDR_CFG_REG               0x0
#define CVDR_SRT_CVDR_WR_QOS_CFG_REG        0xC
#define CVDR_SRT_CVDR_RD_QOS_CFG_REG        0x10
#define CVDR_SRT_VP_WR_IF_CFG_10_REG        0xC8
#define CVDR_SRT_VP_WR_IF_CFG_11_REG        0xD8
#define CVDR_SRT_VP_WR_IF_CFG_25_REG        0x1B8
#define CVDR_SRT_NR_RD_CFG_1_REG            0xA10
#define CVDR_SRT_LIMITER_NR_RD_1_REG        0xA18
#define CVDR_SRT_NR_RD_CFG_3_REG            0xA30

#define SUB_CTRL_ISP_FLUX_CTRL2_0_REG       0x190
#define SUB_CTRL_ISP_FLUX_CTRL2_1_REG       0x194
#define SUB_CTRL_ISP_FLUX_CTRL3_0_REG       0x198
#define SUB_CTRL_ISP_FLUX_CTRL3_1_REG       0x19C

#define CVDR_CLK_ENABLE                     (0x40020)
#endif

enum hisp_clk_dvfs_type {
	HISP_CLK_TURBO      = 0,
	HISP_CLK_NORMINAL   = 1,
	HISP_CLK_LOWSVS     = 2,
	HISP_CLK_HIGHSVS    = 3,
	HISP_CLK_SVS        = 4,
	HISP_CLK_DISDVFS    = 5,
	HISP_CLK_DVFS_MAX
};

enum isp_pwstat_e {
	PWSTAT_MEDIA	= 0,
	PWSTAT_ISPSYS	= 1,
	PWSTAT_MAX
};

#ifdef DEBUG_HISP
#define DUMP_ISPCPU_PC_TIMES    3

struct hisp_clk_dump_s {
	bool enable;
	unsigned int ispcpu_stat;
	unsigned int ispcpu_pc[DUMP_ISPCPU_PC_TIMES];
	unsigned long freq[ISP_CLK_MAX];
	unsigned int freqmask;
};
#endif

struct hisp_qos {
	unsigned int secreg;
	unsigned int num;
	unsigned int *offset;
	unsigned int *value;
};

struct hisp_pwr {
	unsigned int dvfsmask;
	unsigned int usedvfs;
	unsigned int refs_isp_power;
	unsigned int refs_isp_nsec_init;
	unsigned int refs_isp_sec_init;
	unsigned int refs_ispcpu_init;
	unsigned int refs_isp_nsec_dst;
	unsigned int refs_isp_sec_dst;
	unsigned int use_smmuv3_flag;
	unsigned int use_buck13_flag;
	unsigned int ispcpu_supply_flag;
	unsigned int ispsmmu_init_byap;
	unsigned int clock_num;
	unsigned int pwstat_num;
#ifdef DEBUG_HISP
	struct hisp_clk_dump_s hisp_clk;
#endif
	struct mutex pwrlock;
	struct hisp_qos qos;
	struct device *device;
	struct regulator *clockdep_supply;
	struct regulator *ispcore_supply;
	struct regulator *ispcpu_supply;
	struct regulator *isptcu_supply;
	struct clk *ispclk[ISP_CLK_MAX];
	const char *clk_name[ISP_CLK_MAX];
	unsigned int ispclk_value[ISP_CLK_MAX];
#ifdef CONFIG_KERNEL_CAMERA_USE_HISP120
	unsigned int ispclk_value_low[ISP_CLK_MAX];
#endif
	unsigned int clkdis_dvfs[ISP_CLK_MAX];
	unsigned int clkdis_need_div[ISP_CLK_MAX];
	unsigned int clkdn[HISP_CLK_DVFS_MAX][ISP_CLK_MAX];
	unsigned int pwstat_type[PWSTAT_MAX];
	unsigned int pwstat_offset[PWSTAT_MAX];
	unsigned int pwstat_bit[PWSTAT_MAX];
	unsigned int pwstat_wanted[PWSTAT_MAX];
};

struct hisp_pwr isp_pwr_dev;
#ifdef DEBUG_HISP
static unsigned long hisp_pwr_get_ispclk_freq(unsigned int type);
#endif
int hisp_bsp_read_bin(const char *partion_name, unsigned int offset,
		unsigned int length, char *buffer)
{
	int ret = -1;
	char *pathname = NULL;
	unsigned long pathlen;
	struct file *fp = NULL;
	mm_segment_t fs;
	loff_t pos = 0;

	if ((partion_name == NULL) || (buffer == NULL))
		return -EINVAL;

	/*get resource*/
	pathlen = sizeof(DEVICE_PATH) +
		strnlen(partion_name, (unsigned long)PART_NAMELEN);
	pathname = kmalloc(pathlen, GFP_KERNEL);
	if (pathname == NULL)
		return -EINVAL;

	ret = flash_find_ptn_s((const char *)partion_name, pathname, pathlen);
	if (ret < 0) {
		pr_err("partion_name.%s not in partion table!\n", partion_name);
		goto error;
	}

	fp = filp_open(pathname, O_RDONLY, 0600);
	if (IS_ERR(fp)) {
		pr_err("filp_open(%s) failed", pathname);
		ret = -1;
		goto error;
	}

	ret = vfs_llseek(fp, offset, SEEK_SET);
	if (ret < 0) {
		pr_err("seek ops failed, ret %d", ret);
		goto err_close;
	}

	fs = get_fs();/*lint !e501*/
	set_fs(KERNEL_DS);/*lint !e501 */

	pos = fp->f_pos;/*lint !e613 */
	ret = vfs_read(fp, (char __user *)buffer, length, &pos);/*lint !e613 */
	if (ret != (int)length) {
		pr_err("read ops failed, read ret=%d(length=%d)", ret, length);
		set_fs(fs);
		ret = -1;
		goto err_close;
	}
	set_fs(fs);

err_close:
	filp_close(fp, NULL);/*lint !e668 */

error:
	/*free resource*/
	if (pathname != NULL) {
		kfree(pathname);
		pathname = NULL;
	}

	return ret;
}

static int hisp_pwr_need_powerup(unsigned int refs)
{
	if (refs == ISP_MAX_NUM_MAGIC)
		pr_err("[%s] exc, refs == 0xffffffff\n", __func__);

	return ((refs == 0) ? 1 : 0);
}

static int hisp_pwr_need_powerdn(unsigned int refs)
{
	if (refs == ISP_MAX_NUM_MAGIC)
		pr_err("[%s] exc, refs == 0xffffffff\n", __func__);

	return ((refs == 1) ? 1 : 0);
}

static int hisp_pwr_clk_valid_check(int clk)
{
	struct hisp_pwr *dev = &isp_pwr_dev;

	if (clk >= (int)dev->clock_num) {
		pr_err("[%s] Failed : clk %d >= %d\n",
				__func__, clk, dev->clock_num);
		return -EINVAL;
	}

	return 0;
}

int hisp_dvfs_enable(void)
{
	struct hisp_pwr *dev = &isp_pwr_dev;

	return dev->usedvfs;
}

static int hisp_pwr_invalid_dvfsmask(int clk)
{
	struct hisp_pwr *dev = &isp_pwr_dev;
	int ret;

	ret = hisp_pwr_clk_valid_check(clk);
	if (ret < 0)
		return ret;

	return (0x01 & (dev->dvfsmask >> (unsigned int)clk));
}

static int hisp_pwr_clk_dn(struct hisp_pwr *dev,
		int clk, unsigned int clkdown)
{
	int ret = 0;
	unsigned long ispclk = 0;
	unsigned int stat_machine = clkdown;
	unsigned int type;

	if (!hisp_dvfs_enable()) {
		pr_err("[%s] Failed : Do not Support DVFS\n", __func__);
		return -EINVAL;
	}

	if (clk >= (int)dev->clock_num) {
		pr_err("[%s] Failed : clk.%d >= %d\n",
			__func__, clk, dev->clock_num);
		return -EINVAL;
	}

	do {
		type = stat_machine;
		if (type >= HISP_CLK_DVFS_MAX) {
			pr_err("[%s] Failed: type. %d > %d or < 0\n",
					__func__, type, HISP_CLK_DVFS_MAX);
			return -EINVAL;
		}

		stat_machine++;
		ispclk = (unsigned long)dev->clkdn[type][clk];
		pr_info("[%s] Clock Down %lu.%lu MHz\n", __func__,
			ispclk / HZ_TO_MHZ_DIV, ispclk % HZ_TO_MHZ_DIV);
		if (ispclk == 0)
			continue;

		ret = clk_set_rate(dev->ispclk[clk], (unsigned long)ispclk);
		if (ret < 0) {
			pr_err("[%s] Failed: clk_set_rate.%d > %d\n",
				__func__, type, stat_machine);
			goto try_clock_down;
		}

		ret = clk_prepare_enable(dev->ispclk[clk]);
		if (ret < 0) {
			pr_err("[%s] Failed: clk_prepare_enable.%d, %d > %d\n",
				__func__, ret, type, stat_machine);
			goto try_clock_down;
		}
		hisp_share_update_clk_value(clk, (unsigned int)ispclk);
try_clock_down:
		if (ret != 0 && stat_machine < HISP_CLK_DVFS_MAX)
			pr_info("[%s]Clk Dwn %lu.%luMHz > %u.%uMHz\n", __func__,
				ispclk / HZ_TO_MHZ_DIV, ispclk % HZ_TO_MHZ_DIV,
				dev->clkdn[stat_machine][clk] / HZ_TO_MHZ_DIV,
				dev->clkdn[stat_machine][clk] % HZ_TO_MHZ_DIV);
	} while (ret != 0);

	return ret;
}

static unsigned long hisp_pwr_get_clk(struct hisp_pwr *dev, int clk)
{
	unsigned long ispclock = 0;

#ifdef CONFIG_KERNEL_CAMERA_USE_HISP120
	int lowpower_flag = hisp120_get_lowpower();

	if (lowpower_flag)
		ispclock = (unsigned long)dev->ispclk_value_low[clk];
	else
		ispclock = (unsigned long)dev->ispclk_value[clk];
#else
	ispclock = (unsigned long)dev->ispclk_value[clk];
#endif

	return ispclock;
}

static int hisp_pwr_clock_enable(struct hisp_pwr *dev, int clk)
{
	unsigned long ispclock = 0;
	int ret;
#ifdef DEBUG_HISP
	unsigned long ispclock_debug;

	ret = hisp_check_pcie_stat();
	if (ret != ISP_NORMAL_MODE)
		return 0;
#endif

	if (hisp_pwr_clk_valid_check(clk))
		return -EINVAL;

	if (!strncmp(dev->clk_name[clk], "isp_sys", strlen("isp_sys"))) {
		ret = clk_prepare_enable(dev->ispclk[clk]);
		if (ret < 0) {
			pr_err("[%s] Failed: %d.%s.clk_prepare_enable.%d\n",
				__func__, clk, dev->clk_name[clk], ret);
			return -EINVAL;
		}
		pr_info("[%s] %d.%s.clk_prepare_enable\n",
				__func__, clk, dev->clk_name[clk]);
		return 0;
	}

	ispclock = hisp_pwr_get_clk(dev, clk);

#ifdef DEBUG_HISP
	ispclock_debug = hisp_pwr_get_ispclk_freq(clk);
	if (ispclock_debug != 0)
		ispclock = ispclock_debug;
#endif

	ret = clk_set_rate(dev->ispclk[clk], ispclock);
	if (ret < 0) {
		pr_err("[%s] Failed: %d.%d M, %d.%s.clk_set_rate.%d\n",
			__func__, (int)ispclock / HZ_TO_MHZ_DIV,
			(int)ispclock % HZ_TO_MHZ_DIV,
			clk, dev->clk_name[clk], ret);
		goto try_clock_down;
	}

	ret = clk_prepare_enable(dev->ispclk[clk]);
	if (ret < 0) {
		pr_err("[%s] Failed: %d.%d M, %d.%s.clk_prepare_enable.%d\n",
			__func__, (int)ispclock / HZ_TO_MHZ_DIV,
			(int)ispclock % HZ_TO_MHZ_DIV,
			clk, dev->clk_name[clk], ret);
		goto try_clock_down;
	}
	hisp_share_update_clk_value(clk, (unsigned int)ispclock);
	pr_info("[%s] %d.%s.clk_set_rate.%d.%d M\n", __func__,
		clk, dev->clk_name[clk], (int)ispclock / HZ_TO_MHZ_DIV,
		(int)ispclock % HZ_TO_MHZ_DIV);

	return 0;

try_clock_down:
	return hisp_pwr_clk_dn(dev, clk, HISP_CLK_SVS);
}

static void hisp_pwr_clock_disable(struct hisp_pwr *dev, int clk)
{
	unsigned long ispclock = 0;
	int ret = 0;

	if (hisp_pwr_clk_valid_check(clk))
		return;

	if (!strncmp(dev->clk_name[clk], "isp_sys", strlen("isp_sys"))) {
		pr_info("[%s] %d.%s.clk_disable_unprepare\n",
				__func__, clk, dev->clk_name[clk]);
		clk_disable_unprepare(dev->ispclk[clk]);
		return;
	}

	ispclock = (unsigned long)dev->clkdis_need_div[clk];
	if (ispclock != 0) {
		ret = clk_set_rate(dev->ispclk[clk], ispclock);
		if (ret < 0) {
			pr_err("[%s] Failed: need_div %d.%d M, %d.%s.rate.%d\n",
				__func__, (int)ispclock / HZ_TO_MHZ_DIV,
				(int)ispclock % HZ_TO_MHZ_DIV,
				clk, dev->clk_name[clk], ret);
			return;
		}
		pr_info("[%s] %d.%s.need_div clk_set_rate.%d.%d M\n", __func__,
			clk, dev->clk_name[clk], (int)ispclock / HZ_TO_MHZ_DIV,
			(int)ispclock % HZ_TO_MHZ_DIV);
	}

	ispclock = (unsigned long)dev->clkdis_dvfs[clk];
	ret = clk_set_rate(dev->ispclk[clk], ispclock);
	if (ret < 0) {
		pr_err("[%s] Failed: %d.%d M, %d.%s.clk_set_rate.%d\n",
			__func__, (int)ispclock / HZ_TO_MHZ_DIV,
			(int)ispclock % HZ_TO_MHZ_DIV, clk,
			dev->clk_name[clk], ret);
		return;
	}

	hisp_share_update_clk_value(clk, (unsigned int)ispclock);
	pr_info("[%s] %d.%s.clk_set_rate.%d.%d M\n", __func__, clk,
			dev->clk_name[clk], (int)ispclock / HZ_TO_MHZ_DIV,
			(int)ispclock % HZ_TO_MHZ_DIV);

	clk_disable_unprepare(dev->ispclk[clk]);
}

static int hisp_pwr_get_isppwr_state(unsigned int type)
{
	struct hisp_pwr *dev = &isp_pwr_dev;
	void __iomem *cfg_base = NULL;
	unsigned int media_pw_stat = 0;
	unsigned int pwsta_num = 0;

#ifdef DEBUG_HISP
	int status = hisp_check_pcie_stat();
	if (status != ISP_NORMAL_MODE)
		return 0;
#endif

	pwsta_num = dev->pwstat_num > PWSTAT_MAX ? PWSTAT_MAX : dev->pwstat_num;

	if (type >= pwsta_num) {
		pr_err("[%s] Failed: type.%d, pwstat_num.%d\n",
			__func__, type, pwsta_num);
		return 0;
	}

	if (dev->pwstat_offset[type] >= ISP_CFG_SIZE) {
		pr_err("[%s] Failed: pwstat_offset.0x%x, type.%d\n",
			__func__, dev->pwstat_offset[type], type);
		return 0;
	}

	cfg_base = hisp_dev_get_regaddr(dev->pwstat_type[type]);
	if (cfg_base == NULL) {
		pr_err("[%s] Failed: type.%d\n", __func__, type);
		return 0;
	}
	media_pw_stat = __raw_readl(cfg_base + dev->pwstat_offset[type]);
	media_pw_stat >>= dev->pwstat_bit[type];
	media_pw_stat &= 0x01;

	if (media_pw_stat == dev->pwstat_wanted[type])
		return 1;
	else
		return 0;
}

static int hisp_pwr_dst_ispcpu(u64 remap_addr, unsigned int canary)
{
	void __iomem *isp_subctrl_base;
	int ret = 0;

	pr_info("[%s] +\n", __func__);

#ifdef DEBUG_HISP
	int status;

	status = hisp_check_pcie_stat();
	if (status == ISP_PCIE_NREADY)
		return status;
	else if (status == ISP_PCIE_MODE)
		goto smc_disreset_cpu;
#endif
	ret = hisp_pwr_get_isppwr_state(PWSTAT_MEDIA);
	if (ret == 0) {
		pr_err("[%s] Failed : get_media1_subsys_power_state. %d\n",
				__func__, ret);
		return -EINVAL;
	}

	ret = hisp_pwr_get_isppwr_state(PWSTAT_ISPSYS);
	if (ret == 0) {
		pr_err("[%s] Failed : get_isptop_power_state. %d\n",
				__func__, ret);
		return -EINVAL;
	}

	isp_subctrl_base = hisp_dev_get_regaddr(ISP_SUBCTRL);
	if (isp_subctrl_base == NULL) {
		pr_err("[%s] Failed : isp_subctrl_base\n", __func__);
		return -ENOMEM;
	}

	__raw_writel((unsigned int)(ISPCPU_REMAP_ENABLE | (remap_addr >> 16)),
		isp_subctrl_base + ISP_SUB_CTRL_ISP_A7_CTRL_0);/*lint !e648 */

	__raw_writel(canary, isp_subctrl_base + ISP_SUB_CTRL_CANARY_ADDR);

smc_disreset_cpu:
	ret = hisp_smc_disreset_ispcpu();
	if (ret < 0) {
		pr_err("[%s] hisp_smc_disreset_ispcpu. %d\n", __func__, ret);
		return ret;
	}

	pr_info("[%s] -\n", __func__);
	return 0;
}

static int hisp_pwr_setclkrate(unsigned int type, unsigned int rate)
{
	struct hisp_pwr *dev = &isp_pwr_dev;
	int ret;

#ifdef DEBUG_HISP
	int status = hisp_check_pcie_stat();
	if (status != ISP_NORMAL_MODE)
		return 0;
#endif

	if (hisp_pwr_invalid_dvfsmask(type)) {
		pr_err("[%s] Failed : DVFS type.0x%x, rate.%d, dvfsmask.0x%x\n",
				__func__, type, rate, dev->dvfsmask);
		return -EINVAL;
	}

	if ((rate > dev->ispclk_value[type]) || (rate == 0)) {
		pr_err("[%s] Fail: type.0x%x.%s, %d. %d.%08dM > %d. %d.%08dM\n",
			__func__, type, dev->clk_name[type],
			rate, rate / HZ_TO_MHZ_DIV,
			rate % HZ_TO_MHZ_DIV, dev->ispclk_value[type],
			dev->ispclk_value[type] / HZ_TO_MHZ_DIV,
			dev->ispclk_value[type] % HZ_TO_MHZ_DIV);
		return -EINVAL;
	}

	ret = clk_set_rate(dev->ispclk[type], (unsigned long)rate);
	if (ret < 0) {
		pr_err("[%s] Failed : DVFS Set.0x%x.%s Rate.%d. %d.%08d M\n",
				__func__, type, dev->clk_name[type], rate,
				rate / HZ_TO_MHZ_DIV, rate % HZ_TO_MHZ_DIV);
		return ret;
	}
	hisp_share_update_clk_value((int)type, rate);
	pr_info("[%s] DVFS Set.0x%x.%s Rate.%d. %d.%08d M\n",
		__func__, type, dev->clk_name[type], rate,
		rate / HZ_TO_MHZ_DIV, rate % HZ_TO_MHZ_DIV);

	return 0;
}

#ifdef DEBUG_HISP
static int hisp_pwr_subsys_common_up_pcie(void)
{
	struct hisp_pwr *dev = &isp_pwr_dev;
	struct isp_pcie_cfg *cfg;
	int ret;

	cfg = hisp_get_pcie_cfg();
	if (cfg == NULL) {
		pr_err("Failed : isp pcie status abnormal\n");
		return -EINVAL;
	}

	pr_debug("[%s] media2 vcodec start power up", __func__);
	ret = hisp_smc_media2_vcodec_power_up(cfg->isp_pci_inuse_flag, cfg->isp_pci_addr);
	if (ret != 0) {
		pr_err("[%s] Failed: hisp_smc_media2_vcodec_power_up.%d\n", __func__, ret);
		return ret;
	}

	pr_debug("[%s] smmu start power up", __func__);
	if (dev->use_smmuv3_flag) {
#if defined(CONFIG_ARM_SMMU_V3) || defined(CONFIG_MM_SMMU_V3)
		ret = mm_smmu_poweron(dev->device);
		if (ret != 0) {
			pr_err("[%s] Failed:isptcu enable.%d\n", __func__, ret);
			(void)hisp_smc_media2_vcodec_power_down(cfg->isp_pci_inuse_flag, cfg->isp_pci_addr);
			return ret;
		}
#endif
	}

	pr_debug("[%s] ispnoc r8 start power up", __func__);
	ret = hisp_smc_ispnoc_r8_power_up(cfg->isp_pci_inuse_flag, cfg->isp_pci_addr);
	if (ret != 0) {
		pr_err("[%s] Failed: hisp_smc_ispnoc_r8_power_up.%d\n", __func__, ret);
		if (dev->use_smmuv3_flag)
#if defined(CONFIG_ARM_SMMU_V3) || defined(CONFIG_MM_SMMU_V3)
			(void)mm_smmu_poweroff(dev->device);
#endif
		(void)hisp_smc_media2_vcodec_power_down(cfg->isp_pci_inuse_flag, cfg->isp_pci_addr);
	}

	return ret;
}

static int hisp_pwr_subsys_common_dn_pcie(void)
{
	struct hisp_pwr *dev = &isp_pwr_dev;
	struct isp_pcie_cfg *cfg;
	int ret;

	cfg = hisp_get_pcie_cfg();
	if (cfg == NULL) {
		pr_err("Failed : isp pcie status abnormal\n");
		return -EINVAL;
	}

	pr_debug("[%s] ispnoc r8 start power down", __func__);
	ret = hisp_smc_ispnoc_r8_power_down(cfg->isp_pci_inuse_flag, cfg->isp_pci_addr);
	if (ret != 0) {
		pr_err("[%s] Failed: hisp_smc_ispnoc_r8_power_down.%d\n", __func__, ret);
		return ret;
	}

	pr_debug("[%s] smmuv3 start power down", __func__);
	if (dev->use_smmuv3_flag) {
#if defined(CONFIG_ARM_SMMU_V3) || defined(CONFIG_MM_SMMU_V3)
		ret = mm_smmu_poweroff(dev->device);
		if (ret != 0) {
			pr_err("[%s] Failed: isp tcu regulator_disable.%d\n",
					__func__, ret);
			return ret;
		}
#endif
	}

	pr_debug("[%s] media2 vcodec start power down", __func__);
	ret = hisp_smc_media2_vcodec_power_down(cfg->isp_pci_inuse_flag, cfg->isp_pci_addr);
	if (ret != 0)
		pr_err("[%s] Failed: hisp_smc_media2_vcodec_power_down.%d\n", __func__, ret);

	return ret;
}
#endif

static int hisp_pwr_subsys_common_up(void)
{
	struct hisp_pwr *dev = &isp_pwr_dev;
	int ret, index, err_index;

#ifdef CONFIG_PERI_DVFS
	if (dev->use_buck13_flag)
		peri_buck13_mem_volt_set(DVFS_ISP_CHANEL, DVFS_BUCK13_VOLT1);
#endif

	ret = regulator_enable(dev->clockdep_supply);
	if (ret != 0) {
		pr_err("[%s] Failed: clockdep enable.%d\n", __func__, ret);
		return ret;
	}

	if (dev->use_smmuv3_flag) {
		ret = regulator_enable(dev->isptcu_supply);
		if (ret != 0) {
			pr_err("[%s] Failed:isptcu enable.%d\n", __func__, ret);
			goto err_tcu_poweron;
		}
	}

	for (index = 0; index < (int)dev->clock_num; index++) {
		ret = hisp_pwr_clock_enable(dev, index);
		if (ret < 0) {
			pr_err("[%s] Failed: hisp_pwr_clock_enable.%d, index.%d\n",
					__func__, ret, index);
			goto err_ispclk;
		}
	}

	ret = regulator_enable(dev->ispcore_supply);
	if (ret != 0) {
		pr_err("[%s] Failed: ispcore enable.%d\n", __func__, ret);
		goto err_ispclk;
	}

	if (dev->ispcpu_supply_flag) {
		ret = regulator_enable(dev->ispcpu_supply);
		if (ret != 0) {
			pr_err("[%s] Failed:ispcpu enable.%d\n", __func__, ret);
			goto err_ispcpu_supply;
		}
	}

	return 0;

err_ispcpu_supply:
	(void)regulator_disable(dev->ispcore_supply);

err_ispclk:
	for (err_index = 0; err_index < index; err_index++)
		hisp_pwr_clock_disable(dev, err_index);
	if (dev->use_smmuv3_flag)
		(void)regulator_disable(dev->isptcu_supply);

err_tcu_poweron:
	(void)regulator_disable(dev->clockdep_supply);

	return ret;
}

static int hisp_pwr_subsys_common_dn(void)
{
	struct hisp_pwr *dev = &isp_pwr_dev;
	int ret, index;

	if (dev->ispcpu_supply_flag) {
		ret = regulator_disable(dev->ispcpu_supply);
		if (ret != 0)
			pr_err("[%s] Failed: ispcpu regulator_disable.%d\n",
					__func__, ret);
	}

	ret = regulator_disable(dev->ispcore_supply);
	if (ret != 0)
		pr_err("[%s] Failed: ispcore regulator_disable.%d\n",
				__func__, ret);

	for (index = 0; index < (int)dev->clock_num; index++)
		hisp_pwr_clock_disable(dev, index);

	if (dev->use_smmuv3_flag) {
		ret = regulator_disable(dev->isptcu_supply);
		if (ret != 0)
			pr_err("[%s] Failed: isp tcu regulator_disable.%d\n",
					__func__, ret);
	}
	ret = regulator_disable(dev->clockdep_supply);
	if (ret != 0)
		pr_err("[%s] Failed: isp clockdep regulator_disable.%d\n",
				__func__, ret);

#ifdef CONFIG_PERI_DVFS
	if (dev->use_buck13_flag)
		peri_buck13_mem_volt_set(DVFS_ISP_CHANEL, DVFS_BUCK13_VOLT0);
#endif

	return 0;
}

unsigned int hisp_smmuv3_mode(void)
{
	struct hisp_pwr *dev = &isp_pwr_dev;

	return dev->use_smmuv3_flag;
}

int hisp_pwr_subsys_powerup(void)
{
	struct hisp_pwr *dev = &isp_pwr_dev;
	int status = ISP_NORMAL_MODE;
	int ret = 0;
	int err;

	mutex_lock(&dev->pwrlock);
	pr_info("[%s] + refs_isp_power.0x%x\n", __func__, dev->refs_isp_power);
	if (!hisp_pwr_need_powerup(dev->refs_isp_power)) {
		dev->refs_isp_power++;
		pr_info("[%s]: refs_isp.0x%x\n", __func__, dev->refs_isp_power);
		mutex_unlock(&dev->pwrlock);
		return 0;
	}
#ifdef DEBUG_HISP
	status = hisp_check_pcie_stat();
#endif
	switch (status) {
	case ISP_NORMAL_MODE:
		ret = hisp_pwr_subsys_common_up();
		break;
#ifdef DEBUG_HISP
	case ISP_PCIE_MODE:
		ret = hisp_pwr_subsys_common_up_pcie();
		break;
#endif
	default:
		pr_err("unkown PLATFORM: 0x%x\n", status);
		return status;
	}
	if (ret < 0) {
		pr_err("[%s] Failed: hisp_pwr_subsys_common_up.%d\n",
			__func__, ret);
		goto err_common_powerup;
	}

	if (hisp_mntn_dumpregs() < 0)
		pr_err("Failed : get_ispcpu_cfg_info");

	if (hisp_nsec_boot_mode() || (!hisp_ca_boot_mode())) {
		ret = hisp_smc_isptop_power_up();
		if (ret != 0) {
			pr_err("[%s] hisp_smc_isptop_power_up.%d\n", __func__, ret);
			goto err_isptop;
		}
	}

	dev->refs_isp_power++;
	pr_info("[%s] - refs_isp_power.0x%x\n", __func__, dev->refs_isp_power);
	mutex_unlock(&dev->pwrlock);

	return 0;

err_isptop:
	err = hisp_pwr_subsys_common_dn();
	if (err != 0)
		pr_err("[%s] Failed: hisp_pwr_subsys_common_dn.%d\n",
				__func__, err);

err_common_powerup:
	mutex_unlock(&dev->pwrlock);
	return ret;
}

int hisp_pwr_subsys_powerdn(void)
{
	struct hisp_pwr *dev = &isp_pwr_dev;
	int status = ISP_NORMAL_MODE;
	int ret = 0;

	mutex_lock(&dev->pwrlock);
	pr_info("[%s] + refs_isp_power.0x%x\n", __func__, dev->refs_isp_power);
	if (!hisp_pwr_need_powerdn(dev->refs_isp_power)) {
		if (dev->refs_isp_power > 0)
			dev->refs_isp_power--;
		pr_info("[%s] + refs_isp_power.0x%x\n",
			__func__, dev->refs_isp_power);
		mutex_unlock(&dev->pwrlock);
		return 0;
	}
	if (hisp_nsec_boot_mode() || (!hisp_ca_boot_mode())) {
		ret = hisp_smc_isptop_power_down();
		if (ret != 0)
			pr_err("[%s] hisp_smc_isptop_power_down.%d\n", __func__, ret);
	}

#ifdef DEBUG_HISP
	status = hisp_check_pcie_stat();
#endif
	switch (status) {
	case ISP_NORMAL_MODE:
		ret = hisp_pwr_subsys_common_dn();
		break;
#ifdef DEBUG_HISP
	case ISP_PCIE_MODE:
		ret = hisp_pwr_subsys_common_dn_pcie();
		break;
#endif
	default:
		return status;
	}
	if (ret != 0)
		pr_err("[%s] Failed: hisp_pwr_subsys_common_dn.%d\n",
				__func__, ret);

	dev->refs_isp_power--;
	pr_info("[%s] + refs_isp_power.0x%x\n", __func__, dev->refs_isp_power);
	mutex_unlock(&dev->pwrlock);

	return 0;
}

#ifdef CONFIG_KERNEL_CAMERA_USE_HISP120
static int hisp_cvdr_init(void)
{
	void __iomem *isp_regs_base;
	void __iomem *isprt_base;
	void __iomem *ispsrt_base;
	void __iomem *ispsctrl_base;
	unsigned int value;

	pr_info("[%s] +\n", __func__);

	isp_regs_base = hisp_dev_get_regaddr(ISP_ISPCORE);
	if (unlikely(!isp_regs_base)) {
		pr_err("[%s] isp_regs_base failed!\n", __func__);
		return -ENOMEM;
	}

	isprt_base = isp_regs_base + ISP_BASE_ADDR_CVDR_RT;
	ispsrt_base = isp_regs_base + ISP_BASE_ADDR_CVDR_SRT;
	ispsctrl_base = isp_regs_base + ISP_SUB_CTRL_BASE_ADDR;

	value = readl(ISP_020010_MODULE_CGR_TOP + isp_regs_base);
	value |= CVDR_CLK_ENABLE;
	writel(value, ISP_020010_MODULE_CGR_TOP + isp_regs_base);

	/* CVDR RT*/
	writel(0x0B0B4001, isprt_base + CVDR_RT_CVDR_CFG_REG);
	/* CVDR SRT*/
	writel(0x0B132201, ispsrt_base + CVDR_SRT_CVDR_CFG_REG);
	//CVDR_RT QOS
	writel(0xF8765432, isprt_base + CVDR_RT_CVDR_WR_QOS_CFG_REG);
	writel(0xF8122334, isprt_base + CVDR_RT_CVDR_RD_QOS_CFG_REG);

	//CVDR_SRT QOS
	writel(0xD0765432, ispsrt_base + CVDR_SRT_CVDR_WR_QOS_CFG_REG);
	writel(0xD0122334, ispsrt_base + CVDR_SRT_CVDR_RD_QOS_CFG_REG);

	//CVDR SRT PORT STOP
	writel(0x00020000, ispsrt_base + CVDR_SRT_NR_RD_CFG_3_REG);
	writel(0x00020000, ispsrt_base + CVDR_SRT_VP_WR_IF_CFG_10_REG);
	writel(0x00020000, ispsrt_base + CVDR_SRT_VP_WR_IF_CFG_11_REG);

	//JPGENC limiter DU=8
	writel(0x00000000, ispsrt_base + CVDR_SRT_VP_WR_IF_CFG_25_REG);
	writel(0x80060100, ispsrt_base + CVDR_SRT_NR_RD_CFG_1_REG);
	writel(0x0F001111, ispsrt_base + CVDR_SRT_LIMITER_NR_RD_1_REG);

	//SRT READ
	writel(0x00026E10, ispsctrl_base + SUB_CTRL_ISP_FLUX_CTRL2_0_REG);
	writel(0x0000021F, ispsctrl_base + SUB_CTRL_ISP_FLUX_CTRL2_1_REG);
	//SRT WRITE
	writel(0x00027210, ispsctrl_base + SUB_CTRL_ISP_FLUX_CTRL3_0_REG);
	writel(0x0000024E, ispsctrl_base + SUB_CTRL_ISP_FLUX_CTRL3_1_REG);

	pr_info("[%s] -\n", __func__);

	return 0;
}
#endif

int hisp_pwr_core_nsec_init(u64 pgd_base)
{
	struct hisp_pwr *dev = &isp_pwr_dev;
#ifdef CONFIG_KERNEL_CAMERA_USE_HISP120
		int ret;
		void __iomem *isp_regs_base = NULL;
#endif
	mutex_lock(&dev->pwrlock);
	pr_info("[%s] + refs_isp_nsec_init.0x%x\n",
		__func__, dev->refs_isp_nsec_init);
	if (!hisp_pwr_need_powerup(dev->refs_isp_nsec_init)) {
		dev->refs_isp_nsec_init++;
		pr_info("[%s] + refs_isp_nsec_init.0x%x\n",
				__func__, dev->refs_isp_nsec_init);
		mutex_unlock(&dev->pwrlock);
		return 0;
	}

#ifdef CONFIG_KERNEL_CAMERA_USE_HISP120
	isp_regs_base = hisp_dev_get_regaddr(ISP_ISPCORE);
	writel(((unsigned int)readl(ISP_020010_MODULE_CGR_TOP +
		 isp_regs_base) | SMMU_CLK_ENABLE),
		 ISP_020010_MODULE_CGR_TOP + isp_regs_base);
	if (dev->ispsmmu_init_byap)
		hisp_smc_ispsmmu_ns_init(pgd_base);

	ret = hisp_cvdr_init();
	if (ret < 0) {
		pr_err("[%s] Failed: hisp_cvdr_init.%d\n", __func__, ret);
		mutex_unlock(&dev->pwrlock);
		return ret;
	}
#endif
	dev->refs_isp_nsec_init++;
	pr_info("[%s] - refs_isp_nsec_init.0x%x\n",
		__func__, dev->refs_isp_nsec_init);
	mutex_unlock(&dev->pwrlock);

	return 0;
}

int hisp_pwr_core_nsec_exit(void)
{
	struct hisp_pwr *dev = &isp_pwr_dev;

	mutex_lock(&dev->pwrlock);
	pr_info("[%s] + refs_isp_nsec_init.0x%x\n",
		__func__, dev->refs_isp_nsec_init);
	if (!hisp_pwr_need_powerdn(dev->refs_isp_nsec_init)) {
		if (dev->refs_isp_nsec_init > 0)
			dev->refs_isp_nsec_init--;
		pr_info("[%s] + refs_isp_nsec_init.0x%x\n",
			__func__, dev->refs_isp_nsec_init);
		mutex_unlock(&dev->pwrlock);
		return 0;
	}

	dev->refs_isp_nsec_init--;
	pr_info("[%s] - refs_isp_nsec_init.0x%x\n",
		__func__, dev->refs_isp_nsec_init);
	mutex_unlock(&dev->pwrlock);

	return 0;
}

int hisp_pwr_core_sec_init(u64 phy_pgd_base)
{
	struct hisp_pwr *dev = &isp_pwr_dev;

	mutex_lock(&dev->pwrlock);
	pr_info("[%s] + refs_isp_sec_init.0x%x\n",
		__func__, dev->refs_isp_sec_init);
	if (!hisp_pwr_need_powerup(dev->refs_isp_sec_init)) {
		dev->refs_isp_sec_init++;
		pr_info("[%s] + refs_isp_sec_init.0x%x\n",
				__func__, dev->refs_isp_sec_init);
		mutex_unlock(&dev->pwrlock);
		return 0;
	}

	hisp_smc_isp_init(phy_pgd_base);
	dev->refs_isp_sec_init++;
	pr_info("[%s] + refs_isp_sec_init.0x%x\n",
		__func__, dev->refs_isp_sec_init);
	mutex_unlock(&dev->pwrlock);

	return 0;
}

int hisp_pwr_core_sec_exit(void)
{
	struct hisp_pwr *dev = &isp_pwr_dev;

	mutex_lock(&dev->pwrlock);
	pr_info("[%s] + refs_isp_sec_init.0x%x\n",
		__func__, dev->refs_isp_sec_init);
	if (!hisp_pwr_need_powerdn(dev->refs_isp_sec_init)) {
		if (dev->refs_isp_sec_init > 0)
			dev->refs_isp_sec_init--;
		pr_info("[%s] + refs_isp_sec_init.0x%x\n",
			__func__, dev->refs_isp_sec_init);
		mutex_unlock(&dev->pwrlock);
		return 0;
	}

	hisp_smc_isp_exit();
	dev->refs_isp_sec_init--;
	pr_info("[%s] + refs_isp_sec_init.0x%x\n",
		__func__, dev->refs_isp_sec_init);
	mutex_unlock(&dev->pwrlock);

	return 0;
}

int hisp_pwr_cpu_sec_init(void)
{
	struct hisp_pwr *dev = &isp_pwr_dev;
	int ret;

	mutex_lock(&dev->pwrlock);
	pr_info("[%s] + refs_ispcpu_init.0x%x\n",
		__func__, dev->refs_ispcpu_init);
	if (!hisp_pwr_need_powerup(dev->refs_ispcpu_init)) {
		dev->refs_ispcpu_init++;
		pr_info("[%s] + refs_ispcpu_init.0x%x\n",
				__func__, dev->refs_ispcpu_init);
		mutex_unlock(&dev->pwrlock);
		return 0;
	}

	ret = hisp_smc_ispcpu_init();
	if (ret < 0) {
		pr_info("[%s] hisp_smc_ispcpu_init fail.%d\n", __func__, ret);
		mutex_unlock(&dev->pwrlock);
		return ret;
	}

	dev->refs_ispcpu_init++;
	pr_info("[%s] + refs_ispcpu_init.0x%x\n",
		__func__, dev->refs_ispcpu_init);
	mutex_unlock(&dev->pwrlock);

	return 0;
}

int hisp_pwr_cpu_sec_exit(void)
{
	struct hisp_pwr *dev = &isp_pwr_dev;

	mutex_lock(&dev->pwrlock);
	pr_info("[%s] + refs_ispcpu_init.0x%x\n",
		__func__, dev->refs_ispcpu_init);
	if (!hisp_pwr_need_powerdn(dev->refs_ispcpu_init)) {
		if (dev->refs_ispcpu_init > 0)
			dev->refs_ispcpu_init--;
		pr_info("[%s] + refs_ispcpu_init.0x%x\n",
				__func__, dev->refs_ispcpu_init);
		mutex_unlock(&dev->pwrlock);
		return 0;
	}

	hisp_smc_ispcpu_exit();
	dev->refs_ispcpu_init--;
	pr_info("[%s] + refs_ispcpu_init.0x%x\n",
		__func__, dev->refs_ispcpu_init);
	mutex_unlock(&dev->pwrlock);

	return 0;
}

int hisp_pwr_cpu_nsec_dst(u64 remap_addr, unsigned int canary)
{
	struct hisp_pwr *dev = &isp_pwr_dev;
	int ret;

	mutex_lock(&dev->pwrlock);
	pr_info("[%s] + refs_isp_nsec_dst.0x%x\n",
		__func__, dev->refs_isp_nsec_dst);
	if (!hisp_pwr_need_powerup(dev->refs_isp_nsec_dst)) {
		dev->refs_isp_nsec_dst++;
		pr_info("[%s] + refs_isp_nsec_dst.0x%x\n",
		__func__, dev->refs_isp_nsec_dst);
		mutex_unlock(&dev->pwrlock);
		return 0;
	}

	/* need config by secure core */
	ret = hisp_pwr_dst_ispcpu(remap_addr, canary);
	if (ret < 0) {
		pr_err("[%s] Failed : disreset_ispcpu.%d\n", __func__, ret);
		mutex_unlock(&dev->pwrlock);
		return ret;
	}
	dev->refs_isp_nsec_dst++;
	pr_info("[%s] + refs_isp_nsec_dst.0x%x\n",
		__func__, dev->refs_isp_nsec_dst);
	mutex_unlock(&dev->pwrlock);

	return 0;
}

int hisp_pwr_cpu_nsec_rst(void)
{
	struct hisp_pwr *dev = &isp_pwr_dev;

	mutex_lock(&dev->pwrlock);
	pr_info("[%s] + refs_isp_nsec_dst.0x%x\n",
		__func__, dev->refs_isp_nsec_dst);
	if (!hisp_pwr_need_powerdn(dev->refs_isp_nsec_dst)) {
		if (dev->refs_isp_nsec_dst > 0)
			dev->refs_isp_nsec_dst--;
		pr_info("[%s] + refs_isp_nsec_dst.0x%x\n",
			__func__, dev->refs_isp_nsec_dst);
		mutex_unlock(&dev->pwrlock);
		return 0;
	}
	dev->refs_isp_nsec_dst--;
	pr_info("[%s] + refs_isp_nsec_dst.0x%x\n",
		__func__, dev->refs_isp_nsec_dst);
	mutex_unlock(&dev->pwrlock);

	return 0;
}

int hisp_pwr_cpu_sec_dst(int ispmem_reserved, unsigned long image_addr)
{
#ifdef CONFIG_LOAD_IMAGE
	struct hisp_pwr *dev = &isp_pwr_dev;
	struct load_image_info loadinfo;
	int ret;

	loadinfo.ecoretype      = ISP;
	loadinfo.image_addr     = image_addr;
	loadinfo.image_size     = MEM_ISPFW_SIZE;
	loadinfo.partion_name   = ISP_LOAD_PARTITION;

	mutex_lock(&dev->pwrlock);
	pr_info("[%s] + %s.(0x%x), init.%x, ispmem_reserved.%x\n",
		__func__, loadinfo.partion_name, loadinfo.image_size,
		dev->refs_isp_sec_dst, ispmem_reserved);

	if (!hisp_pwr_need_powerup(dev->refs_isp_sec_dst)) {
		dev->refs_isp_sec_dst++;
		pr_info("[%s] + refs_isp_sec_dst.0x%x\n",
		__func__, dev->refs_isp_sec_dst);
		mutex_unlock(&dev->pwrlock);
		return 0;
	}

	if (!ispmem_reserved) {
		ret = bsp_load_and_verify_image(&loadinfo);
		if (ret < 0) {
			pr_err("[%s]Failed : bsp_load_and_verify.%d, %s.0x%x\n",
				__func__, ret, loadinfo.partion_name,
				loadinfo.image_size);
			mutex_unlock(&dev->pwrlock);
			return ret;
		}
	} else {
		ret = bsp_load_sec_img(&loadinfo);
		if (ret < 0) {
			pr_err("[%s] Failed : bsp_load_sec_image.%d, %s.0x%x\n",
				__func__, ret, loadinfo.partion_name,
				loadinfo.image_size);
			mutex_unlock(&dev->pwrlock);
			return ret;
		}
	}

	dev->refs_isp_sec_dst++;
	pr_info("[%s]: bsp_load_image.%d, %s.0x%x, init.%x, ispmem_resved.%x\n",
		__func__, ret, loadinfo.partion_name, loadinfo.image_size,
		dev->refs_isp_sec_dst, ispmem_reserved);
	mutex_unlock(&dev->pwrlock);
#endif
	return 0;
}

int hisp_pwr_cpu_sec_rst(void)
{
#ifdef CONFIG_LOAD_IMAGE
	struct hisp_pwr *dev = &isp_pwr_dev;

	mutex_lock(&dev->pwrlock);
	pr_info("[%s] + refs_isp_sec_dst.0x%x\n",
		__func__, dev->refs_isp_sec_dst);
	if (!hisp_pwr_need_powerdn(dev->refs_isp_sec_dst)) {
		if (dev->refs_isp_sec_dst > 0)
			dev->refs_isp_sec_dst--;
		pr_info("[%s] + refs_isp_sec_dst.0x%x\n",
			__func__, dev->refs_isp_sec_dst);
		mutex_unlock(&dev->pwrlock);
		return 0;
	}

	dev->refs_isp_sec_dst--;
	pr_info("[%s] + refs_isp_sec_dst.0x%x\n",
		__func__, dev->refs_isp_sec_dst);
	mutex_unlock(&dev->pwrlock);
#endif
	return 0;
}

int hisp_pwr_cpu_ca_dst(struct hisp_ca_meminfo *info, int info_size)
{
	struct hisp_pwr *dev = &isp_pwr_dev;
	int ret;

	mutex_lock(&dev->pwrlock);
	if (!hisp_pwr_need_powerup(dev->refs_isp_sec_dst)) {
		dev->refs_isp_sec_dst++;
		pr_info("[%s] + refs_isp_sec_dst.0x%x\n",
		__func__, dev->refs_isp_sec_dst);
		mutex_unlock(&dev->pwrlock);
		return 0;
	}

	ret = hisp_ca_rsvmem_config(info, info_size);
	if (ret < 0) {
		pr_err("[%s] Failed : hisp_ca_rsvmem_config.%d.\n",
			__func__, ret);
		mutex_unlock(&dev->pwrlock);
		return ret;
	}

	ret = hisp_ca_ispcpu_disreset();
	if (ret < 0) {
		pr_err("[%s] Failed : hisp_ca_ispcpu_disreset.%d.\n",
			__func__, ret);
		(void)hisp_ca_rsvmem_deconfig();
		mutex_unlock(&dev->pwrlock);
		return ret;
	}

	dev->refs_isp_sec_dst++;
	mutex_unlock(&dev->pwrlock);

	return 0;
}

int hisp_pwr_cpu_ca_rst(void)
{
	struct hisp_pwr *dev = &isp_pwr_dev;
	int ret, err;

	mutex_lock(&dev->pwrlock);
	if (!hisp_pwr_need_powerdn(dev->refs_isp_sec_dst)) {
		if (dev->refs_isp_sec_dst > 0)
			dev->refs_isp_sec_dst--;
		pr_info("[%s] + refs_isp_sec_dst.0x%x\n",
			__func__, dev->refs_isp_sec_dst);
		mutex_unlock(&dev->pwrlock);
		return 0;
	}

	ret = hisp_ca_ispcpu_reset();
	if (ret < 0) {
		pr_err("[%s] Failed : hisp_ca_ispcpu_reset.%d.\n",
			__func__, ret);
		err = hisp_ca_rsvmem_deconfig();
		if (err < 0)
			pr_err("[%s] Failed : rsvmem_deconfig.%d.\n",
				__func__, err);
		mutex_unlock(&dev->pwrlock);
		return ret;
	}

	ret = hisp_ca_rsvmem_deconfig();
	if (ret < 0) {
		pr_err("[%s] Failed : hisp_ca_rsvmem_deconfig.%d.\n",
			__func__, ret);
		mutex_unlock(&dev->pwrlock);
		return ret;
	}

	dev->refs_isp_sec_dst--;
	mutex_unlock(&dev->pwrlock);

	return 0;
}

unsigned long hisp_pwr_getclkrate(unsigned int type)
{
	struct hisp_pwr *dev = &isp_pwr_dev;

	if (hisp_pwr_invalid_dvfsmask(type)) {
		pr_err("[%s] Failed : DVFS Invalid type.0x%x, dvfsmask.0x%x\n",
				__func__, type, dev->dvfsmask);
		return 0;
	}

	return (unsigned long)clk_get_rate(dev->ispclk[type]);
}

int hisp_pwr_nsec_setclkrate(unsigned int type, unsigned int rate)
{
	struct hisp_pwr *dev = &isp_pwr_dev;
	int ret = 0;

	mutex_lock(&dev->pwrlock);
	if (dev->refs_isp_nsec_dst == 0) {
		pr_info("[%s]Failed: isp_nsec_dst.%d, check ISPCPU PowerDown\n",
				__func__, dev->refs_isp_nsec_dst);
		mutex_unlock(&dev->pwrlock);
		return -ENODEV;
	}

	ret = hisp_pwr_setclkrate(type, rate);
	if (ret < 0)
		pr_info("[%s]Failed : hisp_pwr_setclkrate.%d\n", __func__, ret);
	mutex_unlock(&dev->pwrlock);

	return ret;
}

int hisp_pwr_sec_setclkrate(unsigned int type, unsigned int rate)
{
	struct hisp_pwr *dev = &isp_pwr_dev;
	int ret = 0;

	mutex_lock(&dev->pwrlock);
	if (dev->refs_isp_sec_dst == 0) {
		pr_info("[%s]Failed: isp_sec_dst.%d, check ISPCPU PowerDown\n",
				__func__, dev->refs_isp_sec_dst);
		mutex_unlock(&dev->pwrlock);
		return -ENODEV;
	}

	ret = hisp_pwr_setclkrate(type, rate);
	if (ret < 0)
		pr_info("[%s]Failed : hisp_pwr_setclkrate.%d\n", __func__, ret);
	mutex_unlock(&dev->pwrlock);

	return ret;
}

int hisp_qos_cfg_ap(void)
{
	struct hisp_pwr *dev = &isp_pwr_dev;
	void __iomem *vivobus_va;
	unsigned int i = 0;

	pr_info("[%s] +\n", __func__);
#ifdef DEBUG_HISP
	int ret;
	ret = hisp_check_pcie_stat();
	if (ret != ISP_NORMAL_MODE)
		return 0;
#endif

	vivobus_va = hisp_dev_get_regaddr(ISP_VIVOBUS);
	if (vivobus_va == NULL) {
		pr_err("[%s] vivobus_base remap fail\n", __func__);
		return -ENOMEM;
	}
	pr_info("[%s] vivobus_base.%pK, qos_num.%d",
			__func__, vivobus_va, dev->qos.num);

	if ((dev->qos.num == 0) || (dev->qos.num > QOS_MAX_NUM) ||
		(dev->qos.offset == NULL) || (dev->qos.value == NULL)) {
		pr_err("[%s] Failed: QOS_MAX_NUM.%d, offset.%pK, value.%pK\n",
			__func__, QOS_MAX_NUM, dev->qos.offset, dev->qos.value);
		return -ENODEV;
	}

	for (i = 0; i < dev->qos.num; i++) {
		__raw_writel(dev->qos.value[i], vivobus_va + dev->qos.offset[i]);
#ifdef DEBUG_HISP
		pr_info("QOS : value.0x%x, offset.0x%x, actual.0x%x\n",
			dev->qos.value[i], dev->qos.offset[i],
			__raw_readl(vivobus_va + dev->qos.offset[i]));
#endif
	}

	return 0;
}

int hisp_ispcpu_qos_cfg(void)
{
	struct hisp_pwr *dev = &isp_pwr_dev;
	int ret;

#ifdef DEBUG_HISP
	ret = hisp_check_pcie_stat();
	if (ret != ISP_NORMAL_MODE)
		return 0;
#endif

	if(dev->qos.secreg != QOS_NSECREG)
		ret = hisp_smc_qos_cfg();
	else
		ret = hisp_qos_cfg_ap();

	return ret;
}

static void hisp_pwr_ispclk_info_init(void)
{
#ifdef DEBUG_HISP
	struct hisp_pwr *dev = &isp_pwr_dev;
	int ret;

	ret = memset_s(&dev->hisp_clk, sizeof(struct hisp_clk_dump_s),
		0, sizeof(struct hisp_clk_dump_s));
	if (ret != 0)
		pr_err("[%s] Failed : memset_s dev->hisp_clk.%d\n",
				__func__, ret);

	dev->hisp_clk.freqmask = (1 << ISPI2C_CLK);
	dev->hisp_clk.freqmask |= (1 << ISPI3C_CLK);
	dev->hisp_clk.enable = 0;
#else
	return;
#endif
}

static int hisp_pwr_regulator_getdts(struct device *device,
			struct hisp_pwr *dev)
{
	struct device_node *np = device->of_node;
	int ret;

	dev->clockdep_supply = devm_regulator_get(device, "isp-clockdep");
	if (IS_ERR(dev->clockdep_supply)) {
		pr_err("[%s] Failed : isp-clockdep devm_regulator_get.%pK\n",
				__func__, dev->clockdep_supply);
		return -EINVAL;
	}

	dev->ispcore_supply = devm_regulator_get(device, "isp-core");
	if (IS_ERR(dev->ispcore_supply)) {
		pr_err("[%s] Failed : isp-core devm_regulator_get.%pK\n",
				__func__, dev->ispcore_supply);
		return -EINVAL;
	}

	ret = of_property_read_u32(np, "ispcpu-supply-flag",
			(unsigned int *)(&dev->ispcpu_supply_flag));
	if (ret < 0) {
		dev->ispcpu_supply_flag = 0;
		pr_err("[%s] Failed: ispcpu-supply-flag get from dtsi.%d\n",
			__func__, ret);
	}

	if (dev->ispcpu_supply_flag) {
		dev->ispcpu_supply = devm_regulator_get(device, "isp-cpu");
		if (IS_ERR(dev->ispcpu_supply))
			pr_err("[%s] Failed : isp-cpu devm_regulator_get.%pK\n",
					__func__, dev->ispcpu_supply);
	}

	ret = of_property_read_u32(np, "use-buck13-flag",
			(unsigned int *)(&dev->use_buck13_flag));
	if (ret < 0) {
		dev->use_buck13_flag = 0;
		pr_err("[%s] Failed: use-buck13-flag get from dtsi.%d\n",
			__func__, ret);
	}

	ret = of_property_read_u32(np, "use-smmuv3-flag",
				(unsigned int *)(&dev->use_smmuv3_flag));
	if (ret < 0) {
		pr_err("[%s] Failed: use_smmuv3_flag get from dtsi.%d\n",
				__func__, ret);
		return -EINVAL;
	}
	pr_info("[%s] use_smmuv3_flag.0x%x\n", __func__,
			dev->use_smmuv3_flag);

	if (dev->use_smmuv3_flag) {
		dev->isptcu_supply = devm_regulator_get(device, "isp-smmu-tcu");
		if (IS_ERR(dev->isptcu_supply)) {
			pr_err("[%s] Failed : isp-tcu devm_regulator_get.%pK\n",
					__func__, dev->isptcu_supply);
			return -EINVAL;
		}
	}

	return 0;
}

static int hisp_soc_clock_getdts(struct device_node *np,
			struct hisp_pwr *dev)
{
	int ret = 0;

#ifdef CONFIG_CAMERA_USE_HISP230
	unsigned int freq_level;

	freq_level = get_module_freq_level(kernel, peri);
	if (freq_level != FREQ_LEVEL2)
		return 0;

	ret = of_property_read_u32_array(np, CLOCKVALUELOW,
				dev->ispclk_value, dev->clock_num);
	if (ret < 0) {
		pr_err("[%s] Failed: clock-value-low get from dtsi.%d\n",
			 __func__, ret);
		return -EINVAL;
	}
#endif

#ifdef CONFIG_KERNEL_CAMERA_USE_HISP120
	ret = of_property_read_u32_array(np, CLOCKVALUELOW,
				dev->ispclk_value_low, dev->clock_num);
	if (ret < 0) {
		pr_err("[%s] Failed: clock-value-low get from dtsi.%d\n",
			 __func__, ret);
		return -EINVAL;
	}
#endif
	return ret;
}

static int hisp_pwr_clock_getdts(struct device_node *np,
			struct hisp_pwr *dev)
{
	int ret;

	ret = of_property_read_u32(np, "clock-num",
				(unsigned int *)(&dev->clock_num));
	if (ret < 0) {
		pr_err("[%s] Failed: clock-num get from dtsi.%d\n",
				__func__, ret);
		return -EINVAL;
	}

	ret = of_property_read_string_array(np, "clock-names",
				dev->clk_name, dev->clock_num);
	if (ret < 0) {
		pr_err("[%s] Failed : clock-names get from dtsi.%d\n",
				__func__, ret);
		return -EINVAL;
	}

	pr_info("[%s] CLOCKVALUE.%s\n", __func__, CLOCKVALUE);
	ret = of_property_read_u32_array(np, CLOCKVALUE,
				dev->ispclk_value, dev->clock_num);
	if (ret < 0) {
		pr_err("[%s] Failed: clock-value get from dtsi.%d\n",
				__func__, ret);
		return -EINVAL;
	}

	ret = hisp_soc_clock_getdts(np, dev);
	if (ret < 0) {
		pr_err("[%s] Failed: hisp_soc_clock_getdts.%d\n",
				__func__, ret);
		return -EINVAL;
	}

	ret = of_property_read_u32_array(np, "clkdis-dvfs",
				dev->clkdis_dvfs, dev->clock_num);
	if (ret < 0) {
		pr_err("[%s] Failed: clkdis-dvfs get from dtsi.%d\n",
				__func__, ret);
		return -EINVAL;
	}

	ret = of_property_read_u32_array(np, "clkdis-need-div",
				dev->clkdis_need_div, dev->clock_num);
	if (ret < 0)
		pr_err("[%s] Failed: clkdis-need-div get from dtsi.%d\n",
				__func__, ret);

	return 0;
}

static int hisp_pwr_dvfs_getdts(struct device_node *np,
			struct hisp_pwr *dev)
{
	int ret;

	ret = of_property_read_u32(np, "usedvfs",
				(unsigned int *)(&dev->usedvfs));
	if (ret < 0) {
		pr_err("[%s] Failed: usedvfs get from dtsi.%d\n",
				__func__, ret);
		return -EINVAL;
	}
	pr_info("[%s] usedvfs.0x%x\n", __func__, dev->usedvfs);

	if (dev->usedvfs == 0)
		return 0;

	ret = of_property_read_u32_array(np, "clock-value",
			dev->clkdn[HISP_CLK_TURBO], dev->clock_num);
	if (ret < 0) {
		pr_err("[%s] Failed: TURBO get from dtsi.%d\n",
				__func__, ret);
		return -EINVAL;
	}

	ret = of_property_read_u32_array(np, "clkdn-normal",
			dev->clkdn[HISP_CLK_NORMINAL], dev->clock_num);
	if (ret < 0) {
		pr_err("[%s] Failed: NORMINAL get from dtsi.%d\n",
				__func__, ret);
		return -EINVAL;
	}

	ret = of_property_read_u32_array(np, "clkdn-highsvs",
			dev->clkdn[HISP_CLK_HIGHSVS], dev->clock_num);
	if (ret < 0)
		pr_err("[%s]Failed: HIGHSVS get from dtsi.%d\n", __func__, ret);

	ret = of_property_read_u32_array(np, "clkdn-svs",
			dev->clkdn[HISP_CLK_SVS], dev->clock_num);
	if (ret < 0) {
		pr_err("[%s] Failed: SVS get from dtsi.%d\n", __func__, ret);
		return -EINVAL;
	}

	ret = of_property_read_u32_array(np, "clkdn-lowsvs",
			dev->clkdn[HISP_CLK_LOWSVS], dev->clock_num);
	if (ret < 0)
		pr_err("[%s] Failed: LOW SVS get from dtsi.%d\n",
				__func__, ret);

	ret = of_property_read_u32_array(np, "clkdis-dvfs",
			dev->clkdn[HISP_CLK_DISDVFS], dev->clock_num);
	if (ret < 0) {
		pr_err("[%s] Failed: SVS get from dtsi.%d\n", __func__, ret);
		return -EINVAL;
	}

	return 0;
}

static int hisp_pwr_clk_init(struct device *device, struct hisp_pwr *dev)
{
	struct device_node *np = device->of_node;
	int ret;
	unsigned int i;

	for (i = 0; i < dev->clock_num; i++) {
		dev->ispclk[i] = devm_clk_get(device, dev->clk_name[i]);
		if (IS_ERR_OR_NULL(dev->ispclk[i])) {
			pr_err("[%s] Failed : ispclk.%s.%d.%li\n", __func__,
				dev->clk_name[i], i, PTR_ERR(dev->ispclk[i]));
			return -EINVAL;
		}
		pr_info("[%s] ISP clock.%d.%s: %d.%d M\n", __func__, i,
			dev->clk_name[i], dev->ispclk_value[i] / HZ_TO_MHZ_DIV,
			dev->ispclk_value[i] % HZ_TO_MHZ_DIV);

		pr_info("[%s] clkdis.%d.%s: %d.%d M\n", __func__, i,
			dev->clk_name[i], dev->clkdis_dvfs[i] / HZ_TO_MHZ_DIV,
			dev->clkdis_dvfs[i] % HZ_TO_MHZ_DIV);
	}

	ret = of_property_read_u32(np, "ispsmmu-init-byap",
			&dev->ispsmmu_init_byap);
	if (ret < 0) {
		pr_err("[%s] Failed: ispsmmu-init-byap of_property_read_u32.%d\n",
				__func__, ret);
		return -EINVAL;
	}
	pr_info("[%s] isp-smmu-flag.0x%x\n", __func__, dev->ispsmmu_init_byap);

	return 0;
}

static int hisp_pwstat_getdts(struct device_node *np, struct hisp_pwr *dev)
{
	int ret;
	unsigned int pwsta_num = 0;
	unsigned int i;

	ret = of_property_read_u32(np, "pwstat-num",
				(unsigned int *)(&dev->pwstat_num));
	if (ret < 0) {
		pr_err("[%s] Failed: pwstat-num get from dtsi.%d\n",
				__func__, ret);
		return -EINVAL;
	}

	pwsta_num = dev->pwstat_num > PWSTAT_MAX ? PWSTAT_MAX : dev->pwstat_num;
	pr_info("[%s] pwstat_num.0x%x\n", __func__,
			dev->pwstat_num);

	ret = of_property_read_u32_array(np, "pwstat-type",
				dev->pwstat_type, pwsta_num);
	if (ret < 0) {
		pr_err("[%s] Failed: pwstat-type get from dtsi.%d\n",
				__func__, ret);
		return -EINVAL;
	}

	ret = of_property_read_u32_array(np, "pwstat-offset",
				dev->pwstat_offset, pwsta_num);
	if (ret < 0) {
		pr_err("[%s] Failed: pwstat-offset get from dtsi.%d\n",
				__func__, ret);
		return -EINVAL;
	}

	ret = of_property_read_u32_array(np, "pwstat-bit",
				dev->pwstat_bit, pwsta_num);
	if (ret < 0) {
		pr_err("[%s] Failed: pwstat-bit get from dtsi.%d\n",
				__func__, ret);
		return -EINVAL;
	}

	ret = of_property_read_u32_array(np, "pwstat-wanted",
				dev->pwstat_wanted, pwsta_num);
	if (ret < 0) {
		pr_err("[%s] Failed: pwstat-wanted get from dtsi.%d\n",
				__func__, ret);
		return -EINVAL;
	}
	for (i = 0; i < pwsta_num; i++)
		pr_info("[%s] : type.%d, offset.0x%x, bit.%d, wanted[%d]\n",
			__func__, dev->pwstat_type[i], dev->pwstat_offset[i],
			dev->pwstat_bit[i], dev->pwstat_wanted[i]);

	return 0;
}

static void hisp_qos_init(struct hisp_pwr *dev, unsigned int secreg,
	unsigned int num, unsigned int *offset, unsigned int *value)
{
	dev->qos.secreg = secreg;
	dev->qos.num = num;
	dev->qos.offset = offset;
	dev->qos.value = value;
}

static int hisp_qos_getdts(struct device_node *np, struct hisp_pwr *dev)
{
	int ret;
	unsigned int secreg = 0;

	pr_info("[%s] +\n", __func__);

	ret = of_property_read_u32(np, "qos-sec-flag", &secreg);
	if (ret < 0)
		pr_info("[%s] : get qos-sec-flag.%d, ret.%d\n",
			__func__, secreg, ret);

	if (secreg != QOS_NSECREG) {
		hisp_qos_init(dev, secreg, 0, NULL, NULL);
		return 0;
	}

	ret = of_property_read_u32(np, "qos-num", &dev->qos.num);
	if (ret < 0) {
		pr_err("[%s] Failed: qos-num.0x%x get from dtsi.%d\n",
				__func__, dev->qos.num, ret);
		return -ENODEV;
	}
	pr_info("[%s] qos-num.%d\n", __func__, dev->qos.num);

	if ((dev->qos.num == 0) || (dev->qos.num > QOS_MAX_NUM)) {
		pr_err("[%s] Failed: QOS_MAX_NUM.%d\n", __func__, QOS_MAX_NUM);
		return -ENODEV;
	}

	dev->qos.offset = kmalloc_array(dev->qos.num,
			sizeof(unsigned int), GFP_KERNEL);
	if (dev->qos.offset == NULL)
		return -ENOMEM;

	dev->qos.value = kmalloc_array(dev->qos.num,
			sizeof(unsigned int), GFP_KERNEL);
	if (dev->qos.value == NULL)
		goto free_qos;

	ret = of_property_read_u32_array(np, "qos-offset",
			dev->qos.offset, dev->qos.num);
	if (ret < 0) {
		pr_err("[%s] Failed: qos-offset get from dtsi.%d\n",
				__func__, ret);
		goto free_qos;
	}

	ret = of_property_read_u32_array(np, "qos-value",
			dev->qos.value, dev->qos.num);
	if (ret < 0) {
		pr_err("[%s] Failed: qos-value get from dtsi.%d\n",
				__func__, ret);
		goto free_qos;
	}

	hisp_qos_init(dev, QOS_NSECREG, dev->qos.num,
		dev->qos.offset, dev->qos.value);

	return 0;

free_qos:
	if (dev->qos.offset != NULL)
		kfree(dev->qos.offset);
	if (dev->qos.value != NULL)
		kfree(dev->qos.value);
	return -ENOMEM;
}

void hisp_qos_free(struct hisp_pwr *dev)
{
	if (dev->qos.offset != NULL) {
		kfree(dev->qos.offset);
		dev->qos.offset = NULL;
	}
	if (dev->qos.value != NULL) {
		kfree(dev->qos.value);
		dev->qos.value = NULL;
	}
	dev->qos.num = 0;
	dev->qos.secreg = QOS_NSECREG;
}

static int hisp_pwr_getdts(struct platform_device *pdev,
		struct hisp_pwr *dev)
{
	struct device *device = &pdev->dev;
	struct device_node *np = device->of_node;
	int ret;

	if (np == NULL) {
		pr_err("[%s] Failed : np.%pK\n", __func__, np);
		return -ENODEV;
	}

	pr_info("[%s] +\n", __func__);
	ret = hisp_pwr_regulator_getdts(device, dev);
	if (ret < 0) {
		pr_err("[%s] Failed: regulator_getdts.%d\n", __func__, ret);
		return ret;
	}

	ret = hisp_pwr_clock_getdts(np, dev);
	if (ret < 0) {
		pr_err("[%s] Failed: clock_getdts.%d\n", __func__, ret);
		return ret;
	}

	ret = hisp_pwr_dvfs_getdts(np, dev);
	if (ret < 0) {
		pr_err("[%s] Failed: hisp_pwr_dvfs_getdts.%d\n", __func__, ret);
		return ret;
	}

	ret = hisp_pwr_clk_init(device, dev);
	if (ret != 0) {
		pr_err("[%s] Failed : hisp_pwr_clk_init.%d.\n", __func__, ret);
		return ret;
	}

	ret = hisp_pwstat_getdts(np, dev);
	if (ret != 0) {
		pr_err("[%s] Failed : hisp_pwstat_getdts.%d.\n", __func__, ret);
		return ret;
	}

	ret = hisp_qos_getdts(np, dev);
	if (ret != 0) {
		pr_err("[%s] Failed : hisp_qos_getdts.%d.\n", __func__, ret);
		return ret;
	}

	pr_info("[%s] -\n", __func__);

	return 0;
}

int hisp_pwr_probe(struct platform_device *pdev)
{
	struct hisp_pwr *dev = &isp_pwr_dev;
	int ret = 0;

	pr_alert("[%s] +\n", __func__);
	dev->device = &pdev->dev;
	dev->ispcpu_supply_flag = 0;
	dev->refs_isp_power = 0;
	dev->refs_isp_nsec_init = 0;
	dev->refs_isp_sec_init = 0;
	dev->refs_ispcpu_init = 0;
	dev->refs_isp_nsec_dst = 0;
	dev->refs_isp_sec_dst = 0;

	ret = hisp_pwr_getdts(pdev, dev);
	if (ret != 0) {
		pr_err("[%s] Failed : hisp_nsec_getdts.%d.\n",
				__func__, ret);
		return ret;
	}

	dev->dvfsmask = (1 << ISPI2C_CLK);
	dev->dvfsmask |= (1 << ISPI3C_CLK);
	mutex_init(&dev->pwrlock);

	hisp_pwr_ispclk_info_init();

	pr_alert("[%s] -\n", __func__);

	return 0;
}

int hisp_pwr_remove(struct platform_device *pdev)
{
	struct hisp_pwr *dev = &isp_pwr_dev;
#ifdef DEBUG_HISP
	int ret;
#endif

	hisp_qos_free(dev);
#ifdef DEBUG_HISP
	ret = memset_s(&dev->hisp_clk, sizeof(struct hisp_clk_dump_s),
		0, sizeof(struct hisp_clk_dump_s));
	if (ret != 0)
		pr_err("[%s] Failed : memset_s rproc_dev->hisp_clk.%d\n",
			__func__, ret);
#endif

	mutex_destroy(&dev->pwrlock);
	return 0;
}

#ifdef DEBUG_HISP
#define ISPCPU_CLK_DEBUG  "ispcpu"
#define ISPFUNC_CLK_DEBUG "ispfunc"
#define TMP_SIZE 0x1000

static unsigned int hisp_pwr_get_ispclk_enable(void)
{
	struct hisp_pwr *dev = &isp_pwr_dev;

	return dev->hisp_clk.enable;
}

static int hisp_set_ispclk_enable(int state)
{
	struct hisp_pwr *dev = &isp_pwr_dev;

	if (state != 0 && state != 1) {/*lint !e774 */
		pr_err("[%s] Failed : state.%x\n", __func__, state);
		return -EINVAL;
	}

	dev->hisp_clk.enable = state;

	return 0;
}

static int hisp_pwr_set_ispclk_freq(unsigned int type, unsigned long value)
{
	struct hisp_pwr *dev = &isp_pwr_dev;

	if (type >= ISP_CLK_MAX || ((1 << type) & dev->hisp_clk.freqmask)) {
		pr_err("[%s] Failed : type.%d, freqmask.0x%x\n",
			__func__, type, dev->hisp_clk.freqmask);
		return -EINVAL;
	}

	dev->hisp_clk.freq[type] = value;
	pr_info("[%s] freq.%ld -> type.%d\n", __func__, value, type);

	return 0;
}

static unsigned long hisp_pwr_get_ispclk_freq(unsigned int type)
{
	struct hisp_pwr *dev = &isp_pwr_dev;
	unsigned long freq = 0;

	if (hisp_pwr_get_ispclk_enable() == 0)
		return 0;

	if (type >= ISP_CLK_MAX || ((1 << type) & dev->hisp_clk.freqmask)) {
		pr_err("[%s] Failed : type.%d, freqmask.0x%x\n",
			__func__, type, dev->hisp_clk.freqmask);
		return 0;
	}
	freq = dev->hisp_clk.freq[type];
	pr_info("[%s] type.%d freq.%ld\n", __func__, type, freq);
	return freq;
}

static int hisp_clk_show_status(char *tmp, ssize_t *size)
{
	int ret = 0;

	if (!hisp_pwr_get_ispclk_enable()) {
		pr_err("[%s] debug isp clk not enable!\n", __func__);
		return -EINVAL;
	}

	if (!hisp_rproc_enable_status()) {
		pr_err("[%s] ispcpu not boot!\n", __func__);
		return -EINVAL;
	}

	ret = hisp_pwr_get_isppwr_state(PWSTAT_MEDIA);
	if (ret == 0) {
		pr_err("[%s] failed: hisp_pwr_get_isppwr_state\n", __func__);
		return -EINVAL;
	}

	ret = snprintf_s(tmp, NONSEC_MAX_SIZE, NONSEC_MAX_SIZE-1,
			   "\n Isp Clk Enable: %s\n",
			   (hisp_pwr_get_ispclk_enable() ? "enable":"disable"));
	if (ret < 0) {
		pr_err("[%s] failed: isp clk!\n", __func__);
		return -EINVAL;
	}

	*size += ret;

	ret = snprintf_s(*size + tmp, NONSEC_MAX_SIZE,
			   NONSEC_MAX_SIZE-1, "%s\n", "NOW THE CLOK SET:");
	if (ret < 0) {
		pr_err("[%s] failed: NOW THE CLOCK!\n", __func__);
		return -EINVAL;
	}
	*size += ret;

	return 0;
}

static int hisp_clk_show_ispcore(char *tmp, ssize_t *size)
{
	int ret = 0;
	unsigned char index = 0;

	ret = snprintf_s(tmp + *size, NONSEC_MAX_SIZE, NONSEC_MAX_SIZE - 1,
			   "1. ISPCPU CLK:%ld   Real CLK:%ld\n",
			   hisp_pwr_get_ispclk_freq(ISPCPU_CLK),
			   hisp_pwr_getclkrate(ISPCPU_CLK));
	if (ret < 0) {
		pr_err("[%s] failed: NOW THE CLOCK!\n", __func__);
		return -EINVAL;
	}
	*size += ret;

	ret = snprintf_s(tmp + *size, NONSEC_MAX_SIZE, NONSEC_MAX_SIZE - 1,
			   "2. ISPFUNC CLK:%ld  Real CLK:%ld\n",
			   hisp_pwr_get_ispclk_freq(ISPFUNC_CLK),
			   hisp_pwr_getclkrate(ISPFUNC_CLK));
	if (ret < 0) {
		pr_err("[%s] failed: ISPFUNC CLOCK!\n", __func__);
		return -EINVAL;
	}
	*size += ret;

	for (index = ISPFUNC2_CLK; index < ISP_SYS_CLK; index++) {
		ret = snprintf_s(tmp + *size, NONSEC_MAX_SIZE,
			NONSEC_MAX_SIZE - 1,
			"2. ISPFUNC CLK%d: %ld  Real CLK:%ld\n",
			(index - 1), hisp_pwr_get_ispclk_freq(index),
			hisp_pwr_getclkrate(index));
		if (ret < 0) {
			pr_err("[%s]failed:ISPFUNC.%d!\n", __func__, index - 1);
			return -EINVAL;
		}
		*size += ret;
	}

	return 0;
}

ssize_t hisp_clk_show(struct device *pdev,
		struct device_attribute *attr, char *buf)
{
	ssize_t size = 0;
	int ret = 0;
	char *tmp = NULL;

	pr_info("[%s] +\n", __func__);

	tmp = kzalloc(TMP_SIZE, GFP_KERNEL);
	if (tmp == NULL)
		return 0;

	ret = hisp_clk_show_status(tmp, &size);
	if (ret < 0) {
		pr_err("[%s] failed: hisp_clk_show_status!\n", __func__);
		goto show_err;
	}

	ret = hisp_clk_show_ispcore(tmp, &size);
	if (ret < 0) {
		pr_err("[%s] failed: hisp_clk_show_ispcore!\n", __func__);
		goto show_err;
	}

show_err:
	ret = memcpy_s((void *)buf, TMP_SIZE, (void *)tmp, TMP_SIZE);
	if (ret != 0)
		pr_err("[%s] Failed : memcpy_s buf from tmp fail\n", __func__);

	kfree((void *)tmp);

	pr_info("[%s] -\n", __func__);
	return size;
}

static int hisp_clk_store_freq(const char *buf, size_t count)
{
	char *p = NULL;
	size_t len = 0;
	unsigned long freq_value = 0;
	int ret;

	p = memchr(buf, ':', count);
	if (p == NULL) {
		pr_err("[%s] Failed buf cmd!\n", __func__);
		return -EINVAL;
	}
	len = (size_t)(p - buf);
	if (len == 0)
		return -EINVAL;
	p += 1;

	ret = kstrtoul(p, 10, &freq_value);
	pr_info("[%s] freq_value.%ld,ret.%d\n", __func__, freq_value, ret);
	if (ret < 0)
		return -EINVAL;
	if (strncmp(buf, ISPCPU_CLK_DEBUG, len) == 0) {
		hisp_pwr_set_ispclk_freq(ISPCPU_CLK, freq_value);
		pr_info("[%s] Ispcpu Clk: %ld\n", __func__, freq_value);
	} else if (strncmp(buf, ISPFUNC_CLK_DEBUG, len) == 0) {
		hisp_pwr_set_ispclk_freq(ISPFUNC_CLK, freq_value);
		pr_info("[%s] Ispfunc Clk:%ld\n", __func__, freq_value);
	} else {
		p = strstr(buf, ISPFUNC_CLK_DEBUG);
		if (p != NULL) {
			p += strlen(ISPFUNC_CLK_DEBUG);
			pr_info("[%s] len.%lu buf.%s\n",
				__func__, strlen(ISPFUNC_CLK_DEBUG), p);
			hisp_pwr_set_ispclk_freq(*p - '2' + ISPFUNC2_CLK,
					freq_value);
			pr_info("[%s] Ispfunc%c Clk.%ld\n",
				__func__, *p, freq_value);
		}
	}

	return 0;
}

ssize_t hisp_clk_store(struct device *pdev, struct device_attribute *attr,
		const char *buf, size_t count)
{
	char *p = NULL;
	size_t len = 0;
	char index = 0;
	int ret;

	p = memchr(buf, '\n', count);
	if (p == NULL) {
		pr_err("[%s] Failed p.%pK\n", __func__, p);
		return -EINVAL;
	}

	len = (size_t)(p - buf);
	if (len == 0)
		return -EINVAL;

	if (!strncmp(buf, "enable", len)) {
		hisp_set_ispclk_enable(1);
	} else if (!strncmp(buf, "disable", len)) {
		hisp_set_ispclk_enable(0);
		for (index = ISPCPU_CLK; index < ISP_CLK_MAX; index++)
			hisp_pwr_set_ispclk_freq(ISPCPU_CLK, 0);
	} else {

		ret = hisp_clk_store_freq(buf, count);
		if (ret < 0) {
			pr_err("[%s] Failed buf cmd!\n", __func__);
			return -EINVAL;
		}
	}

	return count;/*lint !e713 */
}
#endif

