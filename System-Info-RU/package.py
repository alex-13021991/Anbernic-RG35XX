"""Package the ARM executable, launcher, matching cover, and source."""
import argparse
from pathlib import Path
import zipfile

p=argparse.ArgumentParser(description=__doc__)
p.add_argument("--out",type=Path,required=True)
args=p.parse_args()
base=Path(__file__).resolve().parent
dest=args.out.resolve()
dest.mkdir(parents=True,exist_ok=True)
app="Roms/APPS/SystemInfoRU/"
resource=base/"resources"
install={
    "Roms/APPS/System Info RU.sh":resource/"System Info RU.sh",
    "Roms/APPS/Imgs/System Info RU.png":resource/"System Info RU.png",
    app+"system-info":base/"build/system-info",
    app+"j2k.so":resource/"j2k.so",
    app+"res/FreeSans.ttf":resource/"FreeSans.ttf",
    "Установка.txt":base/"README_RU.txt",
}
for file in sorted((base/"licenses").glob("*.txt")):
    install[app+"licenses/"+file.name]=file
source={}
for folder in ["src","resources","licenses","tests"]:
    for file in sorted((base/folder).glob("*")):
        if file.is_file(): source[file.relative_to(base).as_posix()]=file
for name in ["build-wsl.sh","build.py","package.py","verify.py","BUILD.md","README_RU.txt"]:
    source[name]=base/name
def pack(name, files):
    archive=dest/name
    with zipfile.ZipFile(archive,"w") as z:
        for path,file in files.items():
            data=file.read_bytes()
            executable=path.endswith((".sh",".so","/system-info"))
            if path.endswith(".sh"):
                assert data.startswith(b"#!/bin/sh\n") and b"\r" not in data
            info=zipfile.ZipInfo(path,(2026,11,22,0,0,0))
            info.create_system=3
            info.external_attr=(0o100755 if executable else 0o100644)<<16
            info.compress_type=zipfile.ZIP_DEFLATED
            z.writestr(info,data)
    with zipfile.ZipFile(archive) as z:
        assert z.testzip() is None and set(z.namelist())==set(files)
        for file,original in files.items():assert z.read(file)==original.read_bytes()
    print(archive,archive.stat().st_size)
pack("SystemInfoRU-1.1.1-RG35XX-GarlicOS-1.4.9.zip",install)
pack("SystemInfoRU-1.1.1-source.zip",source)

