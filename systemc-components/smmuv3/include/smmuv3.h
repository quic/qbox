/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * ARM SMMUv3 System MMU Model
 *
 * configuration example in conf.lua:
local smmu = {
    moduletype = "smmuv3",
    dma = {bind = "&router.target_socket"},
    target_socket = {address=0x15000000, size=0x20000, bind= "&router.initiator_socket"},
    irq_eventq = {bind = "&gic_0.spi_in_65"},  -- Event queue IRQ
    irq_gerror = {bind = "&gic_0.spi_in_97"},  -- Global error IRQ
    irq_cmd_sync = {bind = "&gic_0.spi_in_98"}, -- Command sync IRQ
    irq_priq = {bind = "&gic_0.spi_in_99"},    -- PRI queue IRQ
};

* For TBUS:
platform["tbu_0"] = {
        moduletype = "smmuv3_tbu",
        args = {"&platform.smmu_0"},
        topology_id=0x2000,
        upstream_socket = {bind = "&hex_plugin.nsp0_ss_to_tbu_initiator_socket"},
        downstream_socket = {bind = "&router.target_socket"};
      };

* device tree node:
iommu@15000000 {
    compatible = "arm,smmu-v3";
    reg = <0x00 0x15000000 0x00 0x20000>;
    interrupts = <0x00 0x41 0x04 0x00 0x61 0x04 0x00 0x62 0x04 0x00 0x63 0x04>;
    interrupt-names = "eventq", "gerror", "priq", "cmdq-sync";
    #iommu-cells = <0x01>;
    dma-coherent;
    phandle = <0x41>;
                };
 */

#ifndef SMMUV3_H
#define SMMUV3_H

#define INCBIN_SILENCE_BITCODE_WARNING
#include <reg_model_maker/incbin.h>

// Forward-declare INCBIN'd config zip symbols defined in smmuv3.cc.
// Required because the smmuv3 constructor (defined in this header) references
// these globals - external translation units (e.g. tests) including this
// header need the declaration to compile.
INCBIN_EXTERN(ZipArchive_smmuv3_);

#include <systemc>
#include <cci_configuration>
#include <cciutils.h>
#include <scp/report.h>
#include <tlm>
#include <module_factory_registery.h>
#include <ports/initiator-signal-socket.h>
#include <ports/target-signal-socket.h>
#include <tlm_utils/simple_initiator_socket.h>
#include <tlm_utils/simple_target_socket.h>
#include <tlm-extensions/underlying-dmi.h>
#include <registers.h>
#include <reg_model_maker/reg_model_maker.h>

#include <cassert>
#include <cstdint>
#include <mutex>
#include <vector>
#include <unordered_map>
#include <list>

#include "smmuv3_gen.h"
#include "smmuv3_memory_attrs.h"

#define SMMUV3_PAGESIZE 4096
#define SMMUV3_PAGEMASK (SMMUV3_PAGESIZE - 1)
#define SMMUV3_MAX_TBU  16

// STE/CD sizes from spec
#define SMMUV3_STE_SIZE 64
#define SMMUV3_CD_SIZE  64

// Command sizes
#define SMMUV3_CMD_SIZE   16
#define SMMUV3_EVENT_SIZE 32
#define SMMUV3_PRI_SIZE   16

// Event record type encodings (SMMUv3 specification).
#define SMMUV3_EVT_F_UUT          0x01
#define SMMUV3_EVT_C_BAD_STREAMID 0x02
#define SMMUV3_EVT_F_STE_FETCH    0x03
#define SMMUV3_EVT_C_BAD_STE      0x04
#define SMMUV3_EVT_F_CD_FETCH     0x09
#define SMMUV3_EVT_C_BAD_CD       0x0A
#define SMMUV3_EVT_F_WALK_EABT    0x0B
#define SMMUV3_EVT_F_TRANSLATION  0x10
#define SMMUV3_EVT_F_ADDR_SIZE    0x11
#define SMMUV3_EVT_F_ACCESS       0x12
#define SMMUV3_EVT_F_PERMISSION   0x13

namespace gs {

template <unsigned int BUSWIDTH = 32>
class smmuv3_tbu;

template <unsigned int BUSWIDTH = 32>
class smmuv3 : public sc_core::sc_module, public smmuv3_gen
{
    SCP_LOGGER();
    cci::cci_broker_handle m_broker;
    gs::json_zip_archive m_jza;
    bool loaded_ok;
    gs::json_module M;

public:
    typedef enum {
        IOMMU_NONE = 0,
        IOMMU_RO = 1,
        IOMMU_WO = 2,
        IOMMU_RW = 3,
    } IOMMUAccessFlags;

    struct IOMMUTLBEntry {
        uint64_t iova;
        uint64_t translated_addr;
        uint64_t addr_mask;
        IOMMUAccessFlags perm;
        uint64_t descriptor;  // Original descriptor for attribute extraction
        uint32_t table_attrs; // Accumulated table attributes
    };

    // Configuration parameters
    cci::cci_param<uint32_t> p_pamax;
    cci::cci_param<uint16_t> p_sidsize;
    cci::cci_param<bool> p_ato;
    cci::cci_param<uint8_t> p_num_tbu;
    cci::cci_param<uint32_t> p_iotlb_size;

    // Sockets
    tlm_utils::multi_passthrough_target_socket<smmuv3, BUSWIDTH> socket; // MMIO target socket (like smmu500)
    tlm_utils::simple_initiator_socket<smmuv3> dma_socket;
    InitiatorSignalSocket<bool> irq_eventq;
    InitiatorSignalSocket<bool> irq_priq;
    InitiatorSignalSocket<bool> irq_cmd_sync;
    InitiatorSignalSocket<bool> irq_gerror;
    TargetSignalSocket<bool> reset;

    // TBUs
    std::vector<smmuv3_tbu<BUSWIDTH>*> tbus;

    smmuv3(sc_core::sc_module_name name);

private:
    // STE (Stream Table Entry) structure
    struct STE {
        bool valid;
        uint32_t config;
        uint64_t s1contextptr;
        uint64_t s2ttb;
        uint32_t s2t0sz;
        uint32_t s2sl0;
        uint32_t s2tg;
        uint32_t s2ps;
        bool s1fmt;
        bool s1stalld;
        uint32_t s1cdmax;
        uint16_t vmid;
    };

    // CD (Context Descriptor) structure
    struct CD {
        bool valid;
        bool aarch64;
        uint64_t ttb[2];
        uint32_t tsz[2];
        uint32_t tg[2];
        uint32_t ips;
        uint16_t asid;
        bool epd[2];
    };

    // Page table walk context
    struct PtwCtx {
        uint64_t ttb[2]; // TTBR0, TTBR1
        uint32_t tsz[2]; // T0SZ, T1SZ
        uint32_t tg[2];  // TG0, TG1
        uint32_t ips;
        uint32_t sl0;
        bool epd[2]; // EPD0, EPD1
        int stage;
        uint64_t iova;
        uint32_t access;
        bool s2_enabled;
        STE* ste; // For S2 context
    };

    // Translation request
    struct TransReq {
        uint64_t iova;
        uint32_t access;
        uint32_t sid;
        uint32_t substream_id;
        STE ste;
        CD cd;
        uint64_t pa;
        uint32_t prot;
        uint64_t page_size;
        bool err;
        uint32_t event_type;
        uint32_t tableattrs; // Accumulated table attributes
        int fault_level;     // Level at which fault occurred
        uint64_t descriptor; // Final descriptor for attribute extraction
    };

    // IOTLB key
    struct IOTLBKey {
        uint16_t vmid;
        uint16_t asid;
        uint64_t iova;
        uint8_t tg;
        uint8_t level;

        bool operator==(const IOTLBKey& o) const
        {
            return vmid == o.vmid && asid == o.asid && iova == o.iova && tg == o.tg && level == o.level;
        }
    };

    struct IOTLBKeyHash {
        std::size_t operator()(const IOTLBKey& k) const
        {
            return std::hash<uint64_t>()(((uint64_t)k.vmid << 48) | ((uint64_t)k.asid << 32) | k.iova);
        }
    };

    // IOTLB
    std::unordered_map<IOTLBKey, IOMMUTLBEntry, IOTLBKeyHash> m_iotlb;
    std::list<IOTLBKey> m_iotlb_lru;

    // Stalled transactions
    struct StalledTxn {
        tlm::tlm_generic_payload txn;
        sc_core::sc_time delay;
        uint32_t stag;
        uint32_t sid;
        uint32_t substream_id;
        sc_core::sc_event resume_event;
        bool aborted;
    };
    std::unordered_map<uint32_t, StalledTxn> m_stalled_txns;
    uint32_t m_next_stag;

    // Config cache (STE/CD)
    std::unordered_map<uint32_t, STE> m_ste_cache;
    std::unordered_map<uint64_t, CD> m_cd_cache; // keyed by (sid << 16) | substream_id

    // Helpers
    static uint32_t extract32(uint32_t val, int start, int length) { return (val >> start) & ((1u << length) - 1); }

    static uint64_t extract64(uint64_t val, int start, int length) { return (val >> start) & ((1ULL << length) - 1); }

    static int clz32(uint32_t val) { return val ? __builtin_clz(val) : 32; }

