#!/usr/bin/env python3
"""Native ARM integration: defaults, FPS policy, real rumble timelines, options persistence."""
from pathlib import Path
import os,struct,subprocess,time,zlib
p=Path(__file__).resolve().parents[2];w=p/'build-work'
source=(p/'port/tests/ui-options.py').read_text()
exec(source[:source.index('log=(w/')])
# The shared fixture seeds old options; remove them to test new-install defaults.
settings.unlink()
motor=r/'motor';motor.write_text('0\n')
binaryIndex=cmd.index(str(w/'build/jazz2'))
cmd[binaryIndex:binaryIndex]=['-E','JAZZ2_RUMBLE_PATH='+str(motor)]
logpath=w/'reports/release-options.log'
def state():return struct.unpack('<Iiiii',settings.read_bytes())
def strength():
    try:return int(motor.read_text().split()[0])
    except (ValueError,IndexError):return -1

def fire():
    event(1,0x131,1)
    values=[]
    for i in range(120):
        values.append(strength());time.sleep(.01)
    event(1,0x131,0);time.sleep(.4)
    return values

def snap(name):
    picture(name)
    # Portable PNG, no optional imaging dependency needed.
    import zlib
    raw=fb.read_bytes();rgb=bytearray()
    for b,g,r0,a in struct.iter_unpack('<BBBB',raw):rgb.extend((r0,g,b))
    def chunk(k,v):return struct.pack('>I',len(v))+k+v+struct.pack('>I',zlib.crc32(k+v))
    (w/'reports'/(name+'.png')).write_bytes(b'\x89PNG\r\n\x1a\n'+chunk(b'IHDR',struct.pack('>IIBBBBB',640,480,8,2,0,0,0))+chunk(b'IDAT',zlib.compress(b''.join(b'\0'+rgb[y*1920:(y+1)*1920] for y in range(480))))+chunk(b'IEND',b''))

with logpath.open('wb') as log:
 proc=subprocess.Popen(cmd,cwd=r,stdout=log,stderr=subprocess.STDOUT)
 try:
    time.sleep(9);assert proc.poll() is None
    text=logpath.read_text();assert 'internal 320x240' in text and 'gameplay FPS limit 45' in text,text[-1500:]
    initial=fire();assert max(initial)>0,initial
    key(0x13b) # pause
    assert strength()==0
    assert 'menu FPS limit 30' in logpath.read_text()
    down();key(0x130) # Options
    down();key(0x130) # Graphics
    snap('release-graphics-default')
    # Verify fresh defaults are saved independently of the menu's current 30 FPS.
    key(0x131) # back to options (destructor saves only if dirty: default config already saved on first-run)
    # Reopen graphics, change the gameplay cap while staying in menu.
    key(0x130);down();right() # 45 -> 50
    assert state()==(0x324F4752,1,50,1,1),state()
    right();right() # 60
    assert state()[2]==60
    text=logpath.read_text();assert 'gameplay FPS limit 60' not in text
    key(0x131) # Options
    down();down();key(0x130) # Controls
    down() # vibration on/off
    snap('release-controls-on')
    right();assert state()[3]==0
    assert strength()==0
    down();right();assert state()[4]==2 # intensity high
    right();assert state()[4]==2 # clamp high
    event(3,16,-1);time.sleep(.15);event(3,16,0);time.sleep(.5)
    event(3,16,-1);time.sleep(.15);event(3,16,0);time.sleep(.5)
    assert state()[4]==0
    snap('release-controls-low-off')
    key(0x131);key(0x131);key(0x13b) # controls -> options -> pause -> resume
    assert 'gameplay FPS limit 60' in logpath.read_text()
    assert max(fire())==0 # disabled applies to actual events
    # Return to Controls (fresh pause Options starts on first row).
    key(0x13b);down();key(0x130)
    down();down();down();key(0x130) # controls
    down();right() # enable
    assert state()[3:]==(1,0)
    key(0x131);key(0x131);key(0x13b)
    low=fire();assert max(low)>0 and max(low)<max(initial),(max(low),max(initial))
    key(0x13c);assert proc.wait(timeout=15)==0
    assert strength()==0
    print('Fresh defaults 320x240/45; pause/menu 30; resume/settings 60; rumble on/off/low/medium/high persistence; actual Fire effects and exit-stop: OK',flush=True)
 finally:
    if proc.poll() is None:proc.kill();proc.wait()
os.close(pipe)
