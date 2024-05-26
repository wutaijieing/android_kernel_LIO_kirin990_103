/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2019. All rights reserved.
 * Description: power manage implement.
 * Create: 2019/11/05
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/gpio.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/rtc.h>
#include <linux/regulator/consumer.h>
#include <linux/of_device.h>
#include <linux/suspend.h>
#include <linux/fb.h>
#include <linux/pm_wakeup.h>
#include <linux/delay.h>
#ifdef CONFIG_HUAWEI_DSM
#include <dsm/dsm_pub.h>
#endif
#include <securec.h>
#include <platform_include/smart/linux/base/ap/protocol.h>
#include <platform_include/smart/linux/inputhub_as/device_manager.h>
#include <platform_include/smart/linux/iomcu_boot.h>
#include <platform_include/smart/linux/iomcu_dump.h>
#include <platform_include/smart/linux/iomcu_ipc.h>
#include <platform_include/smart/linux/iomcu_pm.h>
#include <platform_include/smart/linux/iomcu_priv.h>
#include <platform_include/smart/linux/iomcu_log.h>

#define POLL_TIMES 10
#define SENSOR_MAX_RESET_TIME_MS                (400)
#define SENSOR_DETECT_AFTER_POWERON_TIME_MS     (50)
#define SENSORHUB_PM_RESUME_TIMEOUT_MS          1000

/* boot will use, can not add g_ currently */
u32 need_reset_io_power;
u32 need_set_3v_io_power;
u32 need_set_3_1v_io_power;
u32 need_set_3_2v_io_power;
u32 no_need_sensor_ldo24;

static struct iomcu_power_status g_i_power_status;
static unsigned long g_sensor_jiffies;
static int g_sensorhub_reboot_reason_flag = SENSOR_POWER_DO_RESET;
static int g_resume_skip_flag = RESUME_INIT;
int g_sensorhub_wdt_irq = -1;
sys_status_t iom3_sr_status = ST_WAKEUP;

static u32 g_vdd_set_io_power;
static u32 g_use_ldo12_flag;
static int g_iom3_power_state = ST_POWERON;

static struct completion g_iom3_reboot;
static struct completion g_iom3_resume_mini;
static struct completion g_iom3_resume_all;

static struct ipc_debug g_ipc_debug_info;

DEFINE_MUTEX(g_mutex_pstatus);
static struct regulator *g_sensorhub_ldo12_vddio = NULL;

static char *g_sys_status_t_str[] = {
	[ST_SCREENON] = "ST_SCREENON",
	[ST_SCREENOFF] = "ST_SCREENOFF",
	[ST_SLEEP] = "ST_SLEEP",
	[ST_WAKEUP] = "ST_WAKEUP",
};

inline int get_iom3_power_state(void)
{
	return g_iom3_power_state;
}

void atomic_get_iomcu_power_status(struct iomcu_power_status *p)
{
	if (p == NULL) {
		ctxhub_err("atomic_get_iomcu_power_status: input pointer NULL\n");
		return;
	}

	size_t len = sizeof(struct iomcu_power_status);

	mutex_lock(&g_mutex_pstatus);
	(void)memcpy_s(p, len, &g_i_power_status, len);
	mutex_unlock(&g_mutex_pstatus);
}

int get_iomcu_power_state(void)
{
	return g_iom3_power_state;
}
EXPORT_SYMBOL(get_iomcu_power_state); //lint !e546 !e580

static inline void clean_ipc_debug_info(void)
{
	(void)memset_s(&g_ipc_debug_info, sizeof(g_ipc_debug_info), 0,
		sizeof(g_ipc_debug_info));
}

static void print_ipc_debug_info(void)
{
	int i;

	for (i = TAG_BEGIN; i < TAG_END; ++i) {
		if (g_ipc_debug_info.event_cnt[i])
			ctxhub_info("event_cnt[%d]: %d\n", i,
				 g_ipc_debug_info.event_cnt[i]);
	}
	if (g_ipc_debug_info.pack_error_cnt)
		ctxhub_err("pack_err_cnt: %d\n", g_ipc_debug_info.pack_error_cnt);
}

