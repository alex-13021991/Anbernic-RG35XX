#!/usr/bin/env python3
"""Keep actual game effects running for 65 seconds, with a temporary motor I/O outage."""
from pathlib import Path
import os,struct,subprocess,time
p=Path(__file__).resolve().parents[2];w=p/'build-work'
source=(p/'port/tests/ui-options.py').read_text()
exec(source[:source.index('log=(w/')])
settings.write_bytes(struct.pack('<Iiiii',0x324F4752,1,45,1,1))
motor=r/'motor';motor.write_text('0\n');absent=r/'motor-away'
if absent.exists():absent.unlink()
index=cmd.index(str(w/'build/jazz2'))
cmd[index:index]=['-E','JAZZ2_RUMBLE_PATH='+str(motor)]
logpath=w/'reports/rumble-long.log'
with logpath.open('wb') as log:
 proc=subprocess.Popen(cmd,cwd=r,stdout=log,stderr=subprocess.STDOUT)
 try:
    time.sleep(8);assert proc.poll() is None
    event(1,0x131,1)
    begin=time.monotonic();bins=[0]*13;hidden=False;restored=False
    while time.monotonic()-begin<65:
        t=time.monotonic()-begin
        if t>=20 and not hidden:motor.rename(absent);hidden=True
        if t>=22 and not restored:absent.rename(motor);restored=True
        try:n=int(motor.read_text().split()[0])
        except (FileNotFoundError,IndexError,ValueError):n=0
        bins[min(int(t//5),12)]=max(bins[min(int(t//5),12)],n)
        assert proc.poll() is None
        time.sleep(.01)
    event(1,0x131,0);time.sleep(.4)
    assert all(n>0 for i,n in enumerate(bins) if i!=4),bins
    assert 'motor I/O error' in logpath.read_text() and 'motor I/O recovered' in logpath.read_text()
    key(0x13c);assert proc.wait(timeout=15)==0
    assert int(motor.read_text().split()[0])==0
    assert (r/'Saves/Jazz2.resume').stat().st_size>100
    print('65-second actual Fire effects: nonzero strength in all 5-second windows; temporary outage at 20..22 seconds recovered; MENU saved/exited; bins='+str(bins),flush=True)
 finally:
    if proc.poll() is None:proc.kill();proc.wait()
os.close(pipe)
