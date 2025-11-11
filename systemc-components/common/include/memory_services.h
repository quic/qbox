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

#if defined(_WIN32)
#include <windows.h>
#else
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

#define ALIGNEDBITS 12

/* Structure to hold information about allocated memory address and alignment.
Alignment flag is needed on Windows where memory allocated using _aligned_malloc
must be freed using _aligned_free.
 */
struct AllocatedMemory {
    uint8_t* ptr = nullptr;
    bool is_aligned = false;
};

#ifndef _WIN32
// Memory cleaner service is not needed on Windows
#define MAX_SHM_STR_LENGTH 255
#define MAX_SHM_SEGS_NUM   1024

/**
 * @brief Structure to hold an array of names for shared memory usage.
 *
 * This structure maintains a registry of names, where each name is stored as a character array.
 * It is designed to be mapped to a shared memory region, allowing multiple processes to access
 * and share the list of names concurrently. The 'count' field indicates the number of valid names
 * currently stored in the registry.
 */
struct NameRegistry {
    uint32_t count;
    char name[MAX_SHM_SEGS_NUM][MAX_SHM_STR_LENGTH];
};

class SharedMemoryCleaner;

class ShmemCleanerService
{
    SCP_LOGGER((), "ShmemCleanerService");

public:
    ShmemCleanerService(SharedMemoryCleaner* shm_registry_mgr);
    ~ShmemCleanerService() = default;

private:
    /* Function called from the fork. Insure that shared memory is cleaned before exit*/
    void cleanup_routine();

    ProcAliveHandler pahandler;
    pid_t m_cpid = -1;
    SharedMemoryCleaner* m_shm_registry_mgr;
};

/**
 * @brief Forks a process to ensure shared memory cleanup on crash or abnormal exit.
 *
 * The ShmemCleanerService class is responsible for creating a child process (via fork)
 * that monitors the parent process. It maintains access to the shared memory name registry
 * and, in the event of a parent process crash or abnormal termination, ensures that all
 * registered shared memory objects are properly released (unlinked). This mechanism helps
 * prevent resource leaks and guarantees that shared memory segments do not persist after
 * unexpected failures, improving system robustness and reliability.
 */

class SharedMemoryCleaner
{
    SCP_LOGGER((), "SharedMemoryCleaner");

public:
    SharedMemoryCleaner();

    ~SharedMemoryCleaner();

    void add_shared_memory_region(const std::string& memname);

    void cleanup();

private:
    bool is_full() const { return get_count() == MAX_SHM_SEGS_NUM; }

    bool is_name_valid(const std::string& name) const { return name.size() < MAX_SHM_STR_LENGTH; }

    uint32_t get_count() const { return m_named_shm_registry->count; }

    void ensure_shm_cleaner_service()
    {
        // Initialize the shared memory cleaner manager if not already initialized
        if (m_shm_cleaner_service == nullptr) m_shm_cleaner_service = std::make_unique<ShmemCleanerService>(this);
    }

    NameRegistry* m_named_shm_registry;
    std::unique_ptr<ShmemCleanerService> m_shm_cleaner_service;
};
#endif // ifndef WIN32

class MemoryServices
{
    SCP_LOGGER((), "MemoryServices");

private:
    MemoryServices();

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

    void cleanup();

    static void cleanupexit();

    void die_sys_api(int error, const std::string& memname, const std::string& die_msg);

    struct SharedMemoryDescriptor {
        SharedMemoryDescriptor(int file_descriptor, uint8_t* addr, size_t size)
            : file_descriptor(file_descriptor), addr(addr), size(size)
        {
        }

        const int file_descriptor;
        uint8_t* const addr;
        const size_t size;
    };

    std::string m_name;
    std::map<std::string, SharedMemoryDescriptor> m_shmem_desc_map;

#ifndef _WIN32
    void initialize_shm_cleaner_service();
    uint8_t* map_fd_to_mem(const std::string& memname, int fd, size_t size, int mmap_flags);

    SharedMemoryCleaner m_shm_registry_mgr;
#endif

public:
    ~MemoryServices();

    const char* name() const;

    size_t get_shmem_seg_num() const;

    void init();

    static MemoryServices& get();

    MemoryServices(MemoryServices const&) = delete;
    void operator=(MemoryServices const&) = delete;

    int get_shmem_fd(const std::string& memname);

    uint8_t* map_file(const std::string& mapfile, uint64_t size, uint64_t offset);

    void unmap_file(uint8_t* ptr, size_t size);

    uint8_t* map_mem_create(const std::string& memname, uint64_t size);

    uint8_t* map_mem_create(const std::string& memname, uint64_t size, int* fd);

    uint8_t* map_mem_join(const std::string& memname, size_t size);

    uint8_t* map_mem_join_new(const std::string& memname, size_t size);

    AllocatedMemory alloc(uint64_t size);
};

} // namespace gs

#endif // #_GREENSOCS_BASE_COMPONENTS_MEMORY_SERVICES_H