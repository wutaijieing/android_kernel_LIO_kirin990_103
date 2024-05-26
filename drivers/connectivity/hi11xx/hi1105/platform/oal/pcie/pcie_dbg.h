

#ifndef __PCIE_DBG_H__
#define __PCIE_DBG_H__

#ifdef _PRE_PLAT_FEATURE_HI110X_PCIE
#include <linux/kernel.h>
int32_t oal_pcie_sysfs_group_create(oal_kobject *root_obj);
void oal_pcie_sysfs_group_remove(oal_kobject *root_obj);
#endif

#endif /* end of pcie_dbg.h */