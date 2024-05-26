/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2021. All rights reserved.
 * Description: Drivers for Crypto Core file operation.
 * Create: 2019/12/30
 */

#include "crypto_core_fs.h"
#include <linux/fs.h>
#include <platform_include/basicplatform/linux/partition/partition_ap_kernel.h>
#include <linux/printk.h>
#include <linux/sizes.h>
#include <linux/vmalloc.h>
#include <crypto_core_mem_layout.h>
#include <securec.h>
#include "crypto_core_internal.h"
#include "crypto_core_dcs.h"
#include "crypto_core_errno.h"
#include "crypto_core_power.h"

#define ERASE_FLAG_OFFSET             1
#define ERASE_FLAG_STRING             "ERASE"

#define MSPC_THOUSAND                 1000
#define MSPC_HUNDRED                  100
#define MSPC_TEN                      10
enum mspc_timestamp {
	MSPC_YEAR_THOUSAND     = 0,
	MSPC_YEAR_HUNDRED      = 1,
	MSPC_YEAR_TEN          = 2,
	MSPC_YEAR_SINGLE       = 3,
	MSPC_MONTH_TEN         = 5,
	MSPC_MONTH_SINGLE      = 6,
	MSPC_DAY_TEN           = 8,
	MSPC_DAY_SINGLE        = 9,
	MSPC_HOUR_TEN          = 11,
	MSPC_HOUR_SINGLE       = 12,
	MSPC_MINUTE_TEN        = 14,
	MSPC_MINUTE_SINGLE     = 15,
	MSPC_SECOND_TEN        = 17,
	MSPC_SECOND_SINGLE     = 18,
};

#define MSPC_IS_WRITE_ACCESS(type)     \
	(((type) == MSPC_FLAG_INFO_WRITE) ||\
	((type) == MSPC_IMAGE_HEADER_WRITE) ||\
	((type) == MSPC_ENCOS_HEADER_WRITE) ||\
	((type) == MSPC_UPGRADE_FLAG_WRITE) ||\
	((type) == MSPC_IMAGE_WRITE))

struct mspc_encos_header g_mspc_encos_header;

#define MSPC_IMG_PARTITION_SIZE     (4 * SIZE_1M)

static void mspc_get_accessing_info(uint32_t type,
				    struct mspc_access_info *info)
{
	if (type == MSPC_FLAG_INFO_WRITE ||
	    type == MSPC_IMAGE_HEADER_WRITE ||
	    type == MSPC_ENCOS_HEADER_WRITE ||
	    type == MSPC_UPGRADE_FLAG_WRITE ||
	    type == MSPC_IMAGE_WRITE) {
		info->flags = O_WRONLY;
	} else {
		info->flags = O_RDONLY;
	}

	if (type == MSPC_FLAG_INFO_READ || type == MSPC_FLAG_INFO_WRITE) {
		/* Access image flag: last 1KB of mspc ufs partition. */
		info->offset = (int64_t)(MSPC_IMG_PARTITION_SIZE - SIZE_1K);
		info->size = sizeof(struct mspc_ufs_flag_info);
	} else if (type == MSPC_UPGRADE_FLAG_READ ||
		   type == MSPC_UPGRADE_FLAG_WRITE) {
		info->offset = (int64_t)(MSPC_IMG_PARTITION_SIZE - SIZE_1K +
			sizeof(struct mspc_ufs_flag_info));
		info->size = sizeof(uint32_t);
	} else if (type == MSPC_IMAGE_HEADER_READ ||
		   type == MSPC_IMAGE_HEADER_WRITE) {
		/* Access image header. */
		info->offset = 0;
		info->size = MSPC_IMG_HEADER_MAX_LEN;
	} else if (type == MSPC_ENCOS_HEADER_READ ||
		   type == MSPC_ENCOS_HEADER_WRITE) {
		/* Access encos image header. */
		info->offset = 0;
		info->size = sizeof(struct mspc_encos_header);
	} else {
		/*
		 * Acess image data. The size and offset are set
		 * in caller, which are gotten from image header.
		 */
		;
	}
}

static int32_t mspc_prepare_accessing(int8_t *buf, uint32_t size,
				      struct mspc_access_info *info)
{
	int32_t ret;
	int8_t *ptn_name = MSPC_IMAGE_PARTITION_NAME;

	if (!buf || !info) {
		pr_err("%s:Invalid param!\n", __func__);
		return MSPC_INVALID_PARAM_ERROR;
	}

