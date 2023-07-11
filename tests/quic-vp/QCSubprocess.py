#!/usr/bin/env python3
import subprocess
import signal
import re
import sys
import atexit
from threading import Thread
from queue import Queue
from collections import deque

# A simpler and non-interactive version of pexpect
class QCSubprocess:
    EOF = object()
    def __init__(self, args, env=None, timeout=None):
        self.proc = None
        self.name = args[0]
        @atexit.register
        def cleanup():
            if self.proc is not None:
                self.proc.kill()
                self.proc.wait()
        print(f"QCSubprocess: running '{' '.join(args)}'")
        print("With env: ", env)
        self.proc = subprocess.Popen(
                    args,
                    env=env,
                    stdout=subprocess.PIPE,
                    stderr=subprocess.STDOUT,
                    universal_newlines=True,
                )
        if timeout is not None:
            secs = int(round(timeout))
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

    @classmethod
    def _ensure_ssh_connects(cls, args, timeout=60., attempts=6):
        for _ in range(attempts):
            try:
                assert(cls(args + ["true"], timeout=timeout).success())
                print("SSH connection test passed.")
                return
            except (subprocess.TimeoutExpired, AssertionError):
                pass
        raise Exception(f"Failed to connect to sshd after {attempts} attempts")

    @classmethod
    def ssh(cls, args, port, logfile=None, env=None, timeout=None):
        ssh_args = ["ssh", "-p", port, "root@localhost",
                "-o", "UserKnownHostsFile=/dev/null",
                "-o", "StrictHostKeyChecking=no"]
        if logfile is not None:
            ssh_args.extend(["-v", "-E", logfile])
        cls._ensure_ssh_connects(ssh_args)
        return cls(ssh_args + args, env=env, timeout=timeout)
