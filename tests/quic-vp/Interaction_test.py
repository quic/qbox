#!/usr/bin/env python3
import argparse
import os
from pathlib import Path
import sys
from truth.truth import AssertThat
import psutil
import time
from QCSubprocess import QCSubprocess
import signal

def fastrpc_calc_test():
    test = False

    parser = argparse.ArgumentParser()
    parser.add_argument("-e", "--exe", metavar="", help="Path to vp executable")
    parser.add_argument("-l", "--lua", metavar="", help="Path to luafile")
    parser.add_argument("-i", "--img", metavar="", help="Path to binary images")
    parser.add_argument("-p", "--port", metavar="", help="SSH port of the vp")
    parser.add_argument("-a", "--adsp", action='store_true', help="Test ADSP too")
    parser.add_argument("-g", "--enable-gpu",
        action='store_true', help="Enable platform GPU")
    parser.add_argument("-r", "--rplugconf", metavar="", help="Path to remote hexagon plugin config directory")
    parser.add_argument("-d", "--log-dir", metavar="", help="Dir to store log files at")
    parser.add_argument('extra_args', nargs='*',
        help="Forward additional arguments to vp")
    args = parser.parse_args()
    if not args.exe:
        print("vp executable file not found")
        return test
    if not args.lua:
        print("luafile is required")
        return test
    if not args.img:
        print("img argument is required")
        return test
    ssh_port = args.port or "2222"

    vp_path = Path(args.exe)
    lua_path = Path(args.lua)
    img_path = Path(args.img)

    if args.log_dir:
        log_dir = Path(args.log_dir)
        log_dir.mkdir(parents=True, exist_ok=True)
    else:
        log_dir = None

    load = psutil.cpu_percent(2)
    timeout_sec = 60 * 4.
    timeout_scale = 1
    if (load > 0.0):
        timeout_scale = 1 + (load / 100)
    timeout_sec = timeout_sec * timeout_scale
    print ("Timeout Scale: ", timeout_scale)
    print ("Timeout (s): ", timeout_sec)

    env = {
        "QQVP_IMAGE_DIR": img_path.as_posix(),
        "PWD": os.environ['PWD']
    }

    if args.rplugconf:
        env["QQVP_PLUGIN_DIR"] = Path(args.rplugconf).as_posix()

    # Useful for local developement when using custom compiler installation
    # for building vp
    if os.environ.get("LD_LIBRARY_PATH") is not None:
        env["LD_LIBRARY_PATH"] = os.environ["LD_LIBRARY_PATH"]

    print("Starting platform with ENV", env)
    vp_args = [vp_path.as_posix(), "--gs_luafile", args.lua, ]

    if not args.enable_gpu:
        vp_args += [ "--param", "platform.with_gpu=false", ]

    if args.extra_args:
        vp_args += args.extra_args

    vp = QCSubprocess(vp_args, env, timeout_sec)
    num_dsps = 3 if args.adsp else 2
    for _ in range(num_dsps):
        vp.expect(r"DSP Image Creation Date:.+\s*\n")
    time.sleep(4)

    def get_logfile(name):
        if log_dir is not None:
            return log_dir.joinpath(logname).as_posix()
        return None

    adsp_args = "&& /mnt/bin/fastrpc_calc_test 0 100 0" if args.adsp else ""

    frpc_calc_args = [
        "sh -c '/mnt/bin/fastrpc_calc_test 0 100 3 && " # CDSP0
               "/mnt/bin/fastrpc_calc_test 0 100 4 "    # CDSP1
               f"{adsp_args}'"                          # ADSP
    ]

    # NOTE: we use ssh here instead of directly communicating through pexpect
    # and VP's UART due to oddnesses with /dev/tty in QNX when running on
    # MacOS CI. For more details, see QTOOL-95796.
    calc = QCSubprocess.ssh(frpc_calc_args, ssh_port,
                            logfile=get_logfile("frpc_ssh.log"),
                            timeout_sec=timeout_sec)
    for _ in range(num_dsps):
        calc.expect('- sum = 4950')
        calc.expect('- success')

    pcitool_args = [ "sh -l -c '/mnt/bin/pci-tool -v'", ]
    pci = QCSubprocess.ssh(pcitool_args, ssh_port,
                           logfile=get_logfile("pcitool_ssh.log"),
                           timeout_sec=timeout_sec)
    pci.expect('B000:D00:F00 @ idx 0')
    # Look for GPU device:
    if args.enable_gpu:
        pci.expect('vid/did: 1af4/1050')

    # Look for the rtl8139 PCI Ethernet Network Controller device:
    pci.expect('vid/did: 10ec/8139')

    ret = pci.success() and calc.success()

    # make sure to use SIGQUIT to terminate the vp as this signal is handled in
    # include/greensocs/gsutils/utils.h and it should be called for proper cleanup.
    vp.send_signal(signal.SIGQUIT)
    vp.success(timeout_sec=None)

    return ret


AssertThat(fastrpc_calc_test()).IsTrue()
