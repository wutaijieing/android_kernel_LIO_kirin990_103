/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2017-2021. All rights reserved.
 * Description: hhee sandbox
 * Create : 2017/12/6
 */

#include <linux/linkage.h>
#include <linux/version.h>

	.hidden sandbox_vector_base
	.align 11
sandbox_vector_base:
	.rept 32 * 15
	nop
	.endr
	stp	x29, x30, [sp, #-0x20]!
	str	x19, [sp, #0x10]

	mov	x0, #-4			/* Indicate an exception occured */
	mrs	x19, tpidr_el1
	msr	elr_el1, x19

	ldr	x19, [sp, #0x10]
	ldp	x29, x30, [sp], #0x20
	eret

/*
 * void *sandbox_try_execute(void *(*func)(void *), void *opaque);
 *
 * Tries to execute 'return func(opaque);'.
 * On failure, returns ERR_PTR(-EINTR)
 *
 * WARNING: Interrupts MUST be disabled when calling this function.
 */
	.global sandbox_try_execute
sandbox_try_execute:
	stp	x29, x30, [sp, #-0x20]!
	stp	x19, x20, [sp, #0x10]
	mrs	x19, vbar_el1
	mrs	x20, tpidr_el1

	adr	x9, sandbox_vector_base
	adr	x10, 1f
	msr	vbar_el1, x9
	msr	tpidr_el1, x10
	isb

	mov	x2, x0
	mov	x0, x1
	blr	x2
1:
	msr	vbar_el1, x19
	msr	tpidr_el1, x20
	isb
	ldp	x19, x20, [sp, #0x10]
	ldp	x29, x30, [sp], #0x20
	ret

/*
 * int el1_text_sandbox(int)
 *
 * If argument is zero, fall through body of noop's and return 2.
 * Otherwise increment argument by one and return.
 */
	.global el1_text_sandbox
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
SYM_FUNC_START(el1_text_sandbox)
	.align 16		/* align to 64K */
#else
	.type el1_text_sandbox, STT_FUNC
	.func el1_text_sandbox, el1_text_sandbox
	.align 16		/* align to 64K */
el1_text_sandbox:
#endif
	cmp w0, #0x0		/* is first argument 0 */
	b.eq 1f			/* if so jump to 1: */
	add w0, w0, #0x1	/* otherwise, add 1 to the argument */
	ret			/* return */
1:
	mov w0, #0x2		/* set return value to '2' */
	.rept 1024 * 4          /* reserve 4K for test */
	nop			/* lot of noops */
	.endr
	ret			/* return */
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
SYM_FUNC_END(el1_text_sandbox)
#else
	.endfunc
#endif
