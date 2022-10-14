/*
* Copyright (c) 2022 GreenSocs
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
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
* You may obtain a copy of the Apache License at
* http://www.apache.org/licenses/LICENSE-2.0
*/

#ifndef _GREENSOCS_BASE_COMPONENTS_LOADER_H
#define _GREENSOCS_BASE_COMPONENTS_LOADER_H

#include <fstream>

#include <cci_configuration>
#include <systemc>
#include <tlm>
#include <tlm_utils/simple_initiator_socket.h>

#include <cinttypes>
#include <fcntl.h>
#include <libelf.h>
#include <list>
#include <string>
#include <unistd.h>
#include <vector>

#ifndef _WIN32
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#endif

namespace gs {

/**
 * @class Loader
 *
 * @brief A loader that cope with several formats
 *
 * @details
 */

/* simple class to load csv file*/
class CSVRow {
public:
    std::string operator[](std::size_t index) const
    {
        return std::string(&m_line[m_data[index] + 1], m_data[index + 1] - (m_data[index] + 1));
    }
    std::size_t size() const
    {
        return m_data.size() - 1;
    }
    void readNextRow(std::istream& str)
    {
        std::getline(str, m_line);

        m_data.clear();
        m_data.emplace_back(-1);
        std::string::size_type pos = 0;
        while ((pos = m_line.find(',', pos)) != std::string::npos) {
            m_data.emplace_back(pos);
            ++pos;
        }
        // This checks for a trailing comma with no data after it.
        pos = m_line.size();
        m_data.emplace_back(pos);
    }

private:
    std::string m_line;
    std::vector<int> m_data;
};

static std::istream& operator>>(std::istream& str, CSVRow& data)
{
    data.readNextRow(str);
    return str;
}

namespace loader {
    static const char* log_enabled = std::getenv("GS_LOG");
}

template <unsigned int BUSWIDTH = 32>
class Loader : public sc_core::sc_module {

    template <typename MODULE, typename TYPES = tlm::tlm_base_protocol_types>
    class simple_initiator_socket_zero
        : public tlm_utils::simple_initiator_socket_b<MODULE, BUSWIDTH, TYPES, sc_core::SC_ZERO_OR_MORE_BOUND> {
        using typename tlm_utils::simple_initiator_socket_b<MODULE, BUSWIDTH, TYPES, sc_core::SC_ZERO_OR_MORE_BOUND>::base_target_socket_type;
        typedef tlm_utils::simple_initiator_socket_b<MODULE, BUSWIDTH, TYPES, sc_core::SC_ZERO_OR_MORE_BOUND> socket_b;

    public:
        simple_initiator_socket_zero(const char* name)
            : socket_b(name)
        {
        }
    };

public:
    simple_initiator_socket_zero<Loader<BUSWIDTH>> initiator_socket;

private:
    cci::cci_broker_handle m_broker;

    uint64_t m_address = 0;

    std::function<void(const uint8_t* data, uint64_t offset, uint64_t len)> write_cb;
    bool m_use_callback = false;

    void cci_ignore(std::string name)
    {
        auto m_broker = cci::cci_get_broker();
        std::string fullname = std::string(sc_module::name()) + "." + name;
        m_broker.ignore_unconsumed_preset_values(
            [fullname](const std::pair<std::string, cci::cci_value>& iv) -> bool { return iv.first == fullname; });
    }

    std::list<std::string> sc_cci_children(sc_core::sc_module_name name)
    {
        std::list<std::string> children;
        int l = strlen(name) + 1;
        auto uncon = m_broker.get_unconsumed_preset_values([&name](const std::pair<std::string, cci::cci_value>& iv) { return iv.first.find(std::string(name) + ".") == 0; });
        for (auto p : uncon) {
            children.push_back(p.first.substr(l, p.first.find(".", l) - l));
        }
        children.sort();
        children.unique();
        return children;
    }

