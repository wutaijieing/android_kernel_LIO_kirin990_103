/*
 * soundwire.c -- soundwire driver
 *
 * Copyright (c) 2012-2021 Huawei Technologies Co., Ltd.
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

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/printk.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/delay.h>
#include <linux/sort.h>
#include <linux/jiffies.h>
#include <linux/pm_runtime.h>
#include <linux/platform_device.h>
#include <rdr_audio_adapter.h>
#include "soc_asp_cfg_interface.h"
#include "soundwire_type.h"
#include "soundwire_master_reg.h"
#include "soundwire_slave_reg.h"
#include "soundwire_utils.h"
#include "soundwire_bandwidth_adp.h"
#include "soundwire_irq.h"

#include "audio_log.h"
#include "platform_base_addr_info.h"

#define LOG_TAG "soundwire"

#define WAIT_CNT 200
#define hm_en(n) (0x10001U << (n))
#define DP10_FIFO_FULL_CNT 128
#define FIFO_FULL_CNT 64
#define DP10_FIFO_OFFSET 0x900
#define DP11_FIFO_OFFSET 0xB00
#define DP12_FIFO_OFFSET 0xC00
#define DP13_FIFO_OFFSET 0xD00
#define DP14_FIFO_OFFSET 0xE00

#ifdef CONFIG_AUDIO_COMMON_IMAGE
#undef SOC_ACPU_ASP_CFG_BASE_ADDR
#define SOC_ACPU_ASP_CFG_BASE_ADDR 0
#endif

static struct soundwire_priv *g_priv = NULL;

static const int g_enum_irqs[] = {
	IRQ_MSYNC_TIMEOUT,
	IRQ_SLV_ATTACHED,
	IRQ_ENUM_TIMEOUT,
	IRQ_ENUM_FINISHED
};

enum clkswitch_stat {
	M_NORMAL = 0b0001,
	M_STOPPING = 0b0011,
	M_STOOPED = 0b0010,
	M_STOP_RESTARTING = 0b0110,
	M_SWITCHING = 0b0111,
	M_SWITCHED = 0b0101,
	M_SWITCH_RESTARTING = 0b0100,
	S_NORMAL = 0b1100,
	S_STOPPING = 0b1101,
	S_STOPPED = 0b1111,
	S_STOP_RESTARTING = 0b1110,
	S_SWITCHING = 0b1010,
	S_SWITCHED = 0b1011,
	S_SWITCH_RESTARTING = 0b1001
};

static void bus_reset(struct soundwire_priv *priv)
{
	MST_CLKEN4_UNION mst_clken4;

	mst_clken4.value = mst_read_reg(priv, MST_CLKEN4_OFFSET);
	mst_clken4.reg.bus_rst_en = 1;
	mst_write_reg(priv, mst_clken4.value, MST_CLKEN4_OFFSET);
}

static void reset_dp(struct soundwire_priv *priv)
{
	/* dp reset reassertion */
	mst_write_reg(priv, 0xff, MST_RSTEN1_OFFSET);
	mst_write_reg(priv, 0xff, MST_RSTEN2_OFFSET);
	mst_write_reg(priv, 0xff, MST_RSTEN3_OFFSET);

	/* dp fifo reset reassertion */
	mst_write_reg(priv, 0xff, MST_RSTEN6_OFFSET);
	mst_write_reg(priv, 0xff, MST_RSTEN7_OFFSET);
	mst_write_reg(priv, 0xff, MST_RSTEN8_OFFSET);
}

static void enable_sub_module(struct soundwire_priv *priv)
{
	MST_CLKEN1_UNION mst_clken1;

	mst_clken1.value = mst_read_reg(priv, MST_CLKEN1_OFFSET);
	mst_clken1.reg.frame_ctrl_clken = 1;
	mst_clken1.reg.data_port_clken = 1;
	mst_clken1.reg.rx0_clken = 1;
	mst_clken1.reg.tx0_clken = 1;
	mst_clken1.reg.rx1_clken = 1;
	mst_clken1.reg.tx1_clken = 1;
	mst_write_reg(priv, mst_clken1.value, MST_CLKEN1_OFFSET);

	/* open all dp clk */
	mst_write_reg(priv, 0xff, MST_CLKEN2_OFFSET);
	mst_write_reg(priv, 0xff, MST_CLKEN3_OFFSET);
	mst_write_reg(priv, 0xff, MST_CLKEN6_OFFSET);
	mst_write_reg(priv, 0xff, MST_CLKEN7_OFFSET);
	mst_write_reg(priv, 0xff, MST_CLKEN8_OFFSET);
	reset_dp(priv);
}

