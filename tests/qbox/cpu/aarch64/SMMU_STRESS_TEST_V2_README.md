# SMMU Stress Test V2 - Comprehensive SMMU Translation Validation

## Test Goals

### Primary Objectives
1. **Validate ARM SMMU-500 Translation Pipeline**: Ensure complete end-to-end translation functionality from virtual addresses to physical addresses through multi-level page tables
2. **Stress Test Multi-CPU SMMU Operations**: Verify concurrent SMMU translation requests from multiple CPUs without conflicts or corruption
3. **Validate Page Table Walking (PTW)**: Test L1/L2 page table traversal with proper descriptor formats, access permissions, and memory attributes
4. **Verify StreamID to Context Bank Mapping**: Ensure correct routing of different CPU streams to appropriate translation contexts
5. **Test Dynamic SMMU Reconfiguration**: Validate ability to dynamically map/unmap virtual address ranges to different physical regions
6. **Validate Memory Coherency**: Ensure SMMU translations maintain memory coherency across multiple CPUs and memory regions

### Secondary Objectives
1. **Performance Validation**: Measure SMMU translation throughput under concurrent multi-CPU load
2. **Error Detection**: Verify proper handling of translation faults and invalid page descriptors
3. **Memory Layout Flexibility**: Test SMMU functionality with complex memory layouts and address mappings
4. **System Integration**: Validate SMMU integration with SystemC/TLM infrastructure and routing components

## Architecture Overview

### High-Level Architecture
```
┌─────────────────────────────────────────────────────────────────┐
│                    SMMU Stress Test V2                         │
│                 Tester-Controlled Architecture                  │
└─────────────────────────────────────────────────────────────────┘

┌─────────────┐    ┌─────────────┐    ┌─────────────┐
│   CPU[0]    │    │   CPU[1]    │    │   CPU[n]    │
│ (StreamID=0)│    │ (StreamID=1)│    │ (StreamID=n)│
└──────┬──────┘    └──────┬──────┘    └──────┬──────┘
       │                  │                  │
       ▼                  ▼                  ▼
┌─────────────┐    ┌─────────────┐    ┌─────────────┐
│   TBU[0]    │    │   TBU[1]    │    │   TBU[n]    │
│Translation  │    │Translation  │    │Translation  │
│Buffer Unit  │    │Buffer Unit  │    │Buffer Unit  │
└──────┬──────┘    └──────┬──────┘    └──────┬──────┘
       │                  │                  │
       └──────────────────┼──────────────────┘
                          ▼
              ┌─────────────────────┐
              │   Global Router     │
              │  (Address Routing)  │
              └─────────┬───────────┘
                        │
        ┌───────────────┼───────────────┐
        ▼               ▼               ▼
┌─────────────┐ ┌─────────────┐ ┌─────────────┐
│   Memory    │ │   SMMU-500  │ │   Tester    │
│  (256MB)    │ │ (Registers) │ │ Controller  │
│             │ │             │ │   (MMIO)    │
└─────────────┘ └─────────────┘ └─────────────┘
```

### SMMU Internal Architecture
```
┌─────────────────────────────────────────────────────────────────┐
│                      SMMU-500 Internal                         │
└─────────────────────────────────────────────────────────────────┘

StreamID Mapping:
┌─────────────┐    ┌─────────────┐    ┌─────────────┐
│ StreamID=0  │    │ StreamID=1  │    │ StreamID=17 │
│   (CPU 0)   │    │   (CPU 1)   │    │(CPU 1 HighVA)│
└──────┬──────┘    └──────┬──────┘    └──────┬──────┘
       │                  │                  │
       ▼                  ▼                  ▼
┌─────────────┐    ┌─────────────┐    ┌─────────────┐
│   SMR[0]    │    │   SMR[1]    │    │   SMR[2]    │
│Stream Match │    │Stream Match │    │Stream Match │
│  Register   │    │  Register   │    │  Register   │
└──────┬──────┘    └──────┬──────┘    └──────┬──────┘
       │                  │                  │
       ▼                  ▼                  ▼
┌─────────────┐    ┌─────────────┐    ┌─────────────┐
│   S2CR[0]   │    │   S2CR[1]   │    │   S2CR[2]   │
│Stream-to-   │    │Stream-to-   │    │Stream-to-   │
│Context Reg  │    │Context Reg  │    │Context Reg  │
└──────┬──────┘    └──────┬──────┘    └──────┬──────┘
       │                  │                  │
       ▼                  ▼                  ▼
┌─────────────┐    ┌─────────────┐    ┌─────────────┐
│Context Bank │    │Context Bank │    │Context Bank │
│     CB0     │    │     CB1     │    │     CB1     │
│  (CPU 0)    │    │  (CPU 1)    │    │  (CPU 1)    │
└─────────────┘    └─────────────┘    └─────────────┘
```

