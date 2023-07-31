#!/usr/bin/env python3
import argparse
import os
from pathlib import Path
import sys
from truth.truth import AssertThat
import psutil
import time
from QCSubprocess import QCSubprocess

CONFIG_NAME="config.lua"
args = None

def prepare():
    global args
    parser = argparse.ArgumentParser()
    parser.add_argument("-e", "--exe", metavar="", help="Path to vp executable")
    parser.add_argument("-i", "--img", metavar="", help="Path to binary images")
    parser.add_argument("-t", "--test-dir", metavar="", help="Path to LLDB test dir")
    args = parser.parse_args()
    if not args.exe:
        print("vp executable file not found")
        return False
    if not args.test_dir:
        print("test-dir is required")
        return False
    if not args.img:
        print("img argument is required")
        return False
    test_dir = Path(args.test_dir).as_posix()
    return QCSubprocess(f"make -C {test_dir}".split()).success()

def LLDB_test():
    vp_path = Path(args.exe).as_posix()
    test_dir = Path(args.test_dir).as_posix()
    img_path = Path(args.img).as_posix()
    luafile = Path(args.test_dir).joinpath(CONFIG_NAME).as_posix()

    load = psutil.cpu_percent(2)
    timeout_sec = 60 * 3.
    timeout_scale = 1
    if (load > 0.0):
        timeout_scale = 1 + (load / 100)
    timeout = timeout_sec * timeout_scale
    print ("Timeout Scale: ", timeout_scale)
    print ("Timeout (s): ", timeout)

    env = {
        "QQVP_IMAGE_DIR": img_path,
        "PWD": os.environ['PWD']
    }

    # Useful for local developement when using custom compiler installation
    # for building vp
    if os.environ.get("LD_LIBRARY_PATH") is not None:
        env["LD_LIBRARY_PATH"] = os.environ["LD_LIBRARY_PATH"]

    print("Starting platform with ENV", env)
    vp_args = [vp_path, "--gs_luafile", luafile]
    vp = QCSubprocess(vp_args, env, timeout)

    # Make sure the vp has started before running our test
    vp.expect(r"DSP Image Creation Date:.+\s*\n")
    time.sleep(4)

    lldb = QCSubprocess(["hexagon-lldb", "--batch", "--no-lldbinit",
        "-o", f"file {test_dir}/cdsp0_image",
        "--source", f"{test_dir}/script.lldb",
        "-o", "gdb-remote localhost:1234",
        "-o", "continue"], timeout=timeout)

    # Check that our breakpoints were reached
    lldb.expect(r"^PASS2$")
    lldb.expect(r"^PASS3$")
    return lldb.success()

AssertThat(prepare()).IsTrue()
AssertThat(LLDB_test()).IsTrue()
