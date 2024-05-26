/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
 * Description: efuse driver head file
 * Create: 2021/2/8
 */

#ifndef __EFUSE_DRIVER_H__
#define __EFUSE_DRIVER_H__
#include <linux/kernel.h>
#include <linux/mutex.h>
#include <linux/types.h>
#include <linux/string.h>
#include "efuse_kernel_def.h"

#define EFUSE_KERNEL_DIEID_VALUE             0x00030010u
#define EFUSE_KERNEL_PARTIALGOOD_FLAG1       0x00030011u
#define EFUSE_KERNEL_PARTIALGOOD_FLAG2       0x00030012u
#define EFUSE_KERNEL_PARTIALGOOD_FLAG3       0x00030013u
#define EFUSE_KERNEL_PARTIALGOOD_FLAG4       0x00030014u
#define EFUSE_KERNEL_PARTIALGOOD_FLAG5       0x00030015u
#define EFUSE_KERNEL_PARTIALGOOD_FLAG6       0x00030016u
#define EFUSE_KERNEL_PARTIALGOOD_FLAG7       0x00030017u
#define EFUSE_KERNEL_PARTIALGOOD_FLAG8       0x00030018u
#define EFUSE_KERNEL_PARTIALGOOD_FLAG9       0x00030019u
#define EFUSE_KERNEL_MODEM_HUK               0x0003001Au

#define EFUSE_BITCNT_ALLOC_BITS              (16u)

/* error code */
#define EFUSE_OK                              0x0u
#define EFUSE_INVALID_PARAM_ERR               0x00000001u
#define EFUSE_OVERFLOW_ERR                    0x00000002u
#define EFUSE_SMC_PROC_ERR                    0x00000004u
#define EFUSE_LOCK_OPS_ERR                    0x00000008u
#define EFUSE_FUNC_NOT_SUPPORT_ERR            0x00000010u

/* Get i bits mask */
#define bit_mask(i)                           ((1u << (i)) - 1)

/*
 * Extract `j` bits from bit `i` in value `x`.
 * Means that: MSB..{i+j-1..i}..0
 */
#define get_bits(x, i, j)                     (((x) >> (i)) & bit_mask(j))

struct efuse_desc_t {
	uint32_t *buf;          /* the target data will be stored here */
	uint32_t buf_size;      /* the size of buf in uint32_t */
	uint32_t item_vid;      /* the virtual id of efuse item to operate */
};

#ifdef CONFIG_VENDOR_EFUSE
/*
 * Common efuse read function operated by item_vid. All bit values of the
 *     efuse item will be read and stored in `buf`.
 * User should provide enough `buf` and its `buf_size` to store the read data. If read
 *     succeed, the `buf_size` will be changed as the real size of read data in u32.
 *
 * @param
 * desc                         pointer to struct efuse_desc_t
 *
 * @return
 * EFUSE_OK                     operation succeed
 * other                        error code
 */
uint32_t efuse_read_value_t(struct efuse_desc_t *desc);

/*
 * get efuse bit_cnt by item_vid.
 *
 * @param
 * item_vid             the item to query
 * bit_cnt              pointer to save the result
 *
 * @return
 * EFUSE_OK           operation succeed
 * other              error code
 */
uint32_t efuse_get_item_bit_cnt(uint32_t item_vid, uint32_t *bit_cnt);

#else
static inline uint32_t efuse_read_value_t(struct efuse_desc_t *desc)
{
	pr_err("%s not implement\n", __func__);
	return EFUSE_FUNC_NOT_SUPPORT_ERR;
}

static inline uint32_t efuse_get_item_bit_cnt(uint32_t item_vid, uint32_t *bit_cnt)
{
	pr_err("%s not implement\n", __func__);
	return EFUSE_FUNC_NOT_SUPPORT_ERR;
}
#endif

enum tag_efuse_log_level {
	LOG_LEVEL_DISABLE = 0,
	LOG_LEVEL_ERROR = 1,
	LOG_LEVEL_WARNING,
	LOG_LEVEL_DEBUG,
	LOG_LEVEL_INFO,
	LOG_LEVEL_TOTAL = LOG_LEVEL_INFO
};

