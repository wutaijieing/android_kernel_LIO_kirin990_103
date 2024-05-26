/*
 * ananke_max_power_wfe.S
 *
 * little cluster wfe max power
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
.extern g_l1_memory_addr
.extern g_ananke_outer_loop_count
.extern g_ananke_inter_loop_count
.extern g_gpio_addr
.equ PAGE_SIZE,        4*1024
.equ COPY_SIZE,        8*1024
.equ INIT_SIZE,        32*1024

// Pick one PMU event from each group of PMU counters in the DPU
.equ PMU_L1DC_MISS_R,             0x0003
.equ PMU_L2DC_CACHE_RD,           0x0016
.equ PMU_L1D_TLB_RS,              0x0025
.equ PMU_L1DC_RD_RS,              0x0040
.equ PMU_LD_SPEC,                 0x0070
.equ PMU_L3_CACHE_REFILL_PF_RS,   0x00C0

.global ananke_max_power_pi_wfe

//; .global Start_test
ananke_max_power_pi_wfe:
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
        mov x23,x0 // for gpio addr
        mrs     x12, mpidr_el1          // Read MPIDR, cpu number
        ubfx    x12, x12, #8, #8        // x12 == CPU number 8~15: core number.
        // Enable debug then enable all breakpoint and watchpoint
        // comparators.  Enabling breakpoints and watchpoints
        // increases power consumption for the test because the
        // comparators have to compare addresses against the breakpoint
        // and watchpoint registers.

        // Enable halting debug by setting the HDE bit of the MSDCR
        // and then clear the OS lock to enable programming of the
        // breakpoint and watchpoint registers.
        mrs     x0, mdscr_el1
        orr     x0, x0, #(0x1 << 14)    // HDE bit
        msr     mdscr_el1, x0
        mov     x0, #0
        msr     oslar_el1, x0           // Clear OS lock
        isb

        // Set breakpoint and watchpoint value registers
        adr     x0, bkpt0
        msr     dbgbvr0_el1, x0   // Write DBGBVR0
        adr     x0, bkpt1
        msr     dbgbvr1_el1, x0   // Write DBGBVR1
        adr     x0, bkpt2
        msr     dbgbvr2_el1, x0   // Write DBGBVR2
        adr     x0, bkpt3
        msr     dbgbvr3_el1, x0   // Write DBGBVR3
        adr     x0, bkpt4
        msr     dbgbvr4_el1, x0   // Write DBGBVR4
        adr     x0, bkpt5
        msr     dbgbvr5_el1, x0   // Write DBGBVR5
        adr     x0, wpt0
        msr     dbgwvr0_el1, x0   // Write DBGWVR0
        adr     x0, wpt1
        msr     dbgwvr1_el1, x0   // Write DBGWVR1
        adr     x0, wpt2
        msr     dbgwvr2_el1, x0   // Write DBGWVR2
        adr     x0, wpt3
        msr     dbgwvr3_el1, x0   // Write DBGWVR3

        // Set breakpoint and watchpoint enable registers to enable
        // in all states.  The value written to the DBGBCRn registers
        // is:
        //   Bit[13]   (HMC) - 1'b1    - State matching (match all states)
        //   Bits[8:5] (BAS) - 4'b1111 - Match A32/A64 instructions
        //   Bits[2:1] (PMC) - 2'b11   - State matching (match all states)
        //   Bit[0]    (E)   - 1'b1    - Enable this breakpoint
        mov     x0, #0x21E7       // DBGBCRn value
        msr     dbgbcr0_el1, x0   // Write DBGBCR0
        msr     dbgbcr1_el1, x0   // Write DBGBCR1
        msr     dbgbcr2_el1, x0   // Write DBGBCR2
        msr     dbgbcr3_el1, x0   // Write DBGBCR3
        msr     dbgbcr4_el1, x0   // Write DBGBCR4
        msr     dbgbcr5_el1, x0   // Write DBGBCR5

        // The value written to the DBGWCRn registers is:
        //   Bit[13]    (HMC) - 1'b1  - State matching (match all states)
        //   Bits[12:5] (BAS) - 8'hFF - Watch all byte lanes
        //   Bits[2:1]  (PAC) - 2'b11 - State matching (match all states)
        //   Bit[0]     (E)   - 1'b1  - Enable this watchpoint
        mov     x0, #0x3FE7       // DBGWCRn value
        msr     dbgwcr0_el1, x0   // Write DBGWCR0
        msr     dbgwcr1_el1, x0   // Write DBGWCR1
        msr     dbgwcr2_el1, x0   // Write DBGWCR2
        msr     dbgwcr3_el1, x0   // Write DBGWCR3

        // ----------------------
        // Enable the PMU events
        // ----------------------

        // Permit PMU counting at EL3
        //mrs     x0, mdcr_el3            // Read MDCR_EL3
        //orr     x0, x0, #1<<17          // Set SPME
        //msr     mdcr_el3, x0            // Write MDCR_EL3

        // Clear all counters
        mrs     x0, pmcr_el0            // Read PMCR
        bic     x0, x0, #0xF            // Clear PMCR.{D,C,P,E}
        msr     pmcr_el0, x0            // Write PMCR
        isb

        // Set up counters
        mov     x1, #0x0
        msr     pmselr_el0, x1          // Write PMSELR
        mov     x1, #PMU_L1DC_MISS_R
        msr     pmxevtyper_el0, x1      // Write PM0EVTYPER

        mov     x1, #0x1
        msr     pmselr_el0, x1          // Write PMSELR
        mov     x1, #PMU_L2DC_CACHE_RD
        msr     pmxevtyper_el0, x1      // Write PM0EVTYPER

        mov     x1, #0x2
        msr     pmselr_el0, x1          // Write PMSELR
        mov     x1, #PMU_L1D_TLB_RS
        msr     pmxevtyper_el0, x1      // Write PM0EVTYPER

        mov     x1, #0x3
        msr     pmselr_el0, x1          // Write PMSELR
        mov     x1, #PMU_L1DC_RD_RS
        msr     pmxevtyper_el0, x1      // Write PM0EVTYPER

        mov     x1, #0x4
        msr     pmselr_el0, x1          // Write PMSELR
        mov     x1, #PMU_LD_SPEC
        msr     pmxevtyper_el0, x1      // Write PM0EVTYPER

        mov     x1, #0x5
        msr     pmselr_el0, x1          // Write PMSELR
        mov     x1, #PMU_L3_CACHE_REFILL_PF_RS
        msr     pmxevtyper_el0, x1      // Write PM0EVTYPER

        // Clear overflow status and enable
        ldr     x1, =0xffffffff
        msr     pmovsclr_el0, x1        // Write PMOVSCLR
        msr     pmcntenset_el0, x1      // Write PMCNTENSET

        // Reset and enable counters
        orr     x0, x0, #0x7            // Set PMCR.{C,P,E}
        msr     pmcr_el0, x0            // Write PMCR
        isb

        // Enable caches and MMU
        //mrs     x0, sctlr_el3
        //orr     x0, x0, #(0x1 << 2)     // C bit (data cache)
        //orr     x0, x0, #(0x1 << 12)    // I bit (instruction cache)
        //orr     x0, x0, #0x1            // M bit (MMU)
        //msr     sctlr_el3, x0
        //dsb     sy
        //isb     sy

        // ----------------------------
        // Pre-load the L1
        // ----------------------------
        // Disable the L1, and L3 prefetchers
        // init some register for preloading
        ldr     x3, =0xFFFFFFFFFFFFFFFF
        ldr     x4, =0xAAAA55555555AAAA
        ldr     x5, =0x5555AAAAAAAA5555

        // Bring INIT_SIZE of data starting at g_l1_memory_addr into the L1
        ldr     w9, =INIT_SIZE    // ok: .equ INIT_SIZE,        32*1024
        ldr     x0, =g_l1_memory_addr    // ok: .equ g_l1_memory_addr,        0x05000000 // L1 memory address
        ldr  x7, [x0]
        //mov     x7, x28
        add     x7, x7, x12, LSL 18  // Dedicated g_l1_memory_addr address space for each CPU R7 = R7 + (R12 << 18)
 preload_loop:
        stp     x4,  x4,  [x7]
        stp     x5,  x5,  [x7, #0x10]
        stp     x4,  x4,  [x7, #0x20]
        stp     x5,  x5,  [x7, #0x30]

        eor     x4,  x4,  x3
        eor     x5,  x5,  x3

        add     x7, x7, #0x40
        subs    w9, w9, #0x40
        b.ne    preload_loop

        //Evict data from L1 to upper cache levels. The intention
        //is to make the prefetchers to hit in the upper levels of cache

        // Invalidate the L1 data cache back to L2
        mov   w0,  #0x0
        msr   csselr_el1,  x0
        isb
        mrs  x0,  ccsidr_el1
        ubfx w0, w0, #13, #15
        mov  w1, #0x0

inv_loop:
        orr  w2,  w1,  #0x40000000
        orr  w3,  w1,  #0x80000000
        orr  w4,  w1,  #0xc0000000
        dc   cisw, x1
        dc   cisw, x2
        dc   cisw, x3
        dc   cisw, x4
        add  w1,  w1,  #0x40
        subs w0,  w0,  #1
        b.cs inv_loop

        //Invalidate the L2 data cache back to L3
        //Read the CLIDR first to check whether an L2 cache is present
        mrs  x0, clidr_el1       //CLIDR
        ubfx x1, x0, #3, #3      //Extract Ctype2 field
        ubfx x2, x0, #6, #3      //Extract Ctype3 field

        mrs  x4, S3_0_C15_C3_0   //Read CLUSTERCFR_EL1
        ubfx x3, x4, #4, #1      //Extract L3 present field from CLUSTERCFR

        cmp  x1, #0              // If Ctype2 is zero
        beq  l2_done             // then L2 is not present

        cmp  x2, #0              //Otherwise if Ctype3 is zero
        ccmp x3, #1, #0, eq      //then L2 is not present
        beq  l2_done

        mov  w0, #2              // Select L2 cache
        msr  csselr_el1, x0
        isb
        mrs  x0, ccsidr_el1
        ubfx w0, w0, #13, #15

        mov  w1, #2
inv_loop_l2:
        orr  w2, w1, #0x40000000
        orr  w3, w1, #0x80000000
        orr  w4, w1, #0xC0000000
        dc   cisw, x1
        dc   cisw, x2
        dc   cisw, x3
        dc   cisw, x4
        add  w1, w1, #0x40
        subs w0, w0, #1
        b.cs inv_loop_l2
l2_done:
        dsb   sy
        // ----------------------------
        // Set initial register values
        // ----------------------------

        movi v0.16b, #0x55  // q1 = 0x66666666
        movi v1.16b, #0xAA  // q1 = 0x66666666
        movi v2.16b, #0x55  // q2 = 0xAAAAAAAA
        movi v3.16b, #0xAA  // q3 = 0x66666666
        movi v4.16b, #0x55  // q4 = 0xAAAAAAAA
        movi v5.16b, #0xAA  // q5 = 0x66666666
        movi v6.16b, #0x55  // q6 = 0xAAAAAAAA
        movi v7.16b, #0xAA  // q7 = 0x66666666
        movi v8.16b, #0x55  // q8 = 0xAAAAAAAA
        movi v9.16b, #0xAA  // q9 = 0x66666666
        movi v10.16b, #0x55  // q9 = 0x66666666
        movi v11.16b, #0xAA  // q9 = 0x66666666
        movi v12.16b, #0x55  // q9 = 0x66666666
        movi v13.16b, #0xAA  // q9 = 0x66666666
        movi v14.16b, #0x55  // q9 = 0x66666666
        movi v15.16b, #0xAA  // q9 = 0x66666666
        movi v16.16b, #0x55  // q9 = 0x66666666
        movi v17.16b, #0xAA  // q9 = 0x66666666
        movi v18.16b, #0x55  // q9 = 0x66666666
        movi v19.16b, #0xAA  // q9 = 0x66666666
        movi v20.16b, #0x55  // q9 = 0x66666666
        movi v21.16b, #0xAA  // q9 = 0x66666666
        movi v22.16b, #0x55  // q9 = 0x66666666
        movi v23.16b, #0xAA  // q9 = 0x66666666
        movi v24.16b, #0x55  // q9 = 0x66666666

//-------------------------------------------------------------------------------
// Main power loop
//-------------------------------------------------------------------------------
        // Set up outer loop counter
        ldr x0, =g_ananke_outer_loop_count
        //ldr     w14, =MAIN_LOOP_ITERS
        ldr x14, [x0]

        // Align the loop to an 8-byte boundary so that two instructions
        // will be in the same instruction cache data bank.  This allows
        // us to dual-issue all pairs of instructions in the loop for
        // maximum power in the pipeline.
        .balign 8

ananke_test_start:
        // begin gpio169, bit 1 output high
        //LDR x23, =GPIO_DATA  // OK .equ GPIO_DATA ,            0xfa8a93fc  //GPIO21DATA, gpio169
        LDR w27, [x23]
        ORR w27, w27, #0x2
        // end
        sevl
        wfe
        wfe
        // begin gpio169, let less code here
        STR w27, [x23]
        // end

        ldr  x0, =g_l1_memory_addr
        ldr  x6, [x0]
        //mov     x6, x28
        add  x6, x6, x12, LSL #18
        add  x4, x6, #COPY_SIZE*4
        add  x4, x4, x12, LSL #12
ananke_outer_loop:
        //ldr     x11, =NUM_INNER_ITERS
        ldr x0, =g_ananke_inter_loop_count
        ldr x11, [x0]
        ldr     x0, =g_l1_memory_addr
        ldr  x6, [x0]
        //mov     x6, x28
        add     x6, x6, x12, LSL 18
        add     x7, x6, #COPY_SIZE
        add     x8, x6, #COPY_SIZE*2
        add     x9, x6, #COPY_SIZE
        .balign 8
power_inner_loop:
        // Group 1
        fmla      v0.4s, v1.4s, v2.4s  // Fmla 1
        ldr       x0, [x6, #0x40]     // Load data into x8
        fmla      v3.4s, v4.4s, v5.4s  // Load data into x8
        str       x0, [x7, #0x40]
        fmla      v6.4s, v7.4s, v8.4s
        ldr       x1, [x8, #0x40]
        fmla      v9.4s, v10.4s, v11.4s
        str       x1, [x9, #0x40]

        // Group 2
        fmla      v12.4s, v13.4s, v14.4s  // Fmla 2
        ldr       x0, [x6, #0x40]        // Advance read pointer
        fmla      v15.4s, v16.4s, v17.4s  // Fmla 2
        str       x0, [x7, #0x40]        // Advance read pointer
        fmla      v18.4s, v19.4s, v20.4s  // Fmla 2
        ldr       x1, [x8, #0x40]        // Advance read pointer
        fmla      v21.4s, v22.4s, v23.4s  // Fmla 2
        str       x1, [x9, #0x40]        // Advance read pointer

        // Group 3
        fmla      v0.4s, v4.4s, v5.4s     // Fmla 2
        ldr       x0, [x6, #0x40]        // Advance read pointer
        fmla      v3.4s, v1.4s, v2.4s     // Fmla 2
        str       x0, [x7, #0x40]        // Advance read pointer
        fmla      v6.4s, v10.4s, v11.4s   // Fmla 2
        ldr       x1, [x8, #0x40]        // Advance read pointer
        fmla      v9.4s, v7.4s, v8.4s     // Fmla 2
        str       x1, [x9, #0x40]        // Advance read pointer

        // Group 4
        fmla      v12.4s, v16.4s, v17.4s  // Fmla 2
        ldr       x0, [x6, #0x40]        // Advance read pointer
        fmla      v15.4s, v13.4s, v14.4s  // Fmla 2
        str       x0, [x7, #0x40]        // Advance read pointer
        fmla      v18.4s, v22.4s, v23.4s  // Fmla 2
        ldr       x1, [x8, #0x40]        // Advance read pointer
        fmla      v21.4s, v19.4s, v20.4s  // Fmla 2
        str       x1, [x9, #0x40]        // Advance read pointer

         // Group 5
        fmla      v12.4s, v13.4s, v14.4s  // Fmla 2
        prfm      pldl2strm, [x4]         // Advance read pointer
        fmla      v15.4s, v16.4s, v17.4s  // Fmla 2
        add       x4, x4, #0x40           // Advance read pointer
        fmla      v18.4s, v19.4s, v20.4s  // Fmla 2
        subs      x11, x11, #1            // Advance read pointer
        fmla      v21.4s, v22.4s, v23.4s  // Fmla 2
        // End of loop check
        bne     power_inner_loop

        subs    w14, w14, #1
        b.ne    ananke_outer_loop

        // begin output low
//              LDR x23, =0x10E81000
        LDR w27, [x23]
        LDR w20, =0xfffd
        AND w27, w27, w20
        STR w27, [x23]
        // end
//        add     w29, w29 , #1
        //b check_wait_ananke
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
//-------------------------------------------------------------------------------
// Breakpoint and watchpoint labels
//-------------------------------------------------------------------------------

// These labels are used to set the breakpoint and watchpoint registers.
// There is no real code at these labels and they are never executed: the
// breakpoints and watchpoints are set purely to increase power consumption.
bkpt0:        .word   0
bkpt1:        .word   0
bkpt2:        .word   0
bkpt3:        .word   0
bkpt4:        .word   0
bkpt5:        .word   0

// Align watchpoints to double-word boundaries so that all byte address select
// bits can be set in the watchpoint control register
        .balign 8
wpt0:        .word   0
             .word   0
wpt1:        .word   0
             .word   0
wpt2:        .word   0
             .word   0
wpt3:        .word   0
             .word   0
.end
