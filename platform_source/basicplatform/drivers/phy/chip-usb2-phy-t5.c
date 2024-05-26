/*
 * usb2-phy-t5.c
 * Copyright (c) 2020-2020 Huawei Technologies Co., Ltd.
 * Create:2019-09-24
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

#include <linux/atomic.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of_address.h>
#include <linux/phy/phy.h>
#include <linux/platform_device.h>
#include <linux/regmap.h>
#include <linux/workqueue.h>
#include <linux/version.h>
#include <linux/platform_drivers/usb/chip_usb.h>
#include <linux/platform_drivers/usb/chip_usb_phy.h>
#include <linux/platform_drivers/usb/usb_misc_ctrl.h>
#include <linux/platform_drivers/usb/chip_usb_helper.h>
#include <linux/platform_drivers/usb/usb_reg_cfg.h>

#define USB2_T5_INFO(format, arg...) \
	pr_info("[USB2_T5][I][%s]"format, __func__, ##arg)

#define USB2_T5_ERR(format, arg...) \
	pr_err("[USB2_T5][E][%s]"format, __func__, ##arg)

#define chip_usb2_phy_to_phy_priv(pointer) \
	container_of(pointer, struct phy_priv, usb_phy)

struct bc_detect_cfg {
	struct chip_usb_reg_cfg **bc12_detect_dcd;
	int bc12_detect_dcd_num;
	struct chip_usb_reg_cfg **bc12_check_fsvplus;
	int bc12_check_fsvplus_num;
	struct chip_usb_reg_cfg **bc12_detect_dcd_end;
	int bc12_detect_dcd_end_num;
	struct chip_usb_reg_cfg **bc12_primary_detect;
	int bc12_primary_detect_num;
	struct chip_usb_reg_cfg **bc12_check_chrdet;
	int bc12_check_chrdet_num;
	struct chip_usb_reg_cfg **bc12_detect_sdp_stop;
	int bc12_detect_sdp_stop_num;
	struct chip_usb_reg_cfg **bc12_secondary_detect;
	int bc12_secondary_detect_num;
	struct chip_usb_reg_cfg **bc12_secondary_detect_done;
	int bc12_secondary_detect_done_num;
	struct chip_usb_reg_cfg **bc12_detect_cdp_done;
	int bc12_detect_cdp_done_num;
	struct chip_usb_reg_cfg **bc12_detect_dcd_fail;
	int bc12_detect_dcd_fail_num;
	struct chip_usb_reg_cfg **bc12_primary_detect_fail;
	int bc12_primary_detect_fail_num;
	struct chip_usb_reg_cfg **bc12_detect_close;
	int bc12_detect_close_num;
};

struct phy_priv {
	struct device *dev;
	struct chip_usb2_phy usb_phy;

	enum phy_mode mode;
	bool disable_bc;
	bool is_fpga;
	struct clk **clks;
	int num_clks;
	struct chip_usb_reg_cfg **reset;
	int reset_num;
	struct chip_usb_reg_cfg **unreset;
	int unreset_num;
	struct chip_usb_reg_cfg **vbus_valid;
	int vbus_valid_num;
	struct chip_usb_reg_cfg **vbus_invalid;
	int vbus_invalid_num;
	bool set_eye_param_enable;
	struct chip_usb_reg_cfg **host_eye_param;
	int host_eye_param_num;
	struct chip_usb_reg_cfg **device_eye_param;
	int device_eye_param_num;
	bool bc12_detect_enable;
	struct bc_detect_cfg bc_detect_cfg;

	struct regulator *usb20phy_power;

	atomic_t desired_vdp_src;
};

static int usb2_phy_open_clk(struct phy_priv *priv)
{
	int ret;

	USB2_T5_INFO("+\n");

	if (priv->is_fpga)
		return 0;

	ret = chip_usb_init_clks(priv->clks, priv->num_clks);
	if (ret)
		USB2_T5_ERR("open clk failed %d\n", ret);

	USB2_T5_INFO("-\n");
	return ret;
}

static void set_usb2_eye_diagram_param(struct phy_priv *priv)
{
	struct chip_usb_reg_cfg **reg_cfgs = NULL;
	struct chip_usb_reg_cfg *eye1_cfgs = NULL;
	struct chip_usb_reg_cfg *eye2_cfgs = NULL;
	int ret = 0;

	if (priv->is_fpga)
		return;

	if (!(priv->set_eye_param_enable)) {
		USB2_T5_ERR("usb eye param not ready!!!");
		return;
	}
	/* set high speed phy parameter */
	if (priv->mode == PHY_MODE_USB_HOST) {
		chip_usb_reg_write_array(priv->host_eye_param, priv->host_eye_param_num);
		reg_cfgs = priv->host_eye_param;
		ret = chip_usb_reg_test_cfg(*(priv->host_eye_param));
	} else if (priv->mode == PHY_MODE_USB_DEVICE) {
		chip_usb_reg_write_array(priv->device_eye_param, priv->device_eye_param_num);
		reg_cfgs = priv->device_eye_param;
		ret = chip_usb_reg_test_cfg(*(priv->device_eye_param));
	}

	if (reg_cfgs && ret) {
		eye1_cfgs = reg_cfgs[0];
		eye2_cfgs = reg_cfgs[1];
		USB2_T5_INFO("set hs phy param 0x%x, 0x%x for device\n",
				eye1_cfgs->value,
				eye2_cfgs->value);
	} else {
		USB2_T5_ERR("usb parameters set fails!");
	}

	return;
}

