#!/usr/bin/env python3
import argparse
import os
from pathlib import Path
import sys
from truth.truth import AssertThat
import psutil
import time
from QCSubprocess import QCSubprocess

def fastrpc_calc_test():
    test = False

    parser = argparse.ArgumentParser()
    parser.add_argument("-e", "--exe", metavar="", help="Path to vp executable")
    parser.add_argument("-l", "--lua", metavar="", help="Path to luafile")
    parser.add_argument("-i", "--img", metavar="", help="Path to binary images")
    parser.add_argument("-p", "--port", metavar="", help="SSH port of the vp")
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

    load = psutil.cpu_percent(2)
    timeout_sec = 60 * 3.
    timeout_scale = 1
    if (load > 0.0):
        timeout_scale = 1 + (load / 100)
    timeout = timeout_sec * timeout_scale
    print ("Timeout Scale: ", timeout_scale)
    print ("Timeout (s): ", timeout)

    env = {
        "QQVP_IMAGE_DIR": img_path.as_posix(),
        "PWD": os.environ['PWD']
    }

    # Useful for local developement when using custom compiler installation
    # for building vp
    if os.environ.get("LD_LIBRARY_PATH") is not None:
        env["LD_LIBRARY_PATH"] = os.environ["LD_LIBRARY_PATH"]

    print("Starting platform with ENV", env)
    vp_args = [vp_path.as_posix(), "--gs_luafile", args.lua,
               "--param", "platform.with_gpu=false"]
    if args.extra_args:
        vp_args += args.extra_args

    vp = QCSubprocess(vp_args, env, timeout)
    vp.expect(r"DSP Image Creation Date:.+\s*\n") # CDSP{0,1}
    vp.expect(r"DSP Image Creation Date:.+\s*\n") # CDSP{0,1}
    time.sleep(4)

    ssh_args = ["ssh", "-p", ssh_port, "root@localhost",
                "-o", "UserKnownHostsFile=/dev/null",
                "-o", "StrictHostKeyChecking=no",
                "sh -c '/mnt/bin/fastrpc_calc_test 0 100 3 && "
                       "/mnt/bin/fastrpc_calc_test 0 100 4'"]

    # NOTE: we use ssh here instead of directly communicating through pexpect
    # and VP's UART due to oddnesses with /dev/tty in QNX when running on
    # MacOS CI. For more details, see QTOOL-95796.
    ssh = QCSubprocess(ssh_args, timeout=timeout)
    for _ in range(2):
        ssh.expect('- sum = 4950')
        ssh.expect('- success')

    test = True
    return test


AssertThat(fastrpc_calc_test()).IsTrue()