int tell_ap_status_to_mcu(int ap_st)
{
	struct read_info pkg_mcu;
	struct write_info winfo;
	pkt_sys_statuschange_req_t pkt;

	if ((ap_st >= ST_BEGIN) && (ap_st < ST_END)) {
		(void)memset_s(&winfo, sizeof(struct write_info), 0, sizeof(struct write_info));
		(void)memset_s(&pkg_mcu, sizeof(struct read_info), 0, sizeof(struct read_info));
		(void)memset_s(&pkt, sizeof(pkt_sys_statuschange_req_t), 0, sizeof(pkt_sys_statuschange_req_t));
		winfo.tag = TAG_SYS;
		winfo.cmd = CMD_SYS_STATUSCHANGE_REQ;
		winfo.wr_len = sizeof(pkt) - sizeof(pkt.hd);
		pkt.status = ap_st;
		winfo.wr_buf = &pkt.status;

		if (likely((ap_st >= ST_SCREENON) && (ap_st <= ST_WAKEUP))) {
			ctxhub_info("------------>tell mcu ap in status %s\n",
				 g_sys_status_t_str[ap_st]);
			iom3_sr_status = ap_st;
		} else {
			ctxhub_info("------------>tell mcu ap in status %d\n",
				 ap_st);
		}
		return write_customize_cmd(&winfo,
					(ap_st == ST_SLEEP) ?
					(&pkg_mcu) : NULL,
					true);
	} else {
		ctxhub_err("error status %d in %s\n", ap_st, __func__);
		return -EINVAL;
	}
}

sys_status_t get_iom3_sr_status(void)
{
	return iom3_sr_status;
}

void update_current_app_status(u8 tag, u8 cmd)
{
	if (tag >= TAG_END)
		return;

	mutex_lock(&g_mutex_pstatus);
	if ((cmd == CMD_CMN_OPEN_REQ) || (cmd == CMD_CMN_INTERVAL_REQ))
		g_i_power_status.app_status[tag] = 1;
	else if (cmd == CMD_CMN_CLOSE_REQ)
		g_i_power_status.app_status[tag] = 0;
	mutex_unlock(&g_mutex_pstatus);
}

int sensorhub_pm_s4_entry(void)
{
	int ret = RET_SUCC;

	ctxhub_info("%s, iom3_sr_status:%u\n", __func__, iom3_sr_status);
	if (iom3_sr_status != ST_POWEROFF) {
		ret = tell_ap_status_to_mcu(ST_POWEROFF);
		if (ret != RET_SUCC)
			ctxhub_warn("tell_ap_status_to_mcu err:%d in %s\n", ret, __func__);
		g_iom3_power_state = ST_POWEROFF;
		clean_ipc_debug_info();
	}

	return ret;
}

static void check_current_app(void)
{
	int i = 0;
	int flag = 0;

	mutex_lock(&g_mutex_pstatus);
	for (i = 0; i < TAG_END; i++) {
		if (g_i_power_status.app_status[i])
			flag++;
	}
	if (flag > 0) {
		ctxhub_info("total %d app running after ap suspend\n", flag);
		g_i_power_status.power_status = SUB_POWER_ON;
	} else {
		ctxhub_info("iomcu will power off after ap suspend\n");
		g_i_power_status.power_status = SUB_POWER_OFF;
	}
	mutex_unlock(&g_mutex_pstatus);
}

static int sensorhub_pm_suspend(struct device *dev)
{
	int ret = RET_SUCC;

	ctxhub_info("%s+\n", __func__);
	if (iom3_sr_status != ST_SLEEP) {
		ret = tell_ap_status_to_mcu(ST_SLEEP);
		g_iom3_power_state = ST_SLEEP;
		check_current_app();
		clean_ipc_debug_info();
	}
	ctxhub_info("%s-\n", __func__);
	return ret;
}

