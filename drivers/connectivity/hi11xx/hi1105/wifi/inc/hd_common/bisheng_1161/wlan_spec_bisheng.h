

#ifndef __WLAN_SPEC_BISHENG_H__
#define __WLAN_SPEC_BISHENG_H__

/* BISHENG 规格待确认 */
/* 作为P2P GO 允许关联最大用户数 */
#define WLAN_P2P_GO_ASSOC_USER_MAX_NUM_BISHENG 8


/* 支持的建立rx ba 的最大个数 */
#define WLAN_MAX_RX_BA_BISHENG 32

/* 支持的建立tx ba 的最大个数 */
#define WLAN_MAX_TX_BA_BISHENG 1024

/* bisheng 16个VAP只最后4个(12~15)支持STA和AP模式 */
#define BISHENG_HAL_VAP_OFFSET 12
#define BISHENG_OTHER_BSS_ID 0xFF

/* rx cb vap id 5bit 全1表示其他BSS */
#define WLAN_HAL_OHTER_BSS_ID_BISHENG 0x1F

/* tx最大ampdu聚合规格 */
#define WLAN_AMPDU_TX_MAX_NUM_BISHENG 256

#endif /* #ifndef __WLAN_SPEC_BISHENG_H__ */

