/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "memory_services.h"

#ifndef _WIN32

gs::ShmemCleanerService::ShmemCleanerService(SharedMemoryCleaner* shm_registry_mgr)
    : m_shm_registry_mgr(shm_registry_mgr)
{
    SCP_WARN(()) << "ShmemCleanerService constructor called";

    m_cpid = fork();
    if (m_cpid > 0) {
        pahandler.setup_parent_conn_checker();
        SigHandler::get().set_nosig_chld_stop();
        SigHandler::get().add_sigchld_handler(Handler_CB::EXIT);
    } else if (m_cpid == 0) {
        /**
         * these signals are already handled on the parent process
         * block them here and wait for parent exit.
         */
        SigHandler::get().block_curr_handled_signals();
        pahandler.check_parent_conn_sth([this]() { this->cleanup_routine(); });
    } else {
        SCP_FATAL(()) << "failed to fork (shm_cleaner) child process, error: " << std::strerror(errno);
    }
}

void gs::ShmemCleanerService::cleanup_routine()
{
    /*Function must be called from the fork*/
    assert(m_cpid == 0);

    m_shm_registry_mgr->cleanup();
    _Exit(EXIT_SUCCESS);
}

gs::SharedMemoryCleaner::SharedMemoryCleaner()
{
    void* shared_memory_registry = mmap(NULL, sizeof(NameRegistry), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS,
                                        -1, 0);
    assert(shared_memory_registry != nullptr);
    m_named_shm_registry = static_cast<NameRegistry*>(shared_memory_registry);
}

gs::SharedMemoryCleaner::~SharedMemoryCleaner()
{
    if (munmap(m_named_shm_registry, sizeof(m_named_shm_registry)) == -1) {
        SCP_FATAL(()) << "failed to munmap shm_cleaner_info struct at: 0x" << std::hex << m_named_shm_registry
                      << " with size: 0x" << std::hex << sizeof(m_named_shm_registry)
                      << " error: " << std::strerror(errno);
    }
}

void gs::SharedMemoryCleaner::add_shared_memory_region(const std::string& memname)
{
    if (is_full())
        SCP_FATAL(()) << "can't shm_open create " << memname << ", exceeded: " << MAX_SHM_SEGS_NUM << std::endl;

    if (!is_name_valid(memname))
        SCP_FATAL(()) << "shm name length exceeded max allowed length: " << MAX_SHM_STR_LENGTH << std::endl;

    ensure_shm_cleaner_service();
    strncpy(m_named_shm_registry->name[m_named_shm_registry->count++], memname.c_str(), memname.size() + 1);
}

void gs::SharedMemoryCleaner::cleanup()
{
    for (int i = 0; i < get_count(); i++) {
        std::cerr << "shm_cleaner (" << getpid() << ")  Deleting : " << m_named_shm_registry->name[i] << std::endl;
        shm_unlink(m_named_shm_registry->name[i]);
    }
}
uint8_t* gs::MemoryServices::map_mem_create(const std::string& memname, uint64_t size)
{
    assert(m_shmem_desc_map.count(memname) == 0);

    int fd = shm_open(memname.c_str(), O_CREAT | O_EXCL | O_RDWR, S_IRUSR | S_IWUSR);
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

    uint8_t* ptr = map_fd_to_mem(memname, fd, size, MAP_SHARED);

    m_shm_registry_mgr.add_shared_memory_region(memname);
    m_shmem_desc_map.emplace(memname, SharedMemoryDescriptor(fd, ptr, size));

    return ptr;
}

uint8_t* gs::MemoryServices::map_mem_join_new(const std::string& memname, size_t size)
{
    int fd = shm_open(memname.c_str(), O_RDWR, S_IRUSR | S_IWUSR);
    if (fd == -1) {
        die_sys_api(errno, memname, "can't shm_open join");
    }
    SCP_INFO(()) << "Join Length " << size;

    uint8_t* ptr = map_fd_to_mem(memname, fd, size, MAP_SHARED);

    m_shmem_desc_map.emplace(memname, SharedMemoryDescriptor(fd, ptr, size));

    close(fd);
    return ptr;
}

