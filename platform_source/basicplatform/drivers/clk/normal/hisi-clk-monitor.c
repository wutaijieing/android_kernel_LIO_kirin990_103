/*
 * hisi-clk-monitor.c
 *
 * CKM driver support
 *
 * Copyright (c) 2019-2019 Huawei Technologies Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */
#include <linux/version.h>
#include <linux/module.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/pm_runtime.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/mod_devicetable.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/kern_levels.h>
#include <securec.h>
#include "hisi-clk-smc.h"

#define SENSOR_CFG_NUM		2
#define CLK_LOW_THRESHOLD	0
#define CLK_HIGH_THRESHOLD	1
#define SENSOR_NAME_LENGTH	16
#define SENSOR_NO_USED		0
#define CKM_ALARM_NUM		3
#define CKM_ENABLE_FLAG		0x1ACCE551
#define CKM_DISABLE_FLAG	0
#define INTERVAL_TIME_500MS	500000000
#define CKM_IRQ_CONTINUE_TIMES	100

#define hickm_err(fmt, args ...) pr_err("[HICKM]"fmt, ##args)

#define hickm_info(fmt, args ...) pr_info("[HICKM]"fmt, ##args)

#define hickm_debug(fmt, args ...) pr_debug("[HICKM]"fmt, ##args)

enum {
	CKM_LOW_FREQ_ALARM,
	CKM_HIGH_FREQ_ALARM
};

struct hickm_sensor {
	u32 sensor_cfg[SENSOR_CFG_NUM];
	const char *sensor_name;
	u32 sensor_index;
	u32 is_used;
};

struct hickm_device {
	u32 sensor_num;
	u32 ckm_irq;
	u32 osc_en_cfg;
	u32 osc_cfg;
	struct work_struct irq_work;
	struct workqueue_struct *ckm_irq_wq;
	struct platform_device *pdev;
	struct hickm_sensor **sensor_list;
};

const char *ckm_alarm_analysis[CKM_ALARM_NUM] = {
	"low freq alarm",
	"high freq alarm",
	"freq alarm"
};

static unsigned int g_ckm_is_enable;
static u64 g_last_time;
static unsigned int g_irq_happened;
static struct hickm_device *g_test_hickm;

static DEFINE_SPINLOCK(g_ckm_cnt_lock);

void hisi_ckm_enable(struct hickm_device *hickm);
void hisi_ckm_disable(struct hickm_device *hickm);

static bool hisi_ckm_irq_cnt_update(void)
{
	unsigned long flags;
	u64 now_time;

	spin_lock_irqsave(&g_ckm_cnt_lock, flags);
	if (g_irq_happened > CKM_IRQ_CONTINUE_TIMES) {
		spin_unlock_irqrestore(&g_ckm_cnt_lock, flags);
		return false;
	}
	g_irq_happened++;

	now_time = ktime_get_ns();
	if (now_time > g_last_time) {
		if (now_time - g_last_time > INTERVAL_TIME_500MS)
			g_irq_happened = 0;

		g_last_time = now_time;
	} else {
		g_last_time = 0;
	}

	hickm_info("%s irq_happened: %u, last_time:%llu\n", __func__,
		g_irq_happened, g_last_time);

	spin_unlock_irqrestore(&g_ckm_cnt_lock, flags);
	return true;
}

