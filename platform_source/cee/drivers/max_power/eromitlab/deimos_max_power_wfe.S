/*
 * deimos_max_power_wfe.S
 *
 * cs middle&big cluster wfe max power
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
.extern g_deimos_data_pool_03
.extern g_deimos_data_pool_00
.extern g_deimos_data_pool_02
.extern g_deimos_data_pool_13
.extern g_deimos_data_pool_10
.extern g_deimos_data_pool_12
.extern g_deimos_data_pool_23
.extern g_deimos_data_pool_20
.extern g_deimos_data_pool_22
.extern g_deimos_data_pool_33
.extern g_deimos_data_pool_30
.extern g_deimos_data_pool_32
.extern g_deimos_outer_loop_count
.global deimos_max_power_pi_wfe
.set MAXPWR_LOOP_COUNT, 32    // post warmup. 64cyc/loop nominal

deimos_max_power_pi_wfe:
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
        mov x24, x0

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

        mrs     x0, mpidr_el1      // read cpu id
        and     x22, x0, #0xff00

        // initialize data pool pointer
        cmp  x22, #0x400
        bne  core_5_0
        ldr x0,  = g_deimos_data_pool_00 // dp0 is 2MB page aligned
        ldr x1, [x0]

core_5_0:
        cmp  x22, #0x500
        bne  core_6_0
        ldr     x0, = g_deimos_data_pool_10 // dp0 is 2MB page aligned
        ldr x1, [x0]
core_6_0:
        cmp  x22, #0x600
        bne  core_7_0
        ldr     x0, =  g_deimos_data_pool_20 // dp0 is 2MB page aligned
        ldr x1, [x0]
core_7_0:
        cmp  x22, #0x700
        bne  core_lab_0
        ldr     x0, =  g_deimos_data_pool_30 // dp0 is 2MB page aligned
        ldr x1, [x0]

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
        ldr     x0, =g_deimos_data_pool_02 // dp0 is 2MB page aligned
        ldr x1, [x0]
core_5_1:
        cmp  x22, #0x500
        bne  core_6_1
        ldr    x0, = g_deimos_data_pool_12 // dp0 is 2MB page aligned
        ldr x1, [x0]
core_6_1:
        cmp  x22, #0x600
        bne  core_7_1
        ldr     x0, = g_deimos_data_pool_22 // dp0 is 2MB page aligned
        ldr x1, [x0]
core_7_1:
        cmp  x22, #0x700
        bne  core_lab_1
        ldr     x0, = g_deimos_data_pool_32 // dp0 is 2MB page aligned
        ldr x1, [x0]

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
        ldr     x0, = g_deimos_data_pool_00 // dp0 is 2MB page aligned
        ldr x1, [x0]
core_5_2:
        cmp  x22, #0x500
        bne  core_6_2
        ldr     x0, = g_deimos_data_pool_10 // dp0 is 2MB page aligned
        ldr x1, [x0]
core_6_2:
        cmp  x22, #0x600
        bne  core_7_2
        ldr     x0, = g_deimos_data_pool_20 // dp0 is 2MB page aligned
        ldr x1, [x0]
core_7_2:
        cmp  x22, #0x700
        bne  core_lab_2
        ldr     x0, = g_deimos_data_pool_30 // dp0 is 2MB page aligned
        ldr x1, [x0]

core_lab_2:
        MOV      x2, #224*128    // dp0 is 448 64B blocks
        ADD      x1, x1, x2       // add dp0 sized to get dp1
        MOV      x21, x1          // save off address of dp1
        LDR      x2, = g_deimos_data_pool_00
        ldr x0, [x2]
        MOV      x2, #256

copy_loop:
        LDP      q0,q1,[x0],#0x20
        STP      q0,q1,[x1],#0x20
        STP      q0,q1,[x1],#0x20
        SUB      x2, x2, #4
        CBNZ     x2, copy_loop
initialize:
        MOV      x0,  x21         // use previously created dp1 ptr
        cmp  x22, #0x400
        bne  core_5_4
        ldr     x2,= g_deimos_data_pool_00 // 1MB aligned
        ldr x3, [x2]
        ldr     x2,= g_deimos_data_pool_02 // 1MB aligned
        ldr x10, [x2]
        ldr     x2,= g_deimos_data_pool_03 // 1MB aligned
        ldr x14, [x2]
core_5_4:
        cmp  x22, #0x500
        bne  core_6_4
        ldr     x2,= g_deimos_data_pool_10 // 1MB aligned
        ldr x3, [x2]
        ldr     x2,= g_deimos_data_pool_12 // 1MB aligned
        ldr x10, [x2]
        ldr     x2,= g_deimos_data_pool_13 // 1MB aligned
        ldr x14, [x2]
core_6_4:
        cmp  x22, #0x600
        bne  core_7_4
        ldr     x2,= g_deimos_data_pool_20 // 1MB aligned
        ldr x3, [x2]
        ldr     x2,= g_deimos_data_pool_22 // 1MB aligned
        ldr x10, [x2]
        ldr     x2,= g_deimos_data_pool_23 // 1MB aligned
        ldr x14, [x2]
core_7_4:
        cmp  x22, #0x700
        bne  core_lab_4
        ldr     x2, = g_deimos_data_pool_30 // 2MB aligned
        ldr x3, [x2]
        ldr     x2,= g_deimos_data_pool_32 // 2MB aligned
        ldr x10, [x2]
        ldr     x2,= g_deimos_data_pool_33 // 1MB aligned
        ldr x14, [x2]

core_lab_4:
        MOV      x16, x3
        MOV      x18, x10
        MOV      x20, x14
        MOV      x12, #MAXPWR_LOOP_COUNT // loop counter (always start with 32)
        MOV      x2, x0 // save initial address
        LDR      x8, =0xaaaaaaaaaaaaaaaa
        LDR      x9, =0x5555555555555555
        LDR      x5,=0x0f0f0f0f0f0f0f0f
        LDR      x6,=0xf0f0f0f0f0f0f0f0
        LDR      x17, =0x5a5a5a5a5a5a5a5a
        LDP      q0,q1,[x0],#0x20
        LDP      q2,q3,[x0],#0x20
        LDP      q4,q5,[x0],#0x20
        LDP      q6,q7,[x0],#0x20
        LDP      q8,q9,[x0],#0x20
        LDP      q10,q11,[x0],#0x20
        LDP      q12,q13,[x0],#0x20
        LDP      q14,q15,[x0],#0x20
        LDP      q16,q17,[x0],#0x20
        LDP      q18,q19,[x0],#0x20
        LDP      q20,q21,[x0],#0x20
        LDP      q22,q23,[x0],#0x20
        LDP      q24,q25,[x0],#0x20
        LDP      q26,q27,[x0],#0x20
        LDP      q28,q29,[x0],#0x20
        LDP      q30,q31,[x0],#0x20

        LDR x25,=0x100
        MOV      x0,x2

// Pattern variation for BBLK macro
.macro PATTERN_BLK base3, o3, b0, b1, base1, o1, pattern
        //
        // 0,1: loop management
        //
        .if \pattern == 0
        ADDS  x7, x9, w8, SXTB #2   // MX-any instr to turn on non-alu MX1
        .endif
        .if \pattern == 1
        SUB x12, x12, #0 // decrement loop counter (x12 is loop counter)
        .endif
        //
        // 2,3,4: pointer management for LDP
        //
        .if \pattern == 2
        ADD  x1, x0, #0x400
        .endif
        .if \pattern == 3
        ADD  x0, x1, #0x400
        .endif
        .if \pattern == 4
        MOV x0, x2
        .endif
        //
        // 5,6: pointer management for STP
        //
        .if \pattern == 5
        ADD  x4, x3, #0x100
        .endif
        .if \pattern == 6
        ADD  x3, x4, #0x100
        .endif
        //
        //  7,8,9: prefetch
        //
        .if \pattern == 7
        PRFM PLDL1KEEP, [\base3, \o3]
        .endif
        .if \pattern == 8
        ADD x11, x10, #0x200
        .endif
        .if \pattern == 9
        ADD x10, x11, #0x200
        .endif
        //
        // other NOP equivalent operation
        // NOP should use x14,x15,...
        // DIV, MUL, MADD, ...
        //
        .if \pattern == 10
        UDIV x7, x8, x12 // NOP a
        .endif
        .if \pattern == 19
        LDP      \b1, \b0, [\base1, \o1]
        .endif
        .if \pattern == 20
        UDIV x7, x9, x12 // NOP a
        .endif
.endm

// 2nd Pattern variation for BBLK macro
.macro PATTERN_BLK2 base1, o1, s0, s1, pattern
        //
        .if \pattern == 1
//        ADDS  x7, \s0, \s1, LSL #3   // IX uop, with flag write
        ADD x19, x8, #0x200
        .endif
        .if \pattern == 7
//        ADDS  x7, \s0, \s1, LSL #3   // IX uop, with flag write
        ADD x19, x9, #0x200
        .endif
        .if \pattern == 2
//        PRFM PLDL2KEEP, [\base1, \o1]  // additional L2 prefetch
        ADD x19, x8, #0x200
        .endif
        .if \pattern == 3
        ADD x15, x14, #0x100  // was x200
        .endif
        .if \pattern == 4
        ADD x14, x15, #0x100  // was x200
        .endif
        //
        // 5,6: pointer management for LDP
        //
        .if \pattern == 5
        ADD  x1, x0, #0x400
        .endif
        .if \pattern == 6
        ADD  x0, x1, #0x400
        .endif
.endm

// 3rd Pattern variation for BBLK macro
.macro PATTERN_BLK3 base1, o1, s0, s1, pattern
        //
        .if \pattern == 1
        ADDS  x7, x8, w9, SXTB #2   // MX-any instr to turn on non-alu MX1 change to 1 src reg
//        ADDS  x7, x8, WZR, SXTB #2   // MX-any instr to turn on non-alu MX1 change to 1 src reg
        .endif
        .if \pattern == 2
        ADDS  x17, x17, x17, LSL #3   // IX uop
        .endif
        .if \pattern == 3
        UDIV x7, x9, x12            // additional UDIVs to keep divider busy
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
        ADDS  \md, \m0, w8, SXTB #2   // mx-any instr to turn on non-alu MX1
        .endif
        .if \pattern == 2
        MADD \md, \m0, \m1, \m2
        .endif
        .if \pattern == 3
        MADD x7, x7, x7, x7 // LOOP Marker (x7 should be reserved for this purpose)
        .endif
.endm

// Basic components to maxmize power consumption
.macro BBLK label1, label2 a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, b0, b1,  md, m0, m1, m2, base1, o1,  s0, s1, base2, o2, base3, o3, base4, o4, c0, pattern, pattern2, pattern3, pattern4, pattern5
        .balign 32
\label1:
        // base combination 4FMLA + LDP(for VRF) + STNP(from GRF)
        PATTERN_BLK2 \base4, \o4, \s1, \s0, \pattern2    // choose ADD or PLD
        PATTERN_BLK3 \base1, \o1, \s1, \s0, \pattern3    // choose ADD or ADDS, mostly ADDS for MX-any
        FMLA     \a0, \a4, \a6
        FMLA     \a1, \a7, \a10
        BR_PATTERN \label1, \pattern5  // alternate CBNZ or BL, for pred pwr and mop$ fragmentation
bt\label1:
        MUL_PATTERN \md, \m0, \m1, \m2, \pattern4
        FMLA     \a2, \a8, \a9
        FMLA     \a3, \a11, \a5
        LDP      \b0, \b1, [\base1, \o1]
        STNP     \s0, \s1, [\base2, \o2]
        // ---- different control based on pattern value ----
        // pattern = 0-1: loop management
        // pattern = 2-4: pointer management for LDP
        // pattern = 5-6: pointer management for STP
        // pattern = 7-9: prefetch
        // pattern = >10: NOP (do something to consume power)
        PATTERN_BLK \base3, \o3, \b0, \b1, \base1, \o1, \pattern
        // last instruction is always CBNZ
        CBNZ \c0, \label2

        MOV  x3,  x16 // store pointer reset
        MOV  x10, x18 // prefetch pointer reset
        MOV  x10, x18 // pad to keep mop alignment on return to loop
        RET
.endm

///////////////////////////////////////////

deimos_test_start:
        //begin gpio169, bit 1 output high
//        LDR x24, =GPIO_DATA
        LDR w19, [x24]
        ORR w19, w19, #0x2
        //end
        SEVL
        WFE
        WFE
        //begin gpio169, let less code here
        STR w19, [x24]
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

        ldr x28, =g_deimos_outer_loop_count
        ldr x26, [x28]

deimos_outer_loop:
        LDR x27,=0x20

        BBLK extrabranch00, extrabranch01,  v0.2D,  v1.2D,  v2.2D,  v3.2D,  v16.2D, v17.2D, v18.2D, v19.2D, v20.2D, v21.2D, v22.2D, v23.2D,  q12, q13,  x23, x5, x9, x8, x0, #0x030,  x19, x19, x3, #0x00, x10, #0x000, x14, #0x000, x18, 0,  1, 2, 3, 2 // start loop
        BBLK extrabranch01, extrabranch02,  v4.2D,  v5.2D,  v6.2D,  v7.2D,  v18.2D, v19.2D, v20.2D, v21.2D, v22.2D, v23.2D, v24.2D, v25.2D,  q14, q15,  x23, x9, x6, x9, x0, #0x0B0,  x19, x19, x3, #0x10, x10, #0x000, x14, #0x000, x18, 7,  1, 2, 2, 1 // prefetch
        BBLK extrabranch02, extrabranch03,  v8.2D,  v9.2D, v10.2D, v11.2D,  v20.2D, v21.2D, v22.2D, v23.2D, v24.2D, v25.2D, v26.2D, v27.2D,  q12, q13,  x23, x8, x9, x5, x0, #0x130,  x19, x19, x3, #0x20, x10, #0x000, x14, #0x040, x18, 10, 7, 2, 2, 2 // NOP
        BBLK extrabranch03, extrabranch04,  v0.2D,  v1.2D,  v2.2D,  v3.2D,  v22.2D, v23.2D, v24.2D, v25.2D, v26.2D, v27.2D, v28.2D, v29.2D,  q14, q15,  x23, x9, x8, x9, x0, #0x1B0,  x19, x19, x3, #0x30, x10, #0x000, x14, #0x040, x18, 7,  5, 1, 2, 1 // prefetch, x1<-x0+400
        BBLK extrabranch04, extrabranch05,  v4.2D,  v5.2D,  v6.2D,  v7.2D,  v24.2D, v25.2D, v26.2D, v27.2D, v28.2D, v29.2D, v30.2D, v31.2D,  q12, q13,  x23, x6, x8, x9, x0, #0x230,  x19, x19, x3, #0x40, x10, #0x040, x14, #0x080, x18, 0,  1, 1, 2, 2 // NOP
        BBLK extrabranch05, extrabranch06,  v8.2D,  v9.2D, v10.2D, v11.2D,  v26.2D, v27.2D, v28.2D, v29.2D, v30.2D, v31.2D, v16.2D, v17.2D,  q14, q15,  x23, x8, x5, x8, x0, #0x2B0,  x19, x19, x3, #0x50, x10, #0x040, x14, #0x080, x18, 7,  1, 1, 2, 1 // prefetch
        BBLK extrabranch06, extrabranch07,  v0.2D,  v1.2D,  v2.2D,  v3.2D,  v28.2D, v29.2D, v30.2D, v31.2D, v16.2D, v17.2D, v18.2D, v19.2D,  q12, q13,  x23, x9, x8, x6, x0, #0x330,  x19, x19, x3, #0x60, x10, #0x040, x14, #0x0C0, x18, 0,  7, 1, 2, 2 // NOP
        BBLK extrabranch07, extrabranch08,  v4.2D,  v5.2D,  v6.2D,  v7.2D,  v30.2D, v31.2D, v16.2D, v17.2D, v18.2D, v19.2D, v20.2D, v21.2D,  q14, q15,  x23, x8, x9, x8, x0, #0x3B0,  x19, x19, x3, #0x70, x10, #0x040, x14, #0x0C0, x18, 7,  1, 1, 2, 1 // prefetch
        BBLK extrabranch08, extrabranch09,  v8.2D,  v9.2D, v10.2D, v11.2D,  v16.2D, v17.2D, v18.2D, v19.2D, v20.2D, v21.2D, v22.2D, v23.2D,  q12, q13,  x23, x5, x9, x8, x1, #0x030,  x19, x19, x3, #0x80, x10, #0x080, x14, #0x100, x18, 0,  1, 1, 2, 2 // NOP
        BBLK extrabranch09, extrabranch10,  v0.2D,  v1.2D,  v2.2D,  v3.2D,  v18.2D, v19.2D, v20.2D, v21.2D, v22.2D, v23.2D, v24.2D, v25.2D,  q14, q15,  x23, x9, x6, x9, x1, #0x0B0,  x19, x19, x3, #0x90, x10, #0x080, x14, #0x100, x18, 7,  7, 1, 2, 1 // prefetch
        BBLK extrabranch10, extrabranch11,  v4.2D,  v5.2D,  v6.2D,  v7.2D,  v20.2D, v21.2D, v22.2D, v23.2D, v24.2D, v25.2D, v26.2D, v27.2D,  q12, q13,  x23, x8, x9, x5, x1, #0x130,  x19, x19, x3, #0xA0, x10, #0x080, x14, #0x140, x18, 3,  1, 1, 2, 2 // copy from x1 to x0
        BBLK extrabranch11, extrabranch12,  v8.2D,  v9.2D, v10.2D, v11.2D,  v22.2D, v23.2D, v24.2D, v25.2D, v26.2D, v27.2D, v28.2D, v29.2D,  q14, q15,  x23, x9, x8, x9, x1, #0x1B0,  x19, x19, x3, #0xB0, x10, #0x080, x14, #0x140, x18, 7,  7, 1, 2, 1 // prefetch
        BBLK extrabranch12, extrabranch13,  v0.2D,  v1.2D,  v2.2D,  v3.2D,  v24.2D, v25.2D, v26.2D, v27.2D, v28.2D, v29.2D, v30.2D, v31.2D,  q12, q13,  x23, x6, x8, x9, x1, #0x230,  x19, x19, x3, #0xC0, x10, #0x0C0, x14, #0x180, x18, 5,  1, 3, 2, 2 // copy from x4 to x3
        BBLK extrabranch13, extrabranch14,  v4.2D,  v5.2D,  v6.2D,  v7.2D,  v26.2D, v27.2D, v28.2D, v29.2D, v30.2D, v31.2D, v16.2D, v17.2D,  q14, q15,  x23, x8, x5, x8, x1, #0x2B0,  x19, x19, x3, #0xD0, x10, #0x0C0, x14, #0x180, x18, 7,  3, 1, 2, 1 // prefetch, x14 to x15
        BBLK extrabranch14, extrabranch15,  v8.2D,  v9.2D, v10.2D, v11.2D,  v28.2D, v29.2D, v30.2D, v31.2D, v16.2D, v17.2D, v18.2D, v19.2D,  q12, q13,  x23, x9, x8, x6, x1, #0x330,  x19, x19, x3, #0xE0, x10, #0x0C0, x14, #0x1C0, x18, 8,  1, 1, 2, 2 // copy from x10 to x11
        BBLK extrabranch15, extrabranch48,  v0.2D,  v1.2D,  v2.2D,  v3.2D,  v30.2D, v31.2D, v16.2D, v17.2D, v18.2D, v19.2D, v20.2D, v21.2D,  q14, q15,  x23, x8, x9, x8, x1, #0x3B0,  x19, x19, x3, #0xF0, x10, #0x0C0, x14, #0x1C0, x18, 7,  7, 1, 2, 1 // prefetch

        BBLK extrabranch48, extrabranch49,  v4.2D,  v5.2D,  v6.2D,  v7.2D,  v16.2D, v17.2D, v18.2D, v19.2D, v20.2D, v21.2D, v22.2D, v23.2D,  q12, q13,  x23, x5, x9, x8, x0, #0x030,  x19, x19, x4, #0x00, x11, #0x000, x15, #0x000, x18, 0,  1, 2, 2, 2 // NOP
        BBLK extrabranch49, extrabranch50,  v8.2D,  v9.2D, v10.2D, v11.2D,  v18.2D, v19.2D, v20.2D, v21.2D, v22.2D, v23.2D, v24.2D, v25.2D,  q14, q15,  x23, x9, x6, x9, x0, #0x0B0,  x19, x19, x4, #0x10, x11, #0x000, x15, #0x000, x18, 7,  1, 2, 2, 3 // prefetch
        BBLK extrabranch50, extrabranch51,  v0.2D,  v1.2D,  v2.2D,  v3.2D,  v20.2D, v21.2D, v22.2D, v23.2D, v24.2D, v25.2D, v26.2D, v27.2D,  q12, q13,  x23, x8, x9, x5, x0, #0x130,  x19, x19, x4, #0x20, x11, #0x000, x15, #0x040, x18, 0,  7, 2, 2, 2 // NOP
        BBLK extrabranch51, extrabranch52,  v4.2D,  v5.2D,  v6.2D,  v7.2D,  v22.2D, v23.2D, v24.2D, v25.2D, v26.2D, v27.2D, v28.2D, v29.2D,  q14, q15,  x23, x9, x8, x9, x0, #0x1B0,  x19, x19, x4, #0x30, x11, #0x000, x15, #0x040, x18, 7,  5, 1, 2, 3 // prefetch, x1<-x0+400
        BBLK extrabranch52, extrabranch53,  v8.2D,  v9.2D, v10.2D, v11.2D,  v24.2D, v25.2D, v26.2D, v27.2D, v28.2D, v29.2D, v30.2D, v31.2D,  q12, q13,  x23, x6, x8, x9, x0, #0x230,  x19, x19, x4, #0x40, x11, #0x040, x15, #0x080, x18, 0,  1, 1, 2, 2 // NOP
        BBLK extrabranch53, extrabranch54,  v0.2D,  v1.2D,  v2.2D,  v3.2D,  v26.2D, v27.2D, v28.2D, v29.2D, v30.2D, v31.2D, v16.2D, v17.2D,  q14, q15,  x23, x8, x5, x8, x0, #0x2B0,  x19, x19, x4, #0x50, x11, #0x040, x15, #0x080, x18, 7,  1, 1, 2, 3 // prefetch
        BBLK extrabranch54, extrabranch55,  v4.2D,  v5.2D,  v6.2D,  v7.2D,  v28.2D, v29.2D, v30.2D, v31.2D, v16.2D, v17.2D, v18.2D, v19.2D,  q12, q13,  x23, x9, x8, x6, x0, #0x330,  x19, x19, x4, #0x60, x11, #0x040, x15, #0x0C0, x18, 0,  7, 3, 2, 2 // NOP
        BBLK extrabranch55, extrabranch56,  v8.2D,  v9.2D, v10.2D, v11.2D,  v30.2D, v31.2D, v16.2D, v17.2D, v18.2D, v19.2D, v20.2D, v21.2D,  q14, q15,  x23, x8, x9, x8, x0, #0x3B0,  x19, x19, x4, #0x70, x11, #0x040, x15, #0x0C0, x18, 7,  1, 1, 2, 3 // prefetch
        BBLK extrabranch56, extrabranch57,  v0.2D,  v1.2D,  v2.2D,  v3.2D,  v16.2D, v17.2D, v18.2D, v19.2D, v20.2D, v21.2D, v22.2D, v23.2D,  q12, q13,  x23, x5, x9, x8, x1, #0x030,  x19, x19, x4, #0x80, x11, #0x080, x15, #0x100, x18, 1,  1, 1, 2, 2 // decrement loop counter
        BBLK extrabranch57, extrabranch58,  v4.2D,  v5.2D,  v6.2D,  v7.2D,  v18.2D, v19.2D, v20.2D, v21.2D, v22.2D, v23.2D, v24.2D, v25.2D,  q14, q15,  x23, x9, x6, x9, x1, #0x0B0,  x19, x19, x4, #0x90, x11, #0x080, x15, #0x100, x18, 7,  7, 1, 2, 3 // prefetch
        BBLK extrabranch58, extrabranch59,  v8.2D,  v9.2D, v10.2D, v11.2D,  v20.2D, v21.2D, v22.2D, v23.2D, v24.2D, v25.2D, v26.2D, v27.2D,  q12, q13,  x23, x8, x9, x5, x1, #0x130,  x19, x19, x4, #0xA0, x11, #0x080, x15, #0x140, x18, 4,  1, 1, 2, 2 // recover x0
        BBLK extrabranch59, extrabranch60,  v0.2D,  v1.2D,  v2.2D,  v3.2D,  v22.2D, v23.2D, v24.2D, v25.2D, v26.2D, v27.2D, v28.2D, v29.2D,  q14, q15,  x23, x9, x8, x9, x1, #0x1B0,  x19, x19, x4, #0xB0, x11, #0x080, x15, #0x140, x18, 7,  7, 1, 2, 3 // prefetch
        BBLK extrabranch60, extrabranch61,  v4.2D,  v5.2D,  v6.2D,  v7.2D,  v24.2D, v25.2D, v26.2D, v27.2D, v28.2D, v29.2D, v30.2D, v31.2D,  q12, q13,  x23, x6, x8, x9, x1, #0x230,  x19, x19, x4, #0xC0, x11, #0x0C0, x15, #0x180, x18, 6,  1, 1, 2, 2 // copy from x3 to x4
        BBLK extrabranch61, extrabranch62,  v8.2D,  v9.2D, v10.2D, v11.2D,  v26.2D, v27.2D, v28.2D, v29.2D, v30.2D, v31.2D, v16.2D, v17.2D,  q14, q15,  x23, x8, x5, x8, x1, #0x2B0,  x19, x19, x4, #0xD0, x11, #0x0C0, x15, #0x180, x18, 7,  4, 1, 2, 3 // prefetch, x15 to x14
        BBLK extrabranch62, extrabranch63,  v0.2D,  v1.2D,  v2.2D,  v3.2D,  v28.2D, v29.2D, v30.2D, v31.2D, v16.2D, v17.2D, v18.2D, v19.2D,  q12, q13,  x23, x9, x8, x6, x1, #0x330,  x19, x19, x4, #0xE0, x11, #0x0C0, x15, #0x1C0, x18, 9,  1, 1, 2, 2 // copy from x11 to x10
        BBLK extrabranch63, check_inner_loop,  v4.2D,  v5.2D,  v6.2D,  v7.2D,  v30.2D, v31.2D, v16.2D, v17.2D, v18.2D, v19.2D, v20.2D, v21.2D,  q14, q15,  x23, x8, x9, x8, x1, #0x3B0,  x19, x19, x4, #0xF0, x11, #0x0C0, x15, #0x1C0, x12, 7,  7, 1, 2, 3 // prefetch

check_inner_loop:
       SUBS   w27, w27, #1
       BNE    extrabranch00

       MOV      x2, x0 // save initial address
       //end
       add      w29, w29 , #1
       SUBS   w26, w26, #1
       BNE    deimos_outer_loop

// begin output low
       LDR w19, [x24]
       LDR w20, =0xfffd
       AND w19, w19, w20
       STR w19, [x24]
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
