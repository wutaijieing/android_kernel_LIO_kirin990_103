/*
 * ov16885.c
 *
 * driver for ov16885 sensor.
 *
 * Copyright (c) 2001-2021, Huawei Tech. Co., Ltd. All rights reserved.
 *

 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */


#include <linux/module.h>
#include <linux/printk.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/rpmsg.h>

#include "hwsensor.h"
#include "sensor_commom.h"
#include "../pmic/hw_pmic.h"

//lint -save -e846 -e866 -e826 -e785 -e838 -e715 -e747 -e774 -e778 -e732 -e731
//lint -save -e514 -e30 -e84 -e64 -e650 -e737 -e31 -e64 -esym(528,*) -esym(753,*)
#define MASTER_SENSOR_INDEX  (0)
#define DELAY_6MS            (6)
#define DELAY_5MS            (5)
#define DELAY_1MS            (1)
#define DELAY_0MS            (0)

#define I2S(i) container_of((i), sensor_t, intf)
#define Sensor2Pdev(s) container_of((s).dev, struct platform_device, dev)

static bool s_ov16885_power_on = false; //false for power down, ture for power up
struct mutex ov16885_power_lock;
static struct platform_device *s_pdev = NULL;
static sensor_t *s_sensor = NULL;
static sensor_t s_ov16885;
static struct sensor_power_setting ov16885_power_setting[] = {
    //SCAM0 AVDD 2.8V [LDO13]
    {
        .seq_type = SENSOR_AVDD2,
        .config_val = LDO_VOLTAGE_V2P8V,
        .sensor_index = SENSOR_INDEX_INVALID,
        .delay = DELAY_0MS,
    },

    //SCAM0 DVDD 1.05V [LDO32]
    {
        .seq_type = SENSOR_DVDD2,
        .config_val = LDO_VOLTAGE_1P2V,//LDO_VOLTAGE_1P05V
        .sensor_index = SENSOR_INDEX_INVALID,
        .delay = DELAY_0MS,
    },

    //SCAM0 IOVDD 1.80V[LDO21]
    {
        .seq_type = SENSOR_IOVDD,
        .config_val = LDO_VOLTAGE_1P8V,
        .sensor_index = SENSOR_INDEX_INVALID,
        .delay = DELAY_1MS,
    },

    //S1 MCLK [ISP-CLK1]
    {
        .seq_type = SENSOR_MCLK,
        .sensor_index = SENSOR_INDEX_INVALID,
        .delay = DELAY_1MS,
    },

    //S1 RESET  [GPIO_031]
    {
        .seq_type = SENSOR_RST,
        .config_val = SENSOR_GPIO_HIGH,
        .sensor_index = SENSOR_INDEX_INVALID,
        .delay = DELAY_1MS,
    },
};
static struct sensor_power_setting ov16885_power_downsetting[] = {
    //S1 RESET  [GPIO_031]
    {
        .seq_type = SENSOR_RST,
        .config_val = SENSOR_GPIO_LOW,
        .sensor_index = SENSOR_INDEX_INVALID,
        .delay = DELAY_1MS,
    },
    //S1 MCLK [ISP-CLK1]
    {
        .seq_type = SENSOR_MCLK,
        .sensor_index = SENSOR_INDEX_INVALID,
        .delay = DELAY_1MS,
    },
    //SCAM0 IOVDD 1.80V[LDO21]
    {
        .seq_type = SENSOR_IOVDD,
        .config_val = LDO_VOLTAGE_1P8V,
        .sensor_index = SENSOR_INDEX_INVALID,
        .delay = DELAY_1MS,
    },
    //SCAM0 DVDD 1.05V [LDO32]
    {
        .seq_type = SENSOR_DVDD2,
        .config_val = LDO_VOLTAGE_1P2V,//LDO_VOLTAGE_1P05V
        .sensor_index = SENSOR_INDEX_INVALID,
        .delay = DELAY_0MS,
    },
    //SCAM0 AVDD 2.8V [LDO13]
    {
        .seq_type = SENSOR_AVDD2,
        .config_val = LDO_VOLTAGE_V2P8V,
        .sensor_index = SENSOR_INDEX_INVALID,
        .delay = DELAY_0MS,
    },
};

