

#include "pcie_linux.h"
#include <linux/types.h>
#include <linux/list.h>
#include "securec.h"
#include "oal_types.h"
#include "oal_util.h"
#include "oal_pci_if.h"
#include "pcie_host.h"
#include "pcie_comm.h"
#include "hisi_ini.h"

LIST_HEAD(g_pcie_res_head);
OAL_STATIC oal_pcie_comm_res g_comm_res;

oal_pcie_linux_res* oal_pcie_res_init(oal_pci_dev_stru *pst_pci_dev, uint32_t dev_id)
{
    int32_t ret;
    uint8_t reg8 = 0;
    oal_pcie_linux_res *pcie_res = NULL;
    oal_pcie_res_node *cur = NULL;
    oal_pcie_res_node *next = NULL;

    ret = oal_pci_read_config_byte(pst_pci_dev, PCI_REVISION_ID, &reg8);
    if (ret) {
        oal_io_print("pci: read revision id  failed, ret=%d\n", ret);
        return NULL;
    }

    list_for_each_entry_safe(cur, next, &g_pcie_res_head, list) {
        pcie_res = &cur->pcie_res;
        if ((pcie_res->pst_bus != NULL) && (pcie_res->pst_bus->dev_id == dev_id)) {
            return pcie_res;
        }
    }

    cur = (oal_pcie_res_node *)oal_memalloc(sizeof(oal_pcie_res_node));
    if (cur == NULL) {
        return NULL;
    }
    memset_s((void *)cur, sizeof(*cur), 0, sizeof(*cur));
    list_add_tail(&cur->list, &g_pcie_res_head);
    pcie_res = &cur->pcie_res;
    oal_init_completion(&pcie_res->ep_res.st_pcie_ready);
    pcie_res->comm_res = &g_comm_res;
    if (atomic_read(&pcie_res->comm_res->refcnt) != 0) {
        atomic_inc(&pcie_res->comm_res->refcnt);
        return pcie_res;
    }
    oal_spin_lock_init(&pcie_res->comm_res->st_lock);
    pcie_res->comm_res->pcie_dev = pst_pci_dev;
    pcie_res->comm_res->version = (pst_pci_dev->vendor) | (pst_pci_dev->device << 16);
    pcie_res->comm_res->revision = (uint32_t)reg8;
    pci_print_log(PCI_LOG_INFO, "wifi pcie version:0x%8x, revision:%d",
        pcie_res->comm_res->version, pcie_res->comm_res->revision);
    atomic_set(&pcie_res->comm_res->refcnt, 1);
    pci_set_drvdata(pst_pci_dev, &g_pcie_res_head);
    return pcie_res;
}

int32_t oal_pcie_res_deinit(oal_pcie_linux_res *pcie_res)
{
    oal_pcie_res_node *cur = list_entry(pcie_res, oal_pcie_res_node, pcie_res);
    if (atomic_read(&pcie_res->comm_res->refcnt) != 0) {
        atomic_dec(&pcie_res->comm_res->refcnt);
    }

    list_del(&cur->list);
    oal_free(cur);
    return OAL_SUCC;
}

oal_pcie_linux_res *oal_get_default_pcie_handler(void)
{
    oal_pcie_res_node *cur = NULL;
    oal_pcie_res_node *next = NULL;
    oal_pcie_linux_res *pcie_res = NULL;

    list_for_each_entry_safe(cur, next, &g_pcie_res_head, list) {
        pcie_res = &cur->pcie_res;
        if ((pcie_res->pst_bus != NULL) && (pcie_res->pst_bus->dev_id == hcc_get_main_dev())) {
            return pcie_res;
        }
    }

    return NULL;
}

oal_pcie_linux_res *oal_get_pcie_handler(int32_t dev_id)
{
    oal_pcie_res_node *cur = NULL;
    oal_pcie_res_node *next = NULL;
    oal_pcie_linux_res *pcie_res = NULL;

    list_for_each_entry_safe(cur, next, &g_pcie_res_head, list) {
        pcie_res = &cur->pcie_res;
        if ((pcie_res->pst_bus != NULL) && (pcie_res->pst_bus->dev_id == dev_id)) {
            return pcie_res;
        }
    }

    return NULL;
}
EXPORT_SYMBOL(oal_get_pcie_handler);
