/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2021. All rights reserved.
 * Description: Header file for Crypto Core upgrade process.
 * Create: 2019/11/19
 */

#ifndef CRYPTO_CORE_UPGRADE_H
#define CRYPTO_CORE_UPGRADE_H

#include <crypto_core_internal.h>
#include <linux/types.h>

#define MSPC_IMAGE_UPGRADE_RUNNING     0x87654321

#define MSPC_ATF_UPGRADE_TIMEOUT        30000 /* 30s */

#define MSPC_MAX_IMG_SIZE    (640 * SIZE_1K)
#define MSPC_SHARE_BUFF_SIZE round_up((MSPC_MAX_IMG_SIZE + \
	MSPC_ATF_MESSAGE_HEADER_LEN), PAGE_SIZE)

#define MSPC_IMG_SLOADER_NAME          "SLOADER"
#define MSPC_IMG_ULOADER_NAME          "ULOADER"
#define MSPC_IMG_COS_NAME              "COS"
#define MSPC_IMG_COS1_NAME             "COS1"
#define MSPC_IMG_COS2_NAME             "COS2"
#define MSPC_IMG_COS3_NAME             "COS3"
#define MSPC_IMG_OTP_NAME              "OTP"
#define MSPC_IMG_OTP0_NAME             "OTP0"
#define MSPC_IMG_OTP1_NAME             "OTP1"
#define MSPC_IMG_DCS_NAME              "DCS"
#define MSPC_IMG_MISC_NAME             "MISC"

enum mspc_img_file_type {
	SLOADER_IMG_TYPE = 0,
	ULOADER_IMG_TYPE,
	COS_IMG_TYPE,
	NATIVE_COS_IMG_TYPE,
	COS1_IMG_TYPE,
	COS2_IMG_TYPE,
	COS3_IMG_TYPE,
	COS4_IMG_TYPE,
	COS5_IMG_TYPE,
	COS6_IMG_TYPE,
	COS7_IMG_TYPE,
	DCS0_IMG_TYPE,
	DCS1_IMG_TYPE,
	DCS2_IMG_TYPE,
	DCS3_IMG_TYPE,
	DCS4_IMG_TYPE,
	DCS5_IMG_TYPE,
	DCS6_IMG_TYPE,
	DCS7_IMG_TYPE,
	DCS8_IMG_TYPE,
	DCS9_IMG_TYPE,
	DCS10_IMG_TYPE,
	DCS11_IMG_TYPE,
	DCS12_IMG_TYPE,
	DCS13_IMG_TYPE,
	DCS14_IMG_TYPE,
	DCS15_IMG_TYPE,
	DCS16_IMG_TYPE,
	DCS17_IMG_TYPE,
	DCS18_IMG_TYPE,
	DCS19_IMG_TYPE,
	OTP_IMG_TYPE,
	OTP0_IMG_TYPE,
	OTP1_IMG_TYPE,
	MISC0_IMG_TYPE,
	MISC1_IMG_TYPE,
	MISC2_IMG_TYPE,
	MISC3_IMG_TYPE,
	MISC4_IMG_TYPE,
	MISC5_IMG_TYPE,
	MISC6_IMG_TYPE,
	MISC7_IMG_TYPE,
	MISC8_IMG_TYPE,
	MISC9_IMG_TYPE,
	MISC10_IMG_TYPE,
	MISC11_IMG_TYPE,
	MISC12_IMG_TYPE,
	MISC13_IMG_TYPE,
	MISC14_IMG_TYPE,
	MISC15_IMG_TYPE,
	MISC16_IMG_TYPE,
	MISC17_IMG_TYPE,
	MISC18_IMG_TYPE,
	MISC19_IMG_TYPE,
	/* make sure COS_FLASH_IMG_TYPE as the last vaild IMG_TYPE */
	COS_FLASH_IMG_TYPE,
	MAX_IMG_TYPE,
};

/* Only normal type will check whether the cos is new. */
enum mspc_upgrade_type {
	MSPC_UPGRADE_NORMAL    = 0,
	MSPC_UPGRADE_CHIP_TEST = 1,
	MSPC_UPGRADE_BOOT_EXCP = 2,
	MSPC_UPGRADE_FLASHCOS  = 3,
	MSPC_UPGRADE_MAX_TYPE,
};

