"""Exercise the ARM core and the packaged editor on two temporary cards."""
import argparse
import os
import shutil
import struct
import subprocess
import tempfile
import zipfile
import zlib
from pathlib import Path

PROJECT = Path(__file__).resolve().parents[1]

def chunk(name, data):
    return struct.pack(">I", len(data)) + name + data + struct.pack(">I", zlib.crc32(name + data))

def fixture(palette):
    w, h = 90, 76
    rows = []
    colors = [(0,0,0,0), (24,140,195,128), (141,240,160,255)]
    for y in range(h):
        row = bytearray([0])
        for x in range(w):
            inside = 10 <= x < 60 and 11 <= y < 44
            index = (1 if x < 35 else 2) if inside else 0
            row.extend((index,) if palette else colors[index])
        rows.append(row)
    ihdr = struct.pack(">IIBBBBB", w, h, 8, 3 if palette else 6, 0, 0, 0)
    body = chunk(b"IHDR", ihdr)
    if palette:
        body += chunk(b"PLTE", b"".join(bytes(c[:3]) for c in colors)) + chunk(b"tRNS", bytes(c[3] for c in colors))
    # Ancillary metadata makes byte-for-byte restore stronger than pixel equality.
    return b"\x89PNG\r\n\x1a\n" + body + chunk(b"tEXt", b"origin\0test") + chunk(b"IDAT", zlib.compress(b"".join(rows))) + chunk(b"IEND", b"")

def read_result(path):
    data = path.read_bytes()
    offset, compressed = 8, bytearray()
    while offset < len(data):
        size, name = struct.unpack_from(">I4s",data,offset)
        body = data[offset+8:offset+8+size]
        if name == b"IHDR":
            assert struct.unpack(">IIBBBBB",body) == (640,480,8,6,0,0,0)
        if name == b"IDAT":
            compressed.extend(body)
        offset += size+12
    raw = zlib.decompress(compressed)
    stride, rows = 640*4, []
    previous = bytearray(stride)
    assert len(raw) == (stride+1)*480
    for y in range(480):
        at = (stride+1)*y
        kind = raw[at]
        row = bytearray(raw[at+1:at+1+stride])
        if kind:
            for i, value in enumerate(row):
                a = row[i-4] if i>=4 else 0
                b = previous[i]
                c = previous[i-4] if i>=4 else 0
                if kind == 1: delta = a
                elif kind == 2: delta = b
                elif kind == 3: delta = (a+b)//2
                else:
                    assert kind == 4
                    p = a+b-c
                    delta = min((a,b,c),key=lambda n: abs(p-n))
                row[i] = (value+delta)&255
        rows.append(row)
        previous = row
    return rows

def verify_pixels(path, x, y, w, h, split):
    rows = read_result(path)
    for iy, row in enumerate(rows):
        expected = bytearray(640*4)
        if y <= iy < y+h:
            expected[x*4:(x+w)*4] = bytes([24,140,195,128])*split + bytes([141,240,160,255])*(w-split)
        assert row == expected, ("unexpected scanline",iy,x,y,w,h)
    
