# SMMU-500 Model Specification Compliance Report

Reference: ARM DDI 0517 (CoreLink MMU-500 TRM), ARM IHI 0062 (SMMUv2 Architecture Specification)

## 1. Summary

This model implements a functional subset of the ARM MMU-500 sufficient for
AArch64 LPAE page table walks (stage 1, stage 2, and nested stage 1+2), stream
matching, TLB invalidation via DMI, and global address translation services.
Only registers with associated behavioral logic are explicitly declared in the
model; all other registers in the SMMU address space (identification, debug,
performance monitors, etc.) are accessible as memory via the reg_model_maker
ZIP configuration.

---

## 2. Features Covered

### 2.1 Register Map

| Area | Status | Notes |
|------|--------|-------|
| Global config (SCR0, SCR1, SACR) | Functional | SCR0.CLIENTPD used for bypass; NSCR0 syncs to SCR0 |
| Identification (IDR0-2, IDR7) | Functional | IDR0.NUMSMRG, IDR0.ATOSNS, IDR1.NUMCB, IDR1.NUMPAGENDXB, IDR7 set from CCI params at start_of_simulation |
| Stream Match (SMR[n]) | Functional | VALID/MASK/ID fields used by smmu500_stream_id_match() |
| Stream-to-Context (S2CR[n]) | Functional | CBNDX (bits [7:0]) extracted; TYPE field not checked (bypass/fault modes ignored) |
| Context Bank Attribute (CBAR[n]) | Functional | TYPE (bits [17:16]) drives stage selection; CBNDX for S2 CB (bits [15:8]) |
| Context Bank Attribute 2 (CBA2R[n]) | Functional | VA64 (bit 0) checked during page table walk |
| Global fault status (SGFSR) | Stored | Register exists; not written by current fault paths |
| GATS (GATS1PR/PW, GATS12PR/PW, GPAR) | Functional | Post-write on the _H register triggers translation; result in GPAR |
| Per-CB TLB invalidation (TLBIALL, TLBIASID, TLBSYNC, TLBSTATUS) | Functional | TLBIALL and TLBIASID trigger DMI invalidation across all TBUs |
| TBU_PWR_STATUS | Stored | Populated from p_num_tbu |

All other registers in the SMMU address space (IDR3-6, PID/CID, global fault
syndrome, non-secure register copies, global TLB invalidation, performance
monitors, integration/test, and vendor-specific registers) are accessible as
memory via the reg_model_maker ZIP configuration but have no behavioral logic.

### 2.2 Context Bank Registers (per CB page)

| Register | Status | Notes |
|----------|--------|-------|
| CB_SCTLR | Functional | Bit 0 (M) enables translation; bit 6 (CFIE) used for IRQ |
| CB_ACTLR | Stored | |
| CB_RESUME | Stored | No stall/resume logic |
| CB_TCR2 | Functional | Upper 32 bits of 64-bit TCR for stage 1 |
| CB_TTBR0 (LOW/HIGH) | Functional | Base address for page table walk |
| CB_TTBR1 (LOW/HIGH) | Functional | Used for VA[63]=1 (upper address range) |
| CB_TCR_LPAE | Functional | T0SZ, T1SZ, TG0, TG1, EPD, SL0, PS fields all used |
| CB_CONTEXTIDR | Stored | |
| CB_PRRR_MAIR0 | Stored | Memory attribute registers present but not used in translation |
| CB_NMRR_MAIR1 | Stored | |
| CB_FSR | Functional | TF bit set on fault; drives context IRQ; post_write callback |
| CB_FSRRESTORE | Stored | Exists at correct offset |
| CB_FAR (LOW/HIGH) | Functional | Written on stage 2 faults |
| CB_FSYNR0 | Stored | Register present but not written by fault logic |
| CB_IPAFAR (LOW/HIGH) | Functional | Written with faulting VA/IPA on faults |
| CB_TLBIASID | Functional | Post-write triggers DMI invalidation by value |
| CB_TLBIALL | Functional | Post-write triggers DMI invalidation for the CB |
| CB_TLBSYNC | Stored | No sync logic (invalidation is synchronous) |
| CB_TLBSTATUS | Stored | Always reads 0 (no pending operations) |

### 2.3 Translation Logic

