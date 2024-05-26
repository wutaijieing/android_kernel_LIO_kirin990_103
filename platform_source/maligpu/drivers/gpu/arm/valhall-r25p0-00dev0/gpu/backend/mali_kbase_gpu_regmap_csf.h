/*
 *
 * (C) COPYRIGHT 2019-2020 ARM Limited. All rights reserved.
 *
 * This program is free software and is provided to you under the terms of the
 * GNU General Public License version 2 as published by the Free Software
 * Foundation, and any use by you of this program is subject to the terms
 * of such GNU licence.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you can access it online at
 * http://www.gnu.org/licenses/gpl-2.0.html.
 *
 * SPDX-License-Identifier: GPL-2.0
 *
 */

#ifndef _KBASE_GPU_REGMAP_CSF_H_
#define _KBASE_GPU_REGMAP_CSF_H_

#if !MALI_USE_CSF
#error "Cannot be compiled with JM"
#endif


/* Ported from file gpu/generated/tODx/current/mali_gpu_csf_cshw_registers.h */
#define GPU_CONTROL_MCU 0x3000 /* () MCU control registers */
#define GPU_CONTROL_MCU_REG(r)  (GPU_CONTROL_MCU + (r))


/* Set to implementation defined, outer caching */
#define AS_MEMATTR_AARCH64_OUTER_IMPL_DEF 0x88ull
/* Set to write back memory, outer caching */
#define AS_MEMATTR_AARCH64_OUTER_WA       0x8Dull
/* Set to inner non-cacheable, outer-non-cacheable
 * Setting defined by the alloc bits is ignored, but set to a valid encoding:
 * - no-alloc on read
 * - no alloc on write
 */
#define AS_MEMATTR_AARCH64_NON_CACHEABLE  0x4Cull
/* Set to shared memory, that is inner cacheable on ACE and inner or outer
 * shared, otherwise inner non-cacheable.
 * Outer cacheable if inner or outer shared, otherwise outer non-cacheable.
 */
#define AS_MEMATTR_AARCH64_SHARED         0x8ull

/* Symbols for default MEMATTR to use
 * Default is - HW implementation defined caching
 */
#define AS_MEMATTR_INDEX_DEFAULT               0
#define AS_MEMATTR_INDEX_DEFAULT_ACE           3

/* HW implementation defined caching */
#define AS_MEMATTR_INDEX_IMPL_DEF_CACHE_POLICY 0
/* Force cache on */
#define AS_MEMATTR_INDEX_FORCE_TO_CACHE_ALL    1
/* Write-alloc */
#define AS_MEMATTR_INDEX_WRITE_ALLOC           2
/* Outer coherent, inner implementation defined policy */
#define AS_MEMATTR_INDEX_OUTER_IMPL_DEF        3
/* Outer coherent, write alloc inner */
#define AS_MEMATTR_INDEX_OUTER_WA              4
/* Normal memory, inner non-cacheable, outer non-cacheable (ARMv8 mode only) */
#define AS_MEMATTR_INDEX_NON_CACHEABLE         5
/* Normal memory, shared between MCU and Host */
#define AS_MEMATTR_INDEX_SHARED                6

/* Configuration bits for the Command Stream Frontend. */
#define CSF_CONFIG 0xF00

/* CSF_CONFIG register */
#define CSF_CONFIG_FORCE_COHERENCY_FEATURES_SHIFT 2

/* GPU control registers */
#define MCU_CONTROL             0x700
#define MCU_STATUS              0x704

#define MCU_CNTRL_ENABLE        (1 << 0)
#define MCU_CNTRL_AUTO          (1 << 1)
#define MCU_CNTRL_DISABLE       (0)

#define PRFCNT_BASE_LO   0x060  /* (RW) Performance counter memory
				 * region base address, low word
				 */
#define PRFCNT_BASE_HI   0x064  /* (RW) Performance counter memory
				 * region base address, high word
				 */
#define PRFCNT_CONFIG    0x068  /* (RW) Performance counter
				 * configuration
				 */

#define PRFCNT_CSHW_EN   0x06C  /* (RW) Performance counter
				 * enable for Command Stream Hardware
				 */

#define PRFCNT_SHADER_EN 0x070  /* (RW) Performance counter enable
				 * flags for shader cores
				 */
