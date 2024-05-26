/*
 *  Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
 *  Description: enable hhee vmmu for armpc
 *  Create : 2020/4/26
 */

#include <asm/alternative.h>
#include <linux/linkage.h>

ENTRY(hhee_vmmu_enable)
	mov	x13, x0
	mov	x14, x1
	mov	x15, x2
	mov	x16, x3
	mov	x17, x4

	movz	w0, #0x1091
	movk	w0, #0xC600, lsl #16
	hvc	#0

	mov	x0, x13
	mov	x1, x14
	mov	x2, x15
	mov	x3, x16
	mov	x4, x17
	ret
ENDPROC(hhee_vmmu_enable)
