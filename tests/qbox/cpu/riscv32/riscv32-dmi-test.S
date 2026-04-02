.text
.global _start

_start:
    # Load constants from the end of the binary (patched by test framework)
    la t0, dmi_addr_const   # Load address of DMI constant
    lw s0, 0(t0)            # s0 = patched DMI base address

    la t0, mmio_addr_const  # Load address of MMIO constant
    lw s1, 0(t0)            # s1 = patched MMIO base address

    la t0, num_writes_const # Load address of num_writes constant
    lw s2, 0(t0)            # s2 = patched num_writes value

    # Get CPU ID (hartid)
    csrr t1, mhartid        # t1 = CPU ID

    # Calculate CPU-specific offset (multiply by 8)
    slli t2, t1, 3          # t2 = CPU ID * 8

    # Add offset to base addresses
    add s0, s0, t2          # s0 = DMI address for this CPU
    add s1, s1, t2          # s1 = MMIO address for this CPU

    # Init: will ask for a DMI pointer and go from ST_START to ST_READ_DMI
    lw t3, 0(s0)            # Read from DMI (use calculated address)
    sw t3, 0(s1)            # Write to MMIO (report, use calculated address)

    # Initialize counter
    li t4, 0                # t4 = counter

loop:
    # Read on DMI socket, report the read on the control socket
    lw t3, 0(s0)            # Read from DMI (use calculated address)
    sw t3, 0(s1)            # Write to MMIO (report, use calculated address)

    # Increment counter
    addi t4, t4, 1          # t4++

    # Write on DMI socket, report the write on the control socket
    sw t4, 0(s0)            # Write counter to DMI (use calculated address)
    sw t4, 0(s1)            # Write to MMIO (report, use calculated address)

    # Check if we've done enough writes
    bne t4, s2, loop        # if (t4 != num_writes) goto loop

end:
    wfi                     # Wait for interrupt
    j end                   # Infinite loop

# Constants section - will be filled in by the test at runtime
.align 4
dmi_addr_const:
    .word 0x90000000        # DMI_ADDR placeholder
mmio_addr_const:
    .word 0x80000000        # MMIO_ADDR placeholder
num_writes_const:
    .word 0x00000200        # NUM_WRITES placeholder
