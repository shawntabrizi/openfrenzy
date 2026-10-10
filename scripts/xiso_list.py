#!/usr/bin/env python3
"""List the files of an Xbox disc image (XDVDFS), or extract one file.

Run:  python3 -I scripts/xiso_list.py [image] [--summary] [--extract DISC_PATH OUT]
Default image: orig/game.xiso.iso.

Format (as extract-xiso reads it): 2048-byte sectors; the volume descriptor is at
0x10000 (plus the partition offset of a whole-disc image) and starts with
"MICROSOFT*XBOX*MEDIA", then the root directory sector (u32) and size (u32).
A directory is a binary tree of entries: left u16, right u16 (offsets in 4-byte
units), sector u32, size u32, attributes u8, name length u8, name; 4-byte aligned.
"""
import os
import struct
import sys
from collections import Counter

HERE = os.path.dirname(os.path.abspath(__file__))
SECTOR = 2048
MAGIC = b"MICROSOFT*XBOX*MEDIA"
PARTITION_OFFSETS = (0, 0x18300000, 0xFD90000, 0x89D80000, 0x2080000)


def open_image(path):
    fh = open(path, "rb")
    for base in PARTITION_OFFSETS:
        fh.seek(base + 0x10000)
        head = fh.read(28)
        if head[:20] == MAGIC:
            root_sector, root_size = struct.unpack_from("<II", head, 20)
            return fh, base, root_sector, root_size
    sys.exit("not an XDVDFS image: %s" % path)


def walk(fh, base, sector, size, prefix, out):
    fh.seek(base + sector * SECTOR)
    data = fh.read(size)
    seen = set()

    def node(off):
        if off in seen or off + 14 > len(data):
            return
        seen.add(off)
        left, right, sec, sz, attr, nlen = struct.unpack_from("<HHIIBB", data, off)
        if left == 0xFFFF:
            return
        name = data[off + 14:off + 14 + nlen].decode("latin-1")
        if left:
            node(left * 4)
        path = prefix + name
        if attr & 0x10:
            out.append((path + "/", sec, sz, True))
            if sz:
                walk(fh, base, sec, sz, path + "/", out)
        else:
            out.append((path, sec, sz, False))
        if right:
            node(right * 4)

    if data:
        node(0)


def main():
    args = sys.argv[1:]
    image = os.path.join(HERE, "..", "orig", "game.xiso.iso")
    if args and not args[0].startswith("--"):
        image = args.pop(0)
    fh, base, rs, rz = open_image(image)
    files = []
    walk(fh, base, rs, rz, "", files)
    if "--extract" in args:
        i = args.index("--extract")
        want, dest = args[i + 1], args[i + 2]
        for path, sec, sz, is_dir in files:
            if not is_dir and path.lower() == want.replace("\\", "/").lower():
                fh.seek(base + sec * SECTOR)
                with open(dest, "wb") as o:
                    o.write(fh.read(sz))
                print("%s -> %s (%d bytes)" % (path, dest, sz))
                return
        sys.exit("not found: " + want)
    if "--summary" in args:
        exts = Counter()
        ebytes = Counter()
        dirs = Counter()
        for path, sec, sz, is_dir in files:
            if is_dir:
                continue
            ext = os.path.splitext(path)[1].lower() or "(none)"
            exts[ext] += 1
            ebytes[ext] += sz
            dirs[path.split("/")[0] if "/" in path else "/"] += 1
        print("files: %d" % sum(exts.values()))
        for ext, n in exts.most_common():
            print("  %-8s %5d files %12d bytes" % (ext, n, ebytes[ext]))
        print("top-level directories:")
        for d, n in sorted(dirs.items()):
            print("  %-24s %5d files" % (d, n))
        return
    for path, sec, sz, is_dir in files:
        print("%10d %s" % (sz, path) if not is_dir else "%10s %s" % ("<dir>", path))


if __name__ == "__main__":
    main()
