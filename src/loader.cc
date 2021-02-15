/*
 *  This file is part of libqemu-cxx
 *  Copyright (C) 2021 Luc Michel
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <string>
#include <sstream>
#include <fstream>
#include <cstring>

#include "libqemu-cxx/loader.h"


static void copy_file(const char* src_file, const char* dest_file)
{
    std::ifstream src(src_file, std::ios::binary);
    std::ofstream dest(dest_file, std::ios::binary);
    dest << src.rdbuf();
}

#ifdef _WIN32
#include <windows.h>
#include <Lmcons.h>

class Library : public qemu::LibraryIface {
private:
    HMODULE m_lib;

public:
    Library(HMODULE lib) : m_lib(lib) {}

    bool symbol_exists(const char* name)
    {
        return get_symbol(name) != NULL;
    }

    void* get_symbol(const char* name)
    {
        FARPROC symbol = GetProcAddress(m_lib, name);
        return *(void**)(&symbol);
    }
};

class DefaultLibraryLoader : public qemu::LibraryLoaderIface {
private:
    const char *m_base;

public:
    std::string GetLastErrorAsString()
    {
        // Get the error message, if any.
        DWORD errorMessageID = ::GetLastError();
        if (errorMessageID == 0)
            return std::string(); // No error message has been recorded

        LPSTR messageBuffer = nullptr;
        size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER
                                     | FORMAT_MESSAGE_FROM_SYSTEM
                                     | FORMAT_MESSAGE_IGNORE_INSERTS,
                                     NULL, errorMessageID,
                                     MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                                     (LPSTR)&messageBuffer, 0, NULL);

        std::string message(messageBuffer, size);

        // Free the buffer.
        LocalFree(messageBuffer);

        return message;
    }

    qemu::LibraryLoaderIface::LibraryIfacePtr load_library(const char* lib_name)
    {
        if (!m_base) {
            HMODULE handle = LoadLibraryExA(lib_name, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
            if (handle == nullptr) {
                std::string str = GetLastErrorAsString();
                fprintf(stderr, "error: %s\n", str.c_str());
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

    const char * get_lib_ext() {
        return "dll";
    }
};

#else /* _WIN32 */

#include <dlfcn.h>
#include <link.h>
#include <unistd.h>
#include <sys/stat.h>


class Library : public qemu::LibraryIface {
private:
    void *m_lib;

public:
    Library(void *lib) : m_lib(lib) {}

    bool symbol_exists(const char* name)
    {
        return get_symbol(name) != NULL;
    }

    void* get_symbol(const char* name)
    {
        return dlsym(m_lib, name);
    }
};

class DefaultLibraryLoader : public qemu::LibraryLoaderIface {
private:
    const char *m_base = nullptr;

public:
    qemu::LibraryLoaderIface::LibraryIfacePtr load_library(const char* lib_name)
    {
        if (!m_base) {
            void *handle = dlopen(lib_name, RTLD_NOW);
            if (handle == nullptr) {
                return nullptr;
            }

            struct link_map *map;
            dlinfo(handle, RTLD_DI_LINKMAP, &map);

            if (map) {
                m_base = strdup(map->l_name);
            }

            return std::make_shared<Library>(handle);
        }

        static int counter = 0;
        std::stringstream ss1;
        ss1 << counter++;
        std::string str = ss1.str();

        const char *login = getlogin();
        if (!login) {
            login = "none";
        }

        std::stringstream ss;
        ss << "/tmp/libqemu_" << login;
        std::string dir = ss.str();
        mkdir(dir.c_str(), S_IRWXU);

        std::string lib = dir + "/" + std::string(lib_name) + "." + str;
        copy_file(m_base, lib.c_str());

        void *handle = dlopen(lib.c_str(), RTLD_NOW);
        if (handle == nullptr) {
            return nullptr;
        }
        return std::make_shared<Library>(handle);
    }

    const char * get_lib_ext() {
        return "so";
    }
};
#endif

qemu::LibraryLoaderIface * qemu::get_default_lib_loader()
{
    return new DefaultLibraryLoader;
}
