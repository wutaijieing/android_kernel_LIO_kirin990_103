/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
 * Description: process ion in igs dev
 * Create: 2020-10-15
 */

#ifndef __LINUX_CONTEXTHUB_IGS_ION_H__
#define __LINUX_CONTEXTHUB_IGS_ION_H__

#include <linux/miscdevice.h>
#include <linux/platform_device.h>
#include <linux/types.h>

/* get pyhsical address of ion according to shared fd */
int igs_get_ion_phys(int share_fd, dma_addr_t *addr, struct device *dev);
#endif
