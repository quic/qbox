#!/usr/bin/env python3
import subprocess
import signal
import re
import sys
import atexit
from threading import Thread
from queue import Queue

# A simpler and non-interactive version of pexpect
class QCSubprocess:

    def __init__(self, args, env=None, timeout=None):
        self.proc = None
        @atexit.register
        def cleanup():
            if self.proc is not None:
                self.proc.kill()
                self.proc.wait()
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
                raise Exception(f"Timeout after {secs} secs: {args[0]}")
            signal.signal(signal.SIGALRM, handler)
            signal.alarm(secs)
   
        self.buf = Queue()
        def read_thread():
            for line in self.proc.stdout:
                print(line, end="")
                self.buf.put(line)
        Thread(target=read_thread, daemon=True).start()
    
    def expect(self, regex_string):
        regex = re.compile(regex_string)
        while True:
            line = self.buf.get()
            if line == "":
                raise Exception(f"expected '{regex_string}' but got EOF")
            if regex.search(line) is not None:
                break