static void enable_soundwire_hclk(struct soundwire_priv *priv)
{
	SOC_ASP_CFG_R_GATE1_EN_ADDR_UNION gate1_en;

	/* soundwire hclk enable */
	gate1_en.value = asp_cfg_read_reg(priv, SOC_ASP_CFG_R_GATE1_EN_ADDR_ADDR(0));
#ifdef SOC_ASP_CFG_R_GATE1_EN_ADDR_enable_gt_swire_hclk_START
	gate1_en.reg.enable_gt_swire_hclk = 1;
#else
	gate1_en.reg.gt_swire_hclk = 1;
#endif
	asp_cfg_write_reg(priv, gate1_en.value, SOC_ASP_CFG_R_GATE1_EN_ADDR_ADDR(0));
}

static void asp_cfg_init(struct soundwire_priv *priv)
{
	SOC_ASP_CFG_R_RST_CTRLDIS_UNION rst_ctrldis;
	SOC_ASP_CFG_R_GATE_CLKDIV_EN_UNION clkdiv_en;
	SOC_ASP_CFG_R_INTR_NS_EN_UNION intr_ns_en;

	/* soundwire ahb quit soft reset */
	rst_ctrldis.value = asp_cfg_read_reg(priv, SOC_ASP_CFG_R_RST_CTRLDIS_ADDR(0));
#ifdef SOC_ASP_CFG_R_RST_CTRLDIS_disable_rst_swire_ahb_n_START
	rst_ctrldis.reg.disable_rst_swire_ahb_n = 1;
#else
	rst_ctrldis.reg.rst_swire_ahb_n = 1;
#endif
	asp_cfg_write_reg(priv, rst_ctrldis.value, SOC_ASP_CFG_R_RST_CTRLDIS_ADDR(0));

	/* sio clk 0x1c */
	clkdiv_en.value = asp_cfg_read_reg(priv, SOC_ASP_CFG_R_GATE_CLKDIV_EN_ADDR(0));
	clkdiv_en.reg.gt_siobclk_div = 1;
	asp_cfg_write_reg(priv, clkdiv_en.value, SOC_ASP_CFG_R_GATE_CLKDIV_EN_ADDR(0));

	/* sel slimbus base */
	asp_cfg_write_reg(priv, hm_en(SOC_ASP_CFG_R_CLK_SEL_slimbus_base_clk_sel_START), SOC_ASP_CFG_R_CLK_SEL_ADDR(0));

	enable_soundwire_hclk(priv);

	/* auto clk enable */
	asp_cfg_write_reg(priv, 0x0fff0fff, SOC_ASP_CFG_R_CG_EN_ADDR(0));

	/* soundwire intr ns en */
	intr_ns_en.value = asp_cfg_read_reg(priv, SOC_ASP_CFG_R_INTR_NS_EN_ADDR(0));
#ifdef SOC_ASP_CFG_R_INTR_NS_EN_swire_int_ns_mask_START
	intr_ns_en.reg.swire_int_ns_mask = 1;
#else
	intr_ns_en.reg.swire_int_ns = 1;
#endif
	asp_cfg_write_reg(priv, intr_ns_en.value, SOC_ASP_CFG_R_INTR_NS_EN_ADDR(0));
}

static bool is_clk_switch_success(enum clkswitch_stat stat)
{
	MST_TEST_MONITOR_UNION mst_test_monitor;
	int cnt = WAIT_CNT;

	while (cnt) {
		mst_test_monitor.value = mst_read_reg(g_priv, MST_TEST_MONITOR_OFFSET);
		if (mst_test_monitor.reg.clkswitch_cs == stat)
			return true;
		udelay(25);
		cnt--;
	}

	AUDIO_LOGE("switch to slave failed, clk state = %d", mst_test_monitor.reg.clkswitch_cs);
	return false;
}