    void send(uint64_t addr, uint8_t* data, uint64_t len)
    {
        if (m_use_callback) {
            write_cb(data, addr, len);
        } else {
            tlm::tlm_generic_payload trans;

            trans.set_command(tlm::TLM_WRITE_COMMAND);
            trans.set_address(addr);
            trans.set_data_ptr(data);
            trans.set_data_length(len);
            trans.set_streaming_width(len);
            trans.set_byte_enable_length(0);
            if (initiator_socket->transport_dbg(trans) != len) {
                std::stringstream info;
                info << name() << " : Error loading data to memory @ "
                     << "0x" << std::hex << addr;
                SC_REPORT_FATAL("Loader", info.str().c_str());
            }
        }
    }

    template <typename T>
    T cci_get(std::string name)
    {
        m_broker.ignore_unconsumed_preset_values(
            [name](const std::pair<std::string, cci::cci_value>& iv) -> bool { return iv.first == name; });
        m_broker.lock_preset_value(name);
        T ret;
        if (!m_broker.get_preset_cci_value(name).template try_get<T>(ret)) {
            SC_REPORT_FATAL("Loader", ("Unable to get parameter " + name).c_str());
        };
        return ret;
    }

    uint32_t byte_swap32(uint32_t in)
    {
        uint32_t out;
        out = in & 0xff;
        in >>= 8;
        out <<= 8;
        out |= in & 0xff;
        in >>= 8;
        out <<= 8;
        out |= in & 0xff;
        in >>= 8;
        out <<= 8;
        out |= in & 0xff;
        in >>= 8;
        return out;
    }

public:
    Loader(sc_core::sc_module_name name)
        : m_broker(cci::cci_get_broker())
        , initiator_socket("initiator_socket") //, [&](std::string s) -> void { register_boundto(s); })
    {
    }
    Loader(sc_core::sc_module_name name,
        std::function<void(const uint8_t* data, uint64_t offset, uint64_t len)> _write)
        : m_broker(cci::cci_get_broker())
        , initiator_socket("initiator_socket") //, [&](std::string s) -> void { register_boundto(s); })
        , write_cb(_write)
    {
        m_use_callback = true;
    }

    ~Loader()
    {
    }

protected:
    void load(std::string name)
    {
        bool read = false;
        if (m_broker.has_preset_value(name + ".elf_file")) {
            if (m_use_callback) {
                SC_REPORT_WARNING("loader", "elf file loading into local memory will interpret addresses relative to the memory - probably not what you want?");
            }
            std::string file = cci_get<std::string>(name + ".elf_file");
            elf_load(file);
            read = true;
        } else {
            uint64_t addr;
            if (m_use_callback) {
                if (m_broker.has_preset_value(name + ".offset"))
                    addr = cci_get<uint64_t>(name + ".offset");
                else
                    SC_REPORT_FATAL("loader", "No offset found");
            } else {
                if (m_broker.has_preset_value(name + ".address"))
                    addr = cci_get<uint64_t>(name + ".address");
                else
                    SC_REPORT_FATAL("loader", "No address found");
            }
            if (m_broker.has_preset_value(name + ".bin_file")) {
                std::string file = cci_get<std::string>(name + ".bin_file");
                if (gs::loader::log_enabled) {
                    SC_REPORT_INFO("Loader", ("Loading binary file " + file).c_str());
                }
                file_load(file, addr);
                read = true;
            }
            if (m_broker.has_preset_value(name + ".csv_file")) {
                std::string file = cci_get<std::string>(name + ".csv_file");
                std::string addr_str = cci_get<std::string>(name + ".addr_str");
                std::string val_str = cci_get<std::string>(name + ".value_str");
                bool byte_swap = false;
                if (m_broker.has_preset_value(name + ".byte_swap")) {
                    byte_swap = cci_get<bool>(name + ".byte_swap");
                }
                if (gs::loader::log_enabled) {
                    SC_REPORT_INFO("Loader", ("Loading csv file " + file).c_str());
                }
                csv_load(file, addr, addr_str, val_str, byte_swap);
                read = true;
            }
            if (m_broker.has_preset_value(name + ".param")) {
                std::string param = cci_get<std::string>(name + ".param");
                cci::cci_param_typed_handle<std::string> data(m_broker.get_param_handle(param));
                if (!data.is_valid()) {
                    SC_REPORT_FATAL("Loader", ("Unable to find valid source param " + param).c_str());
                }
                if (gs::loader::log_enabled) {
                    SC_REPORT_INFO("Loader", ("Loading string parameter " + param).c_str());
                }
                str_load(data.get_value(), addr);
                read = true;
            }
            if (sc_cci_children((name + ".data").c_str()).size()) {
                bool byte_swap = false;
                if (m_broker.has_preset_value(name + ".byte_swap")) {
                    byte_swap = cci_get<bool>(name + ".byte_swap");
                }
                if (gs::loader::log_enabled) {
                    SC_REPORT_INFO("Loader", "Loading config data ");
                }
                data_load(name + ".data", addr, byte_swap);
                read = true;
            }
        }
        if (!read) {
            SC_REPORT_FATAL("Loader","Unknown loader type");
        }
    }
    void end_of_elaboration()
    {
        for (std::string s : sc_cci_children(name())) {
            if (std::count_if(s.begin(), s.end(),
                    [](unsigned char c) { return std::isdigit(c); })) {
                load(std::string(name()) + "." + s);
            } else {
                load(name());
                break;
            }
        }
    }

public:
    /**
     * @brief This function reads a file into memory and can be used to load an image.
     *
     * @param filename Name of the file
     * @param addr the address where the memory file is to be read
     * @return size_t
     */
    void file_load(std::string filename, uint64_t addr)
    {
        std::ifstream fin(filename, std::ios::in | std::ios::binary);
        if (!fin.good()) {
            printf("Memory::load(): error file not found (%s)\n", filename.c_str());
            exit(1);
        }

        uint8_t buffer[1024];
        uint64_t c = 0;
        while (!fin.eof()) {
            fin.read(reinterpret_cast<char*>(&buffer[0]), 1024);
            size_t r = fin.gcount();
            send(addr + c, buffer, r);
            c += r;
        }
        fin.close();
    }

