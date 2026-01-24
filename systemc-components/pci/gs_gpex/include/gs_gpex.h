/*
 * Copyright (c) 2025 Qualcomm Innovation Center, Inc. All Rights Reserved.
 * Author: jhieb@micron.com 2025
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _GREENSOCS_BASE_COMPONENTS_GPEX_H
#define _GREENSOCS_BASE_COMPONENTS_GPEX_H


#include <fstream>
#include <memory>

#include <cci_configuration>
#include <systemc>
#include <tlm>
#include <tlm_utils/multi_passthrough_target_socket.h>
#include <tlm_utils/multi_passthrough_initiator_socket.h>

#include <scp/report.h>
#include <scp/helpers.h>

#include <loader.h>
#include <memory_services.h>
#include <cci_configuration>

#include <tlm-extensions/shmem_extension.h>
#include <module_factory_registery.h>
#include <tlm_sockets_buswidth.h>
#include <unordered_map>

#include "tlm-modules/pcie-controller.h"

#include "tlm-extensions/pcie_extension.h"

#include "pci_regs.h"
#include "pci_ids.h"
#include "pci_tlps.h"
/* Size of the standard PCI config space */
#define PCI_CONFIG_SPACE_SIZE 0x1000

namespace gs {

/**
 * @class gs_gpex
 *
 * @brief A Generic PCI host root component used for connecting SystemC PCI devices to.
 *
 * @details handles MMIO and ECAM operations and routing to child PCI objects.
 *          Will handle responses and DMA accesses upstream (including MSIX).
 *          Currently has to translate systemC transactions from router into
 *          TLP format (big endian) to be sent to the PCI controller(s).
 * 
 *          Currently using the xilinx pcie tlm model: https://github.com/Xilinx/pcie-model
 */
template <unsigned int BUSWIDTH = DEFAULT_TLM_BUSWIDTH>
class gs_gpex : public sc_core::sc_module
{

    SCP_LOGGER(());

private:
    uint8_t config[PCI_CONFIG_SPACE_SIZE] = {};
    std::vector<std::unique_ptr<sc_event>> e_wait_for_devices_resp;
    std::vector<tlm::tlm_generic_payload*> dev_responses;
    cci::cci_broker_handle m_broker;
    uint8_t next_tag = 0;
    // TODO(jhieb) any potential for overflow conditions with tag tracking?
    std::unordered_map<uint8_t, int> tag_to_device_map;  // Maps tag -> device_id

    // BAR tracking structure
    struct BARInfo {
        uint64_t base_addr;
        uint64_t size;
        bool is_64bit;
        bool is_io;
        bool enabled;
        uint8_t bar_index;  // 0-5
    };

    struct DeviceBARs {
        uint8_t bus;
        uint8_t device;
        uint8_t function;
        std::vector<BARInfo> bars;  // Up to 6 BARs per device
    };

    std::vector<DeviceBARs> device_bar_map;

protected:

    int find_device_by_mmio_addr(uint64_t addr) {
        for (size_t dev_idx = 0; dev_idx < device_bar_map.size(); dev_idx++) {
            for (const auto& bar : device_bar_map[dev_idx].bars) {
                if (bar.enabled && !bar.is_io) {
                    uint64_t bar_end = bar.base_addr + bar.size - 1;
                    if (addr >= bar.base_addr && addr <= bar_end) {
                        return dev_idx;
                    }
                }
            }
        }
        return -1;  // Not found
    }

    // Helper to update BAR information from ECAM write
    void update_bar_mapping(uint8_t bus, uint8_t device, uint8_t function, 
                           uint8_t bar_index, uint32_t value) {
        // Find or create device entry
        DeviceBARs* dev_bars = nullptr;
        for (auto& dev : device_bar_map) {
            if (dev.bus == bus && dev.device == device && dev.function == function) {
                dev_bars = &dev;
                break;
            }
        }

        if (!dev_bars) {
            device_bar_map.push_back({bus, device, function, std::vector<BARInfo>(6)});
            dev_bars = &device_bar_map.back();
            if(device > devices.size()){
                SCP_FATAL(()) << "Device index is out of range.";
            }
            std::shared_ptr<PCIeController> pci_device = devices[device-1];
            PhysFuncConfig dev_cfg = pci_device->GetPFConfig();
            std::vector<PCIBARConfig> bars_config = dev_cfg.GetBarConfigs();
            // Could also watch the ecam reads for the BAR size.  Currently just reading the BAR size directly.
            uint32_t bar_num;
            for(size_t i = 0; i < bars_config.size(); i++){
                bar_num = bars_config[i].GetBARNum();
                // In some cases there's a strange BAR in the config list?
                if(bar_num < dev_bars->bars.size()){
                    dev_bars->bars[bar_num].size = bars_config[i].GetSize();
                }
            }
        }
        BARInfo& bar = dev_bars->bars[bar_index];

        // Check if this is a 64-bit BAR (bit 2:1 = 10b)
        bool is_64bit = ((value & 0x6) == 0x4);
        bool is_io = (value & 0x1);

        if (bar_index % 2 == 0) {  // Even BAR index (or low 32-bits of 64-bit BAR)
            bar.base_addr = value & (is_io ? 0xFFFFFFFC : 0xFFFFFFF0);
            bar.is_64bit = is_64bit;
            bar.is_io = is_io;
            bar.bar_index = bar_index;
            bar.enabled = (value != 0 && value != 0xFFFFFFFF);

            // For 64-bit BARs, mark the next BAR as part of this one
            if (is_64bit && bar_index < 5) {
                dev_bars->bars[bar_index + 1].enabled = false;  // Upper 32-bits handled separately
            }
        } else if (bar_index > 0 && dev_bars->bars[bar_index - 1].is_64bit) {
            // This is the upper 32-bits of a 64-bit BAR
            BARInfo& lower_bar = dev_bars->bars[bar_index - 1];
            lower_bar.base_addr |= (static_cast<uint64_t>(value) << 32);
        }

        SCP_INFO() << "Updated BAR" << (int)bar_index 
                   << " for device " << (int)device 
                   << ": addr=0x" << std::hex << bar.base_addr
                   << " 64bit=" << bar.is_64bit
                   << " IO=" << bar.is_io;
    }

