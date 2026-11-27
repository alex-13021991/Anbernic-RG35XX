#!/usr/bin/env python3
"""Check original 1.0.0 config migration and subsequent custom binding persistence."""
from pathlib import Path
import time,subprocess,struct,os
p=Path(__file__).resolve().parents[2]
source=(p/'port/tests/ui-options.py').read_text()
# Reuse the exact QEMU fixture and config decoder, with no UI exercise.
exec(source[:source.index('log=(w/')])
settings.unlink()
(r/'Saves/Jazz2.config').unlink(missing_ok=True)
def run(binary,tag):
    current=cmd.copy();current[current.index(str(w/'build/jazz2'))]=str(binary)
    with (w/'reports'/('upgrade-'+tag+'.log')).open('wb') as log:
        proc=subprocess.Popen(current,cwd=r,stdout=log,stderr=subprocess.STDOUT)
        try:
            time.sleep(6)
            assert proc.poll() is None
            key(0x13c)
            assert proc.wait(timeout=10)==0
        finally:
            if proc.poll() is None:proc.kill();proc.wait()
    return mappings(r/'Saves/Jazz2.config')[0]
oldBinary=p/'package-public/Roms/PORTS/Jazz Jackrabbit 2/jazz2'
oldBinary.chmod(oldBinary.stat().st_mode | 0o111)
old=run(oldBinary,'old')
assert not settings.exists()
new=run(w/'build/jazz2','new')
assert [new[i][0] for i in [5,6,7,8]]==[old[i][0] for i in [6,7,8,5]],(old[5:9],new[5:9])
# Existing config remains version 15 and all other mappings are retained.
assert new[:5]==old[:5] and new[9:]==old[9:]
assert settings.read_bytes()==struct.pack('<Iiiii',0x324F4752,1,45,1,1)
assert run(w/'build/jazz2','restart')==new
print('Upgrade: exact 1.0.0 layout migrated; remaining mappings and restart state preserved: OK')
os.close(pipe)