	if (info->partition_type == MSPC_PARTITON_ENCOS)
		ptn_name = MSPC_ENCOS_PARTITION_NAME;
	ret = flash_find_ptn_s(ptn_name,
			       info->fullpath, MSPC_PTN_PATH_BUF_SIZE);
	if (ret != 0) {
		pr_err("%s:find partition failed, ret=%d!\n", __func__, ret);
		return MSPC_OPEN_FILE_ERROR;
	}

	mspc_get_accessing_info(info->access_type, info);
	if (info->size > size) {
		pr_err("%s:Invalid size:%x\n", __func__, size);
		return MSPC_INVALID_PARAM_ERROR;
	}

	return MSPC_OK;
}

int32_t mspc_access_partition(int8_t *buf, uint32_t size,
			      struct mspc_access_info *access_info)
{
	int32_t fd = -1;
	mm_segment_t old_fs;
	ssize_t cnt;
	int32_t ret;

	ret = mspc_prepare_accessing(buf, size, access_info);
	if (ret != MSPC_OK) {
		pr_err("%s: prepare accessing failed!\n", __func__);
		return ret;
	}

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	fd = crypto_core_sys_open(access_info->fullpath, access_info->flags,
				  MSPC_FILESYS_DEFAULT_MODE);
	if (fd < 0) {
		pr_err("%s:open file failed! ret=%d\n", __func__, fd);
		ret = MSPC_OPEN_FILE_ERROR;
		goto exit;
	}
	ret = crypto_core_sys_lseek(fd, access_info->offset, SEEK_SET);
	if (ret < 0) {
		pr_err("%s:lseek offset failed! ret=%d\n", __func__, fd);
		ret = MSPC_LSEEK_FILE_ERROR;
		goto err_close_file;
	}

	ret = MSPC_OK;
	if (MSPC_IS_WRITE_ACCESS(access_info->access_type)) {
		cnt = crypto_core_sys_write(fd, buf, access_info->size);
		ret = crypto_core_sys_fsync(fd);
		if (ret < 0) {
			pr_err("%s:sync file faild, ret=%d\n", __func__, ret);
			ret = MSPC_SYNC_FILE_ERROR;
		} else {
			ret = MSPC_OK;
		}
	} else {
		cnt = crypto_core_sys_read(fd, buf, access_info->size);
	}

	if ((uint64_t)cnt < access_info->size) {
		pr_err("%s:access file failed! ret=%ld\n", __func__, cnt);
		ret = MSPC_ACCESS_FILE_ERROR;
		goto err_close_file;
	}

err_close_file:
	crypto_core_sys_close(fd);
exit:
	set_fs(old_fs);
	return ret;
}

static int32_t mspc_is_subfile_valid(struct mspc_img_header *header)
{
	uint32_t i;
	uint32_t total_size, size;
	uint32_t offset, cos_id;

	/* The size of image header and all subimage headers. */
	total_size = MSPC_IMG_HEADER_LEN +
		     MSPC_IMG_SUB_FILES_LEN * header->file_cnt;

	for (i = 0; i < header->file_cnt; i++) {
		offset = header->file[i].offset;
		size = header->file[i].size;
		if (size == 0 || size > MSPC_MAX_IMG_SIZE) {
			pr_err("%s:image:%d size:%u error!\n",
			       __func__, i, size);
			return MSPC_SUB_FILE_SIZE_ERROR;
		}

		if (offset < total_size) {
			pr_err("%s:image:%d offset:%u error!\n",
			       __func__, i, offset);
			return MSPC_SUB_FILE_SIZE_ERROR;
		}
		total_size += size;

		if (!strncmp(header->file[i].name, MSPC_IMG_DCS_NAME,
			     strlen(MSPC_IMG_DCS_NAME))) {
			cos_id = (uint32_t)(header->file[i].name[strlen(MSPC_IMG_DCS_NAME)] - '0');
			if (cos_id >= MSPC_MAX_COS_IMAGE_NUM ||
			    ++header->dcs_image_cnt[cos_id] >
			    MSPC_MAX_DCS_FILES) {
				pr_err("%s:dcs for image:%d cos is:%d error!\n",
				       __func__, i, cos_id);
				return MSPC_DCS_ID_LARGE_ERROR;
			}
		}
		if (!strncmp(header->file[i].name,
			     MSPC_IMG_COS_NAME,
			     strlen(MSPC_IMG_COS_NAME))) {
			cos_id = (uint32_t)(header->file[i].name[strlen(MSPC_IMG_COS_NAME)] - '0');
			if (cos_id >= MSPC_MAX_COS_IMAGE_NUM) {
				pr_err("%s:cos image:%d cos id:%d is error!\n",
				       __func__, i, cos_id);
				return MSPC_SUB_FILE_ID_ERROR;
			}
			header->is_cos_exist[cos_id] = 1;
		}
	}
	return MSPC_OK;
}