static int switch_master_to_slave(void)
{
	MST_TEST_MONITOR_UNION mst_test_monitor;
	SLV_SCP_Stat_UNION slv_scp_stat;

	mst_test_monitor.value = mst_read_reg(g_priv, MST_TEST_MONITOR_OFFSET);
	if (mst_test_monitor.reg.clkswitch_cs == S_NORMAL) {
		AUDIO_LOGW("clk is already in slave");
		return 0;
	}

	if (mst_test_monitor.reg.clkswitch_cs != M_NORMAL) {
		AUDIO_LOGE("clk not in mst, can't switch, clk state: %d", mst_test_monitor.reg.clkswitch_cs);
		return -EFAULT;
	}

	slv_scp_stat.value = slv_read_reg(g_priv, SLV_SCP_Stat_OFFSET);
	slv_scp_stat.reg.m2s_clockswitch_now = 1;
	slv_write_reg(g_priv, slv_scp_stat.value, SLV_SCP_Stat_OFFSET);
	if (is_clk_switch_success(S_NORMAL)) {
		AUDIO_LOGI("switch to slave success");
		return 0;
	}

	return -EFAULT;
}

static bool is_slv_clock_prepared(void)
{
	SLV_SCP_Stat_UNION slv_scp_stat;
	int cnt = WAIT_CNT;

	while (cnt) {
		slv_scp_stat.value = slv_read_reg(g_priv, SLV_SCP_Stat_OFFSET);
		/* prepare ok */
		if (slv_scp_stat.reg.clockstop_notfinished == 0) {
			AUDIO_LOGI("clk stop prepare ok");
			return true;
		}
		udelay(25);
		cnt--;
	}

	AUDIO_LOGE("clk stop prepare not ok, ret = %d", slv_scp_stat.reg.clockstop_notfinished);
	return false;
}

static int switch_slave_to_master(void)
{
	MST_TEST_MONITOR_UNION mst_test_monitor;
	SLV_SCP_SystemCtrl_UNION slv_scp_system_ctrl;
	SLV_SCP_Stat_UNION slv_scp_stat;

	mst_test_monitor.value = mst_read_reg(g_priv, MST_TEST_MONITOR_OFFSET);
	if (mst_test_monitor.reg.clkswitch_cs == M_NORMAL) {
		AUDIO_LOGW("clk is already in master");
		return 0;
	}

	if (mst_test_monitor.reg.clkswitch_cs != S_NORMAL) {
		AUDIO_LOGE("clk not in slv, can't switch, clk state: %d", mst_test_monitor.reg.clkswitch_cs);
		return -EFAULT;
	}

	slv_scp_system_ctrl.value = slv_read_reg(g_priv, SLV_SCP_SystemCtrl_OFFSET);
	slv_scp_system_ctrl.reg.wakeupenable = 1;
	slv_scp_system_ctrl.reg.clockstopprepare = 1;
	slv_write_reg(g_priv, slv_scp_system_ctrl.value, SLV_SCP_SystemCtrl_OFFSET);

	if (!is_slv_clock_prepared())
		return -EFAULT;

	slv_scp_stat.value = slv_read_reg(g_priv, SLV_SCP_Stat_OFFSET);
	slv_scp_stat.reg.s2m_clockswitch_now = 1;
	slv_write_reg(g_priv, slv_scp_stat.value, SLV_SCP_Stat_OFFSET);

	if (!is_clk_switch_success(S_SWITCHED))
		return -EFAULT;

	mst_write_reg(g_priv, 0x1, MST_CLK_WAKEUP_EN_OFFSET);
	if (!is_clk_switch_success(M_NORMAL))
		return -EFAULT;

	AUDIO_LOGI("switch to master success");
	return 0;
}

static int soundwire_switch_framer(enum framer_type framer_type)
{
	int ret;

	if (g_priv == NULL)
		return -EFAULT;

	if (framer_type >= FRAMER_NUM) {
		AUDIO_LOGE("frame type invalid = %d", framer_type);
		return -EFAULT;
	}

	if (framer_type == g_priv->cur_frame) {
		AUDIO_LOGI("same to cur frame = %d, skip", framer_type);
		return 0;
	}

	if (framer_type == FRAMER_SOC)
		ret = switch_slave_to_master();
	else
		ret = switch_master_to_slave();

	if (ret != 0) {
		AUDIO_LOGE("switch framer to %s failed", (framer_type == FRAMER_SOC) ? "soc" : "codec");
		return ret;
	}

	g_priv->cur_frame = framer_type;
	return 0;
}