    bool check_s2_startlevel(unsigned int pamax_val, int level, int inputsize, int stride)
    {
        if (level < 0) return false;
        switch (stride) {
        case 13: // 64KB
            if (level == 0 || (level == 1 && pamax_val <= 42)) return false;
            break;
        case 11: // 16KB
            if (level == 0 || (level == 1 && pamax_val <= 40)) return false;
            break;
        case 9: // 4KB
            if (level == 0 && pamax_val <= 42) return false;
            break;
        default:
            return false;
        }
        return true;
    }

    bool check_out_addr(uint64_t addr, unsigned int outputsize)
    {
        if (outputsize != 48 && extract64(addr, outputsize, 48 - outputsize)) {
            return false;
        }
        return true;
    }

    // DMA read helper
    bool dma_read(uint64_t addr, void* data, size_t len)
    {
        tlm::tlm_generic_payload txn;
        txn.set_command(tlm::TLM_READ_COMMAND);
        txn.set_address(addr);
        txn.set_data_ptr(reinterpret_cast<unsigned char*>(data));
        txn.set_data_length(len);
        txn.set_streaming_width(len);
        txn.set_byte_enable_length(0);
        txn.set_dmi_allowed(false);
        txn.set_response_status(tlm::TLM_INCOMPLETE_RESPONSE);
        sc_core::sc_time delay = sc_core::SC_ZERO_TIME;
        dma_socket->b_transport(txn, delay);
        return txn.get_response_status() == tlm::TLM_OK_RESPONSE;
    }

    // DMA write helper
    bool dma_write(uint64_t addr, const void* data, size_t len)
    {
        tlm::tlm_generic_payload txn;
        txn.set_command(tlm::TLM_WRITE_COMMAND);
        txn.set_address(addr);
        txn.set_data_ptr(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(data)));
        txn.set_data_length(len);
        txn.set_streaming_width(len);
        txn.set_byte_enable_length(0);
        txn.set_dmi_allowed(false);
        txn.set_response_status(tlm::TLM_INCOMPLETE_RESPONSE);
        sc_core::sc_time delay = sc_core::SC_ZERO_TIME;
        dma_socket->b_transport(txn, delay);
        return txn.get_response_status() == tlm::TLM_OK_RESPONSE;
    }

    // Stream table walk
    bool find_ste(uint32_t sid, STE& ste);

    // CD fetch
    bool read_cd(uint32_t sid, const STE& ste, uint32_t substream_id, CD& cd);

    // Page table walker
    bool smmuv3_ptw64(PtwCtx& ctx, TransReq& req);

    // Translation
    IOMMUTLBEntry smmuv3_translate(tlm::tlm_generic_payload& txn, uint32_t sid, uint32_t substream_id);

    // IOTLB
    void iotlb_insert(const IOTLBKey& key, const IOMMUTLBEntry& entry);
    bool iotlb_lookup(const IOTLBKey& key, IOMMUTLBEntry& entry);
    void iotlb_inv_all();
    void iotlb_inv_asid(uint16_t asid);
    void iotlb_inv_vmid(uint16_t vmid);
    void iotlb_inv_iova(uint16_t asid, uint64_t iova, uint8_t tg, uint32_t addr_mask);

    // Command/Event/PRI queues
    void consume_cmdq();
    void record_event(uint32_t type, uint32_t sid, uint64_t iova, uint32_t info);
    void record_pri(uint32_t sid, uint64_t iova, uint32_t flags);
    bool queue_full(uint32_t prod, uint32_t cons, uint32_t size);
    bool queue_empty(uint32_t prod, uint32_t cons);

    // Command handlers
    void handle_cmd_prefetch_config(const uint8_t* cmd);
    void handle_cmd_prefetch_addr(const uint8_t* cmd);
    void handle_cmd_cfgi_ste(const uint8_t* cmd);
    void handle_cmd_cfgi_ste_range(const uint8_t* cmd);
    void handle_cmd_cfgi_cd(const uint8_t* cmd);
    void handle_cmd_cfgi_cd_all(const uint8_t* cmd);
    void handle_cmd_cfgi_all(const uint8_t* cmd);
    void handle_cmd_tlbi_nh_all(const uint8_t* cmd);
    void handle_cmd_tlbi_nh_asid(const uint8_t* cmd);
    void handle_cmd_tlbi_nh_va(const uint8_t* cmd);
    void handle_cmd_tlbi_nh_vaa(const uint8_t* cmd);
    void handle_cmd_tlbi_s12_vmall(const uint8_t* cmd);
    void handle_cmd_tlbi_s2_ipa(const uint8_t* cmd);
    void handle_cmd_tlbi_nsnh_all(const uint8_t* cmd);
    void handle_cmd_sync(const uint8_t* cmd);
    void handle_cmd_resume(const uint8_t* cmd);
    void handle_cmd_pri_resp(const uint8_t* cmd);

    // Stall/resume
    void stall_txn(TransReq& req, tlm::tlm_generic_payload& txn, sc_core::sc_time& delay);

    // GATOS (manual translation)
    void do_gatos();

    // IRQ helpers
    void trigger_irq(InitiatorSignalSocket<bool>& irq)
    {
        if (irq.size() > 0) {
            irq->write(true);
            irq->write(false);
        }
    }

    // Per SMMUv3: an error N is signalled by toggling GERROR[N] so that it
    // differs from GERRORN[N]. Software acknowledges by flipping GERRORN[N]
    // to match. Therefore re-asserting an already-pending error is a no-op.
    void set_gerror(uint32_t bit)
    {
        uint32_t err = static_cast<uint32_t>(GERROR);
        uint32_t ack = static_cast<uint32_t>(GERRORN);
        if (((err ^ ack) >> bit) & 1u) {
            return;
        }
        GERROR = err ^ (1u << bit);
        if (static_cast<uint32_t>(IRQ_CTRL[IRQ_CTRL_GERROR_IRQEN])) {
            trigger_irq(irq_gerror);
        }
    }

public:
    friend class smmuv3_tbu<BUSWIDTH>;

    void start_of_simulation() override;
    void before_end_of_elaboration() override;
};

// TBU (Translation Buffer Unit)
template <unsigned int BUSWIDTH>
class smmuv3_tbu : public sc_core::sc_module
{
    SCP_LOGGER();
    smmuv3<BUSWIDTH>* m_smmu;
    std::mutex m_dmi_lock;

    struct MemoryView {
        uint64_t address;
        uint64_t page_start;
        uint64_t page_end;
        uint64_t page_size;
    };

protected:
    void b_transport(tlm::tlm_generic_payload& txn, sc_core::sc_time& delay)
    {
        sc_dt::uint64 addr = txn.get_address();
        tlm::tlm_command cmd = txn.get_command();

        uint32_t substream_id = 0;
        typename smmuv3<BUSWIDTH>::IOMMUTLBEntry te = m_smmu->smmuv3_translate(txn, p_topology_id, substream_id);

        if (te.perm == smmuv3<BUSWIDTH>::IOMMU_NONE ||
            (cmd == tlm::TLM_WRITE_COMMAND && te.perm == smmuv3<BUSWIDTH>::IOMMU_RO) ||
            (cmd == tlm::TLM_READ_COMMAND && te.perm == smmuv3<BUSWIDTH>::IOMMU_WO)) {
            txn.set_response_status(tlm::TLM_ADDRESS_ERROR_RESPONSE);
        } else {
            txn.set_address(te.translated_addr | (addr & SMMUV3_PAGEMASK));

            // set_extension() does not transfer ownership; the TBU keeps it.
            smmuv3_memory_attrs_extension ext;
            ext.set_from_descriptor(te.descriptor, te.table_attrs);
            txn.set_extension(&ext);

            SCP_INFO(()) << std::hex << "smmuv3 TBU b_transport: translate 0x" << addr << " to 0x"
                         << (te.translated_addr | (addr & SMMUV3_PAGEMASK)) << " attrs=" << ext.to_string();

            downstream_socket->b_transport(txn, delay);
            txn.set_address(addr);
            txn.clear_extension<smmuv3_memory_attrs_extension>();
        }
    }

    virtual unsigned int transport_dbg(tlm::tlm_generic_payload& txn)
    {
        sc_dt::uint64 addr = txn.get_address();
        uint32_t substream_id = 0;
        typename smmuv3<BUSWIDTH>::IOMMUTLBEntry te = m_smmu->smmuv3_translate(txn, p_topology_id, substream_id);
        txn.set_address(te.translated_addr | (addr & SMMUV3_PAGEMASK));
        int ret = downstream_socket->transport_dbg(txn);
        txn.set_address(addr);
        return ret;
    }

    virtual bool get_direct_mem_ptr(tlm::tlm_generic_payload& txn, tlm::tlm_dmi& dmi_data)
    {
        // DMI not implemented for v3 in initial pass
        return false;
    }

    virtual void invalidate_direct_mem_ptr(sc_dt::uint64 start, sc_dt::uint64 end)
    {
        // DMI not implemented
    }

public:
    cci::cci_param<uint32_t> p_topology_id;
    tlm_utils::simple_target_socket<smmuv3_tbu> upstream_socket;
    tlm_utils::simple_initiator_socket<smmuv3_tbu> downstream_socket;

