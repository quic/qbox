.text
.global _start

_start:
    # Load MMIO address from constants section
    la a0, mmio_addr
    lw a0, 0(a0)

    # Load trigger value from constants section
    la a1, trigger_val
    lw a1, 0(a1)

    # Load done value for comparison
    la t0, done_val
    lw t0, 0(t0)

    # Compare trigger_val with done_val
    # If trigger_val == done_val, branch to reset_done (skip the loop)
    beq a1, t0, reset_done

reset:
    # Write trigger value and loop infinitely
    sw a1, 0(a0)        # Write to MMIO
    j reset             # Infinite loop

reset_done:
    # Write done value and wait for interrupt
    sw a1, 0(a0)        # Write to MMIO
    wfi                 # Wait for interrupt

# Constants section - will be filled in by CMake at build time
.align 4
mmio_addr:
    .word 0x80000000    # MMIO_ADDR placeholder
trigger_val:
    .word 0x00000001    # RESET_TRIGGER placeholder
done_val:
    .word 0x00000000    # RESET_DONE placeholder
