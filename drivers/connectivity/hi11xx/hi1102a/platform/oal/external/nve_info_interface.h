
#ifndef __NVE_INFO_INTERFACE_H__
#define __NVE_INFO_INTERFACE_H__

#ifdef CONFIG_ARCH_PLATFORM
#include <linux/mtd/nve_ap_kernel_interface.h>
#define external_nve_info_user opt_nve_info_user
#define external_nve_direct_access_interface nve_direct_access_interface
#else
#ifdef CONFIG_HISI_NVE
#include <linux/mtd/hisi_nve_interface.h>
#define external_nve_info_user hisi_nve_info_user
#define external_nve_direct_access_interface hisi_nve_direct_access
#else
#include <linux/mtd/hw_nve_interface.h>
#define external_nve_info_user hw_nve_info_user
#define external_nve_direct_access_interface hw_nve_direct_access
#endif
#endif

#endif
