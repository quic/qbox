/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 *
 * ARM SMMUv3 Register Layout
 */

#ifndef _SMMUV3_GEN_H_
#define _SMMUV3_GEN_H_

#include <registers.h>
#include <reg_model_maker/reg_model_maker.h>
#include <string>
#include <vector>
#include <memory>

namespace gs {

class smmuv3_gen
{
public:
    // ID registers (0x0 - 0x1C)
    gs_register<uint32_t> IDR0;
    gs_field<uint32_t> IDR0_S2P;
    gs_field<uint32_t> IDR0_S1P;
    gs_field<uint32_t> IDR0_TTF;
    gs_field<uint32_t> IDR0_COHACC;
    gs_field<uint32_t> IDR0_ASID16;
    gs_field<uint32_t> IDR0_VMID16;
    gs_field<uint32_t> IDR0_PRI;
    gs_field<uint32_t> IDR0_ATOS;
    gs_field<uint32_t> IDR0_HTTU;
    gs_field<uint32_t> IDR0_STLEVEL;

    gs_register<uint32_t> IDR1;
    gs_field<uint32_t> IDR1_SIDSIZE;
    gs_field<uint32_t> IDR1_EVENTQS;
    gs_field<uint32_t> IDR1_CMDQS;

    gs_register<uint32_t> IDR2;
    gs_register<uint32_t> IDR3;
    gs_field<uint32_t> IDR3_BBML1;
    gs_field<uint32_t> IDR3_BBML2;

    gs_register<uint32_t> IDR4;
    gs_register<uint32_t> IDR5;
    gs_field<uint32_t> IDR5_OAS;
    gs_field<uint32_t> IDR5_GRAN4K;
    gs_field<uint32_t> IDR5_GRAN16K;
    gs_field<uint32_t> IDR5_GRAN64K;

    gs_register<uint32_t> IIDR;
    gs_register<uint32_t> AIDR;

    // Control registers (0x20 - 0x2C)
    gs_register<uint32_t> CR0;
    gs_field<uint32_t> CR0_SMMUEN;
    gs_field<uint32_t> CR0_PRIQEN;
    gs_field<uint32_t> CR0_EVENTQEN;
    gs_field<uint32_t> CR0_CMDQEN;
    gs_field<uint32_t> CR0_ATSCHK;
    gs_field<uint32_t> CR0_NSCFG;
    gs_field<uint32_t> CR0_VMW;

    gs_register<uint32_t> CR0ACK;
    gs_register<uint32_t> CR1;
    gs_field<uint32_t> CR1_TABLE_SH;
    gs_field<uint32_t> CR1_TABLE_OC;
    gs_field<uint32_t> CR1_TABLE_IC;
    gs_field<uint32_t> CR1_QUEUE_SH;
    gs_field<uint32_t> CR1_QUEUE_OC;
    gs_field<uint32_t> CR1_QUEUE_IC;

    gs_register<uint32_t> CR2;
    gs_field<uint32_t> CR2_PTM;
    gs_field<uint32_t> CR2_RECINVSID;
    gs_field<uint32_t> CR2_E2H;

    // Status and bypass (0x40 - 0x48)
    gs_register<uint32_t> STATUSR;
    gs_register<uint32_t> GBPA;
    gs_field<uint32_t> GBPA_UPDATE;
    gs_field<uint32_t> GBPA_ABORT;

    gs_register<uint32_t> AGBPA;

    // IRQ control (0x50 - 0x54)
    gs_register<uint32_t> IRQ_CTRL;
    gs_field<uint32_t> IRQ_CTRL_GERROR_IRQEN;
    gs_field<uint32_t> IRQ_CTRL_PRI_IRQEN;
    gs_field<uint32_t> IRQ_CTRL_EVENTQ_IRQEN;

    gs_register<uint32_t> IRQ_CTRL_ACK;

    // Global error (0x60 - 0x64)
    gs_register<uint32_t> GERROR;
    gs_field<uint32_t> GERROR_CMDQ_ERR;
    gs_field<uint32_t> GERROR_EVENTQ_ABT_ERR;
    gs_field<uint32_t> GERROR_PRIQ_ABT_ERR;
    gs_field<uint32_t> GERROR_MSI_CMDQ_ABT_ERR;
    gs_field<uint32_t> GERROR_MSI_EVENTQ_ABT_ERR;
    gs_field<uint32_t> GERROR_MSI_PRIQ_ABT_ERR;
    gs_field<uint32_t> GERROR_MSI_GERROR_ABT_ERR;
    gs_field<uint32_t> GERROR_SFM_ERR;