static char const* ov16885_get_name(hwsensor_intf_t* si)
{
    sensor_t* sensor = NULL;

    if (NULL == si) {
        cam_err("%s. si is NULL.", __func__);
        return NULL;
    }

    sensor = I2S(si);
    if (NULL == sensor || NULL == sensor->board_info || NULL == sensor->board_info->name) {
        cam_err("%s. sensor or board_info->name is NULL.", __func__);
        return NULL;
    }

    return sensor->board_info->name;
}

static int ov16885_power_up(hwsensor_intf_t* si)
{
    int ret = 0;
    sensor_t* sensor = NULL;
    if(NULL == si)
    {
        cam_err("%s. si is NULL.", __func__);
        return -EINVAL;
    }

    sensor = I2S(si);
    if (NULL == sensor || NULL == sensor->board_info || NULL == sensor->board_info->name) {
        cam_err("%s. sensor or board_info->name is NULL.", __func__);
        return -EINVAL;
    }
    cam_info("enter %s. index = %d name = %s", __func__, sensor->board_info->sensor_index, sensor->board_info->name);
    ret = hw_sensor_power_up_config(s_ov16885.dev, sensor->board_info);
    if (0 == ret ){
        cam_info("%s. power up config success.", __func__);
    }else{
        cam_err("%s. power up config fail.", __func__);
        return ret;
    }
    if (hw_is_fpga_board()) {
        cam_info("%s powerup by isp on FPGA", __func__);
    } else {
        ret = hw_sensor_power_up(sensor);
    }
    if (0 == ret )
    {
        cam_info("%s. power up sensor success.", __func__);
    }
    else
    {
        cam_err("%s. power up sensor fail.", __func__);
    }
    return ret;
}

static int ov16885_power_down(hwsensor_intf_t* si)
{
    int ret = 0;
    sensor_t* sensor = NULL;
    if(NULL == si)
    {
        cam_err("%s. si is NULL.", __func__);
        return -EINVAL;
    }

    sensor = I2S(si);
    if (NULL == sensor || NULL == sensor->board_info || NULL == sensor->board_info->name) {
        cam_err("%s. sensor or board_info->name is NULL.", __func__);
        return -EINVAL;
    }

    cam_info("enter %s. index = %d name = %s", __func__, sensor->board_info->sensor_index, sensor->board_info->name);
    if (hw_is_fpga_board()) {
        cam_info("%s poweroff by isp on FPGA", __func__);
    } else {
        ret = hw_sensor_power_down(sensor);
    }
    if (0 == ret )
    {
        cam_info("%s. power down sensor success.", __func__);
    }
    else
    {
        cam_err("%s. power down sensor fail.", __func__);
    }
    hw_sensor_power_down_config(sensor->board_info);
    return ret;
}

static int ov16885_csi_enable(hwsensor_intf_t* si)
{
    (void)si;
    return 0;
}

static int ov16885_csi_disable(hwsensor_intf_t* si)
{
    (void)si;
    return 0;
}

static int ov16885_match_id(hwsensor_intf_t* si, void * data)
{
    sensor_t* sensor = NULL;
    struct sensor_cfg_data *cdata = (struct sensor_cfg_data *)data;

    cam_info("%s enter.", __func__);
    if (!si || !data){
        cam_err("%s : si or data is null", __func__);
        return -EINVAL;
    }

    sensor = I2S(si);
    if (NULL == sensor || NULL == sensor->board_info || NULL == sensor->board_info->name) {
        cam_err("%s. sensor or board_info or name is NULL.", __func__);
        return -EINVAL;
    }

    cam_info("%s name:%s", __func__, sensor->board_info->name);

    cdata->data = sensor->board_info->sensor_index;

    return 0;
}