static int wait_all_ready(void)
{
	if (!wait_for_completion_timeout(&g_iom3_resume_all,
					 msecs_to_jiffies(1000))) {
		ctxhub_err("RESUME :wait for ALL timeout\n");
		return RET_FAIL;
	} else if (g_resume_skip_flag != RESUME_SKIP) {
		ctxhub_info("RESUME mcu all ready!\n");
		device_set_cfg_data();
	} else {
		ctxhub_err("RESUME skip ALL\n");
	}
	return RET_SUCC;
}

static int sensorhub_pm_resume(struct device *dev)
{
	int ret;

	ctxhub_info("%s+\n", __func__);
	print_ipc_debug_info();
	g_resume_skip_flag = RESUME_INIT;

	reinit_completion(&g_iom3_reboot);
	reinit_completion(&g_iom3_resume_mini);
	reinit_completion(&g_iom3_resume_all);
	barrier();
	write_timestamp_base_to_sharemem();
	tell_ap_status_to_mcu(ST_WAKEUP);
	/* wait mini ready */
	if (!wait_for_completion_timeout(&g_iom3_resume_mini,
	                                 msecs_to_jiffies(SENSORHUB_PM_RESUME_TIMEOUT_MS))) {
		ctxhub_err("RESUME :wait for MINI timeout\n");
		goto resume_err;
	} else if (g_resume_skip_flag != RESUME_SKIP) {
		ret = send_fileid_to_mcu();
		if (ret) {
			ctxhub_err("RESUME get sensors cfg data from dts fail,ret=%d, use default config data!\n",
				ret);
			goto resume_err;
		} else {
			ctxhub_info("RESUME get sensors cfg data from dts success!\n");
		}
	} else {
		ctxhub_err("RESUME skip MINI\n");
	}

	/* wait all ready */
	ret = wait_all_ready();
	if (ret != RET_SUCC)
		goto resume_err;

	if (!wait_for_completion_timeout(&g_iom3_reboot,
	                                 msecs_to_jiffies(SENSORHUB_PM_RESUME_TIMEOUT_MS))) {
		ctxhub_err("resume :wait for response timeout\n");
		goto resume_err;
	}

	// not support print_err_info()
	goto done;
resume_err:
	/* always ret 0 */
	iom3_need_recovery(SENSORHUB_MODID, SH_FAULT_RESUME);
done:
	g_iom3_power_state = ST_WAKEUP;
	ctxhub_info("%s-\n", __func__);
	return RET_SUCC;
}

const static struct of_device_id sensorhub_io_supply_ids[] = {
	{.compatible = "huawei,sensorhub_io"},
	{}
};

MODULE_DEVICE_TABLE(of, sensorhub_io_supply_ids);

static void set_ldo12_vddio(struct platform_device *pdev)
{
	int ret;

	if (g_use_ldo12_flag == 1) {
		g_sensorhub_ldo12_vddio =
			regulator_get(&pdev->dev, SENSOR_VBUS_LDO12);
		if (IS_ERR(g_sensorhub_ldo12_vddio)) {
			ctxhub_err("%s: ldo12 regulator_get fail!\n", __func__);
		} else {
			ret = regulator_set_voltage(g_sensorhub_ldo12_vddio,
						    SENSOR_VOLTAGE_1V8,
						SENSOR_VOLTAGE_1V8);
			if (ret < 0)
				ctxhub_err("failed to set ldo12 sensorhub_vddio voltage to 1.8V\n");

			ret = regulator_enable(g_sensorhub_ldo12_vddio);
			if (ret < 0)
				ctxhub_err("failed to enable ldo12 regulator sensorhub_vddio\n");
		}
	}
}