    void get_bdf_from_address(uint64_t addr, uint8_t &bus, uint8_t &device, uint8_t &function){
        // BDF in ECAM address: ECAM_BASE + (Bus << 20) + (Device << 15) + (Function << 12)
        bus = (addr >> 20) & 0xff;
        device = (addr >> 15) & 0x1f;
        function = (addr >> 12) & 0x7;
    }

    void b_transport_ecam(int id, tlm::tlm_generic_payload& txn, sc_core::sc_time& delay){
        unsigned int len = txn.get_data_length();
        unsigned char* ptr = txn.get_data_ptr();
        sc_dt::uint64 addr = txn.get_address();
        unsigned char* byt = txn.get_byte_enable_ptr();
        unsigned int bel = txn.get_byte_enable_length();
        uint8_t bus,device,function;
        
        switch (txn.get_command()) {
            case tlm::TLM_READ_COMMAND:
                handle_ecam_read(txn, delay);
                break;
            case tlm::TLM_WRITE_COMMAND:
                handle_ecam_write(txn, delay);
                break;
            default:
                SCP_FATAL(()) << "TLM command not supported";
                break;
        }
    }

    void handle_ecam_write(tlm::tlm_generic_payload& txn, sc_core::sc_time& delay){
        unsigned int len = txn.get_data_length();
        unsigned char* ptr = txn.get_data_ptr();
        sc_dt::uint64 addr = txn.get_address();
        unsigned char* byt = txn.get_byte_enable_ptr();
        unsigned int bel = txn.get_byte_enable_length();
        uint8_t bus,device,function,tag;
        
        if(addr < PCI_CONFIG_SPACE_SIZE){
            memcpy(config+addr, ptr, len);
        } else {
            get_bdf_from_address(addr, bus, device, function);
            uint32_t reg_offset = addr & 0xFFF;
            // TODO(jhieb) ignore the 0xFFFF status write for now??  How is the root supposed to handle status reg writes?
            if(reg_offset == 0x6){
                SCP_INFO() << "PCI STS WRITE";
                get_bdf_from_address(addr, bus, device, function);
                txn.set_response_status(tlm::TLM_OK_RESPONSE);
                return;
            }
            // Check if this is a BAR write (offsets 0x10-0x24)
            if (reg_offset >= PCI_BASE_ADDRESS_0 && reg_offset <= PCI_BASE_ADDRESS_5) {
                uint8_t bar_index = (reg_offset - PCI_BASE_ADDRESS_0) / 4;
                uint32_t bar_value;
                memcpy(&bar_value, ptr, sizeof(uint32_t));
                // Update our BAR tracking
                update_bar_mapping(bus, device, function, bar_index, bar_value);
            }

            if(device <= devices.size()){
                // Need to build out the TLP in the txn data for the pcie controller to process.
                TLPHeader4DW tlp_header;
                // DW0: Format and Type
                tlp_header.dword0.fmt = 0x02;      // 3DW header WITH data (10b)
                tlp_header.dword0.type = 0x04;     // Configuration Write Type 0 (00100b)
                tlp_header.dword0.tc = 0x0;        // Traffic Class 0
                tlp_header.dword0.length = 1;      // Writing 1 DWORD (4 bytes)
                tlp_header.dword0.at = 0x0;        // Address Type: untranslated
                tlp_header.dword0.attr = 0x0;      // No Relaxed Ordering, No Snoop
                tlp_header.dword0.ep = 0;          // Not poisoned
                tlp_header.dword0.td = 0;          // No TLP digest
                tlp_header.dword0.th = 0;          // No processing hints
                tlp_header.dword0.ln = 0;          // Not a lightweight notification
                
                // DW1: Requester ID and Byte Enables
                tlp_header.dword1.first_be = calculate_first_be(addr & 0x3FF, len);
                tlp_header.dword1.last_be = calculate_last_be(addr & 0x3FF, len);   // Single DWORD, so last_be = 0
                tlp_header.dword1.tag = next_tag;      // Transaction tag
                tag = tlp_header.dword1.tag;
                tag_to_device_map[tlp_header.dword1.tag] = device;
                next_tag++;
                tlp_header.dword1.requester_id = 0x0000;  // Root complex ID (Bus 0, Dev 0, Func 0)
    
                // DW2: Target BDF and Register Address
                tlp_header.dword2.bus = bus;
                tlp_header.dword2.device = device;
                tlp_header.dword2.function = function;
                tlp_header.dword2.reg_num = 0;     // Must be 0 for DWORD-aligned access
                tlp_header.dword2.reg_offset = (addr >> 2) & 0x3FF;  // DWORD offset (bits 11:2)
                tlp_header.dword2.reserved = 0;
                

                uint64_t test_log = 0;
                memcpy(&test_log, ptr, std::min(static_cast<size_t>(len), sizeof(uint64_t)));
                SCP_INFO() << "ECAM Root " << get_txn_command_str(txn) << " to address: 0x" << std::hex << addr << " len: 0x" << std::hex << len << " data: 0x" << std::setw(len*2) << std::setfill('0') << test_log;

                // DW3: Align data payload based on byte offset
                uint8_t byte_offset = addr & 0x3;  // Offset within DWORD
                // Zero out the payload first
                memset(&tlp_header.data_payload, 0, sizeof(tlp_header.data_payload));
                
                // Copy data to the correct offset within the DWORD
                uint8_t* payload_ptr = reinterpret_cast<uint8_t*>(&tlp_header.data_payload);
                memcpy(payload_ptr + byte_offset, ptr, len);

                // Convert to big-endian (network byte order) for PCIe wire format
                tlp_header.to_big_endian();

                // Create NEW transaction instead of reusing txn
                tlm::tlm_generic_payload tlp_txn;
                std::vector<uint8_t> tlp_data(16);  // 4DW = 16 bytes
                memcpy(tlp_data.data(), tlp_header.data, 16);

                tlp_txn.set_command(tlm::TLM_WRITE_COMMAND);
                tlp_txn.set_address(addr & 0xFFF);
                tlp_txn.set_data_ptr(tlp_data.data());
                tlp_txn.set_data_length(16);
                tlp_txn.set_streaming_width(16);
                tlp_txn.set_byte_enable_ptr(nullptr);
                tlp_txn.set_dmi_allowed(false);
                tlp_txn.set_response_status(tlm::TLM_INCOMPLETE_RESPONSE);

                pci_device_initiator_socket[device-1]->b_transport(tlp_txn, delay);
                wait(*e_wait_for_devices_resp[device-1]);
                TLPCompletion3DW resp;
                memcpy(resp.data, dev_responses[device-1]->get_data_ptr(), 12); // TODO(Jhieb) data length is wrong sometimes?
                resp.from_big_endian(); // Do a byte swap.
                if(resp.dword2.tag != tag){
                    SCP_FATAL(()) << "Tag to completion tag don't match";
                }
                if(resp.dword1.status != TLPCompletion3DW::SC){
                    txn.set_response_status(tlm::TLM_GENERIC_ERROR_RESPONSE);
                    return;
                }
            }
        }
        txn.set_response_status(tlm::TLM_OK_RESPONSE);
        return;
    }