static int _usb2_phy_init(struct phy_priv *priv)
{
	int ret;

	USB2_T5_INFO("+\n");

	if (priv->usb20phy_power) {
		ret = regulator_enable(priv->usb20phy_power);
		if (ret) {
			USB2_T5_ERR("regulator enable failed %d!\n", ret);
			return ret;
		}
	}

	ret = misc_ctrl_init();
	if (ret)
		return ret;

	ret = usb2_phy_open_clk(priv);
	if (ret)
		goto exit_misc_ctrl;

	ret = chip_usb_reg_write_array(priv->unreset, priv->unreset_num);
	if (ret) {
		USB2_T5_ERR("phy unreset failed %d\n", ret);
		goto exit_misc_ctrl;
	}

	ret = chip_usb_reg_write_array(priv->vbus_valid, priv->vbus_valid_num);
	if (ret) {
		USB2_T5_ERR("set vbus valid failed %d\n", ret);
		goto exit_misc_ctrl;
	}

	USB2_T5_INFO("set phy param in %s mode\n",
			(priv->mode == PHY_MODE_USB_HOST) ? "host" : "device");
	/* set eye param */
	set_usb2_eye_diagram_param(priv);

	USB2_T5_INFO("-\n");
	return ret;

exit_misc_ctrl:
	misc_ctrl_exit();
	if (priv->usb20phy_power)
		if (regulator_disable(priv->usb20phy_power))
			USB2_T5_ERR("regulator disable failed!\n");

	USB2_T5_ERR("failed to init phy ret: %d\n", ret);
	return ret;
}

static int usb2_phy_init(struct phy *phy)
{
	struct chip_usb2_phy *usb2_phy = NULL;
	struct phy_priv *priv = NULL;

	if (!phy)
		return -ENODEV;

	usb2_phy = phy_get_drvdata(phy);
	if (!usb2_phy)
		return -ENODEV;

	priv = chip_usb2_phy_to_phy_priv(usb2_phy);

	return _usb2_phy_init(priv);
}

static int usb2_phy_close_clk(struct phy_priv *priv)
{
	USB2_T5_INFO("+\n");

	if (priv->is_fpga)
		return 0;

	chip_usb_shutdown_clks(priv->clks, priv->num_clks);

	USB2_T5_INFO("-\n");
	return 0;
}

static int _usb2_phy_exit(struct phy_priv *priv)
{
	int ret;

	USB2_T5_INFO("+\n");

	ret = chip_usb_reg_write_array(priv->vbus_invalid,
			priv->vbus_invalid_num);
	if (ret)
		USB2_T5_ERR("set vbus invalid failed %d\n", ret);

	ret = chip_usb_reg_write_array(priv->reset,
			priv->reset_num);
	if (ret)
		USB2_T5_ERR("phy reset failed %d\n", ret);

	ret = usb2_phy_close_clk(priv);
	if (ret)
		USB2_T5_ERR("close clk failed %d\n", ret);

	misc_ctrl_exit();

	if (priv->usb20phy_power) {
		ret = regulator_disable(priv->usb20phy_power);
		if (ret)
			USB2_T5_ERR("regulator disable failed %d!\n", ret);
	}

	USB2_T5_INFO("-\n");
	return 0;
}

