/*
 * Copyright (c) 2025 Qualcomm Innovation Center, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "nvic.h"

void uart_driver_init(void) { _enable_irq(); }

static void uart_puts(const char* str)
{
    while (*str) {
        *(volatile unsigned int*)0xc0000000 = *str++;
    }
}

void __attribute__((interrupt)) invalid_excp(void)
{
    // We never get here, unless the deliberate unhandled exception in c_entry() is uncommented.
    uart_puts("invalid exception happened\r\n");
}

void __attribute__((interrupt)) _handle_irq(void)
{
    uart_puts("IRQ 17 happened\r\n");
    *(volatile unsigned int*)0xc0001000 = 1;
}

void __attribute__((interrupt)) _handle_nmi(void)
{
    uart_puts("NMI happened\r\n");
    *(volatile unsigned int*)0xc0001004 = 1;
}

void __attribute__((interrupt)) _handle_systick(void)
{
    uart_puts("SysTick happened\r\n");
    *(volatile unsigned int*)0xc0001008 = 1;
}

void c_entry(void)
{
    nvic_enable_irq(0);
    nvic_enable_irq(17);

    uart_puts("Test program is running. Listening for interrupts.\r\n");
    *(volatile unsigned int*)0xc000100c = 1; // start IRQs generation

    while (1) {
        asm volatile("wfi");
    }
}
