/*
 * This file is part of libqemu-cxx
 * Copyright (c) 2022-2023 Qualcomm Innovation Center, Inc. All Rights Reserved.
 * Author: GreenSocs 2021
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <cstring>
#include <fstream>
#include <sstream>
#include <string>
#include <iostream>
#include <map>
#include <atomic>
#include <filesystem>

#include <libqemu-cxx/loader.h>

#if defined(_WIN32)
#include <Lmcons.h>
#include <windows.h>
#else
#include <dlfcn.h>
#include <iostream>
#include <sys/stat.h>
#include <unistd.h>
#if defined(__APPLE__)
#include <mach-o/dyld.h>
#else
#include <link.h>
#endif
#endif

namespace fs = std::filesystem;

class Library : public qemu::LibraryIface
{
private:
    void* m_lib;

public:
    Library(void* lib): m_lib(lib) {}

    bool symbol_exists(const char* name) { return get_symbol(name) != NULL; }

    void* get_symbol(const char* name)
    {
        void* symbol = nullptr;
#if defined(_WIN32)
        symbol = reinterpret_cast<void*>(GetProcAddress((HMODULE)m_lib, name));
#else
        symbol = dlsym(m_lib, name);
#endif
        return symbol;
    }

    ~Library()
    {
    }
};

class DefaultLibraryLoader : public qemu::LibraryLoaderIface
{
private:
    std::string m_last_error;

    bool is_library_loaded(const std::string& lib_path)
    {
#if defined(_WIN32)
        // Extract just the filename from the path for GetModuleHandle
        std::string filename = fs::path(lib_path).filename().string();
        HMODULE handle = GetModuleHandleA(filename.c_str());
        return (handle != nullptr);
#else
        void* handle = dlopen(lib_path.c_str(), RTLD_NOLOAD | RTLD_NOW);
        if (handle != nullptr) {
            // Matching dlclose is needed to decrement the reference count
            dlclose(handle);
            return true;
        }
        return false;
#endif
    }

    const char* dlpath(void* handle)
    {
        const char* path = NULL;
#ifdef __APPLE__
        for (int32_t i = _dyld_image_count(); i >= 0; i--) {
            bool found = FALSE;
            const char* probe_path = _dyld_get_image_name(i);
            void* probe_handle = dlopen(probe_path, RTLD_NOW | RTLD_LOCAL | RTLD_NOLOAD);

            if (handle == probe_handle) {
                found = TRUE;
                path = probe_path;
            }

            dlclose(probe_handle);

            if (found) break;
        }
#elif defined(__linux__)
        struct link_map* map;
        dlinfo(handle, RTLD_DI_LINKMAP, &map);

        if (map) path = map->l_name;
#elif defined(_WIN32)
        // Not applicable on Windows
        path = nullptr;
#endif
        return path;
    }

    void* load_library_internal(const std::string& lib_name)
    {
        void* handle = nullptr;
#if defined(_WIN32)
        handle = LoadLibraryExA(lib_name.c_str(), NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
#else
        handle = dlopen(lib_name.c_str(), RTLD_NOW);
#endif
        return handle;
    }

    fs::path get_loaded_library_path(const std::string& lib_name)
    {
#ifdef _WIN32
        HMODULE handle = GetModuleHandleA(lib_name.c_str());
        if (handle == nullptr) {
            m_last_error = "get_loaded_library_path: Library " + lib_name + " is not loaded.";
            return "";
        }

        char path[MAX_PATH];
        if (GetModuleFileNameA(handle, path, MAX_PATH) == 0) {
            m_last_error = "Failed to get the loaded library path.";
            return "";
        }

        return fs::path(path);
#else
        void* handle = dlopen(lib_name.c_str(), RTLD_NOLOAD | RTLD_NOW);
        if (handle == nullptr) {
            m_last_error = "get_loaded_library_path: Library " + lib_name + " is not loaded.";
            return "";
        }

        const char* path = dlpath(handle);
        dlclose(handle);

        if (path == nullptr) {
            m_last_error = "Failed to get the loaded library path.";
            return "";
        }

        return fs::path(path);
#endif
    }

    fs::path create_temp_path(const std::string& libname)
    {
        static std::atomic<uint64_t> counter{ 0 };
        fs::path temp_dir = fs::temp_directory_path();
        std::string filename = fs::path(libname).filename().string();
        uint64_t count = counter.fetch_add(1);
        std::string temp_filename = std::to_string(count) + "_" + filename;
        return temp_dir / temp_filename;
    }

    qemu::LibraryLoaderIface::LibraryIfacePtr duplicate_and_load_library(const std::string& lib_name)
    {
        fs::path original_path(get_loaded_library_path(lib_name));
        if (original_path.empty()) {
            return nullptr;
        }

        fs::path temp_path = create_temp_path(lib_name);

        try {
            fs::copy_file(original_path, temp_path, fs::copy_options::overwrite_existing);
        } catch (const fs::filesystem_error& e) {
            m_last_error = "Failed to create a copy of the library: " + std::string(e.what());
            return nullptr;
        }

        // Load the copied library
        void* handle = load_library_internal(temp_path.string());
        if (handle == nullptr) {
            m_last_error = get_last_error();
            // Clean up the temporary file
            fs::remove(temp_path);
            return nullptr;
        }

        return std::make_shared<Library>(handle);
    }

public:
    qemu::LibraryLoaderIface::LibraryIfacePtr load_library(const std::string& lib_name, bool load_as_separate_instance)
    {
        // If a separate instance of the shared library is requested and the library has been already loaded
        // the library must be copied into a new location and loaded from there.
        if (load_as_separate_instance && is_library_loaded(lib_name)) {
            std::cout << "Loading separate instance of library: " << lib_name << std::endl;
            return duplicate_and_load_library(lib_name);
        }

        // Load the library
        void* handle = load_library_internal(lib_name);
        if (handle == nullptr) {
            m_last_error = get_last_error();
            return nullptr;
        }

        return std::make_shared<Library>(handle);
    }

    const char* get_lib_ext()
    {
#if defined(_WIN32)
        return "dll";
#elif defined(__APPLE__)
        return "dylib";
#else
        return "so";
#endif
    }

    const char* get_last_error() { return m_last_error.c_str(); }
};

qemu::LibraryLoaderIface& qemu::get_default_lib_loader()
{
    static DefaultLibraryLoader loader;
    return loader;
}
