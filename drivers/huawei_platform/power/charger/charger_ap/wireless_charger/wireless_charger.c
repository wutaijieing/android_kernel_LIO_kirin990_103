#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_address.h>
#include <linux/of_gpio.h>
#include <linux/time.h>
#include <huawei_platform/log/hw_log.h>
#include <chipset_common/hwpower/common_module/power_delay.h>
#include <chipset_common/hwpower/common_module/power_common_macro.h>
#include <chipset_common/hwpower/common_module/power_time.h>
#include <chipset_common/hwpower/common_module/power_wakeup.h>
#include <chipset_common/hwpower/hardware_ic/charge_pump.h>
#include <huawei_platform/power/huawei_charger.h>
#include <chipset_common/hwpower/wireless_charge/wireless_rx_module.h>
#include <chipset_common/hwpower/wireless_charge/wireless_rx_common.h>
#include <chipset_common/hwpower/wireless_charge/wireless_rx_ic_intf.h>
#include <chipset_common/hwpower/wireless_charge/wireless_rx_dts.h>
#include <chipset_common/hwpower/wireless_charge/wireless_rx_status.h>
#include <chipset_common/hwpower/wireless_charge/wireless_buck_ictrl.h>
#include <huawei_platform/power/wireless/wireless_charger.h>
#include <huawei_platform/power/wireless/wireless_transmitter.h>
#include <chipset_common/hwpower/wireless_charge/wireless_dc_check.h>
#include <huawei_platform/power/wireless/wireless_direct_charger.h>
#include <chipset_common/hwpower/wireless_charge/wireless_rx_alarm.h>
#include <../charging_core.h>
#include <chipset_common/hwpower/hardware_channel/wired_channel_switch.h>
#include <platform_include/basicplatform/linux/powerkey_event.h>
#include <chipset_common/hwpower/hardware_channel/vbus_channel.h>
#include <chipset_common/hwpower/hardware_ic/boost_5v.h>
#include <chipset_common/hwpower/wireless_charge/wireless_power_supply.h>
#include <chipset_common/hwpower/wireless_charge/wireless_acc_types.h>
#include <chipset_common/hwpower/wireless_charge/wireless_rx_acc.h>
#include <chipset_common/hwpower/wireless_charge/wireless_rx_ui.h>
#include <chipset_common/hwpower/wireless_charge/wireless_rx_pctrl.h>
#include <chipset_common/hwpower/wireless_charge/wireless_rx_plim.h>
#include <chipset_common/hwpower/wireless_charge/wireless_rx_interfere.h>
#include <chipset_common/hwpower/wireless_charge/wireless_rx_pmode.h>
#include <chipset_common/hwpower/wireless_charge/wireless_rx_fod.h>
#include <chipset_common/hwpower/wireless_charge/wireless_rx_monitor.h>
#include <chipset_common/hwpower/wireless_charge/wireless_rx_event.h>
#ifdef CONFIG_TCPC_CLASS
#include <huawei_platform/usb/hw_pd_dev.h>
#endif
#include <huawei_platform/power/wireless/wireless_keyboard.h>
#include <huawei_platform/hwpower/common_module/power_platform.h>
#include <huawei_platform/hwpower/wireless_charge/wireless_rx_platform.h>
#include <chipset_common/hwpower/hardware_channel/charger_channel.h>

#define HWLOG_TAG wireless_charger
HWLOG_REGIST();

static struct wlrx_dev_info *g_wlrx_di;
static struct wakeup_source *g_rx_con_wakelock;
static struct mutex g_rx_en_mutex;
static int g_fop_fixed_flag;
static int g_rx_vrect_restore_cnt;
static int g_rx_vout_err_cnt;
static bool g_bst_rst_complete = true;
static bool g_high_pwr_test_flag;
static int g_plimit_time_num;
static bool g_need_force_5v_vout;

static bool wlrx_msleep_exit(void)
{
	if ((wlrx_get_wired_channel_state() == WIRED_CHANNEL_ON) ||
		!wlrx_ic_is_tx_exist(WLTRX_IC_MAIN))
		return true;

	return false;
}

int wlc_get_rx_support_mode(void)
{
	struct wlrx_dev_info *di = g_wlrx_di;

	if (!di) {
		hwlog_err("%s: di null\n", __func__);
		return WLRX_SUPP_PMODE_BUCK;
	}

	return di->sysfs_data.rx_support_mode & di->qval_support_mode;
}

bool wlc_is_pwr_good(void)
{
	struct wlrx_dev_info *di = g_wlrx_di;

	if (!di) {
		hwlog_err("%s: di null\n", __func__);
		return false;
	}

	return (di->rx_evt.pwr_good == RX_PWR_ON_GOOD);
}

static void wlc_set_cur_vmode_id(struct wlrx_dev_info *di, int id)
{
	di->curr_vmode_index = id;
	hwlog_info("[set_cur_vmode_id] id=%d\n", di->curr_vmode_index);
}

int wireless_charge_get_rx_iout_limit(void)
{
	struct wlrx_dev_info *di = g_wlrx_di;

	if (!di) {
		hwlog_err("%s: di null\n", __func__);
		return 0;
	}
	return min(di->rx_iout_max, di->rx_iout_limit);
}

static void wlc_rx_chip_reset(struct wlrx_dev_info *di)
{
	if (di->wlc_err_rst_cnt >= WLC_RST_CNT_MAX)
		return;

	wlrx_ic_chip_reset(WLTRX_IC_MAIN);
	di->wlc_err_rst_cnt++;
	di->discon_delay_time = WL_RST_DISCONN_DELAY_MS;
}

static void wireless_charge_set_input_current(unsigned int drv_type)
{
	wlrx_buck_set_iin_for_rx(drv_type, wireless_charge_get_rx_iout_limit());
}

static int wireless_charge_fix_tx_fop(int fop)
{
	struct wlrx_dev_info *di = g_wlrx_di;

	if (!di) {
		hwlog_err("%s: di null\n", __func__);
		return -1;
	}
	if (wlrx_get_wireless_channel_state() == WIRELESS_CHANNEL_OFF) {
		hwlog_err("%s: not in wireless charging\n", __func__);
		return -1;
	}

	return wireless_fix_tx_fop(WLTRX_IC_MAIN, WIRELESS_PROTOCOL_QI, fop);
}

static int wireless_charge_unfix_tx_fop(void)
{
	struct wlrx_dev_info *di = g_wlrx_di;

	if (!di) {
		hwlog_err("%s: di null\n", __func__);
		return -1;
	}
	if (wlrx_get_wireless_channel_state() == WIRELESS_CHANNEL_OFF) {
		hwlog_err("%s: not in wireless charging\n", __func__);
		return -1;
	}

	return wireless_unfix_tx_fop(WLTRX_IC_MAIN, WIRELESS_PROTOCOL_QI);
}

static int wireless_charge_set_tx_vout(int tx_vout)
{
	struct wlrx_dev_info *di = g_wlrx_di;

	if (!di) {
		hwlog_err("%s: di null\n", __func__);
		return -1;
	}
	if (wlrx_get_wireless_channel_state() == WIRELESS_CHANNEL_OFF) {
		hwlog_err("%s: not in wireless charging\n", __func__);
		return -1;
	}
	if ((tx_vout > TX_DEFAULT_VOUT) &&
		(wlrx_get_wired_channel_state() == WIRED_CHANNEL_ON)) {
		hwlog_err("%s: wired vbus connect, tx_vout should be set to %dmV at most\n",
			__func__, TX_DEFAULT_VOUT);
		return -1;
	}
	if (di->pwroff_reset_flag && (tx_vout > TX_DEFAULT_VOUT)) {
		hwlog_err("%s: pwroff_reset_flag = %d, tx_vout should be set to %dmV at most\n",
			__func__, di->pwroff_reset_flag, TX_DEFAULT_VOUT);
		return -1;
	}
	hwlog_info("[%s] tx_vout is set to %dmV\n", __func__, tx_vout);
	return wlrx_ic_set_vfc(WLTRX_IC_MAIN, tx_vout);
}

int wireless_charge_set_rx_vout(int rx_vout)
{
	struct wlrx_dev_info *di = g_wlrx_di;

	if (!di) {
		hwlog_err("%s: di null\n", __func__);
		return -1;
	}
	if (wlrx_get_wireless_channel_state() == WIRELESS_CHANNEL_OFF) {
		hwlog_err("%s: not in wireless charging\n", __func__);
		return -1;
	}
	if (di->pwroff_reset_flag && (rx_vout > RX_DEFAULT_VOUT)) {
		hwlog_err("%s: pwroff_reset_flag = %d, rx_vout should be set to %dmV at most\n",
			__func__, di->pwroff_reset_flag, RX_DEFAULT_VOUT);
		return -1;
	}
	hwlog_info("%s: rx_vout is set to %dmV\n", __func__, rx_vout);
	return wlrx_ic_set_vout(WLTRX_IC_MAIN, rx_vout);
}

static void wireless_charge_count_avg_iout(struct wlrx_dev_info *di)
{
	int cnt_max;
	int iavg = 0;
	struct wlrx_pmode *curr_pcfg = NULL;

	if (di->monitor_interval <= 0)
		return;

	cnt_max = RX_AVG_IOUT_TIME / di->monitor_interval;

	wlrx_ic_get_iavg(WLTRX_IC_MAIN, &iavg);
	if (g_bst_rst_complete && (iavg < RX_LOW_IOUT)) {
		di->iout_high_cnt = 0;
		di->iout_low_cnt++;
		if (di->iout_low_cnt >= cnt_max)
			di->iout_low_cnt = cnt_max;
		return;
	}

	curr_pcfg = wlrx_pmode_get_curr_pcfg(WLTRX_DRV_MAIN);
	if ((iavg > RX_HIGH_IOUT) || (curr_pcfg && strstr(curr_pcfg->name, "SC"))) {
		di->iout_low_cnt = 0;
		di->iout_high_cnt++;
		if (di->iout_high_cnt >= cnt_max)
			di->iout_high_cnt = cnt_max;
		return;
	}
}

static int  wireless_charge_check_fast_charge_succ(unsigned int drv_type)
{
	enum wlrx_ui_icon_type type = wlrx_ui_get_icon_type(drv_type);

	if (((type == WLRX_UI_FAST_CHARGE) || (type == WLRX_UI_SUPER_CHARGE)) &&
		(wlrx_get_charge_stage() >= WLRX_STAGE_CHARGING))
		return WIRELESS_CHRG_SUCC;
	else
		return WIRELESS_CHRG_FAIL;
}

static int wireless_charge_check_normal_charge_succ(unsigned int drv_type)
{
	enum wlrx_ui_icon_type type = wlrx_ui_get_icon_type(drv_type);

	if (!wlrx_is_err_tx(WLTRX_DRV_MAIN) && (type != WLRX_UI_FAST_CHARGE) &&
		(wlrx_get_charge_stage() >= WLRX_STAGE_CHARGING))
		return WIRELESS_CHRG_SUCC;

	return WIRELESS_CHRG_FAIL;
}

static void wlc_update_thermal_control(u8 thermal_ctrl)
{
	u8 thermal_status;

	thermal_status = thermal_ctrl & WLC_THERMAL_EXIT_SC_MODE;
	if ((thermal_status == WLC_THERMAL_EXIT_SC_MODE) && !g_high_pwr_test_flag)
		wlrx_plim_set_src(WLTRX_DRV_MAIN, WLRX_PLIM_SRC_THERMAL);
	else
		wlrx_plim_clear_src(WLTRX_DRV_MAIN, WLRX_PLIM_SRC_THERMAL);
}

static int wireless_set_thermal_ctrl(unsigned char value)
{
	struct wlrx_dev_info *di = g_wlrx_di;

	if (!di || (value > 0xFF)) /* 0xFF: maximum of u8 */
		return -EINVAL;
	di->sysfs_data.thermal_ctrl = value;
	wlc_update_thermal_control(di->sysfs_data.thermal_ctrl);
	hwlog_info("thermal_ctrl = 0x%x", di->sysfs_data.thermal_ctrl);
	return 0;
}

static int wireless_get_thermal_ctrl(unsigned char *value)
{
	struct wlrx_dev_info *di = g_wlrx_di;

	if (!di || !value)
		return -EINVAL;
	*value = di->sysfs_data.thermal_ctrl;
	return 0;
}

static struct power_if_ops wl_if_ops = {
	.set_wl_thermal_ctrl = wireless_set_thermal_ctrl,
	.get_wl_thermal_ctrl = wireless_get_thermal_ctrl,
	.type_name = "wl",
};

bool wlc_pmode_final_judge(unsigned int drv_type, int pid, struct wlrx_pmode *pcfg)
{
	struct wlrx_dev_info *di = g_wlrx_di;

	if (!pcfg || !di || (di->tx_vout_max < pcfg->vtx) || (di->rx_vout_max < pcfg->vrx))
		return false;

	if ((pid == wlrx_pmode_get_curr_pid(drv_type)) &&
		(wlrx_get_charge_stage() != WLRX_STAGE_CHARGING)) {
		if ((pcfg->timeout > 0) && time_after(jiffies, di->curr_power_time_out))
			return false;
	}

	return true;
}

static void wlc_ignore_qval_work(struct work_struct *work)
{
	struct wlrx_dev_info *di = g_wlrx_di;

	if (!di)
		return;

	hwlog_info("[%s] timeout, permit SC mode\n", __func__);
	di->qval_support_mode = WLRX_SUPP_PMODE_ALL;
}

void wlrx_preproccess_fod_status(void)
{
	struct wlrx_dev_info *di = g_wlrx_di;

	if (!di)
		return;

	hwlog_err("%s: tx_fod_err, forbid SC mode\n", __func__);
	di->qval_support_mode = WLRX_SUPP_PMODE_BUCK;
	schedule_delayed_work(&di->ignore_qval_work,
		msecs_to_jiffies(30000)); /* 30s timeout to restore state */
}

static void wlc_reset_icon_pmode(struct wlrx_dev_info *di)
{
	int i;
	int pcfg_level;
	struct wldc_dev_info *sc_di = NULL;

	wireless_sc_get_di(&sc_di);
	if (sc_di && sc_di->force_disable)
		return;

	pcfg_level = wlrx_pmode_get_pcfg_level(WLTRX_DRV_MAIN);
	for (i = 0; i < pcfg_level; i++)
		set_bit(i, &di->icon_pmode);
	hwlog_info("[%s] icon_pmode=0x%x", __func__, di->icon_pmode);
}

