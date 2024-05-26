/*
 * drv_venc_ipc.c
 *
 * This is for VENC memory map.
 *
 * Copyright (c) 2021-2021 Huawei Technologies CO., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef _VRCE_ITCM_MAP_H_
#define _VRCE_ITCM_MAP_H_

void *venc_regbase(void);
#define VEDU_REG_BASE                   ((char *)venc_regbase())
#define VEDU_FUNC_READBACK_BASE         (VEDU_REG_BASE + 0x3000)
#define VEDU_MCU_CTRL_BASE              (VEDU_REG_BASE + 0x8000)
#define VEDU_AXIDFX_BASE                (VEDU_REG_BASE + 0x19000)
#define VEDU_SUB_CTRL_BASE              (VEDU_REG_BASE + 0x24000)
#define VEDU_IPC_BASE                   (VEDU_REG_BASE + 0x29000)
#define VRCE_SRAM_BASE                  (VEDU_REG_BASE + 0x30000)

/* ITCM: 0-24K used for code+stack; 24k-32k used for DFX logs */
#define VRCE_SRAM_SIZE                               (0x8000)
#define VRCE_SRAM_END                               (VRCE_SRAM_BASE + VRCE_SRAM_SIZE)
/* ITCM image area, code is growing upwards */
#define VRCE_SRAM_ROM_SIZE                          (0x400)
#define ENTRY_ADDR                                  (VRCE_SRAM_BASE + 0x280)
#define VRCE_CODE_BASE                      (VRCE_SRAM_BASE + VRCE_SRAM_ROM_SIZE)
#define VRCE_CODE_SIZE                      (0x5000)

/* ITCM stack area, stack is growing downwards */
#define VRCE_STACK_SIZE                      (0x600)
#define VRCE_STACK_BOTTOM                    (VRCE_SRAM_BASE+0x6000)
#define VRCE_STACK_BASE                      (VRCE_STACK_BOTTOM - VRCE_STACK_SIZE)
#define VRCE_STACK_BOTTOM_PROTECT                (VRCE_STACK_BOTTOM)
#define VRCE_STACK_TOP_PROTECT                   (VRCE_STACK_BASE)

/* ITCM storage area, used for dfx and logs */
#define VRCE_STORAGE_AREA_BASE                       (VRCE_STACK_BOTTOM)

#define VRCE_INT_TRACK_SIZE                          (0x240)
#define VRCE_INT_TRACK_BASE                          (VRCE_STORAGE_AREA_BASE)

#define VRCE_REG_BACKUP_SIZE                         (0x1C0)
#define VRCE_REG_BACKUP_BASE                         (VRCE_INT_TRACK_BASE + VRCE_INT_TRACK_SIZE)

/* ITCM IPC prameters memory shared with AP */
#define VRCE_FLAG_MEM_SIZE                           (0x100)
#define VRCE_FLAG_MEM_BASE                           (VRCE_REG_BACKUP_BASE + VRCE_REG_BACKUP_SIZE)

#define VRCE_PRINT_LOG_SIZE                          (0x400)
#define VRCE_PRINT_LOG_BASE                          (VRCE_FLAG_MEM_BASE + VRCE_FLAG_MEM_SIZE)
#define VRCE_ERR_LOG_SIZE                            (0x120)
#define VRCE_ERR_LOG_BASE                            (VRCE_PRINT_LOG_BASE + VRCE_PRINT_LOG_SIZE)

#endif