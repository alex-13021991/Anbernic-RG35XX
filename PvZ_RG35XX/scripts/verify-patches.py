#!/usr/bin/env python3
from pathlib import Path
import tempfile,tarfile,subprocess,hashlib
r=Path(__file__).resolve().parents[1]
def tree(p):return {str(f.relative_to(p)):hashlib.sha256(f.read_bytes()).hexdigest() for f in p.rglob('*') if f.is_file() and '__pycache__' not in f.parts}
with tempfile.TemporaryDirectory(prefix='pvz-patch-') as tmp:
 p=Path(tmp)
 with tarfile.open(r/'vendor/pvz-miyoo-2b3c06c-source.tar.gz') as t:t.extractall(p,filter='data')
 p=p/'pvz'
 for line in (r/'patches/series').read_text().splitlines():
  if line and not line.startswith('#'):
   subprocess.run(['patch','-p1','--batch','-i',str(r/'patches'/line)],cwd=p,check=True,stdout=subprocess.DEVNULL)
 assert tree(p)==tree(r/'sources/pvz'), 'Patched source differs from sources/pvz'
 print('PASS: upstream snapshot + patches equals full release source')