static int usb2_phy_exit(struct phy *phy)
{
	struct chip_usb2_phy *usb2_phy = NULL;
	struct phy_priv *priv = NULL;

	if (!phy)
		return -ENODEV;

	usb2_phy = phy_get_drvdata(phy);
	if (!usb2_phy)
		return -ENODEV;

	priv = chip_usb2_phy_to_phy_priv(usb2_phy);

	return _usb2_phy_exit(priv);
}

static int usb2_phy_poweron(struct phy *phy)
{
	USB2_T5_INFO("+\n");

	USB2_T5_INFO("power on\n");

	USB2_T5_INFO("-\n");
	return 0;
}

static int usb2_phy_poweroff(struct phy *phy)
{
	USB2_T5_INFO("+\n");

	USB2_T5_INFO("power off\n");

	USB2_T5_INFO("-\n");
	return 0;
}

static int usb2_phy_set_mode(struct phy *phy, enum phy_mode mode)
{
	struct chip_usb2_phy *usb2_phy = NULL;
	struct phy_priv *priv = NULL;

	if (!phy)
		return -ENODEV;

	usb2_phy = (struct chip_usb2_phy *)phy_get_drvdata(phy);
	if (!usb2_phy)
		return -ENODEV;

	priv = chip_usb2_phy_to_phy_priv(usb2_phy);

	priv->mode = mode;

	return 0;
}

static int usb2_phy_set_mode_ext(struct phy *phy, enum phy_mode mode,
				 int submode)
{
	return usb2_phy_set_mode(phy, mode);
}

static struct phy_ops usb2_phy_ops = {
	.init		= usb2_phy_init,
	.exit		= usb2_phy_exit,
	.power_on	= usb2_phy_poweron,
	.power_off	= usb2_phy_poweroff,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,10,0)
	.set_mode	= usb2_phy_set_mode_ext,
#else
	.set_mode	= usb2_phy_set_mode,
#endif
	.owner		= THIS_MODULE,
};

static enum chip_charger_type bc12_detect_charger_type(struct phy_priv *priv)
{
	enum chip_charger_type charger_type = CHARGER_TYPE_UNKNOWN;
	int ret;

	chip_usb_reg_write_array(priv->bc_detect_cfg.bc12_detect_dcd,
						priv->bc_detect_cfg.bc12_detect_dcd_num);
	mdelay(150);

	ret = chip_usb_reg_test_cfg(*(priv->bc_detect_cfg.bc12_check_fsvplus));
	if (ret <= 0) {
		USB2_T5_ERR("bc12_check_fsvplus fail\n");
		chip_usb_reg_write_array(priv->bc_detect_cfg.bc12_detect_dcd_fail,
							priv->bc_detect_cfg.bc12_detect_dcd_fail_num);
		goto close_bc12_detect;
	}

	chip_usb_reg_write_array(priv->bc_detect_cfg.bc12_detect_dcd_end,
						priv->bc_detect_cfg.bc12_detect_dcd_end_num);
	mdelay(2);
	chip_usb_reg_write_array(priv->bc_detect_cfg.bc12_primary_detect,
						priv->bc_detect_cfg.bc12_primary_detect_num);
	mdelay(30);

	ret = chip_usb_reg_test_cfg(*(priv->bc_detect_cfg.bc12_check_chrdet));
	if (ret <= 0) {
		USB2_T5_ERR("charger type = SDP\n");
		chip_usb_reg_write_array(priv->bc_detect_cfg.bc12_primary_detect_fail,
							priv->bc_detect_cfg.bc12_primary_detect_fail_num);
		charger_type = CHARGER_TYPE_SDP;
		goto close_bc12_detect;
	}

	chip_usb_reg_write_array(priv->bc_detect_cfg.bc12_detect_sdp_stop,
						priv->bc_detect_cfg.bc12_detect_sdp_stop_num);
	mdelay(2);
	chip_usb_reg_write_array(priv->bc_detect_cfg.bc12_secondary_detect,
						priv->bc_detect_cfg.bc12_secondary_detect_num);
	mdelay(30);

