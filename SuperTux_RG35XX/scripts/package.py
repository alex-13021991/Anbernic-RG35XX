#!/usr/bin/env python3
"""Собрать установочные ZIP en/ru и полный набор исходников с патчами."""
from pathlib import Path
import argparse,os,shutil,subprocess,tempfile,zipfile,hashlib
ROOT=Path(__file__).resolve().parents[1]
ap=argparse.ArgumentParser(description=__doc__)
ap.add_argument('--variant',choices=['en','ru','both'],default='both')
ap.add_argument('--work',type=Path,default=Path(os.environ.get('SUPERTUX_WORK',str(ROOT/'.work'))))
ap.add_argument('--out',type=Path,default=ROOT/'dist')
args=ap.parse_args();args.out.mkdir(parents=True,exist_ok=True)
sdk=Path(os.environ.get('SDK',str(Path.home()/'RG35XX/RG35XX-SDK/miyoo-gcc-7.3.0')))
def zip_tree(source,path,prefix=''):
    with zipfile.ZipFile(path,'w',zipfile.ZIP_DEFLATED,compresslevel=9) as z:
        for item in sorted(source.rglob('*')):
            if not item.is_file():continue
            rel=item.relative_to(source)
            if any(x in {'.work','dist','.git','__pycache__'} for x in rel.parts):continue
            z.write(item,str(Path(prefix)/rel))
    with zipfile.ZipFile(path) as z:
        assert z.testzip() is None
    print(path)
for variant in (['en','ru'] if args.variant=='both' else [args.variant]):
    source=args.work/variant/'source'
    if not (source/'supertux').is_file():raise SystemExit('Сначала выполните build.sh '+variant)
    with tempfile.TemporaryDirectory(prefix='supertux-package-',dir=args.work) as td:
        stage=Path(td);shutil.copytree(ROOT/'package',stage,dirs_exist_ok=True)
        app=stage/'Roms/PORTS/SuperTux'
        shutil.copytree(source/'data',app/'data')
        shutil.copy2(source/'supertux',app/'supertux')
        subprocess.run([str(sdk/'bin/arm-linux-strip'),str(app/'supertux')],check=True)
        (app/'supertux').chmod(0o755)
        for name in ['COPYING','AUTHORS']:shutil.copy2(source/name,app/name)
        shutil.copy2(ROOT/'docs/INSTALL-RU.txt',app/'README-RU.txt')
        shutil.copy2(ROOT/'sources/provenance.json',app/'PROVENANCE.json')
        (app/'VERSION').write_text(('0.2.1-ru' if variant=='ru' else '0.1.1-en')+'\n')
        shutil.copy2(ROOT/'docs/INSTALL-RU.txt',stage/'УСТАНОВКА.txt')
        filename='SuperTux_RG35XX_v0.2.1_RU.zip' if variant=='ru' else 'SuperTux_RG35XX_v0.1.1_EN.zip'
        zip_tree(stage,args.out/filename)
zip_tree(ROOT,args.out/'SuperTux_RG35XX_v0.2.1_sources-and-patches.zip','SuperTux_RG35XX')
if args.variant=='both':
    enpath=args.out/'SuperTux_RG35XX_v0.1.1_EN.zip'
    rupath=args.out/'SuperTux_RG35XX_v0.2.1_RU.zip'
    delta=args.out/'SuperTux_RG35XX_v0.2.1_russifier.zip'
    with zipfile.ZipFile(enpath) as ez,zipfile.ZipFile(rupath) as rz:
        baseline={n:ez.read(n) for n in ez.namelist()}
        expected={n:rz.read(n) for n in rz.namelist()}
        assert not set(baseline)-set(expected)
        with zipfile.ZipFile(delta,'w',zipfile.ZIP_DEFLATED,compresslevel=9) as dz:
            for info in rz.infolist():
                data=expected[info.filename]
                if baseline.get(info.filename)!=data:dz.writestr(info,data)
            dz.writestr('РУССИФИКАТОР.txt',(ROOT/'docs/LOCALIZATION-RU.md').read_bytes())
        with zipfile.ZipFile(delta) as dz:
            assert dz.testzip() is None
            combined=dict(baseline)
            for name in dz.namelist():
                if name in expected:combined[name]=dz.read(name)
            assert combined==expected,'Дополнение не воспроизводит полную русскую сборку'
    print(str(delta)+'; наложение на EN = RU: OK')
checks=[]
for item in sorted(args.out.iterdir()):
    if item.is_file() and item.suffix=='.zip':
        checks.append(hashlib.sha256(item.read_bytes()).hexdigest()+'  '+item.name)
(args.out/'SHA256SUMS.txt').write_text('\n'.join(checks)+'\n')
