#!/usr/bin/env python3
"""Make an SD card image the way a PC would: an MBR with one FAT16 partition at
1 MB, two FATs, 8.3 names, every file in one run of clusters.

  mksd.py <image> [folder] [--mb N] [--spc N]

The folder's tree is copied in (names upper-cased, cut to 8.3); without one the
volume is empty. Fixed dates and volume id, so an image is the same every time.
"""

import argparse
import struct
import sys
from pathlib import Path

SECTOR = 512
PART_LBA = 2048
ROOT_ENTRIES = 512
DATE = ((2026 - 1980) << 9) | (1 << 5) | 1	# 2026-01-01
TIME = 12 << 11


def name83(name, is_dir):
    stem, dot, ext = name.upper().rpartition(".")
    if not dot or is_dir and not stem:
        stem, ext = name.upper(), ""
    stem = "".join(c for c in stem if c.isalnum() or c in "_-~!#$%&'()@^{}")[:8]
    ext = "".join(c for c in ext if c.isalnum() or c in "_-~!#$%&'()@^{}")[:3]
    return stem.ljust(8).encode("ascii") + ext.ljust(3).encode("ascii")


def dirent(name11, attr, cluster, size):
    return struct.pack("<11sBBBHHHHHHHI", name11, attr, 0, 0, TIME, DATE, DATE, 0, TIME, DATE, cluster, size)


class Vol:
    def __init__(self, sectors, spc):
        self.spc = spc
        self.csize = spc * SECTOR
        self.res = 4
        self.root_secs = ROOT_ENTRIES * 32 // SECTOR
        clusters = sectors // spc
        self.fat_secs = ((clusters + 2) * 2 + SECTOR - 1) // SECTOR
        data = sectors - self.res - 2 * self.fat_secs - self.root_secs
        self.clusters = data // spc
        if not 4085 <= self.clusters <= 65524:
            sys.exit(f"{self.clusters} clusters is not FAT16: change --mb or --spc")
        self.sectors = sectors
        self.fat = [0] * (self.clusters + 2)
        self.fat[0], self.fat[1] = 0xfff8, 0xffff
        self.next = 2
        self.data = {}			# cluster -> bytes

    def alloc(self, nbytes):
        n = max(1, (nbytes + self.csize - 1) // self.csize)
        first = self.next
        if first + n > self.clusters + 2:
            sys.exit("volume full")
        for c in range(first, first + n):
            self.fat[c] = c + 1
        self.fat[first + n - 1] = 0xffff
        self.next += n
        return first

    def put(self, first, blob):
        for i in range(0, max(len(blob), 1), self.csize):
            self.data[first + i // self.csize] = blob[i:i + self.csize]

    def add_dir(self, folder, parent):
        """the entries of folder; parent: its parent's cluster (0 = root)"""
        items = sorted(folder.iterdir(), key=lambda p: (not p.is_dir(), p.name.upper())) if folder else []
        ents, seen = [], set()
        for p in items:
            n11 = name83(p.name, p.is_dir())
            if n11 in seen:
                sys.exit(f"8.3 name clash: {p}")
            seen.add(n11)
            ents.append((p, n11))
        return ents

    def write_tree(self, folder, cluster, parent):
        out = []
        if cluster:		# a subdirectory: . and ..
            out.append(dirent(b".          ", 0x10, cluster, 0))
            out.append(dirent(b"..         ", 0x10, parent, 0))
        for p, n11 in self.add_dir(folder, parent):
            if p.is_dir():
                sub_n = len(list(p.iterdir())) + 2
                c = self.alloc(sub_n * 32)
                out.append(dirent(n11, 0x10, c, 0))
                self.put(c, self.write_tree(p, c, cluster))
            else:
                blob = p.read_bytes()
                c = self.alloc(len(blob)) if blob else 0
                if blob:
                    self.put(c, blob)
                out.append(dirent(n11, 0x20, c, len(blob)))
        return b"".join(out)

    def image(self, label, root_blob):
        bs = bytearray(SECTOR)
        bs[0:3] = b"\xeb\x3c\x90"
        bs[3:11] = b"XPECCY  "
        tot16 = self.sectors if self.sectors < 0x10000 else 0
        tot32 = 0 if tot16 else self.sectors
        struct.pack_into("<HBHBHHBHHHII", bs, 11, SECTOR, self.spc, self.res, 2, ROOT_ENTRIES,
                         tot16, 0xf8, self.fat_secs, 63, 255, PART_LBA, tot32)
        struct.pack_into("<BBBI11s8s", bs, 36, 0x80, 0, 0x29, 0x58504346, label.ljust(11).encode(), b"FAT16   ")
        bs[510:512] = b"\x55\xaa"
        fat = struct.pack(f"<{len(self.fat)}H", *self.fat).ljust(self.fat_secs * SECTOR, b"\0")
        root = root_blob.ljust(self.root_secs * SECTOR, b"\0")
        vol = bytearray(self.sectors * SECTOR)
        pos = 0
        vol[pos:pos + SECTOR] = bs
        pos = self.res * SECTOR
        for _ in range(2):
            vol[pos:pos + len(fat)] = fat
            pos += len(fat)
        vol[pos:pos + len(root)] = root
        pos += len(root)
        for c, blob in self.data.items():
            off = pos + (c - 2) * self.csize
            vol[off:off + len(blob)] = blob
        return vol


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("image")
    ap.add_argument("folder", nargs="?")
    ap.add_argument("--mb", type=int, default=32)
    ap.add_argument("--spc", type=int, default=4, help="sectors per cluster")
    args = ap.parse_args()
    total = args.mb * 1024 * 1024 // SECTOR
    v = Vol(total - PART_LBA, args.spc)
    root = v.write_tree(Path(args.folder) if args.folder else None, 0, 0)
    label = dirent(b"XPECCY     ", 0x08, 0, 0)
    vol = v.image("XPECCY", label + root)
    mbr = bytearray(SECTOR)
    # one partition: active, type 06 (FAT16), CHS left at the LBA-only marks
    struct.pack_into("<B3sB3sII", mbr, 446, 0x80, b"\xfe\xff\xff", 0x06, b"\xfe\xff\xff", PART_LBA, v.sectors)
    mbr[510:512] = b"\x55\xaa"
    with open(args.image, "wb") as f:
        f.write(mbr)
        f.write(bytes((PART_LBA - 1) * SECTOR))
        f.write(vol)
    return 0


if __name__ == "__main__":
    sys.exit(main())