union mspc_timestamp_info {
	struct {
		uint8_t second;
		uint8_t minute;
		uint8_t hour;
		uint8_t day;
		uint8_t month;
		uint8_t padding;
		uint16_t year;
	} timestamp;
	uint64_t value;
};

/* Begin */
#define MSPC_MAX_COS_IMAGE_NUM          8

/*
 * mspc image header (32 bytes)
 * 4 bytes magic value + 20 bytes time stamp +
 * 4 bytes image total size + 4 bytes sub image num
 */
#define MSPC_IMG_MAGIC_LEN              4
#define MSPC_IMG_MAGIC_VALUE            "inse"
#define MSPC_IMG_TIME_STAMP_LEN         20
#define MSPC_IMG_DATA_LEN               4
#define MSPC_IMG_SUB_FILE_LEN           4
#define MSPC_IMG_HEADER_LEN             (MSPC_IMG_MAGIC_LEN +\
	 MSPC_IMG_TIME_STAMP_LEN + \
	 MSPC_IMG_DATA_LEN +\
	 MSPC_IMG_SUB_FILE_LEN)

#define MSPC_IMG_SUB_FILE_MAX           50
/* sub file : 8 bytes file name + 4 bytes file offset + 4 bytes data len */
#define MSPC_IMG_SUB_FILE_NAME_LEN      8
#define MSPC_IMG_SUB_FILE_OFFSET_LEN    4
#define MSPC_IMG_SUB_FILE_DATA_LEN      4
#define MSPC_IMG_SUB_FILES_LEN          (MSPC_IMG_SUB_FILE_NAME_LEN + \
	 MSPC_IMG_SUB_FILE_OFFSET_LEN + \
	 MSPC_IMG_SUB_FILE_DATA_LEN)

#define MSPC_IMG_HEADER_MAX_LEN         (MSPC_IMG_HEADER_LEN + \
	(MSPC_IMG_SUB_FILE_MAX * MSPC_IMG_SUB_FILES_LEN))

#define MSPC_SW_VERSION_MAGIC           0xa5a55a5a
#define MSPC_UFS_FLAG_HEADER_VER1       0x5A000001

struct img_file_info {
	int8_t name[MSPC_IMG_SUB_FILE_NAME_LEN];
	uint32_t offset;
	uint32_t size;
};

struct mspc_img_version {
	uint8_t version;
	uint8_t reserved1;
	uint8_t reserved2;
	uint8_t reserved3;
};

struct mspc_img_header {
	int8_t magic[MSPC_IMG_MAGIC_LEN];
	int8_t timestamp[MSPC_IMG_TIME_STAMP_LEN];
	uint32_t total_size;
	uint32_t file_cnt;
	struct img_file_info file[MSPC_IMG_SUB_FILE_MAX];
	/* above is same as in the hisee.img header */
	struct mspc_img_version img_ver[MSPC_MAX_COS_IMAGE_NUM];
	uint32_t is_cos_exist[MSPC_MAX_COS_IMAGE_NUM];
	uint32_t dcs_image_cnt[MSPC_MAX_COS_IMAGE_NUM];
};

/* struct for MSPC_SW_VERSION_TYPE */
struct mspc_ufs_flag_info {
	uint32_t magic;
	uint32_t header_version; /* reserved */
	union mspc_timestamp_info timestamp;
	struct mspc_img_version img_ver[MSPC_MAX_COS_IMAGE_NUM];
};

/* End */

int32_t mspc_parse_image_header(void);
struct mspc_img_header *mspc_get_image_header(void);
bool mspc_check_is_upgrading(void);
int32_t mspc_read_image(uint32_t type, int8_t *buf, uint32_t *size);
int32_t mspc_read_and_send_img(uint32_t type, uint32_t cmd);
void mspc_set_upgrading_flag(bool cfg);
int32_t mspc_upgrade_thread(void *data);
int32_t mspc_check_new_image(bool *is_new);
int32_t mspc_upgrade_image(uint32_t type);
void mspc_trigger_upgrade(void);
int32_t mspc_init_upgrade(struct device *dev);

#endif /* CRYPTO_CORE_UPGRADE_H */