	ret = chip_usb_reg_test_cfg(*(priv->bc_detect_cfg.bc12_check_chrdet));
	 if (ret <= 0) {
		USB2_T5_ERR("charger type = CDP\n");
		chip_usb_reg_write_array(priv->bc_detect_cfg.bc12_secondary_detect_done,
							priv->bc_detect_cfg.bc12_secondary_detect_done_num);
		chip_usb_reg_write_array(priv->bc_detect_cfg.bc12_detect_cdp_done,
							priv->bc_detect_cfg.bc12_detect_cdp_done_num);
		charger_type = CHARGER_TYPE_CDP;
	} else {
		USB2_T5_ERR("charger type = DCP\n");
		chip_usb_reg_write_array(priv->bc_detect_cfg.bc12_secondary_detect_done,
							priv->bc_detect_cfg.bc12_secondary_detect_done_num);
		charger_type = CHARGER_TYPE_DCP;
	}

close_bc12_detect:
	chip_usb_reg_write_array(priv->bc_detect_cfg.bc12_detect_close,
						priv->bc_detect_cfg.bc12_detect_close_num);
	return charger_type;
}

static enum chip_charger_type detect_charger_type(
		struct chip_usb2_phy *usb2_phy)
{
	struct phy_priv *priv = NULL;
	struct phy *phy = NULL;
	enum chip_charger_type charger_type = CHARGER_TYPE_SDP;

	if (!usb2_phy)
		return CHARGER_TYPE_NONE;

	priv = chip_usb2_phy_to_phy_priv(usb2_phy);

	if (priv->disable_bc)
		return CHARGER_TYPE_SDP;

	phy = priv->usb_phy.phy;

	mutex_lock(&phy->mutex);
	if (phy->init_count == 0) {
		mutex_unlock(&phy->mutex);
		USB2_T5_ERR("phy has not been initialized\n");
		return CHARGER_TYPE_NONE;
	}

	if (priv->bc12_detect_enable)
		charger_type = bc12_detect_charger_type(priv);

	mutex_unlock(&phy->mutex);

	return charger_type;
}

static int set_dpdm_pulldown(struct chip_usb2_phy *usb2_phy, bool pulldown)
{
	struct phy_priv *priv = NULL;
	struct phy *phy = NULL;

	if (!usb2_phy)
		return -EINVAL;

	priv = chip_usb2_phy_to_phy_priv(usb2_phy);
	phy = priv->usb_phy.phy;

	if (priv->is_fpga)
		return 0;

	mutex_lock(&phy->mutex);
	if (phy->init_count == 0) {
		mutex_unlock(&phy->mutex);
		USB2_T5_ERR("phy has not been initialized\n");
		return -ENODEV;
	}

	mutex_unlock(&phy->mutex);

	return 0;
}

static int _set_vdp_src(struct phy_priv *priv, bool enable)
{
	struct phy *phy = priv->usb_phy.phy;

	mutex_lock(&phy->mutex);
	if (phy->init_count == 0) {
		mutex_unlock(&phy->mutex);
		USB2_T5_ERR("phy has not been initialized\n");
		return -ENODEV;
	}

	mutex_unlock(&phy->mutex);

	return 0;
}

static int set_vdp_src(struct chip_usb2_phy *usb2_phy, bool enable)
{
	struct phy_priv *priv = NULL;

	if (!usb2_phy)
		return -EINVAL;

	priv = chip_usb2_phy_to_phy_priv(usb2_phy);

	if (priv->is_fpga)
		return 0;

	if (in_atomic() || irqs_disabled()) {
		if (enable)
			atomic_set(&priv->desired_vdp_src, 1);
		else
			atomic_set(&priv->desired_vdp_src, 0);
		/* need to update regster */
		return 0;
	} else {
		return _set_vdp_src(priv, enable);
	}
}

