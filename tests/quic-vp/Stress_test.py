#!/usr/bin/env python3

import argparse
import time
import os
import subprocess
import shlex
import sys

#fixme: switch this to using paramiko instead?
SSH = 'ssh -o BatchMode=yes -o StrictHostKeyChecking=no -p 2222 root@localhost'
def launch_frpc_test(cdsp):
    cdsp_ = {
        0: 3,
        1: 4,
    }
    cmd = f'{SSH} /mnt/bin/fastrpc_calc_test 0 100 {cdsp_[cdsp]}'
    p = subprocess.Popen(cmd.split(' '))
    return p

def check_guest_os():
    cmd = f'{SSH} sh -c "echo $?"'
    p = subprocess.Popen(cmd.split(' '))
    try:
        out, err = p.communicate(timeout=10)
    except subprocess.TimeoutExpired:
        print('timed out probing guest')
        return False
    print('rc:', p.returncode)
    return p.returncode == 0

def run_test(args):
    # TODO - these need to be tuned or we need to use expect :(
#   now_up = False
#   for i in range(30):
#       now_up = check_guest_os()
#       if now_up:
#           break

#       time.sleep(2.5)
#   if not now_up:
#       raise Exception('timeout waiting for guest to come up')

    time.sleep(70.)

    print('<< TEST SUITE waiting a bit >>')
    time.sleep(70.) # prob only needed for debug mode?  a little extra margin...
    print(f'<< TEST SUITE starting, {args.iters} iters >>')

    all_pass = None
    for i in range(args.iters):
        print(f'\n\n\n<< TEST SUITE iterating, iter {i} >>\n\n\n')
        def spawn_tests(concurrent):
            for j in range(concurrent):
                cdsp_index = j % 2
                yield launch_frpc_test(cdsp_index)
        runs = list(spawn_tests(args.concurrent_tests))

        # FIXME: scan or buffer the output, look for '- success'
        results = [ run.wait(timeout=args.timeout_sec) for run in runs ]

        all_pass = all(rc == 0 for rc in results)
        print(f'\n\n\n<< TEST SUITE iterating, iter {i} passed: {all_pass} >>\n\n\n')
        if not all_pass:
            for res_num, res in enumerate(results):
                print(f'{res_num}: {res}')
            break

    return all_pass

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('-q', '--qqvp-install',
        default='/prj/qct/llvm/target/vp_qemu_llvm/vp_builds/branch-0.3/latest/',
        help='Path to QQVP installation')
    parser.add_argument('-f', '--firmware-images',
        default='../firmware-images',
        help='Path to QQVP Firmware Images repo')
    parser.add_argument('-i', '--iters',
        default=5,
        help='Test iteration count',
        type=int)
    parser.add_argument('-t', '--timeout-sec',
        default=60,
        help='Timeout (in seconds) for each iteration',
        type=int)
    parser.add_argument('-c', '--concurrent-tests',
        default=2,
        help='Number of concurrent tests to execute',
        type=int)
    parser.add_argument('-p', '--platform',
        choices=('makena','lemans',),
        default=None,
        help='Hardware platform')

    args = parser.parse_args()


    part_num = '8540' if args.platform == 'makena' else '8775'
    cfgfile = f'{args.qqvp_install}/etc/vp/fw/{part_num}/'
    cfgfile += f'SA{part_num}P_conf.lua'
    cmd = f'{args.qqvp_install}/bin/vp --gs_luafile {cfgfile} -p platform.with_gpu=false'

    firmware_dir = os.path.realpath(f'{args.firmware_images}/makena/default') if args.platform == 'makena' else os.path.realpath(f'{args.firmware_images}/lemans')
    assert(os.path.exists(os.path.join(firmware_dir, 'conf.lua')))

    all_pass = None
    with subprocess.Popen(shlex.split(cmd),
        env={'QQVP_IMAGE_DIR': firmware_dir,
#            'LD_PRELOAD': '/usr/lib/libefence.so',
            }) as p:
        try:
            all_pass = run_test(args)
        except:
            print('exception occurred, unwinding...')
            raise
        finally:
            p.kill()

    print('Test PASS' if all_pass else 'Test FAIL')
    sys.exit(0 if all_pass else 1)
