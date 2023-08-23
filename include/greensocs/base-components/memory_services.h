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

#include <cci_configuration>
#include <systemc>

#include <scp/report.h>

#ifndef _WIN32
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#endif
#include <csignal>
#include <cstdlib>
#include <greensocs/gsutils/uutils.h>
#include <cerrno>
#include <cstring>

namespace gs {
// Singleton class that handles memory allocation, alignment, file mapping and shared memory

#define ALIGNEDBITS        12
#define MAX_SHM_STR_LENGTH 255
#define MAX_SHM_SEGS_NUM   1024

class MemoryServices
{
    SCP_LOGGER((), "MemoryServices");

private:
    MemoryServices(): m_name("MemoryServices")
    {
        SCP_DEBUG(()) << "MemoryServices constructor";
        SigHandler::get().register_on_exit_cb(MemoryServices::cleanupexit);
        SigHandler::get().add_sig_handler(SIGINT, SigHandler::Handler_CB::PASS);
    }

    void die_sys_api(int error, const char* memname, const std::string& die_msg)
    {
        if (shm_unlink(memname) == -1) perror("shm_unlink");
        SCP_FATAL(()) << die_msg << " " << memname << " [Error: " << strerror(error) << "]";
    }

    struct shmem_info {
        uint8_t* base;
        size_t size;
    };
    struct shm_cleaner_info {
        int count;
        char name[MAX_SHM_SEGS_NUM][MAX_SHM_STR_LENGTH];
    };
    std::map<std::string, shmem_info> m_shmem_info_map;
    bool finished = false;
    bool child_cleaner_forked = false;
    pid_t m_cpid;
    ProcAliveHandler pahandler;
    shm_cleaner_info* cl_info = nullptr;
    std::string m_name;

public:
    ~MemoryServices()
    {
        SCP_DEBUG(()) << "MemoryServices Destructor";
        cleanup();
        if (cl_info) {
            if (munmap(cl_info, sizeof(shm_cleaner_info)) == -1) {
                SCP_FATAL(()) << "failed to munmap shm_cleaner_info struct at: 0x" << std::hex << cl_info
                              << " with size: 0x" << std::hex << sizeof(shm_cleaner_info)
                              << " error: " << std::strerror(errno);
            }
            cl_info = nullptr;
        }
    }

    const char* name() const { return m_name.c_str(); }

    void cleanup()
    {
        if (finished || !child_cleaner_forked) return;
        for (auto n : m_shmem_info_map) {
            SCP_INFO(()) << "Deleting " << n.first; // can't use SCP_ in global destructor
                                                    // as it's probably already destroyed
            shm_unlink(n.first.c_str());
        };
        if (cl_info) cl_info->count = 0;
        finished = true;
    }

    static void cleanupexit() { MemoryServices::get().cleanup(); }
    void init() { SCP_DEBUG(()) << "Memory Services Initialization"; }
    static MemoryServices& get()
    {
        static MemoryServices instance;
        return instance;
    }
    MemoryServices(MemoryServices const&) = delete;
    void operator=(MemoryServices const&) = delete;

    /**
     * fork a new child process to cleanup the shared memory opened
     * in this parent process in case the parent process was killed gracefully
     * using kill -9 (SIGKILL) or in case of uncaught run time error. For the child process
     * to know that the parent was killed, an unmamed unix domain socket will be
     * opened in the parent and the child processes, close one of the socket fds in
     * the parent and close the other in the child, and poll for POLLHUB in the
     * child to detect that the parent is hanging (may be because it is killed),
     * in this case use the m_shmem_info_map shread with the child to clean
     * all the opened shm instances.
     */
    void start_shm_cleaner_proc()
    {
        if (child_cleaner_forked) return;
        cl_info = (shm_cleaner_info*)mmap(NULL, sizeof(shm_cleaner_info), PROT_READ | PROT_WRITE,
                                          MAP_SHARED | MAP_ANONYMOUS, -1, 0);
        pahandler.init_peer_conn_checker();
        m_cpid = fork();
        if (m_cpid > 0) {
            pahandler.setup_parent_conn_checker();
            SigHandler::get().set_nosig_chld_stop();
            SigHandler::get().add_sig_handler(SIGCHLD, SigHandler::Handler_CB::EXIT);
            child_cleaner_forked = true;
        } else if (m_cpid == 0) { /*child process*/
            std::cerr << "(shm_cleaner) child pid: " << getpid() << std::endl;
            /**
             * these signals are already handled on the parent process
             * block them here and wait for parent exit.
             */
            SigHandler::get().block_curr_handled_signals();
            pahandler.check_parent_conn_sth([&]() {
                std::cerr << "shm_cleaner (" << getpid() << ") count of cl_info shm_names: " << cl_info->count
                          << std::endl;
                for (int i = 0; i < cl_info->count; i++) {
                    std::cerr << "shm_cleaner (" << getpid() << ")  Deleting : " << cl_info->name[i] << std::endl;
                    shm_unlink(cl_info->name[i]);
                }
                if (munmap(cl_info, sizeof(shm_cleaner_info)) == -1) {
                    perror("munmap");
                    exit(EXIT_FAILURE);
                }
            });

        } // else child process
        else {
            SCP_FATAL(()) << "failed to fork (shm_cleaner) child process, error: " << std::strerror(errno);
        }
    } // start_shm_cleaner_proc()