| Feature | Status | Notes |
|---------|--------|-------|
| AArch64 Stage 1 LPAE walk | Implemented | 4KB, 16KB, 64KB granule sizes supported |
| AArch64 Stage 2 walk | Implemented | SL0 starting level, separate PS field extraction |
| Nested S1+S2 translation | Implemented | S1 descriptor addresses translated through S2 |
| TTBR0/TTBR1 split (VA[63]) | Implemented | Upper/lower VA range with separate TG, TSZ |
| EPD (Translation Disable) | Implemented | EPD0 checked; faults if set |
| Block translations | Implemented | Block entries at correct levels per granule |
| Table attribute accumulation | Implemented | Bits [63:59] of table descriptors accumulated |
| Output address size check | Implemented | Addresses checked against PAMAX/output size |
| Access permission (AP) | Implemented | S1: AP[2] read-only check; S2: HAP S2AP checking |
| Access flag (AF) check | Implemented | Bit 10 of descriptor checked; fault if not set |
| DMA-based descriptor reads | Implemented | Page table descriptors read via dma_socket b_transport |
| CBAR.TYPE stage selection | Implemented | All four types (S2-only, S1+fault, S1-only, S1+S2) |

### 2.4 TBU Architecture

| Feature | Status | Notes |
|---------|--------|-------|
| Multiple TBUs per TCU | Implemented | Vector of TBU pointers; configurable via p_num_tbu |
| TBU b_transport forwarding | Implemented | Translates address, forwards to downstream_socket |
| TBU transport_dbg | Implemented | Debug transport with address translation |
| TBU DMI (get_direct_mem_ptr) | Implemented | Full DMI support with page-aligned virtual-to-physical mapping |
| DMI invalidation on TLBI | Implemented | TLBIALL/TLBIASID trigger upstream DMI invalidation |
| Per-CB DMI range tracking | Implemented | dmi_range[] tracks valid ranges per context bank |
| Thread-safe DMI locking | Implemented | Mutex-based locking; THREAD_SAFE_REENTRANT compile-time option |
| Topology ID (stream ID) | Implemented | CCI param per TBU; used for stream matching |

### 2.5 Interrupts

| Feature | Status | Notes |
|---------|--------|-------|
| Per-CB context fault IRQ | Implemented | irq_context[cb] = FSR.TF && SCTLR.CFIE |
| Global fault IRQ | Port exists | irq_global port declared but never asserted by logic |

### 2.6 Reset

| Feature | Status | Notes |
|---------|--------|-------|
| Register reset via signal | Implemented | reset signal triggers M.reset() which restores all register defaults |

---

## 3. Features Not Covered

### 3.1 Global Fault Reporting

Global fault logic is **not implemented**. The spec requires:

- **Unidentified Stream Fault (USF)**: When no SMR matches, the model returns
  `cbndx = -1` and bypasses (addr_mask = -1) rather than generating a global
  fault based on SCR0.USFCFG.
- **Stream Match Conflict Fault (SMCF)**: Multiple SMR matches are not detected.
- **Invalid Context Fault (ICF)**: Not generated.
- **Global fault IRQ** (irq_global): Never asserted.

### 3.2 S2CR TYPE Field

The S2CR.TYPE field (bits [17:16]) is **not checked**. The model always extracts
CBNDX and performs translation. Per spec:

- TYPE=0b01 (Bypass): Should pass transactions without translation
- TYPE=0b10 (Fault): Should generate a global fault for any transaction
- Only TYPE=0b00 (Translation) should use CBNDX

### 3.3 Write-1-to-Clear (W1C) Semantics

CB_FSR and SGFSR are specified as W1C registers. The model **does not implement
W1C behavior** — writes store the value directly rather than clearing bits
where 1s are written. The FSRRESTORE companion registers exist but have no
special relationship to FSR.

### 3.4 Read-Only / Write-Only Register Enforcement

Read-only and write-only register access semantics are not enforced. Software
can read write-only registers and overwrite read-only registers.

### 3.5 Context Fault Syndrome Details

On translation faults, the model:
- Sets CB_FSR.TF (bit 1) — correct
- Writes CB_IPAFAR — correct
- Writes CB_FAR only for stage 2 — correct
- Does **not** write CB_FSYNR0 (fault syndrome: WnR, PNU, IND, etc.)
- Does **not** write CB_FSYNR1
- Does **not** set CB_FSR.MULTI for subsequent faults while TF is set
- Does **not** set other FSR fault type bits (AFF, PF, EF)

### 3.6 Security State Differentiation

The model treats all accesses as non-secure. It does not:
- Differentiate secure vs non-secure bus transactions
- Enforce SCR1 partitioning of CBs/SMRs between secure and non-secure worlds
- Bank register access based on security state (secure accesses always go to
  secure copies, non-secure to NS copies)

SCR1.NSNUMCBO and NSNUMSMRGO are populated but not used for access control.

### 3.7 VMID Support

- VMID matching in S2CR is not implemented
- VMID-based TLB invalidation is not implemented
- 16-bit VMID (SCR0.VMID16EN, CBA2R.VMID) is not used

