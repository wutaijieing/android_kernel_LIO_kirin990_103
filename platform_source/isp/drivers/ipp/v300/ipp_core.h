/*
 * Hisilicon IPP Head File
 *
 * Copyright (c) 2018 Hisilicon Technologies CO., Ltd.
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

#define HIPP_PD_CHECK 0
#define HIPP_PD_FORCE 1

enum HISP_CPE_IRQ_TYPE {
	CPE_IRQ_0 = 0,		// ACPU
	MAX_HISP_CPE_IRQ
};

enum HIPP_CLK_STATUS {
	IPP_CLK_DISABLE = 0,
	IPP_CLK_EN = 1,
	IPP_CLK_STATUS_MAX
};

enum hipp_wait_mode_type_e {
	WAIT_MODE_GF = 0,
	WAIT_MODE_ORB = 1,
	WAIT_MODE_RDR = 2,
	WAIT_MODE_CMP = 3,
	WAIT_MODE_VBK = 4,
	WAIT_MODE_ENH = 5,
	WAIT_MODE_MC = 6,
	MAX_WAIT_MODE_TYPE
};

struct hispcpe_s {
	struct miscdevice miscdev;
	struct platform_device *pdev;
	int initialized;
	struct wakeup_source *ipp_wakelock;
	struct mutex ipp_wakelock_mutex;

	struct mutex open_lock;
	unsigned int open_refs;

	struct mutex cvdr_lock;

	unsigned int irq_num;
	unsigned int reg_num;
	int irq[MAX_HISP_CPE_IRQ];
	struct resource *r[MAX_HISP_CPE_REG];
	void __iomem *reg[MAX_HISP_CPE_REG];

	unsigned int sid;
	unsigned int ssid;
};

struct hipp_adapter_s {
	wait_queue_head_t gf_wait;
	wait_queue_head_t vbk_wait;
	wait_queue_head_t orb_wait;
	wait_queue_head_t reorder_wait;
	wait_queue_head_t compare_wait;
	wait_queue_head_t orb_enh_wait;
	wait_queue_head_t mc_wait;
	int gf_ready;
	int vbk_ready;
	int orb_ready;
	int reorder_ready;
	int compare_ready;
	int orb_enh_ready;
	int mc_ready;
	int irq;

	unsigned int curr_cpe_rate_value[CLK_RATE_INDEX_MAX];

	struct mutex dev_lock;
	unsigned int  hipp_refs[REFS_TYP_MAX];
	int refs_power_up;
	unsigned int mapbuf_ready;

	unsigned long daddr;
	void *virt_addr;
	struct dma_buf *dmabuf;

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

#define HISP_IPP_PWRUP          _IOWR('C', 0x1001, int)
#define HISP_IPP_PWRDN          _IOWR('C', 0x1002, int)
#define HISP_IPP_CFG_CHECK      _IOWR('C', 0x1003, int)
#define HISP_IPP_GF_REQ         _IOWR('C', 0x2001, int)
#define HISP_IPP_ORB_REQ        _IOWR('C', 0x2005, int)
#ifdef CONFIG_IPP_DEBUG
#define HISP_REORDER_REQ        _IOWR('C', 0x2007, int)
#define HISP_COMPARE_REQ        _IOWR('C', 0x2008, int)
#endif
#define HISP_IPP_VBK_REQ        _IOWR('C', 0x2009, int)
#define HISP_CPE_MAP_IOMMU      _IOWR('C', 0x200A, int)
#define HISP_CPE_UNMAP_IOMMU    _IOWR('C', 0x200B, int)
#define HISP_IPP_MAP_BUF        _IOWR('C', 0x200C, int)
#define HISP_IPP_UNMAP_BUF      _IOWR('C', 0x200D, int)
#define HISP_ANF_REQ            _IOWR('C', 0x200E, int)

#define DTS_NAME_HISI_IPP "hisilicon,ipp"
#ifdef CONFIG_IPP_DEBUG
#define HISP_CPE_TIMEOUT_MS (200)
#else
#define HISP_CPE_TIMEOUT_MS 200
#endif
#define hipp_min(a, b) (((a) < (b)) ? (a) : (b))

int hipp_adapter_cfg_qos_cvdr(void);
int hipp_powerup(void);
int hipp_powerdn(int pd_flag);
int hipp_cmd_gf_req(unsigned long args);
int hipp_cmd_vbk_req(unsigned long args);
int hipp_cmd_orb_req(unsigned long args);
int reorder_process(unsigned long args);
int compare_process(unsigned long args);
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
