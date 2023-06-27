/*
 * Quic Module reg_model_maker
 *
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version, or under the
 * Apache License, Version 2.0 (the "License”) at your discretion.
 *
 * SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,
 * USA. You may obtain a copy of the Apache License at
 * http://www.apache.org/licenses/LICENSE-2.0
 */

#ifndef _REG_MODEL_MAKER_H
#define _REG_MODEL_MAKER_H
#include <systemc>
#include <cci_configuration>
#include <scp/report.h>
#include "rapidjson/document.h"
#include <zip.h>
#include <greensocs/gsutils/cciutils.h>
#include <greensocs/base-components/router.h>
#include <greensocs/base-components/memory.h>
#include <greensocs/gsutils/registers.h>

namespace gs {

class json_zip_archive
{
    std::set<std::string> m_loaded;
    zip_t* m_archive;

public:
    json_zip_archive(zip_t* archive)
        : m_archive(archive)
    {
        if (m_archive == nullptr) {
            SCP_FATAL()("Can't open zip archive");
        }
    }

    uint64_t getNumber(rapidjson::Value& v)
    {
        if (v.IsNumber()) {
            return v.GetUint64();
        }
        if (v.IsString()) {
            const char* p = v.GetString();
            return std::strtoull(p, nullptr, 0);
        }
        SCP_FATAL()("JSON Number neither number, nor string!");
        return 0;
    }

