

#ifndef __PCIE_DBG_H__
#define __PCIE_DBG_H__

#include <linux/kernel.h>
int32_t oal_pcie_sysfs_group_create(oal_kobject *root_obj);
void oal_pcie_sysfs_group_remove(oal_kobject *root_obj);
#endif /* end of pcie_dbg.h */