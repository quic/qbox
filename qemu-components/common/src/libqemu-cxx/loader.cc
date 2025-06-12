/*
 * This file is part of libqemu-cxx
 * Copyright (c) 2022-2025 Qualcomm Innovation Center, Inc. All Rights Reserved.
 * Author: GreenSocs 2021
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <cstring>
#include <fstream>
#include <sstream>
#include <string>
#include <iostream>

#include <libqemu-cxx/loader.h>
#include <cci_configuration>

extern "C" {
/*
 * This function sets the CCI parameter. It gets the broker and parameter handle,
 * then sets the value if the handle is valid, or creates a new parameter if it is not.
 */
void global_set_cci_param(char* key, uint64_t val)
{
    auto b = cci::cci_get_broker();
    auto b_key = b.get_param_handle(key);

    if (b_key.is_valid()) {
        b_key.set_cci_value(cci::cci_value(val));
    } else {
        b.set_preset_cci_value(key, cci::cci_value(val));
    }
}
}

static void copy_file(const char* src_file, const char* dest_file)
{
    std::ifstream src(src_file, std::ios::binary);
    std::ofstream dest(dest_file, std::ios::binary);
    dest << src.rdbuf();
    src.close();
    dest.close();
#if defined(__APPLE__)
    std::stringstream libtool;
    libtool << "install_name_tool -id " << dest_file << " " << dest_file;
    system(libtool.str().c_str());
#endif
}

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
};

class DefaultLibraryLoader : public qemu::LibraryLoaderIface
{
private:
    const char* m_base;
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
    qemu::LibraryLoaderIface::LibraryIfacePtr load_library(const char* lib_name)
    {
        if (!m_base) {
            HMODULE handle = LoadLibraryExA(lib_name, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
            if (handle == nullptr) {
                m_last_error = get_last_error_as_str();
                return nullptr;
            }

            char path[MAX_PATH];
            if (GetModuleFileNameA(handle, path, sizeof(path)) != 0) {
                m_base = _strdup(path);
            }

            return std::make_shared<Library>(handle);
        }

        char TMP[MAX_PATH];
        if (GetTempPathA(MAX_PATH, TMP) == 0) {
            std::string str = GetLastErrorAsString();
            return nullptr;
        }

        static int counter = 0;
        std::stringstream ss1;
        ss1 << counter++;
        std::string str = ss1.str();

        std::stringstream ss;
        ss << std::string(TMP);
        std::string dir = ss.str();

        std::string lib = dir + "\\" + std::string(lib_name) + "." + str;
        copy_file(m_base, lib.c_str());

        HMODULE handle = LoadLibraryExA(lib.c_str(), NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
        if (handle == nullptr) {
            std::string str = GetLastErrorAsString();
            return nullptr;
        }
        return std::make_shared<Library>(handle);
    }

    const char* get_lib_ext() { return "dll"; }

    const char* get_last_error() {}
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
};

class DefaultLibraryLoader : public qemu::LibraryLoaderIface
{
private:
    const char* m_base = nullptr;
    std::string m_last_error;

public:
    qemu::LibraryLoaderIface::LibraryIfacePtr load_library(const char* lib_name)
    {
        if (!m_base) {
            std::cout << "Loading " << lib_name << "\n";
            void* handle = dlopen(lib_name, RTLD_LOCAL | RTLD_NOW);
            if (handle == nullptr) {
                m_last_error = dlerror();
                return nullptr;
            }
            m_base = dlpath(handle);
            return std::make_shared<Library>(handle);
        }
        std::cout << "RE Loading " << m_base << "\n";
        char tmp[] = "/tmp/qbox_lib.XXXXXX";
        if (mkstemp(tmp) < 0) {
            m_last_error = "Unable to create temp file";
            return nullptr;
        }
        copy_file(m_base, tmp);

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