#define PRFCNT_TILER_EN  0x074  /* (RW) Performance counter enable
				 * flags for tiler
				 */
#define PRFCNT_MMU_L2_EN 0x07C  /* (RW) Performance counter enable
				 * flags for MMU/L2 cache
				 */

/* JOB IRQ flags */
#define JOB_IRQ_GLOBAL_IF       (1 << 31)   /* Global interface interrupt received */

/* GPU_COMMAND codes */
#define GPU_COMMAND_CODE_NOP                0x00 /* No operation, nothing happens */
#define GPU_COMMAND_CODE_RESET              0x01 /* Reset the GPU */
#define GPU_COMMAND_CODE_PRFCNT             0x02 /* Clear or sample performance counters */
#define GPU_COMMAND_CODE_TIME               0x03 /* Configure time sources */
#define GPU_COMMAND_CODE_FLUSH_CACHES       0x04 /* Flush caches */
#define GPU_COMMAND_CODE_SET_PROTECTED_MODE 0x05 /* Places the GPU in protected mode */
#define GPU_COMMAND_CODE_FINISH_HALT        0x06 /* Halt CSF */
#define GPU_COMMAND_CODE_CLEAR_FAULT        0x07 /* Clear GPU_FAULTSTATUS and GPU_FAULTADDRESS, TODX */

/* GPU_COMMAND_RESET payloads */

/* This will leave the state of active jobs UNDEFINED, but will leave the external bus in a defined and idle state.
 * Power domains will remain powered on.
 */
#define GPU_COMMAND_RESET_PAYLOAD_FAST_RESET 0x00

/* This will leave the state of active command streams UNDEFINED, but will leave the external bus in a defined and
 * idle state.
 */
#define GPU_COMMAND_RESET_PAYLOAD_SOFT_RESET 0x01

/* This reset will leave the state of currently active streams UNDEFINED, will likely lose data, and may leave
 * the system bus in an inconsistent state. Use only as a last resort when nothing else works.
 */
#define GPU_COMMAND_RESET_PAYLOAD_HARD_RESET 0x02

/* GPU_COMMAND_PRFCNT payloads */
#define GPU_COMMAND_PRFCNT_PAYLOAD_SAMPLE 0x01 /* Sample performance counters */
#define GPU_COMMAND_PRFCNT_PAYLOAD_CLEAR  0x02 /* Clear performance counters */

/* GPU_COMMAND_TIME payloads */
#define GPU_COMMAND_TIME_DISABLE 0x00 /* Disable cycle counter */
#define GPU_COMMAND_TIME_ENABLE  0x01 /* Enable cycle counter */

/* GPU_COMMAND_FLUSH_CACHES payloads */
#define GPU_COMMAND_FLUSH_PAYLOAD_NONE             0x00 /* No flush */
#define GPU_COMMAND_FLUSH_PAYLOAD_CLEAN            0x01 /* Clean the caches */
#define GPU_COMMAND_FLUSH_PAYLOAD_INVALIDATE       0x02 /* Invalidate the caches */
#define GPU_COMMAND_FLUSH_PAYLOAD_CLEAN_INVALIDATE 0x03 /* Clean and invalidate the caches */

/* GPU_COMMAND command + payload */
#define GPU_COMMAND_CODE_PAYLOAD(opcode, payload) \
	((u32)opcode | ((u32)payload << 8))

/* Final GPU_COMMAND form */
/* No operation, nothing happens */
#define GPU_COMMAND_NOP \
	GPU_COMMAND_CODE_PAYLOAD(GPU_COMMAND_CODE_NOP, 0)

/* Stop all external bus interfaces, and then reset the entire GPU. */
#define GPU_COMMAND_SOFT_RESET \
	GPU_COMMAND_CODE_PAYLOAD(GPU_COMMAND_CODE_RESET, GPU_COMMAND_RESET_PAYLOAD_SOFT_RESET)

/* Immediately reset the entire GPU. */
#define GPU_COMMAND_HARD_RESET \
	GPU_COMMAND_CODE_PAYLOAD(GPU_COMMAND_CODE_RESET, GPU_COMMAND_RESET_PAYLOAD_HARD_RESET)