static void enum_cfg_init(struct soundwire_priv *priv)
{
	MST_CTRL1_UNION mst_ctrl1;
	MST_CTRL2_UNION mst_ctrl2;
	MST_CLKEN4_UNION mst_clken4;
	SOC_ASP_CFG_R_SW_SLIM_DMA_SEL_UNION sw_slim_dma_sel;

	/* sel dma_req to soundwire */
	sw_slim_dma_sel.value = asp_cfg_read_reg(priv, SOC_ASP_CFG_R_SW_SLIM_DMA_SEL_ADDR(0));
	sw_slim_dma_sel.reg.sw_slim_dma_sel = 0x41ff;
	asp_cfg_write_reg(priv, sw_slim_dma_sel.value, SOC_ASP_CFG_R_SW_SLIM_DMA_SEL_ADDR(0));

	/* quit bus reset */
	mst_clken4.value = mst_read_reg(priv, MST_CLKEN4_OFFSET);
	mst_clken4.reg.bus_rst_en = 0;
	mst_write_reg(priv, mst_clken4.value, MST_CLKEN4_OFFSET);

	mst_ctrl1.value = mst_read_reg(priv, MST_CTRL1_OFFSET);
	mst_ctrl1.reg.sync_timeout_en = 1;
	mst_ctrl1.reg.frame_ctrl_en = 1;
	mst_write_reg(priv, mst_ctrl1.value, MST_CTRL1_OFFSET);

	mst_ctrl2.value = mst_read_reg(priv, MST_CTRL2_OFFSET);
	mst_ctrl2.reg.enum_timeout_en = 1;
	mst_write_reg(priv, mst_ctrl2.value, MST_CTRL2_OFFSET);

	soundwire_enable_mst_intrs(priv, g_enum_irqs, ARRAY_SIZE(g_enum_irqs));
}

static void enum_cfg_deinit(struct soundwire_priv *priv)
{
	MST_CTRL1_UNION mst_ctrl1;
	MST_CTRL2_UNION mst_ctrl2;

	soundwire_disable_mst_intrs(priv, g_enum_irqs, ARRAY_SIZE(g_enum_irqs));

	mst_ctrl1.value = mst_read_reg(priv, MST_CTRL1_OFFSET);
	mst_ctrl1.reg.sync_timeout_en = 0;
	mst_write_reg(priv, mst_ctrl1.value, MST_CTRL1_OFFSET);

	mst_ctrl2.value = mst_read_reg(priv, MST_CTRL2_OFFSET);
	mst_ctrl2.reg.enum_timeout_en = 0;
	mst_write_reg(priv, mst_ctrl2.value, MST_CTRL2_OFFSET);
}

static void fill_up_the_fifo(struct soundwire_priv *priv)
{
	unsigned int i;

	for (i = 0; i < DP10_FIFO_FULL_CNT; i++)
		write_fifo(priv, DP10_FIFO_OFFSET, 0x0);

	for (i = 0; i < FIFO_FULL_CNT; i++) {
		write_fifo(priv, DP11_FIFO_OFFSET, 0x0);
		write_fifo(priv, DP12_FIFO_OFFSET, 0x0);
		write_fifo(priv, DP13_FIFO_OFFSET, 0x0);
		write_fifo(priv, DP14_FIFO_OFFSET, 0x0);
	}
}

int soundwire_enum_device(void)
{
	unsigned long time_left;

	if (g_priv == NULL)
		return 0;

	reinit_completion(&g_priv->enum_comp);
	enum_cfg_init(g_priv);
	time_left = wait_for_completion_timeout(&g_priv->enum_comp, msecs_to_jiffies(40));
	if (time_left == 0) {
		enum_cfg_deinit(g_priv);
		AUDIO_LOGE("enum device failed");
		return -ETIMEDOUT;
	}

	/* full fifo, solve the pop tone */
	fill_up_the_fifo(g_priv);

	enum_cfg_deinit(g_priv);

	AUDIO_LOGI("enum ok, slave status = 0x%x, time_left = %ld", \
		mst_read_reg(g_priv, MST_SLV_STAT0_OFFSET), time_left);

	return 0;
}