    gs_register<uint32_t> GERRORN;

    // GERROR IRQ config (0x68 - 0x74)
    gs_register<uint32_t> GERROR_IRQ_CFG0_LO;
    gs_register<uint32_t> GERROR_IRQ_CFG0_HI;
    gs_register<uint32_t> GERROR_IRQ_CFG1;
    gs_register<uint32_t> GERROR_IRQ_CFG2;

    // Stream table base (0x80 - 0x88)
    gs_register<uint32_t> STRTAB_BASE_LO;
    gs_register<uint32_t> STRTAB_BASE_HI;
    gs_register<uint32_t> STRTAB_BASE_CFG;
    gs_field<uint32_t> STRTAB_BASE_CFG_FMT;
    gs_field<uint32_t> STRTAB_BASE_CFG_SPLIT;
    gs_field<uint32_t> STRTAB_BASE_CFG_LOG2SIZE;

    // Command queue (0x90 - 0x9C)
    gs_register<uint32_t> CMDQ_BASE_LO;
    gs_field<uint32_t> CMDQ_BASE_LOG2SIZE;

    gs_register<uint32_t> CMDQ_BASE_HI;
    gs_register<uint32_t> CMDQ_PROD;
    gs_field<uint32_t> CMDQ_PROD_WRP;
    gs_field<uint32_t> CMDQ_PROD_RD;

    gs_register<uint32_t> CMDQ_CONS;
    gs_field<uint32_t> CMDQ_CONS_WRP;
    gs_field<uint32_t> CMDQ_CONS_RD;
    gs_field<uint32_t> CMDQ_CONS_ERR;

    // Event queue (0xA0 - 0xBC)
    gs_register<uint32_t> EVENTQ_BASE_LO;
    gs_field<uint32_t> EVENTQ_BASE_LOG2SIZE;

    gs_register<uint32_t> EVENTQ_BASE_HI;
    gs_register<uint32_t> EVENTQ_PROD;
    gs_field<uint32_t> EVENTQ_PROD_WRP;
    gs_field<uint32_t> EVENTQ_PROD_RD;

    gs_register<uint32_t> EVENTQ_CONS;
    gs_field<uint32_t> EVENTQ_CONS_WRP;
    gs_field<uint32_t> EVENTQ_CONS_RD;

    gs_register<uint32_t> EVENTQ_IRQ_CFG0_LO;
    gs_register<uint32_t> EVENTQ_IRQ_CFG0_HI;
    gs_register<uint32_t> EVENTQ_IRQ_CFG1;
    gs_register<uint32_t> EVENTQ_IRQ_CFG2;

    // PRI queue (0xC0 - 0xDC)
    gs_register<uint32_t> PRIQ_BASE_LO;
    gs_field<uint32_t> PRIQ_BASE_LOG2SIZE;

    gs_register<uint32_t> PRIQ_BASE_HI;
    gs_register<uint32_t> PRIQ_PROD;
    gs_field<uint32_t> PRIQ_PROD_WRP;
    gs_field<uint32_t> PRIQ_PROD_RD;

    gs_register<uint32_t> PRIQ_CONS;
    gs_field<uint32_t> PRIQ_CONS_WRP;
    gs_field<uint32_t> PRIQ_CONS_RD;

    gs_register<uint32_t> PRIQ_IRQ_CFG0_LO;
    gs_register<uint32_t> PRIQ_IRQ_CFG0_HI;
    gs_register<uint32_t> PRIQ_IRQ_CFG1;
    gs_register<uint32_t> PRIQ_IRQ_CFG2;

    // GATOS (Global Address Translation Operations) (0x100 - 0x11C)
    gs_register<uint32_t> GATOS_CTRL;
    gs_field<uint32_t> GATOS_CTRL_RUN;
    gs_field<uint32_t> GATOS_CTRL_INPRG;
    gs_field<uint32_t> GATOS_CTRL_READ_NWRITE;

    gs_register<uint32_t> GATOS_SID;
    gs_register<uint32_t> GATOS_ADDR_LO;
    gs_register<uint32_t> GATOS_ADDR_HI;
    gs_register<uint32_t> GATOS_PAR_LO;
    gs_field<uint32_t> GATOS_PAR_F;
    gs_field<uint32_t> GATOS_PAR_FST;

