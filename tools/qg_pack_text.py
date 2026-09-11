#!/usr/bin/env python3
# ============================================================
#  qg_pack_text.py - pack plaintext resource into QGX1 blob
#
#  usage: python qg_pack_text.py <master.txt> <out.bin>
#
#  Blob layout (all integers little-endian):
#    0..3   magic "QGX1"
#    4..7   u32 nonce
#    8..11  u32 plain_len
#    12..15 u32 crc32(plaintext)   (IEEE, same as zlib.crc32)
#    16..   ciphertext = plaintext XOR xorshift32 keystream
#
#  keystream seed = fnv1a(KEY) ^ nonce   (0 -> 0x9E3779B9)
#  xorshift32: x^=x<<13; x^=x>>17; x^=x<<5; byte=(x>>24)&0xFF
#
#  The C++ side in data.cpp implements the identical decoder.
#  Master file format (one directive per line):
#    PAGE <title>   new page
#    S <text>       section line      N <text>  note line
#    B <text>       body line         G         gap line
# ============================================================
import sys, os, zlib, struct

KEY = b"QG63-SECURE-TEXT-KEY"
MAGIC = b"QGX1"


def fnv1a(data):
    h = 2166136261
    for b in data:
        h ^= b
        h = (h * 16777619) & 0xFFFFFFFF
    return h


class XorShift:
    def __init__(self, seed):
        self.s = seed & 0xFFFFFFFF

    def next_byte(self):
        x = self.s
        x ^= (x << 13) & 0xFFFFFFFF
        x ^= x >> 17
        x ^= (x << 5) & 0xFFFFFFFF
        self.s = x
        return (x >> 24) & 0xFF


def crypt(data, nonce):
    seed = fnv1a(KEY) ^ nonce
    if seed == 0:
        seed = 0x9E3779B9
    xs = XorShift(seed)
    return bytes(c ^ xs.next_byte() for c in data)


def main():
    if len(sys.argv) != 3:
        print("usage: python qg_pack_text.py <master.txt> <out.bin>")
        return 1
    master_path, out_path = sys.argv[1], sys.argv[2]
    with open(master_path, "rb") as f:
        raw = f.read()
    text = raw.decode("utf-8-sig")
    # normalize newlines: strip \r so C++ only splits on \n
    plain = "\n".join(line.rstrip("\r") for line in text.split("\n")).encode("utf-8")

    pages = sum(1 for ln in plain.split(b"\n")
                if ln.startswith(b"PAGE ") and len(ln) > 5)
    if pages == 0:
        print("ERROR: master has no PAGE lines")
        return 1

    nonce = struct.unpack("<I", os.urandom(4))[0]
    blob = MAGIC + struct.pack("<III", nonce, len(plain),
                               zlib.crc32(plain) & 0xFFFFFFFF)
    blob += crypt(plain, nonce)
    with open(out_path, "wb") as f:
        f.write(blob)

    # self-check: decrypt what we just wrote and compare
    with open(out_path, "rb") as f:
        chk = f.read()
    n2, l2, c2 = struct.unpack("<III", chk[4:16])
    back = crypt(chk[16:16 + l2], n2)
    ok = (chk[:4] == MAGIC and len(back) == len(plain)
          and zlib.crc32(back) & 0xFFFFFFFF == c2 and back == plain)

    import hashlib
    print("pages=%d plain=%d blob=%d sha256=%s roundtrip=%s"
          % (pages, len(plain), len(blob),
             hashlib.sha256(blob).hexdigest()[:16], "OK" if ok else "FAIL"))
    leak = [w for w in (b"Simalth", b"lloj.dev", "再次提醒".encode())
            if w in blob]
    print("plaintext-leak-check:", "CLEAN" if not leak else leak)
    return 0 if ok and not leak else 1


if __name__ == "__main__":
    sys.exit(main())