    void handle_ecam_read(tlm::tlm_generic_payload& txn, sc_core::sc_time& delay){
        unsigned int len = txn.get_data_length();
        unsigned char* ptr = txn.get_data_ptr();
        sc_dt::uint64 addr = txn.get_address();
        unsigned char* byt = txn.get_byte_enable_ptr();
        unsigned int bel = txn.get_byte_enable_length();
        uint8_t bus,device,function,tag;
        
        if(addr < PCI_CONFIG_SPACE_SIZE){
            memcpy(ptr, config+addr, len);
        } else {
            get_bdf_from_address(addr, bus, device, function);
            
            if(device <= devices.size()){
                // Need to build out the TLP in the txn data for the pcie controller to process.
                TLPHeader3DW tlp_header;
                // DW0: Format and Type
                tlp_header.dword0.fmt = 0x00;      // 3DW header, no data
                tlp_header.dword0.type = 0x04;     // Configuration Read Type 0
                tlp_header.dword0.tc = 0x0;        // Traffic Class 0
                tlp_header.dword0.length = 1;      // Requesting 1 DWORD (4 bytes)
                tlp_header.dword0.at = 0x0;        // Address Type: untranslated
                tlp_header.dword0.attr = 0x0;      // No Relaxed Ordering, No Snoop
                tlp_header.dword0.ep = 0;          // Not poisoned
                tlp_header.dword0.td = 0;          // No TLP digest
                tlp_header.dword0.th = 0;          // No processing hints
                tlp_header.dword0.ln = 0;          // Not a lightweight notification
                
                // DW1: Requester ID and Byte Enables
                tlp_header.dword1.first_be = calculate_first_be(addr & 0x3FF, len);  // Enable all 4 bytes (DWORD-aligned read)
                tlp_header.dword1.last_be = calculate_last_be(addr & 0x3FF, len);   // Single DWORD, so last_be = 0
                tlp_header.dword1.tag = next_tag;      // Transaction tag (could increment for tracking)
                tag = tlp_header.dword1.tag;
                tag_to_device_map[tlp_header.dword1.tag] = device;
                next_tag++;
                tlp_header.dword1.requester_id = 0x0000;  // Root complex ID (Bus 0, Dev 0, Func 0)
                
                // DW2: Target BDF and Register Address
                tlp_header.dword2.bus = bus;
                tlp_header.dword2.device = device;
                tlp_header.dword2.function = function;
                tlp_header.dword2.reg_num = 0;     // Must be 0 for DWORD-aligned access
                tlp_header.dword2.reg_offset = (addr >> 2) & 0x3FF;  // DWORD offset (bits 11:2)
                tlp_header.dword2.reserved = 0;
                
                // Convert to big-endian (network byte order) for PCIe wire format
                tlp_header.to_big_endian();

                // Create NEW transaction with properly sized buffer
                tlm::tlm_generic_payload tlp_txn;
                std::vector<uint8_t> tlp_data(12);  // 3DW = 12 bytes
                memcpy(tlp_data.data(), tlp_header.data, 12);
                tlp_txn.set_command(tlm::TLM_READ_COMMAND);
                tlp_txn.set_address(addr & 0xFFF);
                tlp_txn.set_data_ptr(tlp_data.data());
                tlp_txn.set_data_length(12);
                tlp_txn.set_streaming_width(12);
                tlp_txn.set_byte_enable_ptr(nullptr);
                tlp_txn.set_dmi_allowed(false);
                tlp_txn.set_response_status(tlm::TLM_INCOMPLETE_RESPONSE);

                pci_device_initiator_socket[device-1]->b_transport(tlp_txn, delay);
                wait(*e_wait_for_devices_resp[device-1]);
                TLPCompletion3DW resp;
                memcpy(resp.data, dev_responses[device-1]->get_data_ptr(), dev_responses[device-1]->get_data_length());
                resp.from_big_endian(); // Do a byte swap.
                if(resp.dword2.tag != tag){
                    SCP_FATAL(()) << "Tag to completion tag don't match";
                }
                if(resp.dword1.status == TLPCompletion3DW::SC){
                    uint8_t* payload_ptr = reinterpret_cast<uint8_t*>(&resp.data_payload);
                    uint8_t byte_offset = addr & 0x3;
                    memcpy(ptr, payload_ptr + byte_offset, len);       
                    uint64_t test_log = 0;
                    memcpy(&test_log, payload_ptr + byte_offset, std::min(static_cast<size_t>(len), sizeof(uint64_t)));
                    SCP_INFO() << "ECAM Root " << get_txn_command_str(txn) << " to address: 0x" << std::hex << addr << " len: 0x" << std::hex << len << " data: 0x" << std::setw(len*2) << std::setfill('0') << test_log;
                } else {
                    txn.set_response_status(tlm::TLM_GENERIC_ERROR_RESPONSE);
                    return;
                }
            }
        }
        txn.set_response_status(tlm::TLM_OK_RESPONSE);
    }