int32_t mspc_check_encos_header(struct mspc_encos_header *header)
{
	uint32_t i;

	if (strncmp(header->magic, MSPC_ENCOS_MAGIC_STRING,
		    MSPC_ENCOS_MAGIC_LEN)) {
			pr_err("%s:magic check %s failed\n",
				__func__, header->magic);
			return MSPC_ERROR;
	}

	if (header->file_cnt != MSPC_ENCOS_FILE_MAX) {
			pr_err("%s:file cnt:%d check failed\n", __func__, header->file_cnt);
			return MSPC_ERROR;
	}

	if (header->total_size !=
	    sizeof(struct mspc_encos_header) + MSPC_ENCOS_TOTAL_FILE_SIZE) {
			pr_err("%s:total size:%d check failed\n", __func__, header->total_size);
			return MSPC_ERROR;
	}

	for (i = 0; i < header->file_cnt; i++) {
		if ((header->file[i].offset >
		    (sizeof(struct mspc_encos_header) +
		    (i * MSPC_ENCOS_FILE_LEN))) ||
		    (MSPC_ENCOS_FILE_LEN <
		    header->file[i].size)) {
			pr_err("%s:file %d offset check %s failed\n",
				__func__, i, header->file[i].name);
			return MSPC_ERROR;
		}
	}

	return MSPC_OK;
}

int32_t mspc_get_encos_img_header(struct mspc_encos_header *header)
{
	int32_t ret;
	struct mspc_access_info info;

	if (!header) {
		pr_err("%s:Invalid param!\n", __func__);
		return MSPC_INVALID_PARAM_ERROR;
	}

	(void)memset_s(&info, sizeof(info), 0, sizeof(info));
	info.access_type = MSPC_ENCOS_HEADER_READ;
	info.partition_type = MSPC_PARTITON_ENCOS;
	ret = mspc_access_partition((int8_t *)header,
				    sizeof(struct mspc_encos_header),
				    &info);
	if (ret != MSPC_OK) {
		pr_err("%s:Read encos header failed!\n", __func__);
		return ret;
	}

	ret = mspc_check_encos_header(header);
	if (ret != MSPC_OK)
		pr_err("%s:encos header is invalid!\n", __func__);

	return ret;
}

int32_t mspc_get_and_parse_img_header(struct mspc_img_header *header)
{
	int32_t ret;
	struct mspc_access_info info;

	if (!header) {
		pr_err("%s:Invalid param!\n", __func__);
		return MSPC_INVALID_PARAM_ERROR;
	}

	(void)memset_s(&info, sizeof(info), 0, sizeof(info));
	info.access_type = MSPC_IMAGE_HEADER_READ;
	ret = mspc_access_partition((int8_t *)header,
				    MSPC_IMG_HEADER_MAX_LEN, &info);
	if (ret != MSPC_OK) {
		pr_err("%s:Read image header failed!\n", __func__);
		return ret;
	}

	if (strncmp(header->magic, MSPC_IMG_MAGIC_VALUE, MSPC_IMG_MAGIC_LEN)) {
		pr_err("%s: mspc img magic error!\n", __func__);
		ret = MSPC_IMG_PARTITION_MAGIC_ERROR;
		return ret;
	}
	if (header->file_cnt > MSPC_IMG_SUB_FILE_MAX) {
		pr_err("%s: Invalid file numbers:%d!\n",
		       __func__, header->file_cnt);
		ret = MSPC_IMG_PARTITION_FILES_ERROR;
		return ret;
	}

	ret = mspc_is_subfile_valid(header);
	if (ret != MSPC_OK) {
		pr_err("%s:image header is invalid!\n", __func__);
		return ret;
	}
	return ret;
}