enum tag_efuse_mem_attr {
	EFUSE_MEM_ATTR_NONE = -1,
	EFUSE_MEM_ATTR_HUK = 0,
	EFUSE_MEM_ATTR_SCP,
	EFUSE_MEM_ATTR_AUTHKEY,
	EFUSE_MEM_ATTR_CHIPID,
	EFUSE_MEM_ATTR_TSENSOR_CALIBRATION,
	EFUSE_MEM_ATTR_HUK_RD_DISABLE,
	EFUSE_MEM_ATTR_AUTHKEY_RD_DISABLE,
	EFUSE_MEM_ATTR_DBG_CLASS_CTRL,
	EFUSE_MEM_ATTR_DIEID,
#ifdef CONFIG_DFX_DEBUG_FS
	EFUSE_MEM_ATTR_SLTFINISHFLAG,
#endif
	EFUSE_MEM_ATTR_MODEM_AVS,
	EFUSE_MEM_ATTR_MAX
};

struct tag_efuse_attr_info {
	u32 bits_width;
};

struct tag_efusec_data {
	u32 efuse_group_max;
	phys_addr_t  paddr;
	u8 *vaddr;
	s32 (*atf_fn)(u64, u64, u64, u64);
	struct tag_efuse_attr_info efuse_attrs_from_dts[EFUSE_MEM_ATTR_MAX];
	struct mutex efuse_mutex; /* mutex to limit one caller in a time */
	u32 is_init_success;
};

/* efuse r&w func id */

/* enable and disable debug mode */
#define EFUSE_FN_ENABLE_DEBUG              0xCA000048
#define EFUSE_FN_DISABLE_DEBUG             0xCA000049

/* HEALTH_LEVEL */
#define EFUSE_FN_WR_HEALTH_LEVEL           0xCA000051

/* partial pass2 */
#define EFUSE_FN_RD_PARTIAL_PASSP2         0xCA000052

/* CHIPLEVEL_TESTFLAG */
#define EFUSE_FN_RD_CHIPLEVEL_TESTFLAG     0xCA000053

/* PARTIALGOOD_FLAG */
#define EFUSE_FN_RD_PARTIALGOOD_FLAG       0xCA000054

/* the max num of efuse group for a feature */
#define EFUSE_BUFFER_MAX_NUM               10

#ifndef OK
#define OK                                 0
#endif
#ifndef ERROR
#define ERROR                              (-1)
#endif

#define EFUSE_READ_CHIPID                  0x1000
#define EFUSE_READ_DIEID                   0x2000
#define EFUSE_WRITE_CHIPID                 0x3000
#define EFUSE_READ_AUTHKEY                 0x4000
#define EFUSE_WRITE_AUTHKEY                0x5000
#define EFUSE_READ_CHIPIDLEN               0x6000
#define EFUSE_WRITE_DEBUGMODE              0x7000
#define EFUSE_READ_DEBUGMODE               0x8000

#ifdef CONFIG_DFX_DEBUG_FS
#define EFUSE_TEST_WR                      0xa001
#define EFUSE_TEST_READ_CHIPID             0xa002
#define EFUSE_TEST_READ_DIEID              0xa003
#define EFUSE_TEST_READ_KCE                0xa004
#define EFUSE_TEST_WRITE_KCE               0xa005
#endif

#ifdef CONFIG_DFX_DEBUG_FS
#define EFUSE_WRITE_SLTFINISHFLAG          0xb000
#define EFUSE_READ_SLTFINISHFLAG           0xb001
#define EFUSE_SLTFINISHFLAG_LENGTH_BYTES   4
#endif

#define EFUSE_READ_DESKEW                  0xe000
#define EFUSE_READ_SOCID                   0xf000
#define EFUSE_GET_NVCNT                    0xf001
#define EFUSE_SET_NVCNT                    0xf002
#define EFUSE_UPDATE_NVCNT                 0xf003

#define EFUSE_MODULE_INIT_SUCCESS          0x12345678