    uint8_t* map_file(const char* mapfile, uint64_t size, uint64_t offset)
    {
        int fd = open(mapfile, O_RDWR);
        if (fd < 0) {
            SCP_FATAL(()) << "Unable to find backing file [Error: " << strerror(errno) << "]";
        }
        uint8_t* ptr = (uint8_t*)mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, offset);
        int mmap_error = errno;
        close(fd);
        if (ptr == MAP_FAILED) {
            SCP_FATAL(()) << "Unable to map backing file [Error: " << strerror(mmap_error) << "]";
        }
        return ptr;
    }

    uint8_t* map_mem_create(const char* memname, uint64_t size)
    {
        if (cl_info && cl_info->count == MAX_SHM_SEGS_NUM)
            SCP_FATAL(()) << "can't shm_open create " << memname << ", exceeded: " << MAX_SHM_SEGS_NUM << std::endl;
        assert(m_shmem_info_map.count(memname) == 0);
        int fd = shm_open(memname, O_CREAT | O_EXCL | O_RDWR, S_IRUSR | S_IWUSR);
        if (fd == -1) {
            die_sys_api(errno, memname, "can't shm_open create");
        }
#if defined(__APPLE__)
        if (ftruncate(fd, size) == -1) {
            die_sys_api(errno, memname, "can't truncate");
        }
#elif defined(__linux__)
        if (fallocate64(fd, 0, 0, size) == -1) {
            die_sys_api(errno, memname, "can't allocate");
        }
#else
        // FIXME: use an equivalent fallocate function for other platforms
        shm_unlink(memname);
        SCP_FATAL(()) << "no fallocate() equivalent function for this platform!";
#endif
        SCP_DEBUG(()) << "Create Length " << size;
        uint8_t* ptr = (uint8_t*)mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        int mmap_error = errno; // even if no error happens here, we need to capture errno directly after mmap()
        close(fd);
        if (ptr == MAP_FAILED) {
            die_sys_api(mmap_error, memname, "can't mmap(shared memory create)");
        }
        SCP_DEBUG(()) << "Shared memory created: " << memname << " length " << size;
        m_shmem_info_map.insert({ std::string(memname), { ptr, size } });
        if ((strlen(memname) + 1) > MAX_SHM_STR_LENGTH)
            SCP_FATAL(()) << "shm name length exceeded max allowed length: " << MAX_SHM_STR_LENGTH << std::endl;
        start_shm_cleaner_proc();
        strncpy(cl_info->name[cl_info->count++], memname,
                strlen(memname) + 1); // must be called after start_shm_cleaner_proc() to make sure
                                      // that cl_info is allocated.
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
            die_sys_api(errno, memname, "can't shm_open join");
        }
        SCP_INFO(()) << "Join Length " << size;
        uint8_t* ptr = (uint8_t*)mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        int mmap_error = errno;
        close(fd);
        if (ptr == MAP_FAILED) {
            die_sys_api(mmap_error, memname, "can't mmap(shared memory join)");
        }
        m_shmem_info_map.insert({ std::string(memname), { ptr, size } });
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
        SCP_INFO(()) << "Aligned allocation failed, using normal allocation";
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