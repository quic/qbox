_bootloader_riscv64 = {
     0x00000297,                 -- /*     1: auipc   t0,0x0 */
     0x03428613,                 -- /*        addi    a2,t0,52 # 10074 <fw_dyn> */
     0xf1402573,                 -- /*        csrr    a0,mhartid */
     0x00000013,                 -- /*        nop                */
     0x02c2b583,                 -- /*        ld      a1,44(t0) */
     0x0242b283,                 -- /*        ld      t0,36(t0) */
     0x00028067,                 -- /*        jr      t0 */
     0x10500073,                 -- /*  stop: wfi */
     0xffdff06f,                 -- /*        j       1005c <stop> */
     (_OPENSBI_LOAD_ADDR & 0xffffffff), -- /* start: .dword */
     (_OPENSBI_LOAD_ADDR >> 32),
     (_DTB_LOAD_ADDR & 0xffffffff),     -- /* fdt_laddr: .dword */
     (_DTB_LOAD_ADDR >> 32),
     -- /* fw_dyn: */
     0x4942534f, 0,               -- /* OSBI magic */
     2, 0,                        -- /* OSBI version */
     (_KERNEL64_LOAD_ADDR & 0xffffffff),  -- /* OSBI next address */
     (_KERNEL64_LOAD_ADDR >> 32),
     1, 0,                       -- /* OSBI next mode: S */
     0, 0,                       -- /* OSBI options */
     0, 0,                       -- /* OSBI prefered boot hart */
}