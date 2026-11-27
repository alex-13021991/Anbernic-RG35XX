#!/usr/bin/env python3
from pathlib import Path
import os,struct,subprocess,time
p=Path(__file__).resolve().parents[2];w=p/'build-work'
source=(p/'port/tests/ui-options.py').read_text()
exec(source[:source.index('log=(w/')])
expected=struct.pack('<Iiiii',0x324F4752,3,55,0,2)
settings.write_bytes(expected)
motor=r/'motor';motor.write_text('0\n')
index=cmd.index(str(w/'build/jazz2'))
cmd[index:index]=['-E','JAZZ2_RUMBLE_PATH='+str(motor)]
with (w/'reports/persistence-options.log').open('wb') as log:
 proc=subprocess.Popen(cmd,cwd=r,stdout=log,stderr=subprocess.STDOUT)
 try:
    time.sleep(7);assert proc.poll() is None
    event(1,0x131,1);time.sleep(.7);event(1,0x131,0);time.sleep(.3)
    assert int(motor.read_text().split()[0])==0
    key(0x13c);assert proc.wait(timeout=15)==0
    assert settings.read_bytes()==expected
 finally:
    if proc.poll() is None:proc.kill();proc.wait()
os.close(pipe)
print('RGO2 reload: resolution 480x360, gameplay 55 FPS, vibration Off, intensity High preserved: OK')