    void b_transport_mmio(int id, tlm::tlm_generic_payload& txn, sc_core::sc_time& delay){
        unsigned int len = txn.get_data_length();
        unsigned char* ptr = txn.get_data_ptr();
        sc_dt::uint64 addr = txn.get_address();
        unsigned char* byt = txn.get_byte_enable_ptr();
        unsigned int bel = txn.get_byte_enable_length();
        bool is_64bit_addr;
        TLPCompletionWithData resp;

        SCP_INFO() << "Root MMIO " << get_txn_command_str(txn) << " to address: 0x" << std::hex << addr << " len: 0x" << std::hex << len;
        switch (txn.get_command()) {
            case tlm::TLM_READ_COMMAND:
                handle_mmio_read(txn, delay);
                break;
            case tlm::TLM_WRITE_COMMAND:
                handle_mmio_write(txn, delay);
                break;
            default:
                SCP_FATAL(()) << "TLM command not supported";
                break;
        }
    }

    void b_transport_mmio_high(int id, tlm::tlm_generic_payload& txn, sc_core::sc_time& delay){
        unsigned int len = txn.get_data_length();
        unsigned char* ptr = txn.get_data_ptr();
        sc_dt::uint64 addr = txn.get_address();
        unsigned char* byt = txn.get_byte_enable_ptr();
        unsigned int bel = txn.get_byte_enable_length();
        
        SCP_INFO() << "Root MMIO HIGH " << get_txn_command_str(txn) << " to address: 0x" << std::hex << addr << " len: 0x" << std::hex << len;
        // TODO support and update if we end up needing 64 bit BARs.
        txn.set_response_status(tlm::TLM_OK_RESPONSE);
        return;
    }

    void handle_mmio_write(tlm::tlm_generic_payload& txn, sc_core::sc_time& delay){
        unsigned int len = txn.get_data_length();
        unsigned char* ptr = txn.get_data_ptr();
        sc_dt::uint64 addr = txn.get_address();
        unsigned char* byt = txn.get_byte_enable_ptr();
        unsigned int bel = txn.get_byte_enable_length();
        bool is_64bit_addr;
        TLPCompletionWithData resp;
        // Build Memory Write TLP (similar structure)
        is_64bit_addr = (addr > 0xFFFFFFFF);
        
        // Figure out which device BAR that this MMIO address relates to.
        int index = find_device_by_mmio_addr(addr);
        if(index < 0){
            SCP_FATAL(()) << "Couldn't find device bar index.";
        }
        uint32_t device = device_bar_map[index].device;

        // Create NEW transaction with properly sized buffer
        tlm::tlm_generic_payload tlp_txn;
        std::vector<uint8_t> tlp_data;

        if (is_64bit_addr) {
            // 4DW Memory Write with data
            TLPMemoryWrite4DW tlp_header;
            
            tlp_header.dword0.fmt = 0x03;      // 4DW header WITH data (11b)
            tlp_header.dword0.type = 0x00;     // Memory Write (00000b)
            tlp_header.dword0.tc = 0x0;
            tlp_header.dword0.length = (len + 3) / 4;
            tlp_header.dword0.at = 0x0;
            tlp_header.dword0.attr = 0x0;
            tlp_header.dword0.ep = 0;
            tlp_header.dword0.td = 0;
            tlp_header.dword0.th = 0;
            
            uint8_t first_be = calculate_first_be(addr, len);
            uint8_t last_be = calculate_last_be(addr, len);
            
            tlp_header.dword1.first_be = first_be;
            tlp_header.dword1.last_be = last_be;
            tlp_header.dword1.tag = next_tag;
            tag_to_device_map[tlp_header.dword1.tag] = device;
            next_tag++;
            tlp_header.dword1.requester_id = 0x0000;
            
            tlp_header.dword2.addr_high = (addr >> 32) & 0xFFFFFFFF;
            tlp_header.dword3.addr_low = addr & 0xFFFFFFFC;
            
            // Align data payload based on byte offset
            uint8_t byte_offset = addr & 0x3;
            memset(tlp_header.data_payload, 0, sizeof(tlp_header.data_payload));
            uint8_t* payload_ptr = reinterpret_cast<uint8_t*>(tlp_header.data_payload);
            memcpy(payload_ptr + byte_offset, ptr, len);

            tlp_header.to_big_endian();
            
            size_t total_size = 16 + ((len + 3) & ~3);  // Header + padded payload
            tlp_data.resize(total_size);
            memcpy(tlp_data.data(), tlp_header.data, total_size);
            
        } else {
            // 3DW Memory Write with data
            TLPMemoryWrite3DW tlp_header;
            
            tlp_header.dword0.fmt = 0x02;      // 3DW header WITH data (10b)
            tlp_header.dword0.type = 0x00;     // Memory Write (00000b)
            tlp_header.dword0.tc = 0x0;
            tlp_header.dword0.length = (len + 3) / 4;
            tlp_header.dword0.at = 0x0;
            tlp_header.dword0.attr = 0x0;
            tlp_header.dword0.ep = 0;
            tlp_header.dword0.td = 0;
            tlp_header.dword0.th = 0;
            
            uint8_t first_be = calculate_first_be(addr, len);
            uint8_t last_be = calculate_last_be(addr, len);
            
            tlp_header.dword1.first_be = first_be;
            tlp_header.dword1.last_be = last_be;
            tlp_header.dword1.tag = next_tag;
            tag_to_device_map[tlp_header.dword1.tag] = device;
            next_tag++;
            tlp_header.dword1.requester_id = 0x0000;
            
            tlp_header.dword2.addr = addr & 0xFFFFFFFC;
            
            // Align data payload based on byte offset
            uint8_t byte_offset = addr & 0x3;
            memset(tlp_header.data_payload, 0, sizeof(tlp_header.data_payload));
            uint8_t* payload_ptr = reinterpret_cast<uint8_t*>(tlp_header.data_payload);
            memcpy(payload_ptr + byte_offset, ptr, len);
            
            tlp_header.to_big_endian();
            
            size_t total_size = 12 + ((len + 3) & ~3);  // Header + padded payload
            tlp_data.resize(total_size);
            memcpy(tlp_data.data(), tlp_header.data, total_size);
        }
        
        // Setup NEW transaction
        tlp_txn.set_command(tlm::TLM_WRITE_COMMAND);
        tlp_txn.set_address(addr);
        tlp_txn.set_data_ptr(tlp_data.data());
        tlp_txn.set_data_length(tlp_data.size());
        tlp_txn.set_streaming_width(tlp_data.size());
        tlp_txn.set_byte_enable_ptr(nullptr);
        tlp_txn.set_dmi_allowed(false);
        tlp_txn.set_response_status(tlm::TLM_INCOMPLETE_RESPONSE);

        // Send to device (Memory Writes typically don't get completions unless posted)
        pci_device_initiator_socket[device-1]->b_transport(tlp_txn, delay);

        // TODO(jhieb) copy the original status back to the host one.
        txn.set_response_status(tlm::TLM_OK_RESPONSE);
    }