    // Delegating constructor for factory registration
    smmuv3_tbu(const sc_core::sc_module_name& name, sc_core::sc_object* o)
        : smmuv3_tbu(name, dynamic_cast<smmuv3<BUSWIDTH>*>(o))
    {
    }

    smmuv3_tbu(sc_core::sc_module_name name, smmuv3<BUSWIDTH>* smmu)
        : sc_core::sc_module(name)
        , m_smmu(smmu)
        , p_topology_id("topology_id", 0)
        , upstream_socket("upstream_socket")
        , downstream_socket("downstream_socket")
    {
        // Register this TBU with the parent SMMU (like smmu500_tbu does)
        if (m_smmu) {
            m_smmu->tbus.push_back(this);
        }

        upstream_socket.register_b_transport(this, &smmuv3_tbu::b_transport);
        upstream_socket.register_transport_dbg(this, &smmuv3_tbu::transport_dbg);
        upstream_socket.register_get_direct_mem_ptr(this, &smmuv3_tbu::get_direct_mem_ptr);
        downstream_socket.register_invalidate_direct_mem_ptr(this, &smmuv3_tbu::invalidate_direct_mem_ptr);
    }
};

// ============================================================================
// Implementation (header-only to avoid module_register() collision)
// ============================================================================

template <unsigned int BUSWIDTH>
smmuv3<BUSWIDTH>::smmuv3(sc_core::sc_module_name name)
    : sc_core::sc_module(name)
    , smmuv3_gen()
    , m_broker(cci::cci_get_broker())
    , m_jza(zip_open_from_source(zip_source_buffer_create(gZipArchive_smmuv3_Data, gZipArchive_smmuv3_Size, 0, nullptr),
                                 ZIP_RDONLY, nullptr))
    , loaded_ok(m_jza.json_read_cci(m_broker, std::string(this->name()) + ".smmuv3"))
    , M("smmuv3", m_jza)
    , p_pamax("pamax", 48)
    , p_sidsize("sidsize", 16)
    , p_ato("ato", true)
    , p_num_tbu("num_tbu", 1)
    , p_iotlb_size("iotlb_size", 256)
    , socket("target_socket")
    , dma_socket("dma")
    , irq_eventq("irq_eventq")
    , irq_priq("irq_priq")
    , irq_cmd_sync("irq_cmd_sync")
    , irq_gerror("irq_gerror")
    , reset("reset")
    , m_next_stag(1)
{
    SCP_LOGGER();
    sc_assert(loaded_ok);

    // Bind MMIO target socket to json_module
    socket.bind(M.target_socket);

    // Bind registers to json_module
    bind_regs(M);

    // Note: TBUs are NOT created internally. They are created externally
    // in the platform Lua config (like smmu500) and register themselves
    // via m_smmu->tbus.push_back(this) in the smmuv3_tbu constructor.

    // Reset callback
    reset.register_value_changed_cb([this](bool v) {
        if (v) {
            // Assert: clear queues, IOTLB, config cache
            m_iotlb.clear();
            m_iotlb_lru.clear();
            m_ste_cache.clear();
            m_cd_cache.clear();
            m_stalled_txns.clear();
        } else {
            // Deassert: seed ID registers
            start_of_simulation();
        }
    });
}

template <unsigned int BUSWIDTH>
void smmuv3<BUSWIDTH>::start_of_simulation()
{
    // Seed ID registers with advertised capabilities
    IDR0 = 0;
    IDR0[IDR0_S2P] = 1;
    IDR0[IDR0_S1P] = 1;
    IDR0[IDR0_TTF] = 0x2; // AArch64 only
    IDR0[IDR0_COHACC] = 1;
    IDR0[IDR0_ASID16] = 1;
    IDR0[IDR0_VMID16] = 1;
    IDR0[IDR0_PRI] = 1;
    IDR0[IDR0_ATOS] = static_cast<uint32_t>(p_ato.get_value());
    IDR0[IDR0_HTTU] = 0;
    IDR0[IDR0_STLEVEL] = 0x2; // 2-level stream table

    IDR1 = 0;
    IDR1[IDR1_SIDSIZE] = p_sidsize.get_value();
    IDR1[IDR1_EVENTQS] = 19; // Log2 max entries
    IDR1[IDR1_CMDQS] = 19;

    IDR5 = 0;
    uint32_t oas_val = 0;
    switch (p_pamax.get_value()) {
    case 32:
        oas_val = 0;
        break;
    case 36:
        oas_val = 1;
        break;
    case 40:
        oas_val = 2;
        break;
    case 42:
        oas_val = 3;
        break;
    case 44:
        oas_val = 4;
        break;
    case 48:
        oas_val = 5;
        break;
    default:
        oas_val = 5;
        break;
    }
    IDR5[IDR5_OAS] = oas_val;
    IDR5[IDR5_GRAN4K] = 1;
    IDR5[IDR5_GRAN16K] = 1;
    IDR5[IDR5_GRAN64K] = 1;

    // PrimeCell IDs (ARM standard values)
    *IDREGS[0] = 0x04;  // PID4
    *IDREGS[1] = 0x00;  // PID5
    *IDREGS[2] = 0x00;  // PID6
    *IDREGS[3] = 0x00;  // PID7
    *IDREGS[4] = 0x34;  // PID0
    *IDREGS[5] = 0xB0;  // PID1
    *IDREGS[6] = 0x0B;  // PID2
    *IDREGS[7] = 0x00;  // PID3
    *IDREGS[8] = 0x0D;  // CID0
    *IDREGS[9] = 0xF0;  // CID1
    *IDREGS[10] = 0x05; // CID2
    *IDREGS[11] = 0xB1; // CID3

    IIDR = 0x43B20001; // Qualcomm implementation

    // Default GBPA: abort on bypass
    GBPA[GBPA_ABORT] = 1;
}

template <unsigned int BUSWIDTH>
void smmuv3<BUSWIDTH>::before_end_of_elaboration()
{
    SCP_INFO(()) << "SMMUv3: registering post_write callbacks";

    // Mirror CR0 → CR0ACK
    CR0.post_write([this](tlm::tlm_generic_payload&, sc_core::sc_time&) {
        uint32_t cr0 = static_cast<uint32_t>(CR0);
        SCP_INFO(()) << "SMMUv3: CR0 write = 0x" << std::hex << cr0;
        CR0ACK = cr0;
    });

    // Mirror IRQ_CTRL → IRQ_CTRL_ACK (software polls the ACK at offset 0x54)
    IRQ_CTRL.post_write([this](tlm::tlm_generic_payload&, sc_core::sc_time&) {
        uint32_t val = static_cast<uint32_t>(IRQ_CTRL);
        SCP_INFO(()) << "SMMUv3: IRQ_CTRL write = 0x" << std::hex << val;
        IRQ_CTRL_ACK = val;
    });

    // Process commands when CMDQ_PROD is updated (also handle via CMDQ_CONS pre_read below)
    CMDQ_PROD.post_write([this](tlm::tlm_generic_payload&, sc_core::sc_time&) {
        uint32_t prod = static_cast<uint32_t>(CMDQ_PROD);
        SCP_INFO(()) << "SMMUv3: CMDQ_PROD write = 0x" << std::hex << prod;
        if (static_cast<uint32_t>(CR0[CR0_CMDQEN])) {
            consume_cmdq();
        }
    });

    // pre_read on CMDQ_CONS - process pending commands before software reads the consumer pointer
    // This ensures CMDQ_CONS reflects the latest state when polled
    CMDQ_CONS.pre_read([this](tlm::tlm_generic_payload&, sc_core::sc_time&) {
        if (static_cast<uint32_t>(CR0[CR0_CMDQEN])) {
            consume_cmdq();
        }
    });

    // Stream table reconfiguration invalidates STE cache
    STRTAB_BASE_CFG.post_write([this](tlm::tlm_generic_payload&, sc_core::sc_time&) { m_ste_cache.clear(); });

    // GATOS - global address translation
    GATOS_CTRL.post_write([this](tlm::tlm_generic_payload&, sc_core::sc_time&) {
        if (static_cast<uint32_t>(GATOS_CTRL[GATOS_CTRL_RUN])) {
            do_gatos();
        }
    });

    // PRI queue consumer advance
    PRIQ_CONS.post_write([this](tlm::tlm_generic_payload&, sc_core::sc_time&) {
        // Advance PRI consumer pointer
    });

    // Page 1 register aliases - mirror writes from Page 1 to Page 0.
    // Software typically writes EVENTQ/PRIQ PROD/CONS through the Page 1 alias.
    CMDQ_PROD_P1.post_write([this](tlm::tlm_generic_payload&, sc_core::sc_time&) {
        CMDQ_PROD = static_cast<uint32_t>(CMDQ_PROD_P1);
        if (static_cast<uint32_t>(CR0[CR0_CMDQEN])) {
            consume_cmdq();
        }
    });

    // Page-1 software-writable aliases mirror writes back to Page 0:
    //   CMDQ_PROD, EVENTQ_CONS, PRIQ_CONS are software-written.
    //   CMDQ_CONS, EVENTQ_PROD, PRIQ_PROD are hardware-only on Page 0; their
    //   Page-1 copies are read-only mirrors and must not propagate writes.
    EVENTQ_CONS_P1.post_write(
        [this](tlm::tlm_generic_payload&, sc_core::sc_time&) { EVENTQ_CONS = static_cast<uint32_t>(EVENTQ_CONS_P1); });

    PRIQ_CONS_P1.post_write(
        [this](tlm::tlm_generic_payload&, sc_core::sc_time&) { PRIQ_CONS = static_cast<uint32_t>(PRIQ_CONS_P1); });

    // Page 1 reads should reflect Page 0 values (pre_read updates the alias before read)
    CMDQ_PROD_P1.pre_read(
        [this](tlm::tlm_generic_payload&, sc_core::sc_time&) { CMDQ_PROD_P1 = static_cast<uint32_t>(CMDQ_PROD); });
    CMDQ_CONS_P1.pre_read([this](tlm::tlm_generic_payload&, sc_core::sc_time&) {
        if (static_cast<uint32_t>(CR0[CR0_CMDQEN])) consume_cmdq();
        CMDQ_CONS_P1 = static_cast<uint32_t>(CMDQ_CONS);
    });
    EVENTQ_PROD_P1.pre_read(
        [this](tlm::tlm_generic_payload&, sc_core::sc_time&) { EVENTQ_PROD_P1 = static_cast<uint32_t>(EVENTQ_PROD); });
    EVENTQ_CONS_P1.pre_read(
        [this](tlm::tlm_generic_payload&, sc_core::sc_time&) { EVENTQ_CONS_P1 = static_cast<uint32_t>(EVENTQ_CONS); });
    PRIQ_PROD_P1.pre_read(
        [this](tlm::tlm_generic_payload&, sc_core::sc_time&) { PRIQ_PROD_P1 = static_cast<uint32_t>(PRIQ_PROD); });
    PRIQ_CONS_P1.pre_read(
        [this](tlm::tlm_generic_payload&, sc_core::sc_time&) { PRIQ_CONS_P1 = static_cast<uint32_t>(PRIQ_CONS); });
}

