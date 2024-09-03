/*
 * This file is part of libqbox
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#define SC_ALLOW_DEPRECATED_IEEE_API

#include <cci_configuration>

#include <libgssync.h>
#include <qemu-instance.h>

#include <module_factory_registery.h>

#include <qemu_gpex.h>
#include <cstring>
#include <errno.h>

class vhost_user_vsock_pci : public qemu_gpex::Device
{
    SCP_LOGGER(());

public:
    cci::cci_param<std::string> p_vsock_backend_path;
    cci::cci_param<std::string> p_vsock_vm_option;
    cci::cci_param<std::string> p_vsock_sock_path;
    cci::cci_param<bool> p_exec_backend;

    vhost_user_vsock_pci(const sc_core::sc_module_name& name, sc_core::sc_object* o, sc_core::sc_object* t)
        : vhost_user_vsock_pci(name, *(dynamic_cast<QemuInstance*>(o)), (dynamic_cast<qemu_gpex*>(t)))
    {
    }

    vhost_user_vsock_pci(const sc_core::sc_module_name& name, QemuInstance& inst, qemu_gpex* gpex)
        : qemu_gpex::Device(name, inst, "vhost-user-vsock-pci")
        , p_vsock_backend_path("vsock_device_backend_path", "", "path of vhost-device-vsock")
        , p_vsock_vm_option(
              "vsock_vm_option",
              "guest-cid=4,uds-path=/tmp/vm4.vsock,socket=/tmp/vsock.sock,tx-buffer-size=65536,queue-size=1024",
              "value of --vm option passed to vhost-device-vsock")
        , p_vsock_sock_path("vsock_socket_path", "/tmp/vsock.sock",
                            "path of vhost chardev socket to communicate with vhost-device-vsock")
        , p_exec_backend("exec_backend", true,
                         "boolean parameter to decide if the platform should start the backend, or we can only connect "
                         "to an already started backend")
    {
        if (p_exec_backend.get_value()) {
            if (p_vsock_backend_path.get_value().empty()) {
                SCP_FATAL(())
                    << "vsock_path CCI parameter is empty, please use the path of vhost-device-vsock executable";
            }
            if (p_vsock_vm_option.get_value().empty()) {
                SCP_FATAL(())
                    << "vsock_vm_option CCI parameter is empty, please use a correct value of \"--vm\" option "
                       "passed to vhost-device-vsock";
            }
            exec_vhost_user_vsock(p_vsock_backend_path.get_value(), p_vsock_vm_option.get_value());
        }
        m_inst.add_arg("-chardev");
        std::string chardev_str = "socket,id=vhost_user_vsock_pci_sock_chardev,path=" + p_vsock_sock_path.get_value();
        m_inst.add_arg(chardev_str.c_str());
        gpex->add_device(*this);
    }

    void before_end_of_elaboration() override
    {
        qemu_gpex::Device::before_end_of_elaboration();
        m_dev.set_prop_str("chardev", "vhost_user_vsock_pci_sock_chardev");
    }

private:
    void exec_vhost_user_vsock(std::string path, std::string vsock_vm)
    {
        if (path.empty()) {
            SCP_FATAL(()) << "exec_vhost_user_vsock() failed, path is empty!";
        }
        if (vsock_vm.empty()) {
            SCP_FATAL(()) << "exec_vhost_user_vsock() failed, vsock_vm is empty!";
        }
        pid_t pid = fork();
        if (pid < 0) {
            SCP_FATAL(()) << "couldn't fork " << path << " Error: " << strerror(errno);
        }
        // child process
        if (pid == 0) {
            const char* vhost_user_vsock_path = path.c_str();
            const char* vhost_vm_option = vsock_vm.c_str();
            const char* vhost_vm = "--vm";
            char* vhost_user_gpu_argv[] = { const_cast<char*>(vhost_user_vsock_path), const_cast<char*>(vhost_vm),
                                            const_cast<char*>(vhost_vm_option), nullptr };
            char* vhost_user_gpu_environ[] = { nullptr };
            execve(vhost_user_vsock_path, vhost_user_gpu_argv, vhost_user_gpu_environ);
            perror(std::string("couldn't execve " + path).c_str());
            exit(EXIT_FAILURE);
        }
    }
};

extern "C" void module_register();
