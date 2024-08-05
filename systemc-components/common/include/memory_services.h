/*
 * Copyright (c) 2022-2024 Qualcomm Innovation Center, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
#include <uutils.h>
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
    MemoryServices();

    void die_sys_api(int error, const char* memname, const std::string& die_msg);

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
    ~MemoryServices();

    const char* name() const;

    size_t get_shmem_seg_num() const;

    void cleanup();

    static void cleanupexit();

    void init();

    static MemoryServices& get();

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
    void start_shm_cleaner_proc();

    uint8_t* map_file(const char* mapfile, uint64_t size, uint64_t offset);

    uint8_t* map_mem_create(const char* memname, uint64_t size, int* o_fd);

    uint8_t* map_mem_join(const char* memname, size_t size);

    uint8_t* alloc(uint64_t size);
};
} // namespace gs
#endif