// Stream table walk
template <unsigned int BUSWIDTH>
bool smmuv3<BUSWIDTH>::find_ste(uint32_t sid, STE& ste)
{
    // Check cache
    auto it = m_ste_cache.find(sid);
    if (it != m_ste_cache.end()) {
        ste = it->second;
        return ste.valid;
    }

    // STRTAB_BASE_LO holds the base address in bits[51:6]; bits[5:0] carry the
    // read-allocate hint and must be stripped before using as a pointer.
    uint64_t strtab_base = ((static_cast<uint64_t>(STRTAB_BASE_HI) << 32) |
                            static_cast<uint64_t>(STRTAB_BASE_LO)) &
                           0x000FFFFFFFFFFFC0ULL;
    uint32_t fmt = static_cast<uint32_t>(STRTAB_BASE_CFG[STRTAB_BASE_CFG_FMT]);
    uint32_t log2size = static_cast<uint32_t>(STRTAB_BASE_CFG[STRTAB_BASE_CFG_LOG2SIZE]);

    uint8_t ste_buf[SMMUV3_STE_SIZE];
    uint64_t ste_addr;

    if (fmt == 0) {
        // Linear stream table
        if (sid >= (1u << log2size)) {
            return false;
        }
        ste_addr = strtab_base + (sid * SMMUV3_STE_SIZE);
    } else {
        // 2-level stream table
        uint32_t split = static_cast<uint32_t>(STRTAB_BASE_CFG[STRTAB_BASE_CFG_SPLIT]);
        uint32_t l1_idx = sid >> split;
        uint32_t l2_idx = sid & ((1u << split) - 1);

        if (l1_idx >= (1u << log2size)) {
            return false;
        }

        // Read L1 STD (8 bytes)
        uint64_t l1std_addr = strtab_base + (l1_idx * 8);
        uint64_t l1std;
        if (!dma_read(l1std_addr, &l1std, 8)) {
            set_gerror(2); // EVENTQ_ABT_ERR
            return false;
        }

        if ((l1std & 1) == 0) {
            // L1 entry not valid
            return false;
        }

        uint64_t l2ptr = l1std & ~((1ULL << 6) - 1);
        ste_addr = l2ptr + (l2_idx * SMMUV3_STE_SIZE);
    }

    // Read STE
    if (!dma_read(ste_addr, ste_buf, SMMUV3_STE_SIZE)) {
        set_gerror(2);
        return false;
    }

    uint64_t dw0 = *reinterpret_cast<uint64_t*>(ste_buf);
    uint64_t dw1 = *reinterpret_cast<uint64_t*>(ste_buf + 8);
    uint64_t dw2 = *reinterpret_cast<uint64_t*>(ste_buf + 16);
    uint64_t dw3 = *reinterpret_cast<uint64_t*>(ste_buf + 24);

    ste.valid = dw0 & 1;
    ste.config = extract64(dw0, 1, 3);
    ste.s1fmt = extract64(dw0, 4, 2);
    ste.s1contextptr = dw0 & 0x000FFFFFFFFFFFC0ULL;
    ste.s1cdmax = extract64(dw0, 59, 5);

    ste.s1stalld = extract64(dw1, 27, 1);

    ste.vmid = extract64(dw2, 0, 16);
    // VTCR is packed into DW2[50:32]: T0SZ[5:0], SL0[7:6], TG0[15:14], PS[18:16].
    ste.s2t0sz = extract64(dw2, 32, 6);
    ste.s2sl0 = extract64(dw2, 38, 2);
    ste.s2tg = extract64(dw2, 46, 2);
    ste.s2ps = extract64(dw2, 48, 3);

    ste.s2ttb = dw3 & 0x000FFFFFFFFFFFF0ULL;

    m_ste_cache[sid] = ste;
    return ste.valid;
}

template <unsigned int BUSWIDTH>
bool smmuv3<BUSWIDTH>::read_cd(uint32_t sid, const STE& ste, uint32_t substream_id, CD& cd)
{
    uint64_t key = (static_cast<uint64_t>(sid) << 32) | substream_id;
    auto it = m_cd_cache.find(key);
    if (it != m_cd_cache.end()) {
        cd = it->second;
        return cd.valid;
    }

    uint64_t cd_addr = ste.s1contextptr + (substream_id * SMMUV3_CD_SIZE);
    uint8_t cd_buf[SMMUV3_CD_SIZE];

    if (!dma_read(cd_addr, cd_buf, SMMUV3_CD_SIZE)) {
        set_gerror(2);
        return false;
    }

    const uint32_t* w = reinterpret_cast<const uint32_t*>(cd_buf);

    cd.valid = (w[0] >> 31) & 0x1;
    cd.aarch64 = (w[1] >> 9) & 0x1;
    cd.tsz[0] = w[0] & 0x3F;
    cd.tg[0] = (w[0] >> 6) & 0x3;
    cd.epd[0] = (w[0] >> 14) & 0x1;
    cd.tsz[1] = (w[0] >> 16) & 0x3F;
    cd.tg[1] = (w[0] >> 22) & 0x3;
    cd.epd[1] = (w[0] >> 30) & 0x1;
    cd.ips = w[1] & 0x7;
    cd.asid = (w[1] >> 16) & 0xFFFF;

    cd.ttb[0] = (static_cast<uint64_t>(w[3] & 0xFFFFF) << 32) | static_cast<uint64_t>(w[2] & 0xFFFFFFF0);
    cd.ttb[1] = (static_cast<uint64_t>(w[5] & 0xFFFFF) << 32) | static_cast<uint64_t>(w[4] & 0xFFFFFFF0);

    m_cd_cache[key] = cd;
    return cd.valid;
}

