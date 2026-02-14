#!/usr/bin/env python3
"""
Create a simple same-size binary patch in the custom "PTCH" format v2:

Header:
- magic: 4 bytes: b"PTCH"
- version: uint32 LE (2)
- output_size: uint64 LE (size of new.bin)
- record_count: uint32 LE
- path_len: uint16 LE (length of target path string)
- target_path: N bytes (relative path from DATA_PATH)

Each record:
- offset: uint64 LE
- length: uint32 LE
- data:   raw bytes (from new.bin)

This patch format assumes:
- old.bin and new.bin are EXACTLY the same size
- patch contains only changed byte ranges (runs)

Usage:
  python make_patch.py old.bin new.bin patch.ptch target_path

Example:
  python make_patch.py original.so modified.so fix.ptch lib/armeabi-v7a/libfoo.so
"""

from __future__ import annotations

import argparse
import os
import struct
from typing import List, Tuple


MAGIC = b"PTCH"
VERSION = 2

# Little-endian: magic(4s), version(I), out_size(Q), record_count(I), path_len(H)
HEADER_STRUCT = struct.Struct("<4sIQIH")  # 22 bytes total
# Little-endian: uint64, uint32
RECORD_HDR_STRUCT = struct.Struct("<QI")  # offset, length


def compute_diff_runs(old_bytes: bytes, new_bytes: bytes) -> List[Tuple[int, bytes]]:
    """
    Return a list of (offset, data_bytes) for contiguous runs where old != new.
    """
    if len(old_bytes) != len(new_bytes):
        raise ValueError("old and new must be the same size")

    runs: List[Tuple[int, bytes]] = []
    n = len(old_bytes)
    i = 0
    while i < n:
        if old_bytes[i] == new_bytes[i]:
            i += 1
            continue

        start = i
        i += 1
        while i < n and old_bytes[i] != new_bytes[i]:
            i += 1

        runs.append((start, new_bytes[start:i]))

    return runs


def write_patch(old_path: str, new_path: str, patch_path: str, target_path: str) -> None:
    old_size = os.path.getsize(old_path)
    new_size = os.path.getsize(new_path)
    if old_size != new_size:
        raise ValueError(f"Size mismatch: old={old_size} bytes, new={new_size} bytes")

    target_path_bytes = target_path.encode("utf-8")
    if len(target_path_bytes) > 0xFFFF:
        raise ValueError("Target path too long (max 65535 bytes)")

    with open(old_path, "rb") as f:
        old_bytes = f.read()
    with open(new_path, "rb") as f:
        new_bytes = f.read()

    runs = compute_diff_runs(old_bytes, new_bytes)

    # Write patch
    with open(patch_path, "wb") as out:
        # Placeholder header with record_count=0; we'll rewrite after writing runs
        out.write(HEADER_STRUCT.pack(MAGIC, VERSION, new_size, 0, len(target_path_bytes)))
        out.write(target_path_bytes)

        record_count = 0
        for offset, data in runs:
            if len(data) > 0xFFFFFFFF:
                raise ValueError("A diff run exceeded uint32 length")
            out.write(RECORD_HDR_STRUCT.pack(offset, len(data)))
            out.write(data)
            record_count += 1

        # Rewrite header with correct record_count
        out.seek(0)
        out.write(HEADER_STRUCT.pack(MAGIC, VERSION, new_size, record_count, len(target_path_bytes)))

    print(f"Wrote patch: {patch_path}")
    print(f"Target: {target_path}")
    print(f"Old/New size: {new_size} bytes")
    print(f"Records: {len(runs)}")
    print(f"Patch size: {os.path.getsize(patch_path)} bytes")


def main() -> None:
    ap = argparse.ArgumentParser(
        description="Create a PTCH v2 patch from old.bin to new.bin (same size).",
        epilog="Example: %(prog)s original.so modified.so fix.ptch lib/armeabi-v7a/libfoo.so"
    )
    ap.add_argument("old", help="Path to old (original) binary")
    ap.add_argument("new", help="Path to new (modified) binary")
    ap.add_argument("patch", help="Output patch file (.ptch)")
    ap.add_argument("target", help="Target path relative to DATA_PATH (e.g., lib/armeabi-v7a/libfoo.so)")
    args = ap.parse_args()

    write_patch(args.old, args.new, args.patch, args.target)


if __name__ == "__main__":
    main()
