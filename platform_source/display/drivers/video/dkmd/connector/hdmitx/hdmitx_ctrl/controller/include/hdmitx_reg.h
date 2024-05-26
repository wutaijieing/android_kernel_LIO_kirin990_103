/*
 * Copyright (c) CompanyNameMagicTag. 2019-2019. All rights reserved.
 * Description: hdmi driver hdmi tx controller header file
 * Author: AuthorNameMagicTag
 * Create: 2019-06-22
 */

#ifndef __HDMITX_REG_H__
#define __HDMITX_REG_H__

#define MAX_SUB_PKT_NUM 4
#define MAX_DSC_EMP_NUM 6

/* tx_hdmi_reg module field info */
#define REG_AVI_PKT_HEADER 0x1818
#define REG_AVI_SUB_PKT0_L 0x181C
#define REG_AVI_SUB_PKT0_H 0x1820

#define REG_AIF_PKT_HEADER 0x183C
#define REG_AIF_SUB_PKT0_L 0x1840
#define REG_AIF_SUB_PKT0_H 0x1844

#define REG_SPIF_PKT_HEADER 0x1860
#define REG_SPIF_SUB_PKT0_L 0x1864
#define REG_SPIF_SUB_PKT0_H 0x1868

#define REG_MPEG_PKT_HEADER 0x1884
#define REG_MPEG_SUB_PKT0_L 0x1888
#define REG_MPEG_SUB_PKT0_H 0x188C

#define REG_GEN_PKT_HEADER 0x18A8
#define REG_GEN_SUB_PKT0_L 0x18AC
#define REG_GEN_SUB_PKT0_H 0x18B0

#define REG_GEN2_PKT_HEADER 0x18CC
#define REG_GEN2_SUB_PKT0_L 0x18D0
#define REG_GEN2_SUB_PKT0_H 0x18D4

#define REG_GEN3_PKT_HEADER 0x18F0
#define REG_GEN3_SUB_PKT0_L 0x18F4
#define REG_GEN3_SUB_PKT0_H 0x18F8

#define REG_GEN4_PKT_HEADER 0x1914
#define REG_GEN4_SUB_PKT0_L 0x1918
#define REG_GEN4_SUB_PKT0_H 0x191C

#define REG_GEN5_PKT_HEADER 0x1938
#define REG_GEN5_SUB_PKT0_L 0x193C
#define REG_GEN5_SUB_PKT0_H 0x1940

#define REG_GAMUT_PKT_HEADER 0x195C
#define REG_GAMUT_SUB_PKT0_L 0x1960
#define REG_GAMUT_SUB_PKT0_H 0x1964

#define REG_VSIF_PKT_HEADER 0x1980
#define REG_VSIF_SUB_PKT0_L 0x1984
#define REG_VSIF_SUB_PKT0_H 0x1988

#define reg_sub_pkt_hb2(x) (((x) & 0xff) << 16)
#define reg_sub_pkt_hb1(x) (((x) & 0xff) << 8)
#define reg_sub_pkt_hb0(x) (((x) & 0xff) << 0)

#define reg_sub_pktx_pb3(x) (((x) & 0xff) << 24)
#define reg_sub_pktx_pb2(x) (((x) & 0xff) << 16)
#define reg_sub_pktx_pb1(x) (((x) & 0xff) << 8)
#define reg_sub_pktx_pb0(x) (((x) & 0xff) << 0)

#define reg_sub_pktx_pb6(x) (((x) & 0xff) << 16)
#define reg_sub_pktx_pb5(x) (((x) & 0xff) << 8)
#define reg_sub_pktx_pb4(x) (((x) & 0xff) << 0)