static void get_power_node_info(struct device_node *power_node)
{
	u32 val;

	if (of_property_read_u32(power_node, "need-reset", &val)) {
		ctxhub_warn("%s failed to find property need-reset.\n",
			__func__);
	} else {
		need_reset_io_power = val;
		ctxhub_info("%s property need-reset is %u.\n",
			 __func__, val);
	}

	if (of_property_read_u32(power_node, "set-3v", &val)) {
		ctxhub_warn("%s failed to find property set-3v.\n",
			__func__);
	} else {
		need_set_3v_io_power = val;
		ctxhub_info("%s property set-3v is %u.\n", __func__, val);
	}

	if (of_property_read_u32(power_node, "set-3_1v", &val)) {
		ctxhub_warn("%s failed to find property set-3_1v.\n",
			__func__);
	} else {
		need_set_3_1v_io_power = val;
		ctxhub_info("%s property set-3_1v is %u.\n", __func__, val);
	}

	if (of_property_read_u32(power_node, "set-3_2v", &val)) {
		ctxhub_warn("%s failed to find property set-3_2v.\n",
			__func__);
	} else {
		need_set_3_2v_io_power = val;
		ctxhub_info("%s property set-3_2v is %u.\n", __func__, val);
	}
}

static void sensorhub_xsensor_poweron(int gpio)
{
	int ret;

	ret = gpio_request(gpio, "sensorhub_1v8");
	if (ret == 0) {
		ret = gpio_direction_output(gpio, 1);
		if (ret != 0)
			ctxhub_err("%s gpio_direction_output faild, gpio is %d.\n", __func__, gpio);
		else
			ctxhub_info("%s gpio_direction_output succ, gpio is %d.\n", __func__, gpio);
	} else {
		ctxhub_err("%s gpio_request faild, gpio is %d.\n", __func__, gpio);
	}
}

static void get_power_param(void)
{
	int tmp_gpio;
	u32 val;
	struct device_node *power_node = NULL;

	power_node = of_find_node_by_name(NULL, "sensorhub_io_power");
	if (!power_node) {
		ctxhub_err("%s failed to find dts node sensorhub_io_power\n",
			__func__);
	} else {
		get_power_node_info(power_node);

		if (of_property_read_u32(power_node, "vdd-set", &val)) {
			ctxhub_info("%s failed to find property vdd-set.\n",
				 __func__);
		} else {
			g_vdd_set_io_power = val;
			ctxhub_info("%s property vdd-set is %u.\n", __func__, val);
		}

		tmp_gpio = of_get_named_gpio(power_node, "sensorhub_gpio_1v8", 0);
		if (tmp_gpio < 0)
			ctxhub_info("%s sensorhub_gpio_1v8 not found.\n", __func__);
		else
			sensorhub_xsensor_poweron(tmp_gpio);
		tmp_gpio = of_get_named_gpio(power_node, "sensorhub_gpio_ag_1v8", 0);
		if (tmp_gpio < 0)
			ctxhub_info("%s sensorhub_gpio_ag_1v8 not found.\n", __func__);
		else
			sensorhub_xsensor_poweron(tmp_gpio);

		if (of_property_read_u32(power_node, "use_ldo12", &val)) {
			ctxhub_info("%s failed to find property use_ldo12.\n",
				 __func__);
		} else {
			g_use_ldo12_flag = val;
			ctxhub_info("%s property use_ldo12 is %u.\n",
				 __func__, val);
		}

		if (of_property_read_u32(power_node,
					 "no_need_sensor_ldo24", &val)) {
			ctxhub_info("%s failed find no_need_sensor_ldo24.\n",
				 __func__);
		} else {
			no_need_sensor_ldo24 = val;
			ctxhub_info("%s property no_need_ldo24 is %u.\n",
				 __func__, val);
		}
	}
}