void wlc_clear_icon_pmode(int pid)
{
	struct wlrx_dev_info *di = g_wlrx_di;

	if (!di || !wlrx_pmode_is_pid_valid(WLTRX_DRV_MAIN, pid))
		return;

	if (test_bit(pid, &di->icon_pmode)) {
		clear_bit(pid, &di->icon_pmode);
		hwlog_info("[%s] icon_pmode=0x%x", __func__, di->icon_pmode);
	}
}

static void wlc_revise_icon_display(unsigned int *icon_type)
{
	if (*icon_type != WLRX_UI_SUPER_CHARGE)
		return;

	if (wlrx_is_highpwr_rvs_tx(WLTRX_DRV_MAIN))
		*icon_type = WLRX_UI_FAST_CHARGE;
}

void wireless_charge_icon_display(int crit_type)
{
	int pid;
	int pcfg_level;
	unsigned int icon_type;
	struct wlrx_dev_info *di = g_wlrx_di;
	struct wlrx_pmode *pcfg = NULL;

	if (!di) {
		hwlog_err("icon_display: para null\n");
		return;
	}

	pcfg_level = wlrx_pmode_get_pcfg_level(WLTRX_DRV_MAIN);
	for (pid = pcfg_level - 1; pid >= 0; pid--) {
		if (test_bit(pid, &di->icon_pmode) &&
			wlrx_pmode_judge(WLTRX_DRV_MAIN, pid, crit_type))
			break;
	}

	if (pid < 0) {
		pid = 0;
		hwlog_err("icon_display: mismatched\n");
	}

	pcfg = wlrx_pmode_get_pcfg_by_pid(WLTRX_DRV_MAIN, pid);
	if (!pcfg)
		return;

	icon_type = pcfg->icon;
	wlc_revise_icon_display(&icon_type);
	if (wlrx_get_wireless_channel_state() == WIRELESS_CHANNEL_OFF) {
		hwlog_err("icon_display: not in wireless charging, return\n");
		return;
	}
	wlrx_ui_send_charge_uevent(WLTRX_DRV_MAIN, icon_type);
}

void wlc_ignore_vbus_only_event(bool ignore_flag)
{
	struct wlrx_dts *dts = wlrx_get_dts();

	if (!dts || !dts->bst5v_ignore_vbus_only)
		return;

#ifdef CONFIG_TCPC_CLASS
	if (ignore_flag)
		pd_dpm_ignore_vbus_only_event(true);
	else if (!boost_5v_status(BOOST_CTRL_WLC) && !boost_5v_status(BOOST_CTRL_WLDC))
		pd_dpm_ignore_vbus_only_event(false);
#endif
}

static void wlc_extra_power_supply(bool enable)
{
	int ret;
	static bool boost_5v_flag;
	struct wlrx_dev_info *di = g_wlrx_di;
	struct wlrx_dts *dts = wlrx_get_dts();

	if (!di || !dts || !dts->hvc_need_5vbst)
		return;

	if (enable && (di->supported_rx_vout > RX_HIGH_VOUT)) {
		ret = boost_5v_enable(BOOST_5V_ENABLE, BOOST_CTRL_WLC);
		if (ret) {
			hwlog_err("%s: boost_5v enable fail\n", __func__);
			di->extra_pwr_good_flag = 0;
			return;
		}
		wlc_ignore_vbus_only_event(true);
		boost_5v_flag = true;
	} else if (!enable && boost_5v_flag) {
		ret = boost_5v_enable(BOOST_5V_DISABLE, BOOST_CTRL_WLC);
		if (ret) {
			hwlog_err("%s: boost_5v disable fail\n", __func__);
			return;
		}
		wlc_ignore_vbus_only_event(false);
		boost_5v_flag = false;
	}
}

static void wlc_get_supported_max_rx_vout(struct wlrx_dev_info *di)
{
	int pcfg_level = wlrx_pmode_get_pcfg_level(WLTRX_DRV_MAIN);
	int pid = pcfg_level - 1;
	struct wlrx_pmode *pcfg = NULL;

	for (; pid >= 0; pid--) {
		if (wlrx_pmode_judge(WLTRX_DRV_MAIN, pid, WLRX_PMODE_QUICK_JUDGE))
			break;
	}
	if (pid < 0)
		pid = 0;

	pcfg = wlrx_pmode_get_pcfg_by_pid(WLTRX_DRV_MAIN, pid);
	if (!pcfg)
		return;

	di->supported_rx_vout = pcfg->vrx;
	hwlog_info("[%s] rx_support_mode=0x%x rx_vmax=%d\n", __func__,
		wlc_get_rx_support_mode(), di->supported_rx_vout);
}

static void wireless_charge_set_ctrl_interval(struct wlrx_dev_info *di)
{
	if (wlrx_get_charge_stage() < WLRX_STAGE_REGULATION)
		di->ctrl_interval = CONTROL_INTERVAL_NORMAL;
	else
		di->ctrl_interval = CONTROL_INTERVAL_FAST;
}

void wireless_charge_chip_init(int tx_vset)
{
	if (wlrx_ic_chip_init(WLTRX_IC_MAIN, tx_vset,
		wlrx_acc_get_tx_type(WLTRX_DRV_MAIN)))
		hwlog_err("chip_init: failed\n");
}

static void wlc_set_iout_min(struct wlrx_dev_info *di)
{
	struct wlrx_dts *dts = wlrx_get_dts();

	if (!dts)
		return;

	di->rx_iout_max = dts->rx_imin;
	wireless_charge_set_input_current(WLTRX_DRV_MAIN);
}

int wireless_charge_select_vout_mode(int vout)
{
	int id = 0;
	struct wlrx_dev_info *di = g_wlrx_di;
	struct wlrx_dts *dts = wlrx_get_dts();

	if (!di || !dts) {
		hwlog_err("select_vout_mode: di or dts is null\n");
		return id;
	}

	for (id = 0; id < dts->vmode_cfg_level; id++) {
		if (vout == dts->vmode_cfg[id].vtx)
			break;
	}
	if (id >= dts->vmode_cfg_level) {
		id = 0;
		hwlog_err("select_vout_mode: match vmode_index failed\n");
	}
	return id;
}

static void wlc_update_high_fop_para(struct wlrx_dev_info *di)
{
	int fop = 0;
	struct wlprot_acc_cap *acc_cap = wlrx_acc_get_cap(WLTRX_DRV_MAIN);

	if (!acc_cap || !acc_cap->support_fop_range)
		return;

	(void)wlrx_ic_get_fop(WLTRX_IC_MAIN, &fop);
	if (fop > WLRX_ACC_NORMAL_MAX_FOP) {
		di->tx_vout_max = min(di->tx_vout_max, VOUT_9V_STAGE_MAX);
		di->rx_vout_max = min(di->rx_vout_max, VOUT_9V_STAGE_MAX);
	}
}

static void wlc_update_iout_low_para(struct wlrx_dev_info *di, bool ignore_cnt_flag)
{
	struct wlprot_acc_cap *acc_cap = wlrx_acc_get_cap(WLTRX_DRV_MAIN);

	if (ignore_cnt_flag || (di->monitor_interval == 0))
		return;
	if (di->iout_low_cnt < RX_AVG_IOUT_TIME / di->monitor_interval)
		return;
	if (acc_cap && acc_cap->support_fop_range &&
		(wlrx_intfr_get_fixed_fop(WLTRX_DRV_MAIN) >= WLRX_ACC_NORMAL_MAX_FOP))
		return;
	di->tx_vout_max = min(di->tx_vout_max, TX_DEFAULT_VOUT);
	di->rx_vout_max = min(di->rx_vout_max, RX_DEFAULT_VOUT);
	di->rx_iout_max = min(di->rx_iout_max, RX_DEFAULT_IOUT);
}

static void wlc_update_tx_alarm_vmax(struct wlrx_dev_info *di)
{
	int vmax;

	vmax = wlrx_get_alarm_vlim();
	if (vmax <= 0)
		return;

	di->tx_vout_max = min(di->tx_vout_max, vmax);
	di->rx_vout_max = min(di->rx_vout_max, vmax);
}

void wireless_charge_update_max_vout_and_iout(bool ignore_cnt_flag)
{
	struct wlrx_dev_info *di = g_wlrx_di;

	if (!di)
		return;

	wlrx_pctrl_update_para(WLTRX_DRV_MAIN, di->pctrl);
	di->tx_vout_max = di->pctrl->vtx;
	di->rx_vout_max = di->pctrl->vrx;
	di->rx_iout_max = di->pctrl->irx;

	if (di->pwroff_reset_flag || !di->extra_pwr_good_flag) {
		di->tx_vout_max = min(di->tx_vout_max, TX_DEFAULT_VOUT);
		di->rx_vout_max = min(di->rx_vout_max, RX_DEFAULT_VOUT);
		di->rx_iout_max = min(di->rx_iout_max, RX_DEFAULT_IOUT);
	}
	wlc_update_tx_alarm_vmax(di);
	wlc_update_high_fop_para(di);
	wlc_update_iout_low_para(di, ignore_cnt_flag);
}

static void wlc_notify_charger_vout(struct wlrx_dev_info *di)
{
	int tx_vout;
	int cp_vout;
	int cp_ratio;
	struct wlrx_dts *dts = wlrx_get_dts();

	if (!dts)
		return;

	tx_vout = dts->vmode_cfg[di->curr_vmode_index].vtx;
	cp_ratio = charge_pump_get_cp_ratio(CP_TYPE_MAIN);
	if (cp_ratio <= 0) {
		hwlog_err("%s: cp_ratio err\n", __func__);
		return;
	}
	hwlog_info("[%s] cp_ratio=%d\n", __func__, cp_ratio);
	cp_vout = tx_vout / cp_ratio;
	power_event_bnc_notify(POWER_BNT_WLC, POWER_NE_WLC_CHARGER_VBUS, &cp_vout);
}

static void wlc_send_bst_succ_msg(void)
{
	if (!wlrx_is_fac_tx(WLTRX_DRV_MAIN) ||
		(wlrx_get_charge_stage() != WLRX_STAGE_CHARGING))
		return;

	if (!wireless_send_rx_boost_succ(WLTRX_IC_MAIN, WIRELESS_PROTOCOL_QI))
		hwlog_info("[%s] send cmd boost_succ ok\n", __func__);
}

static void wlc_report_bst_fail_dmd(struct wlrx_dev_info *di)
{
	static bool dsm_report_flag;
	char dsm_buff[POWER_DSM_BUF_SIZE_0512] = { 0 };

	if (++di->boost_err_cnt < BOOST_ERR_CNT_MAX) {
		dsm_report_flag = false;
		return;
	}

	di->boost_err_cnt = BOOST_ERR_CNT_MAX;
	if (dsm_report_flag)
		return;

	/* dmd caused by misalignment not reported */
	if (di->rx_evt.pwr_good != RX_PWR_ON_GOOD) {
		hwlog_err("%s: misaligned, ignore dmd-report\n", __func__);
		return;
	}
	wlrx_dsm_report(WLTRX_DRV_MAIN, POWER_DSM_ERROR_WIRELESS_BOOSTING_FAIL,
		dsm_buff, sizeof(dsm_buff));
	dsm_report_flag = true;
}

static int wireless_charge_boost_vout(struct wlrx_dev_info *di,
	int cur_vmode_id, int target_vmode_id)
{
	int vmode;
	int ret;
	int tx_vout;
	int bst_delay = RX_BST_DELAY_TIME;
	int last_vfc_reg = 0;
	struct wlrx_dts *dts = wlrx_get_dts();

	if (!dts)
		return -ENODEV;

	if (di->boost_err_cnt >= BOOST_ERR_CNT_MAX) {
		hwlog_debug("%s: boost fail exceed %d times\n",
			__func__, BOOST_ERR_CNT_MAX);
		return -EPERM;
	}

	wlc_set_iout_min(di);
	(void)power_msleep(WLRX_ILIM_DELAY, DT_MSLEEP_25MS, wlrx_msleep_exit);
	g_bst_rst_complete = false;
	wlrx_ic_get_bst_delay_time(WLTRX_IC_MAIN, &bst_delay);

	for (vmode = cur_vmode_id + 1; vmode <= target_vmode_id; vmode++) {
		(void)wlrx_ic_get_vfc_reg(WLTRX_IC_MAIN, &last_vfc_reg);
		tx_vout = dts->vmode_cfg[vmode].vtx;
		ret = wireless_charge_set_tx_vout(tx_vout);
		if (ret) {
			hwlog_err("%s: boost fail\n", __func__);
			wlc_report_bst_fail_dmd(di);
			(void)wireless_charge_set_tx_vout(last_vfc_reg);
			g_bst_rst_complete = true;
			return ret;
		}
		wlc_set_cur_vmode_id(di, vmode);
		wlc_notify_charger_vout(di);
		wlc_set_iout_min(di);
		if (vmode != target_vmode_id)
			(void)power_msleep(bst_delay, DT_MSLEEP_25MS,
				wlrx_msleep_exit);
	}

	g_bst_rst_complete = true;
	di->boost_err_cnt = 0;
	power_event_bnc_notify(POWER_BNT_WLC, POWER_NE_WLC_TX_VSET, &tx_vout);

	wlc_send_bst_succ_msg();
	return 0;
}

static void wireless_charge_wait_fop_fix_to_default(void)
{
	int i;

	hwlog_info("%s\n", __func__);
	g_need_force_5v_vout = true;
	/* delay 60*50 = 3000ms for direct charger check finish */
	for (i = 0; i < 60; i++) {
		if (g_fop_fixed_flag <= WLRX_ACC_NORMAL_MAX_FOP)
			break;
		if (!power_msleep(DT_MSLEEP_50MS, DT_MSLEEP_25MS,
			wlrx_msleep_exit))
			break;
	}
}

bool wlrx_bst_rst_completed(void)
{
	return g_bst_rst_complete;
}

static int wireless_charge_reset_vout(struct wlrx_dev_info *di,
	int cur_vmode_id, int target_vmode_id)
{
	int ret;
	int vmode;
	int tx_vout;
	int last_vfc_reg = 0;
	struct wlrx_dts *dts = wlrx_get_dts();

	if (!dts)
		return -ENODEV;

	wlc_set_iout_min(di);
	(void)power_msleep(WLRX_ILIM_DELAY, DT_MSLEEP_25MS, wlrx_msleep_exit);
	g_bst_rst_complete = false;

	for (vmode = cur_vmode_id - 1; vmode >= target_vmode_id; vmode--) {
		(void)wlrx_ic_get_vfc_reg(WLTRX_IC_MAIN, &last_vfc_reg);
		tx_vout = dts->vmode_cfg[vmode].vtx;
		if ((tx_vout < RX_HIGH_VOUT) && (g_fop_fixed_flag > WLRX_ACC_NORMAL_MAX_FOP))
			wireless_charge_wait_fop_fix_to_default();
		ret = wireless_charge_set_tx_vout(tx_vout);
		if (ret) {
			hwlog_err("%s: reset fail\n", __func__);
			(void)wireless_charge_set_tx_vout(last_vfc_reg);
			g_bst_rst_complete = true;
			return ret;
		}
		wlc_set_cur_vmode_id(di, vmode);
		wlc_notify_charger_vout(di);
		wlc_set_iout_min(di);
	}

	g_bst_rst_complete = true;
	return 0;
}

