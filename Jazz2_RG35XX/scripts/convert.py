#!/usr/bin/env python3
"""Convert a user's installed GOG game without modifying the input directory."""
import argparse
from pathlib import Path
import shutil
import subprocess
import tempfile

p=argparse.ArgumentParser()
p.add_argument('game',type=Path,help='Directory containing Anims.j2a (installed GOG game)')
p.add_argument('output',type=Path,help='New output Content directory')
p.add_argument('--packer',type=Path)
p.add_argument('--engine-content',type=Path)
a=p.parse_args()
port=Path(__file__).resolve().parents[1]
packer=a.packer or port/'Tools/AssetPacker-linux'
engine=a.engine_content or port/'Tools/EngineContent'
game=a.game.resolve()
files={f.name.lower():f for f in game.iterdir() if f.is_file()}
if 'anims.j2a' not in files: p.error('Anims.j2a not found: specify the installed game directory')
if a.output.exists(): p.error('Output already exists; select a new directory to avoid mixing data')
if not packer.is_file() or not (engine/'Metadata').is_dir(): p.error('Packer or EngineContent missing')
with tempfile.TemporaryDirectory(prefix='jazz2-convert-') as tmp:
    staging=Path(tmp)
    shutil.copytree(game,staging/'Source')
    shutil.copytree(engine,staging/'Content')
    subprocess.run([str(packer.resolve()),str(staging),str(a.output.resolve()),'--target=console',
                    '--originals-only','--skip-non-episode-levels','--video-downscale=2'],check=True)
for track in ('menu','bonus2','bonus3','intro','ending'):
    source=files.get(track+'.j2b')
    if source:
        (a.output/'Music').mkdir(exist_ok=True)
        shutil.copy2(source,a.output/'Music'/(track+'.j2b'))
    else: print('Optional music not found:',track+'.j2b')
for name in ('Prebaked.pak','Prebaked.version'):
    if not (a.output/name).is_file(): raise RuntimeError('Conversion incomplete: '+name)
print('Converted Content:',a.output.resolve())
