/*
 * slimbus is a kernel driver which is used to manager slimbus devices
 *
 * Copyright (c) 2020 Huawei Technologies Co., Ltd.
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

#include "slimbus_pm.h"

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/io.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/regulator/consumer.h>
#include <linux/of.h>
#include <linux/slab.h>
#include <linux/pm_runtime.h>
#include <linux/pinctrl/consumer.h>
#include <rdr_audio_adapter.h>
#ifdef CONFIG_HUAWEI_DSM
#include <dsm_audio/dsm_audio.h>
#endif

#include "audio_log.h"
#include "slimbus.h"
#include "slimbus_types.h"
#include "slimbus_utils.h"
#include "slimbus_drv.h"
#include "slimbus_da_combine.h"

#define LOG_TAG "slimbus_pm"

#ifdef CONFIG_PM_SLEEP

static int32_t suspend_resource(struct device *device, struct slimbus_private_data *pd)
{
	int32_t ret = 0;

	/* make sure last msg has been processed finished */
	mdelay(1);
	slimbus_int_need_clear_set(true);
	/*
	 * while fm, da_combine pll is in high freq, slimbus framer is in codec side
	 * we need to switch to soc in this case, and switch to da_combine in resume
	 */
	if (pd->framerstate == FRAMER_CODEC) {
		AUDIO_LOGI("switch framer to soc");
		ret = slimbus_switch_framer(FRAMER_SOC);
		if (ret) {
			AUDIO_LOGE("slimbus switch framer failed");
			return ret;
		}
		pd->lastframer = FRAMER_CODEC;
	} else {
		pd->lastframer = FRAMER_SOC;
	}

	ret = slimbus_pause_clock(SLIMBUS_RT_UNSPECIFIED_DELAY);
	if (ret)
		AUDIO_LOGE("slimbus pause clock failed, ret: %x", ret);

	/* make sure para has updated */
	mdelay(1);

	ret = slimbus_drv_stop();
	if (ret)
		AUDIO_LOGE("slimbus stop failed");

	return ret;
}

static int32_t slimbus_suspend(struct device *device)
{
	struct platform_device *pdev = to_platform_device(device);
	struct slimbus_private_data *pd = platform_get_drvdata(pdev);
	int32_t ret;

	if (!pd) {
		AUDIO_LOGE("pd is null");
		return -EINVAL;
	}

	if (is_pm_runtime_support()) {
		ret = pm_runtime_get_sync(device);
		if (ret < 0) {
			AUDIO_LOGE("pm resume error, pm_ret: %d", ret);
#ifdef CONFIG_DFX_BB
			rdr_system_error(RDR_AUDIO_RUNTIME_SYNC_FAIL_MODID, 0, 0);
#endif
			return ret;
		}
	}

	if (is_bus_active())
		return 0;

	ret = suspend_resource(device, pd);

	return ret;
}

static int32_t resume_resource(struct device *device,
	struct slimbus_private_data *pd)
{
	struct slimbus_device_info *dev_info = NULL;
	struct slimbus_bus_config *bus_cfg = NULL;
	int32_t ret = 0;

	slimbus_int_need_clear_set(false);

	dev_info = slimbus_get_devices_info();
	if (dev_info == NULL) {
		AUDIO_LOGE("device is NULL");
		goto exit;
	}
	slimbus_utils_module_enable(dev_info, true);
	ret = slimbus_drv_resume_clock();
	if (ret)
		AUDIO_LOGE("slimbus resume clock failed, ret: %d", ret);

	ret = slimbus_dev_init(pd->type);
	if (ret) {
		AUDIO_LOGE("slimbus drv init failed");
		goto exit;
	}

	bus_cfg = slimbus_get_bus_config(SLIMBUS_BUS_CONFIG_NORMAL);
	if (bus_cfg == NULL) {
		AUDIO_LOGE("bus cfg is NULL");
		goto exit;
	}

	ret = slimbus_drv_bus_configure(bus_cfg);
	if (ret) {
		AUDIO_LOGE("slimbus bus configure failed");
		goto exit;
	}

	if (pd->lastframer == FRAMER_CODEC) {
		ret = slimbus_switch_framer(FRAMER_CODEC);
		AUDIO_LOGI("switch_framer:%#x +", pd->lastframer);
	}

exit:
	return ret;
}

static int32_t slimbus_resume(struct device *device)
{
	struct platform_device *pdev = to_platform_device(device);
	struct slimbus_private_data *pd = platform_get_drvdata(pdev);
	int32_t ret = 0;

	if (!pd) {
		AUDIO_LOGE("pd is null");
		return -EINVAL;
	}

	if (is_bus_active())
		goto exit;

	ret = resume_resource(device, pd);
exit:
	if (is_pm_runtime_support()) {
		pm_runtime_mark_last_busy(pd->dev);
		pm_runtime_put_autosuspend(pd->dev);
	}

	return ret;
}
#endif