    void handle_mmio_read(tlm::tlm_generic_payload& txn, sc_core::sc_time& delay){
        unsigned int len = txn.get_data_length();
        unsigned char* ptr = txn.get_data_ptr();
        sc_dt::uint64 addr = txn.get_address();
        unsigned char* byt = txn.get_byte_enable_ptr();
        unsigned int bel = txn.get_byte_enable_length();
        bool is_64bit_addr;
        uint8_t tag;
        TLPCompletionWithData resp;
        // Build Memory Read TLP (3DW or 4DW depending on address)
        is_64bit_addr = (addr > 0xFFFFFFFF);
        
        // Figure out which device BAR that this MMIO address relates to.
        int index = find_device_by_mmio_addr(addr);
        if(index < 0){
            SCP_FATAL(()) << "Couldn't find device bar index.";
        }
        uint32_t device = device_bar_map[index].device;

        // Create NEW transaction with properly sized buffer
        tlm::tlm_generic_payload tlp_txn;
        std::vector<uint8_t> tlp_data(16);  // 4DW = 16 bytes
        tlp_txn.set_command(tlm::TLM_READ_COMMAND);
        tlp_txn.set_address(addr);
        tlp_txn.set_data_ptr(tlp_data.data());
        tlp_txn.set_data_length(16);
        tlp_txn.set_streaming_width(16);
        tlp_txn.set_byte_enable_ptr(nullptr);
        tlp_txn.set_dmi_allowed(false);
        tlp_txn.set_response_status(tlm::TLM_INCOMPLETE_RESPONSE);
        tlp_txn.set_address(addr);

        if (is_64bit_addr) {
            // 4DW Memory Read for 64-bit addresses
            TLPMemoryRead4DW tlp_header;
            
            // DW0: Format and Type
            tlp_header.dword0.fmt = 0x01;      // 4DW header, no data (01b)
            tlp_header.dword0.type = 0x00;     // Memory Read (00000b)
            tlp_header.dword0.tc = 0x0;        // Traffic Class 0
            tlp_header.dword0.length = (len + 3) / 4;  // Length in DWORDs (round up)
            tlp_header.dword0.at = 0x0;        // Address Type: untranslated
            tlp_header.dword0.attr = 0x0;      // No Relaxed Ordering, No Snoop
            tlp_header.dword0.ep = 0;          // Not poisoned
            tlp_header.dword0.td = 0;          // No TLP digest
            tlp_header.dword0.th = 0;          // No processing hints
            
            // DW1: Requester ID and Byte Enables
            uint8_t first_be = calculate_first_be(addr, len);
            uint8_t last_be = calculate_last_be(addr, len);
            
            tlp_header.dword1.first_be = first_be;
            tlp_header.dword1.last_be = last_be;
            tlp_header.dword1.tag = next_tag;      // Transaction tag
            tag = tlp_header.dword1.tag;
            tag_to_device_map[tlp_header.dword1.tag] = device;
            next_tag++;
            tlp_header.dword1.requester_id = 0x0000;  // Root complex ID
            
            // DW2-3: 64-bit Address
            tlp_header.dword2.addr_high = (addr >> 32) & 0xFFFFFFFF;
            tlp_header.dword3.addr_low = addr & 0xFFFFFFFC;  // DWORD-aligned (bits [1:0] = 00)
            
            // Convert to big-endian
            tlp_header.to_big_endian();
            
            tlp_txn.set_data_length(16);  // 4DW header
            memcpy(tlp_data.data(), tlp_header.data, 16);
            
        } else {
            // 3DW Memory Read for 32-bit addresses
            TLPMemoryRead3DW tlp_header;
            
            // DW0: Format and Type
            tlp_header.dword0.fmt = 0x00;      // 3DW header, no data (00b)
            tlp_header.dword0.type = 0x00;     // Memory Read (00000b)
            tlp_header.dword0.tc = 0x0;        // Traffic Class 0
            tlp_header.dword0.length = (len + 3) / 4;  // Length in DWORDs (round up)
            tlp_header.dword0.at = 0x0;        // Address Type: untranslated
            tlp_header.dword0.attr = 0x0;      // No Relaxed Ordering, No Snoop
            tlp_header.dword0.ep = 0;          // Not poisoned
            tlp_header.dword0.td = 0;          // No TLP digest
            tlp_header.dword0.th = 0;          // No processing hints
            
            // DW1: Requester ID and Byte Enables
            uint8_t first_be = calculate_first_be(addr, len);
            uint8_t last_be = calculate_last_be(addr, len);
            
            tlp_header.dword1.first_be = first_be;
            tlp_header.dword1.last_be = last_be;
            tlp_header.dword1.tag = next_tag;      // Transaction tag
            tag = tlp_header.dword1.tag;
            tag_to_device_map[tlp_header.dword1.tag] = device;
            next_tag++;
            tlp_header.dword1.requester_id = 0x0000;  // Root complex ID
            
            // DW2: 32-bit Address
            tlp_header.dword2.addr = addr & 0xFFFFFFFC;  // DWORD-aligned (bits [1:0] = 00)
            
            // Convert to big-endian
            tlp_header.to_big_endian();
            
            tlp_txn.set_data_length(12);  // 3DW header
            memcpy(tlp_data.data(), tlp_header.data, 12);
        }
        
        // Send to device
        pci_device_initiator_socket[device-1]->b_transport(tlp_txn, delay);
        
        // Wait for completion
        wait(*e_wait_for_devices_resp[device-1]);
        
        // Parse completion TLP
        memcpy(resp.data, dev_responses[device-1]->get_data_ptr(), dev_responses[device-1]->get_data_length());
        resp.from_big_endian();
        if(resp.dword2.tag != tag){
            SCP_FATAL(()) << "Tag to completion tag don't match";
        }
        if (resp.dword1.status == TLPCompletionWithData::SC) {
            // Extract data from completion
            uint32_t byte_count = resp.dword1.byte_count;
            uint32_t lower_addr = resp.dword2.lower_addr;
            uint8_t* payload_ptr = reinterpret_cast<uint8_t*>(&resp.data_payload);
            uint8_t byte_offset = addr & 0x3;
            // Copy payload data back to original transaction
            memcpy(ptr, payload_ptr + byte_offset, std::min(len, byte_count));

            txn.set_response_status(tlm::TLM_OK_RESPONSE);
        } else {
            SCP_WARN() << "Memory read completion error, status: " << resp.dword1.status;
            txn.set_response_status(tlm::TLM_GENERIC_ERROR_RESPONSE);
            return;
        }
    }

