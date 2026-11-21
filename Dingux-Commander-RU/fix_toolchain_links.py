from pathlib import Path
import re
root=(Path(__file__).parent/'toolchain/miyoo').resolve()
links={}
for p in root.rglob('*'):
    if not p.is_file() or not 0 < p.stat().st_size < 512: continue
    data=p.read_bytes()
    if not re.fullmatch(rb'[A-Za-z0-9_./+\-]+',data): continue
    dest=(p.parent/data.decode()).resolve()
    if dest.is_relative_to(root) and dest.is_file(): links[p]=dest
def resolve(p, seen=None):
    seen=set() if seen is None else seen
    assert p not in seen
    seen.add(p)
    return resolve(links[p],seen) if p in links else p
for p in links:
    p.write_bytes(resolve(p).read_bytes())
print('Materialized',len(links),'toolchain symlinks')