enum framer_type soundwire_get_framer(void)
{
	return g_priv != NULL ? g_priv->cur_frame : FRAMER_SOC;
}

static int soundwire_runtime_get(void)
{
	int pm_ret;

	if (g_priv != NULL && g_priv->dev != NULL) {
		if (is_pm_runtime_support()) {
			pm_ret = pm_runtime_get_sync(g_priv->dev);
			if (pm_ret < 0) {
				AUDIO_LOGE("pm resume error, pm_ret: %d", pm_ret);
#ifdef CONFIG_DFX_BB
				rdr_system_error(RDR_AUDIO_RUNTIME_SYNC_FAIL_MODID, 0, 0);
#endif
				return pm_ret;
			}
			return 0;
		}
		return 0;
	}

	return -EFAULT;
}

static void soundwire_runtime_put(void)
{
	if (g_priv != NULL && g_priv->dev != NULL) {
		if (is_pm_runtime_support()) {
			pm_runtime_mark_last_busy(g_priv->dev);
			pm_runtime_put_autosuspend(g_priv->dev);
		}
	}
}

static int port_cmp(const void* a, const void* b)
{
	return *((enum data_port_type *)a) - *((enum data_port_type *)b);
}

static int32_t convert_to_payloads(const struct scene_param *params,
	struct soundwire_payload *payloads, int32_t *payloads_num)
{
	uint32_t i;

	if (params == NULL || params->channels >= DATA_PORTS_MAX) {
		AUDIO_LOGE("invalid scene params");
		return -EINVAL;
	}

	sort((void *)params->ports, params->channels, sizeof(enum data_port_type), port_cmp, NULL);

	for (i = 0; i < params->channels; i++) {
		uint8_t dp_id = (params->ports[i] + 1) / 2;
		int32_t payload_idx = *payloads_num - 1;

		if (payload_idx < 0 || payloads[payload_idx].dp_id != dp_id) {
			(*payloads_num)++;
			payload_idx++;
			payloads[payload_idx].dp_id = dp_id;
			payloads[payload_idx].bits_per_sample = params->bits_per_sample;
			payloads[payload_idx].sample_rate = params->rate;
			payloads[payload_idx].priority = params->priority;
		}

		payloads[payload_idx].channel_num++;
		payloads[payload_idx].channel_mask |= (1 << ((params->ports[i] + 1) % 2));

		if (payloads[payload_idx].channel_num > 2)
			return -EINVAL;
	}

	return 0;
}

static int soundwire_activate(const char *scene_name, struct scene_param *params)
{
	int payloads_num = 0;
	struct soundwire_payload payloads[DATA_PORTS_MAX] = {{0}};
	int ret;

	AUDIO_LOGI("activate scene %s", scene_name);

	if (g_priv == NULL) {
		AUDIO_LOGE("bus init failed, can't activate");
		return 0;
	}

	if (!soundwire_bandwidth_adp_is_active()) {
		set_bus_active(true);

		ret = soundwire_runtime_get();
		if (ret < 0)
			return ret;

		ret = soundwire_switch_framer(FRAMER_CODEC);
		if (ret) {
			AUDIO_LOGE("soundwire switch framer to codec failed");
			return ret;
		}
	}

	if (convert_to_payloads(params, payloads, &payloads_num) != 0)
		return -EINVAL;

	return soundwire_bandwidth_adp_add(payloads, payloads_num);
}