#define EFUSE_TIMEOUT_SECOND               1000
#define EFUSE_KCE_LENGTH_BYTES             16
#define EFUSE_DIEID_LENGTH_BYTES           20
#define EFUSE_CHIPID_LENGTH_BYTES          8
#define EFUSE_PARTIAL_PASS_LENGTH_BYTES    3
/* calculate the Byte of auth_key */
#define EFUSE_SECDBG_LENGTH_BYTES          4
#define EFUSE_DESKEW_LENGTH_BYTES          1
/* calculate the Byte of modem_avs */
#define EFUSE_AVS_LENGTH_BYTES(bits)       DIV_ROUND_UP(bits, 8)
#define EFUSE_AVS_MAX_LENGTH_BYTES         3
#define EFUSE_NVCNT_LENGTH_BYTES           4
#define EFUSE_SOCID_LENGTH_BYTES           32

#ifdef CONFIG_VENDOR_EFUSE
s32 get_efuse_dieid_value(u8 *buf, u32 size, u32 timeout);
s32 get_efuse_chipid_value(u8 *buf, u32 size, u32 timeout);
#if defined(CONFIG_GENERAL_SEE) || defined(CONFIG_CRYPTO_CORE)
s32 get_efuse_hisee_value(u8 *buf, u32 size, u32 timeout);
s32 set_efuse_hisee_value(u8 *buf, u32 size, u32 timeout);
#endif
s32 get_efuse_kce_value(u8 *buf, u32 size, u32 timeout);
s32 set_efuse_kce_value(u8 *buf, u32 size, u32 timeout);
s32 get_efuse_deskew_value(u8 *buf, u32 size, u32 timeout);
s32 efuse_read_value(u32 *buf, u32 buf_size, u32 func_id);
s32 efuse_write_value(u32 *buf, u32 buf_size, u32 func_id);
s32 get_efuse_avs_value(u8 *buf, u32 buf_size, u32 timeout);
s32 get_partial_pass_info(u8 *buf, u32 size, u32 timeout);
s32 get_efuse_socid_value(u8 *buf, u32 size, u32 timeout);
s32 efuse_update_nvcnt(u8 *buf, u32 size);
#else
static inline s32 get_efuse_dieid_value(u8 *buf, u32 size, u32 timeout)
{
	return OK;
}

static inline s32 get_efuse_chipid_value(u8 *buf, u32 size, u32 timeout)
{
	return OK;
}

#if defined(CONFIG_GENERAL_SEE) || defined(CONFIG_CRYPTO_CORE)
static inline s32 get_efuse_hisee_value(u8 *buf, u32 size, u32 timeout)
{
	return OK;
}

static inline s32 set_efuse_hisee_value(u8 *buf, u32 size, u32 timeout)
{
	return OK;
}
#endif

static inline s32 set_efuse_kce_value(u8 *buf, u32 size, u32 timeout)
{
	return OK;
}

static inline s32 get_efuse_kce_value(u8 *buf, u32 size, u32 timeout)
{
	return OK;
}

static inline s32 get_efuse_deskew_value(u8 *buf, u32 size, u32 timeout)
{
	return OK;
}

static inline s32 efuse_read_value(u32 *buf, u32 buf_size, u32 func_id)
{
	return OK;
}

static inline s32 efuse_write_value(u32 *buf, u32 buf_size, u32 func_id)
{
	return OK;
}

static inline s32 get_efuse_avs_value(u8 *buf, u32 buf_size, u32 timeout)
{
	return OK;
}
static inline s32 get_partial_pass_info(u8 *buf, u32 size, u32 timeout)
{
	return OK;
}
static inline s32 get_efuse_socid_value(u8 *buf, u32 size, u32 timeout)
{
	return OK;
}
#endif

#ifdef CONFIG_VENDOR_EFUSE
#define  EFUSE_DIEID_GROUP_START         32
#define  EFUSE_DIEID_GROUP_WIDTH         5
#define  EFUSE_CHIPID_GROUP_START        57
#define  EFUSE_CHIPID_GROUP_WIDTH        2
#define  EFUSE_KCE_GROUP_START           28
#define  EFUSE_KCE_GROUP_WIDTH           4
#endif

#endif /* __EFUSE_DRIVER_H__ */
