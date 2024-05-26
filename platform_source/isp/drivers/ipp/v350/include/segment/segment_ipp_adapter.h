/******************************************************************
 * Copyright    Copyright (c) 2020- Hisilicon Technologies CO., Ltd.
 * File name    segment_ipp_adapter.h
 * Description:
 *
 * Date         2020-04-17 16:28:10
 ********************************************************************/
#ifndef __SEGMENT_IPP_ADAPTER_H__
#define __SEGMENT_IPP_ADAPTER_H__

enum hipp_wait_mode_type_e {
	WAIT_MODE_GF = 0,
	WAIT_MODE_ARFEATURE = 1,
	WAIT_MODE_RDR = 2,
	WAIT_MODE_CMP = 3,
	WAIT_MODE_HIOF = 4,
	WAIT_MODE_ENH = 5,
	WAIT_MODE_MC = 6,
	MAX_WAIT_MODE_TYPE
};

struct hipp_adapter_s {
	wait_queue_head_t gf_wait;
	wait_queue_head_t hiof_wait;
	wait_queue_head_t arfeature_wait;
	wait_queue_head_t reorder_wait;
	wait_queue_head_t compare_wait;
	wait_queue_head_t orb_enh_wait;
	wait_queue_head_t mc_wait;
	int gf_ready;
	int hiof_ready;
	int arfeature_ready;
	int reorder_ready;
	int compare_ready;
	int orb_enh_ready;
	int mc_ready;
	int irq;

	unsigned int curr_cpe_rate_value[CLK_RATE_INDEX_MAX];
	struct mutex cvdr_lock;

	struct mutex ipp_work_lock;
	unsigned int hipp_refs[REFS_TYP_MAX];

	unsigned long daddr;
	void *virt_addr;
	struct dma_buf *dmabuf;

	struct hipp_common_s *ippdrv;
};

int hispcpe_map_kernel(unsigned long args);
int hispcpe_unmap_kernel(void);
int hipp_powerup(void);
int hipp_powerdn(void);
int hispcpe_work_check(void);
int hipp_path_of_process(unsigned long args);
int gf_process(unsigned long args);
int hiof_process(unsigned long args);
int hipp_adapter_map_iommu(unsigned long args);
int hipp_adapter_unmap_iommu(unsigned long args);
void hipp_adapter_exception(void);

int hispcpe_clean_wait_flag(struct hipp_adapter_s *dev, unsigned int wait_mode);
int hispcpe_wait(struct hipp_adapter_s *dev, enum hipp_wait_mode_type_e wait_mode);

#endif // __SEGMENT_IPP_ADAPTER_H__
// end
