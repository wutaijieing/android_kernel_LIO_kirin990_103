

#ifndef __WLAN_SPEC_SHENKUO_H__
#define __WLAN_SPEC_SHENKUO_H__


/* 作为P2P GO 允许关联最大用户数 */
#define WLAN_P2P_GO_ASSOC_USER_MAX_NUM_SHENKUO 8


/* 支持的建立rx ba 的最大个数 */
#define WLAN_MAX_RX_BA_SHENKUO 32

/* 支持的建立tx ba 的最大个数 */
#define WLAN_MAX_TX_BA_SHENKUO 1024

/* SHENKUO 16个VAP只最后4个支持STA和AP模式 */
#define SHENKUO_HAL_VAP_OFFSET 12
#define SHENKUO_OTHER_BSS_ID 0xFF

/* SHENKUO rx cb vap id 5bit 全1表示其他bSS */
#define WLAN_HAL_OHTER_BSS_ID_SHENKUO 0x1F

/* SHENKUO tx最大ampdu聚合规格 */
#define WLAN_AMPDU_TX_MAX_NUM_SHENKUO 256

#endif /* #ifndef __WLAN_SPEC_SHENKUO_H__ */

