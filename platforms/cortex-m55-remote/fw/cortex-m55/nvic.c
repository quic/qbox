#include <nvic.h>

volatile unsigned int* const nvic = (unsigned int*)0xE000E000;

#define NVIC_TYPE                    (0x004 >> 2)
#define NVIC_SYSTICK_CTRL_STATUS     (0x010 >> 2)
#define NVIC_SYSTICK_RELOAD_VAL      (0x014 >> 2)
#define NVIC_SYSTICK_VAL             (0x018 >> 2)
#define NVIC_SYSTICK_CALIBRATION_VAL (0x01C >> 2)
#define NVIC_SET_ENABLE(x)           ((0x100 + x) >> 2)
#define NVIC_CLR_ENABLE(x)           ((0x180 + x) >> 2)

void nvic_enable_irq(unsigned char irq)
{
    unsigned char reg = irq / 32;

    nvic[NVIC_SET_ENABLE(reg)] |= (1 << ((unsigned int)irq % 32));
}

void nvic_disable_irq(unsigned char irq)
{
    unsigned char reg = irq / 32;

    nvic[NVIC_CLR_ENABLE(reg)] |= (1 << ((unsigned int)irq % 32));
}
