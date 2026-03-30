/*
 * DLL Cleanup Helper
 * Copyright (c) 2022-2023 Qualcomm Innovation Center, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * This helper program waits for a parent process to exit, then deletes
 * a specified DLL file. It is spawned by the library loader to clean up
 * temporary DLL copies that cannot be deleted while the main process is running.
 *
 * Usage: dll_cleanup_helper.exe <parent_handle> <dll_path>
 *   parent_handle: Numeric value of an inheritable handle to the parent process
 *   dll_path: Path to the DLL file to delete after parent exits
 */

#include <windows.h>
#include <cstdlib>
#include <cstdio>

int main(int argc, char* argv[])
{
    if (argc != 3) {
        return 1;
    }

    // Parse the parent process handle from command line
    uintptr_t handle_value = strtoull(argv[1], nullptr, 10);
    HANDLE parent_handle = reinterpret_cast<HANDLE>(handle_value);

    const char* dll_path = argv[2];

    // Wait for the parent process to exit (infinite timeout)
    DWORD wait_result = WaitForSingleObject(parent_handle, INFINITE);

    // Close the inherited handle
    CloseHandle(parent_handle);

    if (wait_result == WAIT_OBJECT_0) {
        // Parent process has exited, delete the DLL
        // Retry a few times in case the file is still briefly locked
        for (int i = 0; i < 10; ++i) {
            if (DeleteFileA(dll_path)) {
                return 0;
            }
            Sleep(100);
        }
    }

    return 1;
}