static int wireless_charge_set_vout(int cur_vmode_index, int target_vmode_index)
{
	int ret;
	int tx_vout;
	struct wlrx_dev_info *di = g_wlrx_di;
	struct wlrx_dts *dts = wlrx_get_dts();

	if (!di || !dts) {
		hwlog_err("set_vout: di or dts is null\n");
		return -ENODEV;
	}

	tx_vout = dts->vmode_cfg[target_vmode_index].vtx;
	if (target_vmode_index > cur_vmode_index)
		ret = wireless_charge_boost_vout(di,
			cur_vmode_index, target_vmode_index);
	else if (target_vmode_index < cur_vmode_index)
		ret = wireless_charge_reset_vout(di,
			cur_vmode_index, target_vmode_index);
	else
		return wireless_charge_set_rx_vout(tx_vout);

	if (wlrx_get_wired_channel_state() == WIRED_CHANNEL_ON) {
		hwlog_err("%s: wired vbus connect\n", __func__);
		return -EPERM;
	}
	if (wlrx_get_wireless_channel_state() == WIRELESS_CHANNEL_OFF) {
		hwlog_err("%s: wireless vbus disconnect\n", __func__);
		return -EPERM;
	}

	if (di->curr_vmode_index == cur_vmode_index)
		return ret;

	tx_vout = dts->vmode_cfg[di->curr_vmode_index].vtx;
	wireless_charge_chip_init(tx_vout);
	wlc_notify_charger_vout(di);

	return ret;
}

int wldc_set_trx_vout(int vtx, int vrx)
{
	int ret;
	int cur_vmode;
	int target_vmode;
	int vfc_reg = 0;

	if (wlrx_ic_is_sleep_enable(WLTRX_IC_MAIN)) {
		hwlog_info("set_trx_vout: sleep_en eanble, return\n");
		return -ENXIO;
	}

	(void)wlrx_ic_get_vfc_reg(WLTRX_IC_MAIN, &vfc_reg);
	cur_vmode = wireless_charge_select_vout_mode(vfc_reg);
	target_vmode = wireless_charge_select_vout_mode(vtx);

	ret = wireless_charge_set_vout(cur_vmode, target_vmode);
	if (ret) {
		hwlog_err("set_trx_vout: set vtx failed\n");
		return ret;
	}

	return wireless_charge_set_rx_vout(vrx);
}

static int wireless_charge_vout_control(struct wlrx_dev_info *di, int pid)
{
	int ret;
	int tx_vout;
	int target_vout;
	int curr_vmode_index;
	int target_vmode_index;
	int vfc_reg = 0;
	struct wlrx_dts *dts = wlrx_get_dts();
	struct wlrx_pmode *pcfg = wlrx_pmode_get_pcfg_by_pid(WLTRX_DRV_MAIN, pid);

	if (!dts || !pcfg)
		return -ENODEV;

	if (strstr(pcfg->name, "SC"))
		return 0;
	if (wlrx_get_wireless_channel_state() != WIRELESS_CHANNEL_ON)
		return -1;
	if (wlrx_ic_is_sleep_enable(WLTRX_IC_MAIN)) {
		hwlog_info("vout_control: sleep_en eanble, return\n");
		return -ENXIO;
	}
	(void)wlrx_ic_get_vfc_reg(WLTRX_IC_MAIN, &vfc_reg);
	tx_vout = dts->vmode_cfg[di->curr_vmode_index].vtx;
	if (vfc_reg != tx_vout) {
		hwlog_err("%s: vfc_reg %dmV != cur_mode_vout %dmV\n", __func__,
			vfc_reg, tx_vout);
		ret = wireless_charge_set_tx_vout(tx_vout);
		if (ret)
			hwlog_err("%s: set tx vout fail\n", __func__);
	}
	target_vout = pcfg->vtx;
	target_vmode_index = wireless_charge_select_vout_mode(target_vout);
	curr_vmode_index = di->curr_vmode_index;
	di->tx_vout_max = min(di->tx_vout_max, pcfg->vtx);
	di->rx_vout_max = min(di->rx_vout_max, pcfg->vrx);
	ret = wireless_charge_set_vout(curr_vmode_index, target_vmode_index);
	if (ret)
		return ret;
	if (di->curr_vmode_index != curr_vmode_index)
		return 0;
	tx_vout = dts->vmode_cfg[di->curr_vmode_index].vtx;
	wireless_charge_chip_init(tx_vout);
	wlc_notify_charger_vout(di);

	return 0;
}

static void wlc_update_imax_by_tx_plimit(struct wlrx_dev_info *di)
{
	int ilim;
	struct wlrx_pmode *curr_pcfg = wlrx_pmode_get_curr_pcfg(WLTRX_DRV_MAIN);

	if (!curr_pcfg)
		return;

	ilim = wlrx_get_alarm_ilim(curr_pcfg->vrx);
	if (ilim <= 0)
		return;

	di->rx_iout_max = min(di->rx_iout_max, ilim);
}

static int wlc_start_sample_iout(struct wlrx_dev_info *di)
{
	struct wlrx_dts *dts = wlrx_get_dts();

	if (!dts || !wlrx_is_fac_tx(WLTRX_DRV_MAIN))
		return -EINVAL;

	if (di->start_sample_type <= 0) {
		di->start_sample_type = WLRX_SAMPLE_WORK_PENDING;
		mod_delayed_work(system_wq, &di->rx_sample_work,
			msecs_to_jiffies(dts->sample_delay_time));
	}
	if (di->start_sample_type == WLRX_SAMPLE_WORK_RUNNING) {
		di->rx_iout_limit = di->rx_iout_max;
		wireless_charge_set_input_current(WLTRX_DRV_MAIN);
		return 0;
	}

	return -EINVAL;
}

static void wlc_revise_vout_para(struct wlrx_dev_info *di)
{
	int ret;
	int vfc_reg = 0;
	int rx_vout_reg = 0;
	struct wlrx_pmode *curr_pcfg = wlrx_pmode_get_curr_pcfg(WLTRX_DRV_MAIN);

	if (!curr_pcfg || (wlrx_get_charge_stage() == WLRX_STAGE_REGULATION_DC) ||
		(wlrx_get_wireless_channel_state() == WIRELESS_CHANNEL_OFF))
		return;

	(void)wlrx_ic_get_vfc_reg(WLTRX_IC_MAIN, &vfc_reg);
	(void)wlrx_ic_get_vout_reg(WLTRX_IC_MAIN, &rx_vout_reg);

	if ((vfc_reg <= curr_pcfg->vtx - RX_VREG_OFFSET) ||
		(vfc_reg >= curr_pcfg->vtx + RX_VREG_OFFSET)) {
		hwlog_err("%s: revise tx_vout\n", __func__);
		ret = wireless_charge_set_tx_vout(curr_pcfg->vtx);
		if (ret)
			hwlog_err("%s: set tx vout fail\n", __func__);
	}

	if ((rx_vout_reg <= curr_pcfg->vrx - RX_VREG_OFFSET) ||
		(rx_vout_reg >= curr_pcfg->vrx + RX_VREG_OFFSET)) {
		hwlog_err("%s: revise rx_vout\n", __func__);
		ret = wireless_charge_set_rx_vout(curr_pcfg->vrx);
		if (ret)
			hwlog_err("%s: set rx vout fail\n", __func__);
	}
}

static void wlc_update_ilim_by_low_vrect(struct wlrx_dev_info *di)
{
	static int rx_vrect_low_cnt;
	int cnt_max;
	int vrect = 0;
	int charger_iin_regval = wlrx_buck_get_iin_regval(WLTRX_DRV_MAIN);
	struct wlrx_dts *dts = wlrx_get_dts();
	struct wlrx_pmode *curr_pfcg = wlrx_pmode_get_curr_pcfg(WLTRX_DRV_MAIN);

	if (!dts || !curr_pfcg || (di->ctrl_interval <= 0))
		return;

	(void)wlrx_ic_get_vrect(WLTRX_IC_MAIN, &vrect);
	cnt_max = RX_VRECT_LOW_RESTORE_TIME / di->ctrl_interval;
	if (vrect < curr_pfcg->vrect_lth) {
		if (++rx_vrect_low_cnt >= RX_VRECT_LOW_CNT) {
			rx_vrect_low_cnt = RX_VRECT_LOW_CNT;
			hwlog_err("update_ilim_by_low_vrect: vrect:%d<lth:%d,decrease irx:%d\n",
				vrect, curr_pfcg->vrect_lth, dts->rx_istep);
			di->rx_iout_limit = max(RX_VRECT_LOW_IOUT_MIN,
				charger_iin_regval - dts->rx_istep);
			g_rx_vrect_restore_cnt = cnt_max;
		}
	} else if (g_rx_vrect_restore_cnt > 0) {
		rx_vrect_low_cnt = 0;
		g_rx_vrect_restore_cnt--;
		di->rx_iout_limit = charger_iin_regval;
	} else {
		rx_vrect_low_cnt = 0;
	}
}

static void wlc_update_iout_para(struct wlrx_dev_info *di)
{
	wlrx_buck_update_ictrl_para(WLTRX_DRV_MAIN, &di->rx_iout_limit);
	wlc_update_ilim_by_low_vrect(di);
	wlc_update_imax_by_tx_plimit(di);
}

static void wlc_iout_control(struct wlrx_dev_info *di)
{
	int ret;
	struct wlrx_pmode *curr_pcfg = wlrx_pmode_get_curr_pcfg(WLTRX_DRV_MAIN);
	struct wlprot_acc_cap *acc_cap = wlrx_acc_get_cap(WLTRX_DRV_MAIN);

	if (!curr_pcfg || !acc_cap ||
		(wlrx_get_charge_stage() == WLRX_STAGE_REGULATION_DC) ||
		(wlrx_get_wireless_channel_state() == WIRELESS_CHANNEL_OFF))
		return;

	if (di->pwroff_reset_flag)
		return;

	di->rx_iout_max = min(di->rx_iout_max, curr_pcfg->irx);
	di->rx_iout_max = min(di->rx_iout_max, acc_cap->imax);
	di->rx_iout_limit = di->rx_iout_max;

	ret = wlc_start_sample_iout(di);
	if (!ret)
		return;

	wlc_update_iout_para(di);
	wireless_charge_set_input_current(WLTRX_DRV_MAIN);
}

static int wlc_high_fop_vout_check(void)
{
	int rx_vout = 0;
	int vfc_reg = 0;

	(void)wlrx_ic_get_vout(WLTRX_IC_MAIN, &rx_vout);
	(void)wlrx_ic_get_vfc_reg(WLTRX_IC_MAIN, &vfc_reg);
	if ((rx_vout < VOUT_9V_STAGE_MIN) || (rx_vout > VOUT_9V_STAGE_MAX) ||
		(vfc_reg < VOUT_9V_STAGE_MIN) || (vfc_reg > VOUT_9V_STAGE_MAX))
		return -1;

	return 0;
}

static int wireless_charge_fop_fix_check(bool force_flag)
{
	int fixed_fop = wlrx_intfr_get_fixed_fop(WLTRX_DRV_MAIN);

	if ((fixed_fop <= 0) || (g_fop_fixed_flag == fixed_fop))
		return 0;

	/* reset tx to 9V for high fop; else delay 40*100ms for limit iout */
	if (fixed_fop >= WLRX_ACC_NORMAL_MAX_FOP) {
		if (wlc_high_fop_vout_check())
			return -EINVAL;
	} else if (!force_flag && (g_plimit_time_num < 40)) {
		g_plimit_time_num++;
		return -EINVAL;
	}
	if (wireless_charge_fix_tx_fop(fixed_fop)) {
		hwlog_err("fop_fix_check: fixed failed\n");
		return -EINVAL;
	}

	g_fop_fixed_flag = fixed_fop;
	g_plimit_time_num = 0;
	return 0;
}

static void wireless_charge_fop_unfix_check(void)
{
	if (g_fop_fixed_flag <= 0)
		return;
	if ((wlrx_intfr_get_fixed_fop(WLTRX_DRV_MAIN) > 0) && !g_need_force_5v_vout)
		return;
	if (wireless_charge_unfix_tx_fop()) {
		hwlog_err("fop_unfix_check: unfix failed\n");
		return;
	}

	g_fop_fixed_flag = 0;
	g_need_force_5v_vout = false;
}

static void wireless_charge_update_fop(bool force_flag)
{
	if (!wlrx_is_std_tx(WLTRX_DRV_MAIN))
		return;
	if (!force_flag && (wlrx_get_charge_stage() <= WLRX_STAGE_CHARGING))
		return;
	if (!wlrx_acc_support_fix_fop(WLTRX_DRV_MAIN, wlrx_intfr_get_fixed_fop(WLTRX_DRV_MAIN)))
		return;
	if (wireless_charge_fop_fix_check(force_flag))
		return;
	wireless_charge_fop_unfix_check();
}

static void wlc_update_charge_state(struct wlrx_dev_info *di)
{
	int ret;
	int soc;
	static int retry_cnt;

	if (!wlrx_is_std_tx(WLTRX_DRV_MAIN) || !wlrx_ic_is_tx_exist(WLTRX_IC_MAIN) ||
		(wlrx_get_wired_channel_state() == WIRED_CHANNEL_ON))
		return;

	if (wlrx_get_charge_stage() <= WLRX_STAGE_CHARGING) {
		retry_cnt = 0;
		return;
	}

	soc = power_platform_get_battery_ui_capacity();
	if (soc >= CAPACITY_FULL)
		di->stat_rcd.chrg_state_cur |= WIRELESS_STATE_CHRG_FULL;
	else
		di->stat_rcd.chrg_state_cur &= ~WIRELESS_STATE_CHRG_FULL;

	if (di->stat_rcd.chrg_state_cur != di->stat_rcd.chrg_state_last) {
		if (retry_cnt >= WLC_SEND_CHARGE_STATE_RETRY_CNT) {
			retry_cnt = 0;
			di->stat_rcd.chrg_state_last =
				di->stat_rcd.chrg_state_cur;
			return;
		}
		hwlog_info("[%s] charge_state=%d\n",
			__func__, di->stat_rcd.chrg_state_cur);
		ret = wireless_send_charge_state(WLTRX_IC_MAIN, WIRELESS_PROTOCOL_QI,
			di->stat_rcd.chrg_state_cur);
		if (ret) {
			hwlog_err("%s: send charge_state fail\n", __func__);
			retry_cnt++;
			return;
		}
		retry_cnt = 0;
		di->stat_rcd.chrg_state_last = di->stat_rcd.chrg_state_cur;
	}
}

