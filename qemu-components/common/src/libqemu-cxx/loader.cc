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
#include <filesystem>

#include <libqemu-cxx/loader.h>

namespace fs = std::filesystem;
/*
 * WINDOWS support
 */
#ifdef _WIN32
#include <Lmcons.h>
#include <windows.h>

class Library : public qemu::LibraryIface
{
private:
    HMODULE m_lib;

public:
    Library(HMODULE lib): m_lib(lib) {}

    bool symbol_exists(const char* name) { return get_symbol(name) != NULL; }

    void* get_symbol(const char* name)
    {
        FARPROC symbol = GetProcAddress(m_lib, name);
        return *(void**)(&symbol);
    }

    void unload()
    {
        if (m_lib) {
            FreeLibrary(m_lib);
            m_lib = nullptr;
        }
    }
    ~Library() { unload(); }
};

class DefaultLibraryLoader : public qemu::LibraryLoaderIface
{
private:
    std::string m_last_error;

    std::string get_last_error_as_str()
    {
        // Get the error message, if any.
        DWORD errorMessageID = ::GetLastError();
        if (errorMessageID == 0) return std::string(); // No error message has been recorded

        LPSTR messageBuffer = nullptr;
        size_t size = FormatMessageA(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL,
            errorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL);

        std::string message(messageBuffer, size);

        // Free the buffer.
        LocalFree(messageBuffer);

        return message;
    }

public:
    qemu::LibraryLoaderIface::LibraryIfacePtr load_library(const std::string& lib_name)
    {
        HMODULE handle = LoadLibraryExA(lib_name.c_str(), NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
        if (handle == nullptr) {
            m_last_error = get_last_error_as_str();
            return nullptr;
        }

        return std::make_shared<Library>(handle);
    }

    const char* get_lib_ext() { return "dll"; }

    const char* get_last_error() { return m_last_error.c_str(); }
};

/*
 * LINUX/APPLE support
 */
#else

#include <dlfcn.h>
#include <iostream>
#if defined(__APPLE__)
#include <mach-o/dyld.h>
#else
#include <link.h>
#endif
#include <sys/stat.h>
#include <unistd.h>

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
#else // Linux
    struct link_map* map;
    dlinfo(handle, RTLD_DI_LINKMAP, &map);

    if (map) path = map->l_name;
#endif
    return path;
}

class Library : public qemu::LibraryIface
{
private:
    void* m_lib;

public:
    Library(void* lib): m_lib(lib) {}

    bool symbol_exists(const char* name) { return get_symbol(name) != NULL; }

    void* get_symbol(const char* name) { return dlsym(m_lib, name); }

    void unload()
    {
        if (m_lib) {
            dlclose(m_lib);
            m_lib = nullptr;
        }
    }
    ~Library() { unload(); }
};

class DefaultLibraryLoader : public qemu::LibraryLoaderIface
{
private:
    std::unordered_map<std::string, fs::path> m_loaded_libs;
    std::string m_last_error;

public:
    qemu::LibraryLoaderIface::LibraryIfacePtr load_library(const std::string& lib_name)
    {
        if (m_loaded_libs.count(lib_name) == 0) {
            void* handle = dlopen(lib_name.c_str(), RTLD_LOCAL | RTLD_NOW);
            if (handle == nullptr) {
                m_last_error = dlerror();
                return nullptr;
            }
            m_loaded_libs[lib_name] = fs::path(dlpath(handle));
            return std::make_shared<Library>(handle);
        }

        std::cout << "RE Loading " << m_loaded_libs[lib_name] << "\n";
        char tmp[] = "/tmp/qbox_lib.XXXXXX";
        if (mkstemp(tmp) < 0) {
            m_last_error = "Unable to create temp file";
            return nullptr;
        }

        try {
            fs::copy(m_loaded_libs[lib_name], fs::path(tmp), fs::copy_options::overwrite_existing);
        } catch (const fs::filesystem_error& e) {
            m_last_error = e.what();
            return nullptr;
        }

        void* handle = dlopen(tmp, RTLD_LOCAL | RTLD_NOW);
        if (handle == nullptr) {
            m_last_error = dlerror();
            return nullptr;
        }

#ifndef DEBUG_TMP_LIBRARIES
        remove(tmp);
#else
        std::cout << "WARNING : leaving " << tmp << "in place\n";
#endif
        return std::make_shared<Library>(handle);
    }

    const char* get_lib_ext()
    {
#if defined(__APPLE__)
        return "dylib";
#else
        return "so";
#endif
    }

    const char* get_last_error() { return m_last_error.c_str(); }
};
#endif

qemu::LibraryLoaderIface* qemu::get_default_lib_loader() { return new DefaultLibraryLoader; }
