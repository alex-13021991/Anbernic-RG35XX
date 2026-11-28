#!/usr/bin/env python3
"""Exercise all 65536 RGB565 values, channel layouts, alpha and padded source/destination."""
import os,struct,subprocess,tempfile,itertools
from pathlib import Path
import sys
work=Path(sys.argv[1]).resolve()
width=int(os.environ.get("TEST_VIDEO_WIDTH","320"))
height=int(os.environ.get("TEST_VIDEO_HEIGHT","240"))
binary=work/("tests/video-colors384" if width==384 else "tests/video-colors")
layouts=[(0,8,16,24,0),(16,8,0,24,0)]
for slots in itertools.permutations([0,8,16,24]):
    layouts.append((*slots[:3],slots[3],8))
for bpp,layout in [(16,None)]+[(32,c) for c in layouts]:
 for padded in [False,True]:
  with tempfile.TemporaryDirectory(dir=work,prefix="colors-") as td:
   root=Path(td)
   for name in ["usr","lib"]: (root/name).symlink_to(work/"sysroot"/name)
   (root/"dev").mkdir(); fb=root/"dev/fb0"
   stride=(656 if padded else 640)*(bpp//8)
   fb.write_bytes(bytes(stride*(484 if padded else 480)))
   env=dict(TEST_FB=str(fb),TEST_BPP=str(bpp),LD_PRELOAD=str(work/"tests/simulated-io.so"))
   if padded: env["TEST_PADDED"]="1"
   if layout: env["TEST_CHANNELS"]=":".join(map(str,layout))
   cmd=["qemu-arm","-cpu","cortex-a9","-L",str(root)]
   for k,v in env.items():cmd+=["-E",k+"="+v]
   subprocess.run(cmd+[str(binary)],stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL,check=True,timeout=10)
   pixels=fb.read_bytes(); xo,yo=(8,2) if padded else (0,0)
   for y in range(480):
    start=(y+yo)*stride+xo*(bpp//8)
    row=pixels[start:start+640*(bpp//8)]
    for x,(actual,) in enumerate(struct.iter_unpack("<H" if bpp==16 else "<I",row)):
     value=((height-1-y*height//480)*width+x*width//640)&65535
     expected=value
     if layout:
      ro,go,bo,ao,al=layout
      rv=value>>11;gv=(value>>5)&63;bv=value&31
      expected=((rv<<3|rv>>2)<<ro)|((gv<<2|gv>>4)<<go)|((bv<<3|bv>>2)<<bo)|((255<<ao) if al else 0)
     assert actual==expected,(bpp,layout,padded,x,y,hex(actual),hex(expected))
   if padded:
    for y in range(484):
     row=pixels[y*stride:(y+1)*stride]
     assert not any(row if y<2 or y>=482 else row[:xo*(bpp//8)]+row[(xo+640)*(bpp//8):])
  print("all RGB565 colors OK",bpp,layout,"padded",padded,flush=True)
