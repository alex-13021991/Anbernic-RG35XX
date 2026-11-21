"""Create a disposable two-card library in the given empty directory."""
from pathlib import Path
import ctypes
import os
import sys
root=Path(sys.argv[1]).resolve()
root.mkdir(exist_ok=True,parents=True)
if any(root.iterdir()):
    raise SystemExit("Specify an empty fixture directory")
def put(path, data):
    p=root/path
    p.parent.mkdir(exist_ok=True,parents=True)
    p.write_bytes(data.encode() if isinstance(data,str) else data)
    return p

def sparse(path,size):
    p=put(path,b"")
    with p.open("r+b") as f:
        if os.name=="nt":
            import msvcrt
            done=ctypes.c_ulong()
            ok=ctypes.windll.kernel32.DeviceIoControl(
                ctypes.c_void_p(msvcrt.get_osfhandle(f.fileno())),0x900C4,
                None,0,None,0,ctypes.byref(done),None)
            if not ok: raise OSError("Sparse NTFS fixture unavailable")
        f.truncate(size)

put("TF1/Roms/FC/Игра.nes",b"N"*7)
put("TF1/Roms/FC/Imgs/Игра.png",b"cover")
put("TF1/Roms/FC/.DS_Store",b"hidden")
put("TF1/Roms/PS/Set.m3u","\ufeff#EXTM3U\r\nDiscs\\Disc 1.cue\r\nDiscs/../Discs/Disc 2.chd\r\n")
put("TF1/Roms/PS/Discs/Disc 1.cue",'FILE "Track 01.bin" BINARY\nFILE "Track 02.bin" BINARY\n')
sparse("TF1/Roms/PS/Discs/Track 01.bin",2200000000)
sparse("TF1/Roms/PS/Discs/Track 02.bin",2200000000)
put("TF1/Roms/PS/Discs/Disc 2.chd",b"2"*11)
put("TF1/Roms/PS/Single.pbp",b"single")
put("TF1/Roms/PS/.hidden/Extra.chd",b"hidden disc")
put("TF1/Roms/PS/Game.ccd",b"ccd")
put("TF1/Roms/PS/Game.img",b"img")
put("TF1/Roms/APPS/Tool.sh",b"launcher")
put("TF1/Roms/APPS/Tool/helper.sh",b"internal")
put("TF1/Roms/APPS/Imgs/Tool.png",b"art")
put("TF1/Roms/PORTS/Doom.sh",b"run")
put("TF1/Roms/PORTS/Doom/start.sh",b"helper")
put("TF1/Saves/CurrentProfile/saves/Nestopia/game.srm",b"sram")
put("TF1/Saves/CurrentProfile/states/Nestopia/game.state.auto",b"state")
put("TF1/Saves/CurrentProfile/states/Nestopia/game.state.auto.png",b"thumb")
put("TF1/Saves/Player2/saves/game.sav",b"save2")
put("TF1/Screenshots/FC/image.png",b"picture")
put("TF1/Screenshots/images/one.bmp",b"1")
put("TF1/CFW/retroarch/.retroarch/screenshots/image.bmp",b"shot2")
put("TF1/BIOS/sub/scph.bin",b"bios")
put("TF1/BIOS/readme.txt",b"readme")
put("TF2/Roms/FC/Same.nes",b"other")
put("TF2/Roms/PICO/Celeste.p8.png",b"pico")
put("TF2/Roms/PICO/Imgs/Celeste.png",b"cover")
put("TF2/Saves/saves/example.sav",b"save")
put("TF2/BIOS/neogeo.zip",b"bios2")

ps=root/"TF1/Roms/PS"
expected=sum(p.stat().st_size for p in ps.rglob("*") if p.is_file())
(root/"expected").write_text(str(expected))
(root/"empty").mkdir()

