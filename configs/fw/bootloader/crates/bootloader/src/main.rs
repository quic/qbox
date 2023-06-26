#![feature(asm_const)]
#![no_std]
#![no_main]
use core::{arch::global_asm, panic::PanicInfo};

#[panic_handler]
fn panic(_info: &PanicInfo) -> ! {
    loop {}
}

global_asm!(
    r#"
    .section .text._start
    .global _start
    .start:
    mrs x6, mpidr_el1
    ldr x7, .aff_mask
    and x7, x7, x6
    cmp x7, #0x0
    b.ne .secondary
    .core0:
    ldr x4, .kernel_entry
    b .boot

    .secondary:
    wfe
    ldr x4, .spintable
    cbz x4, .secondary

    .boot:
    ldr x0, .dtb_ptr
    mov x1, xzr
    mov x2, xzr
    mov x3, xzr
    br x4
    
    .word 0x00000000
    .aff_mask:
    .word {AFF_MASK}

    .word 0x00000000
    .kernel_entry:
    .word {KERNEL_ENTRY}

    .word 0x00000000
    .dtb_ptr:
    .word {DTB_PTR}

    .word 0x00000000
    .spintable:
    .word 0x00000000
    "#,
    AFF_MASK = const 0x00ffffff,
    KERNEL_ENTRY = const 0x00080000,
    DTB_PTR = const 0x08000000,
);