    gs_register<uint32_t> GATOS_PAR_HI;

    // PrimeCell ID registers (0xFD0 - 0xFFC)
    std::vector<std::shared_ptr<gs_register<uint32_t>>> IDREGS;

    // Page 1 register aliases — Page 1 mirrors Page 0 queue registers.
    // Software typically writes EVENTQ/PRIQ PROD/CONS through the Page 1 aliases.
    gs_register<uint32_t> CMDQ_PROD_P1;   // 0x10098
    gs_register<uint32_t> CMDQ_CONS_P1;   // 0x1009C
    gs_register<uint32_t> EVENTQ_PROD_P1; // 0x100A8
    gs_register<uint32_t> EVENTQ_CONS_P1; // 0x100AC
    gs_register<uint32_t> PRIQ_PROD_P1;   // 0x100C8
    gs_register<uint32_t> PRIQ_CONS_P1;   // 0x100CC

    smmuv3_gen()
        : IDR0("IDR0", "smmuv3.IDR0", 0x0, 1)
        , IDR0_S2P(IDR0, "smmuv3.IDR0.S2P", 0, 1)
        , IDR0_S1P(IDR0, "smmuv3.IDR0.S1P", 1, 1)
        , IDR0_TTF(IDR0, "smmuv3.IDR0.TTF", 2, 2)
        , IDR0_COHACC(IDR0, "smmuv3.IDR0.COHACC", 4, 1)
        , IDR0_ASID16(IDR0, "smmuv3.IDR0.ASID16", 12, 1)
        , IDR0_VMID16(IDR0, "smmuv3.IDR0.VMID16", 18, 1)
        , IDR0_PRI(IDR0, "smmuv3.IDR0.PRI", 16, 1)
        , IDR0_ATOS(IDR0, "smmuv3.IDR0.ATOS", 15, 1)
        , IDR0_HTTU(IDR0, "smmuv3.IDR0.HTTU", 6, 2)
        , IDR0_STLEVEL(IDR0, "smmuv3.IDR0.STLEVEL", 27, 2)
        , IDR1("IDR1", "smmuv3.IDR1", 0x4, 1)
        , IDR1_SIDSIZE(IDR1, "smmuv3.IDR1.SIDSIZE", 0, 6)
        , IDR1_EVENTQS(IDR1, "smmuv3.IDR1.EVENTQS", 16, 5)
        , IDR1_CMDQS(IDR1, "smmuv3.IDR1.CMDQS", 21, 5)
        , IDR2("IDR2", "smmuv3.IDR2", 0x8, 1)
        , IDR3("IDR3", "smmuv3.IDR3", 0xC, 1)
        , IDR3_BBML1(IDR3, "smmuv3.IDR3.BBML1", 11, 1)
        , IDR3_BBML2(IDR3, "smmuv3.IDR3.BBML2", 12, 1)
        , IDR4("IDR4", "smmuv3.IDR4", 0x10, 1)
        , IDR5("IDR5", "smmuv3.IDR5", 0x14, 1)
        , IDR5_OAS(IDR5, "smmuv3.IDR5.OAS", 0, 3)
        , IDR5_GRAN4K(IDR5, "smmuv3.IDR5.GRAN4K", 4, 1)
        , IDR5_GRAN16K(IDR5, "smmuv3.IDR5.GRAN16K", 5, 1)
        , IDR5_GRAN64K(IDR5, "smmuv3.IDR5.GRAN64K", 6, 1)
        , IIDR("IIDR", "smmuv3.IIDR", 0x18, 1)
        , AIDR("AIDR", "smmuv3.AIDR", 0x1C, 1)
        , CR0("CR0", "smmuv3.CR0", 0x20, 1)
        , CR0_SMMUEN(CR0, "smmuv3.CR0.SMMUEN", 0, 1)
        , CR0_PRIQEN(CR0, "smmuv3.CR0.PRIQEN", 1, 1)
        , CR0_EVENTQEN(CR0, "smmuv3.CR0.EVENTQEN", 2, 1)
        , CR0_CMDQEN(CR0, "smmuv3.CR0.CMDQEN", 3, 1)
        , CR0_ATSCHK(CR0, "smmuv3.CR0.ATSCHK", 4, 1)
        , CR0_VMW(CR0, "smmuv3.CR0.VMW", 6, 3)
        , CR0_NSCFG(CR0, "smmuv3.CR0.NSCFG", 28, 2)
        , CR0ACK("CR0ACK", "smmuv3.CR0ACK", 0x24, 1)
        , CR1("CR1", "smmuv3.CR1", 0x28, 1)
        , CR1_TABLE_SH(CR1, "smmuv3.CR1.TABLE_SH", 10, 2)
        , CR1_TABLE_OC(CR1, "smmuv3.CR1.TABLE_OC", 8, 2)
        , CR1_TABLE_IC(CR1, "smmuv3.CR1.TABLE_IC", 6, 2)
        , CR1_QUEUE_SH(CR1, "smmuv3.CR1.QUEUE_SH", 4, 2)
        , CR1_QUEUE_OC(CR1, "smmuv3.CR1.QUEUE_OC", 2, 2)
        , CR1_QUEUE_IC(CR1, "smmuv3.CR1.QUEUE_IC", 0, 2)
        , CR2("CR2", "smmuv3.CR2", 0x2C, 1)
        , CR2_PTM(CR2, "smmuv3.CR2.PTM", 2, 1)
        , CR2_RECINVSID(CR2, "smmuv3.CR2.RECINVSID", 1, 1)
        , CR2_E2H(CR2, "smmuv3.CR2.E2H", 0, 1)
        , STATUSR("STATUSR", "smmuv3.STATUSR", 0x40, 1)
        , GBPA("GBPA", "smmuv3.GBPA", 0x44, 1)
        , GBPA_UPDATE(GBPA, "smmuv3.GBPA.UPDATE", 31, 1)
        , GBPA_ABORT(GBPA, "smmuv3.GBPA.ABORT", 20, 1)
        , AGBPA("AGBPA", "smmuv3.AGBPA", 0x48, 1)
        , IRQ_CTRL("IRQ_CTRL", "smmuv3.IRQ_CTRL", 0x50, 1)
        , IRQ_CTRL_GERROR_IRQEN(IRQ_CTRL, "smmuv3.IRQ_CTRL.GERROR_IRQEN", 0, 1)
        , IRQ_CTRL_PRI_IRQEN(IRQ_CTRL, "smmuv3.IRQ_CTRL.PRI_IRQEN", 1, 1)
        , IRQ_CTRL_EVENTQ_IRQEN(IRQ_CTRL, "smmuv3.IRQ_CTRL.EVENTQ_IRQEN", 2, 1)
        , IRQ_CTRL_ACK("IRQ_CTRL_ACK", "smmuv3.IRQ_CTRL_ACK", 0x54, 1)
        , GERROR("GERROR", "smmuv3.GERROR", 0x60, 1)
        , GERROR_CMDQ_ERR(GERROR, "smmuv3.GERROR.CMDQ_ERR", 0, 1)
        , GERROR_EVENTQ_ABT_ERR(GERROR, "smmuv3.GERROR.EVENTQ_ABT_ERR", 2, 1)
        , GERROR_PRIQ_ABT_ERR(GERROR, "smmuv3.GERROR.PRIQ_ABT_ERR", 3, 1)
        , GERROR_MSI_CMDQ_ABT_ERR(GERROR, "smmuv3.GERROR.MSI_CMDQ_ABT_ERR", 4, 1)
        , GERROR_MSI_EVENTQ_ABT_ERR(GERROR, "smmuv3.GERROR.MSI_EVENTQ_ABT_ERR", 5, 1)
        , GERROR_MSI_PRIQ_ABT_ERR(GERROR, "smmuv3.GERROR.MSI_PRIQ_ABT_ERR", 6, 1)
        , GERROR_MSI_GERROR_ABT_ERR(GERROR, "smmuv3.GERROR.MSI_GERROR_ABT_ERR", 7, 1)
        , GERROR_SFM_ERR(GERROR, "smmuv3.GERROR.SFM_ERR", 8, 1)
        , GERRORN("GERRORN", "smmuv3.GERRORN", 0x64, 1)
        , GERROR_IRQ_CFG0_LO("GERROR_IRQ_CFG0_LO", "smmuv3.GERROR_IRQ_CFG0_LO", 0x68, 1)
        , GERROR_IRQ_CFG0_HI("GERROR_IRQ_CFG0_HI", "smmuv3.GERROR_IRQ_CFG0_HI", 0x6C, 1)
        , GERROR_IRQ_CFG1("GERROR_IRQ_CFG1", "smmuv3.GERROR_IRQ_CFG1", 0x70, 1)
        , GERROR_IRQ_CFG2("GERROR_IRQ_CFG2", "smmuv3.GERROR_IRQ_CFG2", 0x74, 1)
        , STRTAB_BASE_LO("STRTAB_BASE_LO", "smmuv3.STRTAB_BASE_LO", 0x80, 1)
        , STRTAB_BASE_HI("STRTAB_BASE_HI", "smmuv3.STRTAB_BASE_HI", 0x84, 1)
        , STRTAB_BASE_CFG("STRTAB_BASE_CFG", "smmuv3.STRTAB_BASE_CFG", 0x88, 1)
        , STRTAB_BASE_CFG_FMT(STRTAB_BASE_CFG, "smmuv3.STRTAB_BASE_CFG.FMT", 16, 2)
        , STRTAB_BASE_CFG_SPLIT(STRTAB_BASE_CFG, "smmuv3.STRTAB_BASE_CFG.SPLIT", 6, 5)
        , STRTAB_BASE_CFG_LOG2SIZE(STRTAB_BASE_CFG, "smmuv3.STRTAB_BASE_CFG.LOG2SIZE", 0, 6)
        , CMDQ_BASE_LO("CMDQ_BASE_LO", "smmuv3.CMDQ_BASE_LO", 0x90, 1)
        , CMDQ_BASE_LOG2SIZE(CMDQ_BASE_LO, "smmuv3.CMDQ_BASE_LO.LOG2SIZE", 0, 5)
        , CMDQ_BASE_HI("CMDQ_BASE_HI", "smmuv3.CMDQ_BASE_HI", 0x94, 1)
        , CMDQ_PROD("CMDQ_PROD", "smmuv3.CMDQ_PROD", 0x98, 1)
        , CMDQ_PROD_WRP(CMDQ_PROD, "smmuv3.CMDQ_PROD.WRP", 19, 1)
        , CMDQ_PROD_RD(CMDQ_PROD, "smmuv3.CMDQ_PROD.RD", 0, 19)
        , CMDQ_CONS("CMDQ_CONS", "smmuv3.CMDQ_CONS", 0x9C, 1)
        , CMDQ_CONS_WRP(CMDQ_CONS, "smmuv3.CMDQ_CONS.WRP", 19, 1)
        , CMDQ_CONS_RD(CMDQ_CONS, "smmuv3.CMDQ_CONS.RD", 0, 19)
        , CMDQ_CONS_ERR(CMDQ_CONS, "smmuv3.CMDQ_CONS.ERR", 23, 7)
        , EVENTQ_BASE_LO("EVENTQ_BASE_LO", "smmuv3.EVENTQ_BASE_LO", 0xA0, 1)
        , EVENTQ_BASE_LOG2SIZE(EVENTQ_BASE_LO, "smmuv3.EVENTQ_BASE_LO.LOG2SIZE", 0, 5)
        , EVENTQ_BASE_HI("EVENTQ_BASE_HI", "smmuv3.EVENTQ_BASE_HI", 0xA4, 1)
        , EVENTQ_PROD("EVENTQ_PROD", "smmuv3.EVENTQ_PROD", 0xA8, 1)
        , EVENTQ_PROD_WRP(EVENTQ_PROD, "smmuv3.EVENTQ_PROD.WRP", 19, 1)
        , EVENTQ_PROD_RD(EVENTQ_PROD, "smmuv3.EVENTQ_PROD.RD", 0, 19)
        , EVENTQ_CONS("EVENTQ_CONS", "smmuv3.EVENTQ_CONS", 0xAC, 1)
        , EVENTQ_CONS_WRP(EVENTQ_CONS, "smmuv3.EVENTQ_CONS.WRP", 19, 1)
        , EVENTQ_CONS_RD(EVENTQ_CONS, "smmuv3.EVENTQ_CONS.RD", 0, 19)
        , EVENTQ_IRQ_CFG0_LO("EVENTQ_IRQ_CFG0_LO", "smmuv3.EVENTQ_IRQ_CFG0_LO", 0xB0, 1)
        , EVENTQ_IRQ_CFG0_HI("EVENTQ_IRQ_CFG0_HI", "smmuv3.EVENTQ_IRQ_CFG0_HI", 0xB4, 1)
        , EVENTQ_IRQ_CFG1("EVENTQ_IRQ_CFG1", "smmuv3.EVENTQ_IRQ_CFG1", 0xB8, 1)
        , EVENTQ_IRQ_CFG2("EVENTQ_IRQ_CFG2", "smmuv3.EVENTQ_IRQ_CFG2", 0xBC, 1)
        , PRIQ_BASE_LO("PRIQ_BASE_LO", "smmuv3.PRIQ_BASE_LO", 0xC0, 1)
        , PRIQ_BASE_LOG2SIZE(PRIQ_BASE_LO, "smmuv3.PRIQ_BASE_LO.LOG2SIZE", 0, 5)
        , PRIQ_BASE_HI("PRIQ_BASE_HI", "smmuv3.PRIQ_BASE_HI", 0xC4, 1)
        , PRIQ_PROD("PRIQ_PROD", "smmuv3.PRIQ_PROD", 0xC8, 1)
        , PRIQ_PROD_WRP(PRIQ_PROD, "smmuv3.PRIQ_PROD.WRP", 19, 1)
        , PRIQ_PROD_RD(PRIQ_PROD, "smmuv3.PRIQ_PROD.RD", 0, 19)
        , PRIQ_CONS("PRIQ_CONS", "smmuv3.PRIQ_CONS", 0xCC, 1)
        , PRIQ_CONS_WRP(PRIQ_CONS, "smmuv3.PRIQ_CONS.WRP", 19, 1)
        , PRIQ_CONS_RD(PRIQ_CONS, "smmuv3.PRIQ_CONS.RD", 0, 19)
        , PRIQ_IRQ_CFG0_LO("PRIQ_IRQ_CFG0_LO", "smmuv3.PRIQ_IRQ_CFG0_LO", 0xD0, 1)
        , PRIQ_IRQ_CFG0_HI("PRIQ_IRQ_CFG0_HI", "smmuv3.PRIQ_IRQ_CFG0_HI", 0xD4, 1)
        , PRIQ_IRQ_CFG1("PRIQ_IRQ_CFG1", "smmuv3.PRIQ_IRQ_CFG1", 0xD8, 1)
        , PRIQ_IRQ_CFG2("PRIQ_IRQ_CFG2", "smmuv3.PRIQ_IRQ_CFG2", 0xDC, 1)
        , GATOS_CTRL("GATOS_CTRL", "smmuv3.GATOS_CTRL", 0x100, 1)
        , GATOS_CTRL_RUN(GATOS_CTRL, "smmuv3.GATOS_CTRL.RUN", 0, 1)
        , GATOS_CTRL_INPRG(GATOS_CTRL, "smmuv3.GATOS_CTRL.INPRG", 1, 1)
        , GATOS_CTRL_READ_NWRITE(GATOS_CTRL, "smmuv3.GATOS_CTRL.READ_NWRITE", 8, 1)
        , GATOS_SID("GATOS_SID", "smmuv3.GATOS_SID", 0x108, 1)
        , GATOS_ADDR_LO("GATOS_ADDR_LO", "smmuv3.GATOS_ADDR_LO", 0x110, 1)
        , GATOS_ADDR_HI("GATOS_ADDR_HI", "smmuv3.GATOS_ADDR_HI", 0x114, 1)
        , GATOS_PAR_LO("GATOS_PAR_LO", "smmuv3.GATOS_PAR_LO", 0x118, 1)
        , GATOS_PAR_F(GATOS_PAR_LO, "smmuv3.GATOS_PAR_LO.F", 0, 1)
        , GATOS_PAR_FST(GATOS_PAR_LO, "smmuv3.GATOS_PAR_LO.FST", 1, 7)
        , GATOS_PAR_HI("GATOS_PAR_HI", "smmuv3.GATOS_PAR_HI", 0x11C, 1)
        , IDREGS(12)
        // Page 1 aliases (offsets are Page 0 offset + 0x10000)
        , CMDQ_PROD_P1("CMDQ_PROD_P1", "smmuv3.CMDQ_PROD_P1", 0x10098, 1)
        , CMDQ_CONS_P1("CMDQ_CONS_P1", "smmuv3.CMDQ_CONS_P1", 0x1009C, 1)
        , EVENTQ_PROD_P1("EVENTQ_PROD_P1", "smmuv3.EVENTQ_PROD_P1", 0x100A8, 1)
        , EVENTQ_CONS_P1("EVENTQ_CONS_P1", "smmuv3.EVENTQ_CONS_P1", 0x100AC, 1)
        , PRIQ_PROD_P1("PRIQ_PROD_P1", "smmuv3.PRIQ_PROD_P1", 0x100C8, 1)
        , PRIQ_CONS_P1("PRIQ_CONS_P1", "smmuv3.PRIQ_CONS_P1", 0x100CC, 1)
    {
        // Allocate PrimeCell ID registers
        for (uint32_t i = 0; i < 12; i++) {
            IDREGS[i] = std::make_shared<gs_register<uint32_t>>("IDREG" + std::to_string(i),
                                                                "smmuv3.IDREG" + std::to_string(i), 0xFD0 + (i * 4), 1);
        }
    }