uint8_t* gs::MemoryServices::map_fd_to_mem(const std::string& memname, int fd, size_t size, int mmap_flags)
{
    uint8_t* ptr = (uint8_t*)mmap(NULL, size, PROT_READ | PROT_WRITE, mmap_flags, fd, 0);
    int mmap_error = errno;
    if (ptr == MAP_FAILED) {
        close(fd);
        die_sys_api(mmap_error, memname, "can't mmap(shared memory)");
    }
    return ptr;
}

uint8_t* gs::MemoryServices::map_file(const std::string& mapfile, uint64_t size, uint64_t offset)
{
    int fd = open(mapfile.c_str(), O_RDWR);
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

void gs::MemoryServices::unmap_file(uint8_t* ptr, size_t size)
{
    if (ptr != nullptr) {
        munmap(ptr, size);
    }
}

void gs::MemoryServices::die_sys_api(int error, const std::string& memname, const std::string& die_msg)
{
    if (shm_unlink(memname.c_str()) == -1) perror("shm_unlink");
    SCP_FATAL(()) << die_msg << " " << memname << " [Error: " << strerror(error) << "]";
}

void gs::MemoryServices::cleanup()
{
    for (auto n : m_shmem_desc_map) {
        SCP_INFO(()) << "Deleting " << n.first; // can't use SCP_ in global destructor
                                                // as it's probably already destroyed
        shm_unlink(n.first.c_str());
    };
}

#else

#include <io.h>

uint8_t* gs::MemoryServices::map_mem_create(const std::string& memname, uint64_t size)
{
    assert(m_shmem_desc_map.count(memname.c_str()) == 0);

    DWORD sizeHigh = static_cast<DWORD>(size >> 32);
    DWORD sizeLow = static_cast<DWORD>(size);

    HANDLE hMapFile = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, sizeHigh, sizeLow,
                                         memname.c_str());
    if (hMapFile == NULL) {
        die_sys_api(GetLastError(), memname, "Unable to create file mapping for shared memory");
    }

    void* pBuf = MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, 0);
    if (pBuf == NULL) {
        die_sys_api(GetLastError(), memname, "Unable to map view of file for shared memory");
    }

    int fd = _open_osfhandle(reinterpret_cast<uintptr_t>(hMapFile), 0);
    m_shmem_desc_map.emplace(memname.c_str(), SharedMemoryDescriptor(fd, static_cast<uint8_t*>(pBuf), size));

    return static_cast<uint8_t*>(pBuf);
}

uint8_t* gs::MemoryServices::map_mem_join_new(const std::string& memname, size_t size)
{
    HANDLE hMapFile = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, memname.c_str());
    if (hMapFile == NULL) {
        die_sys_api(GetLastError(), memname, "Unable to open file mapping for shared memory");
    }

    LPVOID pBuf = MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, size);
    if (pBuf == NULL) {
        die_sys_api(GetLastError(), memname, "Unable to map view of file for shared memory");
    }

    int fd = _open_osfhandle(reinterpret_cast<uintptr_t>(hMapFile), 0);
    m_shmem_desc_map.emplace(memname, SharedMemoryDescriptor(fd, static_cast<uint8_t*>(pBuf), size));

    return static_cast<uint8_t*>(pBuf);
}

