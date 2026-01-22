
---- Single output example  -----

-- A virtio_gpu_* module exposes one output by default,
-- the parameter `outputs` is optional in this scenario
gpu = {
    moduletype = "virtio_gpu_" .. gpu_type .. "_pci";
    args = {"&platform.qemu_inst", "&platform.gpex_0"};
}


vnc = {
    moduletype = "vnc";
    gpu_output = 0;  -- Use default output
    args = {"&platform.gpu_0"};
}

----- Multi output example with VNC over websocket -----

gpu = {
    moduletype = "virtio_gpu_" .. gpu_type .. "_pci";
    args = {"&platform.qemu_inst", "&platform.gpex_0"};
}

vnc0 = {
    moduletype = "vnc";
    gpu_output = 0;
    params = "websocket=on";
    args = {"&platform.gpu_0"};
}

vnc1 = {
    moduletype = "vnc";
    gpu_output = 1;
    params = "websocket=on";
    args = {"&platform.gpu_0"};
}
