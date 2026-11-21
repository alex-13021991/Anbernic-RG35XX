"""Check the ELF linked image and all strong imports in the target SDK."""
import argparse
import struct
from pathlib import Path

class Elf:
    def __init__(self, path):
        self.b = Path(path).read_bytes()
        self.h = struct.unpack_from("<16sHHIIIIIHHHHHH",self.b)
        assert self.b[:7] == b"\x7fELF\x01\x01\x01" and self.h[2] == 40, path
        self.sections = [struct.unpack_from("<10I",self.b,self.h[6]+i*self.h[11])
                         for i in range(self.h[12])]
        names = self.data(self.sections[self.h[13]])
        self.byname = {self.string(names,s[0]):s for s in self.sections}
    @staticmethod
    def string(buf, off):
        return buf[off:buf.index(0,off)].decode()
    def data(self, sec):
        return self.b[sec[4]:sec[4]+sec[5]]
    def deps(self):
        dyn = self.byname[".dynamic"]
        names = self.data(self.sections[dyn[6]])
        return [self.string(names,n) for tag,n in struct.iter_unpack("<II",self.data(dyn)) if tag==1]
    def symbols(self, imports):
        sec = self.byname[".dynsym"]
        names = self.data(self.sections[sec[6]])
        return {self.string(names,n) for n,_,_,info,_,sh in struct.iter_unpack("<IIIBBH",self.data(sec))
                if n and (sh==0)==imports and (not imports or info>>4!=2)}

if __name__=="__main__":
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--sdk",type=Path,required=True)
    args=parser.parse_args()
    base=Path(__file__).resolve().parent
    root=args.sdk/"arm-miyoo-linux-uclibcgnueabi/sysroot"
    app=Elf(base/"build/system-info"); ref=Elf(base/"resources/j2k.so")
    assert app.h[1]==2 and app.h[7]==0x5000200
    assert app.data(app.byname[".interp"])==b"/lib/ld-uClibc.so.0\x00"
    for obj in [app,ref]:
        names=set(); pending=list(obj.deps()); seen=set()
        while pending:
            n=pending.pop()
            if n in seen: continue
            seen.add(n)
            p=next(p for p in [root/"lib"/n,root/"usr/lib"/n] if p.is_file())
            dep=Elf(p);names.update(dep.symbols(False));pending.extend(dep.deps())
        missing=obj.symbols(True)-names-app.symbols(False)
        assert not missing,missing
        print("ARM ELF imports OK:",", ".join(obj.deps()))
    print("PASS: executable ARM32 EABI5, soft-float calling convention, uClibc loader")

