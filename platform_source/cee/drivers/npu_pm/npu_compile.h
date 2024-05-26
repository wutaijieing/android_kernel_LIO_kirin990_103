/*
 * npu_compile.h
 *
 * npu compile info
 *
 * Copyright (C) 2021-2021 Huawei Technologies Co., Ltd.
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

#ifndef __NPU_COMPILE_H__
#define __NPU_COMPILE_H__

#ifndef __asmeq
#define __asmeq(x, y)  ".ifnc " x "," y " ; .err ; .endif\n\t"
#endif

#endif
