/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2020. All rights reserved.
 * Description: This file is for dynamic code upgrade process, including send
 *              dynamic data to ATF by SMC call, read encrypted dynamic
 *              data from RPMB.
 * Create: 2018-03-12
 */

#ifndef GENERAL_SEE_DCS_H
#define GENERAL_SEE_DCS_H
#include "general_see_fs.h"

/* Error number for dynamic data process. */
#define HISEE_DCS_IMAGECNT_ERROR       (-10050)
#define HISEE_DCS_MEM_FREE_ERROR       (-10051)
#define HISEE_DCS_MEM_ALLOC_ERROR      (-10052)
#define HISEE_DCS_COS_ID_ERROR         (-10053)
#define HISEE_DCS_INPUT_PARAM_ERROR    (-10054)
#define HISEE_DCS_IDX_ERROR            (-10055)
#define HISEE_DCS_ID_LARGE_ERROR       (-10056)

/* For current usage, we assume that 10 dynamic data files at most. */
#define HISEE_MAX_DCS_FILES             10

struct dcs_cnt_header {
	unsigned int dcs_image_cnt;
	unsigned int dcs_image_size[HISEE_MAX_DCS_FILES];
};

#define HISEE_DCS_FREE_CNT              1
#define HISEE_DCS_COUNT_HEADER          (sizeof(struct dcs_cnt_header))
#define HISEE_DCS_FILE_HEADER           12
#define HISEE_DCS_FILE_ROUND_SIZE       16
#define HISEE_DCS_FILE_MAX_SIZE         (96 * SIZE_1K)
#define HISEE_DCS_MAX_IMG_SIZE    round_up(HISEE_DCS_FILE_MAX_SIZE + HISEE_DCS_FILE_HEADER, \
	HISEE_DCS_FILE_ROUND_SIZE)
#define hisee_dcs_buff_size(cnt)  round_up(((HISEE_DCS_MAX_IMG_SIZE * (cnt)) + \
	HISEE_DCS_COUNT_HEADER + \
	HISEE_ATF_MESSAGE_HEADER_LEN), \
	PAGE_SIZE)
#define HISEE_ATF_DCS_TIMEOUT           1000000

#define HISEE_IMG_DCS_NAME              "DCS0_00"
#define HISEE_IMG_DCS_NAME_IDX          3
#define HISEE_IMG_DCS_COS_ID_OFF        3
#define HISEE_IMG_DCS_IDX_OFF           5
#define HISEE_MAX_DCS_ID_NUMBER         (HISEE_MAX_DCS_FILES)

/* check ret is ok or otherwise goto err_process */
#define dcs_check_result_and_return_void(ret) do { \
	if ((ret) != HISEE_OK) { \
		pr_err("hisee:%s() run failed,line=%d.\n", __func__, __LINE__);\
		return; \
	} \
} while (0)

/* check ret is ok or otherwise goto err_process */
#define dcs_check_result_and_return(ret) do { \
	if ((ret) != HISEE_OK) { \
		pr_err("hisee:%s() run failed,line=%d.\n", __func__, __LINE__); \
		return (ret); \
	} \
} while (0)

/*
 * @brief      : parse_dcs_id_from_name : get the dynamic image number
 *               from hisee_img.
 * @param[in]  : curr_file_info: the dynamic image name get from hisee_img.
 * @param[in]  : dcs_image_cnt_array: the result of dynamic image number.
 * @return     : HISEE_OK is success, others are failure.
 */
int parse_dcs_id_from_name(const struct img_file_info *curr_file_info,
			   unsigned int *dcs_image_cnt_array);

/*
 * @brief      : hisee_dcs_read : power up hisee and send dynamic data to atf.
 * @param[in]  : cos_id:the cos index for current dynamic data upgrade process.
 * @return     : HISEE_OK is success, others are failure.
 * @note       : It will call hisee_dcs_read to read all dynamic data from
 *               hisee_img, and then send those data to ATF by SMC call.
 */
int hisee_dcs_read(unsigned int cos_id);

/*
 * @brief      : hisee_dcs_data_load : load the dynamic data to atf.

 */
void hisee_dcs_data_load(void);

/*
 * @brief      : hisee_cos_dcs_upgrade :
 *               power up hisee and send dynamic data to atf.
 * @param[in]  : buf: the buf contain information of cos index and
 *               processor id for hisee power up.
 * @return     : void
 * @note       : It will call hisee_dcs_read to read all dynamic data from
 *               hisee_img, and then send those data to ATF by SMC call.
 *               After that, the hisee will be powered on, it will send
 *               a request for dynamic data upgrade to ATF, then ATF will
 *               send the dynamic data to hisee by IPC&Mailbox.
 *               When all dynamic data is reay, hisee will set the
 *               state of hisee to HISEE_STATE_DCS_UPGRADE_DONE state
 *               to indicate the dynamic data is upgrade completed.
 */
void hisee_cos_dcs_upgrade(const void *buf);

/*
 * @brief      : hisee_free_dcs_mem : free the cma memory allocated for dynamic
 *               data upgrade when it upgrade completed.
 */
void hisee_free_dcs_mem(void);

#endif
