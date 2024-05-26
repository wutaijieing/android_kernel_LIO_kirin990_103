#ifndef _ADUIO_PCIE_PROC_H
#define _ADUIO_PCIE_PROC_H

void start_pcie_thread(void);
int8_t pcie_write_om_data_init(void);
int8_t pcie_read_nv_data_init(void);
int32_t write_om_data_async(uint8_t *addr, uint32_t len);
void pcie_deinit(void);

#endif