    std::string get_json_topname()
    {
        zip_stat_t stat;
        zip_stat_index(m_archive, zip_get_num_entries(m_archive, 0) - 1, ZIP_FL_NOCASE, &stat);
        std::string ret = stat.name;
        if (ret.find(".json", ret.length() - 5) != ret.npos) ret = ret.substr(0, ret.length() - 5);
        return ret;
    }
    rapidjson::Document& get_json(std::string file, rapidjson::Document& d)
    {
        zip_stat_t stat;
        SCP_TRACE("json_cci")("Looking for {} in archive", file.c_str());

        if (file.empty()) {
            file = get_json_topname() + ".json";
        }
        if (zip_stat(m_archive, file.c_str(), ZIP_FL_NOCASE, &stat) != 0) {
            SCP_DEBUG("json_cci")("Unable to find {} in archive", file.c_str());
            return d;
        }

        char* json_info = (char*)malloc(stat.size + 1);
        zip_file_t* fd = zip_fopen(m_archive, file.c_str(), ZIP_FL_NOCASE);
        if (zip_fread(fd, json_info, stat.size) != stat.size) {
            SCP_FATAL("json_cci")("Unable to read {} in archive", file);
        }
        json_info[stat.size] = '\0';

        d.Parse(json_info);
        if (d.HasParseError()) {
            SCP_FATAL("json_cci")("Unable to parse {}", stat.name);
        }
        zip_fclose(fd);
        free(json_info);
        return d;
    }

private:
    template <class T>
    void cci_set(cci::cci_broker_handle broker, std::string n, T value)
    {
        if (!broker.has_preset_value(n)) {
            SCP_DEBUG("json_cci")("set {} to {}", n, value);
            broker.set_preset_cci_value(n, cci::cci_value(value));
        } else {
            SCP_FATAL("json_cci")("Trying to re-set a value? {}", n);
        }
    }

public:
    bool json_read_cci(cci::cci_broker_handle broker, std::string sc_name, std::string _path = "",
                       rapidjson::Value* _node = nullptr)
    {
        std::string module_path = _path.empty() ? get_json_topname() : _path;

        rapidjson::Document json_doc;
        rapidjson::Value& node = _node ? *_node : get_json(module_path + ".json", json_doc);

        if (node.IsNull()) return false;

        if (m_loaded.count(sc_name)) return true;
        m_loaded.insert(sc_name);

        SCP_INFO("json_cci")("JSON read into sc object {} from JSON node {}", sc_name, node["NAME"].GetString());

        uint64_t min = 0;
        uint64_t max = 0;

        if (node.HasMember("REGISTERS")) {
            // max, base address, bitwidth
            std::map<std::string, std::tuple<uint, uint64_t, uint, uint64_t>> vecs;
            for (auto& reg : node["REGISTERS"].GetArray()) {
                std::string rn = sc_name + "." + reg["NAME"].GetString();

                /* collect vectors for convenience, you coud still use the full name of the register */
                auto indx = rn.substr(rn.find_last_not_of("0123456789") + 1);
                if (indx.size()) {
                    auto base = rn.substr(0, rn.size() - indx.size()); // + "_";
                    uint i = std::stoi(indx);
                    uint maxi = 0;
                    uint64_t offset = 0;
                    uint bitwidth = 0;
                    uint64_t num = 0;
                    if (vecs.count(base) > 0) {
                        std::tie(maxi, offset, bitwidth, num) = vecs[base];
                    }
                    if (i > maxi) maxi = i;
                    uint64_t o = getNumber(reg["OFFSET"]);
                    if (i == 0) offset = o;
                    bitwidth = getNumber(reg["BITWIDTH"]);
                    vecs[base] = std::make_tuple(maxi, offset, bitwidth, num + 1);
                } // else {
                cci_set(broker, rn + ".target_socket.address", getNumber(reg["OFFSET"]));
                cci_set(broker, rn + ".target_socket.size", getNumber(reg["BITWIDTH"]) / 8);
                cci_set(broker, rn + ".target_socket.relative_addresses", true);
                //}
                SCP_TRACE("json_cci")("Register : {} at 0x{:x}", rn, getNumber(reg["OFFSET"]));

                if (reg.HasMember("FIELDS")) {
                    for (auto& field : reg["FIELDS"].GetArray()) {
                        if (field["NAME"] == "RESERVED") continue;
                        std::string fn = rn + "." + field["NAME"].GetString();
                        int msb = getNumber(field["MSB"]);
                        int len = 1;
                        int start = msb;
                        if (field.HasMember("LSB")) {
                            int lsb = getNumber(field["LSB"]);
                            start = lsb;
                            len = (msb - lsb) + 1;
                        }
                        cci_set(broker, fn + ".bit_start", start);
                        cci_set(broker, fn + ".bit_length", len);
                        SCP_TRACE("json_cci")("Register field : {} (start bit: {}, length: {})", fn, start, len);
                    }
                }
            }
            for (auto& v : vecs) {
                uint maxi;
                uint64_t offset;
                uint bitwidth;
                uint64_t num;
                std::tie(maxi, offset, bitwidth, num) = v.second;
                if (num > 1 && num == maxi + 1) { //  Well behaved arrays only
                    std::string rn = v.first;
                    sc_assert(num == maxi + 1);
                    cci_set(broker, rn + ".number", num);
                    cci_set(broker, rn + ".target_socket.address", offset);
                    cci_set(broker, rn + ".target_socket.size", (bitwidth / 8) * num);
                    cci_set(broker, rn + ".target_socket.relative_addresses", true);
                    SCP_TRACE("json_cci")("Register vector : {} at 0x{:x} size {}", rn, offset, num);
                } else {
                    SCP_DEBUG("json_cci")("Register : {} is a badly behaved array", v.first);
                }
            }
        }
        if (node.HasMember("SIZE")) {
            SCP_TRACE("json_cci")("Setting {}.reg_memory", sc_name);
            cci_set(broker, sc_name + ".reg_memory.target_socket.address", 0);
            cci_set(broker, sc_name + ".reg_memory.target_socket.size", getNumber(node["SIZE"]));
            cci_set(broker, sc_name + ".reg_memory.target_socket.relative_addresses", true);
        }
        if (node.HasMember("MODULES")) {
            auto mods = node["MODULES"].GetArray();
            for (auto& mod : mods) {
                if (mod.IsObject()) {
                    std::string sn = sc_name + "." + mod["NAME"].GetString();
                    json_read_cci(broker, sn, module_path, &mod);
                    if (mod.HasMember("OFFSET")) {
                        cci_set(broker, sn + ".target_socket.chained", true);
                    }
                }
            }
        }

        if (node.HasMember("MEMORY")) {
            auto mems = node["MEMORY"].GetArray();
            for (auto& m : mems) {
                std::string sn = sc_name + "." + m["NAME"].GetString();
                if (m.HasMember("OFFSET"))
                    cci_set(broker, sn + ".target_socket.address", getNumber(m["OFFSET"]));
                else
                    cci_set(broker, sn + ".target_socket.address", getNumber(m["ADDRESS"]));
                cci_set(broker, sn + ".target_socket.size", getNumber(m["SIZE"]));
                cci_set(broker, sn + ".target_socket.relative_addresses", true);
            }
        }

        std::string sn = sc_name;
        if (node.HasMember("OFFSET")) {
            cci_set(broker, sn + ".target_socket.address", getNumber(node["OFFSET"]));
            cci_set(broker, sn + ".target_socket.size", getNumber(node["SIZE"]));
            cci_set(broker, sn + ".target_socket.relative_addresses", true);
            cci_set(broker, sn + ".router.lazy_init", true);
        }

        return true;
    }
};

class json_fallback_module : public virtual sc_core::sc_module
{
    SCP_LOGGER();

