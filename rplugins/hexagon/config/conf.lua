if(platform["hex_plugin"] ~= nil) then
    platform.hex_plugin.moduletype = "RemotePass";
    platform.hex_plugin.exec_path = "./vp-hex-remote"; 
    platform.hex_plugin.remote_argv = {"--param=log_level=2"};
    platform.hex_plugin.plugin_pass.moduletype = "RemotePass";
end