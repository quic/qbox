--[[ Arm64 bootloader useful for QQVP debugging.
This bootloader writes some key values at adress 0x60000000.
Set a breakpoint at qemu_io_write to inspect those values. ]]
_bootloader_aarch64 = {
    0xd2ac0001,          --/* <start>: mov x5, 0x60000000 */
    0xd53800a6,          --/* mrs x6, mpidr_el1 */
    0x58000287,          --/* ldr x7, 0x50(20) <aff_mask> */
    0xf90000a7,          --/* str x7, [x5] */
    0x8a0600e7,          --/* and x7, x7, x6 */
    0xf10000ff,          --/* cmp x7, #0x0 */
    0x54000021,          --/* b.ne 0x10 <secondary>
    0x58000224,          --/* <core0>: ldr x4, 0x44(17) <kernel_entry> */
    0xf90000a4,          --/* str x4, [x5] */
    0x14000005,          --/* b 0x14 <boot> */
    0xd503205f,          --/* <secondary>: wfe */
    0x58000264,          --/* ldr x4, 0x4c(19) <spintable> */
    0xf90000a4,          --/* str x4, [x5] */
    0xb4ffffa4,          --/* cbz x4, #-0xc <secondary> */
    0x58000180,          --/* <boot>: ldr x0, 0x30(12) <dtb_ptr> */
    0xf90000a0,          --/* str x0, [x5] */
    0xaa1f03e1,          --/* mov x1, xzr */
    0xaa1f03e2,          --/* mov x2, xzr */
    0xaa1f03e3,          --/* mov x3, xzr */
    0xd61f0080,          --/* br x4 */
    0x00000000,          --/* .word 0x00000000 */
    0x00000000,          --/* .word 0x00000000 */
    0x00ffffff,          --/* <aff_mask>: .word 0x00ffffff */ 
    0x00000000,          --/* .word 0x00000000 */
    _KERNEL64_LOAD_ADDR, --/* <kernel_entry>: .word 0x00080000 */
    0x00000000,          --/* .word 0x00000000 */
    _DTB_LOAD_ADDR,      --/* <dtb_ptr>: .word 0x08000000 */
    0x00000000,          --/* .word 0x00000000 */
    0x00000000,          --/* .word 0x00000000 */
    0x00000000,          --/* .word 0x00000000 */
    0x00000000,          --/* .word 0x00000000 */
    0x00000000,          --/* .word 0x00000000 */
    0x00000000,          --/* .word 0x00000000 */
    0x00000000,          --/* .word 0x00000000 */
    0x00000000,          --/* .word 0x00000000 */
    0x00000000,          --/* .word 0x00000000 */
}