    cci::cci_broker_handle m_broker;
    //    zip_t* m_zip_archive;
    cci::cci_param<std::string> p_zipfile;
    cci::cci_param<bool> p_strict;
    json_zip_archive jza;

    gs::Memory<> m_memory;

public:
    tlm_utils::multi_passthrough_target_socket<json_fallback_module> target_socket;
    tlm_utils::simple_initiator_socket<json_fallback_module> init_socket;

    /*    std::string get_json_topname()
        {
            zip_stat_t stat;
            zip_stat_index(m_zip_archive, zip_get_num_entries(m_archive, 0) - 1, ZIP_FL_NOCASE, &stat);
            std::string ret = stat.name;
            if (ret.find(".json", ret.length() - 5) != ret.npos) ret = ret.substr(0, ret.length() - 5);
            return ret;
        }
    */
    template <class T>
    void cci_set(std::string n, T value)
    {
        if (!m_broker.has_preset_value(n)) {
            m_broker.set_preset_cci_value(n, cci::cci_value(value));
        } else {
            SCP_FATAL(())("{} already set ?", n);
        }
    }
    template <typename T>
    T cci_get(std::string n)
    {
        T ret;
        if (!m_broker.get_preset_cci_value(n).template try_get<T>(ret)) {
            SCP_FATAL(()) << "Unable to get parameter " << n;
        };
        m_broker.lock_preset_value(n);
        return ret;
    }

    struct target_info {
        std::string name;
        sc_dt::uint64 address;
        sc_dt::uint64 size;
    };
    std::vector<target_info> targets;

    bool check_mapping(tlm::tlm_generic_payload& txn)
    {
        uint64_t addr = txn.get_address();
        for (auto& ti : targets) {
            if (addr >= ti.address && addr <= ti.address + ti.size) {
                SCP_DEBUG(())("Access to {:#x} which is in {} (Register only implementation)", addr, ti.name);
                return true;
            }
        }
        std::string f = json_find_module(txn, name());
        if (f.empty()) {
            SCP_WARN(())("NO MODULE FOUND for Access to {:#x}!", addr);
            return !p_strict;
        }

        target_info ti;
        ti.name = f;
        ti.address = cci_get<uint64_t>(f + ".target_socket.address");
        ti.size = cci_get<uint64_t>(f + ".target_socket.size");
        targets.push_back(ti);
        SCP_WARN(())("First Access to {:#x} which is in {} (Register only implementation)", addr, ti.name);
        return true;
    }
    void b_transport(int id, tlm::tlm_generic_payload& txn, sc_core::sc_time& delay)
    {
        if (check_mapping(txn)) {
            init_socket->b_transport(txn, delay);
            SCP_TRACE(())("{}", scp::scp_txn_tostring(txn));
        }
        return;
    }
    unsigned int transport_dbg(int id, tlm::tlm_generic_payload& txn)
    {
        if (check_mapping(txn)) {
            return init_socket->transport_dbg(txn);
        }
        return 0;
    }
    bool get_direct_mem_ptr(int id, tlm::tlm_generic_payload& txn, tlm::tlm_dmi& dmi_data)
    {
        if (check_mapping(txn)) {
            return init_socket->get_direct_mem_ptr(txn, dmi_data);
        }
        return false;
    }
    void invalidate_direct_mem_ptr(sc_dt::uint64 start, sc_dt::uint64 end)
    {
        target_socket->invalidate_direct_mem_ptr(start, end);
    }

