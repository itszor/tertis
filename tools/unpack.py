#!/usr/bin/env python3
"""The game, unpacked by !RunImage's own decompressor.

    unpack.py           game/!Tertis/Data/TertisCode -> unpacked/tertiscode
                        (the image !RunImage runs, at &267FC)
                        game/!Tertis/Data/AllMods -> unpacked/allmods (the
                        game's voice modules, as it decompresses them to &8100),
                        and unpacked/mod<N> each one (N = 1-9, in its order:
                        at &8100 + the table's Nth offset)

!Run runs !RunImage (an absolute at &8000, assembled by the source's
Code/!RunImage). It decompresses Data.StartPic onto the screen, then
OS_File-loads Data.TertisCode at &70000 and decompresses it to &267FC
(&8214-&8244: the decompressor at &8394, which returns repeatedly through
ldmneia r1!,{pc} until it's done; the image ends at its last write) and jumps
to &749F4, the game's entry. Here that part of !RunImage runs under
SuperSpider's tools/arm2.py.

The game decompresses Data.AllMods to &8100 with the same decompressor (its
own copy) and inserts the nine voice modules it holds in place (OS_Module
10): it starts with their offsets from &8100.
"""
import os
import struct
import sys

SS = os.environ.get('SUPERSPIDER', os.path.expanduser('~/stuff/superspider'))
sys.path.insert(0, os.path.join(SS, 'tools'))
import arm2  # noqa: E402

TOP = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..')
APP = os.path.join(TOP, 'game', '!Tertis')
OUT = os.path.join(TOP, 'unpacked')
RUNIMAGE, PACKED, GAME, ENTRY = 0x8000, 0x70000, 0x267FC, 0x749F4
MODS = 0x8100
START, DONE = 0x8220, 0x8248              # r0 the packed data, r2 where to .. the end


def find(d, name):
    """a RISC OS file in a host directory: any case, any ,load-exec suffix"""
    for f in os.listdir(d):
        if f.split(',')[0].lower() == name.lower():
            return os.path.join(d, f)
    raise FileNotFoundError(os.path.join(d, name))


def decompress(runimage, packed, to):
    """packed decompressed to `to` by !RunImage's decompressor"""
    m = arm2.ARM(memsize=0x4000000)
    m.mem[RUNIMAGE:RUNIMAGE + len(runimage)] = runimage
    m.mem[PACKED:PACKED + len(packed)] = packed
    m.r[13] = 0x7F000
    m.r[0], m.r[2] = PACKED, to
    end = [to]                              # the decompressor's last write

    def wrote(a, n):
        if to <= a < max(to, PACKED) + 0x100000 and a + n > end[0]:
            end[0] = a + n
    m.write_hook = wrote
    m.r[15] = START
    while m.r[15] != DONE:
        m.step()
        if m.steps > 200_000_000:
            raise RuntimeError('unpack: no end, pc=&%X' % m.r[15])
    return bytes(m.mem[to:end[0]])


def main():
    os.makedirs(OUT, exist_ok=True)
    runimage = open(find(APP, '!RunImage'), 'rb').read()
    data = os.path.join(APP, 'Data')
    game = decompress(runimage, open(find(data, 'TertisCode'), 'rb').read(), GAME)
    if not GAME <= ENTRY < GAME + len(game):
        sys.exit('unpack: TertisCode decompressed to &%X-&%X (a damaged dump?)' % (GAME, GAME + len(game)))
    open(os.path.join(OUT, 'tertiscode'), 'wb').write(game)
    print('tertiscode: %d bytes at &%X, entry &%X' % (len(game), GAME, ENTRY))
    # (decompressed elsewhere: at &8100 it would overwrite the decompressor)
    mods = decompress(runimage, open(find(data, 'AllMods'), 'rb').read(), 0x100000)
    if not struct.unpack_from('<I', mods, 0)[0] < len(mods):
        sys.exit('unpack: AllMods decompressed to no module table (a damaged dump?)')
    open(os.path.join(OUT, 'allmods'), 'wb').write(mods)
    print('allmods: %d bytes at &%X' % (len(mods), MODS))
    for n, off in enumerate(struct.unpack_from('<9I', mods, 0), 1):
        size = struct.unpack_from('<I', mods, off - 4)[0]
        title = mods[off + struct.unpack_from('<I', mods, off + 0x10)[0]:].split(b'\0')[0].decode('latin-1')
        open(os.path.join(OUT, 'mod%d' % n), 'wb').write(mods[off:off + size])
        print('mod%d: %s, %d bytes at &%X' % (n, title, size, MODS + off))


if __name__ == '__main__':
    main()