    void b_transport_pio(int id, tlm::tlm_generic_payload& txn, sc_core::sc_time& delay){
        unsigned int len = txn.get_data_length();
        unsigned char* ptr = txn.get_data_ptr();
        sc_dt::uint64 addr = txn.get_address();
        unsigned char* byt = txn.get_byte_enable_ptr();
        unsigned int bel = txn.get_byte_enable_length();
        
        SCP_INFO() << "Root PIO " << get_txn_command_str(txn) << " to address: 0x" << std::hex << addr << " len: 0x" << std::hex << len;
        txn.set_response_status(tlm::TLM_OK_RESPONSE);
        return;
    }

    // Helper to calculate first byte enable
    uint8_t calculate_first_be(uint64_t addr, size_t len) {
        uint8_t offset = addr & 0x3;  // Byte offset within DWORD
        uint8_t bytes_in_first_dword = std::min(size_t(4 - offset), len);
        return ((1 << bytes_in_first_dword) - 1) << offset;
    }
    
    // Helper to calculate last byte enable
    uint8_t calculate_last_be(uint64_t addr, size_t len) {
        if (len <= (4 - (addr & 0x3))) {
            return 0x0;  // Single DWORD access
        }
        uint8_t remaining = (addr + len) & 0x3;
        if (remaining == 0) remaining = 4;
        return (1 << remaining) - 1;
    }

    const char* get_txn_command_str(const tlm::tlm_generic_payload& trans)
    {
        switch (trans.get_command()) {
        case tlm::TLM_READ_COMMAND:
            return "READ";
        case tlm::TLM_WRITE_COMMAND:
            return "WRITE";
        case tlm::TLM_IGNORE_COMMAND:
            return "IGNORE";
        default:
            break;
        }
        return "UNKNOWN";
    }

    // Helper to get device_id from tag
    int get_device_from_tag(uint8_t tag) {
        auto it = tag_to_device_map.find(tag);
        if (it != tag_to_device_map.end()) {
            int device_id = it->second;
            tag_to_device_map.erase(it);  // Free the tag
            return device_id;
        }
        return -1;  // Tag not found
    }

    void b_transport_device_response(int id, tlm::tlm_generic_payload& txn, sc_core::sc_time& delay){
        unsigned int len = txn.get_data_length();
        unsigned char* ptr = txn.get_data_ptr();
        sc_dt::uint64 addr = txn.get_address();
        unsigned char* byt = txn.get_byte_enable_ptr();
        unsigned int bel = txn.get_byte_enable_length();
        
        if(len < 12){
            SCP_WARN() << "TLP too short: " << len << "bytes";
            txn.set_response_status(tlm::TLM_GENERIC_ERROR_RESPONSE);
            return;
        }

        // Read the first DWORD to determine TLP type
        uint32_t dword0;
        memcpy(&dword0, ptr, 4);
        dword0 = __builtin_bswap32(dword0);  // Convert from big-endian
        
        uint8_t fmt = (dword0 >> 29) & 0x3;   // Bits 30-29
        uint8_t type = (dword0 >> 24) & 0x1F; // Bits 28-24


        // Determine TLP category
        if (type == 0x0A) {
            // Parse the completion header to get tag and requester_id
            uint32_t dword2;
            memcpy(&dword2, ptr + 8, 4);
            dword2 = __builtin_bswap32(dword2);
            
            uint32_t dword1;
            memcpy(&dword1, ptr + 4, 4);
            dword1 = __builtin_bswap32(dword1);
            uint16_t completer_id = (dword1 >> 16) & 0xFFFF;
            uint8_t tag = (dword2 >> 8) & 0xFF;                // Bits [15:8]
            
            // TODO(jhieb) need to check for out of order responses.  Will be more complex to handle appropriately.
            int cmpltr_dev_id = (completer_id >> 3) & 0x1F;  // Extract device number
            int dev_from_tag = get_device_from_tag(tag);
            if(dev_from_tag <= 0 || dev_from_tag > devices.size()){
                SCP_FATAL(()) << "Invalid device from response tag.";
            }
            dev_responses[dev_from_tag-1] = &txn;
            e_wait_for_devices_resp[dev_from_tag-1]->notify();
            
        } else if (type == 0x00 && (fmt == 0x02 || fmt == 0x03)) {
            // **Memory Write TLP** (DMA write from device)
            //SCP_DEBUG() << "Received DMA Memory Write from device " << id 
            //            << " fmt=" << (int)fmt << " (3DW=" << (fmt==0x02) 
            //            << ", 4DW=" << (fmt==0x03) << ")";
            handle_dma_write(txn, delay, fmt);
            
        } else if (type == 0x00 && (fmt == 0x00 || fmt == 0x01)) {
            // **Memory Read TLP** (DMA read request from device)
            //SCP_DEBUG() << "Received DMA Memory Read Request from device " << id;
            handle_dma_read(txn, delay, fmt);
            
        } else {
            SCP_WARN() << "Unknown TLP type from device " << id 
                    << ": fmt=0x" << std::hex << (int)fmt 
                    << " type=0x" << (int)type;
            txn.set_response_status(tlm::TLM_GENERIC_ERROR_RESPONSE);
        }
        
        return;
    }
    