static int get_bc12_ops_resource(struct phy_priv *priv)
{
	struct device *dev = priv->dev;
	int ret;

	ret = get_chip_usb_reg_cfg_array(dev, "bc12-detect-dcd",
			&(priv->bc_detect_cfg.bc12_detect_dcd), &(priv->bc_detect_cfg.bc12_detect_dcd_num));
	if (ret)
		goto exit_bc12_detect_dcd;

	ret = get_chip_usb_reg_cfg_array(dev, "bc12-check-fsvplus",
			&(priv->bc_detect_cfg.bc12_check_fsvplus), &(priv->bc_detect_cfg.bc12_check_fsvplus_num));
	if (ret)
		goto exit_bc12_check_fsvplus;

	ret = get_chip_usb_reg_cfg_array(dev, "bc12-detect-dcd-end",
			&(priv->bc_detect_cfg.bc12_detect_dcd_end), &(priv->bc_detect_cfg.bc12_detect_dcd_end_num));
	if (ret)
		goto exit_bc12_detect_dcd_end;

	ret = get_chip_usb_reg_cfg_array(dev, "bc12-primary-detect",
			&(priv->bc_detect_cfg.bc12_primary_detect), &(priv->bc_detect_cfg.bc12_primary_detect_num));
	if (ret)
		goto exit_bc12_primary_detect;

	ret = get_chip_usb_reg_cfg_array(dev, "bc12-check-chrdet",
			&(priv->bc_detect_cfg.bc12_check_chrdet), &(priv->bc_detect_cfg.bc12_check_chrdet_num));
	if (ret)
		goto exit_bc12_check_chrdet;

	ret = get_chip_usb_reg_cfg_array(dev, "bc12-detect-sdp-stop",
			&(priv->bc_detect_cfg.bc12_detect_sdp_stop), &(priv->bc_detect_cfg.bc12_detect_sdp_stop_num));
	if (ret)
		goto exit_bc12_detect_sdp_stop;

	ret = get_chip_usb_reg_cfg_array(dev, "bc12-secondary-detect",
			&(priv->bc_detect_cfg.bc12_secondary_detect), &(priv->bc_detect_cfg.bc12_secondary_detect_num));
	if (ret)
		goto exit_bc12_secondary_detect;

	ret = get_chip_usb_reg_cfg_array(dev, "bc12-secondary-detect-done",
			&(priv->bc_detect_cfg.bc12_secondary_detect_done), &(priv->bc_detect_cfg.bc12_secondary_detect_done_num));
	if (ret)
		goto exit_bc12_secondary_detect_done;

	ret = get_chip_usb_reg_cfg_array(dev, "bc12-detect-cdp-done",
			&(priv->bc_detect_cfg.bc12_detect_cdp_done), &(priv->bc_detect_cfg.bc12_detect_cdp_done_num));
	if (ret)
		goto exit_bc12_detect_cdp_done;

	ret = get_chip_usb_reg_cfg_array(dev, "bc12-detect-dcd-fail",
			&(priv->bc_detect_cfg.bc12_detect_dcd_fail), &(priv->bc_detect_cfg.bc12_detect_dcd_fail_num));
	if (ret)
		goto exit_bc12_detect_dcd_fail;

	ret = get_chip_usb_reg_cfg_array(dev, "bc12-primary-detect-fail",
			&(priv->bc_detect_cfg.bc12_primary_detect_fail), &(priv->bc_detect_cfg.bc12_primary_detect_fail_num));
	if (ret)
		goto exit_bc12_primary_detect_fail;

	ret = get_chip_usb_reg_cfg_array(dev, "bc12-detect-close",
			&(priv->bc_detect_cfg.bc12_detect_close), &(priv->bc_detect_cfg.bc12_detect_close_num));
	if (ret)
		goto exit_bc12_detect_close;

	return ret;

exit_bc12_detect_close:
	free_chip_usb_reg_cfg_array(priv->bc_detect_cfg.bc12_primary_detect_fail,
			priv->bc_detect_cfg.bc12_primary_detect_fail_num);
exit_bc12_primary_detect_fail:
	free_chip_usb_reg_cfg_array(priv->bc_detect_cfg.bc12_detect_dcd_fail, priv->bc_detect_cfg.bc12_detect_dcd_fail_num);
exit_bc12_detect_dcd_fail:
	free_chip_usb_reg_cfg_array(priv->bc_detect_cfg.bc12_detect_cdp_done, priv->bc_detect_cfg.bc12_detect_cdp_done_num);
exit_bc12_detect_cdp_done:
	free_chip_usb_reg_cfg_array(priv->bc_detect_cfg.bc12_secondary_detect_done,
			priv->bc_detect_cfg.bc12_secondary_detect_done_num);
exit_bc12_secondary_detect_done:
	free_chip_usb_reg_cfg_array(priv->bc_detect_cfg.bc12_secondary_detect, priv->bc_detect_cfg.bc12_secondary_detect_num);
exit_bc12_secondary_detect:
	free_chip_usb_reg_cfg_array(priv->bc_detect_cfg.bc12_detect_sdp_stop, priv->bc_detect_cfg.bc12_detect_sdp_stop_num);
exit_bc12_detect_sdp_stop:
	free_chip_usb_reg_cfg_array(priv->bc_detect_cfg.bc12_check_chrdet, priv->bc_detect_cfg.bc12_check_chrdet_num);
exit_bc12_check_chrdet:
	free_chip_usb_reg_cfg_array(priv->bc_detect_cfg.bc12_primary_detect, priv->bc_detect_cfg.bc12_primary_detect_num);
exit_bc12_primary_detect:
	free_chip_usb_reg_cfg_array(priv->bc_detect_cfg.bc12_detect_dcd_end, priv->bc_detect_cfg.bc12_detect_dcd_end_num);
exit_bc12_detect_dcd_end:
	free_chip_usb_reg_cfg_array(priv->bc_detect_cfg.bc12_check_fsvplus, priv->bc_detect_cfg.bc12_check_fsvplus_num);
exit_bc12_check_fsvplus:
	free_chip_usb_reg_cfg_array(priv->bc_detect_cfg.bc12_detect_dcd, priv->bc_detect_cfg.bc12_detect_dcd_num);
exit_bc12_detect_dcd:
	USB2_T5_ERR("get bc detect resouce failed %d\n", ret);
	return ret;
}