template <unsigned int BUSWIDTH>
bool smmuv3<BUSWIDTH>::smmuv3_ptw64(PtwCtx& ctx, TransReq& req)
{
    // Indexed by IDR5.OAS (0=32 .. 5..7=48 bits of output addr size).
    const unsigned int outsize_map[] = { 32, 36, 40, 42, 44, 48, 48, 48 };

    unsigned int tsz;
    unsigned int inputsize;
    unsigned int outputsize;
    unsigned int grainsize = 0;
    unsigned int stride;
    int level = 0;
    unsigned int firstblocklevel = 0;
    unsigned int tg;
    unsigned int baselowerbound;
    bool blocktranslate = false;
    bool epd = false;
    uint64_t descmask;
    uint64_t ttbr;
    uint64_t desc;
    uint32_t attrs;
    uint32_t s2attrs;

    req.err = false;
    req.tableattrs = 0;
    req.fault_level = 0;

    // Select TTBR0 or TTBR1 based on VA[63]
    if (ctx.stage == 1) {
        if ((ctx.iova & (1ULL << 63)) == 0) {
            // Use TTBR0
            ttbr = ctx.ttb[0];
            tsz = ctx.tsz[0];
            tg = ctx.tg[0];
            epd = ctx.epd[0];
        } else {
            // Use TTBR1. TG1 uses a different encoding from TG0
            // (01=16K, 10=4K, 11=64K). Remap into the TG0 form the rest of
            // this function expects (0=4K, 1=64K, 2=16K).
            ttbr = ctx.ttb[1];
            tsz = ctx.tsz[1];
            epd = ctx.epd[1];
            switch (ctx.tg[1]) {
            case 1: tg = 2; break;
            case 2: tg = 0; break;
            case 3: tg = 1; break;
            default: tg = 0xFF; break;
            }
        }
    } else {
        // Stage 2: only TTBR0
        ttbr = ctx.ttb[0];
        tsz = ctx.tsz[0];
        tg = ctx.tg[0];
        epd = ctx.epd[0];
    }

    if (epd) {
        req.event_type = SMMUV3_EVT_F_TRANSLATION;
        req.err = true;
        return false;
    }

    inputsize = 64 - tsz;

    // Decode granule size
    switch (tg) {
    case 1: // 64KB
        grainsize = 16;
        level = 3;
        firstblocklevel = 2;
        break;
    case 2: // 16KB
        grainsize = 14;
        level = 3;
        firstblocklevel = 2;
        break;
    case 0: // 4KB
        grainsize = 12;
        level = 2;
        firstblocklevel = 1;
        break;
    default:
        SCP_ERR(()) << "Invalid TG value: " << tg;
        req.event_type = SMMUV3_EVT_F_ADDR_SIZE;
        req.err = true;
        return false;
    }

    outputsize = outsize_map[ctx.ips];
    if (outputsize > p_pamax) outputsize = p_pamax;

    stride = grainsize - 3;

    // Determine starting level
    if (ctx.stage == 1) {
        if (grainsize < 16 && (inputsize > (grainsize + 3 * stride)))
            level = 0;
        else if (inputsize > (grainsize + 2 * stride))
            level = 1;
        else if (inputsize > (grainsize + stride))
            level = 2;

        // Check input address size
        if (inputsize < 25 || inputsize > 48 || extract64(ctx.iova, inputsize, 64 - inputsize)) {
            req.event_type = SMMUV3_EVT_F_ADDR_SIZE;
            req.err = true;
            return false;
        }
    } else {
        // Stage 2: use SL0 to determine starting level
        unsigned int startlevel = ctx.sl0;
        level = 3 - startlevel;
        if (grainsize == 12) level = 2 - startlevel;
        if (!check_s2_startlevel(outputsize, level, inputsize, stride)) {
            req.event_type = SMMUV3_EVT_F_ADDR_SIZE;
            req.err = true;
            return false;
        }
    }

    baselowerbound = 3 + inputsize - ((3 - level) * stride + grainsize);
    ttbr = extract64(ttbr, 0, 48);
    ttbr &= ~((1ULL << baselowerbound) - 1);

    if (!check_out_addr(ttbr, outputsize)) {
        req.event_type = SMMUV3_EVT_F_ADDR_SIZE;
        req.err = true;
        return false;
    }

    descmask = (1ULL << grainsize) - 1;

    // Page table walk loop
    do {
        unsigned int addrselectbottom = (3 - level) * stride + grainsize;
        uint64_t index = (ctx.iova >> (addrselectbottom - 3)) & descmask;
        index &= ~7ULL;
        uint64_t descaddr = ttbr | index;

        // Nested S1+S2: S1 descriptor fetch goes through S2 translation
        if (ctx.stage == 1 && ctx.s2_enabled) {
            PtwCtx s2ctx;
            s2ctx.ttb[0] = ctx.ste->s2ttb;
            s2ctx.tsz[0] = ctx.ste->s2t0sz;
            s2ctx.tg[0] = ctx.ste->s2tg;
            s2ctx.ips = ctx.ste->s2ps;
            s2ctx.sl0 = ctx.ste->s2sl0;
            s2ctx.epd[0] = false;
            s2ctx.stage = 2;
            s2ctx.iova = descaddr;
            s2ctx.access = IOMMU_RO;
            s2ctx.s2_enabled = false;
            s2ctx.ste = nullptr;

            TransReq s2req;
            s2req.iova = descaddr;
            s2req.access = IOMMU_RO;
            s2req.pa = descaddr;

            if (!smmuv3_ptw64(s2ctx, s2req) || s2req.err) {
                req.event_type = SMMUV3_EVT_F_TRANSLATION;
                req.fault_level = level;
                req.err = true;
                return false;
            }
            descaddr = s2req.pa;
        }

        // Read descriptor via DMA
        if (!dma_read(descaddr, &desc, sizeof(desc))) {
            req.event_type = SMMUV3_EVT_F_WALK_EABT;
            req.fault_level = level;
            req.err = true;
            return false;
        }

        unsigned int type = desc & 3;
        SCP_INFO(()) << "S" << ctx.stage << " L" << level << " iova=0x" << std::hex << ctx.iova << " descaddr=0x"
                     << descaddr << " desc=0x" << desc << " type=" << type;

        ttbr = extract64(desc, 0, 48);
        ttbr &= ~descmask;

        // At L3 the only valid encoding is type=3 (page). Anything else
        // (including type=2, which has bit[0]=0, i.e. invalid) must fault.
        if (level == 3) {
            if (type != 3) {
                req.event_type = SMMUV3_EVT_F_TRANSLATION;
                req.fault_level = level;
                req.err = true;
                return false;
            }
            break;
        }

        switch (type) {
        case 2:
        case 0:
            // Invalid or reserved
            req.event_type = SMMUV3_EVT_F_TRANSLATION;
            req.fault_level = level;
            req.err = true;
            return false;

        case 1:
            // Block descriptor
            blocktranslate = true;
            if (level < (int)firstblocklevel) {
                req.event_type = SMMUV3_EVT_F_TRANSLATION;
                req.fault_level = level;
                req.err = true;
                return false;
            }
            break;

        case 3:
            // Table descriptor
            req.tableattrs |= extract64(desc, 59, 5);
            if (!check_out_addr(ttbr, outputsize)) {
                req.event_type = SMMUV3_EVT_F_ADDR_SIZE;
                req.fault_level = level;
                req.err = true;
                return false;
            }
            level++;
            break;
        }
    } while (!blocktranslate);

    if (!check_out_addr(ttbr, outputsize)) {
        req.event_type = SMMUV3_EVT_F_ADDR_SIZE;
        req.fault_level = level;
        req.err = true;
        return false;
    }

    // Calculate page size and final PA
    unsigned long page_size = (1ULL << ((stride * (4 - level)) + 3));
    ttbr |= (ctx.iova & (page_size - 1));
    req.page_size = ((stride * (4 - level)) + 3);
    req.pa = ttbr;

    // Store descriptor for memory attribute extraction
    req.descriptor = desc;

    // Extract and check attributes
    s2attrs = attrs = extract64(desc, 2, 10) | (extract64(desc, 52, 12) << 10);

    if (ctx.stage == 1) {
        // Accumulate table attributes for S1
        attrs |= extract32(req.tableattrs, 0, 2) << 11;
        attrs |= extract32(req.tableattrs, 3, 1) << 5;
    }

    req.prot = IOMMU_RW;

    // attrs[] is the descriptor field with bits shifted down by 2
    //   (extract64(desc, 2, 10) gives desc[11:2] in attrs[9:0])
    // So desc-bit N maps to attrs-bit (N-2) for N in [2..11]:
    //   desc[6]  = AP[1]     -> attrs[4]
    //   desc[7]  = AP[2]     -> attrs[5]
    //   desc[10] = AF        -> attrs[8]
    //   desc[11] = nG        -> attrs[9]

    // Check Access Flag (AF)
    if ((attrs & (1 << 8)) == 0) {
        req.event_type = SMMUV3_EVT_F_ACCESS;
        req.fault_level = level;
        req.err = true;
        return false;
    }

    // Check permissions
    if (ctx.stage == 1) {
        // Stage 1: AP[1] at attrs[4] (desc[6]), AP[2] at attrs[5] (desc[7])
        //   AP[1]=1 means EL0 access allowed (required)
        //   AP[2]=0 means read/write, AP[2]=1 means read-only
        if (!(attrs & (1 << 4))) {
            // AP[1]=0, no user access
            req.event_type = SMMUV3_EVT_F_PERMISSION;
            req.fault_level = level;
            req.err = true;
            return false;
        }
        if (attrs & (1 << 5)) {
            // AP[2]=1, read-only
            if (ctx.access == IOMMU_WO) {
                req.event_type = SMMUV3_EVT_F_PERMISSION;
                req.fault_level = level;
                req.err = true;
                return false;
            }
            req.prot &= ~IOMMU_WO;
        }
    } else {
        // Stage 2: S2AP at desc[7:6] -> s2attrs[5:4]
        switch ((s2attrs >> 4) & 3) {
        case 0:
            // No access
            req.event_type = SMMUV3_EVT_F_PERMISSION;
            req.fault_level = level;
            req.err = true;
            return false;
        case 1:
            // Read-only
            if (ctx.access == IOMMU_WO) {
                req.event_type = SMMUV3_EVT_F_PERMISSION;
                req.fault_level = level;
                req.err = true;
                return false;
            }
            req.prot &= ~IOMMU_WO;
            break;
        case 2:
            // Write-only
            if (ctx.access == IOMMU_RO) {
                req.event_type = SMMUV3_EVT_F_PERMISSION;
                req.fault_level = level;
                req.err = true;
                return false;
            }
            req.prot &= ~IOMMU_RO;
            break;
        case 3:
            // Read/write
            break;
        }
    }

    SCP_INFO(()) << "PTW success: 0x" << std::hex << ctx.iova << " -> 0x" << req.pa << " prot=" << req.prot
                 << " page_size=" << std::dec << req.page_size;
    return true;
}

