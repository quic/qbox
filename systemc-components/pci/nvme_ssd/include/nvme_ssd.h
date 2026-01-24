/*
 * Copyright (c) 2025 Qualcomm Innovation Center, Inc. All Rights Reserved.
 * Author: jhieb@micron.com 2025
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _GREENSOCS_BASE_COMPONENTS_NVME_SSD_H
#define _GREENSOCS_BASE_COMPONENTS_NVME_SSD_H


#include <fstream>
#include <memory>

#include <systemc>
#include <scp/report.h>
#include <scp/helpers.h>
#include <module_factory_registery.h>
#include <tlm_sockets_buswidth.h>
#include <unordered_map>

#include "tlm-modules/pcie-controller.h"
#include "nvme_controller.h"

#include <gs_gpex.h>

#ifndef _WIN32
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#endif

#include "pci_ids.h"

#define QDMA_TYPE qdma_cpm5

#define PCI_VENDOR_ID_SAMSUNG		   (0x144D)
#define PCI_SUBSYSTEM_VENDOR_ID_INTEL  (0x3704) // 2E Intel 750 series nvme ssd
#define PCI_SUBSYSTEM_ID_INTEL         (0x8086)  // Intel corporation
#define PCI_DEVICE_ID_XILINX_EF100	   (0xd004)
#define PCI_SUBSYSTEM_ID_XILINX_TEST   (0x000A)

#define PCI_CLASS_BASE_MASS_STORAGE_CONTROLLER (0x01)
#define PCI_CLASS_BASE_NETWORK_CONTROLLER      (0x02)

#ifndef PCI_EXP_LNKCAP_ASPM_L0S
#define PCI_EXP_LNKCAP_ASPM_L0S 0x00000400 /* ASPM L0s Support */
#endif

#define KiB (1024)
#define RAM_SIZE (4 * KiB)

#define NR_MMIO_BAR  6
#define NR_IRQ       NR_QDMA_IRQ


PhysFuncConfig getSSDPhysFuncConfig(){
	PhysFuncConfig cfg;
	PMCapability pmCap;
	PCIExpressCapability pcieCap;
	MSIXCapability msixCap;

	// 32 bit BARs.  With BAR 0,4,5 it freezes with 64 bit BARs??
	uint32_t bar_flags = PCI_BASE_ADDRESS_MEM_TYPE_32;
	uint32_t msixTableSz = 16;  // 512
	// TODO(jhieb) how do i set the table offset and PBA correctly so the PBA and msix table are on different BARs?
	// TODO(jhieb) the bar5 doesn't get set as a MSIX bar unless the table is on it.
	uint32_t tableOffset = 0x0 | 4; // Table offset: 0, BIR: 4
	uint32_t pba = 0x2000 | 4; // Table offset: 8192, BIR: 4

	cfg.SetPCIVendorID(PCI_VENDOR_ID_SAMSUNG);
	// QDMA
	cfg.SetPCIDeviceID(0x2001);

	cfg.SetPCIClassProgIF(0x02);
	cfg.SetPCIClassDevice(0x08);
	cfg.SetPCIClassBase(PCI_CLASS_BASE_MASS_STORAGE_CONTROLLER);

	cfg.SetPCIBAR0(8 * KiB, bar_flags);
	cfg.SetPCIBAR4(16 * KiB, bar_flags);
	// TODO(jhieb) the bar5 doesn't get set as a MSIX bar unless the table is on it.
	//cfg.SetPCIBAR5(4 * KiB, bar_flags);

	cfg.SetPCISubsystemVendorID(PCI_SUBSYSTEM_VENDOR_ID_INTEL);
	cfg.SetPCISubsystemID(PCI_SUBSYSTEM_ID_INTEL);
	cfg.SetPCIExpansionROMBAR(0, 0);
	
	// TODO(jhieb) seeing if disabling this gets me closer to the right setup?
	pmCap.SetCapabilityID(PCI_CAP_ID_PM);
	pmCap.SetNextCapPtr(0xC0);
	pmCap.SetPMCapabilities(0x02);
	cfg.AddPCICapability(pmCap);

	// TODO(jhieb) since the capability size/start isn't DWORD aligned had to modify pf-config.h line 108 to this from 0x8.
	//			PCIExpressCapSize = PCI_CAP_EXP_ENDPOINT_SIZEOF_V2 + 0xA,
	pcieCap.SetCapabilityID(PCI_CAP_ID_EXP);
	//pcieCap.SetNextCapPtr(0xBC);
	pcieCap.SetDeviceCapabilities(PCI_EXP_DEVCAP_FLR | PCI_EXP_DEVCAP_RBER);
	pcieCap.SetLinkCapabilities(0x0);
	pcieCap.SetLinkStatus(PCI_EXP_LNKSTA_CLS_2_5GB);
	pcieCap.SetDeviceControl(0x0);
	pcieCap.SetDeviceStatus(0x0);
	pcieCap.SetLinkStatus(0x1);
	pcieCap.SetLinkControl(0x0);
	pcieCap.SetPCIECapabilities(0x0002);
	cfg.AddPCICapability(pcieCap);

	//msixCap.
	msixCap.SetCapabilityID(PCI_CAP_ID_MSIX);
	//msixCap.SetNextCapPtr(0x00);
	msixCap.SetMessageControl(msixTableSz);
	msixCap.SetTableOffsetBIR(tableOffset);
	msixCap.SetPendingBitArray(pba);
	cfg.AddPCICapability(msixCap);

	return cfg;
}