### Memory Layout Architecture
```
┌─────────────────────────────────────────────────────────────────┐
│                    Physical Memory Layout                       │
│              (Reorganized for 1:1 Identity Mapping)            │
└─────────────────────────────────────────────────────────────────┘

Physical Address Space:
0x00000000 ┌─────────────────────────────────────────┐
           │           FIRMWARE (256KB)              │
           │                                         │
           │  0x00000000: Boot Loader                │
           │  0x00000200: Diagnostic Handler         │
           │  0x00001000: Main Test Firmware         │
           │              ...                        │
0x0003FFFF └─────────────────────────────────────────┘
0x00040000 ┌─────────────────────────────────────────┐
           │        TESTER CONTROLLER (64KB)         │
           │         (MMIO Registers)                │
           │                                         │
           │  Per-CPU Register Layout (0x100 each):  │
           │  0x00: REG_REQUEST   (W)                │
           │  0x08: REG_STATUS    (R)                │
           │  0x10: REG_REGION_ID (R)                │
           │  0x18: REG_COMPLETE  (W)                │
           │  0x20: REG_ITERATIONS(R)                │
           │  0x28: REG_DEBUG     (W)                │
0x0004FFFF └─────────────────────────────────────────┘
0x00050000 ┌─────────────────────────────────────────┐
           │        SMMU REGISTERS (128KB)           │
           │      (Configuration Space)              │
           │                                         │
           │  0x50000: Global Registers              │
           │  0x50800: Stream Match Registers (SMR)  │
           │  0x50C00: Stream-to-Context Regs (S2CR) │
           │  0x51000: Context Bank Config (CBAR)    │
           │  0x54000: Context Bank 0 Registers      │
           │  0x55000: Context Bank 1 Registers      │
           │           ...                           │
0x0006FFFF └─────────────────────────────────────────┘
           │              GAP                        │
0x10000000 ┌─────────────────────────────────────────┐
           │         MAIN MEMORY (256MB)             │
           │                                         │
           │  0x10000000: Page Tables (64KB total)  │
           │    - CPU 0: 0x10000000-0x10003FFF      │
           │      * L0 Table: 0x10000000 (4KB)      │
           │      * L1 Table: 0x10001000 (4KB)      │
           │      * L2 Table: 0x10002000 (4KB)      │
           │      * L3 Table: 0x10003000 (4KB)      │
           │    - CPU 1: 0x10004000-0x10007FFF      │
           │      * L0 Table: 0x10004000 (4KB)      │
           │      * L1 Table: 0x10005000 (4KB)      │
           │      * L2 Table: 0x10006000 (4KB)      │
           │      * L3 Table: 0x10007000 (4KB)      │
           │    - CPU 2: 0x10008000-0x1000BFFF      │
           │      * L0 Table: 0x10008000 (4KB)      │
           │      * L1 Table: 0x10009000 (4KB)      │
           │      * L2 Table: 0x1000A000 (4KB)      │
           │      * L3 Table: 0x1000B000 (4KB)      │
           │    - (Up to 16 CPUs supported)         │
           │                                         │
           │  0x10010000-0x10FFFFFF: Reserved        │
           │                                         │
           │  0x11000000: Test Regions (4KB each)   │
           │    - Region 0: 0x11000000-0x11000FFF   │
           │    - Region 1: 0x11001000-0x11001FFF   │
           │    - Region 2: 0x11002000-0x11002FFF   │
           │              ...                        │
0x1FFFFFFF └─────────────────────────────────────────┘

Virtual Address Space:
0x00000000 ┌─────────────────────────────────────────┐
           │      IDENTITY MAPPING (1GB Block)       │
           │    VA=PA for all system addresses       │
0x3FFFFFFF └─────────────────────────────────────────┘
           │              ...                        │
0x300000000┌─────────────────────────────────────────┐
           │       VIRTUAL TEST ADDRESS              │
           │    (Maps to physical test regions)      │
           └─────────────────────────────────────────┘
```