static void wlc_check_voltage(struct wlrx_dev_info *di)
{
	int vout = 0;
	int vout_reg = 0;
	int vfc_reg = 0;
	int iavg = 0;
	int vbus = charge_get_vbus();
	int cnt_max = RX_VOUT_ERR_CHECK_TIME / di->monitor_interval;
	struct wlrx_dts *dts = wlrx_get_dts();

	if (!dts)
		return;

	(void)wlrx_ic_get_vout(WLTRX_IC_MAIN, &vout);
	if ((vout <= 0) || !g_bst_rst_complete ||
		(wlrx_get_charge_stage() < WLRX_STAGE_HANDSHAKE))
		return;

	vout = (vout >= vbus) ? vout : vbus;
	(void)wlrx_ic_get_vout_reg(WLTRX_IC_MAIN, &vout_reg);
	if (vout >= vout_reg * dts->rx_vout_err_ratio / POWER_PERCENT) {
		g_rx_vout_err_cnt = 0;
		return;
	}

	wlrx_ic_get_iavg(WLTRX_IC_MAIN, &iavg);
	if (iavg >= RX_EPT_IGNORE_IOUT)
		return;

	hwlog_err("%s: abnormal vout=%dmV", __func__, vout);
	if (++g_rx_vout_err_cnt < cnt_max)
		return;

	g_rx_vout_err_cnt = cnt_max;
	(void)wlrx_ic_get_vfc_reg(WLTRX_IC_MAIN, &vfc_reg);
	if (!wlrx_ic_is_sleep_enable(WLTRX_IC_MAIN) &&
		(vfc_reg >= RX_HIGH_VOUT2)) {
		wlrx_plim_set_src(WLTRX_DRV_MAIN, WLRX_PLIM_SRC_VOUT_ERR);
		hwlog_err("%s: high vout err\n", __func__);
		return;
	}
	hwlog_info("[%s] vout lower than %d*%d%%mV for %dms, send EPT\n",
		__func__, vout_reg, dts->rx_vout_err_ratio,
		RX_VOUT_ERR_CHECK_TIME);
	if (wlrx_get_wireless_channel_state() == WIRELESS_CHANNEL_ON)
		wlrx_ic_send_ept(WLTRX_IC_MAIN, WIRELESS_EPT_ERR_VOUT);
}

void wlc_set_high_pwr_test_flag(bool flag)
{
	g_high_pwr_test_flag = flag;

	if (g_high_pwr_test_flag) {
		wlrx_plim_clear_src(WLTRX_DRV_MAIN, WLRX_PLIM_SRC_THERMAL);
		wlrx_intfr_clear_settings(WLTRX_DRV_MAIN);
	}
}

bool wlc_get_high_pwr_test_flag(void)
{
	return g_high_pwr_test_flag;
}

static bool wlc_is_night_time(struct wlrx_dev_info *di)
{
	if (di->sysfs_data.ignore_fan_ctrl)
		return false;
	if (g_high_pwr_test_flag)
		return false;
	if (wlrx_is_car_tx(WLTRX_DRV_MAIN))
		return false;

	/* night time: 21:00-7:00 */
	return power_is_within_time_interval(21, 0, 7, 0);
}

static void wlc_fan_control_handle(struct wlrx_dev_info *di,
	int *retry_cnt, u8 limit_val)
{
	int ret;

	if (*retry_cnt >= WLC_FAN_LIMIT_RETRY_CNT) {
		*retry_cnt = 0;
		di->stat_rcd.fan_last = di->stat_rcd.fan_cur;
		return;
	}

	hwlog_info("[%s] limit_val=0x%x\n", __func__, limit_val);
	ret = wireless_set_fan_speed_limit(WLTRX_IC_MAIN, WIRELESS_PROTOCOL_QI, limit_val);
	if (ret) {
		(*retry_cnt)++;
		return;
	}
	*retry_cnt = 0;
	di->stat_rcd.fan_last = di->stat_rcd.fan_cur;
}

static bool wlrx_need_fan_control(struct wlrx_dev_info *di)
{
	struct wlprot_acc_cap *acc_cap = wlrx_acc_get_cap(WLTRX_DRV_MAIN);

	if (!wlrx_is_std_tx(WLTRX_DRV_MAIN) || !acc_cap || !acc_cap->support_fan)
		return false;

	/* in charger mode, time zone is not available */
	if (power_cmdline_is_factory_mode() ||
		power_cmdline_is_powerdown_charging_mode())
		return false;

	return true;
}

static void wlc_update_fan_control(struct wlrx_dev_info *di, bool force_flag)
{
	static int retry_cnt;
	int tx_pwr;
	u8 thermal_status;
	u8 fan_limit;
	struct wlprot_acc_cap *acc_cap = NULL;

	if (!wlrx_need_fan_control(di))
		return;
	if (!wlrx_ic_is_tx_exist(WLTRX_IC_MAIN) ||
		(wlrx_get_wired_channel_state() == WIRED_CHANNEL_ON))
		return;
	if (!force_flag &&
		(wlrx_get_charge_stage() <= WLRX_STAGE_CHARGING)) {
		retry_cnt = 0;
		return;
	}

	thermal_status = di->sysfs_data.thermal_ctrl &
		WLC_THERMAL_FORCE_FAN_FULL_SPEED;
	acc_cap = wlrx_acc_get_cap(WLTRX_DRV_MAIN);
	if (!acc_cap)
		return;
	tx_pwr = acc_cap->vmax * acc_cap->imax;
	if (wlc_is_night_time(di)) {
		di->stat_rcd.fan_cur = WLC_FAN_HALF_SPEED_MAX;
		wlrx_plim_set_src(WLTRX_DRV_MAIN, WLRX_PLIM_SRC_FAN);
	} else if (tx_pwr <= WLC_FAN_CTRL_PWR) {
		di->stat_rcd.fan_cur = WLC_FAN_FULL_SPEED_MAX;
		wlrx_plim_clear_src(WLTRX_DRV_MAIN, WLRX_PLIM_SRC_FAN);
	} else if (thermal_status == WLC_THERMAL_FORCE_FAN_FULL_SPEED) {
		di->stat_rcd.fan_cur = WLC_FAN_FULL_SPEED;
		wlrx_plim_clear_src(WLTRX_DRV_MAIN, WLRX_PLIM_SRC_FAN);
	} else {
		di->stat_rcd.fan_cur = WLC_FAN_FULL_SPEED_MAX;
		wlrx_plim_clear_src(WLTRX_DRV_MAIN, WLRX_PLIM_SRC_FAN);
	}

	if (di->stat_rcd.fan_last != di->stat_rcd.fan_cur) {
		switch (di->stat_rcd.fan_cur) {
		case WLC_FAN_HALF_SPEED_MAX:
			fan_limit = WLC_FAN_HALF_SPEED_MAX_QI;
			break;
		case WLC_FAN_FULL_SPEED_MAX:
			fan_limit = WLC_FAN_FULL_SPEED_MAX_QI;
			break;
		case WLC_FAN_FULL_SPEED:
			fan_limit = WLC_FAN_FULL_SPEED_QI;
			break;
		default:
			return;
		}
		wlc_fan_control_handle(di, &retry_cnt, fan_limit);
	}
}

static void wireless_charge_update_status(struct wlrx_dev_info *di)
{
	wireless_charge_update_fop(false);
	wlc_update_charge_state(di);
	wlc_update_fan_control(di, false);
}

static int wireless_charge_set_power_mode(struct wlrx_dev_info *di, int pid)
{
	int ret;
	int curr_pid;
	struct wlrx_pmode *pcfg = NULL;

	ret = wireless_charge_vout_control(di, pid);
	if (ret)
		return ret;

	curr_pid = wlrx_pmode_get_curr_pid(WLTRX_DRV_MAIN);
	if (pid == curr_pid)
		return 0;

	pcfg = wlrx_pmode_get_pcfg_by_pid(WLTRX_DRV_MAIN, pid);
	if (!pcfg)
		return 0;

	if (pcfg->timeout > 0)
		di->curr_power_time_out = jiffies + msecs_to_jiffies(
			pcfg->timeout * POWER_MS_PER_S);
	wlrx_pmode_set_curr_pid(WLTRX_DRV_MAIN, pid);
	if (wireless_charge_set_rx_vout(pcfg->vrx))
		hwlog_err("%s: set rx vout fail\n", __func__);

	return 0;
}

static void wlrx_switch_power_mode(struct wlrx_dev_info *di, int start_pid, int end_pid)
{
	int pid;
	struct wlrx_pmode *pcfg = NULL;

	if (!wlrx_pmode_is_pid_valid(WLTRX_DRV_MAIN, start_pid) ||
		!wlrx_pmode_is_pid_valid(WLTRX_DRV_MAIN, end_pid))
		return;

	/* start sample, don't switch pmode */
	if ((wlrx_get_charge_stage() != WLRX_STAGE_CHARGING) && (di->start_sample_type > 0))
		return;

	for (pid = start_pid; pid >= end_pid; pid--) {
		if (!wlrx_pmode_judge(WLTRX_DRV_MAIN, pid, WLRX_PMODE_FINAL_JUDGE))
			continue;
		pcfg = wlrx_pmode_get_pcfg_by_pid(WLTRX_DRV_MAIN, pid);
		if (pcfg && strstr(pcfg->name, "SC"))
			return;
		if (!wireless_charge_set_power_mode(di, pid))
			break;
	}
	if (pid < 0) {
		wlrx_pmode_set_curr_pid(WLTRX_DRV_MAIN, 0);
		wireless_charge_set_power_mode(di, 0);
	}
}

static void wireless_charge_power_mode_control(struct wlrx_dev_info *di)
{
	int curr_pid;
	int pcfg_level = wlrx_pmode_get_pcfg_level(WLTRX_DRV_MAIN);
	struct wlrx_pmode *curr_pcfg = wlrx_pmode_get_curr_pcfg(WLTRX_DRV_MAIN);

	if (!curr_pcfg || (pcfg_level <= 0))
		return;

	curr_pid = wlrx_pmode_get_curr_pid(WLTRX_DRV_MAIN);
	if (wlrx_pmode_judge(WLTRX_DRV_MAIN, curr_pid, WLRX_PMODE_FINAL_JUDGE)) {
		if (wlrx_get_charge_stage() == WLRX_STAGE_CHARGING)
			wlrx_switch_power_mode(di, pcfg_level - 1, 0);
		else
			wlrx_switch_power_mode(di, curr_pcfg->expect_mode, curr_pid + 1);
	} else {
		wlrx_switch_power_mode(di, curr_pid - 1, 0);
	}
	if (wlrx_ic_is_sleep_enable(WLTRX_IC_MAIN)) {
		hwlog_info("power_mode_control: sleep_en eanble, return\n");
		return;
	}

	wlc_revise_vout_para(di);
	wlc_iout_control(di);
}

static void wlc_recheck_cert_preprocess(struct wlrx_dev_info *di)
{
	/* vout may be 9v, so here reset 5V for cert stability */
	if (wldc_set_trx_vout(TX_DEFAULT_VOUT, TX_DEFAULT_VOUT))
		hwlog_err("%s: set default vout failed\n", __func__);

	wlc_set_iout_min(di);
	(void)power_msleep(WLRX_ILIM_DELAY, DT_MSLEEP_25MS, wlrx_msleep_exit);
	wlrx_set_charge_stage(WLRX_STAGE_AUTH);
}

static void wireless_charge_regulation(struct wlrx_dev_info *di)
{
	if ((wlrx_get_charge_stage() != WLRX_STAGE_REGULATION) ||
		(wlrx_get_wireless_channel_state() == WIRELESS_CHANNEL_OFF))
		return;

	if (wlrx_acc_auth_need_recheck(WLTRX_DRV_MAIN)) {
		wlc_recheck_cert_preprocess(di);
		return;
	}

	wireless_charge_update_max_vout_and_iout(false);
	wireless_charge_power_mode_control(di);
}

static void read_back_color_from_nv(char *back_color, unsigned int back_len)
{
	power_nv_read(POWER_NV_DEVCOLR, back_color, back_len);
}

static void wlc_check_pu_shell(struct wlrx_dev_info *di)
{
	static bool check_done;
	static char dev_color[BACK_DEVICE_COLOR_LEN];

	if (check_done)
		return;

	memset(dev_color, 0, sizeof(BACK_DEVICE_COLOR_LEN));
	read_back_color_from_nv(dev_color, BACK_DEVICE_COLOR_LEN - 1);
	hwlog_info("[%s] pu_color:%s\n", __func__, dev_color);
	check_done = true;

	if (!strncmp(dev_color, "puorange", strlen("puorange")) ||
		!strncmp(dev_color, "pugreen", strlen("pugreen")))
		wlrx_ic_set_pu_shell_flag(WLTRX_IC_MAIN, true);
	else
		wlrx_ic_set_pu_shell_flag(WLTRX_IC_MAIN, false);
}

static void wlc_set_ext_fod_flag(struct wlrx_dev_info *di)
{
	if (wlrx_ic_need_chk_pu_shell(WLTRX_IC_MAIN))
		wlc_check_pu_shell(di);
}

static void wireless_charge_start_charging(struct wlrx_dev_info *di)
{
	int vtx;
	struct wlrx_pmode *curr_pcfg = NULL;

	if ((wlrx_get_charge_stage() != WLRX_STAGE_CHARGING) ||
		(wlrx_get_wireless_channel_state() == WIRELESS_CHANNEL_OFF))
		return;

	wlc_get_supported_max_rx_vout(di);
	wlc_extra_power_supply(true);
	wlc_update_fan_control(di, true);
	wireless_charge_update_fop(true);
	wlc_update_kb_control(WLC_START_CHARING);
	wireless_charge_update_max_vout_and_iout(true);
	wireless_charge_icon_display(WLRX_PMODE_NORMAL_JUDGE);

	curr_pcfg = wlrx_pmode_get_curr_pcfg(WLTRX_DRV_MAIN);
	if (curr_pcfg) {
		vtx = min(di->tx_vout_max, curr_pcfg->vtx);
		power_event_bnc_notify(POWER_BNT_WLC, POWER_NE_WLC_CHARGER_VBUS, &vtx);
	}

	di->iout_low_cnt = 0;
	wireless_charge_power_mode_control(di);
	(void)wireless_send_charge_event(WLTRX_IC_MAIN, WIRELESS_PROTOCOL_QI,
		RX_STATR_CHARGING);

	curr_pcfg = wlrx_pmode_get_curr_pcfg(WLTRX_DRV_MAIN);
	if (curr_pcfg && strstr(curr_pcfg->name, "SC"))
		return;

	wlrx_set_charge_stage(WLRX_STAGE_REGULATION);
}