static void set_power_value(void)
{
	int ret;

	if (need_set_3v_io_power) {
		ret = regulator_set_voltage(sensorhub_vddio,
					    SENSOR_VOLTAGE_3V,
					    SENSOR_VOLTAGE_3V);
		if (ret < 0)
			ctxhub_err("failed to set sensorhub_vddio voltage to 3V\n");
	} else if (need_set_3_1v_io_power) {
		ret = regulator_set_voltage(sensorhub_vddio,
					    SENSOR_VOLTAGE_3_1V,
					    SENSOR_VOLTAGE_3_1V);
		if (ret < 0)
			ctxhub_err("failed to set sensorhub_vddio voltage to 3_1V\n");
	} else if (need_set_3_2v_io_power) {
		ret = regulator_set_voltage(sensorhub_vddio,
					    SENSOR_VOLTAGE_3_2V,
					    SENSOR_VOLTAGE_3_2V);
		if (ret < 0)
			ctxhub_err("failed to set sensorhub_vddio voltage to 3_2V\n");
	} else {
		if (g_vdd_set_io_power) {
			ctxhub_info("%s, set io power %u\n", __func__,
				g_vdd_set_io_power);
			ret = regulator_set_voltage(sensorhub_vddio,
						    g_vdd_set_io_power,
						    g_vdd_set_io_power);
			if (ret < 0)
				ctxhub_err("failed to set sensorhub_vddio voltage\n");
		}
	}
}

static int sensorhub_io_driver_probe(struct platform_device *pdev)
{
	int ret;

	if (pdev == NULL)
		return -EINVAL;

	if (!of_match_device(sensorhub_io_supply_ids, &pdev->dev)) {
		ctxhub_err("[%s,%d]: match fail !\n",
			__func__, __LINE__);
		return -ENODEV;
	}
	g_sensorhub_wdt_irq = platform_get_irq(pdev, 0);
	if (g_sensorhub_wdt_irq < 0) {
		ctxhub_err("[%s] platform_get_irq err\n", __func__);
		return -ENXIO;
	}

	get_power_param();

	if (!no_need_sensor_ldo24) {
		sensorhub_vddio = regulator_get(&pdev->dev, SENSOR_VBUS);
		if (IS_ERR(sensorhub_vddio)) {
			ctxhub_err("%s: regulator_get fail!\n", __func__);
			return -EINVAL;
		}

		set_power_value();

		ret = regulator_enable(sensorhub_vddio);
		if (ret < 0)
			ctxhub_err("failed to enable regulator sensorhub_vddio\n");
	}

	set_ldo12_vddio(pdev);

	if (!no_need_sensor_ldo24) {
		if (need_reset_io_power && (g_sensorhub_reboot_reason_flag ==
			SENSOR_POWER_DO_RESET)) {
			ctxhub_info("%s : disable vddio\n", __func__);
			ret = regulator_disable(sensorhub_vddio);
			if (ret < 0)
				ctxhub_err("failed to disable regulator sensorhub_vddio\n");

			g_sensor_jiffies = jiffies;
		}
	}
	ctxhub_info("%s: success!\n", __func__);
	return RET_SUCC;
}

static int sensorhub_fb_notifier(struct notifier_block *nb,
				 unsigned long action, void *data)
{
	if (!data)
		return NOTIFY_OK;
	switch (action) {
	case FB_EVENT_BLANK:/* change finished */
		{
			struct fb_event *event = data;
			int *blank = event->data;

			if (!blank) {
				ctxhub_err("%s, blank is null, return\n",
					__func__);
				return NOTIFY_OK;
			}
			if (registered_fb[0] != event->info) {/* only main screen on/off info send to hub */
				ctxhub_err("%s, not main screen info, just return\n",
					__func__);
				return NOTIFY_OK;
			}
			switch (*blank) {
			case FB_BLANK_UNBLANK:/* screen on */
				tell_ap_status_to_mcu(ST_SCREENON);
				break;

			case FB_BLANK_POWERDOWN:/* screen off */
				tell_ap_status_to_mcu(ST_SCREENOFF);
				device_redetect_enter();
				break;

			default:
				ctxhub_err("unknown---> lcd unknown in %s\n",
					__func__);
				break;
			}
			break;
		}
	default:
		break;
	}

	return NOTIFY_OK;
}

