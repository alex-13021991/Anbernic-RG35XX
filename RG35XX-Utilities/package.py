from pathlib import Path
import zipfile,shutil,hashlib,struct
base=Path(__file__).resolve().parent
out=base.parent/'outputs';out.mkdir(exist_ok=True)
prior=base.parents[1]/'dingux-commander-dingux-commander-rg35xx-garlicos/work'
resource=prior/'original/DinguxCommander'
stage=base/'package';stage.mkdir(exist_ok=True)
fontpath=resource/'res/FreeSans.ttf'
def write(p,text):
    p.parent.mkdir(parents=True,exist_ok=True);p.write_text(text,encoding='utf8',newline='\n')
for name,folder,binary in [('USB Manager','USBManager','usb-manager'),('Screenshot Viewer','ScreenshotViewer','screenshot-viewer')]:
    target=stage/'APPS'/folder;(target/'res').mkdir(parents=True,exist_ok=True)
    shutil.copy2(base/'build'/binary,target/binary)
    shutil.copy2(resource/'j2k.so',target/'j2k.so')
    shutil.copy2(fontpath,target/'res/FreeSans.ttf')
    shutil.copy2(base/'src'/f'{name}.sh',stage/'APPS'/f'{name}.sh')
    specific='apply-usb.sh' if folder=='USBManager' else 'screenshots.cfg'
    shutil.copy2(base/'src'/specific,target/specific)
    if not (stage/'APPS/Imgs'/f'{name}.png').is_file():
        raise SystemExit(f'Missing existing app icon: {name}.png')
shutil.copy2(base/'README_RU.txt',out/'Установка.txt')
licenses=stage/'licenses';licenses.mkdir(exist_ok=True)
shutil.copy2(base/'LICENSE.txt',licenses/'MIT-Applications.txt')
shutil.copy2(base/'font-license.txt',licenses/'FreeSans.txt')
if not (licenses/'GPL-3.0.txt').is_file():
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
    filename=folder+'-0.1.2-GarlicOS-1.4.9.zip'
    with zipfile.ZipFile(out/filename,'w') as z:
        for p in files:add(z,p,p.relative_to(stage).as_posix())
        add(z,base/'README_RU.txt','README_RU.txt');add(z,base/'TESTS.txt','TESTS.txt')
        hashes='\n'.join(hashlib.sha256(p.read_bytes()).hexdigest()+'  '+p.relative_to(stage).as_posix() for p in files)+'\n'
        z.writestr('SHA256.txt',hashes)
    print(filename,(out/filename).stat().st_size)
with zipfile.ZipFile(out/'RG35XX-Utilities-0.1.2-source.zip','w') as z:
    for p in sorted((base/'src').glob('*')):add(z,p,'src/'+p.name)
    for p in sorted((base/'tests').glob('*')):
        if p.is_file():add(z,p,'tests/'+p.name)
    for name in ['build.py','verify.py','package.py','run_tests.py','README_RU.txt','BUILD.md','TESTS.txt','LICENSE.txt','font-license.txt']:
        add(z,base/name,name)
    for p in licenses.glob('*'):add(z,p,'licenses/'+p.name)
    add(z,fontpath,'resources/FreeSans.ttf');add(z,resource/'j2k.so','resources/j2k.so')
    for p in (stage/'APPS/Imgs').glob('*.png'):add(z,p,'resources/Imgs/'+p.name)
# Verify final archives, directory layout, LF, hashes, and PNG headers.
for archive in out.glob('*0.1.2*.zip'):
    with zipfile.ZipFile(archive) as z:
        assert z.testzip() is None
        for name in z.namelist():
            assert not name.startswith('/') and '..' not in Path(name).parts
            if name.endswith('.sh'):assert b'\r' not in z.read(name)
        if 'SHA256.txt' in z.namelist():
            for line in z.read('SHA256.txt').decode().splitlines():
                digest,name=line.split('  ',1);assert hashlib.sha256(z.read(name)).hexdigest()==digest
for p in (stage/'APPS/Imgs').glob('*.png'):
    raw=p.read_bytes()[:29]
    assert raw[:8]==b'\x89PNG\r\n\x1a\n'
    width,height,depth,color,compression,filtering,interlace=struct.unpack('>IIBBBBB',raw[16:29])
    assert (width,height,color)==(640,480,6)
print('PASS: final ZIP CRC, contents, hashes, launchers LF, independent app payloads, 640x480 RGBA icons')