/* Clear all performance counters, setting them all to zero. */
#define GPU_COMMAND_PRFCNT_CLEAR \
	GPU_COMMAND_CODE_PAYLOAD(GPU_COMMAND_CODE_PRFCNT, GPU_COMMAND_PRFCNT_PAYLOAD_CLEAR)

/* Sample all performance counters, writing them out to memory */
#define GPU_COMMAND_PRFCNT_SAMPLE \
	GPU_COMMAND_CODE_PAYLOAD(GPU_COMMAND_CODE_PRFCNT, GPU_COMMAND_PRFCNT_PAYLOAD_SAMPLE)

/* Starts the cycle counter, and system timestamp propagation */
#define GPU_COMMAND_CYCLE_COUNT_START \
	GPU_COMMAND_CODE_PAYLOAD(GPU_COMMAND_CODE_TIME, GPU_COMMAND_TIME_ENABLE)

/* Stops the cycle counter, and system timestamp propagation */
#define GPU_COMMAND_CYCLE_COUNT_STOP \
	GPU_COMMAND_CODE_PAYLOAD(GPU_COMMAND_CODE_TIME, GPU_COMMAND_TIME_DISABLE)

/* Clean all caches */
#define GPU_COMMAND_CLEAN_CACHES \
	GPU_COMMAND_CODE_PAYLOAD(GPU_COMMAND_CODE_FLUSH_CACHES, GPU_COMMAND_FLUSH_PAYLOAD_CLEAN)

/* Clean and invalidate all caches */
#define GPU_COMMAND_CLEAN_INV_CACHES \
	GPU_COMMAND_CODE_PAYLOAD(GPU_COMMAND_CODE_FLUSH_CACHES, GPU_COMMAND_FLUSH_PAYLOAD_CLEAN_INVALIDATE)

/* Places the GPU in protected mode */
#define GPU_COMMAND_SET_PROTECTED_MODE \
	GPU_COMMAND_CODE_PAYLOAD(GPU_COMMAND_CODE_SET_PROTECTED_MODE, 0)

/* Halt CSF */
#define GPU_COMMAND_FINISH_HALT \
	GPU_COMMAND_CODE_PAYLOAD(GPU_COMMAND_CODE_FINISH_HALT, 0)

/* Clear GPU faults */
#define GPU_COMMAND_CLEAR_FAULT \
	GPU_COMMAND_CODE_PAYLOAD(GPU_COMMAND_CODE_CLEAR_FAULT, 0)

/* End Command Values */

/* GPU_FAULTSTATUS register */
#define GPU_FAULTSTATUS_EXCEPTION_TYPE_SHIFT 0
#define GPU_FAULTSTATUS_EXCEPTION_TYPE_MASK (0xFFul)
#define GPU_FAULTSTATUS_ACCESS_TYPE_SHIFT 8
#define GPU_FAULTSTATUS_ACCESS_TYPE_MASK \
	(0x3ul << GPU_FAULTSTATUS_ACCESS_TYPE_SHIFT)

#define GPU_FAULTSTATUS_ADDR_VALID_SHIFT 10
#define GPU_FAULTSTATUS_ADDR_VALID_FLAG \
	(1ul << GPU_FAULTSTATUS_ADDR_VALID_SHIFT)

#define GPU_FAULTSTATUS_JASID_VALID_SHIFT 11
#define GPU_FAULTSTATUS_JASID_VALID_FLAG \
	(1ul << GPU_FAULTSTATUS_JASID_VALID_SHIFT)

#define GPU_FAULTSTATUS_JASID_SHIFT 12
#define GPU_FAULTSTATUS_JASID_MASK \
	(0xFul << GPU_FAULTSTATUS_JASID_SHIFT)

#define GPU_FAULTSTATUS_SOURCE_ID_SHIFT 16
#define GPU_FAULTSTATUS_SOURCE_ID_MASK \
	(0xFFFFul << GPU_FAULTSTATUS_SOURCE_ID_SHIFT)
/* End GPU_FAULTSTATUS register */

/* GPU_FAULTSTATUS.EXCEPTION_TYPE values */
#define GPU_EXCEPTION_TYPE_GPU_BUS_FAULT (0x80ul)

#endif /* _KBASE_GPU_REGMAP_CSF_H_ */