### ARM LPAE Page Table Architecture (CORRECTED)
```
┌─────────────────────────────────────────────────────────────────┐
│                ARM LPAE Page Table Structure                   │
│              (3-Level: L0 → L1 → L2, T0SZ=16)                 │
└─────────────────────────────────────────────────────────────────┘

Per-CPU Page Tables (3 pages per CPU):
┌─────────────────────────────────────────┐
│              L0 Table (4KB)             │
│                                         │
│  L0[0]:  0x00000000-0x7FFFFFFFFF        │
│          → L1 Table                     │
│          (Table descriptor)             │
│                                         │
│  L0[1-511]: Empty                       │
└─────────────────────────────────────────┘
                    │
                    ▼
┌─────────────────────────────────────────┐
│              L1 Table (4KB)             │
│                                         │
│  L1[0]:  0x00000000-0x3FFFFFFF         │
│          → Identity 1GB Block           │
│          (Block descriptor 0x741)       │ ✅ CORRECTED LEVEL
│                                         │
│  L1[12]: 0x300000000-0x33FFFFFFF       │
│          → L2 Table                     │
│          (Table descriptor)             │
│                                         │
│  L1[1-11,13-511]: Empty                │
└─────────────────────────────────────────┘
                    │
                    ▼
┌─────────────────────────────────────────┐
│              L2 Table (4KB)             │
│                                         │
│  L2[0]:  0x300000000                    │
│          → Physical Test Region         │
│          (4KB page descriptor)          │
│                                         │
│  L2[1-511]: Empty                       │
└─────────────────────────────────────────┘

ARM LPAE Compliance:
• T0SZ=16 (48-bit virtual address space)
• Level 0: Table descriptors only (512GB per entry)
• Level 1: 1GB block descriptors OR table descriptors
• Level 2: 2MB block descriptors OR table descriptors  
• Level 3: 4KB page descriptors

Translation Examples:
• VA=0x00040000 (Tester) → PA=0x00040000 (Identity via L1 1GB block)
• VA=0x00050000 (SMMU)   → PA=0x00050000 (Identity via L1 1GB block)
• VA=0x10000000 (Memory) → PA=0x10000000 (Identity via L1 1GB block)
• VA=0x300000000 (Test)  → PA=0x10001000 (Translated via L0→L1→L2)
```

### Tester Controller Protocol Architecture
```
┌─────────────────────────────────────────────────────────────────┐
│                 Tester Controller Protocol                      │
│                  (MMIO-Based Coordination)                     │
└─────────────────────────────────────────────────────────────────┘

CPU-Tester Interaction Flow:
┌─────────────┐                           ┌─────────────┐
│   CPU[n]    │                           │   Tester    │
│             │                           │ Controller  │
└──────┬──────┘                           └──────┬──────┘
       │                                         │
       │ 1. Write REG_REQUEST (FILL/CHECK)      │
       ├────────────────────────────────────────►│
       │                                         │
       │                                         │ 2. Assign Region
       │                                         │    Configure SMMU
       │                                         │    Set Status=READY
       │                                         │
       │ 3. Poll REG_STATUS                      │
       ├────────────────────────────────────────►│
       │ ◄────────────────────────────────────────┤ Status=READY
       │                                         │
       │ 4. Read REG_REGION_ID                   │
       ├────────────────────────────────────────►│
       │ ◄────────────────────────────────────────┤ Region ID
       │                                         │
       │ 5. Perform Work at VIRTUAL_TEST_ADDR    │
       │    (Fill/Check via SMMU translation)    │
       │                                         │
       │ 6. Write REG_COMPLETE (DONE)            │
       ├────────────────────────────────────────►│
       │                                         │
       │                                         │ 7. Unmap SMMU
       │                                         │    Update Queues
       │                                         │    Increment Count

MMIO Register Layout (per CPU):
Offset  Register      Access  Description
0x00    REG_REQUEST   W       1=fill_request, 2=check_request
0x08    REG_STATUS    R       0=busy, 1=ready, 2=complete
0x10    REG_REGION_ID R       Assigned region ID
0x18    REG_COMPLETE  W       1=fill_done, 2=check_done
0x20    REG_ITERATIONS R      Current iteration count
0x28    REG_DEBUG     W       Debug messages
```

