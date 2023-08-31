#!/usr/bin/env python3
import subprocess
import signal
import os
import re
import sys
import atexit
from threading import Thread
from queue import Queue
from collections import deque

# A simpler and non-interactive version of pexpect
class QCSubprocess:
    EOF = object()
    cleanup_list = []
    cleanup_installed = False
    cleanup_ran = False

    @classmethod
    def _cleanup(cls):
        if cls.cleanup_ran:
            return
        cls.cleanup_ran = True
        print("Running QCSubprocess cleanup...")
        for p in cls.cleanup_list:
            if p.proc is not None and p.proc.poll() is None:
                print(f"Ending '{p.name}'")
                p.proc.send_signal(signal.SIGQUIT)
                p.proc.wait()

    @classmethod
    def _set_to_clean(cls, qcproc):
        cls.cleanup_list.append(qcproc)
        if not cls.cleanup_installed:

            # 1. Normal termination
            @atexit.register
            def atexit_cleanup():
                cls._cleanup()

            # 2. Abormal termination due to uncaught exception
            old_excepthook = sys.excepthook
            def new_excepthook(a, b, c):
                cls._cleanup()
                return old_excepthook(a, b, c)
            sys.excepthook = new_excepthook

            # 3. Abnormal termination due to signal
            for sig in [signal.SIGHUP, signal.SIGQUIT, signal.SIGABRT, signal.SIGTERM]:
                old_handler = signal.getsignal(sig)
                def new_handler(signum, frame):
                    cls._cleanup()
                    signal.signal(signum, old_handler)
                    os.kill(os.getpid(), signum)
                signal.signal(sig, new_handler)

            cls.cleanup_installed = True

    def __init__(self, args, env=None, timeout_sec=None):
        self.proc = None
        self.name = args[0]
        self.__class__._set_to_clean(self)
        print(f"QCSubprocess: running '{' '.join(args)}'")
        print("With env: ", env)
        self.proc = subprocess.Popen(
                    args,
                    env=env,
                    stdout=subprocess.PIPE,
                    stderr=subprocess.STDOUT,
                    universal_newlines=True,
                )
        if timeout_sec is not None:
            secs = int(round(timeout_sec))
            def handler(signum, _):
                self.proc.kill()
                raise subprocess.TimeoutExpired(self.name, secs)
            signal.signal(signal.SIGALRM, handler)
            signal.alarm(secs)

        self.buf = Queue()
        def read_thread():
            for line in self.proc.stdout:
                print(line, end="")
                self.buf.put(line)
            self.buf.put(QCSubprocess.EOF)
        Thread(target=read_thread, daemon=True).start()

    def success(self, timeout_sec=20.):
        try:
            self.proc.wait(timeout=timeout_sec)
        except subprocess.TimeoutExpired:
            raise Exception(f"timed out waiting for {self.name} to finish")
        if self.proc.returncode != 0:
            raise Exception(f"Expecting returncode 0 from {self.name} but got {self.proc.returncode}")
        return True

    def expect(self, regex_string):
        recent_lines = deque([], maxlen=30)
        regex = re.compile(regex_string)
        while True:
            line = self.buf.get()
            if line == QCSubprocess.EOF:
                raise Exception(f"""expected '{regex_string}' but got EOF
context:
{''.join(recent_lines)}
""")
            if regex.search(line) is not None:
                break
            recent_lines.append(line)

    def send_signal(self, signal):
        self.proc.send_signal(signal)

    @classmethod
    def _ensure_ssh_connects(cls, args, timeout_sec=60., attempts=6):
        for _ in range(attempts):
            try:
                assert(cls(args + ["true"], timeout_sec=timeout_sec).success())
                print("SSH connection test passed.")
                return
            except (subprocess.TimeoutExpired, AssertionError):
                pass
        raise Exception(f"Failed to connect to sshd after {attempts} attempts")

    @classmethod
    def ssh(cls, args, port, logfile=None, env=None, timeout_sec=None):
        ssh_args = ["ssh", "-p", port, "root@localhost",
                "-o", "UserKnownHostsFile=/dev/null",
                "-o", "StrictHostKeyChecking=no"]
        if logfile is not None:
            ssh_args.extend(["-v", "-E", logfile])
        cls._ensure_ssh_connects(ssh_args)
        return cls(ssh_args + args, env=env, timeout_sec=timeout_sec)
