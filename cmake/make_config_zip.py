#!/usr/bin/env python3
#
# Copyright (c) Qualcomm Innovation Center, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
"""
Build a config zip from json and hex source files.

Usage: make_config_zip.py output.zip input1.json input2.hex ...

Supported input formats:
  .json  — included as-is
  .hex   — sparse hex format, converted to binary before packing
  .bin   — included as-is

Sparse hex format (.hex):
  Lines of "hex_offset hex_value" (32-bit words, little-endian).
  Lines starting with # are comments. Blank lines are ignored.
  The total binary size is determined by the last offset + 4.
  All unspecified words are zero.

  Example:
    # register reset values
    00000000 00200001
    00000004 0201e080
    000fffb8 80000000
"""
import zipfile, sys, os, struct

def hex_to_bin(path):
    """Convert sparse hex file to binary."""
    entries = []
    max_offset = 0
    with open(path) as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith('#'):
                continue
            parts = line.split()
            if len(parts) != 2:
                continue
            offset = int(parts[0], 16)
            value = int(parts[1], 16)
            entries.append((offset, value))
            if offset + 4 > max_offset:
                max_offset = offset + 4

    buf = bytearray(max_offset)
    for offset, value in entries:
        struct.pack_into('<I', buf, offset, value)
    return bytes(buf)

def bin_to_hex(data):
    """Convert binary to sparse hex format (for reference/generation)."""
    lines = []
    words = struct.unpack(f'<{len(data)//4}I', data)
    for i, w in enumerate(words):
        if w != 0:
            lines.append(f'{i*4:08x} {w:08x}')
    return '\n'.join(lines) + '\n'

outzip = sys.argv[1]
inputs = sys.argv[2:]

with zipfile.ZipFile(outzip, 'w', zipfile.ZIP_DEFLATED) as z:
    for f in inputs:
        base = os.path.basename(f)
        ext = os.path.splitext(f)[1].lower()

        if ext == '.hex':
            # Convert sparse hex → binary, store as .bin in the zip
            bin_name = os.path.splitext(base)[0] + '.bin'
            z.writestr(bin_name, hex_to_bin(f))
        else:
            # json, bin, etc — include as-is
            z.write(f, base)
