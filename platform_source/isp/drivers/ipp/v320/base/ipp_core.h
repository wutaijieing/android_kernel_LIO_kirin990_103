/*
 * IPP Head File
 *
 * Copyright (c) 2018 IPP Technologies CO., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _HIPP_CORE_H_
#define _HIPP_CORE_H_


#define ORB_RATE_INDEX_MAX 3
#define IPP_SEC_ENABLE 0
#define IPPCORE_SWID_INDEX0 0
#define IPPCORE_SWID_LEN0 34
#define IPPCORE_SWID_INDEX1 42
#define IPPCORE_SWID_LEN1 5


enum HISP_CPE_IRQ_TYPE {
	CPE_IRQ_0 = 0,		// ACPU
	MAX_HISP_CPE_IRQ
};

enum HIPP_CLK_TYPE {
	IPPCORE_CLK = 0,
	IPP_CLK_MAX
};

enum HIPP_CLK_STATUS {
	IPP_CLK_DISABLE = 0,
	IPP_CLK_EN = 1,
	IPP_CLK_STATUS_MAX
};

enum hipp_wait_mode_type_e {
	WAIT_MODE_GF = 0,
	WAIT_MODE_ORB = 1,
	MAX_WAIT_MODE_TYPE
};

enum hipp_clk_rate_type_e {
	HIPPCORE_RATE_TBR = 0,
	HIPPCORE_RATE_NOR = 1,
	HIPPCORE_RATE_HSVS = 2,
	HIPPCORE_RATE_SVS = 3,
	HIPPCORE_RATE_OFF = 4,
	MAX_HIPP_CLKRATE_TYPE
};

struct hispcpe_s {
	struct miscdevice miscdev;
	struct platform_device *pdev;
	int initialized;
	atomic_t open_refs;
	struct regulator *media2_supply;
	struct regulator *cpe_supply;
	struct regulator *smmu_tcu_supply;
	struct wakeup_source *ipp_wakelock;
	struct mutex ipp_wakelock_mutex;

	struct mutex dev_lock;

	unsigned int irq_num;
	unsigned int reg_num;
	int irq[MAX_HISP_CPE_IRQ];
	struct resource *r[MAX_HISP_CPE_REG];
	void __iomem *reg[MAX_HISP_CPE_REG];

	unsigned int clock_num;
	struct clk *ippclk[IPP_CLK_MAX];
	unsigned int ippclk_value[IPP_CLK_MAX];
	unsigned int ippclk_normal[IPP_CLK_MAX];
	unsigned int ippclk_hsvs[IPP_CLK_MAX];
	unsigned int ippclk_svs[IPP_CLK_MAX];
	unsigned int ippclk_off[IPP_CLK_MAX];
	unsigned int jpeg_current_rate;
	unsigned int jpeg_rate[MAX_HIPP_COM];
	unsigned int sid;
	unsigned int ssid;
};

struct hipp_adapter_s {
	wait_queue_head_t gf_wait;
	wait_queue_head_t orb_wait;
	int gf_ready;
	int orb_ready;
	unsigned int hipp_refs[REFS_TYP_MAX];
	atomic_t mapbuf_ready;
	int irq;

	unsigned int curr_cpe_rate_value[CLK_RATE_INDEX_MAX];

	struct mutex ipp_power_lock;
	struct mutex ipp_work_lock;
	int refs_power_up;

	int shared_fd;
	unsigned long daddr;
	void *virt_addr;

	struct hipp_common_s *ippdrv;
};

struct pw_memory_s {
	int shared_fd;
	int size;
	unsigned long prot;
};
struct memory_block_s {
	int shared_fd;
	int size;
	unsigned long prot;
	unsigned int da;
	int usage;
	void *viraddr;
};

struct map_buff_block_s {
	int32_t secure;
	int32_t shared_fd;
	int32_t size;
	u_int64_t prot;
};

struct ipp_cfg_s {
	uint32_t platform_version;
	uint32_t mapbuffer;
	uint32_t mapbuffer_sec;
};

struct power_para_s {
	unsigned int pw_flag;
	struct pw_memory_s mem;
};

struct transition_info {
	unsigned int source_rate;
	unsigned int transition_rate;
};

#define HIPP_TRAN_NUM 4

static struct transition_info jpeg_trans_rate[HIPP_TRAN_NUM] = {
	{ 640000000, 214000000},
	{ 480000000, 160000000},
	{ 400000000, 200000000},
};

#define HISP_IPP_PWRUP          _IOWR('C', 0x1001, int)
#define HISP_IPP_PWRDN          _IOWR('C', 0x1002, int)
#define HISP_IPP_CFG_CHECK      _IOWR('C', 0x1003, int)
#define HISP_IPP_GF_REQ         _IOWR('C', 0x2001, int)
#define HISP_IPP_ORB_REQ        _IOWR('C', 0x2005, int)
#define HISP_CPE_MAP_IOMMU      _IOWR('C', 0x200A, int)
#define HISP_CPE_UNMAP_IOMMU    _IOWR('C', 0x200B, int)
#define HISP_IPP_MAP_BUF        _IOWR('C', 0x200C, int)
#define HISP_IPP_UNMAP_BUF      _IOWR('C', 0x200D, int)

#define DTS_NAME_IPP "ipp"
#ifdef CONFIG_IPP_DEBUG
#define HISP_CPE_TIMEOUT_MS (200)
#else
#define HISP_CPE_TIMEOUT_MS 200
#endif
#define hipp_min(a, b) (((a) < (b)) ? (a) : (b))

unsigned int get_clk_rate(unsigned int mode, unsigned int index);


int hipp_adapter_cfg_qos_cvdr(void);
int hipp_powerup(void);
int hipp_powerdn(void);
int hipp_cmd_gf_req(unsigned long args);
int hipp_cmd_orb_req(unsigned long args);
irqreturn_t hispcpe_irq_handler(int irq, void *devid);

void __iomem *hipp_get_regaddr(unsigned int type);
int is_hipp_mapbuf_ready(void);
int get_hipp_smmu_info(int *sid, int *ssid);

int hispcpe_map_kernel(unsigned long args);
int hispcpe_unmap_kernel(void);
int hipp_adapter_map_iommu(struct memory_block_s *frame_buf);
int hipp_adapter_unmap_iommu(struct memory_block_s *frame_buf);

int hipp_adapter_probe(struct platform_device *pdev);
void hipp_adapter_remove(void);
int hipp_adapter_register_irq(int irq);
void hipp_adapter_exception(void);
#endif