    uint64_t max_size()
    {
        std::string module_path = jza.get_json_topname();
        rapidjson::Document json_doc;
        rapidjson::Value& node = jza.get_json(module_path + ".json", json_doc);

        if (node.HasMember("SIZE")) return jza.getNumber(node["SIZE"]);
        uint64_t s = 0;

        auto mods = node["MODULES"].GetArray();
        for (auto& mod : mods) {
            if (mod.IsObject()) {
                uint64_t maddr = jza.getNumber(mod["ADDRESS"]);
                uint64_t msize = jza.getNumber(mod["SIZE"]);
                if (maddr + msize > s) s = maddr + msize;
            }
        }
        return s;
    }
    json_fallback_module(sc_core::sc_module_name _name, std::string zipfile)
        : json_fallback_module(_name, nullptr, zipfile)
    {
    }
    json_fallback_module(sc_core::sc_module_name _name, zip_t* zip_archive = nullptr, std::string zipfile = "")
        : p_zipfile("zipfile", zipfile, "Zip filename to load")
        , jza(zip_archive ? zip_archive : zip_open(p_zipfile.get_value().c_str(), ZIP_RDONLY, nullptr))
        , sc_core::sc_module(_name)
        , m_broker(cci::cci_get_broker())
        //        , m_zip_archive(zip_archive)
        , target_socket("target_socket")
        , init_socket("internal_init_socket")
        , m_memory("memory", max_size())
        , p_strict("strict", false, "Fail on accesses to unknown memory areas")
    {
        SCP_TRACE(())("Constructor");

        m_memory.p_dmi = false;
        m_memory.p_init_mem = true;
        m_memory.p_init_mem_val = 0x0; // NB this must be 0 as the JSON file assumes 0 initialization
        cci_set<uint64_t>(std::string(name()) + ".memory.target_socket.address", 0);
        target_socket.register_b_transport(this, &json_fallback_module::b_transport);
        target_socket.register_transport_dbg(this, &json_fallback_module::transport_dbg);
        target_socket.register_get_direct_mem_ptr(this, &json_fallback_module::get_direct_mem_ptr);
        init_socket.register_invalidate_direct_mem_ptr(this, &json_fallback_module::invalidate_direct_mem_ptr);

        init_socket.bind(m_memory.socket);
    }

    std::string json_find_module(tlm::tlm_generic_payload& txn, std::string sc_name, std::string _path = "",
                                 rapidjson::Value* _node = nullptr)
    {
        sc_dt::uint64 address = txn.get_address();
        unsigned int len = txn.get_data_length();

        SCP_TRACE("json_cci")("JSON find {:#x}({}) in {}", address, len, sc_name);

        std::string module_path = _path.empty() ? jza.get_json_topname() : _path;
        rapidjson::Document json_doc;
        rapidjson::Value& node = _node ? *_node : jza.get_json(module_path + ".json", json_doc);
        if (!_node) {
            std::string topname = jza.get_json_topname();
            sc_name = (sc_name.empty()) ? sc_name + "." + topname : topname;
        }
        /* first check sub modules*/
        if (node.HasMember("MODULES")) {
            auto mods = node["MODULES"].GetArray();
            for (auto& mod : mods) {
                if (mod.IsObject()) {
                    std::string sn = sc_name + "." + mod["NAME"].GetString();
                    uint64_t maddr = jza.getNumber(mod["ADDRESS"]);
                    uint64_t msize = jza.getNumber(mod["SIZE"]);
                    if (address >= maddr && address + len <= maddr + msize) {
                        SCP_TRACE("json_cci")("JSON found module {:#x}({}) in {}", address, len, sn);
                        return json_find_module(txn, sn, module_path, &mod);
                    }
                } else {
                    std::string path = ((module_path != "") ? module_path + "/" : "") + mod["NAME"].GetString();
                    return json_find_module(txn, sc_name, path, nullptr);
                }
            }
        }
        if (node.HasMember("MEMORY")) {
            auto mems = node["MEMORY"].GetArray();
            for (auto& mod : mems) {
                std::string sn = sc_name + "." + mod["NAME"].GetString();

                uint64_t maddr = jza.getNumber(mod["ADDRESS"]);
                uint64_t msize = jza.getNumber(mod["SIZE"]);
                if (address >= maddr && address + len <= maddr + msize) {
                    cci_set<uint64_t>(sn + ".target_socket.address", maddr);
                    cci_set<uint64_t>(sn + ".target_socket.size", msize);
                    cci_set<bool>(sn + ".target_socket.relative_addresses", false);
                    SCP_TRACE("json_cci")("JSON memory found {:#x}({}) in {}", address, len, sn);
                    return sn;
                }
            }
        }

        if ((!node.HasMember("SIZE")) || (!node.HasMember("ADDRESS"))) {
            SCP_DEBUG("json_cci")("JSON no range for address {:#x} (size: {}) in {}", address, len, sc_name);
            return "";
        }

        uint64_t lsize = jza.getNumber(node["SIZE"]);
        uint64_t laddr = jza.getNumber(node["ADDRESS"]);

        if (address < laddr || address + len > laddr + lsize) {
            SCP_DEBUG("json_cci")("JSON address out of range {:#x} (size: {}) in {}", address, len, sc_name);
            return "";
        }

        // For now
        if (node.HasMember("REGISTERS")) {
            std::vector<uint8_t> resetvalue;
            uint len = 0;
            for (auto& reg : node["REGISTERS"].GetArray()) {
                uint64_t rv = jza.getNumber(reg["RESETVALUE"]);
                uint64_t offset = jza.getNumber(reg["OFFSET"]);
                int bw = jza.getNumber(reg["BITWIDTH"]);
                int i;
                for (i = 0; i < bw / 8; i++) {
                    if (resetvalue.size() < (offset + i + 1)) {
                        resetvalue.resize(offset + i + 1);
                    }
                    resetvalue[offset + i] = rv & 0xff;
                    rv >>= 8;
                }
                if (offset + i > len) len = offset + i;
            }
            m_memory.load.ptr_load(&resetvalue[0], laddr, len);
        }

        cci_set<uint64_t>(sc_name + ".target_socket.address", laddr);
        cci_set<uint64_t>(sc_name + ".target_socket.size", lsize);
        cci_set<bool>(sc_name + ".target_socket.relative_addresses", false);
        SCP_TRACE("json_cci")("JSON found {:#x}({}) in {}", address, len, sc_name);
        return sc_name;
    }
};

GSC_MODULE_REGISTER(json_fallback_module);

class json_module : public virtual sc_core::sc_module
{
    SCP_LOGGER();

