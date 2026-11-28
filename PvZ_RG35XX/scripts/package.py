#!/usr/bin/env python3
from pathlib import Path
import argparse,os,shutil,subprocess,re,zipfile,hashlib,json
p=argparse.ArgumentParser();p.add_argument('--out',required=True);p.add_argument('--assets',type=Path);a=p.parse_args()
r=Path(__file__).resolve().parents[1];w=Path(os.environ.get('PVZ_WORK',str(r/'.work')));sdk=Path(os.environ.get('RG_TOOLCHAIN',str(Path.home()/'RG35XX/RG35XX-SDK/miyoo-gcc-7.3.0')));sysroot=Path(os.environ.get('RG_SYSROOT',str(w/'sysroot')))
out=Path(a.out).resolve();out.mkdir(parents=True,exist_ok=True)
stage=w/'package';stage.mkdir(parents=True,exist_ok=True)
root=stage/'public'
if root.exists():shutil.rmtree(root)
shutil.copytree(r/'Package',root)
game=root/'Roms/PORTS/PlantsVsZombies';shutil.copy2(w/'build/PvZ',game/'PvZ');(game/'lib').mkdir()
# System loader/uClibc are supplied by GarlicOS; recursively package other SONAMEs.
excluded={'libc.so.0','libm.so.0','libdl.so.0','libpthread.so.0','librt.so.0','libutil.so.0','libresolv.so.0','ld-uClibc.so.0','ld-uClibc.so.1'}
todo=[game/'PvZ'];seen=set()
while todo:
 obj=todo.pop();data=subprocess.check_output(['readelf','-d',str(obj)],text=True)
 for name in re.findall(r'NEEDED.*\[(.*?)\]',data):
  if name in excluded or name in seen:continue
  seen.add(name)
  candidates=[sysroot/'usr/lib'/name,sysroot/'lib'/name]
  source=next((x for x in candidates if x.exists()),None)
  if source is None:raise RuntimeError('Missing runtime library: '+name)
  dest=game/'lib'/name;shutil.copy2(source.resolve(),dest);todo.append(dest)
env=dict(os.environ,LD_LIBRARY_PATH=str(sdk/'lib'))
subprocess.run([str(sdk/'bin/arm-linux-strip'),str(game/'PvZ'),*[str(x) for x in (game/'lib').iterdir()]],env=env,check=True)
shutil.copytree(r/'LICENSES',game/'LICENSES');shutil.copy2(r/'LICENSE',game/'LICENSE')
shutil.copy2(r/'docs/INSTALL-RU.txt',game/'README_RU.txt');shutil.copy2(r/'docs/TESTING-RU.md',game/'TESTING-RU.md')
def archive(folder,name):
 sums={str(f.relative_to(folder)):hashlib.sha256(f.read_bytes()).hexdigest() for f in sorted(folder.rglob('*')) if f.is_file() and f.name!='SHA256SUMS.json'}
 (folder/'SHA256SUMS.json').write_text(json.dumps(sums,indent=2)+'\n')
 with zipfile.ZipFile(out/name,'w',zipfile.ZIP_DEFLATED,compresslevel=6) as z:
  for f in sorted(folder.rglob('*')):
   if f.is_file():
    i=zipfile.ZipInfo.from_file(f,str(f.relative_to(folder)));i.external_attr=((0o100755 if f.suffix=='.sh' or f.name=='PvZ' else 0o100644)<<16)
    z.writestr(i,f.read_bytes(),compress_type=zipfile.ZIP_DEFLATED)
 print(out/name)
archive(root,'PvZ_RG35XX_v1.0.1.zip')
if a.assets:
 private=stage/'private'
 if private.exists():shutil.rmtree(private)
 shutil.copytree(root,private);target=private/'Roms/PORTS/PlantsVsZombies'
 shutil.copy2(a.assets/'main_mmp.pak',target/'main_mmp.pak');shutil.copytree(a.assets/'properties',target/'properties')
 archive(private,'PvZ_RG35XX_v1.0.1_ready-to-play.zip')
