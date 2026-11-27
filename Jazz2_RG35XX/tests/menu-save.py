#!/usr/bin/env python3
"""MENU saves a native resumable state; next launch can Continue; failed save keeps the game running."""
from pathlib import Path
import os,struct,subprocess,time
p=Path(__file__).resolve().parents[2];w=p/'build-work'
source=(p/'port/tests/ui-options.py').read_text()
exec(source[:source.index('log=(w/')])
settings.write_bytes(struct.pack('<Iiiii',0x324F4752,1,45,1,1))
resume=r/'Saves/Jazz2.resume'
if resume.is_file():resume.unlink()
logpath=w/'reports/menu-save.log'
with logpath.open('wb') as log:
 proc=subprocess.Popen(cmd,cwd=r,stdout=log,stderr=subprocess.STDOUT)
 try:
    time.sleep(8);assert proc.poll() is None
    # Force an open failure at the native save filename.
    resume.mkdir();key(0x13c);time.sleep(.5)
    assert proc.poll() is None and 'MENU save failed; staying in game' in logpath.read_text()
    resume.rmdir()
    event(3,16,1);time.sleep(1);event(3,16,0);time.sleep(.3)
    key(0x13c);assert proc.wait(timeout=15)==0
    assert 'MENU saved game state' in logpath.read_text()
    assert resume.stat().st_size>100
    raw=resume.read_bytes();assert struct.unpack_from('<Q',raw)[0]==0x2095A59FF0BFBBEF
 finally:
    if proc.poll() is None:proc.kill();proc.wait()
# Drain a possibly unconsumed Home release from the previous process.
try:
 while os.read(pipe,4096):pass
except BlockingIOError:pass
menuCmd=cmd[:-2] # Remove /level and name; start the normal main menu.
with (w/'reports/menu-resume.log').open('wb') as log:
 proc=subprocess.Popen(menuCmd,cwd=r,stdout=log,stderr=subprocess.STDOUT)
 try:
    time.sleep(6);assert proc.poll() is None
    key(0x130);time.sleep(2) # Skip the intro cinematic.
    key(0x130);time.sleep(4) # Continue is the first main-menu row.
    text=(w/'reports/menu-resume.log').read_text()
    assert 'Resuming saved state' in text and 'gameplay FPS limit' in text,text[-2000:]
    key(0x13c);assert proc.wait(timeout=15)==0
    assert resume.stat().st_size>100
 finally:
    if proc.poll() is None:proc.kill();proc.wait()
os.close(pipe)
print('MENU: write failure refuses exit; successful native save + system exit; next launch Continue restores gameplay: OK')