#define REG_CEA_AVI_CFG   0x19A4
#define REG_CEA_SPF_CFG   0x19A8
#define REG_CEA_AUD_CFG   0x19AC
#define REG_CEA_MPEG_CFG  0x19B0
#define REG_CEA_GEN_CFG   0x19B4
#define REG_CEA_CP_CFG    0x19B8
#define REG_CEA_GEN2_CFG  0x19BC
#define REG_CEA_GEN3_CFG  0x19C0
#define REG_CEA_GEN4_CFG  0x19C4
#define REG_CEA_GEN5_CFG  0x19C8
#define REG_CEA_GAMUT_CFG 0x19CC
#define REG_CEA_VSIF_CFG  0x19D0


#define REG_AVMIXER_CONFIG         0x1A08
#define reg_cfg_eess_mode_en(x)    (((x) & 1) << 12)
#define REG_CFG_EESS_MODE_EN_M     (1 << 12)
#define reg_cfg_hdmi_dvi_sel(x)    (((x) & 1) << 11)
#define REG_CFG_HDMI_DVI_SEL_M     (1 << 11)
#define reg_cfg_frl_mode(x)        (((x) & 1) << 10)
#define REG_CFG_FRL_MODE_M         (1 << 10)
#define reg_audio_prioroty_ctl(x)  (((x) & 1) << 9)
#define REG_AUDIO_PRIORITY_CTL_M   (1 << 9)
#define reg_avmute_in_phase(x)     (((x) & 1) << 8)
#define REG_AVMUTE_IN_PHASE_M      (1 << 8)
#define reg_pkt_bypass_mode(x)     (((x) & 1) << 7)
#define REG_PKT_BYPASS_MODE_M      (1 << 7)
#define reg_pb_priority_ctl(x)     (((x) & 1) << 6)
#define REG_PB_PRIOTITY_CTL_M      (1 << 6)
#define reg_pb_ovr_dc_pkt_en(x)    (((x) & 1) << 5)
#define REG_PB_OVR_DC_PKT_EN_M     (1 << 5)
#define reg_intr_encryption(x)     (((x) & 1) << 4)
#define REG_INTR_ENCRYPTION_M      (1 << 4)
#define reg_null_pkt_en_vs_high(x) (((x) & 1) << 3)
#define REG_NULL_PKT_EN_VS_HIGH_M  (1 << 3)
#define reg_null_pkt_en(x)         (((x) & 1) << 2)
#define REG_NULL_PKT_EN_M          (1 << 2)
#define reg_dc_pkt_en(x)           (((x) & 1) << 1)
#define REG_DC_PKT_EN_M            (1 << 1)
#define reg_hdmi_mode(x)           (((x) & 1) << 0)
#define REG_HDMI_MODE_M            (1 << 0)

#define reg_rpt_cnt(x) (((x) & 0x3ff) << 2)
#define REG_RPT_CNT_M  (0x3ff << 2)
#define reg_rpt_en(x)  (((x) & 1) << 1)
#define REG_RPT_EN_M   (1 << 1)
#define reg_en(x)      (((x) & 1) << 0)
#define REG_EN_M       (1 << 0)

#define REG_CP_PKT_AVMUTE    0x1A0C
#define reg_cp_clr_avmute(x) (((x) & 1) << 1)
#define REG_CP_CLR_AVMUTE_M  (1 << 1)
#define reg_cp_set_avmute(x) (((x) & 1) << 0)
#define REG_CP_SET_AVMUTE_M  (1 << 0)

#define REG_TX_METADATA_CTRL_ARST_REQ 0x1B9C
#define reg_tx_metadata_arst_req(x)   (((x) & 1) << 0)
#define REG_TX_METADATA_ARST_REQ_M    (1 << 0)

#define REG_TX_METADATA_CTRL      0x1BA0
#define reg_txmeta_vdp_path_en(x) (((x) & 1) << 25)
#define REG_TXMETA_VDP_PATH_EN_M  (1 << 25)

#define reg_txmeta_vdp_avi_en(x) (((x) & 1) << 23)
#define REG_TXMETA_VDP_AVI_EN_M  (1 << 23)

#endif