### 3.8 Per-CB TLB Invalidation by VA

- CB_TLBIVA, CB_TLBIVAL (invalidate by VA / by VA last level): **Not
  implemented** — registers are not instantiated
- CB_TLBIIPAS2, CB_TLBIIPAS2L (invalidate by IPA): **Not implemented**

### 3.9 Global TLB Invalidation

Global TLB invalidation operations (STLBIALL, TLBIVMID, TLBIALLNSNH,
TLBIALLH, etc.) are **not implemented**. Only per-CB TLBIALL and TLBIASID
trigger actual invalidation.

### 3.10 Transaction Stalling

- CB_SCTLR.CFCFG (stall vs terminate on fault) is not checked
- CB_RESUME has no logic — stalled transactions cannot be resumed
- CB_FSR.SS (stalled status) is never set
- SCR0.STALLD / GSE are not used

### 3.11 Missing GATS Variants

Only privileged GATS operations are implemented:
- GATS1PR, GATS1PW, GATS12PR, GATS12PW — **functional**
- GATS1UR, GATS1UW, GATS12UR, GATS12UW (unprivileged) — **not implemented**

### 3.12 AArch32 Short-Descriptor Format

Only the AArch64 LPAE (Long Descriptor) page table format is implemented.
The AArch32 Short-Descriptor format (used when CBA2R.VA64=0 and the CB is
configured for 32-bit virtual addresses) is **not supported**.

### 3.13 Memory Attributes

- MAIR0/MAIR1 (PRRR/NMRR) are stored but not used during translation
- Descriptor memory attributes are not propagated to the translated transaction
- SCR0 shareability/cacheability overrides (SHCFG, RACFG, WACFG, MEMATTR,
  MTCFG) are not applied
- CB_SCTLR cacheability/shareability fields are not used

---

## 4. Implementation Differences from Spec

### 4.1 Stream ID Construction

The model constructs the stream ID as:
```
master_id = topology_id | ((addr >> 32) & 0xf)
```
This encodes 4 bits from the upper address into the stream ID. This is an
**implementation-specific convention** not defined by the ARM spec, which treats
the StreamID as an opaque value provided by the interconnect.

### 4.2 SMMU Bypass Behavior

When SCR0.CLIENTPD=1 or no SMR match is found:
- The model returns `addr_mask = -1` (full bypass, identity mapping)
- The spec behavior for unmatched streams depends on SCR0.USFCFG: fault
  (USFCFG=1) or bypass (USFCFG=0). The model always bypasses.

### 4.3 Fault Handling

Faults only set FSR.TF (Translation Fault, bit 1). The spec defines distinct
fault types:
- Translation Fault (TF, bit 1)
- Access Flag Fault (AFF, bit 2)
- Permission Fault (PF, bit 3)
- External Fault (EF, bit 4)

The model reports all fault conditions as TF regardless of the actual cause
(e.g., permission failures are also reported as TF).

### 4.4 NSCR0-to-SCR0 Sync

Writing NSCR0 copies the full value to SCR0. Per spec, NSCR0 and SCR0 share
some fields but others are secure-only and should not be modifiable via NSCR0.
The model does a wholesale copy without masking.

### 4.5 No TLB Caching in TCU

The model performs a full page table walk on every translation. There is no TLB
cache in the TCU (Translation Control Unit). Only the TBU-side DMI mechanism
provides translation caching. This is functionally correct but differs from
hardware which caches translations in both TBU micro-TLBs and TCU main TLB.

### 4.6 GATS Input Format

The model encodes the GATS input as:
```
va = (value & ~0xFFF) & ((1ULL << 32) - 1)
cb = value & 0xFFF
```
This uses the lower 12 bits for the context bank index and the upper bits for
the VA. The spec uses a different encoding where the CB index and VA are
packed differently depending on the register width.

### 4.7 Synchronous TLB Invalidation

TLBSYNC/TLBSTATUS always indicate completion (TLBSTATUS reads 0) because
invalidation is performed synchronously in the post_write callback. Hardware
would have asynchronous invalidation where TLBSTATUS.SACTIVE indicates pending
operations.

---

## 5. Configurable Parameters

| Parameter | Default | Description |
|-----------|---------|-------------|
| p_pamax | 48 | Physical address width (bits) |
| p_num_smr | 48 | Number of Stream Match Register groups |
| p_num_cb | 16 | Number of context banks |
| p_num_pages | 16 | Number of global register pages |
| p_ato | true | Address Translation Operations supported |
| p_version | 0x21 | IDR7 version (major.minor = 2.1) |
| p_num_tbu | 1 | Number of Translation Buffer Units |