static struct notifier_block fb_notify = {
	.notifier_call = sensorhub_fb_notifier,
};

static void read_reboot_reason_cmdline(void)
{
	char reboot_reason_buf[SENSOR_REBOOT_REASON_MAX_LEN] = { 0 };
	char *pstr = NULL;
	char *dstr = NULL;
	bool checklen = false;

	pstr = strstr(saved_command_line, "reboot_reason=");
	if (!pstr) {
		ctxhub_err("No fastboot reboot_reason info\n");
		return;
	}
	pstr += strlen("reboot_reason=");
	dstr = strstr(pstr, " ");
	if (!dstr) {
		ctxhub_err("No find the reboot_reason end\n");
		return;
	}
	checklen = SENSOR_REBOOT_REASON_MAX_LEN > (unsigned long)(dstr - pstr);
	if (!checklen) {
		ctxhub_err("overrun reboot_reason_buf\n");
		return;
	}

	if (memcpy_s(reboot_reason_buf, SENSOR_REBOOT_REASON_MAX_LEN, pstr,
		     (uint32_t)(uintptr_t)(dstr - pstr)) != EOK) {
		ctxhub_err("[%s] memcpy failed!\n", __func__);
		return;
	}
	reboot_reason_buf[(uint32_t)(uintptr_t)(dstr - pstr)] = '\0';

	if (!strcasecmp(reboot_reason_buf, "AP_S_COLDBOOT"))
		/* reboot flag */
		g_sensorhub_reboot_reason_flag = SENSOR_POWER_NO_RESET;
	else
		/* others */
		g_sensorhub_reboot_reason_flag = SENSOR_POWER_DO_RESET;
	ctxhub_info("sensorhub get reboot reason:%s length:%d flag:%d\n",
		 reboot_reason_buf,
		(int)strlen(reboot_reason_buf),
		g_sensorhub_reboot_reason_flag);
}

void set_pm_notifier(void)
{
	init_completion(&g_iom3_resume_mini);
	init_completion(&g_iom3_resume_all);
	init_completion(&g_iom3_reboot);
	fb_register_client(&fb_notify);
	read_reboot_reason_cmdline();
}

const static struct dev_pm_ops g_sensorhub_io_pm_ops = {
	.suspend = sensorhub_pm_suspend,
	.resume = sensorhub_pm_resume,
};

static struct platform_driver g_sensorhub_io_driver = {
	.probe = sensorhub_io_driver_probe,
	.driver = {
		.name = "Sensorhub_io_driver",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(sensorhub_io_supply_ids),
		.pm = &g_sensorhub_io_pm_ops,
	},
};

static int mcu_power_log_process(const struct pkt_header *head)
{
	ctxhub_info("%s in\n", __func__);
	if (head == NULL) {
		ctxhub_err("%s input head is null\n", __func__);
		return RET_FAIL;
	}

	mutex_lock(&g_mutex_pstatus);
	g_i_power_status.idle_time = ((pkt_power_log_report_req_t *)head)->idle_time;
	g_i_power_status.active_app_during_suspend =
		((pkt_power_log_report_req_t *)head)->current_app_mask;
	mutex_unlock(&g_mutex_pstatus);

	ctxhub_info("last suspend iomcu idle time is %d , active apps high is  0x%x, low is  0x%x\n",
		g_i_power_status.idle_time,
		(uint32_t)((g_i_power_status.active_app_during_suspend >> 32) & 0xffffffff),
		(uint32_t)(g_i_power_status.active_app_during_suspend & 0xffffffff));
	return 0;
}

static int iomcu_power_info_init(void)
{
	int ret;

	(void)memset_s(&g_i_power_status, sizeof(struct iomcu_power_status),
			0, sizeof(struct iomcu_power_status));
	ret = register_mcu_event_notifier(TAG_LOG, CMD_LOG_POWER_REQ,
		mcu_power_log_process);
	if (ret != 0) {
		ctxhub_err("[%s], register power log failed!\n", __func__);
		return ret;
	}
	return 0;
}

