

#ifndef __WLAN_SPEC_BISHENG_H__
#define __WLAN_SPEC_BISHENG_H__

/* BISHENG ����ȷ�� */
/* ��ΪP2P GO �����������û��� */
#define WLAN_P2P_GO_ASSOC_USER_MAX_NUM_BISHENG 8


/* ֧�ֵĽ���rx ba �������� */
#define WLAN_MAX_RX_BA_BISHENG 32

/* ֧�ֵĽ���tx ba �������� */
#define WLAN_MAX_TX_BA_BISHENG 1024

/* bisheng 16��VAPֻ���4��(12~15)֧��STA��APģʽ */
#define BISHENG_HAL_VAP_OFFSET 12
#define BISHENG_OTHER_BSS_ID 0xFF

/* rx cb vap id 5bit ȫ1��ʾ����BSS */
#define WLAN_HAL_OHTER_BSS_ID_BISHENG 0x1F

/* tx���ampdu�ۺϹ�� */
#define WLAN_AMPDU_TX_MAX_NUM_BISHENG 256

#endif /* #ifndef __WLAN_SPEC_BISHENG_H__ */