static int soundwire_deactivate(const char *scene_name, struct scene_param *params)
{
	int payloads_num = 0;
	struct soundwire_payload payloads[DATA_PORTS_MAX] = {{0}};
	int ret;

	AUDIO_LOGI("deactivate scene %s", scene_name);

	if (g_priv == NULL) {
		AUDIO_LOGE("bus init failed, can't deactivate");
		return 0;
	}

	if (convert_to_payloads(params, payloads, &payloads_num) != 0)
		return -EINVAL;

	ret = soundwire_bandwidth_adp_del(payloads, payloads_num);
	if (ret != 0) {
		AUDIO_LOGE("bandwidth del failed");
		return ret;
	}

	if (!soundwire_bandwidth_adp_is_active()) {
		soundwire_runtime_put();
		ret = soundwire_switch_framer(FRAMER_SOC);
		if (ret) {
			AUDIO_LOGE("soundwire switch framer to soc failed");
			return ret;
		}
		set_bus_active(false);
	}
	return ret;
}

static int soundwire_suspend(struct device *dev)
{
	int ret;
	struct platform_device *pdev = to_platform_device(dev);
	struct soundwire_priv *priv = platform_get_drvdata(pdev);

	AUDIO_LOGI("soundwire suspend");

	ret = soundwire_runtime_get();
	if (ret < 0)
		return ret;

	if (is_bus_active()) {
		AUDIO_LOGI("soundwire is active, continue");
		return 0;
	}

	ret = soundwire_switch_framer(FRAMER_SOC);
	if (ret != 0) {
		AUDIO_LOGE("switch to soc failed");
		return ret;
	}

	bus_reset(priv);

	return 0;
}

static int soundwire_resume(struct device *dev)
{
	int ret = 0;
	struct platform_device *pdev = to_platform_device(dev);
	struct soundwire_priv *priv = platform_get_drvdata(pdev);

	AUDIO_LOGI("soundwire resume");

	if (is_bus_active()) {
		AUDIO_LOGI("soundwire is active, continue");
		goto exit;
	}

	asp_cfg_init(priv);
	enable_sub_module(priv);
	soundwire_reset_irq(priv);

	ret = soundwire_enum_device();
	if (ret != 0)
		AUDIO_LOGE("enum device failed");

exit:
	soundwire_runtime_put();
	return ret;
}

static int soundwire_runtime_suspend(struct device *dev)
{
	return 0;
}

static int soundwire_runtime_resume(struct device *dev)
{
	return 0;
}

static void pm_init(struct device *dev)
{
	if (is_pm_runtime_support()) {
		pm_runtime_use_autosuspend(dev);
		/* 2000ms */
		pm_runtime_set_autosuspend_delay(dev, 2000);
		pm_runtime_mark_last_busy(dev);
		pm_runtime_set_active(dev);
		pm_runtime_enable(dev);
	}
}

static int reg_remap_init(struct soundwire_priv *priv, struct device *dev)
{
	/* 1k */
	priv->asp_cfg_mem = devm_ioremap(dev,
		get_phy_addr(SOC_ACPU_ASP_CFG_BASE_ADDR, PLT_ASP_CFG_MEM), 0x400);
	if (priv->asp_cfg_mem == NULL) {
		AUDIO_LOGE("no memory for asp cfg");
		return -ENOMEM;
	}
	/* 4k */
	priv->sdw_mst_mem = devm_ioremap(dev,
		get_phy_addr(SOC_ACPU_SoundWire_BASE_ADDR + 0x3000, PLT_SOUNDWIRE_MEM), 0x1000);
	if (priv->sdw_mst_mem == NULL) {
		AUDIO_LOGE("no memory for sdw mst");
		return -ENOMEM;
	}
	/* 4k */
	priv->sdw_slv_mem = devm_ioremap(dev,
		get_phy_addr(SOC_ACPU_SoundWire_BASE_ADDR + 0x2000, PLT_SOUNDWIRE_MEM), 0x1000);
	if (priv->sdw_slv_mem == NULL) {
		AUDIO_LOGE("no memory for sdw slv");
		return -ENOMEM;
	}
	/* 4k */
	priv->sdw_fifo_mem = devm_ioremap(dev,
		get_phy_addr(SOC_ACPU_SoundWire_BASE_ADDR, PLT_SOUNDWIRE_MEM), 0x1000);
	if (priv->sdw_fifo_mem == NULL) {
		AUDIO_LOGE("no memory for sdw fifo");
		return -ENOMEM;
	}

	return 0;
}

