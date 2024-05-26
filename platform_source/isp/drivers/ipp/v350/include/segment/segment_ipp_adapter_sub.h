/******************************************************************
 * Copyright    Copyright (c) 2020- Hisilicon Technologies CO., Ltd.
 * File name    segment_ipp_adapter_sub.h
 * Description:
 *
 * Date         2020-04-17 16:28:10
 ********************************************************************/
#ifndef __SEGMENT_IPP_ADAPTER_SUB_H__
#define __SEGMENT_IPP_ADAPTER_SUB_H__

#include "segment_matcher.h"
#include "segment_mc.h"
#include "segment_orb_enh.h"

int orb_enh_process(unsigned long args);
int arfeature_process(unsigned long args);
int reorder_process(unsigned long args);
int compare_process(unsigned long args);
int mc_process(unsigned long args);

int hipp_orcm_clk_enable(void);
int hipp_orcm_clk_disable(void);
int hispcpe_gf_clk_enable(void);
int hispcpe_gf_clk_disable(void);
int hispcpe_hiof_clk_enable(void);
int hispcpe_hiof_clk_disable(void);
int hispcpe_matcher_request(struct hipp_adapter_s *dev, struct msg_req_matcher_t *ctrl);
int hispcpe_mc_request(struct hipp_adapter_s *dev, struct msg_req_mc_request_t *ctrl);
int hispcpe_orb_enh_request(struct hipp_adapter_s *dev, struct msg_enh_req_t *req_enh);

#endif // __SEGMENT_IPP_ADAPTER_SUB_H__
// end