void mspc_parse_timestamp(const int8_t *time,
			  uint32_t len, union mspc_timestamp_info *timestamp)
{
	uint32_t value;

	if (!time || !timestamp || len != MSPC_IMG_TIME_STAMP_LEN)
		return;

	timestamp->value = 0;

	value = (time[MSPC_YEAR_THOUSAND] - MSPC_CHAR_ZERO) * MSPC_THOUSAND +
		(time[MSPC_YEAR_HUNDRED] - MSPC_CHAR_ZERO) * MSPC_HUNDRED
		+ (time[MSPC_YEAR_TEN] - MSPC_CHAR_ZERO) * MSPC_TEN +
		(time[MSPC_YEAR_SINGLE] - MSPC_CHAR_ZERO);
	timestamp->timestamp.year = value;

	value = (time[MSPC_MONTH_TEN] - MSPC_CHAR_ZERO) * MSPC_TEN +
		(time[MSPC_MONTH_SINGLE] - MSPC_CHAR_ZERO);
	timestamp->timestamp.month = value;

	value = (time[MSPC_DAY_TEN] - MSPC_CHAR_ZERO) * MSPC_TEN +
		(time[MSPC_DAY_SINGLE] - MSPC_CHAR_ZERO);
	timestamp->timestamp.day = value;

	value = (time[MSPC_HOUR_TEN] - MSPC_CHAR_ZERO) * MSPC_TEN +
		(time[MSPC_HOUR_SINGLE] - MSPC_CHAR_ZERO);
	timestamp->timestamp.hour = value;

	value = (time[MSPC_MINUTE_TEN] - MSPC_CHAR_ZERO) * MSPC_TEN +
		(time[MSPC_MINUTE_SINGLE] - MSPC_CHAR_ZERO);
	timestamp->timestamp.minute = value;

	value = (time[MSPC_SECOND_TEN] - MSPC_CHAR_ZERO) * MSPC_TEN +
		(time[MSPC_SECOND_SINGLE] - MSPC_CHAR_ZERO);
	timestamp->timestamp.second = value;
}

int32_t mspc_check_flash_cos_file(uint32_t *state)
{
	struct mspc_encos_header *header = NULL;
	uint32_t id = MSPC_FLASH_COS_ID - MSPC_ENCOS_START_ID;

	if (!state)
		return MSPC_INVALID_PARAM_ERROR;

	header = mspc_get_encos_header();
	if (!header) {
		pr_err("%s:get encos header ptr failed!\n", __func__);
		return MSPC_ERROR;
	}

	if (mspc_check_encos_header(header) != MSPC_OK) {
		pr_err("%s header invalid\n", __func__);
		return MSPC_ERROR;
	}

	if (strncmp(header->file[id].name,
		    MSPC_FLASH_COS_STRING,
		    strlen(MSPC_FLASH_COS_STRING)) == 0) {
		*state = FLASH_COS_EXIST;
	} else if (strncmp(header->file[id].name +
			   ERASE_FLAG_OFFSET,
			   ERASE_FLAG_STRING,
			   strlen(ERASE_FLAG_STRING)) == 0) {
		*state = FLASH_COS_ERASE;
	} else {
		*state = FLASH_COS_EMPTY;
	}

	return MSPC_OK;
}

