/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
 * npu autofs plat interface
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#ifndef __NPU_AUTOFS_PLAT_H
#define __NPU_AUTOFS_PLAT_H

void npu_autofs_npubus_on(void *autofs_crg_vaddr);
void npu_autofs_npubus_off(void *autofs_crg_vaddr);

void npu_autofs_npubuscfg_on(void *autofs_crg_vaddr);
void npu_autofs_npubuscfg_off(void *autofs_crg_vaddr);

void npu_autofs_npubus_aicore_on(void *autofs_crg_vaddr);
void npu_autofs_npubus_aicore_off(void *autofs_crg_vaddr);

#endif
