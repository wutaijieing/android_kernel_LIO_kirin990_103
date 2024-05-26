/*
 *  ISP driver, qos.c
 *
 * Copyright (c) 2018 ISP Technologies CO., Ltd.
 *
 */
#include <linux/arm-smccc.h>
#include <securec.h>
#include <platform_include/see/bl31_smc.h>

noinline int atfd_isp_service_isp_smc(u64 _funcid,
				u64 _arg0, u64 _arg1, u64 _arg2)
{
	struct arm_smccc_res res;

	arm_smccc_smc(_funcid, _arg0, _arg1, _arg2,
			0, 0, 0, 0, &res);

	return (int)res.a0;
}

int hisp_smc_setparams(u64 shrd_paddr)
{
	atfd_isp_service_isp_smc(ISP_FN_SET_PARAMS, shrd_paddr, 0, 0);
	return 0;
}

int hisp_smc_cpuinfo_dump(u64 param_pa_addr)
{
	atfd_isp_service_isp_smc(ISP_FN_ISPCPU_NS_DUMP, param_pa_addr, 0, 0);
	return 0;
}

void hisp_smc_isp_init(u64 pgd_base)
{
	atfd_isp_service_isp_smc(ISP_FN_ISP_INIT, pgd_base, 0, 0);
}

void hisp_smc_isp_exit(void)
{
	atfd_isp_service_isp_smc(ISP_FN_ISP_EXIT, 0, 0, 0);
}

int hisp_smc_ispcpu_init(void)
{
	return atfd_isp_service_isp_smc(ISP_FN_ISPCPU_INIT, 0, 0, 0);
}

void hisp_smc_ispcpu_exit(void)
{
	atfd_isp_service_isp_smc(ISP_FN_ISPCPU_EXIT, 1, 0, 0);
}

int hisp_smc_ispcpu_map(void)
{
	return atfd_isp_service_isp_smc(ISP_FN_ISPCPU_MAP, 0, 0, 0);
}

void hisp_smc_ispcpu_unmap(void)
{
	atfd_isp_service_isp_smc(ISP_FN_ISPCPU_UNMAP, 0, 0, 0);
}

void hisp_smc_set_nonsec(void)
{
	atfd_isp_service_isp_smc(ISP_FN_SET_NONSEC, 0, 0, 0);
}

int hisp_smc_disreset_ispcpu(void)
{
	return atfd_isp_service_isp_smc(ISP_FN_DISRESET_ISPCPU, 0, 0, 0);
}

void hisp_smc_ispsmmu_ns_init(u64 pgt_addr)
{
	atfd_isp_service_isp_smc(ISP_FN_ISPSMMU_NS_INIT, pgt_addr, 0, 0);
}

int hisp_smc_get_ispcpu_idle(void)
{
	return atfd_isp_service_isp_smc(ISP_FN_GET_ISPCPU_IDLE, 0, 0, 0);
}

int hisp_smc_send_fiq2ispcpu(void)
{
	return atfd_isp_service_isp_smc(ISP_FN_SEND_FIQ_TO_ISPCPU, 0, 0, 0);
}

int hisp_smc_isptop_power_up(void)
{
	return atfd_isp_service_isp_smc(ISP_FN_ISPTOP_PU, 0, 0, 0);
}

int hisp_smc_isptop_power_down(void)
{
	return atfd_isp_service_isp_smc(ISP_FN_ISPTOP_PD, 0, 0, 0);
}

int hisp_smc_phy_csi_connect(u64 info_addr)
{
	return atfd_isp_service_isp_smc(ISP_PHY_CSI_CONNECT, info_addr, 0, 0);
}

int hisp_smc_phy_csi_disconnect(void)
{
	return atfd_isp_service_isp_smc(ISP_PHY_CSI_DISCONNECT, 0, 0, 0);
}

int hisp_smc_qos_cfg(void)
{
	return atfd_isp_service_isp_smc(ISP_FN_QOS_CFG, 0, 0, 0);
}

int hisp_smc_csi_mode_sel(u64 mode)
{
	return atfd_isp_service_isp_smc(ISP_FN_HICSI_SEL, mode, 0, 0);
}

int hisp_smc_i2c_pad_type(void)
{
	return atfd_isp_service_isp_smc(ISP_FN_I2C_PAD_TYPE, 0, 0, 0);
}

int hisp_smc_i2c_deadlock(u64 type)
{
	return atfd_isp_service_isp_smc(ISP_FN_I2C_DEADLOCK, type, 0, 0);
}

#ifdef DEBUG_HISP
int hisp_smc_ispnoc_r8_power_up(u64 pcie_flag, u64 pcie_addr)
{
	return atfd_isp_service_isp_smc(ISP_FN_ISPNOC_R8_PU, pcie_flag, pcie_addr, 0);
}

int hisp_smc_ispnoc_r8_power_down(u64 pcie_flag, u64 pcie_addr)
{
	return atfd_isp_service_isp_smc(ISP_FN_ISPNOC_R8_PD, pcie_flag, pcie_addr, 0);
}

int hisp_smc_media2_vcodec_power_up(u64 pcie_flag, u64 pcie_addr)
{
	return atfd_isp_service_isp_smc(ISP_FN_MEDIA2_VBUS_PU, pcie_flag, pcie_addr, 0);
}

int hisp_smc_media2_vcodec_power_down(u64 pcie_flag, u64 pcie_addr)
{
	return atfd_isp_service_isp_smc(ISP_FN_MEDIA2_VBUS_PD, pcie_flag, pcie_addr, 0);
}
#endif

int hisp_smc_vq_map(void)
{
	return atfd_isp_service_isp_smc(ISP_FN_VQ_MAP, 0, 0, 0);
}

int hisp_smc_vq_unmap(void)
{
	return atfd_isp_service_isp_smc(ISP_FN_VQ_UNMAP, 0, 0, 0);
}