#ifdef CONFIG_HIBERNATION
static void slimbus_get_port_state_info(struct device *device)
{
	struct platform_device *pdev = to_platform_device(device);
	struct slimbus_private_data *pd = platform_get_drvdata(pdev);

	pd->portstate = slimbus_utils_port_state_get(pd->slimbus_mem);
	AUDIO_LOGI("slimbus freeze portstate 0x%x", pd->portstate);
}

static int32_t slimbus_freeze(struct device *device)
{
	struct platform_device *pdev = to_platform_device(device);
	struct slimbus_private_data *pd = platform_get_drvdata(pdev);

	if (!pd) {
		AUDIO_LOGE("pd is null");
		return -EINVAL;
	}

	AUDIO_LOGI("slimbus freeze start");
	pm_runtime_get_sync(pd->dev);
	slimbus_get_port_state_info(device);
	pm_runtime_mark_last_busy(pd->dev);
	pm_runtime_put_autosuspend(pd->dev);
	AUDIO_LOGI("slimbus freeze end");

	return 0;
}

static int32_t slimbus_thaw(struct device *device)
{
	AUDIO_LOGI("slimbus thaw start");
	slimbus_get_port_state_info(device);
	AUDIO_LOGI("slimbus thaw end");

	return 0;
}

static int32_t slimbus_restore(struct device *device)
{
	uint32_t i;
	struct platform_device *pdev = to_platform_device(device);
	struct slimbus_private_data *pd = platform_get_drvdata(pdev);

	/* clean all trackstate,then active will effect */
	for (i = 0; i < pd->slimbus_track_max; i++)
		slimbus_trackstate_set(i, false);

	AUDIO_LOGI("slimbus restore");
	return 0;
}

static int32_t slimbus_poweroff(struct device *device)
{
	AUDIO_LOGI("slimbus poweroff");
	return 0;
}
#endif

#ifdef CONFIG_PM
static int32_t slimbus_runtime_suspend(struct device *device)
{
	struct platform_device *pdev = to_platform_device(device);
	struct slimbus_private_data *pd = platform_get_drvdata(pdev);
	int32_t ret = 0;

	if (!pd) {
		AUDIO_LOGE("pd is null");
		return -EINVAL;
	}

	pd->portstate = slimbus_utils_port_state_get(pd->slimbus_mem);

	AUDIO_LOGI("portstate: 0x%x", pd->portstate);

	if (pd->portstate != 0)
		AUDIO_LOGE("portstate is nozero");

	/* make sure last msg has been processed finished */
	mdelay(1);
	slimbus_int_need_clear_set(true);

	ret = slimbus_pause_clock(SLIMBUS_RT_UNSPECIFIED_DELAY);
	if (ret)
		AUDIO_LOGE("slimbus pause clock failed, ret: %#x", ret);

	/* make sure para has updated */
	mdelay(1);

	return ret;
}

static int32_t slimbus_runtime_resume(struct device *device)
{
	struct platform_device *pdev = to_platform_device(device);
	struct slimbus_private_data *pd = platform_get_drvdata(pdev);
	int32_t ret = 0;

	if (!pd) {
		AUDIO_LOGE("pd is null");
		return -EINVAL;
	}

	AUDIO_LOGI("portstate: 0x%x", pd->portstate);

	if (pd->portstate != 0)
		AUDIO_LOGE("portstate is nozero");

	slimbus_int_need_clear_set(false);

	ret = slimbus_drv_resume_clock();
	if (ret)
		AUDIO_LOGE("slimbus resume clock failed, ret: %d", ret);

	return ret;
}
#endif

const struct dev_pm_ops slimbus_pm_ops = {
#ifdef CONFIG_HIBERNATION
		.suspend = slimbus_suspend,
		.resume = slimbus_resume,
		.freeze = slimbus_freeze,
		.thaw = slimbus_thaw,
		.poweroff = slimbus_poweroff,
		.restore = slimbus_restore,
		.runtime_suspend = slimbus_runtime_suspend,
		.runtime_resume = slimbus_runtime_resume
#else
	SET_SYSTEM_SLEEP_PM_OPS(slimbus_suspend, slimbus_resume)
	SET_RUNTIME_PM_OPS(slimbus_runtime_suspend, slimbus_runtime_resume, NULL)
#endif
};

const struct dev_pm_ops *slimbus_pm_get_ops(void)
{
	return &slimbus_pm_ops;
}

