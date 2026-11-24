"""Execute the supplied v0.5 ARM row renderer. Observe SDL_UpperBlit arguments
and compare them with layout() compiled from the actual preview source."""
import ctypes as C
from hashlib import sha256
from pathlib import Path
from struct import pack, unpack
import subprocess, tempfile
from unicorn import Uc, UC_ARCH_ARM, UC_MODE_ARM, UC_HOOK_CODE
from unicorn.arm_const import UC_ARM_REG_FP, UC_ARM_REG_SP, UC_ARM_REG_R0, UC_ARM_REG_R1, UC_ARM_REG_R3, UC_ARM_REG_PC, UC_ARM_REG_LR

project=Path(__file__).resolve().parents[1]
class Settings(C.Structure):
    _fields_=[(n,C.c_int) for n in "align margin width scroll speed delay pause".split()]
class Row(C.Structure):
    _fields_=[(n,C.c_int) for n in ("x","width","offset")]
binary=(project/"patch/main").read_bytes()
assert sha256(binary).hexdigest()=="b20853ebb449a85e7f27562d5d2f945a6bbcb3c3127dfa762ffc8e785e6eb988"

def compare(layout,s,cover,text,selected,ticks):
    emu=Uc(UC_ARCH_ARM,UC_MODE_ARM)
    emu.mem_map(0x10000,0x50000)
    emu.mem_write(0x10000,binary[:0x14eb3])
    fp,sp,surface,screen,names=0x59000,0x5c000,0x51000,0x52000,0x53000
    def words(addr,*values):
        emu.mem_write(addr,pack("<"+"I"*len(values),*values))
    words(fp-48,2); words(fp-516,s.align); words(fp-140,int(cover))
    words(fp-60,3); words(fp-128,8); words(fp-280,3 if selected else 1)
    words(fp-512,screen); words(fp-4916,s.margin); words(fp-4912,names)
    for i in range(8):
        words(fp-6380+i*4,surface)
        words(names+i*4,0x54100+i*16)
    words(surface+8,text,32); words(screen+8,640,480)
    words(0x3d30c,s.width,s.scroll,s.speed,s.delay,s.pause)
    observed=[]
    def plt(u,addr,size,data):
        if addr==0x111dc: u.reg_write(UC_ARM_REG_R0,now)
        elif addr==0x112cc:
            dest=u.reg_read(UC_ARM_REG_R3); src=u.reg_read(UC_ARM_REG_R1)
            x=unpack("<h",u.mem_read(dest,2))[0]
            offset,_,width,_=unpack("<hhHH",u.mem_read(src,8)) if src else (0,0,text,32)
            observed.append((x,width,offset))
        else: return
        u.reg_write(UC_ARM_REG_PC,u.reg_read(UC_ARM_REG_LR))
    emu.hook_add(UC_HOOK_CODE,plt,begin=0x111dc,end=0x112cc)
    for now in ticks:
        observed.clear()
        emu.reg_write(UC_ARM_REG_FP,fp); emu.reg_write(UC_ARM_REG_SP,sp)
        emu.emu_start(0x14588,0x14908,count=2000)
        r=layout(s,cover,text,selected,now)
        expect=[(r.x,r.width,r.offset)] if r.width else []
        assert observed==expect,(s.align,cover,s.width,text,now,observed,expect)

def main():
    with tempfile.TemporaryDirectory() as tmp:
        so=Path(tmp)/"layout.so"
        subprocess.run(["cc","-shared","-fPIC","-Os","-Isrc","-Ivendor/yyjson","-Ivendor/sha256",
            "src/config.c","src/files.c","vendor/yyjson/yyjson.c","vendor/sha256/sha256.c","-o",so],cwd=project,check=True)
        layout=C.CDLL(str(so)).layout
        layout.argtypes=[Settings,C.c_int,C.c_int,C.c_int,C.c_uint]; layout.restype=Row
        count=0
        for align in (0,1,2):
            for cover in (0,1):
                for width in (0,313,640):
                    for selected in (0,1):
                        for scroll in (0,1):
                            for text in (100,400,787):
                                s=Settings(align,16,width,scroll,40,1000,1000)
                                # End of the first cycle at a sample hit, as in the live renderer.
                                limit=min(width,624)
                                transit=(max(0,text-limit)*1000+39)//40
                                ticks=sorted(set([0,500,1000,1025,1500]+list(range(2000,2000+transit,1000))+
                                      [1000+transit,1999+transit,2000+transit]))
                                compare(layout,s,cover,text,selected,ticks)
                                count+=1
        print(f"{count} combinations matched actual v0.5 ARM blit coordinates and source rectangles")
if __name__=="__main__": main()

