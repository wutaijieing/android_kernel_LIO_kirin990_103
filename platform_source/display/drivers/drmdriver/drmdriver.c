/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2014-2020. All rights reserved.
 * Description: drmdriver for platform
 * Create: 2014/5/16
 */

#include <platform_include/display/linux/dpu_drmdriver.h>
#include <linux/version.h>
#include <linux/arm-smccc.h>
#include <linux/dma-mapping.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <global_ddr_map.h>

#define ATFD_MEM_START_ADDRESS  "dpu,share-memory-drm"
#define DPU_DRM_SEC_NUMB       0xabcde0
/*
 * the interface is used as access some secure registers
 * from unsecure-Kernel layer.
 *
 * main_fun_id -  fixed as ACCESS_REGISTER_FN_MAIN_ID,equal to 0xc500aa01
 * buff_addr_phy - output para for read, the allocated buffer address in Kernel,
 *                 must be physical address
 * data_len - the buffer size, unit is Bytes
 * sub_fun_id - the sub function id, started by 0x55bbcce0,
 *              the index increase one by one
 */
struct dss_reg {
	u32 addr;
	u32 val;
	u8 bw;
	u8 bs;
};

#ifndef CONFIG_AB_APCP_NEW_INTERFACE
noinline s32 atfd_hisi_service_access_register_smc(u64 _main_fun_id,
						   u64 _buff_addr_phy,
						   u64 _data_len,
						   u64 _sub_fun_id)
{
	struct arm_smccc_res res = {0};
	arm_smccc_smc(_main_fun_id, _buff_addr_phy, _data_len, _sub_fun_id,
			0, 0, 0, 0, &res);
	return (int)res.a0;
}
#endif

static struct tag_atfd_data g_atfd_config;

void configure_master_security(u32 is_security, s32 master_id)
{
	struct arm_smccc_res res = {0};

	if (master_id >= MASTER_ID_MAX ||
	    master_id == MASTER_DSS_ID ||
	    master_id < 0) {
		pr_err("%s, invalid master_id=%d.\n", __func__, master_id);
		return;
	}
	if (is_security != 0 && is_security != 1) {
		pr_err("%s, invalid is_security=%u.\n", __func__, is_security);
		return;
	}

	is_security |= DPU_DRM_SEC_NUMB;

	arm_smccc_smc(ACCESS_REGISTER_FN_MAIN_ID, (u64)is_security,
		(u64)(u32)master_id,
		(u64)ACCESS_REGISTER_FN_SUB_ID_MASTER_SECURITY_CONFIG,
		0, 0, 0, 0, &res);
}
EXPORT_SYMBOL_GPL(configure_master_security);

void configure_dss_register_security(u32 addr, u32 val, u8 bw, u8 bs)
{
	struct arm_smccc_res res = {0};
	struct dss_reg dss_data;

	dss_data.addr = addr;
	dss_data.val = val;
	dss_data.bw = bw;
	dss_data.bs = bs;

	if (!g_atfd_config.buf_virt_addr ||
	    g_atfd_config.buf_size < sizeof(dss_data)) {
		pr_err("%s, no available mem\n", __func__);
		return;
	}
	if (DRMDRIVER_MODULE_INIT_SUCCESS_FLG !=
	    g_atfd_config.module_init_success_flg) {
		pr_err("%s, module is not ready now\n", __func__);
		return;
	}
	mutex_lock(&g_atfd_config.atfd_mutex);
	memcpy((struct dss_reg *)g_atfd_config.buf_virt_addr,
	       &dss_data, sizeof(dss_data));

	arm_smccc_smc(ACCESS_REGISTER_FN_MAIN_ID,
		g_atfd_config.buf_phy_addr,
		(u64)MASTER_DSS_ID,
		ACCESS_REGISTER_FN_SUB_ID_MASTER_SECURITY_CONFIG,
		0, 0, 0, 0, &res);

	mutex_unlock(&g_atfd_config.atfd_mutex);
}
EXPORT_SYMBOL_GPL(configure_dss_register_security);

s32 configure_dss_service_security(u32 master_op_type,
				   u32 channel, u32 mode)
{
	struct arm_smccc_res res = {0};
	u64 value;

	if (mode >= MAX_COMPOSE_MODE || channel >= MAX_DSS_CHN_IDX) {
		pr_err("%s:invalid mode=%d, channel=%u",
		       __func__, mode, channel);
		return -1;
	}
	if (master_op_type >= (u32)MASTER_OP_SECURITY_MAX) {
		pr_err("%s:invalid master_op_type=%u",
		       __func__, master_op_type);
		return -1;
	}
	/*
	 * |mode|channel|mater_op_type|DPU_DRM_SEC_NUMB|
	 * the value 48 and 32 are used for combine
	 * the mode and channel value to one single 64bit data
	 */
	value = (((u64)mode << 48) | ((u64)channel << 32) |
		 master_op_type | DPU_DRM_SEC_NUMB);

	arm_smccc_smc(ACCESS_REGISTER_FN_MAIN_ID,
		value,
		(u64)MASTER_DSS_ID,
		(u64)ACCESS_REGISTER_FN_SUB_ID_MASTER_SECURITY_CONFIG,
		0, 0, 0, 0, &res);
	return (int)res.a0;
}
EXPORT_SYMBOL_GPL(configure_dss_service_security);

static s32 __init dpu_drm_driver_init(void)
{
	s32 ret;
	struct device_node *np = NULL;
	u32 data[2] = {0};
	phys_addr_t shm_base = ATF_SUB_RESERVED_BL31_SHARE_MEM_PHYMEM_BASE;

	memset((void *)&g_atfd_config, 0, sizeof(g_atfd_config));
	np = of_find_compatible_node(NULL, NULL, ATFD_MEM_START_ADDRESS);
	if (!np) {
		pr_err("%s: no compatible node found.\n", __func__);
		return -ENODEV;
	}

	ret = of_property_read_u32_array(np, "reg", &data[0], 2);
	if (ret != 0) {
		if (ret == -EINVAL)
			pr_err("%s in reg node does not exist\n", "reg");
		else
			pr_err("%s in reg node has invalid value\n", "reg");
		return ret;
	}
	g_atfd_config.buf_phy_addr = shm_base + data[0];
	g_atfd_config.buf_size = data[1];

	g_atfd_config.buf_virt_addr = (u8 *)ioremap(shm_base +
						    data[0], data[1]);
	if (!g_atfd_config.buf_virt_addr) {
		pr_err("%s:allocate failed.\n", __func__);
		return -EFAULT;
	}
	mutex_init(&g_atfd_config.atfd_mutex);
	g_atfd_config.module_init_success_flg =
		DRMDRIVER_MODULE_INIT_SUCCESS_FLG;
	return ret;
}

arch_initcall(dpu_drm_driver_init);
MODULE_DESCRIPTION("drm driver module");
MODULE_AUTHOR("security-ap");
MODULE_LICENSE("GPL");