### Test Execution Flow
```
┌─────────────────────────────────────────────────────────────────┐
│                    Test Execution Flow                         │
└─────────────────────────────────────────────────────────────────┘

Initialization Phase:
1. Configure SMMU StreamID mappings (SMR/S2CR registers)
2. Initialize Context Banks with identity mapping (MMU disabled)
3. Set up tester controller with region queues
4. Start CPUs with test firmware

Main Test Loop (per CPU):
┌─────────────────────────────────────────┐
│              CPU Main Loop              │
└─────────────────────────────────────────┘
                    │
                    ▼
┌─────────────────────────────────────────┐
│         Request Fill Region             │
│    (Write REG_REQUEST = FILL)           │
└─────────────────┬───────────────────────┘
                  │
                  ▼
┌─────────────────────────────────────────┐
│         Poll for Readiness              │
│     (Read REG_STATUS until READY)       │
└─────────────────┬───────────────────────┘
                  │
                  ▼
┌─────────────────────────────────────────┐
│        Get Assigned Region              │
│       (Read REG_REGION_ID)              │
└─────────────────┬───────────────────────┘
                  │
                  ▼
┌─────────────────────────────────────────┐
│      Fill Region with Pattern           │
│  (Write to VIRTUAL_TEST_ADDR via SMMU)  │
└─────────────────┬───────────────────────┘
                  │
                  ▼
┌─────────────────────────────────────────┐
│        Signal Completion               │
│    (Write REG_COMPLETE = FILL_DONE)     │
└─────────────────┬───────────────────────┘
                  │
                  ▼
┌─────────────────────────────────────────┐
│        Request Check Region             │
│    (Write REG_REQUEST = CHECK)          │
└─────────────────┬───────────────────────┘
                  │
                  ▼
┌─────────────────────────────────────────┐
│         Poll for Readiness              │
│     (Read REG_STATUS until READY)       │
└─────────────────┬───────────────────────┘
                  │
                  ▼
┌─────────────────────────────────────────┐
│       Verify Region Pattern            │
│  (Read from VIRTUAL_TEST_ADDR via SMMU) │
└─────────────────┬───────────────────────┘
                  │
                  ▼
┌─────────────────────────────────────────┐
│        Signal Completion               │
│   (Write REG_COMPLETE = CHECK_DONE)     │
└─────────────────┬───────────────────────┘
                  │
                  ▼
┌─────────────────────────────────────────┐
│       Increment Iteration               │
│    (Continue until target reached)      │
└─────────────────────────────────────────┘
```

## Key Architectural Decisions

### 1. Tester-Controlled Design
**Rationale**: Eliminates complex synchronization protocols and provides centralized control over SMMU configuration, improving reliability and debuggability.

### 2. Simplified Memory Layout
**Rationale**: 1GB identity blocks reduce page table complexity while maintaining full translation testing capability through high virtual addresses.

### 3. MMIO-Based Coordination
**Rationale**: Direct register-based communication provides deterministic timing and clear protocol boundaries between CPUs and test controller.

### 4. Dynamic SMMU Reconfiguration
**Rationale**: Tests real-world usage patterns where SMMU mappings change dynamically based on workload requirements.

## Test Validation Criteria

### Success Criteria
1. **Translation Accuracy**: All virtual addresses correctly translate to expected physical addresses
2. **Multi-CPU Coordination**: No conflicts or corruption during concurrent operations
3. **Pattern Integrity**: All fill/check operations maintain data integrity through SMMU translation
4. **Performance Targets**: Achieve target iteration count within timeout period
5. **Error-Free Operation**: No translation faults or SMMU configuration errors

### Failure Indicators
1. **Translation Faults**: Page table walk failures or invalid descriptors
2. **Pattern Mismatches**: Data corruption during SMMU translation
3. **Coordination Failures**: CPU synchronization issues or deadlocks
4. **Timeout Expiration**: Insufficient performance or system hangs
5. **SMMU Configuration Errors**: Invalid register states or mapping failures

## Build and Execution

### Build
```bash
(cd build; gmake -j)
```

### Run Test
```bash
(cd build; gtimeout 10s ./tests/libqbox/cpu/aarch64/aarch64-smmu-router-stress-test-v2 -p test-bench.num_cpu=2 -p log_level=5 -p test-bench.main_mem.log_level=0)
```

### Parameters
- `test-bench.num_cpu`: Number of CPUs (1-N)
- `log_level`: Logging verbosity (5=debug)
- `test-bench.main_mem.log_level`: Memory logging level

This architecture provides comprehensive validation of SMMU functionality while maintaining simplicity and reliability through its tester-controlled design and optimized memory layout.
