/*
 * Copyright (c) Huawei Technologies Co. Ltd. 2021-2022. All rights reserved.
 * Description: head file used for both efuse svc provider and callers
 * Create: 2022/01/18
 */

#ifndef __EFUSE_KERNEL_DEF_H__
#define __EFUSE_KERNEL_DEF_H__

/* bits allocated to bit_cnt param in a uint32_t */
#define EFUSE_BITCNT_ALLOC_BITS             (16u)

/* 0x00030000 ~ 0x0003FFFF for efuse item define in teeos */
/* item_vid: efuse item virtual id */

/* [3:0] LSB are reserved for more efuse device in the future */
#define EFUSE1_KERNEL_DEBUG_ID              0x00030001u
#define EFUSE2_KERNEL_DEBUG_ID              0x00030002u

#define EFUSE_KERNEL_DIEID_VALUE            0x00030010u
#define EFUSE_KERNEL_PARTIALGOOD_FLAG1      0x00030011u
#define EFUSE_KERNEL_PARTIALGOOD_FLAG2      0x00030012u
#define EFUSE_KERNEL_PARTIALGOOD_FLAG3      0x00030013u
#define EFUSE_KERNEL_PARTIALGOOD_FLAG4      0x00030014u
#define EFUSE_KERNEL_PARTIALGOOD_FLAG5      0x00030015u
#define EFUSE_KERNEL_PARTIALGOOD_FLAG6      0x00030016u
#define EFUSE_KERNEL_PARTIALGOOD_FLAG7      0x00030017u
#define EFUSE_KERNEL_PARTIALGOOD_FLAG8      0x00030018u
#define EFUSE_KERNEL_PARTIALGOOD_FLAG9      0x00030019u
#define EFUSE_KERNEL_MODEM_HUK              0x0003001Au
#define EFUSE_KERNEL_UAPP_ENABLE            0x0003001Bu
#define EFUSE_KERNEL_UAPP_KEYREVOKE         0x0003001Cu
#define EFUSE_KERNEL_ROTPK_HASH0            0x0003001Du
#define EFUSE_KERNEL_ROTPK_HASH1            0x0003001Eu

#endif /* __EFUSE_KERNEL_DEF_H__ */