static bool wlc_is_support_set_rpp_format(void)
{
	int ret;
	u8 tx_rpp = 0;
	struct wlrx_dts *dts = wlrx_get_dts();

	if (!dts || dts->pmax <= 0)
		return false;

	if (!wlrx_acc_auth_succ(WLTRX_DRV_MAIN))
		return false;

	ret = wireless_get_rpp_format(WLTRX_IC_MAIN, WIRELESS_PROTOCOL_QI, &tx_rpp);
	if (!ret && (tx_rpp == HWQI_ACK_RPP_FORMAT_24BIT))
		return true;

	return false;
}

static int wlc_set_rpp_format(struct wlrx_dev_info *di)
{
	int ret;
	int count = 0;
	struct wlrx_dts *dts = wlrx_get_dts();

	if (!dts)
		return -EINVAL;

	do {
		ret = wireless_set_rpp_format(WLTRX_IC_MAIN, WIRELESS_PROTOCOL_QI, dts->pmax);
		if (!ret) {
			hwlog_info("%s: succ\n", __func__);
			return 0;
		}
		(void)power_msleep(WLRX_SET_RPP_FORMAT_RETRY_DELAY,
			DT_MSLEEP_25MS, wlrx_msleep_exit);
		count++;
		hwlog_err("%s: failed, try next time\n", __func__);
	} while (count < WLC_SET_RPP_FORMAT_RETRY_CNT);

	if (count < WLC_SET_RPP_FORMAT_RETRY_CNT) {
		hwlog_info("[%s] succ\n", __func__);
		return 0;
	}

	return -EIO;
}

static void wlc_rpp_format_init(struct wlrx_dev_info *di)
{
	int ret;

	if (!wlrx_is_std_tx(WLTRX_DRV_MAIN) || wlrx_is_fac_tx(WLTRX_DRV_MAIN))
		return;

	if (!wlc_is_support_set_rpp_format()) {
		wlrx_plim_set_src(WLTRX_DRV_MAIN, WLRX_PLIM_SRC_RPP);
		return;
	}

	ret = wlc_set_rpp_format(di);
	if (!ret) {
		hwlog_info("[%s] succ\n", __func__);
		wlrx_plim_clear_src(WLTRX_DRV_MAIN, WLRX_PLIM_SRC_RPP);
		return;
	}
	wlrx_plim_set_src(WLTRX_DRV_MAIN, WLRX_PLIM_SRC_RPP);
}

static void wireless_charge_check_fwupdate(struct wlrx_dev_info *di)
{
	int ret;

	if ((wlrx_get_charge_stage() != WLRX_STAGE_FW_UPDATE) ||
		(wlrx_get_wireless_channel_state() == WIRELESS_CHANNEL_OFF))
		return;

	ret = wlrx_ic_fw_update(WLTRX_IC_MAIN);
	if (!ret)
		wireless_charge_chip_init(WIRELESS_CHIP_INIT);

	charge_pump_chip_init(CP_TYPE_MAIN);
	charge_pump_chip_init(CP_TYPE_AUX);
	wlc_rpp_format_init(di);
	wlrx_set_charge_stage(WLRX_STAGE_CHARGING);
}

static bool wlc_need_check_certification(struct wlrx_dev_info *di)
{
	int pid;
	int pcfg_level;
	struct wlrx_pmode *pcfg = NULL;
	struct wlprot_acc_cap *acc_cap = wlrx_acc_get_cap(WLTRX_DRV_MAIN);

	if (!acc_cap || acc_cap->no_need_cert)
		return false;

	pcfg_level = wlrx_pmode_get_pcfg_level(WLTRX_DRV_MAIN);
	for (pid = pcfg_level - 1; pid >= 0; pid--) {
		if (wlrx_pmode_judge(WLTRX_DRV_MAIN, pid, WLRX_PMODE_QUICK_JUDGE))
			break;
	}
	if (pid < 0)
		pid = 0;

	pcfg = wlrx_pmode_get_pcfg_by_pid(WLTRX_DRV_MAIN, pid);
	if (pcfg && pcfg->auth > 0)
		return true;

	hwlog_info("%s: max pmode=%d\n", __func__, pid);
	return false;
}

static void wireless_charge_check_certification(struct wlrx_dev_info *di)
{
	int ret;
	char dsm_buff[POWER_DSM_BUF_SIZE_0512] = { 0 };

	if ((wlrx_get_charge_stage() != WLRX_STAGE_AUTH) ||
		(wlrx_get_wireless_channel_state() == WIRELESS_CHANNEL_OFF))
		return;

	if (!wlc_need_check_certification(di)) {
		wlrx_set_charge_stage(WLRX_STAGE_FW_UPDATE);
		return;
	}

	wlc_set_iout_min(di);
	ret = wlrx_acc_auth(WLTRX_DRV_MAIN);
	if (ret == WLRX_ACC_AUTH_SUCC) {
		wlrx_set_charge_stage(WLRX_STAGE_FW_UPDATE);
	} else if (ret == WLRX_ACC_AUTH_CNT_ERR) {
		if (di->certi_comm_err_cnt > 0) {
			wlc_rx_chip_reset(di);
		} else {
			wireless_charge_icon_display(WLRX_PMODE_NORMAL_JUDGE);
			wlrx_dsm_report(WLTRX_DRV_MAIN, POWER_DSM_ERROR_WIRELESS_CERTI_COMM_FAIL,
				dsm_buff, sizeof(dsm_buff));
		}
		wlrx_set_charge_stage(WLRX_STAGE_FW_UPDATE);
	} else if ((ret == WLRX_ACC_AUTH_DEV_ERR) || (ret == WLRX_ACC_AUTH_SRV_NOT_READY)) {
		wlrx_set_charge_stage(WLRX_STAGE_FW_UPDATE);
	} else if (ret == WLRX_ACC_AUTH_CM_ERR) {
		++di->certi_comm_err_cnt;
	} else if (ret == WLRX_ACC_AUTH_SRV_ERR) {
		di->certi_comm_err_cnt = 0;
	}
}

static void wireless_charge_cable_detect(struct wlrx_dev_info *di)
{
	if ((wlrx_get_charge_stage() != WLRX_STAGE_CABLE_DET) ||
		(wlrx_get_wireless_channel_state() == WIRELESS_CHANNEL_OFF))
		return;

	di->cable_detect_succ_flag = WIRELESS_CHECK_SUCC;
	wlrx_set_charge_stage(WLRX_STAGE_AUTH);
}

static void wireless_charge_check_tx_ability(struct wlrx_dev_info *di)
{
	int ret;
	char dsm_buff[POWER_DSM_BUF_SIZE_0512] = {0};

	if ((wlrx_get_charge_stage() != WLRX_STAGE_GET_TX_CAP) ||
		(wlrx_get_wireless_channel_state() == WIRELESS_CHANNEL_OFF))
		return;

	wlc_set_iout_min(di);
	ret = wlrx_acc_detect_cap(WLTRX_DRV_MAIN);
	if ((ret == WLRX_ACC_DET_CAP_SUCC) || (ret == WLRX_ACC_DET_CAP_DEV_ERR)) {
		wlrx_set_charge_stage(WLRX_STAGE_CABLE_DET);
	} else if (ret == WLRX_ACC_DET_CAP_CNT_ERR) {
		if (wlrx_is_std_tx(WLTRX_DRV_MAIN) &&
			(di->wlc_err_rst_cnt >= WLC_RST_CNT_MAX))
			wlrx_dsm_report(WLTRX_DRV_MAIN, POWER_DSM_ERROR_WIRELESS_CHECK_TX_ABILITY_FAIL,
				dsm_buff, sizeof(dsm_buff));
		wlc_rx_chip_reset(di);
		wlrx_set_charge_stage(WLRX_STAGE_CABLE_DET);
	}
}

static void wireless_charge_check_tx_id(struct wlrx_dev_info *di)
{
	int ret;

	if ((wlrx_get_charge_stage() != WLRX_STAGE_HANDSHAKE) ||
		(wlrx_get_wireless_channel_state() == WIRELESS_CHANNEL_OFF))
		return;

	wlc_set_iout_min(di);
	ret = wlrx_acc_handshake(WLTRX_DRV_MAIN);
	if (ret == WLRX_ACC_HS_SUCC)
		wlrx_set_charge_stage(WLRX_STAGE_GET_TX_CAP);
	else if ((ret == WLRX_ACC_HS_DEV_ERR) || (ret == WLRX_ACC_HS_CNT_ERR) ||
		(ret == WLRX_ACC_HS_ID_ERR))
		wlrx_set_charge_stage(WLRX_STAGE_CABLE_DET);
}

static void wireless_charge_rx_stop_charing_config(struct wlrx_dev_info *di)
{
	wlrx_ic_stop_charging(WLTRX_IC_MAIN);
	wireless_reset_dev_info(WLTRX_IC_MAIN, WIRELESS_PROTOCOL_QI);
}

static void wlc_state_record_para_init(struct wlrx_dev_info *di)
{
	di->stat_rcd.chrg_state_cur = 0;
	di->stat_rcd.chrg_state_last = 0;
	di->stat_rcd.fan_cur = WLC_FAN_UNKNOWN_SPEED;
	di->stat_rcd.fan_last = WLC_FAN_UNKNOWN_SPEED;
}

static void wireless_charge_para_init(struct wlrx_dev_info *di)
{
	struct wlrx_dts *dts = wlrx_get_dts();

	if (!dts)
		return;

	di->monitor_interval = wlrx_get_monitor_interval();
	di->ctrl_interval = CONTROL_INTERVAL_NORMAL;
	di->tx_vout_max = TX_DEFAULT_VOUT;
	di->rx_vout_max = RX_DEFAULT_VOUT;
	di->rx_iout_max = dts->rx_imin;
	di->rx_iout_limit = dts->rx_imin;
	di->certi_comm_err_cnt = 0;
	di->boost_err_cnt = 0;
	di->sysfs_data.en_enable = 0;
	di->iout_high_cnt = 0;
	di->iout_low_cnt = 0;
	di->cable_detect_succ_flag = 0;
	di->curr_tx_type_index = 0;
	wlrx_pmode_set_curr_pid(WLTRX_DRV_MAIN, 0);
	wlc_set_cur_vmode_id(di, 0);
	di->curr_power_time_out = 0;
	di->pwroff_reset_flag = 0;
	di->supported_rx_vout = RX_DEFAULT_VOUT;
	di->extra_pwr_good_flag = 1;
	di->qval_support_mode = WLRX_SUPP_PMODE_ALL;
	g_rx_vrect_restore_cnt = 0;
	g_rx_vout_err_cnt = 0;
	di->start_sample_type = 0;
	wlrx_evt_clear_err_cnt(WLTRX_DRV_MAIN);
	wlrx_reset_fsk_alarm();
	wlrx_plim_reset_src(WLTRX_DRV_MAIN);
	wlc_state_record_para_init(di);
	wlrx_acc_reset_para(WLTRX_DRV_MAIN);
	wlc_reset_icon_pmode(di);
	wlrx_buck_set_iin_prop(WLTRX_DRV_MAIN, dts->rx_istep, WLRX_BUCK_IIN_DELAY, 0);
}

static void wireless_charge_control_work(struct work_struct *work)
{
	struct wlrx_dev_info *di = container_of(work,
		struct wlrx_dev_info, wireless_ctrl_work.work);

	if (!di) {
		hwlog_err("control_work: para null\n");
		return;
	}

	wireless_charge_check_tx_id(di);
	wireless_charge_check_tx_ability(di);
	wireless_charge_cable_detect(di);
	wireless_charge_check_certification(di);
	wireless_charge_check_fwupdate(di);
	wireless_charge_start_charging(di);
	wireless_charge_regulation(di);
	wireless_charge_set_ctrl_interval(di);

	if ((wlrx_get_wireless_channel_state() == WIRELESS_CHANNEL_ON) &&
		(wlrx_get_charge_stage() != WLRX_STAGE_REGULATION_DC))
		schedule_delayed_work(&di->wireless_ctrl_work,
			msecs_to_jiffies(di->ctrl_interval));
}

void wireless_charge_restart_charging(unsigned int stage_from)
{
	struct wlrx_dev_info *di = g_wlrx_di;

	if (!di) {
		hwlog_err("%s: di null\n", __func__);
		return;
	}
	if ((wlrx_get_wireless_channel_state() == WIRELESS_CHANNEL_ON) &&
		(wlrx_get_charge_stage() > WLRX_STAGE_CHARGING)) {
		wlrx_set_charge_stage(stage_from);
		schedule_delayed_work(&di->wireless_ctrl_work,
			msecs_to_jiffies(100)); /* 100ms for pmode stability */
	}
}

static void wireless_charge_switch_off(void)
{
	wlps_control(WLTRX_IC_MAIN, WLPS_SC_SW2, false);
	wlps_control(WLTRX_IC_MAIN, WLPS_RX_SW_AUX, false);
	wlrx_ic_sleep_enable(WLTRX_IC_MAIN, true);
	charge_pump_chip_enable(CP_TYPE_AUX, false);
	wlps_control(WLTRX_IC_MAIN, WLPS_RX_SW, false);
	charge_pump_chip_enable(CP_TYPE_MAIN, false);
	charger_select_channel(CHARGER_CH_USBIN);
}

static void wireless_charge_stop_charging(struct wlrx_dev_info *di)
{
	hwlog_info("%s ++\n", __func__);
	wlrx_ic_sleep_enable(WLTRX_IC_MAIN, true);
	wlc_extra_power_supply(false);
	pd_dpm_ignore_vbus_only_event(false);
	wlrx_set_charge_stage(WLRX_STAGE_DEFAULT);
	wlrx_buck_set_iin_prop(WLTRX_DRV_MAIN, 0, 0, 0);
	wireless_charge_rx_stop_charing_config(di);
	wlc_update_kb_control(WLC_STOP_CHARING);
	wlrx_ui_reset_icon_flag(WLTRX_DRV_MAIN, WLRX_UI_FAST_CHARGE);
	wlrx_ui_reset_icon_flag(WLTRX_DRV_MAIN, WLRX_UI_SUPER_CHARGE);
	g_fop_fixed_flag = 0;
	g_need_force_5v_vout = false;
	g_plimit_time_num = 0;
	cancel_delayed_work_sync(&di->rx_sample_work);
	cancel_delayed_work_sync(&di->wireless_ctrl_work);
	cancel_delayed_work_sync(&di->ignore_qval_work);
	wlrx_pmode_set_curr_pid(WLTRX_DRV_MAIN, 0);
	wlrx_ui_reset_icon_type(WLTRX_DRV_MAIN);
	di->wlc_err_rst_cnt = 0;
	di->supported_rx_vout = RX_DEFAULT_VOUT;
	wlrx_acc_reset_para(WLTRX_DRV_MAIN);
	hwlog_info("%s --\n", __func__);
}

