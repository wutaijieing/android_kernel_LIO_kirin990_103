/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
 * Description: Hisi fbex ca header files
 * Create : 2020/01/02
 */

#ifndef __HISI_FBEX_DERVER_H_
#define __HISI_FBEX_DERVER_H_

#include "fbex_dev.h"

#include <teek_client_type.h>
#include <linux/types.h>
#include <platform_include/see/fbe_ctrl.h>

#define FILE_DE        0
#define FILE_CE        1
#define FILE_ECE       2
#define FILE_SECE      3
#define FILE_GLOBAL_DE 4

#define FBEX_MAX_UFS_SLOT 32
#define FBEX_FILE_LEN     0x8
#define FBEX_FILE_MASK    0xff
#ifndef CONFIG_FBE_FPGA
#define FBEX_SECE_TIMEOUT 4000 /* ms */
#define FBEX_KEY_TIMEOUT  60000 /* ms */
#define FBEX_LOCK_TIMEOUT 1000 /* ms */
#else
/* These time are increased because of FPGA slow time */
#define FBEX_SECE_TIMEOUT 6000 /* ms */
#define FBEX_KEY_TIMEOUT  20000 /* ms */
#define FBEX_LOCK_TIMEOUT 2000 /* ms */
#endif
#define FBEX_DELAY_MSP    500  /* ms */
#define FBEX_DELAY_POWER  4000 /* ms */
#define MSP_ONLINE        0x78
#define MSP_OFFLINE       0x87
#define MSP_TIMER_CNT     10  /* Keep MSP power On 50s */

/* error ID 0xFBE302XX is error ID for kernel */
#define FBE3_ERROR_CMD_START          0xFBE30200
#define FBE3_ERROR_CMD_INVALID        0xFBE30201
#define FBE3_ERROR_CMD_UNSUPPORT      0xFBE30202
#define FBE3_ERROR_CMD_UNDEFINED      0xFBE30203
#define FBE3_ERROR_SECE_TIMEER_OUT    0xFBE30204
#define FBE3_ERROR_CKEY_TIMEER_OUT    0xFBE30205
#define FBE3_ERROR_DEBUG_TIMEER_OUT   0xFBE30206
#define FBE3_ERROR_BUFFER_NULL        0xFBE30207
#define FBE3_ERROR_COPY_FAIL          0xFBE30208
#define FBE3_ERROR_INIT_INFO          0xFBE30209
#define FBE3_ERROR_INIT_BUFFER        0xFBE3020A
#define FBE3_ERROR_IV_BUFFER          0xFBE3020B
#define FBE3_ERROR_SLOT_ID            0xFBE3020C
#define FBE3_ERROR_BUFFER_INVALID     0xFBE3020D
#define FBE3_ERROR_NULL_POINTER       0xFBE3020E
#define FBE3_ERROR_INVALID_IV         0xFBE3020F

int fbex_init_driver(void);
void file_encry_init_sece(void);
void file_encry_record_error(u32 cmd, u32 user, u32 file, u32 error);
u32 file_encry_unsupported(u32 user, u32 file, u8 *iv, u32 iv_len);
u32 file_encry_undefined(u32 user, u32 file, u8 *iv, u32 iv_len);
u32 file_encry_add_iv(u32 user, u32 file, u8 *iv, u32 iv_len);
u32 file_encry_delete_iv(u32 user, u32 file, u8 *iv, u32 iv_len);
u32 file_encry_lock_screen(u32 user, u32 file, u8 *iv, u32 iv_len);
u32 file_encry_unlock_screen(u32 user, u32 file, u8 *iv, u32 iv_len);
u32 file_encry_logout_iv(u32 user, u32 file, u8 *iv, u32 iv_len);
u32 file_encry_status_report(u32 user, u32 file, u8 *iv, u32 iv_len);

u32 fbex_ca_lock_screen(u32 user, u32 file);
u32 fbex_ca_unlock_screen(u32 user, u32 file, u8 *iv, u32 iv_len);
bool fbex_is_msp_online(void);
void fbex_init_slot(u32 user, u32 file, u8 slot);
void fbex_deinit_slot(u32 user, u32 file);
void fbex_work_finish(u32 ret);
bool hisi_fbex_is_preload(void);
void hisi_fbex_preloading_msp(bool flag);
void hisi_fbex_set_preload(bool flag);

typedef u32 (*file_encry_cb)(u32 user, u32 file, u8 *iv, u32 iv_len);

#define CALLBACK_FN(num, name) [num] = file_encry_##name,

#define FILE_ENCRY_LIST                  \
	CALLBACK_FN(0x00, undefined)     \
	CALLBACK_FN(0x01, add_iv)        \
	CALLBACK_FN(0x02, delete_iv)     \
	CALLBACK_FN(0x03, lock_screen)   \
	CALLBACK_FN(0x04, unlock_screen) \
	CALLBACK_FN(0x05, unsupported)   \
	CALLBACK_FN(0x06, unsupported)   \
	CALLBACK_FN(0x07, unsupported)   \
	CALLBACK_FN(0x08, logout_iv)     \
	CALLBACK_FN(0x09, unsupported)   \
	CALLBACK_FN(0x0A, unsupported)   \
	CALLBACK_FN(0x0B, unsupported)   \
	CALLBACK_FN(0x0C, status_report) \
	CALLBACK_FN(0x0D, undefined)     \
	CALLBACK_FN(0x0E, undefined)     \
	CALLBACK_FN(0x0F, undefined)
#endif /* __HISI_FBEX_DERVER_H_ */
