/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
 * Description: Low-level test harness for HKIP
 * Date: 2020/05/04
 */

#include <linux/linkage.h>
#include <asm/assembler.h>
#include <linux/version.h>
#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 4, 0))
#include <asm/compiler.h>
#endif
#include <asm/errno.h>

#define STACK_ALIGN(x)	((((x) + 15) >> 4) << 4)

	.macro	hkip_bad_mode
	.align	7
	b	vectors + (. - hkip_test_vector)
	.endm

	.macro	hkip_good_mode
	.align	7
	/* Restore potentially offset SP */
	mrs	x9, sp_el0
	mov	sp, x9
	/* Restore potentially clobbered callee-saved registers */
	ldp	x27, x28, [sp, #0x70]
	ldp	x25, x26, [sp, #0x60]
	ldp	x23, x24, [sp, #0x50]
	ldp	x21, x22, [sp, #0x40]
	ldp	x19, x20, [sp, #0x30]
	ldr	x18, [sp, #0x28]

	mrs	x0, esr_el1
	mov	x1, xzr
	b	hkip_test_exception
	.endm

	.text
	.align	11
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
SYM_FUNC_START(hkip_test_vector)
#else
ENTRY(hkip_test_vector)
#endif
	// Kernel thread exception:
	hkip_good_mode
	// Kernel thread IRQ:
	hkip_bad_mode
	// Kernel thread FIQ:
	hkip_bad_mode
	// Kernel thread SError:
	hkip_bad_mode
	// Kernel handler exception:
	hkip_good_mode
	// Kernel handler IRQ:
	hkip_bad_mode
	// Kernel handler FIQ:
	hkip_bad_mode
	// Kernel handler SError:
	hkip_bad_mode
	// 64-bit user exception:
	hkip_good_mode
	// 64-bit user IRQ:
	hkip_bad_mode
	// 64-bit user FIQ:
	hkip_bad_mode
	// 64-bit user SError:
	hkip_bad_mode
	// 32-bit user exception:
	hkip_bad_mode
	// 32-bit user IRQ:
	hkip_bad_mode
	// 32-bit user FIQ:
	hkip_bad_mode
	// 32-bit user SErrorr:
	hkip_bad_mode
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
SYM_FUNC_END(hkip_test_vector)

SYM_FUNC_START(hkip_test_run)
#else
END(hkip_test_vector)

ENTRY(hkip_test_run)
#endif
	stp	x29, x30, [sp, #-0x80]!
	mov	x29, sp
	stp	x19, x20, [sp, #0x10]
	str	x21, [sp, #0x20]

	/* Disable interrupts and override the exception vector */
	mrs	x19, daif
	msr	daifset, #3
	mrs	x20, vbar_el1
	adr	x9, hkip_test_vector
	msr	vbar_el1, x9
	isb

	/* Save callee-saved registers and SP in case of exception */
	mov	x9, sp
	mrs	x21, sp_el0
	msr	sp_el0, x9
	str	x18, [sp, #0x28]
	stp	x19, x20, [sp, #0x30]
	stp	x21, x22, [sp, #0x40]
	stp	x23, x24, [sp, #0x50]
	stp	x25, x26, [sp, #0x60]
	stp	x27, x28, [sp, #0x70]

	/* Call the test function */
	blr	x8

	/* Test passed without exception, return 0 */
	mov	x1, x0
	mov	x0, xzr

hkip_test_exception:
	/* Restore the exception vector, interrupts and SP_EL0 */
	msr	sp_el0, x21
	msr	vbar_el1, x20
	isb
	msr	daif, x19

	ldr	x21, [sp, #0x20]
	ldp	x19, x20, [sp, #0x10]
	ldp	x29, x30, [sp], #0x80
	ret
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
SYM_FUNC_END(hkip_test_run)

SYM_FUNC_START(hkip_sysreg_set_wrapper)
#else
END(hkip_test_run)

ENTRY(hkip_sysreg_set_wrapper)
#endif
	stp	x29, x30, [sp, #-0x10]!
	mov	x29, sp

	adrp	x8, hkip_sysreg_set
	add	x8, x8, #:lo12:hkip_sysreg_set
	bl	hkip_test_run

	mov	x9, #-EACCES
	cmp	x0, #0
	csel	x0, x9, xzr, ne

	ldp	x29, x30, [sp], #0x10
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
SYM_FUNC_END(hkip_sysreg_set_wrapper)
#else
END(hkip_sysreg_set_wrapper)
#endif

#define HKIP_DATA_SIZE	8

	.align	16
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
SYM_FUNC_START(hkip_text)
#else
ENTRY(hkip_text)
#endif
	ret
	svc	#1234
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
SYM_FUNC_END(hkip_text)
#else
END(hkip_text)
#endif

	.pushsection	.rodata, "a"
	.align	3
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
SYM_FUNC_START(hkip_rodata)
#else
ENTRY(hkip_rodata)
#endif
	ret
	svc	#1234
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
SYM_FUNC_END(hkip_rodata)
#else
END(hkip_rodata)
#endif
	.popsection

	.pushsection	.data
	.align	3
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
SYM_FUNC_START(hkip_data)
#else
ENTRY(hkip_data)
#endif
	ret
	svc	#1234
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
SYM_FUNC_END(hkip_data)
#else
END(hkip_data)
#endif
	.popsection

#define HKIP_DATA_STACKSIZE	((HKIP_DATA_SIZE + 15) & ~15)

/* Same as memcpy() but safe if x0 equals x1 */
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
SYM_FUNC_START(hkip_memcpy)
#else
ENTRY(hkip_memcpy)
#endif
	add	x2, x0, x2
	b	2f
1:	ldrb	w9, [x1], #1
	strb	w9, [x0], #1
2:	cmp	x0, x2
	b.ne	1b
	ret
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
SYM_FUNC_END(hkip_memcpy)

SYM_FUNC_START(hkip_memcpy_from_user)
#else
END(hkip_memcpy)

ENTRY(hkip_memcpy_from_user)
#endif
	add	x2, x0, x2
	b	2f
1:	ldtrb	w9, [x1]
	strb	w9, [x0], #1
	add	x1, x1, #1
2:	cmp	x0, x2
	b.ne	1b
	ret
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
SYM_FUNC_END(hkip_memcpy_from_user)

SYM_FUNC_START(hkip_memcpy_to_user)
#else
END(hkip_memcpy_from_user)

ENTRY(hkip_memcpy_to_user)
#endif
	add	x2, x0, x2
	b	2f
1:	ldrb	w9, [x1], #1
	sttrb	w9, [x0]
	add	x0, x0, #1
2:	cmp	x0, x2
	b.ne	1b
	ret
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
SYM_FUNC_END(hkip_memcpy_to_user)

SYM_FUNC_START(hkip_mem_readable_common)
#else
END(hkip_memcpy_to_user)

ENTRY(hkip_mem_readable_common)
#endif
	stp	x29, x30, [sp, #-(0x20 + HKIP_DATA_STACKSIZE)]!
	mov	x29, sp
	stp	x19, x20, [sp, #0x10]

	mov	x19, x0
	mov	x20, x8

	/* Initialise value with garbage so comparison can be relied on */
	mov	x2, #HKIP_DATA_SIZE
	mov	x1, #0x41 // 'A'
	add	x0, sp, #0x20
	bl	memset

	/* Copy memory with harness */
	mov	x2, #HKIP_DATA_SIZE
	mov	x1, x19
	add	x0, sp, #0x20
	mov	x8, x20
	bl	hkip_test_run
	cmp	x0, #0
	mov	x0, xzr
	b.ne	1f

	/* Check that value is as expected */
	add	x0, sp, #0x20
	adrp	x1, hkip_rodata
	add	x1, x1, #:lo12:hkip_rodata
	mov	x2, #HKIP_DATA_SIZE
	bl	memcmp
	cmp	x0, #0
	mov	x0, #1
	cneg	x0, x0, ne
1:
	ldp	x19, x20, [sp, #0x10]
	ldp	x29, x30, [sp], #(0x20 + HKIP_DATA_STACKSIZE)
	ret
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
SYM_FUNC_END(hkip_mem_readable_common)

SYM_FUNC_START(hkip_mem_writable_common)
#else
END(hkip_mem_readable_common)

ENTRY(hkip_mem_writable_common)
#endif
	stp	x29, x30, [sp, #-0x10]!
	mov	x29, sp

	/* Copy memory with harness */
	mov	x2, #HKIP_DATA_SIZE
	adrp	x1, hkip_rodata
	add	x1, x1, #:lo12:hkip_rodata
	bl	hkip_test_run
	cmp	x0, #0
	cset	x0, eq

	ldp	x29, x30, [sp], #0x10
	ret
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
SYM_FUNC_END(hkip_mem_writable_common)

SYM_FUNC_START(hkip_mem_readable)
#else
END(hkip_mem_writable_common)

ENTRY(hkip_mem_readable)
#endif
	adr	x8, hkip_memcpy
	b	hkip_mem_readable_common
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
SYM_FUNC_END(hkip_mem_readable)

SYM_FUNC_START(hkip_mem_writable)
#else
END(hkip_mem_readable)

ENTRY(hkip_mem_writable)
#endif
	adr	x8, hkip_memcpy
	b	hkip_mem_writable_common
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
SYM_FUNC_END(hkip_mem_writable)

SYM_FUNC_START(hkip_mem_executable)
#else
END(hkip_mem_writable)

ENTRY(hkip_mem_executable)
#endif
	stp	x29, x30, [sp, #-0x10]!
	mov	x29, sp

	mov	x8, x0
	bl	hkip_test_run
	cmp	x0, #0
	cset	x0, eq

	ldp	x29, x30, [sp], #0x10
	ret
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
SYM_FUNC_END(hkip_mem_executable)

SYM_FUNC_START(hkip_mem_user_readable)
#else
END(hkip_mem_executable)

ENTRY(hkip_mem_user_readable)
#endif
	adr	x8, hkip_memcpy_from_user
	b	hkip_mem_readable_common
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
SYM_FUNC_END(hkip_mem_user_readable)

SYM_FUNC_START(hkip_mem_user_writable)
#else
END(hkip_mem_user_readable)

ENTRY(hkip_mem_user_writable)
#endif
	adr	x8, hkip_memcpy_to_user
	b	hkip_mem_writable_common
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
SYM_FUNC_END(hkip_mem_user_writable)

SYM_FUNC_START(hkip_mem_user_executable)
#else
END(hkip_mem_user_writable)

ENTRY(hkip_mem_user_executable)
#endif
	stp	x29, x30, [sp, #-0x10]!
	mov	x29, sp

	adr	x8, 1f
	bl	hkip_test_run

	ubfx	x2, x0, #25, #7	// extract ESR.EC and ESR.IL
	ubfx	x3, x0, #0, #16	// extract SVC #imm16
	cmp	x2, #0x2b	// EC == 0x15 (A64 SVC), IL == 1 (4 bytes)
	cset	x0, eq
	cmp	x3, #1234	// imm16 == 1234
	csel	x0, x0, xzr, eq

	ldp	x29, x30, [sp], #0x10
	ret
1:
	add	x0, x0, #4 /* skip RET instruction to SVC */
	mov	x9, #(PSR_I_BIT | PSR_F_BIT | PSR_MODE_EL0t)
	msr	elr_el1, x0
	msr	spsr_el1, x9
	eret
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
SYM_FUNC_END(hkip_mem_user_executable)

SYM_FUNC_START(hkip_literal_read)
#else
END(hkip_mem_user_executable)

ENTRY(hkip_literal_read)
#endif
	stp	x29, x30, [sp, #-0x10]!
	mov	x29, sp

	adr	x8, 1f
	bl	hkip_test_run

	ldp	x29, x30, [sp], #0x10
	ret

1:	/* Read a literal */
	ldr	x1, hkip_literal
	str	x1, [x0]
	ret
hkip_literal:
	.quad	0x0123456789ABCDEF
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
SYM_FUNC_END(hkip_literal_read)

SYM_FUNC_START(hkip_literal_read_user)
#else
END(hkip_literal_read)

ENTRY(hkip_literal_read_user)
#endif
	stp	x29, x30, [sp, #-0x10]!
	mov	x29, sp

	adr	x8, 1f
	bl	hkip_test_run

	ldp	x29, x30, [sp], #0x10
	ret
1:
	adr	x0, 2f
	mov	x9, #(PSR_I_BIT | PSR_F_BIT | PSR_MODE_EL0t)
	msr	elr_el1, x0
	msr	spsr_el1, x9
	eret
2:
	ldr	x1, hkip_literal
	str	x1, [x0]
	svc	#1234
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
SYM_FUNC_END(hkip_literal_read_user)
#else
END(hkip_literal_read_user)
#endif