static void wlc_wireless_vbus_connect_handler(unsigned int stage_from)
{
	struct wlrx_dev_info *di = g_wlrx_di;
	struct wlrx_pmode *curr_pcfg = NULL;

	if (!di) {
		hwlog_err("wireless_vbus_connect_handler: para null\n");
		return;
	}

	if (wlrx_get_wired_channel_state() == WIRED_CHANNEL_ON) {
		hwlog_err("%s: wired vbus connect\n", __func__);
		return;
	}

	wlrx_set_wireless_channel_state(WIRELESS_CHANNEL_ON);
	wlrx_cut_off_wired_channel(WLTRX_DRV_MAIN);
	wlps_control(WLTRX_IC_MAIN, WLPS_RX_SW, true);
	charge_pump_chip_enable(CP_TYPE_MAIN, true);
	wlrx_ic_sleep_enable(WLTRX_IC_MAIN, false);
	charger_select_channel(CHARGER_CH_WLSIN);
	wireless_charge_chip_init(WIRELESS_CHIP_INIT);
	wlc_set_cur_vmode_id(di, 0);

	wlrx_pmode_set_curr_pid(WLTRX_DRV_MAIN, 0);
	curr_pcfg = wlrx_pmode_get_curr_pcfg(WLTRX_DRV_MAIN);
	if (curr_pcfg) {
		di->tx_vout_max = curr_pcfg->vtx;
		di->rx_vout_max = curr_pcfg->vrx;
		if (wireless_charge_set_rx_vout(di->rx_vout_max))
			hwlog_err("%s: set rx vout failed\n", __func__);
	}

	if (wlrx_get_wireless_channel_state() == WIRELESS_CHANNEL_ON) {
		wlrx_set_charge_stage(stage_from);
		wlc_set_iout_min(di);
		mod_delayed_work(system_wq, &di->wireless_ctrl_work,
			msecs_to_jiffies(di->ctrl_interval));
		power_event_bnc_notify(POWER_BNT_WLC, POWER_NE_WLC_CHARGER_VBUS, &di->tx_vout_max);
		hwlog_info("%s --\n", __func__);
	}
}

static void wlrx_wls_disconnect_set_wired_channel(void)
{
	if (!wdcm_dev_exist())
		return;

	wdcm_set_buck_channel_state(WDCM_CLIENT_WLS, WDCM_DEV_OFF);
}

static void wireless_charge_wireless_vbus_disconnect_handler(void)
{
	struct wlrx_dev_info *di = g_wlrx_di;

	if (!di) {
		hwlog_err("%s: di null\n", __func__);
		return;
	}
	if (wlrx_ic_is_tx_exist(WLTRX_IC_MAIN)) {
		hwlog_info("[%s] tx exist, ignore\n", __func__);
		power_event_bnc_notify(POWER_BNT_WLC, POWER_NE_WLC_VBUS_CONNECT, NULL);
		mod_delayed_work(system_wq,
			&di->wireless_monitor_work, msecs_to_jiffies(0));
		mod_delayed_work(system_wq,
			&di->wireless_watchdog_work, msecs_to_jiffies(0));
		wldc_set_charge_stage(WLDC_STAGE_DEFAULT);
		wlc_wireless_vbus_connect_handler(WLRX_STAGE_REGULATION);
		return;
	}
	wlrx_set_wireless_channel_state(WIRELESS_CHANNEL_OFF);
	wireless_charge_switch_off();
	power_msleep(DT_MSLEEP_20MS, 0, NULL); /* delay 20ms for preventing volt backflow */
	wlrx_wls_disconnect_set_wired_channel();
	charger_source_sink_event(STOP_SINK_WIRELESS);
	wireless_charge_stop_charging(di);
}

static void wireless_charge_wireless_vbus_disconnect_work(
	struct work_struct *work)
{
	wireless_charge_wireless_vbus_disconnect_handler();
}

static void wlrx_wired_connect_set_wired_channel(void)
{
	if (!wdcm_dev_exist()) {
		wired_chsw_set_wired_channel(WIRED_CHANNEL_BUCK, WIRED_CHANNEL_RESTORE);
		return;
	}

	wdcm_set_buck_channel_state(WDCM_CLIENT_WIRED, WDCM_DEV_ON);
}

static void wireless_charge_wired_vbus_connect_work(struct work_struct *work)
{
	int i;
	int vout = 0;
	struct wlrx_dev_info *di = g_wlrx_di;

	if (!di) {
		hwlog_err("%s: di null\n", __func__);
		return;
	}
	mutex_lock(&g_rx_en_mutex);
	(void)wlrx_ic_get_vout(WLTRX_IC_MAIN, &vout);
	if (vout >= RX_HIGH_VOUT) {
		wireless_charge_rx_stop_charing_config(di);
		(void)wireless_charge_set_tx_vout(TX_DEFAULT_VOUT);
		(void)wireless_charge_set_rx_vout(TX_DEFAULT_VOUT);
		if (wlrx_get_wired_channel_state() == WIRED_CHANNEL_OFF) {
			hwlog_err("%s: wired vubs already off, reset rx\n", __func__);
			wlc_rx_chip_reset(di);
		}
		if (!wireless_is_in_tx_mode())
			wlrx_ic_chip_enable(WLTRX_IC_MAIN, false);
		wlrx_set_wireless_channel_state(WIRELESS_CHANNEL_OFF);
	} else {
		if (!wireless_is_in_tx_mode())
			wlrx_ic_chip_enable(WLTRX_IC_MAIN, false);
		wlrx_set_wireless_channel_state(WIRELESS_CHANNEL_OFF);
	}
	mutex_unlock(&g_rx_en_mutex);
	/* wait for 50ms*10=500ms at most, generally 300ms at most */
	for (i = 0; i < 10; i++) {
		if (wldc_is_stop_charging_complete()) {
			wlrx_wired_connect_set_wired_channel();
			break;
		}
		msleep(50);
	}
	hwlog_info("wired vbus connect, turn off wireless channel\n");
	wireless_charge_stop_charging(di);
}

static void wireless_charge_wired_vbus_disconnect_work(struct work_struct *work)
{
	mutex_lock(&g_rx_en_mutex);
	wlrx_ic_chip_enable(WLTRX_IC_MAIN, true);
	mutex_unlock(&g_rx_en_mutex);
	hwlog_info("wired vbus disconnect, turn on wireless channel\n");
}

void wireless_charge_wired_vbus_connect_handler(void)
{
	struct wlrx_dev_info *di = g_wlrx_di;
	enum wlrx_ui_icon_type type = wlrx_ui_get_icon_type(WLTRX_DRV_MAIN);

	if (!di) {
		hwlog_err("%s: di null\n", __func__);
		wlrx_wired_connect_set_wired_channel();
		wireless_charge_switch_off();
		return;
	}
	if (wlrx_get_wired_channel_state() == WIRED_CHANNEL_ON) {
		hwlog_err("%s: already in sink_vbus state, ignore\n", __func__);
		return;
	}
	hwlog_info("[%s] wired vbus connect\n", __func__);
	power_event_bnc_notify(POWER_BNT_WLC, POWER_NE_WLC_WIRED_VBUS_CONNECT, NULL);
	wlrx_ui_reset_icon_flag(WLTRX_DRV_MAIN, WLRX_UI_SUPER_CHARGE);
	wlrx_set_wired_channel_state(WIRED_CHANNEL_ON);
	wldc_tx_disconnect_handler();
	wireless_charge_switch_off();
	if (type != WLRX_UI_FAST_CHARGE)
		wlrx_wired_connect_set_wired_channel();
	schedule_work(&di->wired_vbus_connect_work);
}

static void wlrx_wired_disconnect_set_wired_channel(void)
{
	if (!wdcm_dev_exist()) {
		wired_chsw_set_wired_channel(WIRED_CHANNEL_ALL, WIRED_CHANNEL_CUTOFF);
		return;
	}

	wdcm_set_buck_channel_state(WDCM_CLIENT_WIRED, WDCM_DEV_OFF);
}

void wireless_charge_wired_vbus_disconnect_handler(void)
{
	struct wlrx_dev_info *di = g_wlrx_di;
	static bool first_in = true;

	if (!di) {
		hwlog_err("%s: di null\n", __func__);
		return;
	}
	if (!first_in && (wlrx_get_wired_channel_state() == WIRED_CHANNEL_OFF)) {
		hwlog_err("%s: not in sink_vbus state, ignore\n", __func__);
		return;
	}
	first_in = false;
	hwlog_info("[%s] wired vbus disconnect\n", __func__);
	wlrx_set_wired_channel_state(WIRED_CHANNEL_OFF);
	wlrx_wired_disconnect_set_wired_channel();
	power_event_bnc_notify(POWER_BNT_WLC, POWER_NE_WLC_WIRED_VBUS_DISCONNECT, NULL);
	schedule_work(&di->wired_vbus_disconnect_work);
}

#ifdef CONFIG_DIRECT_CHARGER
void direct_charger_disconnect_event(void)
{
	struct wlrx_dev_info *di = g_wlrx_di;

	if (!di) {
		hwlog_err("%s: di null\n", __func__);
		return;
	}
	hwlog_info("wired vbus disconnect in scp charging mode\n");
	wlrx_set_wired_channel_state(WIRED_CHANNEL_OFF);
	wired_chsw_set_wired_channel(WIRED_CHANNEL_ALL, WIRED_CHANNEL_CUTOFF);
	schedule_work(&di->wired_vbus_disconnect_work);
}
#endif

void wireless_charger_pmic_vbus_handler(bool vbus_state)
{
	int vfc_reg = 0;
	struct wlrx_dev_info *di = g_wlrx_di;

	if (!di)
		return;

	if (wireless_tx_get_tx_open_flag())
		return;
	(void)wlrx_ic_get_vfc_reg(WLTRX_IC_MAIN, &vfc_reg);
	if (!vbus_state && (vfc_reg > TX_REG_VOUT) &&
		wlrx_acc_auth_succ(WLTRX_DRV_MAIN))
		wlrx_ic_sleep_enable(WLTRX_IC_MAIN, true);

	wlrx_ic_pmic_vbus_handler(WLTRX_IC_MAIN, vbus_state);
}

static int wireless_charge_check_tx_disconnect(struct wlrx_dev_info *di)
{
	if (wlrx_ic_is_tx_exist(WLTRX_IC_MAIN))
		return 0;

	g_fop_fixed_flag = 0;
	g_need_force_5v_vout = false;
	g_plimit_time_num = 0;
	wldc_tx_disconnect_handler();
	wlrx_ic_sleep_enable(WLTRX_IC_MAIN, true);
	wlrx_set_wireless_channel_state(WIRELESS_CHANNEL_OFF);
	power_event_bnc_notify(POWER_BNT_WLC, POWER_NE_WLC_VBUS_DISCONNECT, NULL);
	wireless_charge_rx_stop_charing_config(di);
	cancel_delayed_work_sync(&di->wireless_ctrl_work);
	cancel_delayed_work_sync(&di->wireless_vbus_disconnect_work);
	schedule_delayed_work(&di->wireless_vbus_disconnect_work,
		msecs_to_jiffies(di->discon_delay_time));
	hwlog_err("%s: tx not exist, delay %dms to report disconnect event\n",
		__func__, di->discon_delay_time);

	return -1;
}

void wlc_reset_wireless_charge(void)
{
	struct wlrx_dev_info *di = g_wlrx_di;

	if (!di)
		return;

	if (delayed_work_pending(&di->wireless_vbus_disconnect_work))
		mod_delayed_work(system_wq, &di->wireless_vbus_disconnect_work,
			msecs_to_jiffies(0));
}

static void wireless_charge_monitor_work(struct work_struct *work)
{
	int ret;
	struct wlrx_dev_info *di = g_wlrx_di;

	if (!di) {
		hwlog_err("%s: di null\n", __func__);
		return;
	}

	ret = wireless_charge_check_tx_disconnect(di);
	if (ret) {
		hwlog_info("[%s] tx disconnect, stop monitor work\n", __func__);
		return;
	}
	wireless_charge_count_avg_iout(di);
	wlc_check_voltage(di);
	wireless_charge_update_status(di);
	wlrx_mon_show_info(WLTRX_DRV_MAIN);

	schedule_delayed_work(&di->wireless_monitor_work,
		msecs_to_jiffies(di->monitor_interval));
}

static void wireless_charge_watchdog_work(struct work_struct *work)
{
	struct wlrx_dev_info *di = container_of(work,
		struct wlrx_dev_info, wireless_watchdog_work.work);

	if (!di || (wlrx_get_wireless_channel_state() == WIRELESS_CHANNEL_OFF))
		return;

	if (wlrx_ic_kick_watchdog(WLTRX_IC_MAIN))
		hwlog_err("%s: fail\n", __func__);

	/* kick watchdog at an interval of 300ms */
	mod_delayed_work(system_wq, &di->wireless_watchdog_work,
		msecs_to_jiffies(300));
}

static void wireless_charge_rx_sample_work(struct work_struct *work)
{
	int rx_vout = 0;
	int rx_iout = 0;
	struct wlrx_dev_info *di = container_of(work,
		struct wlrx_dev_info, rx_sample_work.work);

	if (!di) {
		hwlog_err("%s: di null\n", __func__);
		return;
	}

	di->start_sample_type = WLRX_SAMPLE_WORK_RUNNING;

	/* send confirm message to TX */
	(void)wlrx_ic_get_vout(WLTRX_IC_MAIN, &rx_vout);
	(void)wlrx_ic_get_iout(WLTRX_IC_MAIN, &rx_iout);
	wireless_send_rx_vout(WLTRX_IC_MAIN, WIRELESS_PROTOCOL_QI, rx_vout);
	wireless_send_rx_iout(WLTRX_IC_MAIN, WIRELESS_PROTOCOL_QI, rx_iout);

	hwlog_info("[%s] rx_vout = %d, rx_iout = %d\n", __func__, rx_vout, rx_iout);

	schedule_delayed_work(&di->rx_sample_work, msecs_to_jiffies(RX_SAMPLE_WORK_DELAY));
}

