/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2017-2021. All rights reserved.
 * Description: hkip hvc
 * Create : 2017/12/6
 */

#include <asm/alternative.h>
#include <linux/linkage.h>
#include <linux/version.h>

	.globl	hkip_hvc2
	.globl	hkip_hvc3
	.globl	hkip_hvc4

	.text
	.align	3
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
SYM_FUNC_START(hkip_hvc)
#else

	.type	hkip_hvc, %function
	.func	hkip_hvc
hkip_hvc:
#endif
hkip_hvc2:
hkip_hvc3:
hkip_hvc4:
alternative_if_not ARM64_HAS_HHEE
	b	hkip_hvc_dispatch
alternative_else
	hvc	#0
alternative_endif
	ret
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
SYM_FUNC_END(hkip_hvc)
#else
	.endfunc
#endif
