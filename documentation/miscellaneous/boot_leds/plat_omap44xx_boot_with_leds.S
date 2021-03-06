/**
 * \file
 * \brief Bootstrap the kernel for ARMv7 processors.  This code is
 *      entered from the bootloader (typically arm_molly, RedBoot,
 *      etc.).
 */
/*
 * Copyright (c) 2009,2016 ETH Zurich.
 * All rights reserved.
 *
 * This file is distributed under the terms in the attached LICENSE file.
 * If you do not find this file, copies can be found by writing to:
 * ETH Zurich D-INFK, Haldeneggsteig 4, CH-8092 Zurich. Attn: Systems Group.
 */

/**
 * Group C: extended version that blinks LEDs during boot process of 2nd core
 * - board starts with LED1 off and LED2 on
 * - shortly both LEDs off
 * - LED1 on for core 0
 * - awakening cores: both LEDs off
 * - if not yet for core 1 -> LED1 for core 0
 *   if booting core 1 -> both LEDs for both cores
 *
 * see boot_with_leds_helper.c for the C code used for the LEDs
 *
 * version 2017-11-10, pisch
 */


#ifndef __ASSEMBLER__
#define __ASSEMBLER__   1
#endif

#include <barrelfish_kpi/flags_arch.h> // ARM_MODE_MASK
#include <offsets.h> // KERNEL_STACK_SIZE
#include <asmoffsets.h>

    .globl start, target_mpid

    .bss
    .align 8
// This stack is used by the boot driver on every core - the CPU driver has
// its own stack on each core.  This means that two boot drivers shouldn't run
// simultaneously, unless and until we change this.
boot_stack:
    .space KERNEL_STACK_SIZE, 0
boot_stack_top:

    .section .text
    .arm

    .extern got_base

    .extern boot_leds_init
    .extern boot_leds_one
    .extern boot_leds_two
    .extern boot_leds_both
    .extern boot_leds_off
    .extern boot_leds_wait

// On ARM platforms that don't implement their own boot protocol (e.g.
// simulators), all cores will start executing here, and it's up to us to park
// the non-BSP cores.
start:
    // On entry:
    //
    // MMU disabled
    // Caches in unknown state, but no lockdown
    // No TLB lockdown.
    // CPU is in a privileged mode.
    //

    // Switch to system mode, with aborts and interrupts off.
    cpsid aif, #ARM_MODE_SYS

    // Init stack
    ldr sp, =boot_stack_top

    ldr PIC_REGISTER, =got_base

    // Check to see if we're core 0.
    mrc p15, 0, r3, c0, c0, 5
    and r3, r3, #0xffffff
    cmp r3, #0

    // If not, wait for the BSP to wake us up.
    bne wait_for_bsp_first

    // Prepare argument
    b boot_bsp_core
    b halt


wait_for_bsp_first:
    bl boot_leds_init
    bl boot_leds_off
    bl boot_leds_one
wait_for_bsp:
    // show LED1 for core 0 while waiting
    bl boot_leds_one

    // Get the address of our mailbox.  Right now there's only one.
    ldr r4, got_boot_records
    ldr r0, [PIC_REGISTER, r4]
    // r3 - MPID
    wfe

    // deactivate LEDs while making the decision
    bl boot_leds_off

    // Check whether we're the core that's being woken.  If not, go back to
    // sleep.
    mrc p15, 0, r3, c0, c0, 5
    and r3, r3, #0xffffff
    ldr r4, [r0, #OFFSETOF_BOOT_TARGET_MPID]
    cmp r3, r4
    bne wait_for_bsp

    // show both LEDs for both cores
    bl boot_leds_both

    // Pass the boot record to boot_app_core()
    b boot_app_core
    b halt

// The GOT offset of the boot record (mailbox) table.
    .type got_boot_records, STT_OBJECT
got_boot_records:
    .word boot_records(GOT)
