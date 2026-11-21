from pathlib import Path
from PIL import Image,ImageDraw,ImageFont
import zipfile,shutil,hashlib,subprocess,os
base=Path(__file__).resolve().parent
out=base.parent/'outputs';out.mkdir(exist_ok=True)
prior=base.parents[2]/'2026-11-17/dingux-commander-dingux-commander-rg35xx-garlicos/work'
resource=prior/'original/DinguxCommander'
stage=base/'package';stage.mkdir(exist_ok=True)
fontpath=resource/'res/FreeSans.ttf'
def write(p,text):
    p.parent.mkdir(parents=True,exist_ok=True);p.write_text(text,encoding='utf8',newline='\n')
def icon(name,kind):
    im=Image.new('RGBA',(640,480),(0,0,0,0));d=ImageDraw.Draw(im)
    fg=(239,235,247);accent=(191,151,245);muted=(190,181,205)
    d.rounded_rectangle((326,54,624,426),radius=20,fill=(25,20,34,248),outline=(101,82,121),width=2)
    f=lambda size:ImageFont.truetype(str(fontpath),size)
    if kind=='usb':
        d.line([(470,178),(470,92)],fill=accent,width=7)
        d.polygon([(470,82),(458,102),(482,102)],fill=accent)
        d.line([(470,148),(440,132),(440,112)],fill=accent,width=6)
        d.ellipse((431,96,449,114),fill=accent)
        d.line([(470,158),(500,139),(500,119)],fill=accent,width=6)
        d.rectangle((491,104,509,122),fill=accent)
        d.ellipse((458,174,482,198),fill=accent)
        lines=['USB','МЕНЕДЖЕР'];sub=['MTP + ADB','USB-геймпады']
    else:
        d.rounded_rectangle((399,88,551,196),radius=8,outline=accent,width=5)
        for yy in range(2):
            for xx in range(3):
                x=410+xx*45;y=101+yy*42
                d.rectangle((x,y,x+35,y+30),fill=(80+xx*24,68+yy*32,112+xx*17))
        lines=['СКРИНШОТЫ'];sub=['Галерея TF1 + TF2','Просмотр и удаление']
    for i,line in enumerate(lines):d.text((475,226+i*40),line,font=f(30 if len(line)>9 else 34),fill=fg,anchor='mm')
    for i,line in enumerate(sub):d.text((475,332+i*27),line,font=f(21),fill=muted,anchor='mm')
    d.text((475,402),'GarlicOS 1.4.9',font=f(16),fill=accent,anchor='mm')
    p=stage/'APPS/Imgs'/f'{name}.png';p.parent.mkdir(parents=True,exist_ok=True);im.save(p)
for name,folder,binary,kind in [('USB Manager','USBManager','usb-manager','usb'),('Screenshot Viewer','ScreenshotViewer','screenshot-viewer','gallery')]:
    target=stage/'APPS'/folder;(target/'res').mkdir(parents=True,exist_ok=True)
    shutil.copy2(base/'build'/binary,target/binary)
    shutil.copy2(resource/'j2k.so',target/'j2k.so')
    shutil.copy2(fontpath,target/'res/FreeSans.ttf')
    shutil.copy2(base/'src'/f'{name}.sh',stage/'APPS'/f'{name}.sh')
    specific='apply-usb.sh' if kind=='usb' else 'screenshots.cfg'
    shutil.copy2(base/'src'/specific,target/specific)
    icon(name,kind)
shutil.copy2(base/'README_RU.txt',out/'Установка.txt')
licenses=stage/'licenses';licenses.mkdir(exist_ok=True)
shutil.copy2(base/'LICENSE.txt',licenses/'MIT-Applications.txt')
shutil.copy2(base/'font-license.txt',licenses/'FreeSans.txt')
with zipfile.ZipFile('D:/Temp/Toggle_ADB_USB_MTP_RU_GarlicOS_1.4.9.zip') as supplied:
    (licenses/'GPL-3.0.txt').write_bytes(supplied.read('LICENSE_MTP_SERVER.txt'))
