#!/usr/bin/env python3
"""Assemble private test package outside the public repository, with dependency closure."""
import argparse
import hashlib
from pathlib import Path
import re
import shutil
import subprocess
import zipfile

p=argparse.ArgumentParser()
p.add_argument('work',type=Path)
p.add_argument('content',type=Path)
p.add_argument('output',type=Path)
args=p.parse_args()
port=Path(__file__).resolve().parents[1]
stage=args.output/'Jazz2-RG35XX-3.7.0-test'
app=stage/'Roms/PORTS/Jazz Jackrabbit 2'
if stage.exists(): raise SystemExit(f'Refusing to overwrite {stage}; choose a new output directory')
shutil.copytree(port/'RG35XXPackage',stage)
shutil.copy2(args.work/'build/jazz2',app/'jazz2')
(app/'lib').mkdir()
(app/'Saves').mkdir()
(app/'Logs').mkdir()
# GarlicOS provides its loader and libc. All other DT_NEEDED objects ship locally.
system={'libc.so.0','ld-uClibc.so.0'}
queue=[app/'jazz2']
seen=set()
while queue:
    obj=queue.pop()
    data=subprocess.check_output(['readelf','-d',str(obj)],text=True,env={'LC_ALL':'C','PATH':'/usr/bin:/bin'})
    for name in re.findall(r'\(NEEDED\).*?\[(.*?)\]',data):
        if name in seen or name in system: continue
        seen.add(name)
        source=next((args.work/'sysroot'/folder/name for folder in ('usr/lib','lib') if (args.work/'sysroot'/folder/name).is_file()),None)
        if source is None: raise RuntimeError(f'Missing {name}')
        target=app/'lib'/name
        shutil.copyfile(source.resolve(),target)
        queue.append(target)
shutil.copytree(args.content,app/'Content')
shutil.copy2(port/'LICENSE',app/'LICENSE')
shutil.copy2(port/'docs/TESTING-RU.txt',stage/'TESTING-RU.txt')
# Diagnostic binaries use the same local libraries as the game.
(app/'Tools').mkdir()
for name in ('video','music'):
    shutil.copy2(args.work/'tests'/name,app/'Tools'/name)
manifest=[]
for file in sorted(stage.rglob('*')):
    if file.is_file(): manifest.append(hashlib.sha256(file.read_bytes()).hexdigest()+'  '+file.relative_to(stage).as_posix())
(stage/'SHA256SUMS.txt').write_text('\n'.join(manifest)+'\n')
archive=args.output/(stage.name+'.zip')
with zipfile.ZipFile(archive,'w',zipfile.ZIP_DEFLATED,compresslevel=6) as z:
    for file in sorted(stage.rglob('*')):
        if file.is_file(): z.write(file,file.relative_to(stage))
with zipfile.ZipFile(archive) as z: assert z.testzip() is None
print(archive)
print('Bundled libraries:',', '.join(sorted(seen)))
print('ZIP SHA256:',hashlib.sha256(archive.read_bytes()).hexdigest())
