#ifndef __HEXAGON_CLUSTER__
#define __HEXAGON_CLUSTER__

#include <systemc>
#include <cci_configuration>
#include <greensocs/libgsutils.h>
#include <greensocs/gsutils/module_factory_registery.h>
#include <libqbox/components/cpu/hexagon/hexagon.h>
#include <greensocs/base-components/router.h>
#include <greensocs/base-components/memory.h>
#include <greensocs/base-components/pass.h>
#include <libqbox/components/irq-ctrl/hexagon-l2vic.h>
#include <libqbox/components/timer/hexagon-qtimer.h>
#include <libqbox-extra/components/meta/global_peripheral_initiator.h>
#include <libqbox/qemu-instance.h>
#include <quic/csr/csr.h>
#include "wdog.h"
#include "pll.h"

class hexagon_cluster : public sc_core::sc_module
{
    SCP_LOGGER();
public:
    cci::cci_param<unsigned> p_hexagon_num_threads;
    cci::cci_param<uint32_t> p_hexagon_start_addr;
    cci::cci_param<bool> p_isdben_secure;
    cci::cci_param<bool> p_isdben_trusted;
    cci::cci_param<uint32_t> p_cfgbase;
    cci::cci_param<unsigned> p_vtcm_num;

    // keep public so that smmu can be associated with hexagon
    QemuInstance& m_qemu_hex_inst;

private:
    cci::cci_broker_handle m_broker;
    QemuHexagonL2vic m_l2vic;
    QemuHexagonQtimer m_qtimer;
    WDog<> m_wdog;
    sc_core::sc_vector<pll<>> m_plls;
    //    pll<> m_pll2;
    csr m_csr;
    gs::Memory<> m_rom;
    sc_core::sc_vector<gs::Memory<>> m_vtcm_mem;

    sc_core::sc_vector<QemuCpuHexagon> m_hexagon_threads;
    GlobalPeripheralInitiator m_global_peripheral_initiator_hex;
    gs::Memory<> m_fallback_mem;
    
    public:
    // gs::Router<> parent_router;
    gs::Router<> m_router;
    hexagon_cluster(const sc_core::sc_module_name& name, sc_core::sc_object* o): hexagon_cluster(name, *(dynamic_cast<QemuInstance*>(o))) {}
    hexagon_cluster(const sc_core::sc_module_name& n, QemuInstance& qemu_hex_inst)
        : sc_core::sc_module(n)
        , m_broker(cci::cci_get_broker())
        , p_hexagon_num_threads("hexagon_num_threads", 8, "Number of Hexagon threads")
        , p_hexagon_start_addr("hexagon_start_addr", 0x100, "Hexagon execution start address")
        , p_isdben_trusted("isdben_trusted", false, "Value of ISDBEN.TRUSTED reg field")
        , p_isdben_secure("isdben_secure", false, "Value of ISDBEN.SECURE reg field")
        , p_cfgbase("cfgtable_base", 0, "config table base address")
        , p_vtcm_num("vtcm_num", 0, "number of VTCM memories")
        , m_qemu_hex_inst(qemu_hex_inst)
        , m_l2vic("l2vic", m_qemu_hex_inst)
        , m_qtimer("qtimer",
                   m_qemu_hex_inst) // are we sure it's in the hex cluster?????
        , m_wdog("wdog")
        , m_plls("pll", 1)
        //        , m_pll2("pll2")
        , m_csr("csr")
        , m_rom("rom")
        , m_vtcm_mem("vtcm", p_vtcm_num)
        , m_hexagon_threads("hexagon_thread", p_hexagon_num_threads,
                            [this](const char* n, size_t i) {
                                /* here n is already "hexagon-cpu_<vector-index>" */
                                uint64_t l2vic_base = gs::cci_get<uint64_t>(cci::cci_get_broker(),
                                                                            std::string(name()) + ".l2vic.mem.address");

                                uint64_t qtmr_rg0 = gs::cci_get<uint64_t>(
                                    cci::cci_get_broker(), std::string(name()) + ".qtimer.mem_view.address");
                                return new QemuCpuHexagon(n, m_qemu_hex_inst, p_cfgbase, QemuCpuHexagon::v68_rev,
                                                          l2vic_base, qtmr_rg0, p_hexagon_start_addr);
                            })
        , m_global_peripheral_initiator_hex("glob-per-init-hex", qemu_hex_inst, m_hexagon_threads[0])
        , m_fallback_mem("fallback_memory")
        // , parent_router("parent_router")
        , m_router("router")
    {
        m_router.initiator_socket.bind(m_l2vic.socket);
        m_router.initiator_socket.bind(m_l2vic.socket_fast);
        m_router.initiator_socket.bind(m_qtimer.socket);
        m_router.initiator_socket.bind(m_qtimer.view_socket);
        m_router.initiator_socket.bind(m_wdog.socket);
        for (auto& pll : m_plls) {
            m_router.initiator_socket.bind(pll.socket);
        }
        
        m_router.initiator_socket.bind(m_rom.socket);
        
        for(auto& vtcm : m_vtcm_mem){
            m_router.initiator_socket.bind(vtcm.socket);
        }

        m_router.initiator_socket.bind(m_csr.socket);
        m_csr.hex_halt.bind(m_hexagon_threads[0].halt);

        for (auto& cpu : m_hexagon_threads) {
            cpu.socket.bind(m_router.target_socket);
        }

        for (int i = 0; i < m_l2vic.p_num_outputs; ++i) {
            m_l2vic.irq_out[i].bind(m_hexagon_threads[0].irq_in[i]);
        }

        m_qtimer.irq[0].bind(m_l2vic.irq_in[2]); // FIXME: Depends on static boolean
                                                 // syscfg_is_linux, may be 2
        m_qtimer.irq[1].bind(m_l2vic.irq_in[3]);

        m_global_peripheral_initiator_hex.m_initiator.bind(m_router.target_socket);

        m_router.initiator_socket.bind(m_fallback_mem.socket);
    }
};
GSC_MODULE_REGISTER(hexagon_cluster, sc_core::sc_object*);
#endif