static int get_ops_resource(struct phy_priv *priv)
{
	struct device *dev = priv->dev;
	int ret;

	USB2_T5_INFO("+\n");

	ret = get_chip_usb_reg_cfg_array(dev, "phy-reset",
			&priv->reset, &priv->reset_num);
	if (ret) {
		USB2_T5_ERR("get phy reset failed\n");
		return -ENOENT;
	}

	ret = get_chip_usb_reg_cfg_array(dev, "phy-unreset",
			&priv->unreset, &priv->unreset_num);
	if (ret) {
		USB2_T5_ERR("get phy unrest failed %d\n", ret);
		goto exit_unreset;
	}

	ret = get_chip_usb_reg_cfg_array(dev, "vbus-valid",
				&priv->vbus_valid, &priv->vbus_valid_num);
	if (ret) {
		USB2_T5_ERR("get vbus valid failed %d\n", ret);
		goto exit_vbus_valid;
	}

	ret = get_chip_usb_reg_cfg_array(dev, "vbus-invalid",
				&priv->vbus_invalid, &priv->vbus_invalid_num);
	if (ret) {
		USB2_T5_ERR("get vbus invalid failed %d\n", ret);
		goto exit_vbus_invalid;
	}

	priv->set_eye_param_enable = of_property_read_bool(priv->dev->of_node, "set-eye-param-enable");
	if (priv->set_eye_param_enable) {
		ret = get_chip_usb_reg_cfg_array(dev, "host-eye-param",
				&priv->host_eye_param, &priv->host_eye_param_num);
		if (ret) {
			USB2_T5_ERR("get host eye param failed %d\n", ret);
			goto exit_host_eye_param;
		}
		ret = get_chip_usb_reg_cfg_array(dev, "device-eye-param",
				&priv->device_eye_param, &priv->device_eye_param_num);
		if (ret) {
			USB2_T5_ERR("get  device eye param failed %d\n", ret);
			goto exit_device_eye_param;
		}
	}

	priv->bc12_detect_enable = of_property_read_bool(priv->dev->of_node, "bc12-detect-enable");
	if (priv->bc12_detect_enable) {
		ret = get_bc12_ops_resource(priv);
		if (ret) {
			USB2_T5_ERR("get bc1.2 ops resource failed %d\n", ret);
			goto exit_bc12_resource;
		}
	}

	priv->usb20phy_power = regulator_get(priv->dev, "phy-supply");
	if (IS_ERR_OR_NULL(priv->usb20phy_power))
		USB2_T5_ERR("get usb2 phy ldo 3.3v failed!\n");

	USB2_T5_INFO("-\n");
	return ret;

exit_bc12_resource:
	if (priv->set_eye_param_enable)
		free_chip_usb_reg_cfg_array(priv->device_eye_param, priv->device_eye_param_num);
exit_device_eye_param:
	if (priv->set_eye_param_enable)
		free_chip_usb_reg_cfg_array(priv->host_eye_param, priv->host_eye_param_num);
exit_host_eye_param:
	free_chip_usb_reg_cfg_array(priv->vbus_invalid, priv->vbus_invalid_num);
exit_vbus_invalid:
	free_chip_usb_reg_cfg_array(priv->vbus_valid, priv->vbus_valid_num);
exit_vbus_valid:
	free_chip_usb_reg_cfg_array(priv->unreset, priv->unreset_num);
exit_unreset:
	free_chip_usb_reg_cfg_array(priv->reset, priv->reset_num);
	USB2_T5_ERR("- %d\n", ret);
	return ret;
}