    void csv_load(std::string filename, uint64_t offset, std::string addr_str, std::string value_str, bool byte_swap)
    {
        std::ifstream file(filename);

        CSVRow row;
        if (!(file >> row)) {
            SC_REPORT_FATAL("Loader", ("Unable to find " + filename).c_str());
        }
        int addr_i = -1;
        int value_i = -1;
        for (int i = 0; i < row.size(); i++) {
            if (row[i] == addr_str) {
                addr_i = i;
            }
            if (row[i] == value_str) {
                value_i = i;
            }
        }
        if (addr_i == -1 || value_i == -1) {
            SC_REPORT_FATAL("Loader", ("Unable to find " + filename).c_str());
        }
        while (file >> row) {
            const std::string addr = std::string(row[addr_i]);
            uint32_t value = std::stoi(std::string(row[value_i]));
            if (byte_swap)
                value = byte_swap32(value);
            send(std::stoi(addr), (uint8_t*)(&value), sizeof(value));
        }
    }


private:
// it makes no sense to expose this function, you could only sensibly use it from a config anyway
    void data_load(std::string param, uint64_t addr, bool byte_swap)
    {
        for (std::string s : sc_cci_children(param.c_str())) {
            if (std::count_if(s.begin(), s.end(),
                    [](unsigned char c) { return std::isdigit(c); })) {
                union {
                    uint32_t d;
                    uint8_t b[sizeof(uint32_t)];
                } data;
                data.d = cci_get<uint32_t>(param + "." + std::string(s));
                if (byte_swap) {
                    data.d = byte_swap32(data.d);
                }
                send(addr + (stoi(s) * 0x4), &(data.b[0]), 4);
            } else {
                SC_REPORT_FATAL("Loader", "unknown format for parameter load");
                break;
            }
        }
    }
public:

    void str_load(std::string data, uint64_t addr)
    {
        send(addr, reinterpret_cast<uint8_t*>(const_cast<char*>(data.c_str())), data.length());
    }

    void ptr_load(uint8_t *data, uint64_t addr, uint64_t len)
    {
        send(addr, data, len);
    }

    void elf_load(const std::string& path)
    {
        elf_reader(path, [&](uint64_t addr, uint8_t* data, uint64_t len) -> void { send(addr, data, len); });
    }