notice='''Third-party resources (unchanged)

FreeSans.ttf: GNU FreeFont 2012, GPL-3.0-or-later with font exception.
Copyright and exception: licenses/FreeSans.txt; full GPL: licenses/GPL-3.0.txt.
Upstream font sources: https://ftp.gnu.org/gnu/freefont/freefont-src-20120503.tar.gz
Homepage: https://www.gnu.org/software/freefont/
This is the user's existing font from DinguxCommander/res/FreeSans.ttf.

j2k.so: unchanged input adapter from the user's existing DinguxCommander.
Original attribution/license is retained by that supplied component; it is
not newly authored or covered by the MIT license for these applications.
Provenance: local user DinguxCommander package, RG35XX GarlicOS build.
Its separate source/license was not present in the supplied resource folder.

System dependencies (not redistributed): SDL 1.2, SDL_image, SDL_ttf, uClibc.
MTP server (not redistributed): https://github.com/IonAgorria/rg35xx-mtp-server
The existing MTP installation is required for the MTP checkbox to be applied.

New app-menu PNGs: programmatically drawn for this package; no game art/ROMs.
'''
write(licenses/'THIRD-PARTY.txt',notice)
def add(z,p,name):
    info=zipfile.ZipInfo(name,(2026,11,19,12,0,0));info.create_system=3
    executable=p.suffix in ['.sh','.so'] or p.name in ['usb-manager','screenshot-viewer']
    info.external_attr=(0o100755 if executable else 0o100644)<<16;info.compress_type=zipfile.ZIP_DEFLATED
    z.writestr(info,p.read_bytes())
for name,folder in [('USB Manager','USBManager'),('Screenshot Viewer','ScreenshotViewer')]:
    files=[stage/'APPS'/f'{name}.sh',stage/'APPS/Imgs'/f'{name}.png',*sorted((stage/'APPS'/folder).rglob('*')),*sorted(licenses.glob('*'))]
    files=[p for p in files if p.is_file()]
    filename=folder+'-0.1.0-GarlicOS-1.4.9.zip'
    with zipfile.ZipFile(out/filename,'w') as z:
        for p in files:add(z,p,p.relative_to(stage).as_posix())
        add(z,base/'README_RU.txt','README_RU.txt');add(z,base/'TESTS.txt','TESTS.txt')
        hashes='\n'.join(hashlib.sha256(p.read_bytes()).hexdigest()+'  '+p.relative_to(stage).as_posix() for p in files)+'\n'
        z.writestr('SHA256.txt',hashes)
    print(filename,(out/filename).stat().st_size)
with zipfile.ZipFile(out/'RG35XX-Utilities-0.1.0-source.zip','w') as z:
    for p in sorted((base/'src').glob('*')):add(z,p,'src/'+p.name)
    for p in sorted((base/'tests').glob('*')):
        if p.is_file():add(z,p,'tests/'+p.name)
    for name in ['build.py','verify.py','package.py','run_tests.py','README_RU.txt','BUILD.md','TESTS.txt','LICENSE.txt','font-license.txt']:
        add(z,base/name,name)
    for p in licenses.glob('*'):add(z,p,'licenses/'+p.name)
    add(z,fontpath,'resources/FreeSans.ttf');add(z,resource/'j2k.so','resources/j2k.so')
    for p in (stage/'APPS/Imgs').glob('*.png'):add(z,p,'resources/Imgs/'+p.name)
# Verify final archives, directory layout, LF, hashes, and PNG alpha/size.
for archive in out.glob('*0.1.0*.zip'):
    with zipfile.ZipFile(archive) as z:
        assert z.testzip() is None
        for name in z.namelist():
            assert not name.startswith('/') and '..' not in Path(name).parts
            if name.endswith('.sh'):assert b'\r' not in z.read(name)
        if 'SHA256.txt' in z.namelist():
            for line in z.read('SHA256.txt').decode().splitlines():
                digest,name=line.split('  ',1);assert hashlib.sha256(z.read(name)).hexdigest()==digest
for p in (stage/'APPS/Imgs').glob('*.png'):
    im=Image.open(p);assert im.size==(640,480) and im.mode=='RGBA'
    assert im.getchannel('A').crop((0,0,320,480)).getbbox() is None
print('PASS: final ZIP CRC, contents, hashes, launchers LF, independent app payloads, transparent 640x480 previews')
