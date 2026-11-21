from pathlib import Path
import struct,re,json
from PIL import ImageFont
base=Path(__file__).resolve().parent
prior=base.parents[2]/'2026-11-17/dingux-commander-dingux-commander-rg35xx-garlicos/work'
root=prior/'toolchain/miyoo/arm-miyoo-linux-uclibcgnueabi/sysroot'
class Elf:
    def __init__(self,path):
        self.b=Path(path).read_bytes(); b=self.b
        assert b[:7]==b'\x7fELF\x01\x01\x01'
        self.h=struct.unpack_from('<16sHHIIIIIHHHHHH',b)
        assert self.h[2]==40
        shoff=self.h[6]; shsize=self.h[11];shnum=self.h[12]
        self.sections=[struct.unpack_from('<10I',b,shoff+i*shsize) for i in range(shnum)]
        table=self.sections[self.h[13]];names=b[table[4]:table[4]+table[5]]
        self.byname={self.string(names,s[0]):s for s in self.sections}
    @staticmethod
    def string(b,o):return b[o:b.index(0,o)].decode('utf8','replace')
    def data(self,s):return self.b[s[4]:s[4]+s[5]]
    def needed(self):
        sec=self.byname['.dynamic'];strs=self.data(self.sections[sec[6]])
        return [self.string(strs,v) for t,v in struct.iter_unpack('<II',self.data(sec)) if t==1]
    def symbols(self,undefined):
        sec=self.byname['.dynsym'];strs=self.data(self.sections[sec[6]])
        return {self.string(strs,n) for n,v,size,info,other,sh in struct.iter_unpack('<IIIBBH',self.data(sec))
                if n and (sh==0)==undefined and (not undefined or info>>4!=2)}
    def interp(self):
        for i in range(self.h[10]):
            p=struct.unpack_from('<8I',self.b,self.h[5]+i*self.h[9])
            if p[0]==3:return self.b[p[1]:p[1]+p[4]].rstrip(b'\0').decode()
def verify_elf(p):
    e=Elf(p);assert e.h[7]==0x5000200;assert e.interp()=='/lib/ld-uClibc.so.0'
    exports=set()
    for name in e.needed():
        file=next((d/name for d in [root/'lib',root/'usr/lib'] if (d/name).exists()),None)
        assert file,name
        exports|=Elf(file).symbols(False)
    missing=e.symbols(True)-exports;assert not missing,missing
    print(p.name,'ARM32 EABI5 softfp; uClibc loader; imports resolved:',', '.join(e.needed()))
    return e.needed()
for app in ['usb-manager','screenshot-viewer']:verify_elf(base/'build'/app)
fontpath=prior/'original/DinguxCommander/res/FreeSans.ttf'
b=fontpath.read_bytes();tables={}
for i in range(struct.unpack_from('>H',b,4)[0]):
    tag,checksum,offset,length=struct.unpack_from('>4sIII',b,12+16*i);tables[tag]=(offset,length)
offset,length=tables[b'name'];fmt,count,strings=struct.unpack_from('>HHH',b,offset)
licenses=[]
for i in range(count):
    platform,encoding,lang,name,n,off=struct.unpack_from('>6H',b,offset+6+12*i)
    if name in (0,13,14):
        raw=b[offset+strings+off:offset+strings+off+n]
        t=raw.decode('utf-16-be' if platform in (0,3) else 'latin1')
        if t not in licenses:licenses.append(t)
(base/'font-license.txt').write_text('\n\n'.join(licenses),encoding='utf8')
print('Font license metadata:',licenses)
# Verify Unicode BMP glyph coverage using the font's format-4 cmap.
off,length=tables[b'cmap'];covered=set()
for i in range(struct.unpack_from('>H',b,off+2)[0]):
    plat,enc,rel=struct.unpack_from('>HHI',b,off+4+i*8);sub=off+rel
    if struct.unpack_from('>H',b,sub)[0]!=4:continue
    seg=struct.unpack_from('>H',b,sub+6)[0]//2
    ends=struct.unpack_from('>'+str(seg)+'H',b,sub+14)
    starts=struct.unpack_from('>'+str(seg)+'H',b,sub+16+seg*2)
    deltas=struct.unpack_from('>'+str(seg)+'h',b,sub+16+seg*4)
    ranges=struct.unpack_from('>'+str(seg)+'H',b,sub+16+seg*6)
    for j,(start,end) in enumerate(zip(starts,ends)):
        for cp in range(start,end+1):
            if ranges[j]:
                ix=sub+16+seg*6+j*2+ranges[j]+(cp-start)*2
                glyph=struct.unpack_from('>H',b,ix)[0]
                if glyph:glyph=(glyph+deltas[j])&65535
            else:glyph=(cp+deltas[j])&65535
            if glyph:covered.add(cp)
strings=[]
for path in (base/'src').glob('*.c'):
    src=path.read_text(encoding='utf8')
    strings+=re.findall(r'"([^"\n]*[А-Яа-яЁё↑↓←→][^"\n]*)"',src)
missing={c for s in strings for c in s if ord(c)>127 and ord(c) not in covered}
assert not missing,missing
print('Cyrillic/control glyph coverage: PASS')
small=ImageFont.truetype(str(fontpath),18)
for text in strings:
    if any(token in text for token in ['A Открыть','↑↓ Выбор','↑↓←→ Сдвиг','←→ Листать']):
        print('Footer width',round(small.getlength(text),1),text)
        assert small.getlength(text)<=604,text
print('PASS: ELF compatibility, symbols, font coverage, control hint widths')
