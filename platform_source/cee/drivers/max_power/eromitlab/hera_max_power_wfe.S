/*
 * hera_max_power_wfe.S
 *
 * es big cluster wfe max power
 *
 * Copyright (c) 2020-2020 Huawei Technologies Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */
//-----------------------------------------------------------------------------
// optimization is described below
// (1) load/store address stoggle
// (2) load/srote data stoggle
// (3) data of alu operation stoggle
// (4) some integer operation replaced by float convert operation
// (5) all breakpoint/watchpoint/etm/pmu are on
// (6) add a few of condition branch to make branch predictor run
//-----------------------------------------------------------------------------
.extern g_hera_data_pool_33; // 1M
.extern g_hera_data_pool_30; // 2M
.extern g_hera_data_pool_32; // 2M
.extern g_hera_data_pool_34; // 2M
.extern g_hera_outer_loop_count
.extern g_hera_inter_loop_count

.global hera_max_power_pi_wfe
.set MAXPWR_LOOP_COUNT, 32    // 32 post warmup. 64cyc/loop nominal

//; .global Start_test
hera_max_power_pi_wfe:
//; Save temporary registers and return address from LR
        stp     x1,  x2,  [sp,#-16]!
        stp     x3,  x4,  [sp,#-16]!
        stp     x5,  x6,  [sp,#-16]!
        stp     x7,  x8,  [sp,#-16]!
        stp     x9,  x10, [sp,#-16]!
        stp     x11, x12, [sp,#-16]!
        stp     x13, x14, [sp,#-16]!
        stp     x15, x16, [sp,#-16]!
        stp     x17, x18, [sp,#-16]!
        stp     x19, x20, [sp,#-16]!
        stp     x21, x22, [sp,#-16]!
        stp     x23, x24, [sp,#-16]!
        stp     x25, x26, [sp,#-16]!
        stp     x27, x28, [sp,#-16]!
        stp     x29, x30, [sp,#-16]!
        mov     x1,v0.d[0]
        mov     x2,v0.d[1]
        mov     x3,v1.d[0]
        mov     x4,v1.d[1]
        mov     x5,v2.d[0]
        mov     x6,v2.d[1]
        mov     x7,v3.d[0]
        mov     x8,v3.d[1]
        mov     x9,v4.d[0]
        mov     x10,v4.d[1]
        mov     x11,v5.d[0]
        mov     x12,v5.d[1]
        mov     x13,v6.d[0]
        mov     x14,v6.d[1]
        mov     x15,v7.d[0]
        mov     x16,v7.d[1]
        mov     x17,v8.d[0]
        mov     x18,v8.d[1]
        mov     x19,v9.d[0]
        mov     x20,v9.d[1]
        mov     x21,v10.d[0]
        mov     x22,v10.d[1]
        mov     x23,v11.d[0]
        mov     x24,v11.d[1]
        mov     x25,v12.d[0]
        mov     x26,v12.d[1]
        mov     x27,v13.d[0]
        mov     x28,v13.d[1]
        mov     x29,v14.d[0]
        mov     x30,v14.d[1]
        stp     x1,  x2,  [sp,#-16]!
        stp     x3,  x4,  [sp,#-16]!
        stp     x5,  x6,  [sp,#-16]!
        stp     x7,  x8,  [sp,#-16]!
        stp     x9,  x10, [sp,#-16]!
        stp     x11, x12, [sp,#-16]!
        stp     x13, x14, [sp,#-16]!
        stp     x15, x16, [sp,#-16]!
        stp     x17, x18, [sp,#-16]!
        stp     x19, x20, [sp,#-16]!
        stp     x21, x22, [sp,#-16]!
        stp     x23, x24, [sp,#-16]!
        stp     x25, x26, [sp,#-16]!
        stp     x27, x28, [sp,#-16]!
        stp     x29, x30, [sp,#-16]!
        mov     x1,v15.d[0]
        mov     x2,v15.d[1]
        mov     x3,v16.d[0]
        mov     x4,v16.d[1]
        mov     x5,v17.d[0]
        mov     x6,v17.d[1]
        mov     x7,v18.d[0]
        mov     x8,v18.d[1]
        mov     x9,v19.d[0]
        mov     x10,v19.d[1]
        mov     x11,v20.d[0]
        mov     x12,v20.d[1]
        mov     x13,v21.d[0]
        mov     x14,v21.d[1]
        mov     x15,v22.d[0]
        mov     x16,v22.d[1]
        mov     x17,v23.d[0]
        mov     x18,v23.d[1]
        mov     x19,v24.d[0]
        mov     x20,v24.d[1]
        mov     x21,v25.d[0]
        mov     x22,v25.d[1]
        mov     x23,v26.d[0]
        mov     x24,v26.d[1]
        mov     x25,v27.d[0]
        mov     x26,v27.d[1]
        mov     x27,v28.d[0]
        mov     x28,v28.d[1]
        mov     x29,v29.d[0]
        mov     x30,v29.d[1]
        stp     x1,  x2,  [sp,#-16]!
        stp     x3,  x4,  [sp,#-16]!
        stp     x5,  x6,  [sp,#-16]!
        stp     x7,  x8,  [sp,#-16]!
        stp     x9,  x10, [sp,#-16]!
        stp     x11, x12, [sp,#-16]!
        stp     x13, x14, [sp,#-16]!
        stp     x15, x16, [sp,#-16]!
        stp     x17, x18, [sp,#-16]!
        stp     x19, x20, [sp,#-16]!
        stp     x21, x22, [sp,#-16]!
        stp     x23, x24, [sp,#-16]!
        stp     x25, x26, [sp,#-16]!
        stp     x27, x28, [sp,#-16]!
        stp     x29, x30, [sp,#-16]!
        mov     x1,v30.d[0]
        mov     x2,v30.d[1]
        mov     x3,v31.d[0]
        mov     x4,v31.d[1]
        stp     x1,  x2,  [sp,#-16]!
        stp     x3,  x4,  [sp,#-16]!
// gpio
        mov x27, x0
// PMU
// Set up PMU so all counters count 'cycle' event
        LDR  x1, =0x11
        MSR PMEVTYPER0_EL0, x1
        MSR PMEVTYPER1_EL0, x1
        MSR PMEVTYPER2_EL0, x1
        MSR PMEVTYPER3_EL0, x1
        MSR PMEVTYPER4_EL0, x1
        MSR PMEVTYPER5_EL0, x1

// Set up PMU so that cycle counter increments in all modes
        LDR  x1, =0x0
        MSR PMCCFILTR_EL0, x1

// Initialize all counters to a large value
        LDR  x1, =0xF00FFFFF
        MSR PMEVCNTR0_EL0, x1
        MSR PMEVCNTR1_EL0, x1
        MSR PMEVCNTR2_EL0, x1
        MSR PMEVCNTR3_EL0, x1
        MSR PMEVCNTR4_EL0, x1
        MSR PMEVCNTR5_EL0, x1
        LDR  x1, =0xF00FFFFFF00FFFFF
        MSR PMCCNTR_EL0, x1

// Enable all counters
        LDR x1, =0x8000003F
        MSR PMCNTENSET_EL0, x1

// Enable PMU long counter, export and event counters
        MRS x1, PMCR_EL0
        LDR x2, =0x51
        ORR x1, x1, x2
        MSR PMCR_EL0, x1

// Program breakpoints and watchpoints (but make sure they don't hit)
// At EL3 in AArch64 state, monitor mode debug exceptions can not occur
bkpt:
        NOP
wpt:
        NOP
        ISB

// Set up break point value register
        LDR x1, =bkpt
        MSR DBGBVR0_EL1, x1
        MSR DBGBVR1_EL1, x1
        MSR DBGBVR2_EL1, x1
        MSR DBGBVR3_EL1, x1
        MSR DBGBVR4_EL1, x1
        MSR DBGBVR5_EL1, x1

// Set up break point control register with BAS=1111 and EN=1
        LDR x1, =0xF1
        MSR DBGBCR0_EL1, x1
        MSR DBGBCR1_EL1, x1
        MSR DBGBCR2_EL1, x1
        MSR DBGBCR3_EL1, x1
        MSR DBGBCR4_EL1, x1
        MSR DBGBCR5_EL1, x1

// Set up watch point value register
        LDR x1, =wpt
        MSR DBGWVR0_EL1, x1
        MSR DBGWVR1_EL1, x1
        MSR DBGWVR2_EL1, x1
        MSR DBGWVR3_EL1, x1

// Set up watch point control register with BAS=00000001, LSC=11 and EN=1
        LDR x1, =0x39
        MSR DBGWCR0_EL1, x1
        MSR DBGWCR1_EL1, x1
        MSR DBGWCR2_EL1, x1
        MSR DBGWCR3_EL1, x1

// Enable monitor mode debug
// Set MDSCR_EL1.MDE to 1
        MRS x1, MDSCR_EL1
        LDR x2, =0x8000
        ORR x1, x1, x2
        MSR MDSCR_EL1, x1

// Clear OS Lock
        LDR x1, =0x0
        MSR OSLAR_EL1, x1

// Sync up setup above
        DSB SY
        ISB

        mrs     x24, mpidr_el1      //read cpu id
        and     x22, x24, #0xff00


//;# init data in other ways of data RAM
// initialize data pool pointer
        cmp  x22, #0x400
        bne  core_5_0
        b test_end
core_5_0:
        cmp  x22, #0x500
        bne  core_6_0
        b test_end
core_6_0:
        cmp  x22, #0x600
        bne  core_7_0
        b test_end
core_7_0:
        cmp  x22, #0x700
        bne  core_lab_0
        ldr     x0, =g_hera_data_pool_30 // dp0 is 2MB page aligned
        ldr  x1, [x0]
core_lab_0:
        MOV      x2, #36*1024   // 576 64B blocks in pool_0 + pool_1
preload_loop:
        LDR      x3, [x1]
        ADD      x1, x1, #64
        SUB      x2, x2, #64
        CBNZ     x2, preload_loop

// initialize data pool 2 pointer
       cmp  x22, #0x400
       bne  core_5_1
       b test_end
core_5_1:
       cmp  x22, #0x500
       bne  core_6_1
       b test_end
core_6_1:
       cmp  x22, #0x600
       bne  core_7_1
       b test_end
core_7_1:
       cmp  x22, #0x700
       bne  core_lab_1
       ldr     x0, =g_hera_data_pool_32 // dp0 is 2MB page aligned
       ldr  x1, [x0]
core_lab_1:

        MOV      x2, #1472       // 1472 64B blocks in pool_2
preload_loop2:
        LDR      x3, [x1]
        ADD      x1, x1, #64
        SUB      x2, x2, #1
        CBNZ     x2, preload_loop2
// copy some data from init data pool to data pool 1
        cmp  x22, #0x400
        bne  core_5_2
        b test_end
core_5_2:
        cmp  x22, #0x500
        bne  core_6_2
        b test_end
core_6_2:
        cmp  x22, #0x600
        bne  core_7_2
        b test_end
core_7_2:
        cmp  x22, #0x700
        bne  core_lab_2
        ldr     x0, =g_hera_data_pool_30 // dp0 is 2MB page aligned
        ldr  x1, [x0]
core_lab_2:
        MOV      x2, #1024*128    // dp0 is 448 64B blocks
        ADD      x1, x1, x2       // add dp0 sized to get dp1
        MOV      x21, x1          // save off address of dp1
        MOV      x2, #256
        LDR      x3, =g_hera_data_pool_30
        ldr  x0, [x3]

copy_loop:
        LDP      q0,q1,[x0],#0x20
        STP      q0,q1,[x1],#0x20
        STP      q0,q1,[x1],#0x20
        SUB      x2, x2, #4
        CBNZ     x2, copy_loop

        // store to warm up 4 streams of data of aaaa into L2
        // then store 4 streams of 5555 in L1, setting up dirty data in all 4 ways
        // in loop, will store to warmed streams, causing fill and evict
        //;LDR      x1, =HERA_DATA_POOL_0
       cmp  x22, #0x400
       bne  core_5_3
       b test_end
core_5_3:
       cmp  x22, #0x500
       bne  core_6_3
       b test_end
core_6_3:
       cmp  x22, #0x600
       bne  core_7_3
       b test_end
core_7_3:
       cmp  x22, #0x700
       bne  core_lab_3
       ldr     x2,=g_hera_data_pool_30 // dp0 is 2MB page aligned
       ldr  x1, [x2]
core_lab_3:
       MOV      x2, x1             // where the next stream is
       MOV      x18, x1            // save for later
       MOV      x20, #0x4000       // offset hits same set in L1 cache
       LDR      x8, =0xaaaaaaaaaaaaaaaa
       LDR      x9, =0x5555555555555555
       MOV      x23, #4   //store a total of 8 streams, 4 with aaaa data, 4 with 5555
       MOV      x24, #4   //store to 4 sets for each stream
dirty_l2_loop:
       STP      x8,x8,[x1]      // loaded/dirtied in L1. First 4 streams then evicted to L2.
       STP      x8,x8,[x1],#32  // write entire cache line (only for initialization)
       STP      x8,x8,[x1],#64  // write 2nd bank cache line (only for initialization)
       STP      x8,x8,[x1],#96  // write 2nd bank cache line (only for initialization)
       SUB      x24, x24, #1
       ADD      x1, x1, #0x100
       CBNZ     x24, dirty_l2_loop

       ADD      x2, x2, x20        //calculate next stream
       MOV      x1, x2             //go to next stream
       MOV      x24, #4
       SUB      x23, x23, #1
       CBNZ     x23, dirty_l2_loop

       MOV      x23, #4
dirty_l2_loop2:
       STP      x9,x9,[x1]      // loaded/dirtied in L1. First 4 streams then evicted to L2.
       STP      x9,x9,[x1],#32  // write entire cache line (only for initialization)
       STP      x9,x9,[x1],#64  // write 2nd bank cache line (only for initialization)
       STP      x9,x9,[x1],#96  // write 2nd bank cache line (only for initialization)
       SUB      x24, x24, #1
       ADD      x1, x1, #0x100
       CBNZ     x24, dirty_l2_loop2

       ADD      x2, x2, x20        //calculate next stream
       MOV      x1, x2             //go to next stream
       MOV      x24, #4
       SUB      x23, x23, #1
       CBNZ     x23, dirty_l2_loop2
initialize:
       MOV      x0,  x21         // use previously created dp1 ptr
       cmp  x22, #0x400
       bne  core_5_4
       b test_end
core_5_4:
       cmp  x22, #0x500
       bne  core_6_4
       b test_end
core_6_4:
       cmp  x22, #0x600
       bne  core_7_4
       b test_end
core_7_4:
       cmp  x22, #0x700
       bne  core_lab_4
       ldr     x16,  =g_hera_data_pool_30 // 2MB aligned
       ldr     x3,  [x16]
       ldr     x16, =g_hera_data_pool_32 // 2MB aligned
       ldr  x10, [x16]
       ldr     x16, =g_hera_data_pool_33 // 1MB aligned
       ldr     x14, [x16]
       ldr     x16, =g_hera_data_pool_34
       ldr     x28, [x16]
core_lab_4:
        MOV      x16, x14 // save for later
        ADD      x15, x14, #0x800 // save for later
       // ; ADRP     x28, HERA_DATA_POOL_4
        MOV      x12, #MAXPWR_LOOP_COUNT // loop counter
        MOV      x2, x0 // save initial address
        LDR      x8, =0xaaaaaaaaaaaaaaaa
        LDR      x9, =0x5555555555555555
        LDR      x5,=0x0f0f0f0f0f0f0f0f
        LDR      x6,=0xf0f0f0f0f0f0f0f0
        LDR      x24, =0xaaaaaaaa55555555
        LDP      q24,q25,[x28]
        LDP      q26,q27,[x28], #0x10
        LDP      q28,q29,[x0]
        LDP      q30,q31,[x0]

        FMOV v0.2D, #0.125
        FMOV v1.2D, #0.625
        FMOV v2.2D, #0.875
        FMOV v3.2D, #0.375
        FMOV v4.2D, #0.5
        FMOV v5.2D, #0.25
        FMOV v6.2D, #0.75
        FMOV v7.2D, #0.4375
        FMOV v8.2D, #0.125
        FMOV v9.2D, #0.625
        FMOV v10.2D, #0.875
        FMOV v11.2D, #0.375
        FMOV v12.2D, #0.5
        FMOV v13.2D, #0.25
        FMOV v14.2D, #0.75
        FMOV v15.2D, #0.4375
        FMOV v16.2D, #0.125
        FMOV v17.2D, #0.625
        FMOV v18.2D, #0.875
        FMOV v19.2D, #0.375
        FMOV v20.2D, #0.5
        FMOV v21.2D, #0.25
        FMOV v22.2D, #0.75
        FMOV v23.2D, #0.4375
        MOV      x0,x2

// Pattern variation for BBLK macro

.macro PATTERN_BLK base3, o3, b0, b1, base1, o1, pattern
        //
        // 0,1: loop management
        //
        .if \pattern == 0
//        ADDS  x17, x9, w8, SXTB #2   // MX-any instr to turn on non-alu MX1
        ADD  x17, x9, #0x100
        .endif
        .if \pattern == 1
        SUB x12, x12, #0 // decrement loop counter (x12 is loop counter)
        .endif
        //
        // 2,3: pointer management for LDP
        //
        .if \pattern == 2
        ADD  x1, x0, #0x400
        .endif
        .if \pattern == 3
        // mov opt will use 2 rcq entries
        ADD x0, x2, #0
        .endif
        //
        // 4,5,6: pointer management for STP
        //
        .if \pattern == 4
        ADD  x4, x3, x20
        .endif
        .if \pattern == 5
        ADD  x3, x4, x20
        .endif
        .if \pattern == 6
        ADD  x3, x18, #0
        .endif
        //
        //  7,8,9: prefetch
        //
        .if \pattern == 7
//        PRFM PLDL1KEEP, [\base3, \o3]
        ADD x17, x9, #0x200
        .endif
        .if \pattern == 8
        ADD x11, x10, #0x400
        .endif
        .if \pattern == 9
        ADD x10, x11, #0x400
        .endif
.endm

// 2nd Pattern variation for BBLK macro
.macro PATTERN_BLK2 base4, o4, b2, b3, pattern
        //
        .if \pattern == 1
//         ADD x17, x8, #0x200
        //@        ADDS  x17, \s0, \s1, LSL #3   // IX uop, with flag write
        ADD x14, x16, #0
        .endif
        .if \pattern == 2
//        PRFM PLDL1KEEP, [\base4, \o4]  // additional L2 prefetch
//        LDR \b2, [\base4, \o4]
        ADD x17, x8, #0x200
        .endif
        .if \pattern == 3
//        ADD x17, x8, #0x200
        ADD x15, x14, #0x800
        .endif
        .if \pattern == 4
//        ADD x17, x8, #0x200
        ADD x14, x15, #0x800
        .endif
        //
        // 5,6: pointer management for LDP
        //
        .if \pattern == 5
        ADD  x1, x0, #0x800
        .endif
        .if \pattern == 6
        ADD  x0, x1, #0x800
        .endif
.endm

// 3rd Pattern variation for BBLK macro
.macro PATTERN_BLK3 base1, o1, s0, s1, pattern
        //
        .if \pattern == 1
//        ADDS  x17, x9, w8, SXTB #2   // MX-any instr to turn on non-alu MX1
        MUL     x23, x6, x9
        .endif
        .if \pattern == 2
  //      ADDS  x17, \s1, \s0, LSL #3   // IX uop
        MUL     x23, x5, x8
        .endif
        .if \pattern == 3
        UDIV x17, x9, x12            // additional UDIVs to keep divider busy
        .endif
.endm

// Pattern to choose branch +4 variation. too many BLs can impact fetch BW
.macro BR_PATTERN label pattern
        //
        .if \pattern == 1
        CBNZ     x13, bt\label
        .endif
        .if \pattern == 2
        BL     bt\label
        .endif
        .if \pattern == 3
        .ifdef DIDT_MODE
        cmp  x7,#5
        .endif
        .ifdef MAXPWR_MODE
        CBNZ     x13, bt\label
        .endif
        .endif
.endm

// Pattern to add additional DW multiplies
.macro MUL_PATTERN md, m0, m1, m2, pattern
        //
        .if \pattern == 1
        // never used
//        ADDS  \md, \m0, w8, SXTB #2   // mx-any instr to turn on non-alu MX1
        ADD   x17, x8, #0x200
        .endif
        .if \pattern == 2
        MADD \md, \m0, \m1, \m2
        .endif
        .if \pattern == 3
        MADD x7, x7, x7, x7 // LOOP Marker (x7 should be reserved for this purpose
        .endif
.endm

// STR pattern
.macro STR_PATTERN base2, o2, s0, s1, b2, base4, o4, pattern
        //
        .if \pattern == 1
        STNP     \s0, \s1, [\base2, \o2]
        .endif
        .if \pattern == 2
        STNP     \s0, \s1, [\base2, \o2]
        .endif
        .if \pattern == 3
        //PRFM PLDL1KEEP, [\base4, \o4]  // additional L2 prefetch
        //LDR \b2, [\base4, \o4]
        ADD   x17, x8, #0x200
        .endif
.endm


// Basic components to maxmize power consumption
.macro BBLK label0, label1, label2 a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, b0, b1, b2, b3, md, m0, m1, m2, base1, o1,  s0, s1, base2, o2, base3, o3, base4, o4, c0, pattern, pattern2, pattern3, pattern4, pattern5
        .balign 64
\label0:  //startup (only used first time)
        .rept 16
        NOP // 16 instructions to align the cache boundary
        .endr

        // make sure this is aligned to make sure mop$ doesn't miss
        // since need to read 8 instr per cycle, will need to read 2 6-instr lines anyway, so no need to fragment mop$ line
\label1:
        // base combination 4FMLA + LDP(for VRF) + STNP(from GRF)

        MUL_PATTERN \md, \m0, \m1, \m2, \pattern4
        STR_PATTERN  \base2, \o2, \s0, \s1, \b2, \base4, \o1, \pattern5 // 3/4 ST
        PATTERN_BLK2 \base4, \o3, \b2, \b3, \pattern2    // choose LD or ptr add
//        BR_PATTERN \label1, \pattern5  // alternate CBNZ or BL, for pred pwr and mop$ fragmentation
//bt\label1:
//        ADD   x17, x8, #0x200
        LDR      \b1, [\base1, \o1]
        FMLA     \a0, \a10, v24.2D
        FMLA     \a1, \a1,  v25.2D
        FMLA     \a4, \a10, v26.2D
        FMLA     \a5, \a5,  v27.2D

        PATTERN_BLK \base3, \o3, \b0, \b1, \base1, \o1, \pattern
        PATTERN_BLK3 \base1, \o1, \s1, \s0, \pattern3    // choose MUL or 3 UDIV
//        ADD   x17, x8, #0x200
        LDR      \b0, [\base1, \o4]
        FMLA     \a3, \a3,  v27.2D
        FMLA     \a2, \a11, v26.2D
        FMLA     \a7, \a7,  v25.2D
        FMLA     \a6, \a11, v24.2D
        // last instruction is always CBNZ
        CBNZ \c0, \label2

        RET
.endm

hera_test_start:
//begin gpio169, bit 1 output high
//        LDR x27, =GPIO_DATA
        LDR w19, [x27]
        ORR w19, w19, #0x2
        //end
        SEVL
        WFE
        WFE
        //begin gpio169, let less code here
        STR w19, [x27]
        //end
        //
        // Max power CPU:
        // Test will maximize the power consumption for CPU core (including private L2).
        // Test will dispatch following instructions (up to 4 instruction can be delivered per cycle)
        //   - 2.00 VX double precision FMAs (FMAL vx.2D, vy.2D, vz.2D)
        //   - 0.50 CBNZ (conditional branch) to increase BX/I$/branch predictor power
        //   - 0.50 LDP quad word load which fetch 32B data from L1D cache (16B read / cycle)
        //   - 0.50 STNP double word store which store steaming data (8B write / cycle for L2, L1D bypass) Because of VRF read port constraint, STP uses GRF to fetch storing data.
        //   - 0.25 PRFM which fetch 64B data from L2 to L1D (16B write / cycle for L1D, 16B read / cycle for L2)
        //   - 0.25 other instruction (loop control, pointer management, activate the other function units)
        //
       ldr x28, =g_hera_outer_loop_count
       ldr x26, [x28]
hera_outer_loop:
        ldr x28, =g_hera_inter_loop_count
        ldr x25, [x28]

        BBLK init00, extrabranch00, extrabranch01,  v0.2D,  v1.2D,  v2.2D,  v3.2D,  v12.2D, v13.2D, v14.2D, v15.2D, v24.2D, v27.2D, v28.2D, v31.2D,  q28, q30,  q23, q26, x23, x5, x9, x8, x0, #0x080,  x8, x9, x3, #0x000, x10, #0x0c0, x14, #0x0e0, x12, 0,  5, 2, 3, 2 // start loop
        BBLK init01, extrabranch01, extrabranch02,  v4.2D,  v5.2D,  v6.2D,  v7.2D,  v16.2D, v17.2D, v18.2D, v19.2D, v25.2D, v26.2D, v29.2D, v30.2D,  q29, q31,  q15, q27, x23, x9, x6, x9, x0, #0x190,  x9, x8, x3, #0x040, x10, #0x1d0, x14, #0x1f0, x12, 7,  2, 1, 2, 1 // prefetch
        BBLK init02, extrabranch02, extrabranch03,  v8.2D,  v9.2D, v10.2D, v11.2D,  v20.2D, v21.2D, v22.2D, v23.2D, v24.2D, v27.2D, v28.2D, v31.2D,  q28, q30,  q19, q26, x23, x8, x9, x5, x0, #0x2a0,  x8, x9, x3, #0x100, x10, #0x2c0, x14, #0x2e0, x12, 4,  2, 2, 2, 2 // NOP
        BBLK init03, extrabranch03, extrabranch04,  v0.2D,  v1.2D,  v2.2D,  v3.2D,  v12.2D, v13.2D, v14.2D, v15.2D, v25.2D, v26.2D, v29.2D, v30.2D,  q29, q31,  q23, q27, x23, x9, x8, x9, x0, #0x3b0,  x9, x8, x3, #0x140, x10, #0x3d0, x14, #0x3f0, x12, 7,  3, 1, 2, 1 // prefetch, x1<-x0+400
        BBLK init04, extrabranch04, extrabranch05,  v4.2D,  v5.2D,  v6.2D,  v7.2D,  v16.2D, v17.2D, v18.2D, v19.2D, v24.2D, v27.2D, v28.2D, v31.2D,  q28, q30,  q15, q26, x23, x6, x8, x9, x0, #0x480,  x8, x9, x4, #0x000, x10, #0x4c0, x14, #0x4e0, x12, 0,  2, 2, 2, 2 // NOP
        BBLK init05, extrabranch05, extrabranch06,  v8.2D,  v9.2D, v10.2D, v11.2D,  v20.2D, v21.2D, v22.2D, v23.2D, v25.2D, v26.2D, v29.2D, v30.2D,  q29, q31,  q19, q27, x23, x8, x5, x8, x0, #0x590,  x9, x8, x4, #0x040, x10, #0x5d0, x14, #0x5f0, x12, 7,  2, 1, 2, 1 // prefetch
        BBLK init06, extrabranch06, extrabranch07,  v0.2D,  v1.2D,  v2.2D,  v3.2D,  v12.2D, v13.2D, v14.2D, v15.2D, v24.2D, v27.2D, v28.2D, v31.2D,  q28, q30,  q23, q26, x23, x9, x8, x6, x0, #0x6a0,  x8, x9, x4, #0x100, x10, #0x6c0, x14, #0x6e0, x12, 5,  2, 2, 2, 2 // NOP
        BBLK init07, extrabranch07, extrabranch08,  v4.2D,  v5.2D,  v6.2D,  v7.2D,  v16.2D, v17.2D, v18.2D, v19.2D, v25.2D, v26.2D, v29.2D, v30.2D,  q29, q31,  q15, q27, x23, x8, x9, x8, x0, #0x7b0,  x9, x8, x4, #0x140, x10, #0x7d0, x14, #0x7f0, x12, 7,  2, 1, 2, 3 // prefetch
        BBLK init08, extrabranch08, extrabranch09,  v8.2D,  v9.2D, v10.2D, v11.2D,  v20.2D, v21.2D, v22.2D, v23.2D, v24.2D, v27.2D, v28.2D, v31.2D,  q28, q30,  q19, q26, x23, x5, x9, x8, x1, #0x080,  x8, x9, x3, #0x000, x10, #0x0c0, x15, #0x0e0, x12, 4,  6, 2, 2, 2 // NOP
        BBLK init09, extrabranch09, extrabranch10,  v0.2D,  v1.2D,  v2.2D,  v3.2D,  v12.2D, v13.2D, v14.2D, v15.2D, v25.2D, v26.2D, v29.2D, v30.2D,  q29, q31,  q23, q27, x23, x9, x6, x9, x1, #0x190,  x9, x8, x3, #0x040, x10, #0x1d0, x15, #0x1f0, x12, 7,  2, 1, 2, 1 // prefetch
        BBLK init10, extrabranch10, extrabranch11,  v4.2D,  v5.2D,  v6.2D,  v7.2D,  v16.2D, v17.2D, v18.2D, v19.2D, v24.2D, v27.2D, v28.2D, v31.2D,  q28, q30,  q15, q26, x23, x8, x9, x5, x1, #0x2a0,  x8, x9, x3, #0x100, x10, #0x2c0, x15, #0x2e0, x12, 0,  2, 2, 2, 2 // copy from x1 to x0
        BBLK init11, extrabranch11, extrabranch12,  v8.2D,  v9.2D, v10.2D, v11.2D,  v20.2D, v21.2D, v22.2D, v23.2D, v25.2D, v26.2D, v29.2D, v30.2D,  q29, q31,  q19, q27, x23, x9, x8, x9, x1, #0x3b0,  x9, x8, x3, #0x140, x10, #0x3d0, x15, #0x3f0, x12, 7,  4, 1, 2, 1 // prefetch
        BBLK init12, extrabranch12, extrabranch13,  v0.2D,  v1.2D,  v2.2D,  v3.2D,  v12.2D, v13.2D, v14.2D, v15.2D, v24.2D, v27.2D, v28.2D, v31.2D,  q28, q30,  q23, q26, x23, x6, x8, x9, x1, #0x480,  x8, x9, x4, #0x000, x10, #0x4c0, x15, #0x4e0, x12, 5,  2, 2, 2, 2 // copy from x4 to x3
        BBLK init13, extrabranch13, extrabranch14,  v4.2D,  v5.2D,  v6.2D,  v7.2D,  v16.2D, v17.2D, v18.2D, v19.2D, v25.2D, v26.2D, v29.2D, v30.2D,  q29, q31,  q15, q27, x23, x8, x5, x8, x1, #0x590,  x9, x8, x4, #0x040, x10, #0x5d0, x15, #0x5f0, x12, 7,  2, 1, 2, 1 // prefetch, x14 to x15
        BBLK init14, extrabranch14, extrabranch15,  v8.2D,  v9.2D, v10.2D, v11.2D,  v20.2D, v21.2D, v22.2D, v23.2D, v24.2D, v27.2D, v28.2D, v31.2D,  q28, q30,  q19, q26, x23, x9, x8, x6, x1, #0x6a0,  x8, x9, x4, #0x100, x10, #0x6c0, x15, #0x6e0, x12, 8,  2, 2, 2, 2 // copy from x10 to x11
        BBLK init15, extrabranch15, extrabranch48,  v0.2D,  v1.2D,  v2.2D,  v3.2D,  v12.2D, v13.2D, v14.2D, v15.2D, v25.2D, v26.2D, v29.2D, v30.2D,  q29, q31,  q23, q27, x23, x8, x9, x8, x1, #0x7b0,  x9, x8, x4, #0x140, x10, #0x7d0, x15, #0x7f0, x12, 7,  2, 1, 2, 3 // prefetch

        BBLK init48, extrabranch48, extrabranch49,  v4.2D,  v5.2D,  v6.2D,  v7.2D,  v16.2D, v17.2D, v18.2D, v19.2D, v24.2D, v27.2D, v28.2D, v31.2D,  q28, q30,  q15, q26, x23, x5, x9, x8, x0, #0x080,  x8, x9, x3, #0x000, x11, #0x0c0, x14, #0x0e0, x12, 0,  5, 2, 2, 2 // NOP
        BBLK init49, extrabranch49, extrabranch50,  v8.2D,  v9.2D, v10.2D, v11.2D,  v20.2D, v21.2D, v22.2D, v23.2D, v25.2D, v26.2D, v29.2D, v30.2D,  q29, q31,  q19, q27, x23, x9, x6, x9, x0, #0x190,  x9, x8, x3, #0x040, x11, #0x1d0, x14, #0x1f0, x12, 7,  2, 1, 2, 1 // prefetch
        BBLK init50, extrabranch50, extrabranch51,  v0.2D,  v1.2D,  v2.2D,  v3.2D,  v12.2D, v13.2D, v14.2D, v15.2D, v24.2D, v27.2D, v28.2D, v31.2D,  q28, q30,  q23, q26, x23, x8, x9, x5, x0, #0x2a0,  x8, x9, x3, #0x100, x11, #0x2c0, x14, #0x2e0, x12, 4,  2, 2, 2, 2 // NOP
        BBLK init51, extrabranch51, extrabranch52,  v4.2D,  v5.2D,  v6.2D,  v7.2D,  v16.2D, v17.2D, v18.2D, v19.2D, v25.2D, v26.2D, v29.2D, v30.2D,  q29, q31,  q15, q27, x23, x9, x8, x9, x0, #0x3b0,  x9, x8, x3, #0x140, x11, #0x3d0, x14, #0x3f0, x12, 7,  3, 1, 2, 1 // prefetch, x1<-x0+400
        BBLK init52, extrabranch52, extrabranch53,  v8.2D,  v9.2D, v10.2D, v11.2D,  v20.2D, v21.2D, v22.2D, v23.2D, v24.2D, v27.2D, v28.2D, v31.2D,  q28, q30,  q19, q26, x23, x6, x8, x9, x0, #0x480,  x8, x9, x4, #0x000, x11, #0x4c0, x14, #0x4e0, x12, 9,  2, 2, 2, 2 // copy from x11 to x10
        BBLK init53, extrabranch53, extrabranch54,  v0.2D,  v1.2D,  v2.2D,  v3.2D,  v12.2D, v13.2D, v14.2D, v15.2D, v25.2D, v26.2D, v29.2D, v30.2D,  q29, q31,  q23, q27, x23, x8, x5, x8, x0, #0x590,  x9, x8, x4, #0x040, x11, #0x5d0, x14, #0x5f0, x12, 7,  2, 1, 2, 1 // prefetch
        BBLK init54, extrabranch54, extrabranch55,  v4.2D,  v5.2D,  v6.2D,  v7.2D,  v16.2D, v17.2D, v18.2D, v19.2D, v24.2D, v27.2D, v28.2D, v31.2D,  q28, q30,  q15, q26, x23, x9, x8, x6, x0, #0x6a0,  x8, x9, x4, #0x100, x11, #0x6c0, x14, #0x6e0, x12, 5,  2, 2, 2, 2 // NOP
        BBLK init55, extrabranch55, extrabranch56,  v8.2D,  v9.2D, v10.2D, v11.2D,  v20.2D, v21.2D, v22.2D, v23.2D, v25.2D, v26.2D, v29.2D, v30.2D,  q29, q31,  q19, q27, x23, x8, x9, x8, x0, #0x7b0,  x9, x8, x4, #0x140, x11, #0x7d0, x14, #0x7f0, x12, 7,  2, 1, 2, 3 // prefetch
        BBLK init56, extrabranch56, extrabranch57,  v0.2D,  v1.2D,  v2.2D,  v3.2D,  v12.2D, v13.2D, v14.2D, v15.2D, v24.2D, v27.2D, v28.2D, v31.2D,  q28, q30,  q23, q26, x23, x5, x9, x8, x1, #0x080,  x8, x9, x3, #0x000, x11, #0x0c0, x15, #0x0e0, x12, 4,  2, 2, 2, 2 // decrement loop counter
        BBLK init57, extrabranch57, extrabranch58,  v4.2D,  v5.2D,  v6.2D,  v7.2D,  v16.2D, v17.2D, v18.2D, v19.2D, v25.2D, v26.2D, v29.2D, v30.2D,  q29, q31,  q15, q27, x23, x9, x6, x9, x1, #0x190,  x9, x8, x3, #0x040, x11, #0x1d0, x15, #0x1f0, x12, 7,  2, 1, 2, 1 // prefetch
        BBLK init58, extrabranch58, extrabranch59,  v8.2D,  v9.2D, v10.2D, v11.2D,  v20.2D, v21.2D, v22.2D, v23.2D, v24.2D, v27.2D, v28.2D, v31.2D,  q28, q30,  q19, q26, x23, x8, x9, x5, x1, #0x2a0,  x8, x9, x3, #0x100, x11, #0x2c0, x15, #0x2e0, x12, 3,  2, 2, 2, 2 // recover x0
        BBLK init59, extrabranch59, extrabranch60,  v0.2D,  v1.2D,  v2.2D,  v3.2D,  v12.2D, v13.2D, v14.2D, v15.2D, v25.2D, v26.2D, v29.2D, v30.2D,  q29, q31,  q23, q27, x23, x9, x8, x9, x1, #0x3b0,  x9, x8, x3, #0x140, x11, #0x3d0, x15, #0x3f0, x12, 7,  1, 1, 2, 1 // prefetch
        BBLK init60, extrabranch60, extrabranch61,  v4.2D,  v5.2D,  v6.2D,  v7.2D,  v16.2D, v17.2D, v18.2D, v19.2D, v24.2D, v27.2D, v28.2D, v31.2D,  q28, q30,  q15, q26, x23, x6, x8, x9, x1, #0x480,  x8, x9, x4, #0x000, x11, #0x4c0, x15, #0x4e0, x12, 6,  2, 2, 2, 2 // copy from x3 to x4
        BBLK init61, extrabranch61, extrabranch62,  v8.2D,  v9.2D, v10.2D, v11.2D,  v20.2D, v21.2D, v22.2D, v23.2D, v25.2D, v26.2D, v29.2D, v30.2D,  q29, q31,  q19, q27, x23, x8, x5, x8, x1, #0x590,  x9, x8, x4, #0x040, x11, #0x5d0, x15, #0x5f0, x12, 7,  2, 1, 2, 1 // prefetch, x15 to x14
        BBLK init62, extrabranch62, extrabranch63,  v0.2D,  v1.2D,  v2.2D,  v3.2D,  v12.2D, v13.2D, v14.2D, v15.2D, v24.2D, v27.2D, v28.2D, v31.2D,  q28, q30,  q23, q26, x23, x9, x8, x6, x1, #0x6a0,  x8, x9, x4, #0x100, x11, #0x6c0, x15, #0x6e0, x12, 1,  2, 2, 2, 2 // decrement loop counter
        BBLK init63, extrabranch63, check_inner_loop,  v4.2D,  v5.2D,  v6.2D,  v7.2D,  v16.2D, v17.2D, v18.2D, v19.2D, v25.2D, v26.2D, v29.2D, v30.2D,  q29, q31,  q15, q27, x23, x8, x9, x8, x1, #0x7b0,  x9, x8, x4, #0x140, x11, #0x7d0, x15, #0x7f0, x12, 7,  2, 1, 2, 3 // prefetch
check_inner_loop:
        SUBS   w25, w25, #1
        BNE    extrabranch00

        MOV      x2, x0 // save initial address
        SUBS   w26, w26, #1

        BNE    hera_outer_loop
// begin output low gpio_data
        mov x29, x27
        LDR w19, [x29]
        LDR w27, =0xfffd
        AND w19, w19, w27
        STR w19, [x29]
test_end:
// ;Restore SIMD registers
        ldp     x3,  x4,  [sp], #16
        ldp     x1,  x2,  [sp], #16
        mov     v30.d[0],  x1
        mov     v30.d[1],  x2
        mov     v31.d[0],  x3
        mov     v31.d[1],  x4

        ldp     x29, x30, [sp], #16
        ldp     x27, x28, [sp], #16
        ldp     x25, x26, [sp], #16
        ldp     x23, x24, [sp], #16
        ldp     x21, x22, [sp], #16
        ldp     x19, x20, [sp], #16
        ldp     x17, x18, [sp], #16
        ldp     x15, x16, [sp], #16
        ldp     x13, x14, [sp], #16
        ldp     x11, x12, [sp], #16
        ldp     x9,  x10, [sp], #16
        ldp     x7,  x8,  [sp], #16
        ldp     x5,  x6,  [sp], #16
        ldp     x3,  x4,  [sp], #16
        ldp     x1,  x2,  [sp], #16

        mov     v15.d[0],  x1
        mov     v15.d[1],  x2
        mov     v16.d[0],  x3
        mov     v16.d[1],  x4
        mov     v17.d[0],  x5
        mov     v17.d[1],  x6
        mov     v18.d[0],  x7
        mov     v18.d[1],  x8
        mov     v19.d[0],  x9
        mov     v19.d[1],  x10
        mov     v20.d[0],  x11
        mov     v20.d[1],  x12
        mov     v21.d[0],  x13
        mov     v21.d[1],  x14
        mov     v22.d[0],  x15
        mov     v22.d[1],  x16
        mov     v23.d[0],  x17
        mov     v23.d[1],  x18
        mov     v24.d[0],  x19
        mov     v24.d[1],  x20
        mov     v25.d[0],  x21
        mov     v25.d[1],  x22
        mov     v26.d[0],  x23
        mov     v26.d[1],  x24
        mov     v27.d[0],  x25
        mov     v27.d[1],  x26
        mov     v28.d[0],  x27
        mov     v28.d[1],  x28
        mov     v29.d[0],  x29
        mov     v29.d[1],  x30
        ldp     x29, x30, [sp], #16
        ldp     x27, x28, [sp], #16
        ldp     x25, x26, [sp], #16
        ldp     x23, x24, [sp], #16
        ldp     x21, x22, [sp], #16
        ldp     x19, x20, [sp], #16
        ldp     x17, x18, [sp], #16
        ldp     x15, x16, [sp], #16
        ldp     x13, x14, [sp], #16
        ldp     x11, x12, [sp], #16
        ldp     x9,  x10, [sp], #16
        ldp     x7,  x8,  [sp], #16
        ldp     x5,  x6,  [sp], #16
        ldp     x3,  x4,  [sp], #16
        ldp     x1,  x2,  [sp], #16
        mov     v0.d[0],  x1
        mov     v0.d[1],  x2
        mov     v1.d[0],  x3
        mov     v1.d[1],  x4
        mov     v2.d[0],  x5
        mov     v2.d[1],  x6
        mov     v3.d[0],  x7
        mov     v3.d[1],  x8
        mov     v4.d[0],  x9
        mov     v4.d[1],  x10
        mov     v5.d[0],  x11
        mov     v5.d[1],  x12
        mov     v6.d[0],  x13
        mov     v6.d[1],  x14
        mov     v7.d[0],  x15
        mov     v7.d[1],  x16
        mov     v8.d[0],  x17
        mov     v8.d[1],  x18
        mov     v9.d[0],  x19
        mov     v9.d[1],  x20
        mov     v10.d[0],  x21
        mov     v10.d[1],  x22
        mov     v11.d[0],  x23
        mov     v11.d[1],  x24
        mov     v12.d[0],  x25
        mov     v12.d[1],  x26
        mov     v13.d[0],  x27
        mov     v13.d[1],  x28
        mov     v14.d[0],  x29
        mov     v14.d[1],  x30
        ldp     x29, x30, [sp], #16
        ldp     x27, x28, [sp], #16
        ldp     x25, x26, [sp], #16
        ldp     x23, x24, [sp], #16
        ldp     x21, x22, [sp], #16
        ldp     x19, x20, [sp], #16
        ldp     x17, x18, [sp], #16
        ldp     x15, x16, [sp], #16
        ldp     x13, x14, [sp], #16
        ldp     x11, x12, [sp], #16
        ldp     x9,  x10, [sp], #16
        ldp     x7,  x8,  [sp], #16
        ldp     x5,  x6,  [sp], #16
        ldp     x3,  x4,  [sp], #16
        ldp     x1,  x2,  [sp], #16
        ret
.end
