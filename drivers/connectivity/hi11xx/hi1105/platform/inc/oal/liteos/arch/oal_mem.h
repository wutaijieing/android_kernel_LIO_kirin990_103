

#ifndef __OAL_LITEOS_MEM_H__
#define __OAL_LITEOS_MEM_H__

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif


#define OAL_TX_CB_LEN 48
#define OAL_MAX_CB_LEN  19
#define MAX_MAC_HEAD_LEN            WLAN_MAX_MAC_HDR_LEN

/* 对于enhanced类型的申请接口与释放接口，每一个内存块都包含一个4字节的头部， */
/* 用来指向内存块管理结构体oal_mem_struc，整个内存块的结构如下所示。           */
/*                                                                           */
/* +-------------------+------------------------------------------+---------+ */
/* | oal_mem_stru addr |                    payload               | dog tag | */
/* +-------------------+------------------------------------------+---------+ */
/* |      4/8 byte       |                                          | 4 byte  | */
/* +-------------------+------------------------------------------+---------+ */
#ifdef CONFIG_ARM64
/* when 64 os, c pointer take 8 bytes */
#define OAL_MEM_INFO_SIZE            8
#else
#define OAL_MEM_INFO_SIZE            4
#endif
#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif /* end of oal_mm.h */

