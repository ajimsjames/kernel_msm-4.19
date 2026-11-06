#!/usr/bin/env python3
import struct
import sys

def pack_dtbo(dtb_path, out_img):
    with open(dtb_path, "rb") as f:
        dtb_data = f.read()

    dtb_size = len(dtb_data)
    # Header format:
    # magic: 0xd7b7ab1e (uint32 big-endian)
    # total_size: header_size (32) + entry_size (32) + dtb_size
    # header_size: 32
    # dt_entry_size: 32
    # dt_entry_count: 1
    # dt_entries_offset: 32
    # page_size: 2048
    # version: 0
    header_size = 32
    dt_entry_size = 32
    dt_entries_offset = 32
    dtbo_offset = header_size + dt_entry_size  # 64 bytes
    total_size = dtbo_offset + dtb_size
    page_size = 2048
    version = 0

    header = struct.pack(
        ">8I",
        0xd7b7ab1e,
        total_size,
        header_size,
        dt_entry_size,
        1,
        dt_entries_offset,
        page_size,
        version
    )

    # Entry format:
    # dt_size, dt_offset, id, rev, custom[4]
    entry = struct.pack(
        ">8I",
        dtb_size,
        dtbo_offset,
        0, 0, 0, 0, 0, 0
    )

    with open(out_img, "wb") as f:
        f.write(header)
        f.write(entry)
        f.write(dtb_data)

    print(f"[+] Successfully packed {out_img} (Total: {total_size} bytes, DTB: {dtb_size} bytes)")

if __name__ == "__main__":
    if len(sys.argv) < 3:
        print("Usage: pack_dtbo_container.py <input.dtbo> <output_dtbo.img>")
        sys.exit(1)
    pack_dtbo(sys.argv[1], sys.argv[2])
