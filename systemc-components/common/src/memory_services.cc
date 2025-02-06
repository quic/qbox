/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "memory_services.h"

gs::MemoryServices::MemoryServices(): m_name("MemoryServices")
{
    SCP_DEBUG(()) << "MemoryServices constructor";
    SigHandler::get().register_on_exit_cb("gs::MemoryServices::cleanupexit", MemoryServices::cleanupexit);
    SigHandler::get().add_sig_handler(SIGINT, SigHandler::Handler_CB::PASS);
}

void gs::MemoryServices::die_sys_api(int error, const char* memname, const std::string& die_msg)
{
    if (shm_unlink(memname) == -1) perror("shm_unlink");
    SCP_FATAL(()) << die_msg << " " << memname << " [Error: " << strerror(error) << "]";
}

gs::MemoryServices::~MemoryServices()
{
    SCP_DEBUG(()) << "MemoryServices Destructor";
    SigHandler::get().deregister_on_exit_cb("gs::MemoryServices::cleanupexit");
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

const char* gs::MemoryServices::name() const { return m_name.c_str(); }

size_t gs::MemoryServices::get_shmem_seg_num() const { return m_shmem_info_map.size(); }

void gs::MemoryServices::cleanup()
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

void gs::MemoryServices::cleanupexit() { MemoryServices::get().cleanup(); }
void gs::MemoryServices::init() { SCP_DEBUG(()) << "Memory Services Initialization"; }
gs::MemoryServices& gs::MemoryServices::get()
{
    static MemoryServices instance;
    return instance;
}

void gs::MemoryServices::start_shm_cleaner_proc()
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
        // std::cerr << "(shm_cleaner) child pid: " << getpid() << std::endl; // Enable for debug
        /**
         * these signals are already handled on the parent process
         * block them here and wait for parent exit.
         */
        SigHandler::get().block_curr_handled_signals();
        pahandler.check_parent_conn_sth([&]() {
            // std::cerr << "shm_cleaner (" << getpid() << ") count of cl_info shm_names: " << cl_info->count <<
            // std::endl; // Enable for debug
            for (int i = 0; i < cl_info->count; i++) {
                std::cerr << "shm_cleaner (" << getpid() << ")  Deleting : " << cl_info->name[i] << std::endl;
                shm_unlink(cl_info->name[i]);
            }
            if (munmap(cl_info, sizeof(shm_cleaner_info)) == -1) {
                perror("munmap");
                _Exit(EXIT_FAILURE);
            }
            _Exit(EXIT_SUCCESS);
        });

    } // else child process
    else {
        SCP_FATAL(()) << "failed to fork (shm_cleaner) child process, error: " << std::strerror(errno);
    }
} // start_shm_cleaner_proc()

uint8_t* gs::MemoryServices::map_file(const char* mapfile, uint64_t size, uint64_t offset)
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

uint8_t* gs::MemoryServices::map_mem_create(const char* memname, uint64_t size, int* o_fd)
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
    *o_fd = fd;
    return ptr;
}

uint8_t* gs::MemoryServices::map_mem_join(const char* memname, size_t size)
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

uint8_t* gs::MemoryServices::alloc(uint64_t size)
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