uint8_t* gs::MemoryServices::map_file(const std::string& mapfile, uint64_t size, uint64_t offset)
{
    HANDLE hFile = CreateFileA(mapfile.c_str(), GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING,
                               FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        die_sys_api(GetLastError(), mapfile, "Unable to open backing file");
    }

    HANDLE hMapFile = CreateFileMappingA(hFile, NULL, PAGE_READWRITE, 0, 0, NULL);
    if (hMapFile == NULL) {
        die_sys_api(GetLastError(), mapfile, "Unable to create file mapping");
    }

    DWORD offsetLow = static_cast<DWORD>(offset);
    DWORD offsetHigh = static_cast<DWORD>(offset >> 32);

    LPVOID pBuf = MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, offsetLow, offsetHigh, size);
    if (pBuf == NULL) {
        die_sys_api(GetLastError(), mapfile, "Unable to map view of file");
    }

    return static_cast<uint8_t*>(pBuf);
}

void gs::MemoryServices::unmap_file(uint8_t* ptr, size_t size)
{
    (void)size;
    if (ptr != nullptr) {
        UnmapViewOfFile(ptr);
    }
}

void gs::MemoryServices::die_sys_api(int error, const std::string& memname, const std::string& die_msg)
{
    SCP_FATAL(()) << " Resource: " << memname << ", Error number: " << error << ", Error msg: " << die_msg;
}

void gs::MemoryServices::cleanup() {}

/* Windows/MSCV provides _aligned_malloc() instead of aligned_alloc() */
inline void* aligned_alloc(size_t alignment, size_t size) { return _aligned_malloc(size, alignment); }
#endif // #ifndef _WIN32

gs::MemoryServices::MemoryServices(): m_name("MemoryServices")
{
    SCP_DEBUG(()) << "MemoryServices constructor";
    SigHandler::get().register_on_exit_cb("gs::MemoryServices::cleanupexit", MemoryServices::cleanupexit);
    SigHandler::get().add_sigint_handler(Handler_CB::PASS);
}

gs::MemoryServices::~MemoryServices()
{
    SCP_DEBUG(()) << "MemoryServices Destructor";
    SigHandler::get().deregister_on_exit_cb("gs::MemoryServices::cleanupexit");
    cleanup();
}

const char* gs::MemoryServices::name() const { return m_name.c_str(); }

size_t gs::MemoryServices::get_shmem_seg_num() const { return m_shmem_desc_map.size(); }

void gs::MemoryServices::cleanupexit() { MemoryServices::get().cleanup(); }
void gs::MemoryServices::init() { SCP_DEBUG(()) << "Memory Services Initialization"; }
gs::MemoryServices& gs::MemoryServices::get()
{
    static MemoryServices instance;
    return instance;
}

uint8_t* gs::MemoryServices::map_mem_join(const std::string& memname, size_t size)
{
    auto cache = m_shmem_desc_map.find(memname);
    if (cache != m_shmem_desc_map.end()) {
        assert(cache->second.size == size);
        return cache->second.addr;
    }

    return map_mem_join_new(memname, size);
}

uint8_t* gs::MemoryServices::map_mem_create(const std::string& memname, uint64_t size, int* fd)
{
    uint8_t* addr = map_mem_create(memname, size);
    if (addr == nullptr) {
        *fd = get_shmem_fd(memname);
    }

    return addr;
}

int gs::MemoryServices::get_shmem_fd(const std::string& memname)
{
    auto it = m_shmem_desc_map.find(memname);
    if (it != m_shmem_desc_map.end()) {
        return it->second.file_descriptor;
    }
    return -1;
}

gs::AllocatedMemory gs::MemoryServices::alloc(uint64_t size)
{
    gs::AllocatedMemory alloc_mem;

    if ((size & ((1 << ALIGNEDBITS) - 1)) == 0) {
        uint8_t* ptr = static_cast<uint8_t*>(aligned_alloc((1 << ALIGNEDBITS), size));
        if (ptr) {
            alloc_mem.ptr = ptr;
            alloc_mem.is_aligned = true;
        }
    } else {
        SCP_INFO(()) << "Aligned allocation failed, using normal allocation";
        uint8_t* ptr = (uint8_t*)malloc(size);
        if (ptr) {
            alloc_mem.ptr = ptr;
        }
    }
    return alloc_mem;
}