    cci::cci_broker_handle m_broker;
    gs::Router<> m_router;

    gs::Memory<>* m_reg_memory = nullptr;
    sc_core::sc_vector<gs::Memory<>> memories;
    sc_core::sc_vector<json_module> sub_modules;

public:
    template <class T>
    void cci_set(std::string n, T value)
    {
        if (!m_broker.has_preset_value(n)) {
            m_broker.set_preset_cci_value(n, cci::cci_value(value));
        } else {
            SCP_FATAL(())("Trying to re-set a value? {}", n);
        }
    }

    template <typename T>
    T cci_get(std::string n)
    {
        T ret;
        if (!m_broker.get_preset_cci_value(n).template try_get<T>(ret)) {
            SCP_FATAL(()) << "Unable to get parameter " << n << "\nIs your .lua file up-to-date?";
        };
        m_broker.lock_preset_value(n);
        return ret;
    }

private:
public:
    tlm_utils::multi_passthrough_target_socket<json_module> target_socket;

    ~json_module()
    {
        if (m_reg_memory) free(m_reg_memory);
    }

    json_module(sc_core::sc_module_name _name, json_zip_archive& jza, rapidjson::Value* _node = nullptr,
                std::string _path = "", bool top = true)
        : sc_core::sc_module(_name)
        , m_broker(cci::cci_get_broker())
        , target_socket("target_socket")
        , m_router("router")
    {
        //        json_read_cci(std::string(name()), zip_archive, _node, _path);

        SCP_TRACE(())("Constructor");

        std::string n = std::string(name());

        std::string module_path = _path.empty() ? jza.get_json_topname() : _path;

        rapidjson::Document json_doc;
        rapidjson::Value& node = _node ? *_node : jza.get_json(module_path + ".json", json_doc);

        if (node.HasMember("REGISTERS")) {
            m_reg_memory = new gs::Memory<>("reg_memory");
            std::vector<uint8_t> resetvalue;
            uint len = 0;
            for (auto& reg : node["REGISTERS"].GetArray()) {
                uint64_t rv = jza.getNumber(reg["RESETVALUE"]);
                uint64_t offset = jza.getNumber(reg["OFFSET"]);
                int bw = jza.getNumber(reg["BITWIDTH"]);
                int i;
                for (i = 0; i < bw / 8; i++) {
                    if (resetvalue.size() < (offset + i + 1)) {
                        resetvalue.resize(offset + i + 1);
                    }
                    resetvalue[offset + i] = rv & 0xff;
                    rv >>= 8;
                }
                if (offset + i > len) len = offset + i;
            }
            m_reg_memory->load.ptr_load(&resetvalue[0], 0, len);
        }

        if (node.HasMember("MODULES")) {
            auto mods = node["MODULES"].GetArray();
            sub_modules.init(mods.Size(), [&, mods](const char* name, size_t n) -> json_module* {
                if (!mods[n].IsObject()) {
                    SCP_FATAL(())("JSON file for json_module must be fully self contained");
                }
                return new json_module(mods[n]["NAME"].GetString(), jza, &mods[n], module_path, false);
            });
        }

        if (node.HasMember("MEMORY")) {
            auto mems = node["MEMORY"].GetArray();
            memories.init(mems.Size(), [&, mems](const char* name, size_t n) -> gs::Memory<>* {
                return new gs::Memory<>(mems[n]["NAME"].GetString());
            });
        }
        for (auto& m : sub_modules) {
            m_router.initiator_socket(m.target_socket);
        }
        for (auto& m : memories) {
            m_router.initiator_socket(m.socket);
        }
        target_socket.bind(m_router.target_socket);
        if (!m_reg_memory && memories.size() == 0 && sub_modules.size() == 0) {
            m_reg_memory = new gs::Memory<>("reg_memory");
        }

        /* must be last one*/
        if (m_reg_memory) m_router.initiator_socket(m_reg_memory->socket);
    }