// Translation entry point
template <unsigned int BUSWIDTH>
typename smmuv3<BUSWIDTH>::IOMMUTLBEntry smmuv3<BUSWIDTH>::smmuv3_translate(tlm::tlm_generic_payload& txn, uint32_t sid,
                                                                            uint32_t substream_id)
{
    uint64_t iova = txn.get_address();
    IOMMUTLBEntry ret = {
        .iova = iova,
        .translated_addr = iova,
        .addr_mask = (1ULL << 12) - 1,
        .perm = IOMMU_RW,
    };

    // Check if SMMU is enabled
    if (static_cast<uint32_t>(CR0[CR0_SMMUEN]) == 0) {
        ret.addr_mask = -1ULL;
        return ret;
    }

    TransReq req;
    req.iova = iova;
    req.sid = sid;
    req.substream_id = substream_id;
    req.access = (txn.get_command() == tlm::TLM_WRITE_COMMAND) ? IOMMU_WO : IOMMU_RO;

    // Find STE
    if (!find_ste(sid, req.ste) || !req.ste.valid) {
        // Global bypass or fault
        if (static_cast<uint32_t>(GBPA[GBPA_ABORT])) {
            ret.perm = IOMMU_NONE;
            record_event(SMMUV3_EVT_F_STE_FETCH, sid, iova, 0);
            return ret;
        } else {
            ret.addr_mask = -1ULL;
            return ret;
        }
    }

    // STE.Config: 0=Abort, 4=Bypass, 5=S1, 6=S2, 7=Nested.
    switch (req.ste.config) {
    case 0: // Abort
        ret.perm = IOMMU_NONE;
        record_event(SMMUV3_EVT_C_BAD_STE, sid, iova, 0);
        return ret;
    case 4: // Bypass
        ret.addr_mask = -1ULL;
        return ret;
    case 5: // S1 only
        if (!read_cd(sid, req.ste, substream_id, req.cd) || !req.cd.valid) {
            ret.perm = IOMMU_NONE;
            record_event(SMMUV3_EVT_F_CD_FETCH, sid, iova, 0);
            return ret;
        }
        {
            IOTLBKey lookup{ req.ste.vmid, req.cd.asid, iova & ~((1ULL << 12) - 1),
                             static_cast<uint8_t>(req.cd.tg[0]), 3 };
            if (iotlb_lookup(lookup, ret)) {
                return ret;
            }
        }
        {
            PtwCtx ctx;
            ctx.ttb[0] = req.cd.ttb[0];
            ctx.ttb[1] = req.cd.ttb[1];
            ctx.tsz[0] = req.cd.tsz[0];
            ctx.tsz[1] = req.cd.tsz[1];
            ctx.tg[0] = req.cd.tg[0];
            ctx.tg[1] = req.cd.tg[1];
            ctx.ips = req.cd.ips;
            ctx.epd[0] = req.cd.epd[0];
            ctx.epd[1] = req.cd.epd[1];
            ctx.stage = 1;
            ctx.iova = iova;
            ctx.access = req.access;
            ctx.s2_enabled = false;
            ctx.ste = nullptr;
            smmuv3_ptw64(ctx, req);
        }
        break;
    case 6: // S2 only
    {
        IOTLBKey lookup{ req.ste.vmid, 0, iova & ~((1ULL << 12) - 1),
                         static_cast<uint8_t>(req.ste.s2tg), 3 };
        if (iotlb_lookup(lookup, ret)) {
            return ret;
        }
        PtwCtx ctx;
        ctx.ttb[0] = req.ste.s2ttb;
        ctx.tsz[0] = req.ste.s2t0sz;
        ctx.tg[0] = req.ste.s2tg;
        ctx.ips = req.ste.s2ps;
        ctx.sl0 = req.ste.s2sl0;
        ctx.epd[0] = false;
        ctx.stage = 2;
        ctx.iova = iova;
        ctx.access = req.access;
        ctx.s2_enabled = false;
        ctx.ste = nullptr;
        smmuv3_ptw64(ctx, req);
    } break;
    case 7: // Nested S1+S2
        if (!read_cd(sid, req.ste, substream_id, req.cd) || !req.cd.valid) {
            ret.perm = IOMMU_NONE;
            record_event(SMMUV3_EVT_F_CD_FETCH, sid, iova, 0);
            return ret;
        }
        {
            IOTLBKey lookup{ req.ste.vmid, req.cd.asid, iova & ~((1ULL << 12) - 1),
                             static_cast<uint8_t>(req.cd.tg[0]), 3 };
            if (iotlb_lookup(lookup, ret)) {
                return ret;
            }
        }
        // S1 walk; descriptor fetches go through S2.
        {
            PtwCtx ctx;
            ctx.ttb[0] = req.cd.ttb[0];
            ctx.ttb[1] = req.cd.ttb[1];
            ctx.tsz[0] = req.cd.tsz[0];
            ctx.tsz[1] = req.cd.tsz[1];
            ctx.tg[0] = req.cd.tg[0];
            ctx.tg[1] = req.cd.tg[1];
            ctx.ips = req.cd.ips;
            ctx.epd[0] = req.cd.epd[0];
            ctx.epd[1] = req.cd.epd[1];
            ctx.stage = 1;
            ctx.iova = iova;
            ctx.access = req.access;
            ctx.s2_enabled = true;
            ctx.ste = &req.ste;
            smmuv3_ptw64(ctx, req);

            // If S1 succeeded, translate the result through S2
            if (!req.err) {
                uint64_t s1_pa = req.pa;
                PtwCtx s2ctx;
                s2ctx.ttb[0] = req.ste.s2ttb;
                s2ctx.tsz[0] = req.ste.s2t0sz;
                s2ctx.tg[0] = req.ste.s2tg;
                s2ctx.ips = req.ste.s2ps;
                s2ctx.sl0 = req.ste.s2sl0;
                s2ctx.epd[0] = false;
                s2ctx.stage = 2;
                s2ctx.iova = s1_pa;
                s2ctx.access = req.access;
                s2ctx.s2_enabled = false;
                s2ctx.ste = nullptr;
                smmuv3_ptw64(s2ctx, req);
            }
        }
        break;
    default:
        ret.perm = IOMMU_NONE;
        record_event(SMMUV3_EVT_C_BAD_STE, sid, iova, 0);
        return ret;
    }

    if (req.err) {
        ret.perm = IOMMU_NONE;
        record_event(req.event_type, sid, iova, req.fault_level);
        return ret;
    }

    ret.translated_addr = req.pa;
    ret.perm = static_cast<IOMMUAccessFlags>(req.prot);
    ret.addr_mask = (1ULL << req.page_size) - 1;
    ret.descriptor = req.descriptor;
    ret.table_attrs = req.tableattrs;

    // Insert into IOTLB
    IOTLBKey key;
    key.vmid = req.ste.vmid;
    key.asid = req.cd.asid;
    key.iova = iova & ~ret.addr_mask;
    key.tg = req.cd.tg[0];
    key.level = 3;
    iotlb_insert(key, ret);

    return ret;
}

// IOTLB
template <unsigned int BUSWIDTH>
void smmuv3<BUSWIDTH>::iotlb_insert(const IOTLBKey& key, const IOMMUTLBEntry& entry)
{
    if (m_iotlb.size() >= p_iotlb_size.get_value()) {
        // Evict LRU
        auto lru_key = m_iotlb_lru.back();
        m_iotlb_lru.pop_back();
        m_iotlb.erase(lru_key);
    }
    m_iotlb[key] = entry;
    m_iotlb_lru.push_front(key);
}

template <unsigned int BUSWIDTH>
bool smmuv3<BUSWIDTH>::iotlb_lookup(const IOTLBKey& key, IOMMUTLBEntry& entry)
{
    auto it = m_iotlb.find(key);
    if (it != m_iotlb.end()) {
        entry = it->second;
        return true;
    }
    return false;
}

template <unsigned int BUSWIDTH>
void smmuv3<BUSWIDTH>::iotlb_inv_all()
{
    m_iotlb.clear();
    m_iotlb_lru.clear();
}

template <unsigned int BUSWIDTH>
void smmuv3<BUSWIDTH>::iotlb_inv_asid(uint16_t asid)
{
    for (auto it = m_iotlb.begin(); it != m_iotlb.end();) {
        if (it->first.asid == asid) {
            m_iotlb_lru.remove(it->first);
            it = m_iotlb.erase(it);
        } else {
            ++it;
        }
    }
}

template <unsigned int BUSWIDTH>
void smmuv3<BUSWIDTH>::iotlb_inv_vmid(uint16_t vmid)
{
    for (auto it = m_iotlb.begin(); it != m_iotlb.end();) {
        if (it->first.vmid == vmid) {
            m_iotlb_lru.remove(it->first);
            it = m_iotlb.erase(it);
        } else {
            ++it;
        }
    }
}

template <unsigned int BUSWIDTH>
void smmuv3<BUSWIDTH>::iotlb_inv_iova(uint16_t asid, uint64_t iova, uint8_t tg, uint32_t addr_mask)
{
    uint64_t iova_masked = iova & ~addr_mask;
    for (auto it = m_iotlb.begin(); it != m_iotlb.end();) {
        uint64_t entry_masked = it->first.iova & ~addr_mask;
        if (it->first.asid == asid && entry_masked == iova_masked && it->first.tg == tg) {
            m_iotlb_lru.remove(it->first);
            it = m_iotlb.erase(it);
        } else {
            ++it;
        }
    }
}