static void hisi_ckm_work_handler(struct work_struct *work)
{
	struct hickm_device *hisi_ckm = container_of(work, struct hickm_device, irq_work);
	unsigned int sensor, alarm, index;

	sensor = atfd_hisi_service_clk_smc(CKM_QUERY_SENSOR, 0, 0, 0);
	hickm_info("%s sensor is %u\n", hisi_ckm->sensor_list[sensor]->sensor_name,
		sensor);
	if (sensor > hisi_ckm->sensor_num) {
		hickm_err("%s sensor index get err!\n", __func__);
		goto exit;
	}
	alarm = atfd_hisi_service_clk_smc(CKM_QUERY_ALARM, sensor, 0, 0);
	hickm_info("%s alarm is %u\n", hisi_ckm->sensor_list[sensor]->sensor_name,
		alarm);

	index = 0;
	while (index < CKM_ALARM_NUM) {
		if (alarm & BIT(0))
			hickm_err("[%s] %s occur!\n",
				hisi_ckm->sensor_list[sensor]->sensor_name,
				ckm_alarm_analysis[index]);
		alarm = alarm >> 1;
		index++;
	}
exit:
	(void)atfd_hisi_service_clk_smc(CKM_CLEAR_IRQ, sensor, 0, 0);

	hisi_ckm_disable(hisi_ckm);

	if (hisi_ckm_irq_cnt_update()) {
		hisi_ckm_enable(hisi_ckm);
		enable_irq(hisi_ckm->ckm_irq);
	}
}

static irqreturn_t hisi_ckm_interrupt(int irq, void *p)
{
	struct hickm_device *hisi_ckm = (struct hickm_device *)p;

	queue_work(hisi_ckm->ckm_irq_wq, &hisi_ckm->irq_work);

	disable_irq_nosync(hisi_ckm->ckm_irq);
	return IRQ_HANDLED;
}

void hisi_ckm_enable(struct hickm_device *hickm)
{
	int index;

	if (hickm == NULL) {
		hickm_err("%s input NULL\n", __func__);
		return;
	}

	if (g_ckm_is_enable != CKM_ENABLE_FLAG) {
		hickm_info("%s ckm is not enable\n", __func__);
		return;
	}

	/* ckm clk enable & unreset rely on system default state */
	(void)atfd_hisi_service_clk_smc(CKM_ENABLE, 0, hickm->osc_cfg,
		hickm->osc_en_cfg);

	for (index = 0; index < hickm->sensor_num; index++) {
		if (hickm->sensor_list[index] == NULL) {
			hickm_err("%s hickm-sensor-%d is NULL\n", __func__, index);
			return;
		}

		if (hickm->sensor_list[index]->is_used == SENSOR_NO_USED)
			continue;

		(void)atfd_hisi_service_clk_smc(CKM_SENSOR_CFG, index,
			hickm->sensor_list[index]->sensor_cfg[CLK_LOW_THRESHOLD],
			hickm->sensor_list[index]->sensor_cfg[CLK_HIGH_THRESHOLD]);

		(void)atfd_hisi_service_clk_smc(CKM_SENSOR_ENABLE, index, 0, 0);
	}
}

void hisi_ckm_disable(struct hickm_device *hickm)
{
	int index;

	if (hickm == NULL) {
		hickm_err("%s input NULL\n", __func__);
		return;
	}

	if (g_ckm_is_enable != CKM_ENABLE_FLAG) {
		hickm_info("%s ckm is not enable\n", __func__);
		return;
	}

	for (index = 0; index < hickm->sensor_num; index++)
		(void)atfd_hisi_service_clk_smc(CKM_SENSOR_DISABLE, index, 0, 0);

	(void)atfd_hisi_service_clk_smc(CKM_DISABLE, 0, 0, 0);

	/* ckm clk disable & reset rely on system default state */
}

int hisi_ckm_test_enable(unsigned int test_mode, unsigned int enable)
{
	hickm_err("%s test cmd is 0x%x 0x%x\n",
		__func__, test_mode, enable);

	if (test_mode == CKM_ENABLE_FLAG)
		g_ckm_is_enable = CKM_ENABLE_FLAG;
	else
		g_ckm_is_enable = CKM_DISABLE_FLAG;

	if (g_test_hickm != NULL) {
		if (enable)
			hisi_ckm_enable(g_test_hickm);
		else
			hisi_ckm_disable(g_test_hickm);
	}
	return 0;
}