static int32_t mspc_encos_erase(uint32_t fd, uint32_t id, struct mspc_encos_header *header)
{
	int64_t cnt;
	off_t offset;
	int8_t *file_buff = NULL;
	int8_t *magic_position = header->file[id].name;

	/* Set erase magic. */
	(void)memset_s(magic_position, MSPC_ENCOS_FILE_NAME_LEN,
		       0, MSPC_ENCOS_FILE_NAME_LEN);
	if (strcpy_s(magic_position + ERASE_FLAG_OFFSET,
		     MSPC_ENCOS_FILE_NAME_LEN - ERASE_FLAG_OFFSET,
		     ERASE_FLAG_STRING) != EOK) {
		pr_err("%s: strcpy_s failed!\n", __func__);
		return MSPC_SECUREC_ERROR;
	}

	if (crypto_core_sys_lseek(fd, 0, SEEK_SET) < 0) {
		pr_err("%s: lseek at 0 failed!\n", __func__);
		return MSPC_ENCOS_LSEEK_FILE_ERROR;
	}

	cnt = crypto_core_sys_write(fd, (const char *)header, sizeof(struct mspc_encos_header));
	if ((uint64_t)cnt < sizeof(struct mspc_encos_header)) {
		pr_err("%s: write header failed, ret: %lld\n", __func__, cnt);
		return MSPC_ENCOS_WRITE_FILE_ERROR;
	}

	/* Erase the flash cos data. */
	offset = (off_t)(header->file[id].offset);
	if (crypto_core_sys_lseek(fd, offset, SEEK_SET) < 0) {
		pr_err("%s: lseek at %ld failed!\n", __func__, offset);
		return MSPC_ENCOS_LSEEK_FILE_ERROR;
	}

	file_buff = (int8_t *)vmalloc(MSPC_ENCOS_FILE_LEN);
	if (!file_buff) {
		pr_err("%s: vmalloc failed!\n", __func__);
		return MSPC_NO_RESOURCES_ERROR;
	}
	(void)memset_s(file_buff, MSPC_ENCOS_FILE_LEN,
		0, MSPC_ENCOS_FILE_LEN);

	cnt = crypto_core_sys_write(fd, file_buff, MSPC_ENCOS_FILE_LEN);
	if (cnt < MSPC_ENCOS_FILE_LEN) {
		pr_err("%s:erase flash cos failed, ret:%lld\n", __func__, cnt);
		vfree(file_buff);
		return MSPC_ENCOS_WRITE_FILE_ERROR;
	}
	if (crypto_core_sys_fsync(fd) < 0) {
		pr_err("%s:sync file failed!\n", __func__);
		vfree(file_buff);
		return MSPC_SYNC_FILE_ERROR;
	}

	vfree(file_buff);
	return MSPC_OK;
}

int32_t mspc_remove_flash_cos(void)
{
	int32_t ret;
	int64_t fd = -1;
	mm_segment_t old_fs;
	int8_t fullpath[MSPC_PTN_PATH_BUF_SIZE] = {0};
	const char *filename = NULL;
	const uint32_t id = MSPC_FLASH_COS_ID - MSPC_ENCOS_START_ID;
	struct mspc_encos_header *header = NULL;

	header = mspc_get_encos_header();
	if (!header) {
		pr_err("%s: get encos header failed!\n", __func__);
		return MSPC_ERROR;
	}

	ret = mspc_check_encos_header(header);
	if (ret != MSPC_OK)
		return ret;

	/* 1. find the partition path name. */
	ret = flash_find_ptn_s(MSPC_ENCOS_PARTITION_NAME, fullpath,
			       sizeof(fullpath));
	if (ret != 0) {
		pr_err("%s: flash_find_ptn failed:%d\n", __func__, ret);
		return MSPC_ENCOS_FIND_PTN_ERROR;
	}

	fullpath[MSPC_PTN_PATH_BUF_SIZE - 1] = 0;
	filename = fullpath;
	old_fs = get_fs();
	set_fs(KERNEL_DS);

	/* 2. open file by for write with access rights 0200 */
	fd = crypto_core_sys_open(filename, O_WRONLY, S_IWUSR);
	if (fd < 0) {
		pr_err("%s: open %s failed:%lld!\n", __func__, filename, fd);
		set_fs(old_fs);
		return MSPC_ENCOS_OPEN_FILE_ERROR;
	}

	/* 3. Do real erase operate */
	ret = mspc_encos_erase((uint32_t)fd, id, header);
	crypto_core_sys_close((uint32_t)fd);
	set_fs(old_fs);
	return ret;
}

#define MSPC_FS_TOTAL_SIZE       ((MSPC_FAKE_RPMB_SIZE + SZ_1M - 1) / SZ_1M * SZ_1M)
#define MSPC_FS_ONCE_CHK_SIZE    SZ_1M