// Command queue
template <unsigned int BUSWIDTH>
void smmuv3<BUSWIDTH>::consume_cmdq()
{
    // CMDQ_BASE address: bits 51:5 of the 64-bit register (mask out RA bit 62 in HI)
    uint64_t hi = static_cast<uint64_t>(CMDQ_BASE_HI) & 0x000FFFFFULL; // bits 51:32 only
    uint64_t lo = static_cast<uint64_t>(CMDQ_BASE_LO) & ~0x1FULL;      // bits 31:5
    uint64_t base = (hi << 32) | lo;
    uint32_t log2size = static_cast<uint32_t>(CMDQ_BASE_LO[CMDQ_BASE_LOG2SIZE]);
    if (log2size == 0) return; // Queue not configured yet

    uint32_t prod = static_cast<uint32_t>(CMDQ_PROD);
    uint32_t cons = static_cast<uint32_t>(CMDQ_CONS);
    uint32_t wrap_mask = (1u << (log2size + 1)) - 1; // INDEX + WRP bits
    uint32_t idx_mask = (1u << log2size) - 1;        // INDEX bits only

    SCP_INFO(()) << "SMMUv3: consume_cmdq base=0x" << std::hex << base << " log2size=" << std::dec << log2size
                 << " prod=0x" << std::hex << prod << " cons=0x" << cons;

    while ((prod & wrap_mask) != (cons & wrap_mask)) {
        uint32_t idx = cons & idx_mask;
        uint64_t cmd_addr = base + (idx * SMMUV3_CMD_SIZE);
        uint8_t cmd[SMMUV3_CMD_SIZE];

        if (!dma_read(cmd_addr, cmd, SMMUV3_CMD_SIZE)) {
            SCP_WARN(()) << "SMMUv3: dma_read failed at 0x" << std::hex << cmd_addr;
            set_gerror(0); // CMDQ_ERR
            CMDQ_CONS = cons;
            return;
        }

        uint8_t opcode = cmd[0];
        SCP_INFO(()) << "SMMUv3: cmd opcode=0x" << std::hex << (int)opcode << " at idx=" << std::dec << idx;

        // CMDQ command opcodes
        switch (opcode) {
        case 0x01:
            handle_cmd_prefetch_config(cmd);
            break; // PREFETCH_CONFIG
        case 0x02:
            handle_cmd_prefetch_addr(cmd);
            break; // PREFETCH_ADDR
        case 0x03:
            handle_cmd_cfgi_ste(cmd);
            break; // CFGI_STE
        case 0x04:
            handle_cmd_cfgi_ste_range(cmd);
            break; // CFGI_STE_RANGE (also encodes CFGI_ALL with Range=0x1F)
        case 0x05:
            handle_cmd_cfgi_cd(cmd);
            break; // CFGI_CD
        case 0x06:
            handle_cmd_cfgi_cd_all(cmd);
            break; // CFGI_CD_ALL
        case 0x07:
            handle_cmd_cfgi_all(cmd);
            break; // CFGI_ALL
        case 0x10:
            handle_cmd_tlbi_nh_all(cmd);
            break; // TLBI_NH_ALL
        case 0x11:
            handle_cmd_tlbi_nh_asid(cmd);
            break; // TLBI_NH_ASID
        case 0x12:
            handle_cmd_tlbi_nh_va(cmd);
            break; // TLBI_NH_VA
        case 0x13:
            handle_cmd_tlbi_nh_vaa(cmd);
            break; // TLBI_NH_VAA
        case 0x18:
            handle_cmd_tlbi_nh_all(cmd);
            break; // TLBI_EL3_ALL (treat as NH_ALL)
        case 0x1A:
            handle_cmd_tlbi_nh_va(cmd);
            break; // TLBI_EL3_VA (treat as NH_VA)
        case 0x20:
            handle_cmd_tlbi_nh_all(cmd);
            break; // TLBI_EL2_ALL
        case 0x21:
            handle_cmd_tlbi_nh_asid(cmd);
            break; // TLBI_EL2_ASID
        case 0x22:
            handle_cmd_tlbi_nh_va(cmd);
            break; // TLBI_EL2_VA
        case 0x23:
            handle_cmd_tlbi_nh_vaa(cmd);
            break; // TLBI_EL2_VAA
        case 0x28:
            handle_cmd_tlbi_s12_vmall(cmd);
            break; // TLBI_S12_VMALL
        case 0x2A:
            handle_cmd_tlbi_s2_ipa(cmd);
            break; // TLBI_S2_IPA
        case 0x30:
            handle_cmd_tlbi_nsnh_all(cmd);
            break; // TLBI_NSNH_ALL
        case 0x40: /* ATC_INV - NOP in Qbox */
            break;
        case 0x41:
            handle_cmd_pri_resp(cmd);
            break; // PRI_RESP
        case 0x44:
            handle_cmd_resume(cmd);
            break; // RESUME
        case 0x45: /* STALL_TERM - NOP */
            break;
        case 0x46:
            handle_cmd_sync(cmd);
            break; // SYNC
        default:
            SCP_WARN(()) << "SMMUv3: unknown opcode 0x" << std::hex << (int)opcode;
            set_gerror(0); // CMDQ_ERR
            CMDQ_CONS = cons;
            return;
        }

        // Increment cons (natural wrap handles WRP bit)
        cons = (cons + 1) & wrap_mask;
    }

    CMDQ_CONS = cons;
    SCP_INFO(()) << "SMMUv3: consume_cmdq done, CMDQ_CONS=0x" << std::hex << cons;
}

template <unsigned int BUSWIDTH>
bool smmuv3<BUSWIDTH>::queue_full(uint32_t prod, uint32_t cons, uint32_t size)
{
    // Queue full: indices match but wrap bits differ (bit `size` is the wrap).
    uint32_t mask = (1u << size) - 1;
    uint32_t wrap = 1u << size;
    return ((prod ^ cons) == wrap) && ((prod & mask) == (cons & mask));
}

template <unsigned int BUSWIDTH>
bool smmuv3<BUSWIDTH>::queue_empty(uint32_t prod, uint32_t cons)
{
    return prod == cons;
}

// Event recording
template <unsigned int BUSWIDTH>
void smmuv3<BUSWIDTH>::record_event(uint32_t type, uint32_t sid, uint64_t iova, uint32_t info)
{
    if (static_cast<uint32_t>(CR0[CR0_EVENTQEN]) == 0) {
        return;
    }

    uint64_t base = (static_cast<uint64_t>(EVENTQ_BASE_HI) << 32) |
                    (static_cast<uint64_t>(EVENTQ_BASE_LO) & ~((1ULL << 5) - 1));
    uint32_t log2size = static_cast<uint32_t>(EVENTQ_BASE_LO[EVENTQ_BASE_LOG2SIZE]);
    uint32_t prod = static_cast<uint32_t>(EVENTQ_PROD);
    uint32_t cons = static_cast<uint32_t>(EVENTQ_CONS);

    if (queue_full(prod, cons, log2size)) {
        set_gerror(2); // EVENTQ_ABT_ERR
        return;
    }

    uint32_t idx = prod & ((1u << log2size) - 1);
    uint64_t evt_addr = base + (idx * SMMUV3_EVENT_SIZE);
    uint8_t evt[SMMUV3_EVENT_SIZE] = { 0 };

    evt[0] = type;
    *reinterpret_cast<uint32_t*>(evt + 4) = sid;
    *reinterpret_cast<uint64_t*>(evt + 8) = iova;
    *reinterpret_cast<uint32_t*>(evt + 16) = info;

    if (!dma_write(evt_addr, evt, SMMUV3_EVENT_SIZE)) {
        set_gerror(2);
        return;
    }

    prod = (prod + 1) & ((1u << (log2size + 1)) - 1);
    EVENTQ_PROD = prod;

    if (static_cast<uint32_t>(IRQ_CTRL[IRQ_CTRL_EVENTQ_IRQEN])) {
        trigger_irq(irq_eventq);
    }
}

// PRI recording
template <unsigned int BUSWIDTH>
void smmuv3<BUSWIDTH>::record_pri(uint32_t sid, uint64_t iova, uint32_t flags)
{
    if (static_cast<uint32_t>(CR0[CR0_PRIQEN]) == 0) {
        return;
    }

    uint64_t base = (static_cast<uint64_t>(PRIQ_BASE_HI) << 32) |
                    (static_cast<uint64_t>(PRIQ_BASE_LO) & ~((1ULL << 5) - 1));
    uint32_t log2size = static_cast<uint32_t>(PRIQ_BASE_LO[PRIQ_BASE_LOG2SIZE]);
    uint32_t prod = static_cast<uint32_t>(PRIQ_PROD);
    uint32_t cons = static_cast<uint32_t>(PRIQ_CONS);

    if (queue_full(prod, cons, log2size)) {
        set_gerror(3); // PRIQ_ABT_ERR
        return;
    }

    uint32_t idx = prod & ((1u << log2size) - 1);
    uint64_t pri_addr = base + (idx * SMMUV3_PRI_SIZE);
    uint8_t pri[SMMUV3_PRI_SIZE] = { 0 };

    *reinterpret_cast<uint32_t*>(pri) = sid;
    *reinterpret_cast<uint64_t*>(pri + 8) = iova;
    *reinterpret_cast<uint32_t*>(pri + 4) = flags;

    if (!dma_write(pri_addr, pri, SMMUV3_PRI_SIZE)) {
        set_gerror(3);
        return;
    }

    prod = (prod + 1) & ((1u << (log2size + 1)) - 1);
    PRIQ_PROD = prod;

    if (static_cast<uint32_t>(IRQ_CTRL[IRQ_CTRL_PRI_IRQEN])) {
        trigger_irq(irq_priq);
    }
}