static void wireless_charge_pwroff_reset_work(struct work_struct *work)
{
	struct wlrx_dev_info *di = container_of(work,
		struct wlrx_dev_info, wireless_pwroff_reset_work);

	if (!di) {
		hwlog_err("%s: di null\n", __func__);
		power_wakeup_unlock(g_rx_con_wakelock, false);
		return;
	}
	if (di->pwroff_reset_flag) {
		msleep(60); /* test result, about 60ms */
		wlrx_ic_chip_reset(WLTRX_IC_MAIN);
		wireless_charge_set_tx_vout(TX_DEFAULT_VOUT);
		wireless_charge_set_rx_vout(RX_DEFAULT_VOUT);
	}
	power_wakeup_unlock(g_rx_con_wakelock, false);
}

static void wlc_rx_power_on_ready_handler(struct wlrx_dev_info *di)
{
	wldc_set_charge_stage(WLDC_STAGE_DEFAULT);
	wlrx_set_charge_stage(WLRX_STAGE_DEFAULT);
	wireless_charge_para_init(di);

	if (((di->rx_evt.type == POWER_NE_WLRX_PWR_ON) &&
		(di->rx_evt.pwr_good == RX_PWR_ON_GOOD)) ||
		(di->rx_evt.type == POWER_NE_WLRX_READY)) {
		wltx_reset_reverse_charging();
		wlps_control(WLTRX_IC_MAIN, WLPS_TX_SW, false);
		power_event_bnc_notify(POWER_BNT_WLC, POWER_NE_WLC_VBUS_CONNECT, NULL);
		charger_source_sink_event(START_SINK_WIRELESS);
	}
	pd_dpm_ignore_vbus_only_event(true);
	mod_delayed_work(system_wq, &di->wireless_monitor_work,
		msecs_to_jiffies(0));
	mod_delayed_work(system_wq, &di->wireless_watchdog_work,
		msecs_to_jiffies(0));
	if (delayed_work_pending(&di->wireless_vbus_disconnect_work))
		cancel_delayed_work_sync(&di->wireless_vbus_disconnect_work);
	if (di->rx_evt.type == POWER_NE_WLRX_READY) {
		if (!di->wlc_err_rst_cnt)
			wlrx_ui_reset_icon_flag(WLTRX_DRV_MAIN, WLRX_UI_FAST_CHARGE);
		wlc_set_ext_fod_flag(di);
		di->discon_delay_time = WL_DISCONN_DELAY_MS;
		power_event_bnc_notify(POWER_BNT_WLC, POWER_NE_WLC_READY, NULL);
		wlc_wireless_vbus_connect_handler(WLRX_STAGE_HANDSHAKE);
	}
}

static void wlc_handle_tx_bst_err_evt(void)
{
	hwlog_info("handle_tx_bst_err_evt\n");
	wlrx_plim_set_src(WLTRX_DRV_MAIN, WLRX_PLIM_SRC_TX_BST_ERR);
	wireless_charge_update_max_vout_and_iout(true);
}

static void wlc_handle_rx_ldo_off_evt(struct wlrx_dev_info *di)
{
	hwlog_info("handle_rx_ldo_off_evt\n");
	charger_source_sink_event(STOP_SINK_WIRELESS);
	cancel_delayed_work_sync(&di->wireless_ctrl_work);
	cancel_delayed_work_sync(&di->wireless_monitor_work);
	cancel_delayed_work_sync(&di->wireless_watchdog_work);
}

static void wlc_rx_event_work(struct work_struct *work)
{
	struct wlrx_dev_info *di = container_of(work,
		struct wlrx_dev_info, rx_event_work.work);

	if (!di) {
		power_wakeup_unlock(g_rx_con_wakelock, false);
		return;
	}

	switch (di->rx_evt.type) {
	case POWER_NE_WLRX_PWR_ON:
		hwlog_info("[%s] RX power on\n", __func__);
		wlc_rx_power_on_ready_handler(di);
		break;
	case POWER_NE_WLRX_READY:
		hwlog_info("[%s] RX ready\n", __func__);
		wlc_rx_power_on_ready_handler(di);
		break;
	case POWER_NE_WLRX_LDO_OFF:
		wlc_handle_rx_ldo_off_evt(di);
		break;
	case POWER_NE_WLRX_TX_ALARM:
		wlrx_handle_fsk_alarm(&di->rx_evt.tx_alarm);
		break;
	case POWER_NE_WLRX_TX_BST_ERR:
		wlc_handle_tx_bst_err_evt();
		break;
	default:
		break;
	}
	power_wakeup_unlock(g_rx_con_wakelock, false);
}

static void wlc_save_rx_evt_data(struct wlrx_dev_info *di,
	unsigned long event, void *data)
{
	if (!data)
		return;

	switch (di->rx_evt.type) {
	case POWER_NE_WLRX_PWR_ON:
		di->rx_evt.pwr_good = *(int *)data;
		break;
	case POWER_NE_WLRX_TX_ALARM:
		memcpy(&di->rx_evt.tx_alarm, data,
			sizeof(struct wireless_protocol_tx_alarm));
		break;
	default:
		break;
	}
}

static int wireless_charge_rx_event_notifier_call(struct notifier_block *rx_event_nb,
	unsigned long event, void *data)
{
	struct wlrx_dev_info *di = container_of(rx_event_nb,
		struct wlrx_dev_info, rx_event_nb);

	if (!di)
		return NOTIFY_OK;

	switch (event) {
	case POWER_NE_WLRX_PWR_ON:
	case POWER_NE_WLRX_READY:
	case POWER_NE_WLRX_LDO_OFF:
	case POWER_NE_WLRX_TX_ALARM:
	case POWER_NE_WLRX_TX_BST_ERR:
		break;
	default:
		return NOTIFY_OK;
	}

	power_wakeup_lock(g_rx_con_wakelock, false);
	di->rx_evt.type = (int)event;
	wlc_save_rx_evt_data(di, event, data);

	cancel_delayed_work_sync(&di->rx_event_work);
	mod_delayed_work(system_wq, &di->rx_event_work,
		msecs_to_jiffies(0));

	return NOTIFY_OK;
}

static int wireless_charge_pwrkey_event_notifier_call(struct notifier_block *pwrkey_event_nb,
	unsigned long event, void *data)
{
	struct wlrx_dev_info *di = g_wlrx_di;

	if (!di) {
		hwlog_err("%s: di is NULL\n", __func__);
		return NOTIFY_OK;
	}

	if (wlrx_get_wireless_channel_state() == WIRELESS_CHANNEL_OFF) {
		hwlog_err("%s: not in wireless charging\n", __func__);
		return NOTIFY_OK;
	}

	switch (event) {
	case PRESS_KEY_6S:
		power_wakeup_lock(g_rx_con_wakelock, false);
		hwlog_err("%s: response long press 6s interrupt, reset tx vout\n", __func__);
		di->pwroff_reset_flag = 1;
		schedule_work(&di->wireless_pwroff_reset_work);
		break;
	case PRESS_KEY_UP:
		di->pwroff_reset_flag = 0;
		break;
	default:
		break;
	}
	return NOTIFY_OK;
}

static int wireless_charge_chrg_event_notifier_call(struct notifier_block *chrg_event_nb,
	unsigned long event, void *data)
{
	struct wlrx_dev_info *di = container_of(chrg_event_nb,
		struct wlrx_dev_info, chrg_event_nb);

	if (!di)
		return NOTIFY_OK;

	switch (event) {
	case POWER_NE_CHG_CHARGING_DONE:
		hwlog_debug("[%s] charge done\n", __func__);
		di->stat_rcd.chrg_state_cur |= WIRELESS_STATE_CHRG_DONE;
		break;
	default:
		break;
	}

	return NOTIFY_OK;
}

/*
 * There are a numerous options that are configurable on the wireless receiver
 * that go well beyond what the power_supply properties provide access to.
 * Provide sysfs access to them so they can be examined and possibly modified
 * on the fly.
 */
#ifdef CONFIG_SYSFS
static ssize_t wireless_charge_sysfs_show(struct device *dev,
	struct device_attribute *attr, char *buf);

static ssize_t wireless_charge_sysfs_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count);

static struct power_sysfs_attr_info wireless_charge_sysfs_field_tbl[] = {
	power_sysfs_attr_ro(wireless_charge, 0444, WIRELESS_CHARGE_SYSFS_CHIP_INFO, chip_info),
	power_sysfs_attr_ro(wireless_charge, 0444,
		WIRELESS_CHARGE_SYSFS_TX_ADAPTOR_TYPE, tx_adaptor_type),
	power_sysfs_attr_rw(wireless_charge, 0644, WIRELESS_CHARGE_SYSFS_RX_TEMP, rx_temp),
	power_sysfs_attr_rw(wireless_charge, 0644, WIRELESS_CHARGE_SYSFS_VOUT, vout),
	power_sysfs_attr_rw(wireless_charge, 0644, WIRELESS_CHARGE_SYSFS_IOUT, iout),
	power_sysfs_attr_rw(wireless_charge, 0644, WIRELESS_CHARGE_SYSFS_VRECT, vrect),
	power_sysfs_attr_rw(wireless_charge, 0644, WIRELESS_CHARGE_SYSFS_EN_ENABLE, en_enable),
	power_sysfs_attr_ro(wireless_charge, 0444,
		WIRELESS_CHARGE_SYSFS_NORMAL_CHRG_SUCC, normal_chrg_succ),
	power_sysfs_attr_ro(wireless_charge, 0444,
		WIRELESS_CHARGE_SYSFS_FAST_CHRG_SUCC, fast_chrg_succ),
	power_sysfs_attr_rw(wireless_charge, 0644, WIRELESS_CHARGE_SYSFS_FOD_COEF, fod_coef),
	power_sysfs_attr_rw(wireless_charge, 0644,
		WIRELESS_CHARGE_SYSFS_INTERFERENCE_SETTING, interference_setting),
	power_sysfs_attr_rw(wireless_charge, 0644,
		WIRELESS_CHARGE_SYSFS_RX_SUPPORT_MODE, rx_support_mode),
	power_sysfs_attr_rw(wireless_charge, 0644,
		WIRELESS_CHARGE_SYSFS_THERMAL_CTRL, thermal_ctrl),
	power_sysfs_attr_rw(wireless_charge, 0644, WIRELESS_CHARGE_SYSFS_NVM_DATA, nvm_data),
	power_sysfs_attr_rw(wireless_charge, 0644,
		WIRELESS_CHARGE_SYSFS_IGNORE_FAN_CTRL, ignore_fan_ctrl),
};
static struct attribute *wireless_charge_sysfs_attrs[ARRAY_SIZE(wireless_charge_sysfs_field_tbl) + 1];
static const struct attribute_group wireless_charge_sysfs_attr_group = {
	.attrs = wireless_charge_sysfs_attrs,
};

static void wireless_charge_sysfs_create_group(struct device *dev)
{
	power_sysfs_init_attrs(wireless_charge_sysfs_attrs,
		wireless_charge_sysfs_field_tbl, ARRAY_SIZE(wireless_charge_sysfs_field_tbl));
	power_sysfs_create_link_group("hw_power", "charger", "wireless_charger",
		dev, &wireless_charge_sysfs_attr_group);
}

static void wireless_charge_sysfs_remove_group(struct device *dev)
{
	power_sysfs_remove_link_group("hw_power", "charger", "wireless_charger",
		dev, &wireless_charge_sysfs_attr_group);
}
#else
static inline void wireless_charge_sysfs_create_group(struct device *dev)
{
}

static inline void wireless_charge_sysfs_remove_group(struct device *dev)
{
}
#endif

static ssize_t wireless_charge_sysfs_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int chrg_succ;
	int vrect = 0;
	int vout = 0;
	int iout = 0;
	int temp = 0;
	struct wlrx_pmode *curr_pcfg = wlrx_pmode_get_curr_pcfg(WLTRX_DRV_MAIN);
	struct wlprot_acc_cap *acc_cap = wlrx_acc_get_cap(WLTRX_DRV_MAIN);
	struct wlrx_dev_info *di = dev_get_drvdata(dev);
	struct power_sysfs_attr_info *info = power_sysfs_lookup_attr(
		attr->attr.name, wireless_charge_sysfs_field_tbl,
		ARRAY_SIZE(wireless_charge_sysfs_field_tbl));

	if (!di || !curr_pcfg || !acc_cap || !info)
		return -EINVAL;

	switch (info->name) {
	case WIRELESS_CHARGE_SYSFS_CHIP_INFO:
		return wlrx_ic_get_chip_info(WLTRX_IC_MAIN, buf, PAGE_SIZE);
	case WIRELESS_CHARGE_SYSFS_TX_ADAPTOR_TYPE:
		return snprintf(buf, PAGE_SIZE, "%d\n", acc_cap->adp_type);
	case WIRELESS_CHARGE_SYSFS_RX_TEMP:
		(void)wlrx_ic_get_temp(WLTRX_IC_MAIN, &temp);
		return snprintf(buf, PAGE_SIZE, "%d\n", temp);
	case WIRELESS_CHARGE_SYSFS_VOUT:
		(void)wlrx_ic_get_vout(WLTRX_IC_MAIN, &vout);
		return snprintf(buf, PAGE_SIZE, "%d\n", vout);
	case WIRELESS_CHARGE_SYSFS_IOUT:
		(void)wlrx_ic_get_iout(WLTRX_IC_MAIN, &iout);
		return snprintf(buf, PAGE_SIZE, "%d\n", iout);
	case WIRELESS_CHARGE_SYSFS_VRECT:
		(void)wlrx_ic_get_vrect(WLTRX_IC_MAIN, &vrect);
		return snprintf(buf, PAGE_SIZE, "%d\n", vrect);
	case WIRELESS_CHARGE_SYSFS_EN_ENABLE:
		return snprintf(buf, PAGE_SIZE, "%d\n",
			di->sysfs_data.en_enable);
	case WIRELESS_CHARGE_SYSFS_NORMAL_CHRG_SUCC:
		chrg_succ = wireless_charge_check_normal_charge_succ(WLTRX_DRV_MAIN);
		return snprintf(buf, PAGE_SIZE, "%d\n", chrg_succ);
	case WIRELESS_CHARGE_SYSFS_FAST_CHRG_SUCC:
		chrg_succ = wireless_charge_check_fast_charge_succ(WLTRX_DRV_MAIN);
		return snprintf(buf, PAGE_SIZE, "%d\n", chrg_succ);
	case WIRELESS_CHARGE_SYSFS_FOD_COEF:
		return wlrx_ic_get_fod_coef(WLTRX_IC_MAIN, buf, PAGE_SIZE);
	case WIRELESS_CHARGE_SYSFS_INTERFERENCE_SETTING:
		return snprintf(buf, PAGE_SIZE, "%u\n", wlrx_intfr_get_src(WLTRX_DRV_MAIN));
	case WIRELESS_CHARGE_SYSFS_RX_SUPPORT_MODE:
		return snprintf(buf, PAGE_SIZE, "mode[support|current]:[0x%x|%s]\n",
			di->sysfs_data.rx_support_mode, curr_pcfg->name);
	case WIRELESS_CHARGE_SYSFS_THERMAL_CTRL:
		return snprintf(buf, PAGE_SIZE, "%u\n", di->sysfs_data.thermal_ctrl);
	case WIRELESS_CHARGE_SYSFS_NVM_DATA:
		return snprintf(buf, PAGE_SIZE, "%s\n", wlrx_ic_read_nvm_info(
			WLTRX_IC_MAIN, di->sysfs_data.nvm_sec_no));
	case WIRELESS_CHARGE_SYSFS_IGNORE_FAN_CTRL:
		return snprintf(buf, PAGE_SIZE, "%d\n",
			di->sysfs_data.ignore_fan_ctrl);
	default:
		break;
	}
	return 0;
}