/* check hisee_fs partition to be all zero */
int32_t mspc_check_hisee_fs_empty(void)
{
	int64_t fd = -1;
	mm_segment_t old_fs;
	int8_t fullpath[MSPC_PTN_PATH_BUF_SIZE] = {0};
	int8_t *file_buff = NULL;
	int32_t i, ret;
	int64_t cnt;
	uint32_t j;

	/* 1. find the partition path name. */
	ret = flash_find_ptn_s(MSPC_FS_PARTITION_NAME, fullpath,
			       sizeof(fullpath));
	if (ret != 0) {
		pr_err("%s flash_find_ptn fail\n", __func__);
		return MSPC_FS_FIND_PTN_ERROR;
	}

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	/* 2. open file for read with accessrights 0400 */
	fd = crypto_core_sys_open(fullpath, O_RDONLY, S_IRUSR);
	if (fd < 0) {
		pr_err("%s %d open file failed\n", __func__, __LINE__);
		set_fs(old_fs);
		return MSPC_FS_OPEN_FILE_ERROR;
	}

	file_buff = (char *)vmalloc(MSPC_FS_ONCE_CHK_SIZE);
	if (!file_buff) {
		pr_err("%s vmalloc failed\n", __func__);
		ret = MSPC_NO_RESOURCES_ERROR;
		goto exit;
	}

	for (i = 0; i < MSPC_FS_TOTAL_SIZE / MSPC_FS_ONCE_CHK_SIZE; i++) {
		cnt = crypto_core_sys_read((uint32_t)fd, file_buff, MSPC_FS_ONCE_CHK_SIZE);
		if (cnt != MSPC_FS_ONCE_CHK_SIZE) {
			pr_err("%s read failed\n", __func__);
			ret = MSPC_FS_ACCESS_FILE_ERROR;
			goto exit;
		}
		for (j = 0; j < MSPC_FS_ONCE_CHK_SIZE / sizeof(int32_t); j++) {
			if (((int32_t *)file_buff)[j] != 0) {
				pr_err("%s check not zero\n", __func__);
				ret = MSPC_FS_CHECK_ZERO_ERROR;
				goto exit;
			}
		}
	}
	ret = MSPC_OK;
exit:
	if (file_buff)
		vfree(file_buff);
	crypto_core_sys_close((uint32_t)fd);
	set_fs(old_fs);
	return ret;
}

#ifdef CONFIG_DFX_DEBUG_FS
/*
 * @brief      : read input test file funciton
 * @param[in]  : fullname, the full path name should be read
 * @param[out] : buffer, output the data has been read
 * @param[in]  : offset, the offset in this file started to read
 * @param[in]  : size, the count bytes should be read.if zero means read total file
 * @return     : ::int, MSPC_OK for success,other value for failure.
 */
int mspc_read_input_test_file(const char *fullname, char *buffer, size_t offset,
			      size_t size)
{
	struct file *fp = NULL;
	int ret;
	ssize_t cnt;
	size_t read_bytes;
	loff_t pos;
	mm_segment_t old_fs;

	if (!fullname || !buffer) {
		pr_err("%s(): passed ptr is NULL\n", __func__);
		return MSPC_INVALID_PARAM_ERROR;
	}

	if (size == 0 || size > MSPC_MAX_IMG_SIZE) {
		pr_err("%s(): passed size is invalid\n", __func__);
		return MSPC_INVALID_PARAM_ERROR;
	}

	old_fs = get_fs();
	/* the kernel API generate pclint warning, ignore it */
	set_fs(KERNEL_DS); /*lint !e501*/
	fp = filp_open(fullname, O_RDONLY, MSPC_FILESYS_DEFAULT_MODE);
	if (IS_ERR(fp)) {
		pr_err("%s():open %s failed\n", __func__, fullname);
		ret = MSPC_OPEN_FILE_ERROR;
		goto out;
	}
	ret = vfs_llseek(fp, 0L, SEEK_END);
	if (ret < 0) {
		pr_err("%s():lseek %s failed from end.\n", __func__, fullname);
		ret = MSPC_LSEEK_FILE_ERROR;
		goto close;
	}
	pos = fp->f_pos;
	if ((offset + size) > (size_t)pos) {
		pr_err("%s(): offset(%lx), size(%lx) both invalid.\n",
		       __func__, offset, size);
		ret = MSPC_FS_OPEN_FILE_ERROR;
		goto close;
	}
	ret = vfs_llseek(fp, offset, SEEK_SET);
	if (ret < 0) {
		pr_err("%s():lseek %s failed from begin.\n",
		       __func__, fullname);
		ret = MSPC_LSEEK_FILE_ERROR;
		goto close;
	}

	read_bytes = size;
	cnt = vfs_read(fp, (char __user *)buffer, read_bytes, &fp->f_pos);
	if (cnt < (ssize_t)read_bytes) {
		pr_err("%s():read %s failed, return [%ld]\n",
		       __func__, fullname, cnt);
		ret = MSPC_ACCESS_FILE_ERROR;
		goto close;
	}
	ret = MSPC_OK;
close:
	filp_close(fp, NULL);
out:
	set_fs(old_fs);
	return ret;
}
#endif
