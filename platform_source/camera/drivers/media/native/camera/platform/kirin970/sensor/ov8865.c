 /*
 *  SOC camera driver source file
 *
 *  Copyright (C) Huawei Technology Co., Ltd.
 *
 * Date:
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
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

#define I2S(i) container_of(i, sensor_t, intf)
#define Sensor2Pdev(s) container_of((s).dev, struct platform_device, dev)
#define CAMERA_LOG_DEBUG

static char const* ov8865_get_name(hwsensor_intf_t* si);
static int ov8865_config(hwsensor_intf_t* si, void  *argp);
static int ov8865_power_up(hwsensor_intf_t* si);
static int ov8865_power_down(hwsensor_intf_t* si);
static int ov8865_match_id(hwsensor_intf_t *si, void *data);
static int ov8865_csi_enable(hwsensor_intf_t* si);
static int ov8865_csi_disable(hwsensor_intf_t* si);

static bool s_ov8865_power_on = false;

// ES UDP power up seq
static struct sensor_power_setting ov8865_power_setting[] = {
    //disable main camera reset
    {
        .seq_type = SENSOR_SUSPEND,
        .config_val = SENSOR_GPIO_LOW,
        .sensor_index = SENSOR_INDEX_INVALID,
        .delay = 0,
    },
    //disable sub camera reset
    {
        .seq_type = SENSOR_SUSPEND2,
        .config_val = SENSOR_GPIO_LOW,
        .sensor_index = SENSOR_INDEX_INVALID,
        .delay = 0,
    },
    //MCAM IOVDD 1.80V
    {
        .seq_type = SENSOR_IOVDD,
        .config_val = LDO_VOLTAGE_1P8V,
        .sensor_index = SENSOR_INDEX_INVALID,
        .delay = 1,
    },
    //MCAM1 AVDD 2.8V
    {
        .seq_type = SENSOR_AVDD,
        .config_val = LDO_VOLTAGE_V2P8V,
        .sensor_index = SENSOR_INDEX_INVALID,
        .delay = 0,
    },
    //MCAM1 DVDD 1.2V
    {
        .seq_type = SENSOR_DVDD,
        .config_val = LDO_VOLTAGE_1P2V,
        .sensor_index = SENSOR_INDEX_INVALID,
        .delay = 0,
    },
    //VCM [2.60v]
    {
        .seq_type = SENSOR_VCM_AVDD,
        .config_val = LDO_VOLTAGE_V2P8V,
        .sensor_index = SENSOR_INDEX_INVALID,
        .delay = 0,
    },
    {
        .seq_type = SENSOR_MCLK,
        .sensor_index = SENSOR_INDEX_INVALID,
        .delay = 1,
    },
    {
        .seq_type = SENSOR_RST,
        .config_val = SENSOR_GPIO_LOW,
        .sensor_index = SENSOR_INDEX_INVALID,
        .delay = 1,
    },
};

static struct sensor_power_setting ov8865_fpga_power_setting[] = {
    //SENSOR IO
    {
        .seq_type = SENSOR_PMIC,
        .seq_val  = VOUT_LDO_1,
        .config_val = LDO_VOLTAGE_1P8V,
        .sensor_index = SENSOR_INDEX_INVALID,
        .delay = 0,
    },

    //MCAM1 AVDD 2.8V
    {
        .seq_type = SENSOR_AVDD,
        .data = (void*)"front-sensor-avdd",
        .config_val = LDO_VOLTAGE_V2P8V,
        .sensor_index = SENSOR_INDEX_INVALID,
        .delay = 0,
    },

    //MCAM1 DVDD 1.2V
    {
        .seq_type = SENSOR_DVDD,
        .config_val = LDO_VOLTAGE_V1P25V,
        .sensor_index = SENSOR_INDEX_INVALID,
        .delay = 0,
    },

    //VCM [2.85v]
    {
        .seq_type = SENSOR_VCM_AVDD,
        .config_val = LDO_VOLTAGE_V2P85V,
        .sensor_index = SENSOR_INDEX_INVALID,
        .delay = 0,
    },

    {
        .seq_type = SENSOR_MCLK,
        .sensor_index = SENSOR_INDEX_INVALID,
        .delay = 1,
    },
    {
        .seq_type = SENSOR_RST,
        .config_val = SENSOR_GPIO_LOW,
        .sensor_index = SENSOR_INDEX_INVALID,
        .delay = 1,
    },
};

static struct sensor_power_setting ov8865_ml_power_setting[] = {
    //SENSOR IO
    {
        .seq_type = SENSOR_PMIC,
        .seq_val  = VOUT_LDO_1,
        .config_val = LDO_VOLTAGE_1P8V,
        .sensor_index = SENSOR_INDEX_INVALID,
        .delay = 0,
    },

    //MCAM1 AVDD 2.8V
    {
        .seq_type = SENSOR_AVDD,
        .data = (void*)"front-sensor-avdd",
        .config_val = LDO_VOLTAGE_V2P8V,
        .sensor_index = SENSOR_INDEX_INVALID,
        .delay = 0,
    },

    //MCAM1 DVDD 1.2V
    {
        .seq_type = SENSOR_DVDD,
        .config_val = LDO_VOLTAGE_V1P25V,
        .sensor_index = SENSOR_INDEX_INVALID,
        .delay = 0,
    },

    //VCM [2.85v]
    {
        .seq_type = SENSOR_VCM_AVDD,
        .config_val = LDO_VOLTAGE_V2P85V,
        .sensor_index = SENSOR_INDEX_INVALID,
        .delay = 0,
    },

    {
        .seq_type = SENSOR_MCLK,
        .sensor_index = SENSOR_INDEX_INVALID,
        .delay = 1,
    },
    {
        .seq_type = SENSOR_RST,
        .config_val = SENSOR_GPIO_LOW,
        .sensor_index = SENSOR_INDEX_INVALID,
        .delay = 1,
    },
};

static hwsensor_vtbl_t
s_ov8865_vtbl =
{
    .get_name = ov8865_get_name,
    .config = ov8865_config,
    .power_up = ov8865_power_up,
    .power_down = ov8865_power_down,
    .match_id = ov8865_match_id,
    .csi_enable = ov8865_csi_enable,
    .csi_disable = ov8865_csi_disable,
};

static sensor_t s_ov8865 =
{
    .intf = { .vtbl = &s_ov8865_vtbl, },
    .power_setting_array = {
        .size = ARRAY_SIZE(ov8865_power_setting),
        .power_setting = ov8865_power_setting,
    },
};

static sensor_t ov8865_fpga =
{
    .intf = { .vtbl = &s_ov8865_vtbl, },
    .power_setting_array = {
        .size = ARRAY_SIZE(ov8865_fpga_power_setting),
        .power_setting = ov8865_fpga_power_setting,
    },
};

static sensor_t s_ov8865_ml =
{
    .intf = { .vtbl = &s_ov8865_vtbl, },
    .power_setting_array = {
            .size = ARRAY_SIZE(ov8865_ml_power_setting),
            .power_setting = ov8865_ml_power_setting,
    },
};

static const struct of_device_id
s_ov8865_dt_match[] =
{
    {
        .compatible = "huawei,ov8865",
        .data = &s_ov8865.intf,
    },
    {
        .compatible = "huawei,ov8865_ml",
        .data = &s_ov8865_ml.intf,
    },
    {
        .compatible = "huawei,ov8865_fpga",
        .data = &ov8865_fpga.intf,
    },
    {
    },
};

MODULE_DEVICE_TABLE(of, s_ov8865_dt_match);

/* platform driver struct */
static int32_t ov8865_platform_probe(struct platform_device* pdev);
static int32_t ov8865_platform_remove(struct platform_device* pdev);
static struct platform_driver
s_ov8865_driver =
{
    .probe = ov8865_platform_probe,
    .remove = ov8865_platform_remove,
    .driver =
    {
        .name = "huawei,ov8865",
        .owner = THIS_MODULE,
        .of_match_table = s_ov8865_dt_match,
    },
};