static int hisi_ckm_get_sensors(struct device *dev, struct device_node *node,
	struct hickm_sensor **sensor_array, unsigned int sensor_num)
{
	struct device_node *son = NULL;
	struct hickm_sensor *sensor = NULL;
	const char *sensor_name = NULL;
	u32 i, j, is_used, sensor_index, ret;
	u32 sensor_cfg[SENSOR_CFG_NUM] = {0};
	u32 sensor_length = 0;

	for (i = 0; (son = of_get_next_child(node, son)); i++) {
		if (sensor_length >= sensor_num) {
			hickm_err("[%s] sensor length over boundary\n", __func__);
			return 0;
		}

		sensor = devm_kzalloc(dev, sizeof(struct hickm_sensor), GFP_KERNEL);
		if (!sensor) {
			ret = -ENOMEM;
			break;
		}

		ret = of_property_read_u32(son, "is_used", &is_used);
		if (ret) {
			hickm_err("sensor-%u has no tag <is_used>\n", i);
			break;
		}

		ret = of_property_read_u32(son, "index", &sensor_index);
		if (ret) {
			hickm_err("sensor-%u has no tag <index>\n", i);
			break;
		}

		ret = of_property_read_u32_array(son, "config", sensor_cfg,
			SENSOR_CFG_NUM);
		if (ret) {
			hickm_err("sensor-%u has no tag <config>\n", i);
			break;
		}

		ret = of_property_read_string(son, "sensor_name", &sensor_name);
		if (ret < 0) {
			hickm_err("sensor-%u has no tag <sensor_name>\n", i);
			break;
		}

		sensor->sensor_name = sensor_name;
		sensor->is_used = is_used;
		sensor->sensor_index = sensor_index;

		hickm_debug("used[%u], index[%u], name[%s]\n",
			sensor->is_used, sensor->sensor_index, sensor->sensor_name);
		for (j = 0; j < SENSOR_CFG_NUM; j++) {
			sensor->sensor_cfg[j] = sensor_cfg[j];
			hickm_debug("sensor_cfg[%u] is %u\n", j,
				sensor->sensor_cfg[j]);
		}

		sensor_array[sensor_length] = sensor;
		sensor_length++;
		sensor = NULL;
		sensor_name = NULL;
		is_used = 0;
		sensor_index = 0xFFFF;
		ret = memset_s(sensor_cfg, sizeof(sensor_cfg),
			0, SENSOR_CFG_NUM * sizeof(u32));
		if (ret != EOK)
			hickm_err("[%s] mmset failed\n", __func__);
	}
	return ret;
}

