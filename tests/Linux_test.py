#!/usr/bin/python3
import argparse
import os
from pathlib import Path
from sys import exit, stdout
import pexpect
from truth.truth import AssertThat
from subprocess import Popen, PIPE


def linux_boot():
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
        "QQVP_IMAGE_DIR": img_path.as_posix(),
        "PWD": os.environ['PWD']
    }

    # Useful for local developement when using custom compiler installation
    # for building vp
    if os.environ.get("LD_LIBRARY_PATH") is not None:
        env["LD_LIBRARY_PATH"] = os.environ["LD_LIBRARY_PATH"]

    print("Starting platform with ENV", env)
    with pexpect.spawn(vp_path.as_posix(),
            ["--gs_luafile", args.lua,
             "--param", "platform.with_gpu=false", ],
            env=env,
            timeout=300
        ) as child:

        child.logfile = stdout.buffer

        child.expect('buildroot login:')
        child.sendline('root')
        child.expect("#")
        child.sendline('/sbin/halt')
        child.expect('reboot: System halted')

        return True


AssertThat(linux_boot()).IsTrue()
