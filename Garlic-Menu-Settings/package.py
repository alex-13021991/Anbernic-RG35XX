"""Create both versioned ZIPs with Unix mode bits and stable member timestamps."""
import argparse,hashlib,stat,zipfile
from pathlib import Path

project=Path(__file__).resolve().parent
name="GarlicMenuSettings-1.0"
parser=argparse.ArgumentParser()
parser.add_argument("out",type=Path)
parser.add_argument("--stock",type=Path,default=project/"tests/stock")
a=parser.parse_args(); a.out.mkdir(parents=True,exist_ok=True)
stock_main=(a.stock/"main").read_bytes()
assert hashlib.sha256(stock_main).hexdigest()=="4b3192302227875074f15d6e706abf19721aaebae90c8a9e5b37ecfb3b069157"
def add(z,dest,path,mode=0o644):
    info=zipfile.ZipInfo(str(dest),(2026,11,24,0,0,0))
    info.create_system=3; info.external_attr=(stat.S_IFREG|mode)<<16
    info.compress_type=zipfile.ZIP_DEFLATED
    z.writestr(info,path.read_bytes())
def tree(z,src,dest):
    for f in sorted(src.rglob("*")):
        if f.is_file() and "__pycache__" not in f.parts:
            add(z,str(dest)+"/"+f.relative_to(src).as_posix(),f)
package=a.out/(name+"-GarlicOS-1.4.9.zip")
with zipfile.ZipFile(package,"w") as z:
    base="Roms/APPS/"
    app=base+"GarlicMenuSettings/"
    add(z,base+"Garlic Menu Settings.sh",project/"Garlic Menu Settings.sh",0o755)
    add(z,base+"Imgs/Garlic Menu Settings.png",project/"Garlic Menu Settings.png")
    add(z,app+"garlic-menu-settings",project/"build/garlic-menu-settings",0o755)
    for d in ("assets","patch","licenses"): tree(z,project/d,app+d)
    add(z,"Установка.txt",project/"Установка.txt")
source=a.out/(name+"-source.zip")
with zipfile.ZipFile(source,"w") as z:
    for d in ("src","patch","assets","vendor","licenses"): tree(z,project/d,name+"/"+d)
    for f in sorted((project/"tests").glob("*")):
        if f.is_file(): add(z,name+"/tests/"+f.name,f)
    for f in ("main","settings.json"): add(z,name+"/tests/stock/"+f,a.stock/f)
    for f in ("README.md","Установка.txt","build.sh","package.py","Garlic Menu Settings.sh","Garlic Menu Settings.png"):
        add(z,name+"/"+f,project/f,0o755 if f.endswith(".sh") else 0o644)
for path in (source,package):
    with zipfile.ZipFile(path) as z:
        assert z.testzip() is None
    print(hashlib.sha256(path.read_bytes()).hexdigest(),path)

