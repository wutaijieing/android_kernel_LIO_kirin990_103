/*
 * Copyright (c) 2018-2019 Huawei Technologies Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef __MM_LB_PRIV_H
#define __MM_LB_PRIV_H

#include <linux/platform_device.h>

#define     GLB_RTL_VER              0x0000
#define     GLB_RTL_INFO             0x0004
#define     GLB_RTL_INF1             0x0008
#define     GLB_RTL_INF2             0x000C
#define     GLB_ACPU                 0x0020
#define     GLB_ROUTE_PLC_S          0x0060
#define     GLB_WAY_EN               0x0080
#define     GLB_SCQ_CTRL0            0x0140
#define     GLB_SCQ_CTRL1            0x0144
#define     GLB_CFC0                 0x0180
#define     GLB_CFC1                 0x0184
#define     GLB_CKG_BYP              0x0800
#define     GLB_CKG_EXT              0x0804
#define     GLB_FUNC_BYP             0x0808
#define     GLB_PARAM_OVRD           0x080C
#define     GLB_PMU_CTRL             0x0820
#define     GLB_PMU_CNT8_11_MODE_SEL 0x0824
#define     CFG_GID_SEC_ATTR_S       0x1000
#define     GLB_ALLC_PLC_S           0x1100
#define     GLB_SCB_EN_S             0x1104
#define     GLB_MEM_LP_DYN           0x1200
#define     GLB_MEM_LP0_STA          0x1210
#define     GLB_MEM_LP1_STA          0x1214
#define     GLB_MEM_LP2_STA          0x1218
#define     GLB_MEM_LP_PARAM         0x121C
#define     GLB_SPMEM_CTRL           0x1220
#define     GLB_TPMEM_CTRL           0x1224
#define     GLB_FUNC_BYP_S           0x1280
#define     GLB_DUMMY_RW0_S          0x1284
#define     GLB_DUMMY_RW1_S          0x1288
#define     GLB_DUMMY_RO0_S          0x128C
#define     GLB_DUMMY_RO1_S          0x1290
#define     CFG_INT_STATUS           0x2000
#define     CMO_CFG_INT_INI          0x2004
#define     CMO_CFG_INT_EN           0x2008
#define     CMO_CFG_INT_CLR          0x200C
#define     CFG_CMO_QUEUE_CNT        0x2018
#define     CFG_CMO_REGULAR_EVT_TIME 0x2020
#define     CFG_READY_WAIT_TIME      0x2030
#define     CFG_CMO_ERR_CMD_L        0x2058
#define     CFG_CMO_ERR_CMD_H        0x205C
#define     CFG_DFX_DISTR            0x2078
#define     CFG_CMO_STATUS           0x2100
#define     CFG_CMO_CLEAR            0x2200
#define     SC_RT_INT_EN             0x2308
#define     SC_RT_INT_CLR            0x230C
#define     CFG_NON_SECURITY_CFG_S   0x3000
#define     CFG_ALLOW_MID_S          0x3010
#define     CFG_CMO_SET_S            0x3100
#define     CFG_CMO_EN_ADDR_HOLE_S   0x3014
#define     CFG_CMO_STR_ADDR_HOLE_0_S    0x3018
#define     CFG_CMO_END_ADDR_HOLE_0_S    0x301C
#define     CFG_CMO_STR_ADDR_HOLE_1_S    0x3020
#define     CFG_CMO_END_ADDR_HOLE_1_S    0x3024
#define     CFG_CMO_STR_ADDR_HOLE_2_S    0x3028
#define     CFG_CMO_END_ADDR_HOLE_2_S    0x302C
#define     CFG_CMO_STR_ADDR_HOLE_3_S    0x3030
#define     CFG_CMO_END_ADDR_HOLE_3_S    0x3034
#define     CFG_CMO_TPMEM_CTRL_S    0x3038
#define     CFG_JTAG_SUPER_ACCESS   0x3038
#define     GID_QUOTA               0x4000
#define     GID_WAY_ALLC            0x4004
#define     GID_ALLC_PLC            0x4008
#define     GID_MID_FLT0            0x400C
#define     GID_MID_FLT1            0x4010

#define     CMO_CMD                 0x5000
#define     CMO_STAT                CFG_CMO_STATUS
#define     CMO_CLEAR               CFG_CMO_CLEAR

#define     SLC_PERF_MUX_MODE       0x6000
#define     SLC_CM_CNT_EN           0x6004
#define     SLC_CM_CNT_MUX0         0x6008
#define     SLC_CM_CNT_MUX1         0x600C
#define     SLC_CM_CNT_MUX2         0x6010
#define     SLC_PMU_CNT0_TH_L       0x6014
#define     SLC_PMU_CNT0_TH_H       0x6018
#define     SLC_PMU_CNT1_TH_L       0x601C
#define     SLC_PMU_CNT1_TH_H       0x6020
#define     SLC_PMU_CNT2_TH_L       0x6024
#define     SLC_PMU_CNT2_TH_H       0x6028
#define     SLC_PMU_CNT3_TH_L       0x602C
#define     SLC_PMU_CNT3_TH_H       0x6030
#define     SLC_PMU_CNT4_TH_L       0x6034
#define     SLC_PMU_CNT4_TH_H       0x6038
#define     SLC_PMU_CNT5_TH_L       0x603C
#define     SLC_PMU_CNT5_TH_H       0x6040
#define     SLC_PMU_CNT6_TH_L       0x6044
#define     SLC_PMU_CNT6_TH_H       0x6048
#define     SLC_PMU_CNT7_TH_L       0x604C
#define     SLC_PMU_CNT7_TH_H       0x6050
#define     SLC_PMU_CNT8_TH_L       0x6054
#define     SLC_PMU_CNT8_TH_H       0x6058
#define     SLC_PMU_CNT9_TH_L       0x605C
#define     SLC_PMU_CNT9_TH_H       0x6060
#define     SLC_PMU_CNT10_TH_L      0x6064
#define     SLC_PMU_CNT10_TH_H      0x6068
#define     SLC_PMU_CNT11_TH_L      0x606C
#define     SLC_PMU_CNT11_TH_H      0x6070

#define     SLC_BP2_CFG             0x6094
#define     SLC_BP5_CFG             0x6098
#define     SLC_BP6_CFG             0x609C
#define     SLC_BP7_CFG_0           0x60A0
#define     SLC_BP7_CFG_1           0x60A4
#define     SLC_BP7_CFG_2           0x60A8
#define     SLC_BP8_CFG_0           0x60AC
#define     SLC_BP8_CFG_1           0x60B0
#define     SLC_BP8_CFG_3           0x60B4
#define     SLC_BP8_CFG_4           0x60B8
#define     SLC_BP9_CFG_0           0x60BC
#define     SLC_BP10_CFG_0          0x60C8
#define     SLC_BP10_CFG_1          0x60CC
#define     SLC_BP10_CFG_2          0x60D0
#define     SLC_BP11_CFG_0          0x60D4
#define     SLC_BP12_CFG_0          0x60DC
#define     SLC_BP12_CFG_1          0x60E0
#define     SLC_BP12_CFG_2          0x60E4
#define     SLC_BP14A_CFG_0         0x60E8
#define     SLC_BP14A_CFG_1         0x60EC
#define     SLC_BP14B_CFG_0         0x60F0
#define     SLC_BP14B_CFG_1         0x60F4
#define     SLC_BP15_CFG_0          0x60F8

#define     SLC_TCP0_CFG_0          0x60FC
#define     SLC_TCP0_CFG_1          0x6100
#define     SLC_TCP0_CFG_2          0x6104
#define     SLC_TCP1_CFG_0          0x6108
#define     SLC_TCP1_CFG_1          0x610C
#define     SLC_TCP1_CFG_2          0x6110
#define     SLC_TCP2A_CFG_0         0x6114
#define     SLC_TCP2A_CFG_1         0x6118
#define     SLC_TCP2B_CFG_0         0x611C
#define     SLC_TCP2B_CFG_1         0x6120
#define     SLC_TCP3A_CFG_0         0x6124
#define     SLC_TCP3A_CFG_1         0x6128
#define     SLC_TCP3B_CFG_0         0x612C
#define     SLC_TCP3B_CFG_1         0x6130
#define     SLC_TCP4_CFG_0          0x6134
#ifdef CONFIG_MM_LB_GEMINI
#define     SLC_TCP5A_CFG_0         0x6300
#define     SLC_TCP5A_CFG_1         0x6304
#define     SLC_TCP5A_CFG_2         0x6308
#define     SLC_TCP5B_CFG_0         0x6310
#define     SLC_TCP5B_CFG_1         0x6314
#define     SLC_TCP5B_CFG_2         0x6318
#else
#define     SLC_TCP5_CFG_0          0x6138
#define     SLC_TCP5_CFG_1          0x613C
#endif
#define     SLC_TCP6A_CFG_0         0x6140
#define     SLC_TCP6A_CFG_1         0x6144
#define     SLC_TCP6B_CFG_0         0x6148
#define     SLC_TCP6B_CFG_1         0x614C
#define     SLC_TCP7A_CFG_0         0x6150
#define     SLC_TCP7A_CFG_1         0x6154
#define     SLC_TCP7B_CFG_0         0x6158
#define     SLC_TCP7B_CFG_1         0x615C
#ifdef CONFIG_MM_LB_GEMINI
#define     SLC_TCP8A_CFG_0         0x6320
#define     SLC_TCP8A_CFG_1         0x6324
#define     SLC_TCP8A_CFG_2         0x6328
#define     SLC_TCP8B_CFG_0         0x6330
#define     SLC_TCP8B_CFG_1         0x6334
#define     SLC_TCP8B_CFG_2         0x6338
#else
#define     SLC_TCP8A_CFG_0         0x6160
#define     SLC_TCP8A_CFG_1         0x6164
#define     SLC_TCP8B_CFG_0         0x6168
#define     SLC_TCP8B_CFG_1         0x616C
#endif
#define     SLC_TCP9_CFG_0          0x6170
#define     SLC_TCP9_CFG_1          0x6174
#define     SLC_TCP10_CFG_0         0x6178
#define     SLC_DCP03_CFG_0         0x617C
#define     SLC_DCP6_CFG_0          0x6180
#define     SLC_DCP7_CFG_0          0x6184
#define     SLC_DCP8_CFG_0          0x6188
#define     SLC_DCP9_CFG_0          0x618C
#define     SLC_PMP_CFG_0           0x6190
#define     SLC_PMU_CNT0_L          0x6194
#define     SLC_PMU_CNT0_H          0x6198
#define     SLC_PMU_CNT1_L          0x619C
#define     SLC_PMU_CNT1_H          0x61A0
#define     SLC_PMU_CNT2_L          0x61A4
#define     SLC_PMU_CNT2_H          0x61A8
#define     SLC_PMU_CNT3_L          0x61AC
#define     SLC_PMU_CNT3_H          0x61B0
#define     SLC_PMU_CNT4_L          0x61B4
#define     SLC_PMU_CNT4_H          0x61B8
#define     SLC_PMU_CNT5_L          0x61BC
#define     SLC_PMU_CNT5_H          0x61C0
#define     SLC_PMU_CNT6_L          0x61C4
#define     SLC_PMU_CNT6_H          0x61C8
#define     SLC_PMU_CNT7_L          0x61CC
#define     SLC_PMU_CNT7_H          0x61D0
#define     SLC_PMU_CNT8_L          0x61D4
#define     SLC_PMU_CNT8_H          0x61D8
#define     SLC_PMU_CNT9_L          0x61DC
#define     SLC_PMU_CNT9_H          0x61E0
#define     SLC_PMU_CNT10_L         0x61E4
#define     SLC_PMU_CNT10_H         0x61E8
#define     SLC_PMU_CNT11_L         0x61EC
#define     SLC_PMU_CNT11_H         0x61F0
#define     SLC_PMU_CNT12_L         0x61F4
#define     SLC_PMU_CNT12_H         0x61F8
#define     SLC_PMU_CNT13_L         0x61FC
#define     SLC_PMU_CNT13_H         0x6200
#define     SLC_PMU_CNT14_L         0x6204
#define     SLC_PMU_CNT14_H         0x6208
#define     SLC_PMU_CNT15_L         0x620C
#define     SLC_PMU_CNT15_H         0x6210
#define     SLC_PMU_STAT_CYCLE_L    0x6214
#define     SLC_PMU_STAT_CYCLE_H    0x6218
#define     SLC_PMU_STAT_BUSY_L     0x621C
#define     SLC_PMU_STAT_BUSY_H     0x6220
#define     SLC_WAY_PWRUP_STAT      0x8000
#define     SLC_GID_COUNT           0x8010
#define     SLC02_INT_ST            0x8080
#define     SLC02_INT_INI           0x8084
#define     SLC02_INT_CLEAR         0x8088
#define     SLC02_INT_EN            0x8090
#define     SLC13_INT_ST            0x8080
#define     SLC13_INT_INI           0x8084
#define     SLC13_INT_CLEAR         0x8088
#define     SLC13_INT_EN            0x8090

#ifdef CONFIG_MM_LB_REMAP
#define     GLB_FLOW_STAT           0x8108
#define     GLB_FLOW_STAT_IN        0x8204
#define     GLB_FLOW_STAT_OUT       0x8208
#endif

#define     SLC_DFX_MODULE          0x8400
#define     SLC_DFX_SCQ             0x8404
#define     SLC_DFX_SQUE0           0x8410
#define     SLC_DFX_REQ0            0x8418
#define     SLC_DFX_BUF             0x8420
#define     SLC_DFX_OT              0x8420
#define     SLC_DFX_FIFO0           0x8430
#define     SLC_DFX_FIFO1           0x8434
#define     SLC_DFX_BP              0x8440

#define     SLC_DFX_ERR_INF1        0x8454
#define     SLC_DFX_ERR_INF2        0x8458
#define     SLC_DFX_ECCERR          0x8460
#define     SLC_DFX_MEM0            0x8470
#define     SLC_DFX_MEM1            0x8474
#define     SLC_DFX_SCH_CTRL        0x8480
#define     SLC_DFX_SCH_INFO0       0x8484
#define     SLC_DFX_SCH_INFO1       0x8488
#define     SLC_DFX_SCH_INFO2       0x848C
#define     SLC_DFX_SCH_INFO3       0x8490
#define     SLC_DFX_SCH_INFO4       0x8494
#define     SLC_DFX_CMOGEN          0x84E0
#define     SLC_DFX_DUMMY0          0x8500
#define     SLC_DFX_DUMMY1          0x8504
#ifdef CONFIG_MM_LB_GEMINI
#define     SLC_PMU_CTRL            0x8508
#endif

#define     SLC_WAY_PWRUP           0x9000

#define SLC_ADDR800(addr, slc_id) \
	((addr) + (slc_id) * 0x800)
#define SLC_ADDR2000(addr, slc_id) \
	((addr) + (slc_id) * 0x2000)

#define SLC_ADDR400(addr, slc_id) \
	((addr) + (slc_id) * 0x400)

#define SLC_ADDR4(addr, slc_id) \
	((addr) + (slc_id) * 0x4)
#define SLC_GID_ADDR4(addr, slc_id, id) \
	((addr) + (slc_id) * 0x2000 + (id) * 0x4)
#define GID_ADDR4(addr, id) \
	((addr) + (id) * 0x4)
#define GID_ADDR100(addr, id) \
	((addr) + (id) * 0x100)

#define GID_IDX   16
#define SLC_IDX   4
#define CMO_ST    8
#define SLC_STEP  2

#ifndef BIT
#define BIT(n)                   (0x1U << (n))
#endif

#define MAX_JOB_CNT   16
#define CLASS_CNT     2

#define LB_ERROR  1
#define LB_WARN   3
#define LB_INFO   5

static int lb_d_lvl = LB_WARN;

#define lb_print(level, x...)                          \
	do {                                           \
		if (lb_d_lvl >= (level))               \
			pr_err(x);                     \
	} while (0)

typedef enum {
	EVENT = 0,
	INTR = 1,
} sync_type;

typedef enum {
	INV = 0,
	CLEAN = 2,
	CI = 4,
} ops_type;

typedef enum {
	CMO_BY_WAY = 0,
	CMO_BY_GID,
	CMO_BY_64PA,
	CMO_BY_128PA,
	CMO_BY_4XKPA,
} cmo_type;

enum {
	POWERDOWN,
	POWERUP,
	PARTPOWERDOWN,
	PARTPOWERUP,
	GID_ENABLE,
	GID_BYPASS,
	SEC_GID_REQUEST,
	SEC_GID_RELEASE,
};

enum {
	STATUS_DISABLE,
	STATUS_ENABLE,
	STATUS_BYPASS,
};

enum {
	MASTER_PRIO,
	MASTER_QUOTA,
	MASTER_NUM,
};

typedef union {
	u64 value;
	union {
		struct {
			u64 opt : 4;
			u64 cmdt : 4;
			u64 way_bitmap : 16;
			u64 res : 40;
		} by_way;

		struct {
			u64 opt : 4;
			u64 cmdt : 4;
			u64 gid_bitmap : 16;
			u64 res : 40;
		} by_gid;
		struct {
			u64 opt : 4;
			u64 cmdt : 4;
			u64 res : 2;
			u64 pa : 30;
		} by_64pa;

		struct {
			u64 opt : 4;
			u64 cmdt : 4;
			u64 res0 : 3;
			u64 pa : 29;
		} by_128pa;

		struct {
			u64 opt : 4;
			u64 cmdt : 4;
			u64 spa : 24;
			u64 epa : 24;
		} by_4xkpa;

		struct {
			u64 opt : 4;
			u64 cmdt : 4;
			u64 rslt : 4;
			u64 id : 16;
			u64 seq : 10;
		} sync;
	} param;
} cmd;

typedef union {
	u64 value;
	struct {
		u64 finished : 1;
		u64 res0 : 2;
		u64 sque_nr : 10;
		u64 res1 : 51;
	} stat;
} sync_stat;

typedef union {
	u64 value;
	struct {
		u64 res0 : 28;
		u64 sque_nr : 10;
		u64 res1 : 26;
	} clear;
} sync_clear;

typedef union {
	u32 value;
	union {
		struct {
			u32 mid_flt0 : 7;
			u32 res0 : 9;
			u32 mid_flt0_msk : 7;
			u32 res1 : 8;
			u32 mid_flt0_en : 1;
		} gid_mid_flt0;

		struct {
			u32 mid_flt1 : 7;
			u32 res0 : 9;
			u32 mid_flt1_msk : 7;
			u32 res1 : 8;
			u32 mid_flt1_en : 1;
		} gid_mid_flt1;

		struct {
			u32 nor_r_plc : 2;
			u32 nor_w_plc : 2;
			u32 flt_r_plc : 2;
			u32 flt_w_plc : 2;
			u32 sam_rplc  : 1;
			u32 gid_prpty : 2;
			u32 gid_en : 1;
			u32 res : 20;
		} gid_allc_plc;

		struct {
			u32 gid_way_en : 16;
			u32 gid_way_srch : 16;
		} gid_way_allc;

		struct {
			u32 quota_l : 16;
			u32 quota_h : 16;
		} gid_quota;
	} reg;
} gid;

struct lb_area {
	struct list_head next;
	struct vm_struct *area;
};

struct vm_info {
	struct list_head list;
	struct mutex mutex;
	struct gen_pool *pool;
};

struct lb_plc_info {
	const char *name;
	unsigned int prio;
	unsigned int r_prio;
	unsigned int pid;
	unsigned int stat;
	unsigned int gid;
	unsigned int quota;
	unsigned int r_quota;
	unsigned int flowid;
	unsigned int alloc;
	unsigned int plc;
	unsigned int enabled;
	unsigned long page_count;
	struct vm_info *vm;
	spinlock_t lock;
};

struct gid_policy {
	int nr;
	struct lb_plc_info *ptbl;

	unsigned int gidmap;
	unsigned int max_quota;
	unsigned int max_size;
	unsigned int is_support_bypass;
	unsigned int job_queue[MAX_JOB_CNT][CLASS_CNT];
	int job_cnt;
	spinlock_t lock;
};

struct dsm_info {
	struct dsm_client *ai_client;
	struct work_struct dsm_work;
};

struct lb_device {
	struct device *dev;
	void __iomem  *base;
	int dfx_irq;
	int cmd_irq;
	unsigned int power_state;
	unsigned int is_available;
	unsigned int way_efuse;
	struct gid_policy gdplc;
	struct dsm_info dsm;
};

#define SC_POLL_TIMEOUT_US	1000000

#define DEFINE_INIT_QUOTA(quota, l, h)                                         \
	do {                                                                   \
		(quota).value = 0;                                             \
		(quota).reg.gid_quota.quota_l = (l);                           \
		(quota).reg.gid_quota.quota_h = (h);                           \
	} while (0)

#define DEFINE_INIT_ALLOC(allc, way_en, way_srch_en)                           \
	do {                                                                   \
		(allc).value = 0;                                              \
		(allc).reg.gid_way_allc.gid_way_en = (way_en);                 \
		(allc).reg.gid_way_allc.gid_way_srch = (way_srch_en);          \
	} while (0)

#define DEFINE_INIT_PLC(plc, rd, wr, flt_rd, flt_wr, rplc_sam_en, priopty, en) \
	do {                                                                   \
		(plc).value = 0;                                               \
		(plc).reg.gid_allc_plc.nor_r_plc = (rd);                       \
		(plc).reg.gid_allc_plc.nor_w_plc = (wr);                       \
		(plc).reg.gid_allc_plc.flt_r_plc = (flt_rd);                   \
		(plc).reg.gid_allc_plc.flt_w_plc = (flt_wr);                   \
		(plc).reg.gid_allc_plc.sam_rplc = (rplc_sam_en);               \
		(plc).reg.gid_allc_plc.gid_prpty = (priopty);                  \
		(plc).reg.gid_allc_plc.gid_en = (en);                          \
	} while (0)

#define DEFINE_INIT_FLT0(flt0, mid, mask, en)                                  \
	do {                                                                   \
		(flt0).value = 0;                                              \
		(flt0).reg.gid_mid_flt0.mid_flt0 = (mid);                      \
		(flt0).reg.gid_mid_flt0.mid_flt0_msk = (mask);                 \
		(flt0).reg.gid_mid_flt0.mid_flt0_en = (en);                    \
	} while (0)

#define DEFINE_INIT_FLT1(flt1, mid, mask, en)                                  \
	do {                                                                   \
		(flt1).value = 0;                                              \
		(flt1).reg.gid_mid_flt1.mid_flt0 = (mid);                      \
		(flt1).reg.gid_mid_flt1.mid_flt0_msk = (mask);                 \
		(flt1).reg.gid_mid_flt1.mid_flt0_en = (en);                    \
	} while (0)

extern struct lb_device *lbdev;
extern void lb_cmo_sync_cmd_by_event(void);
extern int lb_ops_cache(sync_type synct, ops_type ops, cmo_type by_x,
	u32 bitmap, u64 pa, size_t sz);

#ifdef CONFIG_MM_LB_PMU
int lb_pmu_init(struct platform_device *pdev, struct lb_device *lbd);
#else
static inline int lb_pmu_init(struct platform_device *pdev,
	struct lb_device *lbd) { return 0; }
#endif

#endif