int sensorhub_io_driver_init(void)
{
	int ret;

	ctxhub_info("[%s] ++", __func__);
	ret = iomcu_power_info_init();
	if (ret != 0)
		return ret;

	set_pm_notifier();
	ret = platform_driver_register(&g_sensorhub_io_driver);
	if (ret) {
		ctxhub_err("%s: platform_device_register(sensorhub_io_driver) failed, ret:%d.\n",
			__func__, ret);
		return ret;
	}
	ctxhub_info("[%s] --", __func__);
	return RET_SUCC;
}

#ifdef SENSOR_1V8_POWER
static int hw_extern_pmic_query_state(int index, int *state)
{
	ctxhub_err("the camera power cfg donot define\n");
	return RET_FAIL;
}

static int check_sensor_1v8_from_pmic_poll(int sensor_1v8_flag,
					   int sensor_1v8_ldo)
{
	int state = 0;

	if (sensor_1v8_flag) {
		for (i = 0; i < POLL_TIMES; i++) {
			ret = hw_extern_pmic_query_state(sensor_1v8_ldo,
							 &state);
			if (state) {
				ctxhub_info("sensor_1V8 set high succ!\n");
				break;
			}
			msleep(SLEEP_TIME);
		}

		if (i == POLL_TIMES && state == 0) {
			ctxhub_err("sensor_1V8 set high fail, ret:%d!\n", ret);
			return RET_SUCC;
		}
	}
	return RET_FAIL;
}

static int check_sensor_1v8_from_pmic(void)
{
	int len = 0;
	int sensor_1v8_flag = 0;
	int sensor_1v8_ldo = 0;
	struct device_node *sensorhub_node = NULL;
	const char *sensor_1v8_from_pmic = NULL;
	int ret;

	sensorhub_node = of_find_compatible_node(NULL, NULL,
						 "huawei,sensorhub");
	if (!sensorhub_node) {
		ctxhub_err("%s, can't find node sensorhub\n", __func__);
		return RET_SUCC;
	}

	sensor_1v8_from_pmic = of_get_property(sensorhub_node,
					       "sensor_1v8_from_pmic", &len);
	if (!sensor_1v8_from_pmic) {
		ctxhub_info("%s, can't find property sensor_1v8_from_pmic\n",
			 __func__);
		return RET_FAIL;
	}

	sensor_power_pmic_flag = 1;
	if (strstr(sensor_1v8_from_pmic, "yes")) {
		ctxhub_info("%s, sensor_1v8 from pmic\n", __func__);
		if (of_property_read_u32(sensorhub_node,
					 "sensor_1v8_ldo", &sensor_1v8_ldo)) {
			ctxhub_err("%s, read sensor_1v8_ldo fail\n", __func__);
			return RET_SUCC;
		}
		ctxhub_info("%s, read sensor_1v8_ldo %d succ\n",
			__func__, sensor_1v8_ldo);
		sensor_1v8_flag = 1;
	} else {
		ctxhub_info("%s, sensor_1v8 not from pmic\n", __func__);
		return RET_FAIL;
	}

	ret = check_sensor_1v8_from_pmic_poll(sensor_1v8_flag, sensor_1v8_ldo);
	return ret;
}
#endif

