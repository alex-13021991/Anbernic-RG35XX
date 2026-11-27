#!/usr/bin/env python3
"""Exercise actual ARM menu code with RG35XX evdev events under QEMU."""
from pathlib import Path
import os, struct, subprocess, time, zlib, shutil
p=Path(__file__).resolve().parents[2]
w=p/'build-work'
r=w/'ui-test'
r.mkdir(exist_ok=True)
for part in ['usr','lib']:
    if not (r/part).exists(): (r/part).symlink_to(w/'sysroot'/part)
(r/'dev/input').mkdir(parents=True,exist_ok=True)
fb=r/'dev/fb0'; fb.write_bytes(bytes(640*480*4))
pad=r/'dev/input/event7'
if not pad.exists(): os.mkfifo(pad)
pipe=os.open(pad,os.O_RDWR|os.O_NONBLOCK)
if not (r/'Content').exists(): (r/'Content').symlink_to(p/'package-private/Roms/PORTS/Jazz Jackrabbit 2/Content')
(r/'Saves').mkdir(exist_ok=True)
(r/'Saves/Jazz2.config').unlink(missing_ok=True)
settings=r/'Saves/RG35XX.options'
settings.write_bytes(struct.pack('<Iii',0x314F4752,0,30))
env=dict(SDL_VIDEODRIVER='dummy',JAZZ2_RESOLUTION='640x480',LANG='ru_RU.UTF-8',ALSOFT_DRIVERS='null',HOME=str(r/'Saves'),TEST_FB=str(fb),TEST_PAD=str(pad),LD_PRELOAD=str(w/'tests/simulated-io.so'),TEST_BPP='32')
cmd=['qemu-arm','-cpu','cortex-a9','-L',str(r)]
for k,v in env.items(): cmd+=['-E',k+'='+v]
cmd += [str(w/'build/jazz2'),'/config','Saves/Jazz2.config','/level','prince/01_castle1']
def event(t,c,v): os.write(pipe,struct.pack('<iiHHi',0,0,t,c,v))
def key(c):
    event(1,c,1); time.sleep(.15); event(1,c,0); time.sleep(.55)
def down():
    event(3,17,1); time.sleep(.15); event(3,17,0); time.sleep(.5)
def right():
    event(3,16,1); time.sleep(.15); event(3,16,0); time.sleep(.5)
def picture(name):
    # fb storage is bottom-up relative to the game renderer; presenter already flips it.
    raw=fb.read_bytes(); rgb=bytearray()
    for b,g,r,a in struct.iter_unpack('<BBBB',raw): rgb.extend((r,g,b))
    target=w/'reports'/name
    target.with_suffix('.ppm').write_bytes(b'P6\n640 480\n255\n'+rgb)
    try:
        from PIL import Image
        Image.frombytes('RGB',(640,480),bytes(rgb)).save(target.with_suffix('.png'))
    except ImportError: pass

def mappings(path):
    raw=path.read_bytes()
    assert raw[9]==15
    data=zlib.decompress(raw[10:],-15)
    flags=struct.unpack_from('<Q',data)[0]
    pos=8+(6 if flags&0x20000 else 0)+4+1+3+1+1+1+16
    length=0; shift=0
    while True:
        b=data[pos];pos+=1;length|=(b&127)<<shift;shift+=7
        if not b&128:break
    pos+=length+16+1+7*6
    players,count=data[pos:pos+2];pos+=2
    result=[]
    for i in range(players):
        rows=[]
        for j in range(count):
            n=data[pos];pos+=1
            rows.append(list(struct.unpack_from('<'+'I'*n,data,pos)));pos+=n*4
        result.append(rows)
    return result
log=(w/'reports/ui-options.log').open('wb')
process=subprocess.Popen(cmd,cwd=r,stdout=log,stderr=subprocess.STDOUT)
try:
    time.sleep(9)
    assert process.poll() is None
    key(0x13b) # START pause
    down();key(0x130) # Options
    down();key(0x130) # Graphics
    picture('graphics-default')
    right() # internal 320x240
    down();right() # limit 35
    assert struct.unpack('<Iiiii',settings.read_bytes())==(0x324F4752,1,35,1,1),settings.read_bytes()
    picture('graphics-selected')
    # Exercise all seven values and both clamped FPS endpoints.
    for fps in [40,45,50,55,60,60]:
        right()
        assert struct.unpack('<Iiiii',settings.read_bytes())[2]==fps
    for i in range(7):
        event(3,16,-1);time.sleep(.15);event(3,16,0);time.sleep(.5)
    assert struct.unpack('<Iiiii',settings.read_bytes())[2]==30
    right()
    key(0x131) # B must go back, never confirm
    assert struct.unpack('<Iiiii',settings.read_bytes())==(0x324F4752,1,35,1,1)
    down();down();key(0x130) # Controls
    key(0x130) # Remap
    for i in range(5):down() # Fire row
    picture('remap-before')
    before=mappings(r/'Saves/Jazz2.config')[0]
    assert len({before[i][0] for i in [5,6,7,8]})==4,before[5:9]
    key(0x130) # begin capture
    key(0x130) # assign A to Fire: swaps Jump/Fire
    picture('remap-swapped')
    key(0x131) # leave/save
    after=mappings(r/'Saves/Jazz2.config')[0]
    assert after[5][0]==before[6][0] and after[6][0]==before[5][0],(before[5:9],after[5:9])
    assert after[7:9]==before[7:9]
    key(0x13c)
    assert process.wait(timeout=10)==0
    print('UI: graphics 304 -> 320; FPS 30 -> 35; B/back; Fire/Jump primary swap; config saved: OK',flush=True)
finally:
    if process.poll() is None:process.kill();process.wait()
    log.close();os.close(pipe)
