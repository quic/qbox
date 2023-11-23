#!/usr/bin/python3
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


def vp_test():
    test = False

    parser = argparse.ArgumentParser()
    parser.add_argument("-e", "--exe", metavar="", help="Path to vp executable")
    parser.add_argument("-l", "--lua", metavar="", help="Path to luafile")
    parser.add_argument("-i", "--img", metavar="", help="Path to binary images")
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

    env = {
        "QBOX_IMAGE_DIR": img_path.as_posix(),
    }

    # Useful for local developement when using custom compiler installation
    # for building vp
    if os.environ.get("LD_LIBRARY_PATH") is not None:
        env["LD_LIBRARY_PATH"] = os.environ["LD_LIBRARY_PATH"]

    # start vp platform
        cmd = ["ip", "tuntap", "add", "qbox0", "mode", "tap"]
        proc = subprocess.Popen(
            cmd,
            shell=True,
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
        )

    child = pexpect.spawn(vp_path.as_posix(), ["--gs_luafile", args.lua], env=env, timeout=40)
    child.logfile = stdout.buffer
    child.expect("buildroot")
    child.expect("login:")
    child.sendline("root")
    child.expect("#")

    test = True
    return test


AssertThat(vp_test()).IsTrue()
