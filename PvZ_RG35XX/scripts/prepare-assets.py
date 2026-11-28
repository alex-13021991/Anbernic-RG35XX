#!/usr/bin/env python3
"""Convert the user's PAK; never download or execute Windows game binaries."""
import argparse,sys,shutil
sys.dont_write_bytecode=True
from pathlib import Path,PurePosixPath
p=argparse.ArgumentParser()
p.add_argument('main_pak',type=Path)
p.add_argument('destination',type=Path)
p.add_argument('--properties',type=Path,help='Original PC properties directory; default: beside main.pak')
a=p.parse_args()
port=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(port/'sources/pvz/tools'))
from optimize_assets import optimize_assets
from pak_lib import PakArchive
properties=a.properties or a.main_pak.parent/'properties'
if not (properties/'partner_logo.jpg').is_file():raise SystemExit('Supply original PC properties directory with --properties (partner_logo.jpg is required)')
a.destination.mkdir(parents=True,exist_ok=True)
optimize_assets(str(a.main_pak),str(a.destination/'main_mmp.pak'),downscale_textures=False,resample_audio=True)
with PakArchive(str(a.main_pak)) as pak:
 for name in pak.list_files():
  rel=PurePosixPath(name.replace(chr(92),'/'))
  if not rel.parts or rel.parts[0].lower()!='properties':continue
  if rel.is_absolute() or '..' in rel.parts:raise ValueError('Unsafe PAK path: '+name)
  target=a.destination/Path(*rel.parts)
  target.parent.mkdir(parents=True,exist_ok=True)
  target.write_bytes(pak.read_file(name))
shutil.copytree(properties,a.destination/'properties',dirs_exist_ok=True)
print(a.destination)
