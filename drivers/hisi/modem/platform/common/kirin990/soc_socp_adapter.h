#ifndef __SOC_SOCP_ADAPTER_H__
#define __SOC_SOCP_ADAPTER_H__ 
#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif
#define SOC_SOCP_CODER_SRC_CHAN 0x00
#define SOC_SOCP_CHAN_DEF(chan_type,chan_id) (((chan_type) << 16) | (chan_id))
enum SOCP_CODER_SRC_ENUM {
    SOCP_CODER_SRC_LOM_CNF1 = SOC_SOCP_CHAN_DEF(SOC_SOCP_CODER_SRC_CHAN, 0),
    SOCP_CODER_SRC_BSP_ACORE = SOC_SOCP_CHAN_DEF(SOC_SOCP_CODER_SRC_CHAN, 1),
    SOCP_CODER_SRC_LOM_IND1 = SOC_SOCP_CHAN_DEF(SOC_SOCP_CODER_SRC_CHAN, 2),
    SOCP_CODER_SRC_LOM_IND4 = SOC_SOCP_CHAN_DEF(SOC_SOCP_CODER_SRC_CHAN, 3),
    SOCP_CODER_SRC_LOM_IND2 = SOC_SOCP_CHAN_DEF(SOC_SOCP_CODER_SRC_CHAN, 4),
    SOCP_CODER_SRC_LOM_IND3 = SOC_SOCP_CHAN_DEF(SOC_SOCP_CODER_SRC_CHAN, 5),
    SOCP_CODER_SRC_LOM_CNF2 = SOC_SOCP_CHAN_DEF(SOC_SOCP_CODER_SRC_CHAN, 6),
    SOCP_CODER_SRC_XDSP = SOC_SOCP_CHAN_DEF(SOC_SOCP_CODER_SRC_CHAN, 7),
    SOCP_CODER_SRC_BSP_CCORE = SOC_SOCP_CHAN_DEF(SOC_SOCP_CODER_SRC_CHAN, 8),
    SOCP_CODER_SRC_HIFI = SOC_SOCP_CHAN_DEF(SOC_SOCP_CODER_SRC_CHAN, 9),
    SOCP_CODER_SRC_RSV2 = SOC_SOCP_CHAN_DEF(SOC_SOCP_CODER_SRC_CHAN, 10),
    SOCP_CODER_SRC_AXI_MONITOR = SOC_SOCP_CHAN_DEF(SOC_SOCP_CODER_SRC_CHAN, 11),
    SOCP_CODER_SRC_MCU1 = SOC_SOCP_CHAN_DEF(SOC_SOCP_CODER_SRC_CHAN, 12),
    SOCP_CODER_SRC_RSV3 = SOC_SOCP_CHAN_DEF(SOC_SOCP_CODER_SRC_CHAN, 13),
    SOCP_CODER_SRC_LDSP1 = SOC_SOCP_CHAN_DEF(SOC_SOCP_CODER_SRC_CHAN, 14),
    SOCP_CODER_SRC_LDSP2 = SOC_SOCP_CHAN_DEF(SOC_SOCP_CODER_SRC_CHAN, 15),
    SOCP_CODER_SRC_BBP_LOG = SOC_SOCP_CHAN_DEF(SOC_SOCP_CODER_SRC_CHAN, 16),
    SOCP_CODER_SRC_BBP_DS = SOC_SOCP_CHAN_DEF(SOC_SOCP_CODER_SRC_CHAN, 17),
    SOCP_CODER_SRC_CPROC = SOC_SOCP_CHAN_DEF(SOC_SOCP_CODER_SRC_CHAN, 18),
    SOCP_CODER_SRC_AP_BSP = SOC_SOCP_CHAN_DEF(SOC_SOCP_CODER_SRC_CHAN, 19),
    SOCP_CODER_SRC_AP_APP = SOC_SOCP_CHAN_DEF(SOC_SOCP_CODER_SRC_CHAN, 20),
    SOCP_CODER_SRC_AP_DDR = SOC_SOCP_CHAN_DEF(SOC_SOCP_CODER_SRC_CHAN, 21),
    SOCP_CODER_SRC_ISP = SOC_SOCP_CHAN_DEF(SOC_SOCP_CODER_SRC_CHAN, 22),
    SOCP_CODER_SRC_RSV1 = SOC_SOCP_CHAN_DEF(SOC_SOCP_CODER_SRC_CHAN, 23),
    SOCP_CODER_SRC_TMP = SOC_SOCP_CHAN_DEF(SOC_SOCP_CODER_SRC_CHAN, 24),
    SOCP_CODER_SRC_GUBBP1 = SOC_SOCP_CHAN_DEF(SOC_SOCP_CODER_SRC_CHAN, 25),
    SOCP_CODER_SRC_GUBBP2 = SOC_SOCP_CHAN_DEF(SOC_SOCP_CODER_SRC_CHAN, 26),
    SOCP_CODER_SRC_GUDSP1 = SOC_SOCP_CHAN_DEF(SOC_SOCP_CODER_SRC_CHAN, 27),
    SOCP_CODER_SRC_GUDSP2 = SOC_SOCP_CHAN_DEF(SOC_SOCP_CODER_SRC_CHAN, 28),
    SOCP_CODER_SRC_BBP_BUS = SOC_SOCP_CHAN_DEF(SOC_SOCP_CODER_SRC_CHAN, 29),
    SOCP_CODER_SRC_RSV5 = SOC_SOCP_CHAN_DEF(SOC_SOCP_CODER_SRC_CHAN, 30),
    SOCP_CODER_SRC_RSV6 = SOC_SOCP_CHAN_DEF(SOC_SOCP_CODER_SRC_CHAN, 31),
    SOCP_CODER_SRC_BUTT
};
#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif
#endif