    /* Elf reader helper class */
private:
    /* elf loading */
    typedef enum _endianess {
        ENDIAN_UNKNOWN = 0,
        ENDIAN_LITTLE = 1,
        ENDIAN_BIG = 2,
    } endianess;

    struct elf_segment {
        const uint64_t virt;
        const uint64_t phys;
        const uint64_t size;
        const uint64_t filesz;
        const uint64_t offset;
        const bool r, w, x;
    };

    class elf_reader {
    private:
        std::string m_filename;
        int m_fd;
        uint64_t m_entry;
        uint64_t m_machine;
        endianess m_endian;

        std::vector<struct elf_segment> m_segments;

        std::function<void(uint64_t, uint8_t*, uint64_t)> m_send;

    public:
        uint64_t entry() const { return m_entry; }
        uint64_t machine() const { return m_machine; }

        endianess endian() const { return m_endian; }
        bool is_little_endian() const { return m_endian == ENDIAN_LITTLE; }
        bool is_big_endian() const { return m_endian == ENDIAN_BIG; }

        const char* filename() const { return m_filename.c_str(); }

        const std::vector<elf_segment>& segments() const { return m_segments; }

        elf_reader(const elf_reader&) = delete;

        struct elf32_traits {
            typedef Elf32_Ehdr Elf_Ehdr;
            typedef Elf32_Phdr Elf_Phdr;
            typedef Elf32_Shdr Elf_Shdr;
            typedef Elf32_Sym Elf_Sym;

            static Elf_Ehdr* elf_getehdr(Elf* elf)
            {
                return elf32_getehdr(elf);
            }

            static Elf_Phdr* elf_getphdr(Elf* elf)
            {
                return elf32_getphdr(elf);
            }

            static Elf_Shdr* elf_getshdr(Elf_Scn* scn)
            {
                return elf32_getshdr(scn);
            }
        };

        struct elf64_traits {
            typedef Elf64_Ehdr Elf_Ehdr;
            typedef Elf64_Phdr Elf_Phdr;
            typedef Elf64_Shdr Elf_Shdr;
            typedef Elf64_Sym Elf_Sym;

            static Elf_Ehdr* elf_getehdr(Elf* elf)
            {
                return elf64_getehdr(elf);
            }

            static Elf_Phdr* elf_getphdr(Elf* elf)
            {
                return elf64_getphdr(elf);
            }

            static Elf_Shdr* elf_getshdr(Elf_Scn* scn)
            {
                return elf64_getshdr(scn);
            }
        };

        static endianess elf_endianess(Elf* elf)
        {
            const char* ident = elf_getident(elf, nullptr);
            if (ident == nullptr)
                return ENDIAN_UNKNOWN;

            switch (ident[EI_DATA]) {
            case ELFDATA2LSB:
                return ENDIAN_LITTLE;
            case ELFDATA2MSB:
                return ENDIAN_BIG;
            default:
                return ENDIAN_UNKNOWN;
            }
        }

        template <typename T>
        static std::vector<elf_segment> elf_segments(Elf* elf)
        {
            size_t count = 0;
            int err = elf_getphdrnum(elf, &count);
            if (err)
                SC_REPORT_FATAL("Loader", ("elf_begin failed: " + std::string(elf_errmsg(err))).c_str());

            std::vector<elf_segment> segments;
            typename T::Elf_Phdr* hdr = T::elf_getphdr(elf);
            for (uint64_t i = 0; i < count; i++, hdr++) {
                if (hdr->p_type == PT_LOAD) {
                    bool r = hdr->p_flags & PF_R;
                    bool w = hdr->p_flags & PF_W;
                    bool x = hdr->p_flags & PF_X;
                    segments.push_back({ hdr->p_vaddr, hdr->p_paddr, hdr->p_memsz,
                        hdr->p_filesz, hdr->p_offset, r, w, x });
                }
            }

            return segments;
        }

