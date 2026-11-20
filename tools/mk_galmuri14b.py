# t858-C-① galmuri14b.bmp 합성: galmuri14.bmp 글리프를 오른쪽으로 1px 덧그리기(OR).
# 규칙: 셀(16x16) 안에서 out(x,y) = in(x,y) OR in(x-1,y) (ink=0비트).
# 셀 경계 넘침은 셀 안으로 클립 (x%16==0 칸은 원본 유지). 헤더·크기·배치 그대로.
import sys

SRC = sys.argv[1] if len(sys.argv) > 1 else r"/mnt/h/RETRO_SET/PC98_PLAYER/live_v050/Kyuhachi/res/fonts/menu/galmuri14.bmp"   # 사용: python mk_galmuri14b.py <galmuri14.bmp> <출력 galmuri14b.bmp>
DST = sys.argv[2] if len(sys.argv) > 2 else r"/mnt/h/RETRO_SET/PC98_PLAYER/work/20261118_메뉴개편/t858_뮤즈/C/res/fonts/menu/galmuri14b.bmp"

with open(SRC, "rb") as f:
    d = bytearray(f.read())
assert d[:2] == b"BM", "not BMP"
assert len(d) == 524350, "size %d" % len(d)
import struct
off, w, h = struct.unpack("<III", d[10:14] + d[18:22] + d[22:26])
# NOTE: h is signed in BMP header; galmuri sheets are 2048x2048 bottom-up (positive height)
hs = struct.unpack("<i", d[22:26])[0]
assert (off, w, hs) == (62, 2048, 2048), (off, w, hs)

out = bytearray(d)  # header identical


def is_ink(buf, x, y):
    ry = 2047 - y
    return ((buf[62 + ry * 256 + (x >> 3)] >> (7 - (x & 7))) & 1) == 0


def set_ink(buf, x, y):
    ry = 2047 - y
    o = 62 + ry * 256 + (x >> 3)
    buf[o] &= ~(1 << (7 - (x & 7)))


added = 0
for y in range(2048):
    for x in range(2048):
        if x % 16 == 0:
            continue  # clip at cell left edge
        if not is_ink(d, x, y) and is_ink(d, x - 1, y):
            set_ink(out, x, y)
            added += 1

with open(DST, "wb") as f:
    f.write(out)

# 검증: BmpFont::check_mem 상당 (크기·헤더·빈칸 아님) + 샘플 셀 대조
import os
n = os.path.getsize(DST)
ink = sum(
    1
    for y in range(0, 2048, 3)
    for x in range(0, 2048, 3)
    if is_ink(out, x, y)
)
print("wrote", DST)
print("size=", n, "(expect 524350)")
print("added_ink=", added)
print("ink_sample_1/9px=", ink, "(expect >= 2000*?)")
print("header_same=", bytes(d[:62]) == bytes(out[:62]))
