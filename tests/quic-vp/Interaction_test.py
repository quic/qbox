#!/usr/bin/env python3
import argparse, pprint
import os
from os import sched_get_priority_max
from pathlib import Path
import sys
from sys import exit, stdout
import pexpect
from pexpect import *
import getpass
from truth.truth import AssertThat
import subprocess
from subprocess import Popen, PIPE
import psutil


def fastrpc_calc_test():
    test = False

    parser = argparse.ArgumentParser()
    parser.add_argument("-e", "--exe", metavar="", help="Path to vp executable")
    parser.add_argument("-l", "--lua", metavar="", help="Path to luafile")
    parser.add_argument("-i", "--img", metavar="", help="Path to binary images")
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

    vp_path = Path(args.exe)
    lua_path = Path(args.lua)
    img_path = Path(args.img)

    load = psutil.cpu_percent(2)
    timeout_sec = 60 * 3.
    timeout_scale = 1
    if (load > 0.0):
        timeout_scale = 1 + (load / 100)
    print ("Timeout Scale: ", timeout_scale)

    env = {
        "QQVP_IMAGE_DIR": img_path.as_posix(),
        "PWD": os.environ['PWD']
    }

    # Useful for local developement when using custom compiler installation
    # for building vp
    if os.environ.get("LD_LIBRARY_PATH") is not None:
        env["LD_LIBRARY_PATH"] = os.environ["LD_LIBRARY_PATH"]

    print("Starting platform with ENV", env)
    vp_args = ["--gs_luafile", args.lua, "--param", "platform.with_gpu=false", ]
    if args.extra_args:
        vp_args += args.extra_args

    child = pexpect.spawn(
                vp_path.as_posix(),
                vp_args,
                env=env,
                timeout=timeout_sec*timeout_scale
            )

    child.logfile = stdout.buffer
    child.expect(r"DSP Image Creation Date:.+\s*\n") # CDSP{0,1}
    child.expect(r"DSP Image Creation Date:.+\s*\n") # CDSP{0,1}

    cdsp0 = 3
    cdsp1 = 4
    for cdsp_index in (cdsp0, cdsp1):
        child.send("\n")
        child.send("\n")
        child.send("\n")
        child.expect("#")
        child.sendline('fastrpc_calc_test 0 100 {}'.format(cdsp_index))
        child.expect('- sum = 4950', timeout=300*timeout_scale)
        child.expect('- success')

    test = True
    return test


AssertThat(fastrpc_calc_test()).IsTrue()
