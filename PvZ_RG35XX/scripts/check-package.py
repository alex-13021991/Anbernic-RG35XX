#!/usr/bin/env python3
from pathlib import Path
import argparse,zipfile,json,hashlib
p=argparse.ArgumentParser();p.add_argument('zip',type=Path);p.add_argument('--private',action='store_true');a=p.parse_args()
with zipfile.ZipFile(a.zip) as z:
 assert z.testzip() is None
 assert 'Roms/PORTS/Imgs/PlantsVsZombies.png' in z.namelist()
 names=z.namelist();manifest=json.loads(z.read('SHA256SUMS.json'))
 for name,digest in manifest.items():assert hashlib.sha256(z.read(name)).hexdigest()==digest,name
 assert len(manifest)==len(names)-1
 assert not any('/save/' in x or x.endswith(('.exe','.rar','run.log','crash.log')) for x in names)
 assert z.read('Roms/PORTS/PlantsVsZombies/render-width.txt').strip()==b'640'
 assets='Roms/PORTS/PlantsVsZombies/main_mmp.pak' in names
 assert assets==a.private
 if a.private:assert 'Roms/PORTS/PlantsVsZombies/properties/partner_logo.jpg' in names
 else:assert not any('/properties/' in x for x in names)
 print(f'PASS: {a.zip.name}: CRC, SHA256, default 640, assets policy, no saves/logs/Windows executables')