namespace gs {


template<typename QDMA_t>
class pcie_versal : public pci_device_base
{
private:
	QDMA_t qdma;

	// BARs towards the QDMA
	tlm_utils::simple_initiator_socket<pcie_versal> user_bar_init_socket;
	tlm_utils::simple_initiator_socket<pcie_versal> cfg_init_socket;

	// QDMA towards PCIe interface (host)
	tlm_utils::simple_target_socket<pcie_versal> brdg_dma_tgt_socket;

	// MSI-X propagation
	sc_vector<sc_signal<bool> > signals_irq;


	// TODO(jhieb) this is a hack till i figure out the iconnect/memory part.
	tlm_utils::simple_target_socket<pcie_versal> card_bus;

	//
	// Nothing to attach to the QDMA yet, just add a dummy memory.
	// With that the testcase will be able to check what has been
	// written in the memory..  Add an interconnect, so we can map
	// it anywhere.
	//
	// TODO(jhieb) evaluate how we fix this for DMA and what not.
	//iconnect<1, 1> bus;
	//memory sbi_dummy;

	void bar_b_transport(int bar_nr, tlm::tlm_generic_payload &trans,
				sc_time &delay)
	{
		switch (bar_nr) {
			case QDMA_USER_BAR_ID:
				user_bar_init_socket->b_transport(trans, delay);
				break;
			case 0:
				cfg_init_socket->b_transport(trans, delay);
				break;
			default:
				SC_REPORT_ERROR("pcie_versal",
					"writing to an unimplemented bar");
				trans.set_response_status(
					tlm::TLM_GENERIC_ERROR_RESPONSE);
				break;
		}
	}

	//
	// Forward DMA requests received from the CPM5 QDMA
	//
	void fwd_dma_b_transport(tlm::tlm_generic_payload& trans,
					sc_time& delay)
	{
		dma->b_transport(trans, delay);
	}

	//
	// MSI-X propagation
	//
	void irq_thread(unsigned int i)
	{
		while (true) {
			wait(signals_irq[i].value_changed_event());
			irq[i].write(signals_irq[i].read());
		}
	}

public:
	SC_HAS_PROCESS(pcie_versal);

	pcie_versal(sc_core::sc_module_name name) :