static void put_bc12_ops_resource(struct phy_priv *priv)
{
	if (!(priv->bc12_detect_enable))
		return;

	free_chip_usb_reg_cfg_array(priv->bc_detect_cfg.bc12_detect_close,
					priv->bc_detect_cfg.bc12_detect_close_num);
	free_chip_usb_reg_cfg_array(priv->bc_detect_cfg.bc12_primary_detect_fail,
					priv->bc_detect_cfg.bc12_primary_detect_fail_num);
	free_chip_usb_reg_cfg_array(priv->bc_detect_cfg.bc12_detect_dcd_fail,
					priv->bc_detect_cfg.bc12_detect_dcd_fail_num);
	free_chip_usb_reg_cfg_array(priv->bc_detect_cfg.bc12_detect_cdp_done,
					priv->bc_detect_cfg.bc12_detect_cdp_done_num);
	free_chip_usb_reg_cfg_array(priv->bc_detect_cfg.bc12_secondary_detect_done,
					priv->bc_detect_cfg.bc12_secondary_detect_done_num);
	free_chip_usb_reg_cfg_array(priv->bc_detect_cfg.bc12_secondary_detect,
					priv->bc_detect_cfg.bc12_secondary_detect_num);
	free_chip_usb_reg_cfg_array(priv->bc_detect_cfg.bc12_detect_sdp_stop,
					priv->bc_detect_cfg.bc12_detect_sdp_stop_num);
	free_chip_usb_reg_cfg_array(priv->bc_detect_cfg.bc12_check_chrdet,
					priv->bc_detect_cfg.bc12_check_chrdet_num);
	free_chip_usb_reg_cfg_array(priv->bc_detect_cfg.bc12_primary_detect,
					priv->bc_detect_cfg.bc12_primary_detect_num);
	free_chip_usb_reg_cfg_array(priv->bc_detect_cfg.bc12_detect_dcd_end,
					priv->bc_detect_cfg.bc12_detect_dcd_end_num);
	free_chip_usb_reg_cfg_array(priv->bc_detect_cfg.bc12_check_fsvplus,
					priv->bc_detect_cfg.bc12_check_fsvplus_num);
	free_chip_usb_reg_cfg_array(priv->bc_detect_cfg.bc12_detect_dcd,
					priv->bc_detect_cfg.bc12_detect_dcd_num);
	return;
}
static void put_ops_resource(struct phy_priv *priv)
{
	USB2_T5_INFO("+\n");

	free_chip_usb_reg_cfg_array(priv->reset, priv->reset_num);

	free_chip_usb_reg_cfg_array(priv->unreset, priv->unreset_num);

	free_chip_usb_reg_cfg_array(priv->vbus_valid,
					priv->vbus_valid_num);
	free_chip_usb_reg_cfg_array(priv->vbus_invalid,
					priv->vbus_invalid_num);
	if (priv->set_eye_param_enable) {
		free_chip_usb_reg_cfg_array(priv->host_eye_param,
						priv->host_eye_param_num);
		free_chip_usb_reg_cfg_array(priv->device_eye_param,
						priv->device_eye_param_num);
	}
	put_bc12_ops_resource(priv);

	USB2_T5_INFO("-\n");
}

static int get_clk_resource(struct phy_priv *priv)
{
	struct device *dev = priv->dev;
	int ret;

	if (priv->is_fpga)
		return 0;

	ret = chip_usb_get_clks(dev->of_node, &priv->clks, &priv->num_clks);
	if (ret)
		USB2_T5_ERR("get clk fail %d\n", ret);

	return ret;
}

static void put_clk_resource(struct phy_priv *priv)
{
	int ret;
	if (priv->is_fpga)
		return;

	ret = chip_usb_put_clks(priv->clks, priv->num_clks);
	if (ret)
		USB2_T5_ERR("put clk fail %d\n", ret);
}

