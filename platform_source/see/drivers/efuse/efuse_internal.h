/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.
 * Description: efuse header file used for driver
 * Create: 2022/01/19
 */

#ifndef __EFUSE_INTERNAL_H__
#define __EFUSE_INTERNAL_H__

#include <linux/types.h>                       /* uint32_t */

/* kernel to atf smc id */
#define FID_BL31_KERNEL_GET_EFUSE_ITEM_INFO    0xCA00004Au
#define FID_BL31_KERNEL_READ_EFUSE             0xCA00004Bu


#define EFUSE_MAX_SIZE                         4096
#define EFUSEC_GROUP_MAX_COUNT                 128
#define EFUSE_BITS_PER_GROUP                   32

#define div_round_up(nr, d)                    (((nr) + (d) - 1) / (d))

struct efuse_trans_t {
	uint32_t *buf;      /* store the divided data */
	uint32_t buf_size;  /* the size of buf in u32, need to be `1` here */
	uint32_t start_bit; /* the offset in the item */
	uint32_t bit_cnt;   /* the number of bits to operate */
	uint32_t item_vid;  /* the efuse item to operate */
};

uint32_t efuse_inner_read_value(struct efuse_desc_t *desc);
uint32_t efuse_get_item_info(struct efuse_trans_t *trans);

#endif /* __EFUSE_INTERNAL_H__ */