static int ov16885_config_power_on(hwsensor_intf_t* si)
{
    int ret = 0;

    if (NULL == si || NULL == si->vtbl) {
        cam_err("%s : si or si->vtbl is null", __func__);
        return -EINVAL;
    }
    mutex_lock(&ov16885_power_lock);

    if (NULL == si->vtbl->power_up)
    {
        cam_err("%s. si power_up is null", __func__);
        /*lint -e455 -esym(455,*)*/
        mutex_unlock(&ov16885_power_lock);
        /*lint -e455 +esym(455,*)*/
        return -EINVAL;
    }

    if (!s_ov16885_power_on){
        ret = si->vtbl->power_up(si);
        if (0 == ret) {
            s_ov16885_power_on = true;
        } else {
            cam_err("%s. power up fail.", __func__);
        }
    } else {
        cam_err("%s camera has powered on",__func__);
    }

    /*lint -e455 -esym(455,*)*/
    mutex_unlock(&ov16885_power_lock);
    /*lint -e455 +esym(455,*)*/

    return ret;
}

static int ov16885_config_power_off(hwsensor_intf_t* si)
{
    int ret = 0;

    if (NULL == si || NULL == si->vtbl) {
        cam_err("%s : si or si->vtbl is null", __func__);
        return -EINVAL;
    }
    mutex_lock(&ov16885_power_lock);

    if (NULL == si->vtbl->power_down)
    {
        cam_err("%s. si power_down is null", __func__);
        /*lint -e455 -esym(455,*)*/
        mutex_unlock(&ov16885_power_lock);
        /*lint -e455 +esym(455,*)*/
        return -EINVAL;
    }

    if (s_ov16885_power_on){
        ret = si->vtbl->power_down(si);
        if (0 != ret) {
            cam_err("%s. power down fail.", __func__);
        }
        s_ov16885_power_on = false;
    } else {
        cam_err("%s camera has powered off",__func__);
    }
    /*lint -e455 -esym(455,*)*/
    mutex_unlock(&ov16885_power_lock);
    /*lint -e455 +esym(455,*)*/

    return ret;
}

static int ov16885_config_match_id(hwsensor_intf_t* si, void *argp)
{
    int ret = 0;

    if (NULL == si->vtbl->match_id)
    {
        cam_err("%s. si power_up is null", __func__);
        ret = -EINVAL;
    } else {
        ret = si->vtbl->match_id(si,argp);
    }

    return ret;
}

static int ov16885_config(hwsensor_intf_t* si,void  *argp)
{
    struct sensor_cfg_data *data = NULL;
    int ret =0;

    if ((NULL == si) || (NULL == argp) || (NULL == si->vtbl)) {
        cam_err("%s : si, argp or si->vtbl is null", __func__);
        return -EINVAL;
    }

    data = (struct sensor_cfg_data *)argp;
    cam_debug("ov16885 cfgtype = %d",data->cfgtype);
    switch(data->cfgtype){
        case SEN_CONFIG_POWER_ON:
            ret = ov16885_config_power_on(si);
            break;
        case SEN_CONFIG_POWER_OFF:
            ret = ov16885_config_power_off(si);
            break;
        case SEN_CONFIG_WRITE_REG:
            break;
        case SEN_CONFIG_READ_REG:
            break;
        case SEN_CONFIG_WRITE_REG_SETTINGS:
            break;
        case SEN_CONFIG_READ_REG_SETTINGS:
            break;
        case SEN_CONFIG_ENABLE_CSI:
            break;
        case SEN_CONFIG_DISABLE_CSI:
            break;
        case SEN_CONFIG_MATCH_ID:
            ret = ov16885_config_match_id(si, argp);
            break;
        case SEN_CONFIG_RESET_HOLD:
            break;
        case SEN_CONFIG_RESET_RELEASE:
            break;
        default:
            cam_err("%s cfgtype(%d) is error", __func__, data->cfgtype);
            break;
    }
    return ret;
}

static hwsensor_vtbl_t s_ov16885_vtbl =
{
    .get_name = ov16885_get_name,
    .config = ov16885_config,
    .power_up = ov16885_power_up,
    .power_down = ov16885_power_down,
    .match_id = ov16885_match_id,
    .csi_enable = ov16885_csi_enable,
    .csi_disable = ov16885_csi_disable,
};
/* individual driver data for each device */

