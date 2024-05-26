/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
 * Description: pcie soc
 * Author: platform
 * Create: 2021-03-09
 * History:
 */

#ifndef __PCIE_SOC_HI1161_H__
#define __PCIE_SOC_HI1161_H__
#define HISI_PCIE_SLAVE_START_ADDRESS        (0x80000000)
#define HISI_PCIE_SLAVE_END_ADDRESS          (0xFFFFFFFFFFFFFFFF)
#define HISI_PCIE_IP_REGION_SIZE             (HISI_PCIE_SLAVE_END_ADDRESS - HISI_PCIE_SLAVE_START_ADDRESS + 1)
#define HISI_PCIE_MASTER_START_ADDRESS       (0x0)
#define HISI_PCIE_MASTER_END_ADDRESS         (HISI_PCIE_MASTER_START_ADDRESS + HISI_PCIE_IP_REGION_SIZE - 1)
#define HISI_PCIE_IP_REGION_OFFSET           (HISI_PCIE_SLAVE_START_ADDRESS - HISI_PCIE_MASTER_START_ADDRESS)

#endif
