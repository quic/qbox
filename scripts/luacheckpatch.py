#!/usr/bin/env python3

# A lua linter for PRs powered by luacheck
#
# When submitting a PR from, say, "my-feature-branch" to "main", run this
# like the following"
#    $ ./scripts/luacheckpatch.py main my-feature-branch
#
# To install luacheck:
#    $ sudo apt-get install luarocks && sudo luarocks install luacheck
#
# Must be run from the top of the repository

import sys
import subprocess
import re

ADDED_FILE_PREFIX="+++ b/"
LUACHECK_CMD="luacheck --no-color --formatter=plain --allow-defined-top --no-max-line-length $(find . -name '*.lua')"

def run(cmd, can_fail=True):
    proc = subprocess.run(cmd, shell=True,
                          stdout=subprocess.PIPE,
                          stderr=subprocess.PIPE,
                          encoding="utf-8")
    if not can_fail and proc.returncode != 0:
        print(f"Command failed: '{cmd}'")
        print(f"Stdout:\n{proc.stdout}")
        print(f"Stderr:\n{proc.stderr}")
        sys.exit(proc.returncode)
    return proc.stdout.split("\n")[:-1]

def check_requirements():
    error = False
    for cmd in ["luacheck", "git"]:
        if run(f"command -v {cmd}") == []:
            print(f"Please install '{cmd}'")
            error = True
    if error:
        sys.exit(1)

def main():
    if len(sys.argv) != 3:
        sys.exit(f"usage: {sys.argv[0]} <from-sha1> <to-sha1>")
    from_rev = sys.argv[1]
    to_rev = sys.argv[2]

    check_requirements()

    changes = {}

    for line in run(f"git diff -U0 '{from_rev}' '{to_rev}' -- '*.lua'", can_fail=False):
        if line.startswith(ADDED_FILE_PREFIX):
            file = line.lstrip(ADDED_FILE_PREFIX)
        elif line.startswith("@@ "):
            match = re.search(r'^@@ \-[0-9]*[,]*[0-9]* \+([0-9]*)[,]*([0-9]*) .*', line)
            if match is None or len(match.groups()) != 2:
                sys.exit("unexpected regex matching error")
            matches = match.groups()
            start, count = int(matches[0]), int(matches[1]) if matches[1] != '' else 1
            if file not in changes:
                changes[file] = set()
            changes[file] = changes[file].union(range(start, start + count))
    
    error = False

    for line in run(LUACHECK_CMD):
        file, line_nr, *_ = line.split(":")
        if file in changes and int(line_nr) in changes[file]:
            print(f"WARN: {line}")
            error = True

    return error

sys.exit(main())
