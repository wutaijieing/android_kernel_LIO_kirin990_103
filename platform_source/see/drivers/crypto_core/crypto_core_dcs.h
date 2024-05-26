/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2020. All rights reserved.
 * Description: This file is for dynamic code upgrade process,
 *              including send dynamic data to ATF by SMC call,
 *              read encrypted dynamic data from RPMB.
 * Create: 2018-12-12
 */

#ifndef CRYPTO_CORE_DCS_H
#define CRYPTO_CORE_DCS_H

#include <linux/types.h>

#define MSPC_WAIT_DCS_DONE_TIMEOUT     30000

/* Error number for dynamic data process. */
#define MSPC_DCS_IMAGECNT_ERROR        (-10050)
#define MSPC_DCS_MEM_FREE_ERROR        (-10051)
#define MSPC_DCS_MEM_ALLOC_ERROR       (-10052)
#define MSPC_DCS_COS_ID_ERROR          (-10053)
#define MSPC_DCS_INPUT_PARAM_ERROR     (-10054)
#define MSPC_DCS_IDX_ERROR             (-10055)
#define MSPC_DCS_ID_LARGE_ERROR        (-10056)
#define MSPC_DCS_PARSER_IMG_ERROR      (-10057)

/* For current usage, we assume that 10 dynamic data files at most. */
#define MSPC_MAX_DCS_FILES             10

struct dcs_cnt_header {
	unsigned int dcs_image_cnt;
	unsigned int dcs_image_size[MSPC_MAX_DCS_FILES];
};

#define MSPC_DCS_FREE_CNT              1
#define MSPC_DCS_COUNT_HEADER          (sizeof(struct dcs_cnt_header))
#define MSPC_DCS_FILE_HEADER           12
#define MSPC_DCS_FILE_ROUND_SIZE       16
#define MSPC_DCS_FILE_MAX_SIZE         (96 * (SIZE_1K))
#define MSPC_DCS_MAX_IMG_SIZE          round_up(MSPC_DCS_FILE_MAX_SIZE + \
	MSPC_DCS_FILE_HEADER, \
	MSPC_DCS_FILE_ROUND_SIZE)
#define MSPC_DCS_BUFF_SIZE(cnt)     round_up(((MSPC_DCS_MAX_IMG_SIZE * (cnt)) + \
	MSPC_DCS_COUNT_HEADER + \
	MSPC_ATF_MESSAGE_HEADER_LEN), \
	PAGE_SIZE)
#define MSPC_ATF_DCS_TIMEOUT           1000000

#define MSPC_IMG_DCS_NAME_IDX          3
#define MSPC_IMG_DCS_COS_ID_OFF        3
#define MSPC_IMG_DCS_IDX_OFF           5
#define MSPC_MAX_DCS_ID_NUMBER         (MSPC_MAX_DCS_FILES)

/* check ret is ok or otherwise goto err_process */
#define dcs_check_result_and_return_void(ret) do { \
	if ((ret) != MSPC_OK) { \
		pr_err("mspc:%s() run failed,line=%d.\n", __func__, __LINE__);\
		return; \
	} \
} while (0)

/* check ret is ok or otherwise goto err_process */
#define dcs_check_result_and_return(ret) do { \
	if ((ret) != MSPC_OK) { \
		pr_err("mspc:%s() run failed,line=%d.\n", __func__, __LINE__); \
		return (ret); \
	} \
} while (0)

/*
 * @brief      : mspc_dcs_read : power up mspc and send dynamic data to atf.
 * @param[in]  : cos_id:the cos index for current dynamic data upgrade process.
 * @return     : MSPC_OK is success, others are failure.
 * @note       : It will call mspc_dcs_read to read all dynamic data from
 *               mspc_img, and then send those data to ATF by SMC call.
 */
int mspc_dcs_read(unsigned int cos_id);

/*
 * @brief      : mspc_dcs_data_load : load the dynamic data to atf.
 * @param[in]  : void
 * @return     : void
 */
void mspc_dcs_data_load(void);

/**
 * @brief      : mspc_cos_dcs_upgrade :
 *               power up mspc and send dynamic data to atf.
 * @param[in]  : buf: the buf contain infomation of cos index and
 *               processor id for mspc power up.
 * @return     : void
 * @note       : It will call mspc_dcs_read to read all dynamic data from
 *               mspc_img, and then send those data to ATF by SMC call.
 *               After that, the mspc will be powered on, it will send
 *               a request for dynamic data upgrade to ATF, then ATF will
 *               send the dynamic data to mspc by IPC&Mailbox.
 *               When all dynmaic data is reay, mspc will set the
 *               state of mspc to MSPC_STATE_DCS_UPGRADE_DONE state
 *               to indicate the dynamic data is upgrade completed.
 */
int32_t mspc_upgrade_dcs(const int8_t *para, uint32_t size);

/**
 * @brief      : mspc_free_dcs_mem : free the cma memory allocated for dynmaic
 *               data upgrade when it upgrade completed.
 * @param[in]  : void
 * @return     : void
 */
void mspc_free_dcs_mem(void);

#endif