def run(args,tmp):
    sysroot = args.sdk / "arm-miyoo-linux-uclibcgnueabi/sysroot"
    rt = tmp / "runtime"
    for d in ("opt","usr"): (rt/d).mkdir(parents=True,exist_ok=True)
    (rt/"opt/miyoo").symlink_to(args.sdk)
    (rt/"usr/lib").symlink_to(sysroot/"usr/lib")
    (rt/"lib").symlink_to(sysroot/"lib")
    qemu = [str(args.qemu),"-L",str(rt)]
    subprocess.run(qemu+[str(PROJECT/"build/check")],check=True,timeout=90)
    cards = [tmp/"TF1",tmp/"TF2"]
    files = []
    originals = []
    for i, card in enumerate(cards):
        folder=card/"Roms/PORTS/Imgs"
        folder.mkdir(parents=True)
        path=folder/"Обложка.PNG"
        originals.append(fixture(palette=i==0))
        path.write_bytes(originals[-1])
        files.append(path)
    install=tmp/"install"
    with zipfile.ZipFile(args.archive) as z:
        assert z.testzip() is None
        for info in z.infolist():
            f=Path(z.extract(info,install))
            f.chmod((info.external_attr>>16)&0o777)
    app=install/"Roms/APPS/PreviewEditor"
    exe=app/"preview-editor"
    launcher=install/"Roms/APPS/Preview Editor.sh"
    assert os.access(exe,os.X_OK) and os.access(launcher,os.X_OK)
    subprocess.run(["sh","-n",str(launcher)],check=True)
    frame=tmp/"frame.bmp"
    def keys(seq):
        env=os.environ | {"SDL_VIDEODRIVER":"dummy","SDL_AUDIODRIVER":"dummy","PREVIEW_KEYS":seq,
            "PREVIEW_TF1":str(cards[0]),"PREVIEW_TF2":str(cards[1]),"PREVIEW_FRAME":str(frame)}
        subprocess.run(qemu+["-E","LD_LIBRARY_PATH="+str(app/"lib"),"-E",
            "LD_PRELOAD="+str(PROJECT/"build/pad.so")+":"+str(app/"assets/j2k.so"),str(exe)],
            cwd=app,env=env,check=True,timeout=45)
    open2="saaaaa"
    tools="a"
    # Change 16 to 19, confirm upper right, undo and redo it, then save.
    keys(open2+tools+"ssssa"+"ddd"+"a"+"wwww"+"a"+"dw"+"axy"+tools+"sssssada"+"a")
    p=files[1]; bak=p.parent/".preview-originals"/p.name
    assert bak.read_bytes()==originals[1]
    assert files[0].read_bytes()==originals[0]
    assert (app/"preview.cfg").read_text().splitlines()[0]=="margin=19"
    verify_pixels(p,571,62,50,33,25)
    # Reopen, crop left 3 and top 2, undo/redo, move one pixel on each axis.
    keys(open2+tools+"sa"+"ddd"+"sdd"+"axy"+"ds"+tools+"ssssada"+"a")
    verify_pixels(p,575,65,47,31,22)
    assert bak.read_bytes()==originals[1]
    # Preview, cancel a new alignment, cancel the discard prompt, lose no disk changes.
    saved=p.read_bytes()
    keys(open2+tools+"ssa"+"*"+"b"+"wwa"+"w"+"b"+"b"+"d"+"b"+"a")
    assert p.read_bytes()==saved and frame.read_bytes()[:2]==b"BM"
    if args.frame:
        shutil.copyfile(frame,args.frame)
    # Restore a PNG even when its current contents no longer decode.
    p.write_bytes(b"damaged")
    keys(open2[:-1]+"x"+"da"+"a")
    assert p.read_bytes()==originals[1] and bak.read_bytes()==originals[1]
    # Palette+tRNS on TF1; settings survive a restart and bottom alignment uses 49 px.
    keys("aaaaa"+tools+"a"+"qs"+"a"+tools+"sssssada"+"a")
    verify_pixels(files[0],19,379,50,33,25)
    keys("aaaa"+"x"+"da"+"a")
    assert files[0].read_bytes()==originals[0]
    # Reset the margin in the UI and override the top row from auto 43 to 44.
    keys("aaaaa"+tools+"ssssa"+"xsd"+"a"+"wwwwa"+"wq"+"a"+tools+"sssssada"+"a")
    verify_pixels(files[0],16,60,50,33,25)
    assert (app/"preview.cfg").read_text().splitlines()==["margin=16","top=44","bottom=-1"]
    print("PASS packaged release on TF1 and TF2: edit, align, crop, undo/redo, preview, persisted margin, RGBA pixels, exact restore")

if __name__=="__main__":
    parser=argparse.ArgumentParser()
    parser.add_argument("archive",type=lambda p:Path(p).resolve())
    parser.add_argument("--sdk",type=lambda p:Path(p).resolve(),default=Path.home()/"rg35xx-sdk/miyoo-gcc-7.3.0")
    parser.add_argument("--qemu",type=Path,default=shutil.which("qemu-arm-static") or shutil.which("qemu-arm"))
    parser.add_argument("--frame",type=Path)
    args=parser.parse_args()
    if not args.qemu: parser.error("pass --qemu /path/to/qemu-arm-static")
    with tempfile.TemporaryDirectory(prefix="preview-editor-") as temp: run(args,Path(temp))
