#!/usr/bin/env python3
"""Build a public installer and complete source archive, without original game data."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import tarfile
import zipfile

parser=argparse.ArgumentParser()
parser.add_argument('work',type=Path)
parser.add_argument('output',type=Path)
parser.add_argument('--converter',type=Path,help='Windows AssetPacker; defaults to included Tools version')
args=parser.parse_args()
port=Path(__file__).resolve().parents[1]
stem='Jazz2_RG35XX_v1.0.0'
stage=args.output/stem
if stage.exists(): raise SystemExit('Refusing to overwrite '+str(stage))
args.output.mkdir(parents=True,exist_ok=True)
shutil.copytree(port/'RG35XXPackage',stage)
app=stage/'Roms/PORTS/Jazz Jackrabbit 2'
shutil.copy2(args.work/'build/jazz2',app/'jazz2')
(app/'lib').mkdir()
queue=[app/'jazz2']
seen=set()
while queue:
    obj=queue.pop()
    info=subprocess.check_output(['readelf','-d',str(obj)],text=True,env={**os.environ,'LC_ALL':'C'})
    for name in re.findall(r'\(NEEDED\).*?\[(.*?)\]',info):
        if name in seen or name in {'libc.so.0','ld-uClibc.so.0'}: continue
        seen.add(name)
        source=next((args.work/'sysroot'/folder/name for folder in ('usr/lib','lib') if (args.work/'sysroot'/folder/name).is_file()),None)
        if source is None: raise RuntimeError('Missing '+name)
        target=app/'lib'/name
        shutil.copyfile(source.resolve(),target)
        queue.append(target)
shutil.copy2(port/'LICENSE',app/'LICENSE')
shutil.copytree(port/'LICENSES',stage/'LICENSES')
shutil.copytree(port/'Tools',stage/'Tools',ignore=shutil.ignore_patterns('Content','__pycache__','*.pyc'))
if args.converter: shutil.copy2(args.converter,stage/'Tools/AssetPacker.exe')
shutil.copy2(port/'scripts/convert.py',stage/'Tools/convert.py')
shutil.copy2(port/'docs/CONVERTING.md',stage/'Tools/CONVERTING.md')
shutil.copy2(port/'docs/DEPENDENCIES.md',stage/'LICENSES/DEPENDENCIES.md')
shutil.copy2(port/'docs/INSTALL-RU.txt',stage/'README_RU.txt')
shutil.copy2(port/'docs/BUILD.md',stage/'BUILD.md')
shutil.copy2(port/'vendor/SHA256SUMS',stage/'LICENSES/SOURCE-ARCHIVE-SHA256SUMS')
for filename in ['AssetPacker.exe','AssetPacker-linux']:
    if not (stage/'Tools'/filename).is_file(): raise RuntimeError('Missing converter '+filename)
# These must never enter the public installation or source bundle.
forbidden={'.j2a','.j2l','.j2t','.j2b','.j2v','.j2s','.j2e','.j2d'}
def audit(base):
    for file in base.rglob('*'):
        if file.is_file() and (file.suffix.lower() in forbidden or file.name.lower().startswith('prebaked.')
                               or file.name.lower() in {'jazz2.exe','goggame-1351891846.info'}):
            raise RuntimeError('Original or converted game data detected: '+str(file))
audit(stage)
checks=[]
for file in sorted(stage.rglob('*')):
    if file.is_file(): checks.append(hashlib.sha256(file.read_bytes()).hexdigest()+'  '+file.relative_to(stage).as_posix())
(stage/'SHA256SUMS.txt').write_text('\n'.join(checks)+'\n')
archive=args.output/(stem+'.zip')
with zipfile.ZipFile(archive,'w',zipfile.ZIP_DEFLATED,compresslevel=6) as z:
    for file in sorted(stage.rglob('*')):
        if file.is_file(): z.write(file,file.relative_to(stage))
with zipfile.ZipFile(archive) as z:
    if z.testzip() is not None: raise RuntimeError('ZIP failed integrity test')
audit(port)
source=args.output/(stem+'-source.tar.gz')
with tarfile.open(source,'w:gz') as t:
    for file in sorted(port.rglob('*')):
        rel=file.relative_to(port)
        if any(part in {'build','dist','__pycache__'} for part in rel.parts) or file.suffix=='.pyc': continue
        if file.is_file(): t.add(file,arcname='Jazz2_RG35XX/'+rel.as_posix(),recursive=False)
hashes=[]
for file in [archive,source]:
    hashes.append(hashlib.sha256(file.read_bytes()).hexdigest()+'  '+file.name)
    print(str(file),file.stat().st_size)
(args.output/'SHA256SUMS.txt').write_text('\n'.join(hashes)+'\n')
print('Libraries:',', '.join(sorted(seen)))
print('No original/converted game data in public package')
