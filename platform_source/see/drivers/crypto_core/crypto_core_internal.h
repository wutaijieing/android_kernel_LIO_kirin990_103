/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2021. All rights reserved.
 * Description: Header file for Crypto Core base process.
 * Create: 2019/11/14
 */

#ifndef CRYPTO_CORE_INTERNAL_H
#define CRYPTO_CORE_INTERNAL_H

#include <linux/device.h>
#include <linux/mutex.h>
#include <linux/pm_wakeup.h>
#include <linux/semaphore.h>
#include <linux/types.h>

#define MSPC_DEVICE_NAME     "mspc"

#define SIZE_1K              1024
#define SIZE_4K              (4 * SIZE_1K)
#define SIZE_1M              (1024 * SIZE_1K)

#define MSPC_CMD_MAX_LEN      2048
#define MSPC_CMD_NAME_LEN     128
#define MSPC_BUF_SHOW_LEN     128

#define MSPC_SM_MODE_MAGIC    0x96BFE4AA
#define MSPC_DM_MODE_MAGIC    0x69401B55

/*
 * Any modification of MSPC_NATIVE_COS_ID and MSPC_FLASH_COS_ID
 * must be sync to MSPC_NATIVE_COS_POWER_PARAM, MSPC_FLASH_COS_POWER_PARAM
 * and MSPC_PINCODE_POWER_PARAM in crypto_core_power.h file.
 */
#define MSPC_NATIVE_COS_ID    1
#define MSPC_FLASH_COS_ID     5
#define MSPC_MAX_COS_ID       6

/* mspc lcs mode */
#define MSPC_LCS_BANK_REG   SOC_SCTRL_SCBAKDATA10_ADDR(SOC_ACPU_SCTRL_BASE_ADDR)
#define MSPC_LCS_DM_BIT     13

/* for secflash */
#define SECFLASH_IS_ABSENCE_MAGIC 0x70eb2c2dU
#define SECFLASH_NXP_EXIST_MAGIC  0xa5c89ceaU
#define SECFLASH_ST_EXIST_MAGIC   0xe59a6b89U

#define CRYPTO_CORE_EFUSE_GROUP_BIT_SIZE           32
#define CRYPTO_CORE_EFUSE_GROUP_NUM                2
#define CRYPTO_CORE_EFUSE_LENGTH                   8
#define CRYPTO_CORE_EFUSE_TIMEOUT                  1000
#define CRYPTO_CORE_EFUSE_MASK                     1
#define CRYPTO_CORE_EFUSE_SM_MODE                  1

struct mspc_module_data {
	struct device *device;
	bool is_fpga;
	/* Record the error number of mspc. */
	atomic_t errno;
	bool rpmb_is_ready;
	/* The bit position for SM flga in efuse. */
	uint32_t sm_efuse_pos;
	struct mutex mspc_mutex; /* mutex for global resources */
};

struct mspc_driver_func {
	int8_t *func_name;
	int32_t (*func)(const int8_t *buf, int32_t len);
};

void mspc_lock(void);
void mspc_unlock(void);
void mspc_record_errno(int32_t error);
struct device *mspc_get_device(void);
struct mspc_encos_header *mspc_get_encos_header(void);
uint32_t crypto_core_get_sm_efuse_pos(void);
int32_t mspc_get_recorded_errno(void);
int32_t mspc_get_lcs_mode(uint32_t *mode);
int32_t mspc_filectrl_preprocess(const int8_t *buf, size_t count);
int32_t mspc_secflash_is_available(uint32_t *status_result);
int32_t mspc_load_image2ddr(void);

ssize_t mspc_ioctl_show(struct device *dev,
			 struct device_attribute *attr, char *buf);
ssize_t mspc_ioctl_store(struct device *dev, struct device_attribute *attr,
			  const char *buf, size_t count);
#endif /* CRYPTO_CORE_INTERNAL_H */