		pci_device_base(name, NR_MMIO_BAR, NR_IRQ),

		qdma("qdma"),

		user_bar_init_socket("user_bar_init_socket"),
		cfg_init_socket("cfg_init_socket"),
		brdg_dma_tgt_socket("brdg-dma-tgt-socket"),

		signals_irq("signals_irq", NR_IRQ)

		//bus("bus"),
		//sbi_dummy("sbi_dummy", sc_time(0, SC_NS), RAM_SIZE)
	{
		//
		// QDMA connections
		//
		user_bar_init_socket.bind(qdma.user_bar);
		cfg_init_socket.bind(qdma.config_bar);

		// Setup DMA forwarding path (qdma.dma -> upstream to host)
		qdma.dma.bind(brdg_dma_tgt_socket);
		brdg_dma_tgt_socket.register_b_transport(
			this, &pcie_versal::fwd_dma_b_transport);

		// TODO(jhieb) evaluate fixes to the memory mapping.
		// Connect the SBI dummy RAM
		//bus.memmap(0x102100000ULL, 0x1000 - 1,
		//	   ADDRMODE_RELATIVE, -1, sbi_dummy.socket);
		//qdma.card_bus.bind((*bus.t_sk[0]));
		// TODO(jhieb) hack to bind the card_bus for now.
		qdma.card_bus.bind(card_bus);

		// Setup MSI-X propagation
		for (unsigned int i = 0; i < NR_IRQ; i++) {
			qdma.irq[i](signals_irq[i]);
			sc_spawn(sc_bind(&pcie_versal::irq_thread, this, i));
		}
	}

	void card_bus_b_transport(tlm::tlm_generic_payload& trans, sc_time& delay){
		// TODO(jhieb) hack for binding card bus.
		delay = sc_core::sc_time(1, sc_core::SC_NS);
	}

	void rst(sc_signal<bool>& rst)
	{
		qdma.rst(rst);
	}
};


/**
 * @class nvme_ssd
 *
 * @brief A nvme_ssd PCI based nvme storage device.
 *
 * @details TODO(jhieb) document
 */
template <unsigned int BUSWIDTH = DEFAULT_TLM_BUSWIDTH>
class nvme_ssd : public sc_core::sc_module
{
    SCP_LOGGER(());
    
public:

    std::shared_ptr<PCIeController> pcie_ctlr;
	pcie_versal<QDMA_TYPE> nvme_ctlr;
	sc_signal<bool> rst;

    nvme_ssd(const sc_core::sc_module_name& name, sc_core::sc_object* gpex)
        : nvme_ssd(name, *(dynamic_cast<gs_gpex<BUSWIDTH>*>(gpex)))
    {
    }

    nvme_ssd(sc_core::sc_module_name name, gs_gpex<BUSWIDTH>& gpex) :
        pcie_ctlr(std::make_shared<PCIeController>("pcie_ctlr", getSSDPhysFuncConfig())),
		nvme_ctlr("nvme_ctlr")
    {
		//pcie_ctlr = std::make_shared<PCIeController>("pcie_ctlr", getSSDPhysFuncConfig());
		//nvme_ctlr = std::make_shared<qdma>("nvme_ctlr");
        SCP_DEBUG(()) << "nvme_ssd constructor";

		pcie_ctlr->bind(nvme_ctlr);
		nvme_ctlr.rst(rst);
		// TODO(jhieb) create a PCI base class that we pass to gpex?
		gpex.add_device(pcie_ctlr);

		SC_THREAD(pull_reset);
    }

	void pull_reset(void) {
		/* Pull the reset signal.  */
		rst.write(true);
		wait(1, SC_US);
		rst.write(false);
	}

    void before_end_of_elaboration()
    {
    }

    nvme_ssd() = delete;
    nvme_ssd(const nvme_ssd&) = delete;

    ~nvme_ssd() {}
};
} // namespace gs

extern "C" void module_register();
#endif
