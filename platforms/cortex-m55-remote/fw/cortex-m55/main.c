#include "nvic.h"

void uart_driver_init(void)
{
}

void invalid_excp(void)
{
}

static void uart_puts(const char *str)
{
    while (*str) {
        *(volatile unsigned int *)0xc0000000 = *str++;
    }
}

void c_entry(void)
{
    nvic_enable_irq(0);

    uart_puts("Hello from cortex-m55!\r\n");

    while (1) {
        asm volatile ("wfi");
    }
}