static int resource_init(struct platform_device *pdev, struct soundwire_priv *priv)
{
	int ret;

	g_priv = priv;
	g_priv->dev = &pdev->dev;
	g_priv->cur_frame = FRAMER_SOC;

	ret = soundwire_irq_init(priv);
	if (ret != 0) {
		AUDIO_LOGE("soundwire irq failed ret = %d", ret);
		return ret;
	}

	init_completion(&priv->enum_comp);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
	priv->wake_lock = wakeup_source_register(g_priv->dev, "soundwire-irq");
#else
	priv->wake_lock = wakeup_source_register("soundwire-irq");
#endif
	if (priv->wake_lock == NULL) {
		AUDIO_LOGE("wakeup source register failed");
		return -EFAULT;
	}
	platform_set_drvdata(pdev, priv);
	pm_init(&pdev->dev);
	asp_cfg_init(priv);
	enable_sub_module(priv);
	soundwire_reset_irq(priv);

	return 0;
}

static struct codec_bus_ops soundwire_ops = {
	.runtime_get = soundwire_runtime_get,
	.runtime_put = soundwire_runtime_put,
	.switch_framer = soundwire_switch_framer,
	.enum_device = soundwire_enum_device,
	.get_framer_type = soundwire_get_framer,
	.activate = soundwire_activate,
	.deactivate = soundwire_deactivate,
};

static void resource_deinit(struct soundwire_priv *priv)
{
	wakeup_source_unregister(priv->wake_lock);
	priv->wake_lock = NULL;
}

static int soundwire_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct soundwire_priv *priv = NULL;
	int ret;

	IN_FUNCTION;

	priv = devm_kzalloc(dev, sizeof(struct soundwire_priv), GFP_KERNEL);
	if (!priv) {
		AUDIO_LOGE("not enough memory for soundwire_priv");
		return -ENOMEM;
	}

	ret = reg_remap_init(priv, dev);
	if (ret != 0) {
		AUDIO_LOGE("soundwire reg remap failed");
		return ret;
	}

	ret = resource_init(pdev, priv);
	if (ret != 0) {
		AUDIO_LOGE("soundwire resource init failed ret = %d", ret);
		return ret;
	}

	ret = soundwire_bandwidth_adp_init(priv);
	if (ret != 0) {
		AUDIO_LOGE("soundwire adp init failed ret = %d", ret);
		resource_deinit(priv);
		return ret;
	}

	register_ops(&soundwire_ops);
	OUT_FUNCTION;

	return 0;
}

static int soundwire_remove(struct platform_device *pdev)
{
	struct soundwire_priv *priv = platform_get_drvdata(pdev);
	struct device *dev = &pdev->dev;
	int ret;

	if (!priv) {
		AUDIO_LOGE("priv is null");
		return -EINVAL;
	}

	if (is_pm_runtime_support()) {
		pm_runtime_resume(dev);
		pm_runtime_disable(dev);
	}

	ret = soundwire_switch_framer(FRAMER_SOC);
	if (ret != 0)
		AUDIO_LOGE("switch to soc failed");

	resource_deinit(priv);
	soundwire_bandwidth_adp_deinit();
	platform_set_drvdata(pdev, NULL);

	if (is_pm_runtime_support())
		pm_runtime_set_suspended(dev);

	g_priv = NULL;
	return 0;
}

static const struct dev_pm_ops soundwire_pm_ops = {
	.suspend = soundwire_suspend,
	.resume = soundwire_resume,
	.runtime_suspend = soundwire_runtime_suspend,
	.runtime_resume = soundwire_runtime_resume
};

static const struct of_device_id soundwire_match[] = {
	{
		.compatible = "audio-soundwire",
	},
	{},
};

static struct platform_driver soundwire_platform_driver = {
	.driver = {
		.name = "soundwire",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(soundwire_match),
		.pm = &soundwire_pm_ops,
	},
	.probe = soundwire_probe,
	.remove = soundwire_remove,
};

static int __init soundwire_init(void)
{
	return platform_driver_register(&soundwire_platform_driver);
}
fs_initcall_sync(soundwire_init);

static void __exit soundwire_exit(void)
{
	platform_driver_unregister(&soundwire_platform_driver);
}
module_exit(soundwire_exit);

MODULE_DESCRIPTION("soundwire driver");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Huawei Technologies Co., Ltd.");