    void handle_dma_write(tlm::tlm_generic_payload& txn, sc_core::sc_time& delay, uint8_t fmt) {
        unsigned char* ptr = txn.get_data_ptr();
        
        if (fmt == 0x02) {
            // 3DW Memory Write
            TLPMemoryWrite3DW tlp;
            memcpy(tlp.data, ptr, txn.get_data_length());
            tlp.from_big_endian();
            
            uint64_t target_addr = tlp.dword2.addr;
            uint32_t length_dw = tlp.dword0.length;
            uint32_t byte_len = length_dw * 4;
            
            // Forward to system memory via bus_master socket
            tlm::tlm_generic_payload dma_txn;
            dma_txn.set_command(tlm::TLM_WRITE_COMMAND);
            dma_txn.set_address(target_addr);
            dma_txn.set_data_ptr(tlp.data_payload);
            dma_txn.set_data_length(byte_len);
            dma_txn.set_streaming_width(byte_len);
            dma_txn.set_byte_enable_ptr(nullptr);
            dma_txn.set_dmi_allowed(false);
            dma_txn.set_response_status(tlm::TLM_INCOMPLETE_RESPONSE);
            
            // Attach PCIe extension with requester_id
            PcieExtension* pcie_ext = new PcieExtension();
            pcie_ext->requester_id = tlp.dword1.requester_id;
            dma_txn.set_extension(pcie_ext);

            bus_master->b_transport(dma_txn, delay);
            txn.set_response_status(dma_txn.get_response_status());
        } else if (fmt == 0x03) {
            // 4DW Memory Write (64-bit address)
            TLPMemoryWrite4DW tlp;
            memcpy(tlp.data, ptr, txn.get_data_length());
            tlp.from_big_endian();
            
            uint64_t target_addr = (static_cast<uint64_t>(tlp.dword2.addr_high) << 32) 
                                | tlp.dword3.addr_low;
            uint32_t length_dw = tlp.dword0.length;
            uint32_t byte_len = length_dw * 4;
            
            //SCP_INFO() << "DMA Write (64-bit): addr=0x" << std::hex << target_addr 
            //        << " len=" << std::dec << byte_len << " bytes";
            
            tlm::tlm_generic_payload dma_txn;
            dma_txn.set_command(tlm::TLM_WRITE_COMMAND);
            dma_txn.set_address(target_addr);
            dma_txn.set_data_ptr(tlp.data_payload);
            dma_txn.set_data_length(byte_len);
            dma_txn.set_streaming_width(byte_len);
            dma_txn.set_byte_enable_ptr(nullptr);
            dma_txn.set_dmi_allowed(false);
            dma_txn.set_response_status(tlm::TLM_INCOMPLETE_RESPONSE);

            // Attach PCIe extension with requester_id
            PcieExtension* pcie_ext = new PcieExtension();
            pcie_ext->requester_id = tlp.dword1.requester_id;
            dma_txn.set_extension(pcie_ext);

            bus_master->b_transport(dma_txn, delay);
            txn.set_response_status(dma_txn.get_response_status());
        }
    }


    void handle_dma_read(tlm::tlm_generic_payload& txn, sc_core::sc_time& delay, uint8_t fmt) {
        unsigned char* ptr = txn.get_data_ptr();
        
        // Parse Memory Read Request TLP
        uint64_t target_addr;
        uint32_t length_dw;
        uint16_t requester_id;
        uint8_t tag;
        
        if (fmt == 0x00) {
            // 3DW Memory Read
            TLPMemoryRead3DW tlp;
            memcpy(tlp.data, ptr, 12);
            tlp.from_big_endian();
            
            target_addr = tlp.dword2.addr;
            length_dw = tlp.dword0.length;
            requester_id = tlp.dword1.requester_id;
            tag = tlp.dword1.tag;
            
        } else {
            // 4DW Memory Read (64-bit)
            TLPMemoryRead4DW tlp;
            memcpy(tlp.data, ptr, 16);
            tlp.from_big_endian();
            
            target_addr = (static_cast<uint64_t>(tlp.dword2.addr_high) << 32) 
                        | tlp.dword3.addr_low;
            length_dw = tlp.dword0.length;
            requester_id = tlp.dword1.requester_id;
            tag = tlp.dword1.tag;
        }
        
        uint32_t byte_len = length_dw * 4;
        //SCP_INFO() << "DMA Read Request: addr=0x" << std::hex << target_addr 
        //        << " len=" << std::dec << byte_len 
        //        << " tag=0x" << std::hex << (int)tag;
        
        // Read from system memory
        std::vector<uint8_t> read_data(byte_len);
        tlm::tlm_generic_payload dma_txn;
        dma_txn.set_command(tlm::TLM_READ_COMMAND);
        dma_txn.set_address(target_addr);
        dma_txn.set_data_ptr(read_data.data());
        dma_txn.set_data_length(byte_len);
        dma_txn.set_streaming_width(byte_len);
        dma_txn.set_byte_enable_ptr(nullptr);
        dma_txn.set_dmi_allowed(false);
        dma_txn.set_response_status(tlm::TLM_INCOMPLETE_RESPONSE);
        
        bus_master->b_transport(dma_txn, delay);
        
        if (dma_txn.get_response_status() == tlm::TLM_OK_RESPONSE) {
            // Send Completion with Data back to device
            send_completion_with_data(requester_id, tag, read_data.data(), byte_len, delay);
        }
        txn.set_response_status(dma_txn.get_response_status());
    }

