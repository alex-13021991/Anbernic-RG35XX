"""Run the unmodified installer ZIP executable with a synthetic pad and two virtual cards."""
import argparse,json,os,subprocess,tempfile,zipfile
from pathlib import Path
parser=argparse.ArgumentParser()
for key in ("archive","stock","sdk","qemu","pad"): parser.add_argument(key,type=Path)
parser.add_argument("--upgrade-from",type=Path,help="install the patch with an earlier release ZIP first")
a=parser.parse_args()
sdk=a.sdk.resolve()
sysroot=sdk/"arm-miyoo-linux-uclibcgnueabi/sysroot"
assert not Path("/mnt/mmc").exists(), "Run on a test host without a system TF1 mount"
with tempfile.TemporaryDirectory(prefix="garlic-zip-") as tmp:
    tmp=Path(tmp)
    root=tmp/"runtime"; (root/"usr").mkdir(parents=True); (root/"opt").mkdir()
    (root/"usr/lib").symlink_to(sysroot/"usr/lib"); (root/"lib").symlink_to(sysroot/"lib")
    (root/"opt/miyoo").symlink_to(sdk)
    cfw=root/"mnt/mmc/CFW"; (cfw/"skin").mkdir(parents=True)
    original=(a.stock/"main").read_bytes(); settings=(a.stock/"settings.json").read_bytes()
    m=cfw/"main"; j=cfw/"skin/settings.json"
    m.write_bytes(original); m.chmod(0o755); j.write_bytes(settings)
    card=tmp/"TF2"; app=card/"Roms/APPS/GarlicMenuSettings"
    exe=app/"garlic-menu-settings"
    def unpack(archive):
        with zipfile.ZipFile(archive) as z:
            assert z.testzip() is None
            z.extractall(card)
        exe.chmod(0o755)
    unpack(a.upgrade_from or a.archive)
    def run(keys):
        result=subprocess.run([str(a.qemu),"-L",str(root),"-E","LD_PRELOAD="+str(a.pad.resolve()),str(exe)],
            cwd=app,env=os.environ|{"GMS_PAD":keys,"SDL_VIDEODRIVER":"dummy","SDL_AUDIODRIVER":"dummy"},
            timeout=30,capture_output=True,text=True)
        assert result.returncode==0,(result.returncode,result.stderr)
    run("b"); assert m.read_bytes()==original and j.read_bytes()==settings
    run("daab"); assert m.read_bytes()==(app/"patch/main").read_bytes() and json.loads(j.read_bytes())["text-max-width"]==313
    assert (cfw/"main.bak").read_bytes()==original and (cfw/"skin/settings.json.bak").read_bytes()==settings
    if a.upgrade_from:
        unpack(a.archive)
    run("sllyba"); assert json.loads(j.read_bytes())["text-margin"]==36
    run("ssqb"); assert json.loads(j.read_bytes())["text-max-width"]==313
    run("ssssssadaa"); assert m.read_bytes()==original and j.read_bytes()==settings
    assert set(x.name for x in cfw.iterdir())=={"main","skin"}
    assert set(x.name for x in (cfw/"skin").iterdir())=={"settings.json"}
    run("a"); assert m.read_bytes()==original and j.read_bytes()==settings
    print("PASS: release ZIP ELF under QEMU, production TF1 path, launch from TF2, real SDL loop, install/save/cancel/restore/first launch")