static char const*
ov8865_get_name(
        hwsensor_intf_t* si)
{
    sensor_t* sensor = I2S(si);
    return sensor->board_info->name;
}

static int
ov8865_power_up(
        hwsensor_intf_t* si)
{
    int ret = 0;
    sensor_t* sensor = NULL;
    sensor = I2S(si);
    cam_info("enter %s. index = %d name = %s", __func__, sensor->board_info->sensor_index, sensor->board_info->name);

    if (hw_is_fpga_board()){
        ret = do_sensor_power_on(sensor->board_info->sensor_index, sensor->board_info->name);
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

static int
ov8865_power_down(
        hwsensor_intf_t* si)
{
    int ret = 0;
    sensor_t* sensor = NULL;
    sensor = I2S(si);
    cam_info("enter %s. index = %d name = %s", __func__, sensor->board_info->sensor_index, sensor->board_info->name);
    if (hw_is_fpga_board()) {
        ret = do_sensor_power_off(sensor->board_info->sensor_index, sensor->board_info->name);
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
    return ret;
}

static int ov8865_csi_enable(hwsensor_intf_t* si)
{
    return 0;
}

static int ov8865_csi_disable(hwsensor_intf_t* si)
{
    return 0;
}

static int
ov8865_match_id(
        hwsensor_intf_t *si, void *data)
{
    sensor_t* sensor = I2S(si);
    struct sensor_cfg_data *cdata = (struct sensor_cfg_data *)data;
    char * sensor_name[] = {"OV8865_2L"};

    cam_info("%s enter.", __func__);

    cdata->data = SENSOR_INDEX_INVALID;  /*lint !e569 */

    if(!strncmp(sensor->board_info->name,"OV8865_2L",strlen("OV8865_2L")))
    {
        strncpy(cdata->cfg.name, sensor_name[0], DEVICE_NAME_SIZE-1);
        cdata->data = sensor->board_info->sensor_index;
    }
    else
    {
        strncpy(cdata->cfg.name, sensor->board_info->name, DEVICE_NAME_SIZE-1);
        cdata->data = sensor->board_info->sensor_index;
    }

    if (cdata->data != SENSOR_INDEX_INVALID) /*lint !e650 */
    {
        cam_info("%s, cdata->cfg.name = %s", __func__,cdata->cfg.name );
    }
    cam_info("%s TODO.  cdata->data=%d", __func__, cdata->data);
    return 0;
}

static int
ov8865_config(
        hwsensor_intf_t* si,void  *argp)
{
    struct sensor_cfg_data *data;
    int ret =0;
    data = (struct sensor_cfg_data *)argp;
    cam_info("ov8865 cfgtype = %d",data->cfgtype);
    switch(data->cfgtype){
        case SEN_CONFIG_POWER_ON:
            if (!s_ov8865_power_on) {
                ret = si->vtbl->power_up(si);
                s_ov8865_power_on = true;
            }
            break;
        case SEN_CONFIG_POWER_OFF:
            if (s_ov8865_power_on) {
                ret = si->vtbl->power_down(si);
                s_ov8865_power_on = false;
            }
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
            ret = si->vtbl->match_id(si,argp);
            break;
        default:
            cam_err("%s cfgtype(%d) is error", __func__, data->cfgtype);
            break;
    }

    return ret;
}

static int32_t
ov8865_platform_probe(
        struct platform_device* pdev)
{
    int rc = 0;
    struct device_node *np = pdev->dev.of_node;
    const struct of_device_id *id;
    hwsensor_intf_t *intf;
    sensor_t *sensor;

    cam_info("enter %s",__func__);

    if (!np) {
        cam_err("%s of_node is NULL", __func__);
        return -ENODEV;
    }

    id = of_match_node(s_ov8865_dt_match, np);
    if (!id) {
        cam_err("%s none id matched", __func__);
        return -ENODEV;
    }

    intf = (hwsensor_intf_t*)id->data;
    sensor = I2S(intf);
    rc = hw_sensor_get_dt_data(pdev, sensor);
    if (rc < 0) {
        cam_err("%s no dt data", __func__);
        return -ENODEV;
    }
    sensor->dev = &pdev->dev;

    rc = hwsensor_register(pdev, intf);
    rc = rpmsg_sensor_register(pdev, (void*)sensor);

    return rc;
}

static int32_t
ov8865_platform_remove(
        struct platform_device * pdev)
{
    struct device_node *np = pdev->dev.of_node;
    const struct of_device_id *id;
    hwsensor_intf_t *intf;
    sensor_t *sensor;

    cam_info("enter %s",__func__);

    if (!np) {
        cam_info("%s of_node is NULL", __func__);
        return 0;
    }
    /* don't use dev->p->driver_data
    * we need to search again */
    id = of_match_node(s_ov8865_dt_match, np);
    if (!id) {
        cam_info("%s none id matched", __func__);
        return 0;
    }

    intf = (hwsensor_intf_t*)id->data;
    sensor = I2S(intf);

    rpmsg_sensor_unregister((void*)&sensor);
    hwsensor_unregister(pdev);
    return 0;
}
static int __init
ov8865_init_module(void)
{
    cam_info("Enter: %s", __func__);
    return platform_driver_probe(&s_ov8865_driver,
        ov8865_platform_probe);
}

static void __exit
ov8865_exit_module(void)
{
    rpmsg_sensor_unregister((void*)&s_ov8865);
    hwsensor_unregister(Sensor2Pdev(s_ov8865));
    platform_driver_unregister(&s_ov8865_driver);
}

module_init(ov8865_init_module);
module_exit(ov8865_exit_module);
MODULE_DESCRIPTION("ov8865");
MODULE_LICENSE("GPL v2");

/*  20150412 end> */