// Command handlers (stubs for now)
template <unsigned int BUSWIDTH>
void smmuv3<BUSWIDTH>::handle_cmd_prefetch_config(const uint8_t*)
{
}

template <unsigned int BUSWIDTH>
void smmuv3<BUSWIDTH>::handle_cmd_prefetch_addr(const uint8_t*)
{
}

template <unsigned int BUSWIDTH>
void smmuv3<BUSWIDTH>::handle_cmd_cfgi_ste(const uint8_t* cmd)
{
    uint32_t sid = *reinterpret_cast<const uint32_t*>(cmd + 4);
    m_ste_cache.erase(sid);
}

template <unsigned int BUSWIDTH>
void smmuv3<BUSWIDTH>::handle_cmd_cfgi_ste_range(const uint8_t*)
{
    m_ste_cache.clear();
}

template <unsigned int BUSWIDTH>
void smmuv3<BUSWIDTH>::handle_cmd_cfgi_cd(const uint8_t*)
{
    m_cd_cache.clear();
}

template <unsigned int BUSWIDTH>
void smmuv3<BUSWIDTH>::handle_cmd_cfgi_cd_all(const uint8_t*)
{
    m_cd_cache.clear();
}

template <unsigned int BUSWIDTH>
void smmuv3<BUSWIDTH>::handle_cmd_cfgi_all(const uint8_t*)
{
    m_ste_cache.clear();
    m_cd_cache.clear();
}

template <unsigned int BUSWIDTH>
void smmuv3<BUSWIDTH>::handle_cmd_tlbi_nh_all(const uint8_t*)
{
    iotlb_inv_all();
}

template <unsigned int BUSWIDTH>
void smmuv3<BUSWIDTH>::handle_cmd_tlbi_nh_asid(const uint8_t* cmd)
{
    // SMMUv3 command DW0 packs ASID in bits[63:48] (bytes 6-7).
    uint16_t asid = *reinterpret_cast<const uint16_t*>(cmd + 6);
    iotlb_inv_asid(asid);
}

template <unsigned int BUSWIDTH>
void smmuv3<BUSWIDTH>::handle_cmd_tlbi_nh_va(const uint8_t* cmd)
{
    uint16_t asid = *reinterpret_cast<const uint16_t*>(cmd + 6);
    uint64_t va = *reinterpret_cast<const uint64_t*>(cmd + 8);
    uint8_t tg = cmd[15] & 0x3;
    uint8_t scale = (cmd[15] >> 4) & 0x1F;

    // Calculate mask from scale (log2 of page size)
    uint32_t page_shift = 12; // Base 4KB
    if (tg == 1)
        page_shift = 16; // 64KB
    else if (tg == 2)
        page_shift = 14; // 16KB

    uint32_t mask = ((1ULL << (page_shift + scale)) - 1);
    iotlb_inv_iova(asid, va, tg, mask);
}

template <unsigned int BUSWIDTH>
void smmuv3<BUSWIDTH>::handle_cmd_tlbi_nh_vaa(const uint8_t* cmd)
{
    // Same as TLBI_NH_VA but invalidates for all ASIDs
    uint64_t va = *reinterpret_cast<const uint64_t*>(cmd + 8);
    uint8_t tg = cmd[15] & 0x3;
    uint8_t scale = (cmd[15] >> 4) & 0x1F;

    uint32_t page_shift = 12;
    if (tg == 1)
        page_shift = 16;
    else if (tg == 2)
        page_shift = 14;

    uint32_t mask = ((1ULL << (page_shift + scale)) - 1);
    uint64_t va_masked = va & ~mask;

    for (auto it = m_iotlb.begin(); it != m_iotlb.end();) {
        uint64_t entry_masked = it->first.iova & ~mask;
        if (entry_masked == va_masked && it->first.tg == tg) {
            m_iotlb_lru.remove(it->first);
            it = m_iotlb.erase(it);
        } else {
            ++it;
        }
    }
}

template <unsigned int BUSWIDTH>
void smmuv3<BUSWIDTH>::handle_cmd_tlbi_s12_vmall(const uint8_t* cmd)
{
    uint16_t vmid = *reinterpret_cast<const uint16_t*>(cmd + 2);
    iotlb_inv_vmid(vmid);
}

template <unsigned int BUSWIDTH>
void smmuv3<BUSWIDTH>::handle_cmd_tlbi_s2_ipa(const uint8_t* cmd)
{
    // CMD_TLBI_S2_IPA: [63:12] = IPA, [11:0] = VMID
    uint16_t vmid = *reinterpret_cast<const uint16_t*>(cmd + 2);
    uint64_t ipa = *reinterpret_cast<const uint64_t*>(cmd + 8);
    uint8_t tg = cmd[15] & 0x3;
    uint8_t scale = (cmd[15] >> 4) & 0x1F;

    uint32_t page_shift = 12;
    if (tg == 1)
        page_shift = 16;
    else if (tg == 2)
        page_shift = 14;

    uint32_t mask = ((1ULL << (page_shift + scale)) - 1);
    uint64_t ipa_masked = ipa & ~mask;

    for (auto it = m_iotlb.begin(); it != m_iotlb.end();) {
        uint64_t entry_masked = it->first.iova & ~mask;
        if (it->first.vmid == vmid && entry_masked == ipa_masked && it->first.tg == tg) {
            m_iotlb_lru.remove(it->first);
            it = m_iotlb.erase(it);
        } else {
            ++it;
        }
    }
}

template <unsigned int BUSWIDTH>
void smmuv3<BUSWIDTH>::handle_cmd_tlbi_nsnh_all(const uint8_t*)
{
    iotlb_inv_all();
}

template <unsigned int BUSWIDTH>
void smmuv3<BUSWIDTH>::handle_cmd_sync(const uint8_t* cmd)
{
    // CS field is at bits 13:12 of word[0].
    // CS=0 (NONE), CS=1 (IRQ), CS=2 (SEV)
    uint32_t word0 = cmd[0] | (cmd[1] << 8) | (cmd[2] << 16) | (cmd[3] << 24);
    uint32_t cs = (word0 >> 12) & 0x3;
    if (cs == 1) { // SIG_IRQ
        trigger_irq(irq_cmd_sync);
    }
    // CS=0 (NONE) and CS=2 (SEV) don't need extra action - software polls CMDQ_CONS
}

template <unsigned int BUSWIDTH>
void smmuv3<BUSWIDTH>::handle_cmd_resume(const uint8_t* cmd)
{
    uint32_t stag = *reinterpret_cast<const uint32_t*>(cmd + 4);
    uint8_t resp = cmd[11];

    auto it = m_stalled_txns.find(stag);
    if (it != m_stalled_txns.end()) {
        if (resp != 0) {
            it->second.aborted = true;
        }
        it->second.resume_event.notify();
    }
}

template <unsigned int BUSWIDTH>
void smmuv3<BUSWIDTH>::handle_cmd_pri_resp(const uint8_t*)
{
    // Advance PRI consumer pointer
}

// GATOS
template <unsigned int BUSWIDTH>
void smmuv3<BUSWIDTH>::do_gatos()
{
    GATOS_CTRL[GATOS_CTRL_INPRG] = 1;

    uint32_t sid = static_cast<uint32_t>(GATOS_SID);
    uint64_t iova = (static_cast<uint64_t>(GATOS_ADDR_HI) << 32) | static_cast<uint64_t>(GATOS_ADDR_LO);
    bool wr = static_cast<uint32_t>(GATOS_CTRL[GATOS_CTRL_READ_NWRITE]) == 0;

    tlm::tlm_generic_payload dummy_txn;
    dummy_txn.set_command(wr ? tlm::TLM_WRITE_COMMAND : tlm::TLM_READ_COMMAND);
    dummy_txn.set_address(iova);

    IOMMUTLBEntry te = smmuv3_translate(dummy_txn, sid, 0);

    if (te.perm == IOMMU_NONE) {
        GATOS_PAR_LO[GATOS_PAR_F] = 1;
        GATOS_PAR_LO[GATOS_PAR_FST] = 0x10; // Translation fault
    } else {
        GATOS_PAR_LO = static_cast<uint32_t>(te.translated_addr);
        GATOS_PAR_HI = static_cast<uint32_t>(te.translated_addr >> 32);
        GATOS_PAR_LO[GATOS_PAR_F] = 0;
    }

    GATOS_CTRL[GATOS_CTRL_RUN] = 0;
    GATOS_CTRL[GATOS_CTRL_INPRG] = 0;
}

} // namespace gs

#endif // SMMUV3_H