atomic_t volatile ov16885_powered = ATOMIC_INIT(0);

static sensor_t s_ov16885 =
{
    .intf = { .vtbl = &s_ov16885_vtbl, },
    .power_setting_array = {
        .size = ARRAY_SIZE(ov16885_power_setting),
        .power_setting = ov16885_power_setting,
    },
    .power_down_setting_array = {
        .size = ARRAY_SIZE(ov16885_power_down_setting),
        .power_setting = ov16885_power_down_setting,
    },
    .p_atpowercnt = &ov16885_powered,
};

static const struct of_device_id
s_ov16885_dt_match[] =
{
    {
        .compatible = "huawei,ov16885",
        .data = &s_ov16885.intf,
    },
    { } /* terminate list */
};

MODULE_DEVICE_TABLE(of, s_ov16885_dt_match);
/* platform driver struct */
static int32_t ov16885_platform_probe(struct platform_device* pdev);
static int32_t ov16885_platform_remove(struct platform_device* pdev);
static struct platform_driver s_ov16885_driver =
{
    .probe = ov16885_platform_probe,
    .remove = ov16885_platform_remove,
    .driver =
    {
        .name = "huawei,ov16885",
        .owner = THIS_MODULE,
        .of_match_table = s_ov16885_dt_match,
    },
};


static int32_t ov16885_platform_probe( struct platform_device* pdev)
{
    int rc = 0;

    const struct of_device_id *id = NULL;
    hwsensor_intf_t *intf = NULL;
    sensor_t *sensor = NULL;
    struct device_node *np = NULL;
    cam_info("enter %s ",__func__);

    if(NULL == pdev)
    {
        cam_err("%s pdev is NULL", __func__);
        return -EINVAL;
    }

    mutex_init(&ov16885_power_lock);
    np = pdev->dev.of_node;
    if (!np) {
        cam_err("%s of_node is NULL", __func__);
        return -ENODEV;
    }

    id = of_match_node(s_ov16885_dt_match, np);
    if (!id) {
        cam_err("%s none id matched", __func__);
        return -ENODEV;
    }

    intf = (hwsensor_intf_t*)id->data;
    if (NULL == intf) {
        cam_err("%s intf is NULL", __func__);
        return -ENODEV;
    }

    sensor = I2S(intf);
    if(NULL == sensor){
        cam_err("%s sensor is NULL rc %d", __func__, rc);
        return -ENODEV;
    }
    rc = hw_sensor_get_dt_data(pdev, sensor);
    if (rc < 0) {
        cam_err("%s no dt data rc %d", __func__, rc);
        return -ENODEV;
    }

    sensor->dev = &pdev->dev;

    rc = hwsensor_register(pdev, intf);
    if (rc < 0) {
        cam_err("%s hwsensor_register failed rc %d\n", __func__, rc);
        return -ENODEV;
    }
    s_pdev = pdev;

    rc = rpmsg_sensor_register(pdev, (void*)sensor);
    if (rc < 0) {
        hwsensor_unregister(s_pdev);
        s_pdev = NULL;
        cam_err("%s rpmsg_sensor_register failed rc %d\n", __func__, rc);
        return -ENODEV;
    }
    s_sensor = sensor;

    return rc;
}

static int32_t ov16885_platform_remove(struct platform_device * pdev)
{
    if( NULL != s_sensor)
    {
        rpmsg_sensor_unregister((void*)s_sensor);
        s_sensor = NULL;
    }
    if (NULL != s_pdev) {
        hwsensor_unregister(s_pdev);
        s_pdev = NULL;
    }
    return 0;
}
static int __init ov16885_init_module(void)
{
    cam_info("enter %s",__func__);
    return platform_driver_probe(&s_ov16885_driver, ov16885_platform_probe);
}

static void __exit ov16885_exit_module(void)
{
    platform_driver_unregister(&s_ov16885_driver);
}

module_init(ov16885_init_module);
module_exit(ov16885_exit_module);
MODULE_DESCRIPTION("ov16885");
MODULE_LICENSE("GPL v2");
//lint -restore