static int get_dts_resource(struct phy_priv *priv)
{
	priv->is_fpga = of_property_read_bool(priv->dev->of_node, "is-fpga");
	priv->disable_bc = of_property_read_bool(priv->dev->of_node,
					"disable-bc");

	USB2_T5_INFO("is_fpga %d, disable_bc %d!\n",
		priv->is_fpga, priv->disable_bc);

	if (get_clk_resource(priv)) {
		USB2_T5_ERR("get clk failed\n");
		return -EINVAL;
	}

	if (get_ops_resource(priv)) {
		put_clk_resource(priv);
		USB2_T5_ERR("get ops failed\n");
		return -EINVAL;
	}

	return 0;
}

static void put_dts_resource(struct phy_priv *priv)
{
	put_clk_resource(priv);

	put_ops_resource(priv);
}

static int usb2_phy_t5_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct phy_provider *phy_provider = NULL;
	struct phy *phy = NULL;
	struct phy_priv *priv = NULL;
	int ret;

	USB2_T5_INFO("+\n");
	if (!misc_ctrl_is_ready()) {
		USB2_T5_ERR("misc ctrl is not ready\n");
		return -EPROBE_DEFER;
	}

	pm_runtime_set_active(dev);
	pm_runtime_enable(dev);
	pm_runtime_no_callbacks(dev);
	ret = pm_runtime_get_sync(dev);
	if (ret < 0) {
		USB2_T5_ERR("pm_runtime_get_sync failed\n");
		goto err_pm_put;
	}

	pm_runtime_forbid(dev);

	priv = devm_kzalloc(dev, sizeof(*priv), GFP_KERNEL);
	if (!priv) {
		ret = -ENOMEM;
		goto err_pm_allow;
	}

	priv->dev = dev;

	ret = get_dts_resource(priv);
	if (ret) {
		USB2_T5_ERR("get_dts_resource failed\n");
		goto err_pm_allow;
	}

	phy = devm_phy_create(dev, NULL, &usb2_phy_ops);
	if (IS_ERR(phy)) {
		ret = PTR_ERR(phy);
		goto err_get_dts;
	}

	priv->usb_phy.phy = phy;
	priv->usb_phy.detect_charger_type = detect_charger_type;
	priv->usb_phy.set_dpdm_pulldown = set_dpdm_pulldown;
	priv->usb_phy.set_vdp_src = set_vdp_src;
	priv->mode = PHY_MODE_INVALID;
	phy_set_drvdata(phy, &priv->usb_phy);
	atomic_set(&priv->desired_vdp_src, 0);

	phy_provider = devm_of_phy_provider_register(dev, of_phy_simple_xlate);
	if (IS_ERR(phy_provider)) {
		ret = PTR_ERR(phy_provider);
		goto err_get_dts;
	}

	platform_set_drvdata(pdev, priv);

	USB2_T5_ERR("-\n");
	return 0;

err_get_dts:
	put_dts_resource(priv);
err_pm_allow:
	pm_runtime_allow(dev);
err_pm_put:
	pm_runtime_put_sync(dev);
	pm_runtime_disable(dev);

	USB2_T5_INFO("ret %d\n", ret);
	return ret;
}

static int usb2_phy_t5_remove(struct platform_device *pdev)
{
	struct phy_priv *priv = (struct phy_priv *)platform_get_drvdata(pdev);

	put_dts_resource(priv);
	pm_runtime_allow(&pdev->dev);
	pm_runtime_put_sync(&pdev->dev);
	pm_runtime_disable(&pdev->dev);

	return 0;
}

static const struct of_device_id usb2_phy_t5_of_match[] = {
	{ .compatible = "hisilicon,usb2-phy-t5", },
	{ }
};
MODULE_DEVICE_TABLE(of, usb2_phy_t5_of_match);

static struct platform_driver usb2_phy_t5_driver = {
	.probe = usb2_phy_t5_probe,
	.remove = usb2_phy_t5_remove,
	.driver = {
		.name = "usb2-phy-t5",
		.of_match_table = usb2_phy_t5_of_match,
	}
};
module_platform_driver(usb2_phy_t5_driver);

MODULE_AUTHOR("Hu Wang");
MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("USB2 PHY T5 Driver");
