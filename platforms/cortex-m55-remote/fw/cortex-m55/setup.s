.thumb
.global _start
.global _invalid_exception
.global _disable_irq
.global _enable_irq
.global _handle_nmi
.global _handle_irq
.global _handle_systick
.global _stop

init_vectors:
  .word _initial_sp                 @ msp = _initial_sp
  .word _start + 1                  @ Starting Program address
  .word _handle_nmi + 1	    @ NMI_Handler
  .word _invalid_exception + 1	    @ HardFault_Handler
  .word _invalid_exception + 1      @ MemManage_Handler
  .word _invalid_exception + 1      @ BusFault_Handler
  .word _invalid_exception + 1	    @ UsageFault_Handler
  .word 0                           @ 7
  .word 0                           @ To
  .word 0                           @ 10
  .word 0                           @ Reserved
  .word _invalid_exception + 1	    @ SVC_Handler
  .word _invalid_exception + 1	    @ DebugMon_Handler
  .word 0                           @ Reserved
  .word _invalid_exception + 1	    @ PendSV_Handler
  .word _handle_systick + 1	    @ SysTick_Handler
  .word _invalid_exception + 1	    @ IRQ0
  .word _invalid_exception + 1
  .word _invalid_exception + 1
  .word _invalid_exception + 1
  .word _invalid_exception + 1
  .word _invalid_exception + 1
  .word _invalid_exception + 1
  .word _invalid_exception + 1
  .word _invalid_exception + 1
  .word _invalid_exception + 1
  .word _invalid_exception + 1
  .word _invalid_exception + 1
  .word _invalid_exception + 1
  .word _invalid_exception + 1
  .word _invalid_exception + 1
  .word _invalid_exception + 1
  .word _invalid_exception + 1
  .word _handle_irq + 1      @ IRQ17
  .word _invalid_exception + 1
  .word _invalid_exception + 1
  .word _invalid_exception + 1
  .word _invalid_exception + 1
  .word _invalid_exception + 1
  .word _invalid_exception + 1
  .word _invalid_exception + 1
  .word _invalid_exception + 1
  .word _invalid_exception + 1
  .word _invalid_exception + 1
  .word _invalid_exception + 1
  .word _invalid_exception + 1
  .word _invalid_exception + 1
  .word _invalid_exception + 1
  .word _invalid_exception + 1
  .word _invalid_exception + 1
  .word _invalid_exception + 1
  .word _invalid_exception + 1
  .word _invalid_exception + 1
  .word _invalid_exception + 1
  .word _invalid_exception + 1
  .word _invalid_exception + 1
  .word _invalid_exception + 1
  .word _invalid_exception + 1
  .word _invalid_exception + 1
  .word _invalid_exception + 1
  .word _invalid_exception + 1
  .word _invalid_exception + 1
  .word _invalid_exception + 1
  .word _invalid_exception + 1
  .word _invalid_exception + 1
  .word _invalid_exception + 1
  .word _invalid_exception + 1
  .word _invalid_exception + 1
  .word _invalid_exception + 1
  .word _invalid_exception + 1
  .word _invalid_exception + 1
  .word _invalid_exception + 1
  .word _invalid_exception + 1
  .word _invalid_exception + 1
  .word _invalid_exception + 1
  .word _invalid_exception + 1
  .word _invalid_exception + 1
  .word _invalid_exception + 1
  .word _invalid_exception + 1
  .word _invalid_exception + 1     @ IRQ31

_start:
  mrs r0, msp
  sub r0, r0, #100
  msr psp, r0
  bl uart_driver_init
  bl c_entry
  bl _idle

_invalid_exception:
  bl invalid_excp
  bl _stop

_stop:
  bl _disable_irq
  b _idle

_disable_irq:
  cpsid i
  bx lr

_enable_irq:
  cpsie i
  bx lr

_idle:
  wfi
  b _idle
