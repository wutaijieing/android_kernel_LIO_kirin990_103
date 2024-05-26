

#include "oal_hcc_bus.h"
#define HCC_BUS_DEV_NAME_LEN 32

static int32_t hcc_bus_get_int32_para(int i, hcc_bus_dev_res *devs, const char *para, int *data)
{
    char field[32];
    hcc_bus_dev *bus_dev = NULL;

    bus_dev = &devs->dev_list[i];
    if (snprintf_s(field, sizeof(field), sizeof(field) - 1, "bus_dev_%d_%s", i, para) < 0) {
        return -OAL_EINVAL;
    }
    if (get_cust_conf_int32(INI_MODU_PLAT, (const char *)field, data) == INI_FAILED) {
        return -OAL_EINVAL;
    }
    return OAL_SUCC;
}

static int32_t hcc_bus_get_string_para(int i, hcc_bus_dev_res *devs, const char *para, char *data, uint32_t len)
{
    char field[32];

    if (snprintf_s(field, sizeof(field), sizeof(field) - 1, "bus_dev_%d_%s", i, para) < 0) {
        return -OAL_EINVAL;
    }
    if (get_cust_conf_string(INI_MODU_PLAT, (const char *)field, data, len) == INI_FAILED) {
        return -OAL_EINVAL;
    }
    return OAL_SUCC;
}

static void hcc_bus_param_uninit(hcc_bus_dev_res *devs)
{
    int i;

    for (i = 0; i < devs->dev_num; i++) {
        if (devs->dev_list[i].name != NULL) {
            oal_free(devs->dev_list[i].name);
        }
    }
    oal_free(devs->dev_list);
    devs->dev_list = NULL;
    devs->dev_num = 0;
}

static int32_t hcc_bus_get_ini_para(hcc_bus_dev_res *devs, hcc_bus_dev *bus_dev)
{
    int32_t i, tmp;
    for (i = 0; i < devs->dev_num; i++) {
        bus_dev = &devs->dev_list[i];
        if (hcc_bus_get_int32_para(i, devs, "id", &bus_dev->dev_id) != OAL_SUCC) {
            return OAL_FAIL;
        }
        if (hcc_bus_get_int32_para(i, devs, "bus_type", &bus_dev->init_bus_type) != OAL_SUCC) {
            return OAL_FAIL;
        }
        if (hcc_bus_get_int32_para(i, devs, "wakeup_gpio_en", &bus_dev->is_wakeup_gpio_support) != OAL_SUCC) {
            return OAL_FAIL;
        }
        if (hcc_bus_get_int32_para(i, devs, "cap", &bus_dev->bus_cap) != OAL_SUCC) {
            return OAL_FAIL;
        }
        if (hcc_bus_get_int32_para(i, devs, "flowctrl_gpio_en", &tmp) != OAL_SUCC) {
            return OAL_FAIL;
        }
        bus_dev->en_flowctrl_gpio_registered = tmp;
        bus_dev->name = oal_memalloc(sizeof(char) * HCC_BUS_DEV_NAME_LEN);
        if (bus_dev->name == NULL) {
            return OAL_FAIL;
        }
        if (hcc_bus_get_string_para(i, devs, "name", bus_dev->name, HCC_BUS_DEV_NAME_LEN) != OAL_SUCC) {
            return OAL_FAIL;
        }
        oal_io_print("dev_id:%d, bus_type:%d, wakeup_gpio_en:%d, bus_cap:%d, flowctrl_gpio_en:%d, name:%s\n",
                     bus_dev->dev_id, bus_dev->init_bus_type, bus_dev->is_wakeup_gpio_support, bus_dev->bus_cap,
                     bus_dev->en_flowctrl_gpio_registered, bus_dev->name);
    }
    return OAL_SUCC;
}

int32_t hcc_bus_param_init(hcc_bus_dev_res *devs)
{
    int32_t tmp;
    hcc_bus_dev *bus_dev = NULL;

    if (get_cust_conf_int32(INI_MODU_PLAT, "bus_dev_num", &tmp) == INI_FAILED) {
        return -OAL_EINVAL;
    }
    devs->dev_num = tmp;
#ifdef _PRE_PLAT_FEATURE_MULTI_HCC
    if (get_cust_conf_int32(INI_MODU_PLAT, "main_dev", &tmp) != INI_FAILED) {
        devs->main_dev = tmp;
        oal_io_print("main_dev id 0x%x\n", tmp);
    }
#endif
    devs->dev_list = oal_memalloc(sizeof(hcc_bus_dev) * devs->dev_num);
    if (devs->dev_list == NULL) {
        return -OAL_ENOMEM;
    }
    memset_s(devs->dev_list, sizeof(hcc_bus_dev) * devs->dev_num, 0, sizeof(hcc_bus_dev) * devs->dev_num);

    if (hcc_bus_get_ini_para(devs, bus_dev) != OAL_SUCC) {
        hcc_bus_param_uninit(devs);
        return OAL_FAIL;
    }
    return OAL_SUCC;
}
