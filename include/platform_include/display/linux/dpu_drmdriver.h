/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2014-2020. All rights reserved.
 * Description: drmdriver for dpu platform
 * Create: 2014/5/16
 */

#ifndef __DPU_DRMDRIVER_H__
#define __DPU_DRMDRIVER_H__

#include <linux/init.h>
#include <linux/mutex.h>

/* list sub-function id for access register service */
#define ACCESS_REGISTER_FN_MAIN_ID                          0xc500aa01UL
#define ACCESS_REGISTER_FN_SUB_ID_MASTER_SECURITY_CONFIG    0x55bbcce7UL
#define ACCESS_REGISTER_FN_SUB_ID_DDR_MODEM_SEC             0x55bbcce9UL
#define ACCESS_REGISTER_FN_SUB_ID_DDR_KERNEL_CODE_PROTECT   0x55bbccedUL
#define ACCESS_REGISTER_FN_SUB_ID_SECS_POWER_CTRL           0x55bbccf0UL
#define ACCESS_REGISTER_FN_SUB_ID_DDR_PERF_CTRL             0x55bbccf3UL

#define DRMDRIVER_MODULE_INIT_SUCCESS_FLG                   0x12345678

#define MAX_DSS_CHN_IDX                                     12

enum _master_id_type_ {
	MASTER_VDEC_ID = 0,
	MASTER_VENC_ID,
	MASTER_DSS_ID,
	MASTER_G3D_ID,
	MASTER_ID_MAX
};

enum master_dss_op_type {
	DSS_SMMU_INIT = 0,
	DSS_CH_MMU_SEC_CONFIG,
	DSS_CH_MMU_SEC_DECONFIG,
	DSS_CH_SEC_CONFIG,
	DSS_CH_SEC_DECONFIG,
	DSS_CH_DEFAULT_SEC_CONFIG,
	MASTER_OP_SECURITY_MAX,
};

/*
 * Below typedef are being used in DDR model for now,
 * please fix it in DDR group CSEC.
 */
enum compose_mode {
	ONLINE_COMPOSE_MODE,
	OFFLINE_COMPOSE_MODE,
	OVL1_ONLINE_COMPOSE_MODE,
	OVL3_OFFLINE_COMPOSE_MODE,
	MAX_COMPOSE_MODE = 8, /* is same with scene id max for new platform */
};

struct tag_atfd_data {
	phys_addr_t  buf_phy_addr;
	size_t buf_size;
	u8 *buf_virt_addr;
	struct mutex atfd_mutex;  /* The mutex of ATFD */
	u32 module_init_success_flg;
};

#ifdef CONFIG_DRMDRIVER
#ifndef CONFIG_AB_APCP_NEW_INTERFACE
noinline s32 atfd_hisi_service_access_register_smc(u64 main_fun_id,
						   u64 buff_addr_phy,
						   u64 data_len,
						   u64 sub_fun_id);
#endif
void configure_master_security(u32 is_security, s32 master_id);
void configure_dss_register_security(u32 addr, u32 val, u8 bw, u8 bs);
s32 configure_dss_service_security(u32 master_op_type, u32 channel, u32 mode);
#else /* !CONFIG_DRMDRIVER */
#ifndef CONFIG_AB_APCP_NEW_INTERFACE
static inline s32 atfd_hisi_service_access_register_smc(u64 main_fun_id,
							u64 buff_addr_phy,
							u64 data_len,
							u64 sub_fun_id)
{
	return 0;
}
#endif

static inline void configure_master_security(u32 is_security, s32 master_id)
{
}

static inline void configure_dss_register_security(
	u32 addr, u32 val, u8 bw, u8 bs)
{
}

static inline s32 configure_dss_service_security(u32 master_op_type,
						 u32 channel, u32 mode)
{
	return 0;
}
#endif

#endif /* __DPU_DRMDRIVER_H__ */
