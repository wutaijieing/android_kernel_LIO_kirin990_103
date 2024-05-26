/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2016-2021. All rights reserved.
 * Description: Contexthub share memory driver
 * Create: 2016-04-01
 */
#ifndef __IOMCU_BOOT_H__
#define __IOMCU_BOOT_H__
#include <platform_include/smart/linux/base/ap/protocol.h>
#include <linux/types.h>
#include <iomcu_ddr_map.h>

#ifdef __cplusplus
extern "C" {
#endif

#define IOMCU_CONFIG_SIZE (DDR_CONFIG_SIZE * 3 / 8)
#define IOMCU_CONFIG_START DDR_CONFIG_ADDR_AP

#define PA_MARGIN_NUM           2 /* PA_HCI:0 PA_NBTI:1 */
#define NPU_AVS_DATA_NUN        14
#define NPU_SVFD_DATA_NUN       28
#define NPU_SVFD_PARA_NUN       2
#define PROFILE_NUM             7

enum dump_loc {
	DL_NONE = 0,
	DL_TCM,
	DL_EXT,
	DL_BOTTOM = DL_EXT,
};

enum DUMP_REGION {
	DE_TCM_CODE,
	DE_DDR_CODE,
	DE_DDR_DATA,
	DE_BOTTOM = 16,
};

struct dump_region_config {
	u8 loc;
	u8 reserved[3];
};

struct dump_config {
	u64 dump_addr;
	u32 dump_size;
	u32 reserved1;
	u64 ext_dump_addr;
	u32 ext_dump_size;
	u8 enable;
	u8 finish;
	u8 reason;
	u8 reserved2;
	struct dump_region_config elements[DE_BOTTOM];
};

struct timestamp_kernel {
	u64 syscnt;
	u64 kernel_ns;
};

struct timestamp_iomcu_base {
	u64 syscnt;
	u64 kernel_ns;
	u32 timer32k_cyc;
	u32 reserved;
};

struct bbox_config {
	u64 log_addr;
	u32 log_size;
	u32 rsvd;
};

struct npu_cfg_data {
	u32 pa_margin[PA_MARGIN_NUM];
	u32 avs_data[NPU_AVS_DATA_NUN];
	u32 svfd_data[NPU_SVFD_DATA_NUN];
	u32 svfd_para[NPU_SVFD_PARA_NUN];
	u32 prof_margin[PROFILE_NUM];
	u32 level2_chip;
};

struct plat_cfg_str {
	u64 magic;
	struct timestamp_kernel timestamp_base;
	struct timestamp_iomcu_base timestamp_base_iomcu;
	struct bbox_config bbox_conifg;
	struct dump_config dump_config;
	struct npu_cfg_data npu_data;
};

void write_timestamp_base_to_sharemem(void);
void inputhub_init_before_boot(void);
void inputhub_init_after_boot(void);
struct plat_cfg_str *get_plat_cfg_str(void);

#ifdef __cplusplus
}
#endif
#endif /* __LINUX_INPUTHUB_CMU_H__ */
