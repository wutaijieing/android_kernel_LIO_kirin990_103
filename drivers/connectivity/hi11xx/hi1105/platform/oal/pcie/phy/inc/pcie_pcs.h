

#ifndef __PCIE_PCS_H__
#define __PCIE_PCS_H__

#include "oal_types.h"

#ifndef NEWLINE
#define NEWLINE "\n"
#endif

#define pcie_pcs_udelay(utime) oal_udelay(utime)
#define pcie_pcs_print oal_io_print
void pcie_pcs_print_split_log_info(char *str);
#endif

