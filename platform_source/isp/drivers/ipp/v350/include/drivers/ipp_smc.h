/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2012-2018. All rights reserved.
 * File name    ipp_smc.h
 * Description:

 * Version      0
 */

#ifndef IPP_SMC_H
#define IPP_SMC_H

int atfhipp_smmu_enable(unsigned int mode);
int atfhipp_smmu_disable(unsigned int mode);
int atfhipp_smmu_smrx(unsigned int swid, unsigned int len, unsigned int sid,
	unsigned int ssid, unsigned int mode);
int atfhipp_smmu_pref(unsigned int swid, unsigned int len);

#ifdef IPP_KERNEL_USE_PCIE_VERIFY
int atfhipp_vbus_pcie_pwrup(u64 pcie_flag, u64 pcie_addr);
int atfhipp_vbus_pcie_pwrdn(u64 pcie_flag, u64 pcie_addr);
int atfhipp_ipp_pcie_pwrup(u64 pcie_flag, u64 pcie_addr);
int atfhipp_ipp_pcie_pwrdn(u64 pcie_flag, u64 pcie_addr);
#endif

#endif
/* ************************************** END ***************************** */