        template <typename T, typename ELF>
        void read_sections(ELF* elf)
        {
            Elf_Scn* scn = nullptr;
            while ((scn = elf_nextscn(elf, scn)) != nullptr) {
                typename T::Elf_Shdr* shdr = T::elf_getshdr(scn);
                if (shdr->sh_type != SHT_SYMTAB)
                    continue;

                Elf_Data* data = elf_getdata(scn, nullptr);
                size_t num_symbols = shdr->sh_size / shdr->sh_entsize;
                typename T::Elf_Sym* syms = (typename T::Elf_Sym*)(data->d_buf);

                for (size_t i = 0; i < num_symbols; i++) {
                    if (syms[i].st_size == 0)
                        continue;

                    char* name = elf_strptr(elf, shdr->sh_link, syms[i].st_name);
                    if (name == nullptr || strlen(name) == 0)
                        continue;

                    uint64_t size = syms[i].st_size;
                    uint64_t virt = syms[i].st_value;
                    uint64_t phys = to_phys(virt);
                }
            }
        }

        uint64_t to_phys(uint64_t virt) const
        {
            for (auto& seg : m_segments) {
                if ((virt >= seg.virt) && virt < (seg.virt + seg.size))
                    return seg.phys + virt - seg.virt;
            }

            return virt;
        }

        elf_reader(const std::string& path, std::function<void(uint64_t, uint8_t*, uint64_t)> _send)
            : m_filename(path)
            , m_send(_send)
            , m_fd(-1)
            , m_entry(0)
            , m_machine(0)
            , m_endian(ENDIAN_UNKNOWN)
        {
            if (elf_version(EV_CURRENT) == EV_NONE)
                SC_REPORT_FATAL("Loader", "failed to read libelf version");

            m_fd = open(filename(), O_RDONLY, 0);
            if (m_fd < 0)
                SC_REPORT_FATAL("Loader", ("cannot open elf file " + std::string(filename())).c_str());

            Elf* elf = elf_begin(m_fd, ELF_C_READ, nullptr);
            if (elf == nullptr)
                SC_REPORT_FATAL("Loader", ("error reading " + std::string(filename()) + " : " + std::string(elf_errmsg(-1))).c_str());

            if (elf_kind(elf) != ELF_K_ELF)
                SC_REPORT_FATAL("Loader", ("ELF version error in " + std::string(filename())).c_str());

            Elf32_Ehdr* ehdr32 = elf32_getehdr(elf);
            Elf64_Ehdr* ehdr64 = elf64_getehdr(elf);
            if (ehdr32) {
                m_entry = ehdr32->e_entry;
                m_machine = ehdr32->e_machine;
                m_endian = elf_endianess(elf);
                m_segments = elf_segments<elf32_traits>(elf);
                read_sections<elf32_traits>(elf);
            }

            if (ehdr64) {
                m_entry = ehdr64->e_entry;
                m_machine = ehdr64->e_machine;
                m_endian = elf_endianess(elf);
                m_segments = elf_segments<elf64_traits>(elf);
                read_sections<elf64_traits>(elf);
            }

            elf_end(elf);

            for (auto s : m_segments) {
                read_segment(s);
            }
        }

        ~elf_reader()
        {
            if (m_fd >= 0)
                close(m_fd);
        }

        uint64_t read_segment(const elf_segment& segment)
        {
            if (m_fd < 0)
                SC_REPORT_FATAL("Loader", ("ELF file '" + std::string(filename()) + "' not open").c_str());

            if (lseek(m_fd, segment.offset, SEEK_SET) != (ssize_t)segment.offset)
                SC_REPORT_FATAL("Loader", ("cannot seek within ELF file " + std::string(filename())).c_str());

            uint8_t buff[1024];
            size_t sz = segment.filesz;
            size_t offset = 0;
            while (sz) {
                int s = read(m_fd, buff, (sz < sizeof(buff) ? sz : sizeof(buff)));
                sz -= s;
                if (!s) {
                    SC_REPORT_FATAL("Loader", ("cannot read ELF file " + std::string(filename())).c_str());
                    exit(0);
                }
                m_send(segment.phys + offset, buff, s);
                offset += s;
            }
            if (lseek(m_fd, 0, SEEK_SET) != 0)
                SC_REPORT_FATAL("Loader", ("cannot seek within ELF file " + std::string(filename())).c_str());

            return segment.size;
        }
    };
};
}
#endif
