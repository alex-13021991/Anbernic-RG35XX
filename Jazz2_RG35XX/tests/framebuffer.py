#!/usr/bin/env python3
"""Check actual ARM presenter pixels, inversion, scaling, alpha and page bounds."""
import os
from pathlib import Path
import struct
import subprocess
import sys
import tempfile

work = Path(sys.argv[1]).resolve()
width=int(os.environ.get('TEST_VIDEO_WIDTH','304'))
bars = [0xf800,0x07e0,0x001f,0xffe0,0xf81f,0x07ff,0xffff,0]
for bpp, padded, garlic in [(16,False,False),(32,False,False),(16,True,False),
                            (32,True,False),(32,False,True),(32,True,True)]:
    with tempfile.TemporaryDirectory(dir=work, prefix="fb-test-") as tmp:
        root=Path(tmp)
        for name in ["usr","lib"]:
            (root/name).symlink_to(work/"sysroot"/name)
        (root/"dev").mkdir()
        fb=root/"dev/fb0"
        stride=(656 if padded else 640)*(bpp//8)
        fb.write_bytes(bytes(stride*(484 if padded else 480)))
        env=dict(TEST_FB=str(fb), TEST_BPP=str(bpp), LD_PRELOAD=str(work/"tests/simulated-io.so"))
        if garlic: env['TEST_GARLIC_RGB']='1'
        (root/"dev/input").mkdir()
        pad=root/"dev/input/event7"
        os.mkfifo(pad)
        pipe=os.open(pad,os.O_RDWR|os.O_NONBLOCK)
        env["TEST_PAD"]=str(pad)
        events=[]
        for code in [0x130,0x131,0x133,0x134,0x136,0x137,0x13a,0x13b]:
            events += [(1,code,1),(1,code,0)]
        for code in [16,17]:
            events += [(3,code,-1),(3,code,0),(3,code,1),(3,code,0)]
        for code in [2,5]:
            events += [(3,code,255),(3,code,0)]
        events += [(1,0x13c,1),(1,0x13c,0)]
        os.write(pipe,b''.join(struct.pack('<iiHHi',0,0,*event) for event in events))
        if padded: env["TEST_PADDED"]="1"
        cmd=["qemu-arm","-cpu","cortex-a9","-L",str(root)]
        for key,value in env.items(): cmd += ["-E",f"{key}={value}"]
        result=subprocess.run(cmd+[str(work/("tests/video320" if width==320 else "tests/video"))],stdout=subprocess.PIPE,stderr=subprocess.STDOUT,timeout=20)
        os.close(pipe)
        assert result.returncode == 0, result.stdout.decode(errors="replace")
        keys=[line[4:] for line in result.stdout.decode().splitlines() if line.startswith('key=')]
        assert keys == ['Left Alt','Space','Left Shift','Left Ctrl','E','Tab','Right Ctrl','Return',
                        'Left','Right','Up','Down','Tab','Backspace','Home'],keys
        pixels=fb.read_bytes()
        xoff,yoff=(8,2) if padded else (0,0)
        for y in range(480):
            row=pixels[(y+yoff)*stride+xoff*(bpp//8): (y+yoff)*stride+(xoff+640)*(bpp//8)]
            for x,pixel in enumerate(struct.iter_unpack("<H" if bpp==16 else "<I",row)):
                sx=x*width//640
                sy=227-y*228//480
                color=0xffff if sy>207 and sx<20 else bars[sx//(width//8)]
                if bpp==32:
                    r=(color>>11)&31; g=(color>>5)&63; b=color&31
                    color=0xff000000 | ((r<<3|r>>2)<<16) | ((g<<2|g>>4)<<8) | (b<<3|b>>2)
                    if garlic: color=(r<<3|r>>2) | ((g<<2|g>>4)<<8) | ((b<<3|b>>2)<<16)
                assert pixel[0]==color,(bpp,padded,x,y,hex(pixel[0]),hex(color))
        if padded:
            for y in range(484):
                row=pixels[y*stride:(y+1)*stride]
                assert not any(row if y<2 or y>=482 else row[:8*(bpp//8)]+row[648*(bpp//8):])
        print(f"framebuffer {bpp} padded={padded} garlic={garlic}: pixels, input and bounds OK",flush=True)
