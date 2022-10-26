/*
 * Copyright (c) 2022 Qualcomm Innovation Center, Inc. All Rights Reserved.
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

#ifndef _GREENSOCS_BASE_COMPONENTS_MEMORY_SERVICES_H
#define _GREENSOCS_BASE_COMPONENTS_MEMORY_SERVICES_H

#include <fstream>
#include <memory>

//#include <cci_configuration>
#include <systemc>
//#include <tlm>
//#include <tlm_utils/simple_target_socket.h>
#include <scp/report.h>

//#include "loader.h"

//#include "shmem_extension.h"

#ifndef _WIN32
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#endif
#include <csignal>
#include <cstdlib>
namespace gs {
// Singleton class that handles memory allocation, alignment, file mapping and shared memory

#define ALIGNEDBITS 12
class MemoryServices
{
private:
    MemoryServices()
    {
        signal(SIGABRT, MemoryServices::cleanupsig);
        signal(SIGINT, MemoryServices::cleanupsig);
        signal(SIGKILL, MemoryServices::cleanupsig);
        signal(SIGSEGV, MemoryServices::cleanupsig);
        signal(SIGBUS, MemoryServices::cleanupsig);
    }
    struct shmem_info {
        uint8_t *base;
        size_t size;
    };
    std::map<std::string, shmem_info> m_shmem_info_map;

public:
    ~MemoryServices() { cleanup(); }

    void cleanup()
    {
        for (auto n : m_shmem_info_map) {
            SC_REPORT_INFO("MemoryServices", ("Deleting " + n.first).c_str());
            shm_unlink(n.first.c_str());
        };
    }
    static void cleanupsig(int num)
    {
        std::cerr<<"Received signal "<<num<<"\n";
        MemoryServices::get().cleanup();
        exit(num);
    }
    static MemoryServices& get()
    {
        static MemoryServices instance;
        return instance;
    }
    MemoryServices(MemoryServices const&) = delete;
    void operator=(MemoryServices const&) = delete;

    uint8_t* map_file(const char* mapfile, uint64_t size, uint64_t offset)
    {
        int fd = open(mapfile, O_RDWR);
        if (fd < 0) {
            SC_REPORT_FATAL("MemoryServices", "Unable to find backing file\n");
        }
        uint8_t* ptr = (uint8_t*)mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, offset);
        close(fd);
        if (ptr == MAP_FAILED) {
            SC_REPORT_FATAL("MemoryServices", "Unable to map backing file\n");
        }
        return ptr;
    }

    uint8_t* map_mem_create(const char* memname, uint64_t size)
    {
        assert (m_shmem_info_map.count(memname)==0);
        int fd = shm_open(memname, O_CREAT | O_EXCL | O_RDWR, S_IRUSR | S_IWUSR);
        if (fd == -1) {
            shm_unlink(memname);
            SC_REPORT_FATAL("MemoryServices",
                            ("can't shm_open create " + std::string(memname)).c_str());
        }
        ftruncate(fd, size);
        SC_REPORT_INFO("MemoryServices", ("Length " + std::to_string(size)).c_str());
        uint8_t* ptr = (uint8_t*)mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        close(fd);
        if (ptr == MAP_FAILED) {
            shm_unlink(memname);
            SC_REPORT_FATAL("MemoryServices", ("can't mmap(shared memory create) " +
                                               std::string(memname) + " " + std::to_string(errno))
                                                  .c_str());
        }
        SC_REPORT_INFO("MemoryServices", ("Shared memory created: " + std::string(memname) +
                                          " length " + std::to_string(size) + " ")
                                             .c_str());
        m_shmem_info_map.insert({ std::string(memname), { ptr, size } });
        return ptr;
    }

    uint8_t* map_mem_join(const char* memname, size_t size)
    {
        auto cache = m_shmem_info_map.find(memname);
        if (cache != m_shmem_info_map.end()) {
            assert(cache->second.size == size);
            return cache->second.base;
        }

        int fd = shm_open(memname, O_RDWR, S_IRUSR | S_IWUSR);
        if (fd == -1) {
            shm_unlink(memname);
            SC_REPORT_FATAL("MemoryServices",
                            ("can't shm_open join " + std::string(memname)).c_str());
        }
        ftruncate(fd, size);
        SC_REPORT_INFO("MemoryServices", ("Length " + std::to_string(size)).c_str());
        uint8_t* ptr = (uint8_t*)mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        close(fd);
        if (ptr == MAP_FAILED) {
            shm_unlink(memname);
            SC_REPORT_FATAL("MemoryServices", ("can't mmap(shared memory join) " +
                                               std::string(memname) + " " + std::to_string(errno))
                                                  .c_str());
        }
        m_shmem_info_map.insert({std::string(memname), {ptr, size}});
        return ptr;
    }

    uint8_t* alloc(uint64_t size)
    {
        if ((size & ((1 << ALIGNEDBITS) - 1)) == 0) {
            uint8_t* ptr = static_cast<uint8_t*>(aligned_alloc((1 << ALIGNEDBITS), size));
            if (ptr) {
                return ptr;
            }
        }
        SC_REPORT_INFO("MemoryServices", "Aligned allocation failed, using normal allocation");
        {
            uint8_t* ptr = (uint8_t*)malloc(size);
            if (ptr) {
                return ptr;
            }
        }
        return nullptr;
    }
};
} // namespace gs
#endif