static void sensor_ldo24_setup(void)
{
	int ret;
	unsigned int time_reset;
	unsigned long new_sensor_jiffies;

	if (no_need_sensor_ldo24) {
		ctxhub_info("no_need set ldo24\n");
		return;
	}

	if (need_reset_io_power &&
	    (g_sensorhub_reboot_reason_flag == SENSOR_POWER_DO_RESET)) {
		new_sensor_jiffies = jiffies - g_sensor_jiffies;
		time_reset = jiffies_to_msecs(new_sensor_jiffies);
		if (time_reset < SENSOR_MAX_RESET_TIME_MS)
			msleep(SENSOR_MAX_RESET_TIME_MS - time_reset);

		if (need_set_3v_io_power) {
			ret = regulator_set_voltage(sensorhub_vddio,
						    SENSOR_VOLTAGE_3V,
						SENSOR_VOLTAGE_3V);
			if (ret < 0)
				ctxhub_err("failed to set ldo24 to 3V\n");
		}

		if (need_set_3_1v_io_power) {
			ret = regulator_set_voltage(sensorhub_vddio,
						    SENSOR_VOLTAGE_3_1V,
						    SENSOR_VOLTAGE_3_1V);
			if (ret < 0)
				ctxhub_err("failed to set ldo24 to 3_1V\n");
		}

		if (need_set_3_2v_io_power) {
			ret = regulator_set_voltage(sensorhub_vddio,
						    SENSOR_VOLTAGE_3_2V,
						SENSOR_VOLTAGE_3_2V);
			if (ret < 0)
				ctxhub_err("failed to set ldo24 to 3_2V\n");
		}

		ctxhub_info("time_of_vddio_power_reset %u\n", time_reset);
		ret = regulator_enable(sensorhub_vddio);
		if (ret < 0)
			ctxhub_err("sensor vddio enable 2.85V\n");

		msleep(SENSOR_DETECT_AFTER_POWERON_TIME_MS);
	}
}

int sensor_power_check(void)
{
#ifdef SENSOR_1V8_POWER
	int ret;
	ret = check_sensor_1v8_from_pmic();
	if (ret != RET_SUCC)
		ctxhub_err("check sensor_1V8 from pmic fail\n");
#endif
	ctxhub_info("need_reset_io_power:%d reboot_reason_flag:%d\n",
		 need_reset_io_power, g_sensorhub_reboot_reason_flag);
	sensor_ldo24_setup();
	return RET_SUCC;
}

static bool is_mcu_wakeup(const struct pkt_header *head)
{
	if ((head->tag == TAG_SYS) && (head->cmd == CMD_SYS_STATUSCHANGE_REQ) &&
		(((pkt_sys_statuschange_req_t *)head)->status == ST_WAKEUP)) {
		return true;
	}
	return false;
}

static bool is_mcu_resume_mini(const struct pkt_header *head)
{
	if ((head->tag == TAG_SYS) && (head->cmd == CMD_SYS_STATUSCHANGE_REQ) &&
		((pkt_sys_statuschange_req_t *)head)->status == ST_MINSYSREADY &&
		atomic_read(&iom3_rec_state) != IOM3_RECOVERY_MINISYS) {
		return true;
	}
	return false;
}

static bool is_mcu_resume_all(const struct pkt_header *head)
{
	if ((head->tag == TAG_SYS) && (head->cmd == CMD_SYS_STATUSCHANGE_REQ) &&
		((pkt_sys_statuschange_req_t *)head)->status == ST_MCUREADY &&
		atomic_read(&iom3_rec_state) != IOM3_RECOVERY_MINISYS) {
		return true;
	}
	return false;
}

static int mcu_reboot_callback(const struct pkt_header *head)
{
	ctxhub_info("%s\n", __func__);
	complete(&g_iom3_reboot);
	return 0;
}

void inputhub_pm_callback(struct pkt_header *head)
{
	if (is_mcu_resume_mini(head)) {
		ctxhub_warn("%s MINI ready\n", __func__);
		g_resume_skip_flag = RESUME_MINI;
		barrier();
		complete(&g_iom3_resume_mini);
	}
	if (is_mcu_resume_all(head)) {
		ctxhub_warn("%s ALL ready\n", __func__);
		complete(&g_iom3_resume_all);
	}
	if (is_mcu_wakeup(head)) {
		if (g_resume_skip_flag != RESUME_MINI) {
			g_resume_skip_flag = RESUME_SKIP;
			barrier();
			complete(&g_iom3_resume_mini);
			complete(&g_iom3_resume_all);
		}
		mcu_reboot_callback(head);
	}
}
