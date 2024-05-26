

#ifndef __PCIE_FIRMWARE_H__
#define __PCIE_FIRMWARE_H__

#include "pcie_linux.h"

int32_t oal_pcie_firmware_read(oal_pcie_linux_res *pst_pcie_lres, uint8_t *buff, int32_t len, uint32_t timeout);
int32_t oal_pcie_firmware_write(oal_pcie_linux_res *pst_pcie_lres, uint8_t *buff, int32_t len);

#endif /* end of oal_pcie_firmware.h */
