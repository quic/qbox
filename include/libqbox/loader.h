/*
 *  This file is part of libqbox
 *  Copyright (c) 2020 Clement Deschamps
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

#pragma once

void copy_file(const char* srce_file, const char* dest_file);

#ifdef _WIN32
#include <windows.h>
#include <Lmcons.h>

class LibQemuLibrary : public qemu::LibraryIface {
private:
    HMODULE m_lib;

public:
    LibQemuLibrary(HMODULE lib) : m_lib(lib) {}

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

class LibQemuLibraryLoader : public qemu::LibraryLoaderIface {
    std::string GetLastErrorAsString()
    {
        //Get the error message, if any.
        DWORD errorMessageID = ::GetLastError();
        if (errorMessageID == 0)
            return std::string(); //No error message has been recorded

        LPSTR messageBuffer = nullptr;
        size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL, errorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL);

        std::string message(messageBuffer, size);

        //Free the buffer.
        LocalFree(messageBuffer);

        return message;
    }

    qemu::LibraryLoaderIface::LibraryIfacePtr load_library(const char* lib_name)
    {
        static const char* base = NULL;
        if (!base) {
            HMODULE handle = LoadLibraryExA(lib_name, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
            if (handle == nullptr) {
                std::string str = GetLastErrorAsString();
                printf("error: %s\n", str.c_str());
                return nullptr;
            }

            char path[MAX_PATH];
            if (GetModuleFileNameA(handle, path, sizeof(path)) != 0) {
                base = _strdup(path);
            }

            return std::make_shared<LibQemuLibrary>(handle);
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
        copy_file(base, lib.c_str());

        HMODULE handle = LoadLibraryExA(lib.c_str(), NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
        if (handle == nullptr) {
            std::string str = GetLastErrorAsString();
            return nullptr;
        }
        return std::make_shared<LibQemuLibrary>(handle);
    }
};
#else
#include <dlfcn.h>
#include <link.h>
#include <unistd.h>
#include <sys/stat.h>


class LibQemuLibrary : public qemu::LibraryIface {
private:
    void *m_lib;

public:
    LibQemuLibrary(void *lib) : m_lib(lib) {}

    bool symbol_exists(const char* name)
    {
        return get_symbol(name) != NULL;
    }

    void* get_symbol(const char* name)
    {
        return dlsym(m_lib, name);
    }
};

class LibQemuLibraryLoader : public qemu::LibraryLoaderIface {
    qemu::LibraryLoaderIface::LibraryIfacePtr load_library(const char* lib_name)
    {
        static const char *base = NULL;
        if (!base) {
            void *handle = dlopen(lib_name, RTLD_NOW);
            if (handle == nullptr) {
                return nullptr;
            }

            struct link_map *map;
            dlinfo(handle, RTLD_DI_LINKMAP, &map);

            if (map) {
                base = strdup(map->l_name);
            }

            return std::make_shared<LibQemuLibrary>(handle);
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
        copy_file(base, lib.c_str());

        void *handle = dlopen(lib.c_str(), RTLD_NOW);
        if (handle == nullptr) {
            return nullptr;
        }
        return std::make_shared<LibQemuLibrary>(handle);
    }
};
#endif


