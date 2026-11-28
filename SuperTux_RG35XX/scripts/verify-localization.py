#!/usr/bin/env python3
import argparse,os,re,json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
ap=argparse.ArgumentParser()
ap.add_argument('--work',type=Path,default=Path(os.environ.get('SUPERTUX_WORK',str(ROOT/'.work'))))
a=ap.parse_args()
en=a.work/'en/source';ru=a.work/'ru/source'
result={}
for name in ['images','sounds','music']:
    count=0
    for original in (en/'data'/name).rglob('*'):
        if original.is_file():
            translated=ru/'data'/original.relative_to(en/'data')
            assert translated.read_bytes()==original.read_bytes(),str(translated)
            count+=1
    result[name]=count
pattern=re.compile(r'\((name|title|description)\s+"[^"]*"')
count=0
for original in (en/'data/levels').rglob('*'):
    if not original.is_file():continue
    translated=ru/'data/levels'/original.relative_to(en/'data/levels')
    try:left=original.read_text();right=translated.read_text()
    except UnicodeError:
        assert original.read_bytes()==translated.read_bytes()
        continue
    assert pattern.sub('(metadata ""',left)==pattern.sub('(metadata ""',right),str(translated)
    if left!=right:count+=1
result['localized_level_metadata_files']=count
print(json.dumps(result,ensure_ascii=False,indent=2))
