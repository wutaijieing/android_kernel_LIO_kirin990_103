/* Copyright (c) Hisilicon Technologies Co., Ltd. 2012-2019. All rights reserved.
 * Description: define struct opt_nve_info_user and declear function
 * nve_direct_access_interface that will be used by other functions or files.
 * if you want to visit NV partition, i.e. read NV items or write NV
 * items in other files, you should include this .h file.
 */

#ifndef __NVE_AP_KERNEL_INTERFACE_H__
#define __NVE_AP_KERNEL_INTERFACE_H__

#include <linux/types.h>
#define NV_NAME_LENGTH 8     /* NV name maximum length */
#define NVE_NV_DATA_SIZE 104 /* NV data maximum length */

#define NV_WRITE 0 /* NV write operation */
#define NV_READ 1  /* NV read  operation */

struct opt_nve_info_user {
	uint32_t nv_operation;
	uint32_t nv_number;
	char nv_name[NV_NAME_LENGTH];
	uint32_t valid_size;
	u_char nv_data[NVE_NV_DATA_SIZE];
};
#define NVEACCESSDATA _IOWR('M', 25, struct opt_nve_info_user)

#ifdef CONFIG_NVE_AP_KERNEL
extern int nve_direct_access_interface(struct opt_nve_info_user *user_info);
#else
static inline int nve_direct_access_interface(struct opt_nve_info_user *user_info)
{
	return -EOPNOTSUPP;
};
#endif

#endif
