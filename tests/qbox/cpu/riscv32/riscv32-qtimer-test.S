.text
.global _start

_start:
    # Initialize stack pointer to a safe memory location
    # Use 0x10000 as stack top (64KB into memory, growing down)
    li sp, 0x10000

    # Set up interrupt vector table
    # RISC-V uses mtvec register to point to interrupt handler
    la t0, timer_interrupt_handler
    csrw mtvec, t0

    # Enable machine timer interrupts in mie register
    li t0, 0x80           # MIE_MTIE (bit 7) - Machine Timer Interrupt Enable
    csrw mie, t0

    # Enable interrupts globally in mstatus register
    li t0, 0x8            # MSTATUS_MIE (bit 3) - Machine Interrupt Enable
    csrw mstatus, t0

    # Load base addresses from constants section
    la a0, mmio_addr
    lw a0, 0(a0)          # MMIO communication address

    la a2, aclint_addr
    lw a2, 0(a2)          # ACLINT MTimer base address

    la a3, timer_period
    lw a3, 0(a3)          # Timer period in ticks

    # Initialize ACLINT MTimer for periodic interrupts

    # Read current time from ACLINT MTimer TIME register (at offset 0x7ff8)
    li t1, 0x7ff8
    add t0, a2, t1        # a2 + 0x7ff8 = TIME register address
    lw t1, 0(t0)          # Read lower 32 bits of TIME
    lw t2, 4(t0)          # Read upper 32 bits of TIME

    # Add a safety margin of 2 periods to avoid race condition
    # This ensures TIMECMP is safely in the future even with timing variations
    slli t3, a3, 1        # t3 = 2 * timer_period (safety margin)
    add t1, t1, t3        # Add safety margin to lower 32 bits
    # Note: Ignoring carry for simplicity in this test

    # Write to TIMECMP register (at offset 0x0) to set next interrupt
    sw t1, 0(a2)          # Write lower 32 bits to TIMECMP[0]
    sw t2, 4(a2)          # Write upper 32 bits to TIMECMP[0]

    # Signal that timer setup is complete
    la t0, setup_val
    lw t0, 0(t0)
    sw t0, 0(a0)          # Write TIMER_SETUP_COMPLETE to MMIO

    # Main loop - wait for timer interrupts
main_loop:
    wfi                   # Wait for interrupt
    j main_loop           # Loop back

# Timer interrupt handler
timer_interrupt_handler:
    # Save registers
    addi sp, sp, -16
    sw ra, 12(sp)
    sw t0, 8(sp)
    sw t1, 4(sp)
    sw a0, 0(sp)

    # Load base addresses
    la a0, mmio_addr
    lw a0, 0(a0)

    la t0, aclint_addr
    lw t0, 0(t0)

    la t1, timer_period
    lw t1, 0(t1)

    # Clear the timer interrupt by setting next TIMECMP
    # Read current TIME
    li a1, 0x7ff8
    add a1, t0, a1        # TIME register address
    lw a2, 0(a1)          # Read lower 32 bits of TIME
    lw a3, 4(a1)          # Read upper 32 bits of TIME

    # Calculate next interrupt time with safety margin to avoid race condition
    # Add double the period to ensure TIMECMP is safely in the future
    slli a4, t1, 1        # a4 = 2 * timer_period (safety margin)
    add a2, a2, a4        # Add safety margin to lower 32 bits

    # Write new TIMECMP to schedule next interrupt
    sw a2, 0(t0)          # Write lower 32 bits to TIMECMP[0]
    sw a3, 4(t0)          # Write upper 32 bits to TIMECMP[0]

    # Signal that timer interrupt was handled
    la t0, handled_val
    lw t0, 0(t0)
    sw t0, 0(a0)          # Write TIMER_INTERRUPT_HANDLED to MMIO

    # Restore registers
    lw a0, 0(sp)
    lw t1, 4(sp)
    lw t0, 8(sp)
    lw ra, 12(sp)
    addi sp, sp, 16

    # Return from interrupt
    mret

# Constants section - will be patched by the test framework
.align 4
mmio_addr:
    .word 0x80000000    # MMIO_ADDR placeholder
aclint_addr:
    .word 0x02000000    # ACLINT_MTIMER_BASE placeholder
timer_period:
    .word 0x00002710    # TIMER_PERIOD_TICKS placeholder (10000 decimal)
setup_val:
    .word 0x00000001    # TIMER_SETUP_COMPLETE placeholder
handled_val:
    .word 0x00000002    # TIMER_INTERRUPT_HANDLED placeholder
complete_val:
    .word 0x00000003    # TEST_COMPLETE placeholder