static ssize_t wireless_charge_sysfs_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	long val = 0;
	struct wlrx_dev_info *di = dev_get_drvdata(dev);
	struct power_sysfs_attr_info *info = power_sysfs_lookup_attr(
		attr->attr.name, wireless_charge_sysfs_field_tbl,
		ARRAY_SIZE(wireless_charge_sysfs_field_tbl));

	if (!di || !info)
		return -EINVAL;

	switch (info->name) {
	case WIRELESS_CHARGE_SYSFS_EN_ENABLE:
		if ((kstrtol(buf, POWER_BASE_DEC, &val) < 0) ||
			(val < 0) || (val > 1))
			return -EINVAL;
		di->sysfs_data.en_enable = val;
		hwlog_info("set rx en_enable = %d\n", di->sysfs_data.en_enable);
		wlrx_ic_sleep_enable(WLTRX_IC_MAIN, val);
		wlps_control(WLTRX_IC_MAIN, WLPS_SYSFS_EN_PWR,
			di->sysfs_data.en_enable ? true : false);
		break;
	case WIRELESS_CHARGE_SYSFS_FOD_COEF:
		hwlog_info("[%s] set fod_coef: %s\n", __func__, buf);
		(void)wlrx_ic_set_fod_coef(WLTRX_IC_MAIN, buf);
		break;
	case WIRELESS_CHARGE_SYSFS_INTERFERENCE_SETTING:
		if (kstrtol(buf, POWER_BASE_DEC, &val) < 0)
			return -EINVAL;
		hwlog_info("[sysfs_store] interference_settings: 0x%x\n", val);
		wlrx_intfr_handle_settings(WLTRX_DRV_MAIN, (u8)val);
		break;
	case WIRELESS_CHARGE_SYSFS_RX_SUPPORT_MODE:
		if ((kstrtol(buf, POWER_BASE_HEX, &val) < 0) ||
			(val < 0) || (val > WLRX_SUPP_PMODE_ALL))
			return -EINVAL;
		if (!val)
			di->sysfs_data.rx_support_mode = WLRX_SUPP_PMODE_ALL;
		else
			di->sysfs_data.rx_support_mode = val;
		hwlog_info("[%s] rx_support_mode = 0x%x", __func__, val);
		break;
	case WIRELESS_CHARGE_SYSFS_THERMAL_CTRL:
		if ((kstrtol(buf, POWER_BASE_DEC, &val) < 0) ||
			(val < 0) || (val > 0xFF)) /* 0xFF: maximum of u8 */
			return -EINVAL;
		wireless_set_thermal_ctrl((unsigned char)val);
		break;
	case WIRELESS_CHARGE_SYSFS_NVM_DATA:
		if ((strict_strtol(buf, POWER_BASE_DEC, &val) < 0) || (val < 0))
			return -EINVAL;
		di->sysfs_data.nvm_sec_no = val;
		break;
	case WIRELESS_CHARGE_SYSFS_IGNORE_FAN_CTRL:
		if ((kstrtol(buf, POWER_BASE_DEC, &val) < 0) ||
			(val < 0) || (val > 1)) /* 1: ignore 0:otherwise */
			return -EINVAL;
		hwlog_info("[%s] ignore_fan_ctrl=0x%x", __func__, val);
		di->sysfs_data.ignore_fan_ctrl = val;
		break;
	default:
		break;
	}
	return count;
}

static struct wlrx_dev_info *wlrx_dev_info_alloc(void)
{
	struct wlrx_dev_info *di = NULL;
	struct wlrx_pctrl *pctrl = NULL;

	di = kzalloc(sizeof(*di), GFP_KERNEL);
	if (!di)
		return NULL;
	pctrl = kzalloc(sizeof(*pctrl), GFP_KERNEL);
	if (!pctrl)
		goto pctrl_fail;

	di->pctrl = pctrl;
	return di;

pctrl_fail:
	kfree(di);
	return NULL;
}

static void wlrx_dev_info_free(struct wlrx_dev_info *di)
{
	if (di) {
		if (di->pctrl) {
			kfree(di->pctrl);
			di->pctrl = NULL;
		}
		kfree(di);
		di = NULL;
	}
	g_wlrx_di = NULL;
}

static void wlrx_init_probe_para(struct wlrx_dev_info *di)
{
	di->sysfs_data.rx_support_mode = WLRX_SUPP_PMODE_ALL;
	di->qval_support_mode = WLRX_SUPP_PMODE_ALL;
	if (power_cmdline_is_factory_mode())
		di->sysfs_data.rx_support_mode &= ~WLRX_SUPP_PMODE_SC2;
	di->discon_delay_time = WL_DISCONN_DELAY_MS;
	wlrx_acc_reset_para(WLTRX_DRV_MAIN);
	wlc_reset_icon_pmode(di);
}

static void wireless_charge_shutdown(struct platform_device *pdev)
{
	int ret;
	struct wlrx_dev_info *di = platform_get_drvdata(pdev);

	hwlog_info("%s ++\n", __func__);
	if (!di) {
		hwlog_err("%s: di is null\n", __func__);
		return;
	}
	if (wlrx_get_wireless_channel_state() == WIRELESS_CHANNEL_ON) {
		di->pwroff_reset_flag = true;
		wireless_charge_switch_off();
		wlps_control(WLTRX_IC_MAIN, WLPS_RX_EXT_PWR, false);
		msleep(50); /* delay 50ms for power off */
		ret = wireless_charge_set_tx_vout(ADAPTER_5V *
			POWER_MV_PER_V);
		if (ret)
			hwlog_err("%s: wlc sw control fail\n", __func__);
	}
	wlrx_ic_sleep_enable(WLTRX_IC_MAIN, true);
	cancel_delayed_work(&di->rx_sample_work);
	cancel_delayed_work(&di->wireless_ctrl_work);
	cancel_delayed_work(&di->wireless_monitor_work);
	cancel_delayed_work(&di->wireless_watchdog_work);
	cancel_delayed_work(&di->ignore_qval_work);
	hwlog_info("%s --\n", __func__);
}

static int wireless_charge_remove(struct platform_device *pdev)
{
	struct wlrx_dev_info *di = platform_get_drvdata(pdev);

	hwlog_info("%s ++\n", __func__);
	if (!di) {
		hwlog_err("%s: di is null\n", __func__);
		return 0;
	}

	power_event_bnc_unregister(POWER_BNT_CHG, &di->chrg_event_nb);
	power_event_bnc_unregister(POWER_BNT_WLRX, &di->rx_event_nb);
	wireless_charge_sysfs_remove_group(di->dev);
	wlrx_kfree_dts();
	power_wakeup_source_unregister(g_rx_con_wakelock);

	hwlog_info("%s --\n", __func__);

	return 0;
}

static void wlrx_module_deinit(unsigned int drv_type)
{
	wlrx_common_deinit(drv_type);
	wlrx_evt_deinit(drv_type);
	wlrx_acc_deinit(drv_type);
	wlrx_fod_deinit(drv_type);
	wlrx_pctrl_deinit(drv_type);
	wlrx_pmode_deinit(drv_type);
	wlrx_buck_ictrl_deinit(drv_type);
	wlrx_ui_deinit(drv_type);
}

static int wlrx_module_init(unsigned int drv_type, struct device *dev)
{
	int ret;

	ret = wlrx_common_init(drv_type, dev);
	if (ret)
		goto exit;
	ret = wlrx_evt_init(drv_type, dev);
	if (ret)
		goto exit;
	ret = wlrx_acc_init(drv_type, dev);
	if (ret)
		goto exit;
	ret = wlrx_fod_init(drv_type, dev);
	if (ret)
		goto exit;
	ret = wlrx_pctrl_init(drv_type, dev);
	if (ret)
		goto exit;
	ret = wlrx_pmode_init(drv_type, dev);
	if (ret)
		goto exit;
	ret = wlrx_buck_ictrl_init(drv_type, dev);
	if (ret)
		goto exit;
	ret = wlrx_ui_init(drv_type, dev);
	if (ret)
		goto exit;

	return 0;

exit:
	wlrx_module_deinit(drv_type);
	return -ENODEV;
}

static int wireless_charge_probe(struct platform_device *pdev)
{
	int ret;
	struct wlrx_dev_info *di = NULL;
	struct device_node *np = NULL;

	if (!wlrx_ic_is_ops_registered(WLTRX_IC_MAIN))
		return -EPROBE_DEFER;

	di = wlrx_dev_info_alloc();
	if (!di) {
		hwlog_err("alloc di failed\n");
		return -ENOMEM;
	}

	g_wlrx_di = di;
	di->dev = &pdev->dev;
	np = di->dev->of_node;
	platform_set_drvdata(pdev, di);
	g_rx_con_wakelock = power_wakeup_source_register(di->dev, "rx_con_wakelock");

	ret = wlrx_parse_dts(np);
	if (ret)
		goto wireless_charge_fail_0;

	ret = wlrx_module_init(WLTRX_DRV_MAIN, di->dev);
	if (ret)
		goto module_init_fail;

	wlrx_init_probe_para(di);

	mutex_init(&g_rx_en_mutex);
	INIT_WORK(&di->wired_vbus_connect_work, wireless_charge_wired_vbus_connect_work);
	INIT_WORK(&di->wired_vbus_disconnect_work, wireless_charge_wired_vbus_disconnect_work);
	INIT_DELAYED_WORK(&di->rx_event_work, wlc_rx_event_work);
	INIT_WORK(&di->wireless_pwroff_reset_work, wireless_charge_pwroff_reset_work);
	INIT_DELAYED_WORK(&di->wireless_ctrl_work, wireless_charge_control_work);
	INIT_DELAYED_WORK(&di->rx_sample_work, wireless_charge_rx_sample_work);
	INIT_DELAYED_WORK(&di->wireless_monitor_work, wireless_charge_monitor_work);
	INIT_DELAYED_WORK(&di->wireless_watchdog_work, wireless_charge_watchdog_work);
	INIT_DELAYED_WORK(&di->wireless_vbus_disconnect_work,
		wireless_charge_wireless_vbus_disconnect_work);
	INIT_DELAYED_WORK(&di->ignore_qval_work, wlc_ignore_qval_work);

	di->rx_event_nb.notifier_call = wireless_charge_rx_event_notifier_call;
	ret = power_event_bnc_register(POWER_BNT_WLRX, &di->rx_event_nb);
	if (ret < 0) {
		hwlog_err("register rx_connect notifier failed\n");
		goto  wireless_charge_fail_1;
	}
	di->chrg_event_nb.notifier_call = wireless_charge_chrg_event_notifier_call;
	ret = power_event_bnc_register(POWER_BNT_CHG, &di->chrg_event_nb);
	if (ret < 0) {
		hwlog_err("register charger_event notifier failed\n");
		goto  wireless_charge_fail_2;
	}
	di->pwrkey_nb.notifier_call = wireless_charge_pwrkey_event_notifier_call;
	ret = powerkey_register_notifier(&di->pwrkey_nb);
	if (ret < 0) {
		hwlog_err("register power_key notifier failed\n");
		goto  wireless_charge_fail_3;
	}
	if (wlrx_ic_is_tx_exist(WLTRX_IC_MAIN)) {
		wireless_charge_para_init(di);
		charger_source_sink_event(START_SINK_WIRELESS);
		pd_dpm_ignore_vbus_only_event(true);
		wlc_wireless_vbus_connect_handler(WLRX_STAGE_HANDSHAKE);
		schedule_delayed_work(&di->wireless_monitor_work, msecs_to_jiffies(0));
		schedule_delayed_work(&di->wireless_watchdog_work, msecs_to_jiffies(0));
	} else {
		wireless_charge_switch_off();
		wlrx_wls_disconnect_set_wired_channel();
	}
	wireless_charge_sysfs_create_group(di->dev);
	power_if_ops_register(&wl_if_ops);
	hwlog_info("wireless_charger probe ok\n");
	return 0;

wireless_charge_fail_3:
	power_event_bnc_unregister(POWER_BNT_CHG, &di->chrg_event_nb);
wireless_charge_fail_2:
	power_event_bnc_unregister(POWER_BNT_WLRX, &di->rx_event_nb);
wireless_charge_fail_1:
	wlrx_module_deinit(WLTRX_DRV_MAIN);
module_init_fail:
	wlrx_kfree_dts();
wireless_charge_fail_0:
	power_wakeup_source_unregister(g_rx_con_wakelock);
	wlrx_dev_info_free(di);
	platform_set_drvdata(pdev, NULL);
	return ret;
}

static struct of_device_id wireless_charge_match_table[] = {
	{
		.compatible = "huawei,wireless_charger",
		.data = NULL,
	},
	{},
};

static struct platform_driver wireless_charge_driver = {
	.probe = wireless_charge_probe,
	.remove = wireless_charge_remove,
	.shutdown = wireless_charge_shutdown,
	.driver = {
		.name = "huawei,wireless_charger",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(wireless_charge_match_table),
	},
};

static int __init wireless_charge_init(void)
{
	hwlog_info("wireless_charger init ok\n");

	return platform_driver_register(&wireless_charge_driver);
}

static void __exit wireless_charge_exit(void)
{
	platform_driver_unregister(&wireless_charge_driver);
}

device_initcall_sync(wireless_charge_init);
module_exit(wireless_charge_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("wireless charge module driver");
MODULE_AUTHOR("HUAWEI Inc");