    void bind_regs(gs::json_module& jm)
    {
        jm.bind_reg(IDR0);
        jm.bind_reg(IDR1);
        jm.bind_reg(IDR2);
        jm.bind_reg(IDR3);
        jm.bind_reg(IDR4);
        jm.bind_reg(IDR5);
        jm.bind_reg(IIDR);
        jm.bind_reg(AIDR);
        jm.bind_reg(CR0);
        jm.bind_reg(CR0ACK);
        jm.bind_reg(CR1);
        jm.bind_reg(CR2);
        jm.bind_reg(STATUSR);
        jm.bind_reg(GBPA);
        jm.bind_reg(AGBPA);
        jm.bind_reg(IRQ_CTRL);
        jm.bind_reg(IRQ_CTRL_ACK);
        jm.bind_reg(GERROR);
        jm.bind_reg(GERRORN);
        jm.bind_reg(GERROR_IRQ_CFG0_LO);
        jm.bind_reg(GERROR_IRQ_CFG0_HI);
        jm.bind_reg(GERROR_IRQ_CFG1);
        jm.bind_reg(GERROR_IRQ_CFG2);
        jm.bind_reg(STRTAB_BASE_LO);
        jm.bind_reg(STRTAB_BASE_HI);
        jm.bind_reg(STRTAB_BASE_CFG);
        jm.bind_reg(CMDQ_BASE_LO);
        jm.bind_reg(CMDQ_BASE_HI);
        jm.bind_reg(CMDQ_PROD);
        jm.bind_reg(CMDQ_CONS);
        jm.bind_reg(EVENTQ_BASE_LO);
        jm.bind_reg(EVENTQ_BASE_HI);
        jm.bind_reg(EVENTQ_PROD);
        jm.bind_reg(EVENTQ_CONS);
        jm.bind_reg(EVENTQ_IRQ_CFG0_LO);
        jm.bind_reg(EVENTQ_IRQ_CFG0_HI);
        jm.bind_reg(EVENTQ_IRQ_CFG1);
        jm.bind_reg(EVENTQ_IRQ_CFG2);
        jm.bind_reg(PRIQ_BASE_LO);
        jm.bind_reg(PRIQ_BASE_HI);
        jm.bind_reg(PRIQ_PROD);
        jm.bind_reg(PRIQ_CONS);
        jm.bind_reg(PRIQ_IRQ_CFG0_LO);
        jm.bind_reg(PRIQ_IRQ_CFG0_HI);
        jm.bind_reg(PRIQ_IRQ_CFG1);
        jm.bind_reg(PRIQ_IRQ_CFG2);
        jm.bind_reg(GATOS_CTRL);
        jm.bind_reg(GATOS_SID);
        jm.bind_reg(GATOS_ADDR_LO);
        jm.bind_reg(GATOS_ADDR_HI);
        jm.bind_reg(GATOS_PAR_LO);
        jm.bind_reg(GATOS_PAR_HI);

        for (auto& reg : IDREGS) {
            jm.bind_reg(*reg);
        }

        // Page 1 aliases for queue PROD/CONS registers
        jm.bind_reg(CMDQ_PROD_P1);
        jm.bind_reg(CMDQ_CONS_P1);
        jm.bind_reg(EVENTQ_PROD_P1);
        jm.bind_reg(EVENTQ_CONS_P1);
        jm.bind_reg(PRIQ_PROD_P1);
        jm.bind_reg(PRIQ_CONS_P1);

        jm.log_end_of_binding_msg("smmuv3_gen");
    }
};

} // namespace gs

#endif // _SMMUV3_GEN_H_