    void send_completion_with_data(uint16_t requester_id, uint8_t tag, 
                                    uint8_t* data, uint32_t byte_len,
                                    sc_core::sc_time& delay) {
        TLPCompletionWithData cpl;
        
        // Build completion header
        cpl.dword0.fmt = 0x02;  // 3DW with data
        cpl.dword0.type = 0x0A; // Completion
        cpl.dword0.length = (byte_len + 3) / 4;
        
        cpl.dword1.completer_id = 0x0000;  // Root complex
        cpl.dword1.status = TLPCompletionWithData::SC;  // Successful completion
        cpl.dword1.byte_count = byte_len;
        
        cpl.dword2.requester_id = requester_id;
        cpl.dword2.tag = tag;
        cpl.dword2.lower_addr = 0;  // Assume DWORD-aligned
        
        // Copy payload
        memcpy(cpl.data_payload, data, byte_len);
        
        cpl.to_big_endian();
        
        // Send back to requesting device
        tlm::tlm_generic_payload resp_txn;
        resp_txn.set_command(tlm::TLM_WRITE_COMMAND);
        resp_txn.set_data_ptr(cpl.data);
        resp_txn.set_data_length(12 + byte_len);
        resp_txn.set_response_status(tlm::TLM_INCOMPLETE_RESPONSE);
        
        // Send to device that made the request
        int device_id = (requester_id >> 3) & 0x1F;  // Extract device number
        if (device_id > 0 && device_id <= devices.size()) {
            pci_device_initiator_socket[device_id - 1]->b_transport(resp_txn, delay);
        } else {
            SCP_FATAL(()) << "Invalid device ID for DMA.";
        }
    }

public:
    std::vector<std::shared_ptr<PCIeController>> devices;

    tlm_utils::multi_passthrough_target_socket<gs_gpex<BUSWIDTH>, BUSWIDTH> ecam_iface_socket;
    tlm_utils::multi_passthrough_target_socket<gs_gpex<BUSWIDTH>, BUSWIDTH> mmio_iface_socket;
    tlm_utils::multi_passthrough_target_socket<gs_gpex<BUSWIDTH>, BUSWIDTH> mmio_iface_high_socket;
    tlm_utils::multi_passthrough_target_socket<gs_gpex<BUSWIDTH>, BUSWIDTH> pio_iface_socket;

    tlm_utils::multi_passthrough_target_socket<gs_gpex<BUSWIDTH>, BUSWIDTH> pci_device_target_socket;
    tlm_utils::multi_passthrough_initiator_socket<gs_gpex<BUSWIDTH>, BUSWIDTH> pci_device_initiator_socket;

    TargetSignalSocket<bool> reset;
    tlm_utils::simple_initiator_socket<gs_gpex, BUSWIDTH> bus_master;
    // TODO(jhieb) Setup IRQ pins?? Currently use MSIX.
    //sc_core::sc_vector<QemuInitiatorSignalSocket> irq_out;
        /*
     * In qemu the gpex host is aware of which gic spi indexes it
     * plugged into. This is optional and the indexes can be left to -1
     * but it removes some feature.
     */
    //int irq_num[4];

    gs_gpex(sc_core::sc_module_name name)
        : m_broker(cci::cci_get_broker())
        , ecam_iface_socket("ecam_iface")
        , mmio_iface_socket("mmio_iface")
        , mmio_iface_high_socket("mmio_iface_high")
        , pio_iface_socket("pio_iface")
        , bus_master("bus_master")
        , reset("reset")
        //, irq_out("irq_out", 4)
        //, irq_num{ -1, -1, -1, -1 }
    {
       // Set relative_addresses to false for MMIO sockets so we get the full addressing to map BARs in GPEX.
        m_broker.set_preset_cci_value(std::string(sc_module::name()) + ".mmio_iface.relative_addresses", 
                                      cci::cci_value(false));
        m_broker.set_preset_cci_value(std::string(sc_module::name()) + ".mmio_iface_high.relative_addresses", 
                                      cci::cci_value(false));
        m_broker.set_preset_cci_value(std::string(sc_module::name()) + ".ecam_iface.relative_addresses", 
                                      cci::cci_value(true));

        SCP_DEBUG(()) << "gs_gpex constructor";
        ecam_iface_socket.register_b_transport(this, &gs_gpex::b_transport_ecam);
        mmio_iface_socket.register_b_transport(this, &gs_gpex::b_transport_mmio);
        mmio_iface_high_socket.register_b_transport(this, &gs_gpex::b_transport_mmio_high);
        pio_iface_socket.register_b_transport(this, &gs_gpex::b_transport_pio);
        pci_device_target_socket.register_b_transport(this, &gs_gpex::b_transport_device_response);

        reset.register_value_changed_cb([&](bool value) {
            if (value) {
                SCP_WARN(()) << "Reset";
            }
        });

        // We need to setup the PCI configuration for the root so that it can be read by the host.
        write_to_config(config + PCI_VENDOR_ID, 0x1b36, 2); // PCI_VENDOR_ID_REDHAT
        write_to_config(config + PCI_DEVICE_ID, 0x0008, 2); // PCI_DEVICE_ID_REDHAT_PCIE_HOST
        config[PCI_COMMAND] = 0x0;
        config[PCI_STATUS] = 0x0;
        config[PCI_REVISION_ID] = 0;
        write_to_config(config + PCI_CLASS_DEVICE, PCI_CLASS_BRIDGE_HOST, 2);
        config[PCI_CACHE_LINE_SIZE] = 0x0;
        config[PCI_LATENCY_TIMER] = 0x0;
        config[PCI_HEADER_TYPE] = 0x0;
        config[PCI_BIST] = 0x0;
    }

    void add_device(std::shared_ptr<PCIeController>& pcie_ctlr){
        devices.push_back(pcie_ctlr);
        e_wait_for_devices_resp.push_back(std::make_unique<sc_core::sc_event>());
        dev_responses.push_back(nullptr);
        pcie_ctlr->init_socket.bind(pci_device_target_socket);
        pci_device_initiator_socket.bind(pcie_ctlr->tgt_socket);
    }

    void write_to_config(uint8_t* addr, uint64_t val, uint8_t len = 1){
        if(len > 8){
            SCP_FATAL(()) << "Can't copy more than the 64 bit value passed in";
        }
        memcpy(addr, &val, len);
    }

    void before_end_of_elaboration()
    {
    }

    gs_gpex() = delete;
    gs_gpex(const gs_gpex&) = delete;

    ~gs_gpex() {}
};
} // namespace gs

extern "C" void module_register();
#endif