static int hisi_ckm_probe(struct platform_device *pdev)
{
	struct hickm_device *hisi_ckm = NULL;
	struct hickm_sensor **sensor_array = NULL;
	struct device *dev = &pdev->dev;
	struct device_node *node = pdev->dev.of_node;
	int ret;
	u32 sensor_num, osc_en_cfg, osc_cfg;

	if (node == NULL) {
		hickm_err("dts[%s] node not found\n", "hisilicon,clk_monitor");
		return -ENODEV;
	}

	hisi_ckm = devm_kzalloc(dev, sizeof(struct hickm_device), GFP_KERNEL);
	if (hisi_ckm == NULL) {
		hickm_err("no mem for hickm device\n");
		return -ENOMEM;
	}

	ret = of_property_read_u32(node, "sensor_num", (u32 *)&sensor_num);
	if (ret) {
		hickm_err("no sensor_num resources\n");
		return -ENODEV;
	}
	hisi_ckm->sensor_num = sensor_num;
	hickm_debug("sensor_num is [%u]\n", hisi_ckm->sensor_num);

	ret = of_property_read_u32(node, "osc_clk_eh_cfg", (u32 *)&osc_en_cfg);
	if (ret) {
		hickm_err("no osc_en_cfg resources\n");
		return -ENODEV;
	}
	hisi_ckm->osc_en_cfg = osc_en_cfg;
	hickm_debug("osc_en_cfg is [%u]\n", hisi_ckm->osc_en_cfg);

	ret = of_property_read_u32(node, "osc_clk_cfg", (u32 *)&osc_cfg);
	if (ret) {
		hickm_err("no osc_cfg resources\n");
		return -ENODEV;
	}
	hisi_ckm->osc_cfg = osc_cfg;
	hickm_debug("osc_cfg is [%u]\n", hisi_ckm->osc_cfg);

	hisi_ckm->ckm_irq = irq_of_parse_and_map(node, 0);
	hickm_debug("ckm_irq is [%u]\n", hisi_ckm->ckm_irq);

	sensor_array = devm_kzalloc(dev,
			sizeof(struct hickm_sensor) * sensor_num, GFP_KERNEL);
	if (!sensor_array) {
		hickm_err("no mem for hickm sensors\n");
		return -ENOMEM;
	}

	ret = hisi_ckm_get_sensors(dev, node, sensor_array, sensor_num);
	if (ret) {
		hickm_err("can not get ckm resource, ret = %u\n", ret);
		return -ENODEV;
	}

	hisi_ckm->sensor_list = sensor_array;

	platform_set_drvdata(pdev, hisi_ckm);

	ret = request_irq(hisi_ckm->ckm_irq, hisi_ckm_interrupt, 0,
		"hisi_ckm", (void *)hisi_ckm);
	if (ret) {
		hickm_err("request ckm irq failed, ret = %u\n", ret);
		return ret;
	}

	hisi_ckm->ckm_irq_wq = create_singlethread_workqueue("hisi_clk_monitor");
	if (hisi_ckm->ckm_irq_wq == NULL) {
		hickm_err("workqueue create failed!\n");
		return -ENODEV;
	}

	INIT_WORK(&hisi_ckm->irq_work, hisi_ckm_work_handler);

	hisi_ckm_enable(hisi_ckm);

	g_test_hickm = hisi_ckm;

	hickm_info("[%s]hickm module init ok!\n", __func__);
	return ret;
}

static int hisi_ckm_remove(struct platform_device *pdev)
{
	hickm_debug("%s ++\n", __func__);
	platform_set_drvdata(pdev, NULL);
	hickm_debug("%s --\n", __func__);
	return 0;
}

static int hisi_ckm_suspend(struct device *dev)
{
	struct platform_device *pdev = container_of(dev, struct platform_device, dev);
	struct hickm_device *hickm = platform_get_drvdata(pdev);

	if (g_ckm_is_enable != CKM_ENABLE_FLAG)
		return 0;

	hickm_info("%s: suspend +", __func__);
	if (hickm != NULL)
		hisi_ckm_disable(hickm);
	hickm_info("%s: suspend -", __func__);
	return 0;
}

static int hisi_ckm_resume(struct device *dev)
{
	struct platform_device *pdev = container_of(dev, struct platform_device, dev);
	struct hickm_device *hickm = platform_get_drvdata(pdev);

	if (g_ckm_is_enable != CKM_ENABLE_FLAG)
		return 0;

	hickm_info("%s: resume +", __func__);
	if (hickm != NULL)
		hisi_ckm_enable(hickm);
	hickm_info("%s: resume -", __func__);
	return 0;
}

static const struct of_device_id hisi_ckm_of_match[] = {
	{ .compatible = "hisilicon,clk_monitor", },
	{},
};

MODULE_DEVICE_TABLE(of, hisi_ckm_of_match);

#ifdef CONFIG_PM
static const struct dev_pm_ops hisi_ckm_pm_ops = {
	.suspend = hisi_ckm_suspend,
	.resume = hisi_ckm_resume,
};
#endif

static struct platform_driver hisi_ckm_driver = {
	.probe = hisi_ckm_probe,
	.remove = hisi_ckm_remove,
	.driver = {
			   .name = "clk-monitor",
			   .owner = THIS_MODULE,
			   .of_match_table = of_match_ptr(hisi_ckm_of_match),
			   .pm = &hisi_ckm_pm_ops,
		},
};

module_platform_driver(hisi_ckm_driver);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("CKM driver support");
MODULE_AUTHOR("Xiaodong Guo <guoxiaodong2@huawei.com>");