    template <class TYPE>
    void bind_reg(gs::gs_register<TYPE>& reg, std::string path, std::string regname)
    {
        auto parent = path.substr(0, path.find_last_of("."));
        SCP_TRACE(())("Binding {} to path {} in {}", reg.name(), parent, std::string(sc_module::name()));
        auto mem = dynamic_cast<tlm_utils::multi_passthrough_target_socket<gs::Memory<>>*>(
            gs::find_sc_obj(this, std::string(sc_module::name()) + "." + parent + ".reg_memory.target_socket"));
        sc_assert(mem);
        reg.initiator_socket.bind(*mem);

        auto rtr = dynamic_cast<gs::Router<>*>(
            gs::find_sc_obj(this, std::string(sc_module::name()) + "." + parent + ".router"));
        sc_assert(rtr);
        rtr->initiator_socket.bind(reg);
        rtr->rename_last(std::string(sc_module::name()) + "." + path + ".target_socket");
    }
};

} // namespace gs

#define EVAL(...)     EVAL1024(__VA_ARGS__)
#define EVAL1024(...) EVAL512(EVAL512(__VA_ARGS__))
#define EVAL512(...)  EVAL256(EVAL256(__VA_ARGS__))
#define EVAL256(...)  EVAL128(EVAL128(__VA_ARGS__))
#define EVAL128(...)  EVAL64(EVAL64(__VA_ARGS__))
#define EVAL64(...)   EVAL32(EVAL32(__VA_ARGS__))
#define EVAL32(...)   EVAL16(EVAL16(__VA_ARGS__))
#define EVAL16(...)   EVAL8(EVAL8(__VA_ARGS__))
#define EVAL8(...)    EVAL4(EVAL4(__VA_ARGS__))
#define EVAL4(...)    EVAL2(EVAL2(__VA_ARGS__))
#define EVAL2(...)    EVAL1(EVAL1(__VA_ARGS__))
#define EVAL1(...)    __VA_ARGS__

#define EMPTY()
#define DEFER2(m) m EMPTY EMPTY()()

#define MAP(m, first, ...) \
    IIF(IS_PAREN(FIRST_ARG(first)))(m first) IIF(IS_PAREN(FIRST_ARG(__VA_ARGS__)))(DEFER2(_MAP)()(m, __VA_ARGS__))
#define _MAP() MAP

#define __GSREG_DECLARE(TYPE, PATH, NAME) \
    IIF(IS_PAREN(FIRST_ARG(PATH)))(gs::gs_field<TYPE> NAME;, gs::gs_register<TYPE> NAME;)
#define GSREG_DECLARE(...) EVAL(MAP(__GSREG_DECLARE, __VA_ARGS__)) bool _dummy

#define __GSREG_FIELD_HELPER(PATH, NAME) PATH, #NAME
#define __GSREG_CONSTRUCT(TYPE, PATH, NAME) \
    , IIF(IS_PAREN(FIRST_ARG(PATH)))(NAME(__GSREG_FIELD_HELPER PATH), NAME(#NAME, "M." #PATH))
#define GSREG_CONSTRUCT(...) _dummy(false) EVAL(MAP(__GSREG_CONSTRUCT, __VA_ARGS__))

#define __GSREG_BIND(TYPE, PATH, NAME) IIF(IS_PAREN(FIRST_ARG(PATH)))(/* */, M.bind_reg<TYPE>(NAME, #PATH, #NAME));
#define GSREG_BIND(...)                      \
    do {                                     \
        EVAL(MAP(__GSREG_BIND, __VA_ARGS__)) \
    } while (0)

#define TXN(T) tlm::tlm_generic_payload &FIRST_ARG(T, _txn), sc_core::sc_time &delay

#endif // _REG_MODEL_MAKER_H