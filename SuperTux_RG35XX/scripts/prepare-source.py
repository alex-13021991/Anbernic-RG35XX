#!/usr/bin/env python3
"""Восстановить проверенное дерево en/ru из исходной базы и отдельных патчей."""
import argparse,hashlib,json,os,shutil,subprocess,tarfile
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def verify(folder,variant):
    manifest=json.loads((ROOT/'sources'/('prepared-'+variant+'-sha256.json')).read_text())
    for name,digest in manifest.items():
        item=folder/name
        if not item.is_file() or hashlib.sha256(item.read_bytes()).hexdigest()!=digest:
            raise SystemExit('Дерево не соответствует манифесту: '+str(item))
    print('Проверено файлов ('+variant+'): '+str(len(manifest)))
def prepare(variant,work):
    work=work.expanduser().resolve()
    target=work/variant/'source'
    if target.exists():
        verify(target,variant)
        return target
    target.parent.mkdir(parents=True,exist_ok=True)
    upstream=ROOT/'sources/upstream'
    shutil.copytree(upstream,target)
    # This snapshot restores original 640x480 assets and their metadata.
    with tarfile.open(ROOT/'sources/high-resolution-data-b1619ec.tar.gz') as archive:
        archive.extractall(target,filter='data')
    # Preserve Miyoo branch's sound/music fixes and OGG effects.
    for folder in ['sounds','music']:
        shutil.copytree(upstream/'data'/folder,target/'data'/folder,dirs_exist_ok=True)
    for wav in (target/'data/sounds').glob('*.wav'):
        if wav.with_suffix('.ogg').exists():wav.unlink()
    patches=[ROOT/'patches/rg35xx/0001-rg35xx-platform.patch']
    if variant=='ru':patches.append(ROOT/'patches/russian/0001-russian-text.patch')
    for patch in patches:
        subprocess.run(['patch','--binary','--batch','--forward','--fuzz=0','-p1','-i',str(patch)],cwd=target,check=True)
    if variant=='ru':
        shutil.copytree(ROOT/'patches/russian/fonts',target/'data/fonts')
        shutil.copytree(ROOT/'patches/russian/font-source',target/'tools/font-source')
    verify(target,variant)
    print('Исходники: '+str(target))
    return target
if __name__=='__main__':
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('variant',choices=['en','ru'],nargs='?',default='ru')
    ap.add_argument('--work',type=Path,default=Path(os.environ.get('SUPERTUX_WORK',str(ROOT/'.work'))))
    a=ap.parse_args()
    prepare(a.variant,a.work)
