"""Destructive test cases are confined to freshly created TemporaryDirectory trees."""
import argparse, fcntl, json, os, shutil, subprocess, tempfile
from pathlib import Path
from hashlib import sha256

project=Path(__file__).resolve().parents[1]
parser=argparse.ArgumentParser()
parser.add_argument("stock",type=Path)
parser.add_argument("--qemu")
parser.add_argument("--sysroot")
args=parser.parse_args()
original=(args.stock/"main").read_bytes()
orig_json=(args.stock/"settings.json").read_bytes()
assert sha256(original).hexdigest()=="4b3192302227875074f15d6e706abf19721aaebae90c8a9e5b37ecfb3b069157"
patched=(project/"patch/main").read_bytes()
custom=json.loads(orig_json)
custom.update({"color-active":"#baf4ff","custom":{"huge":10**100,"label":"Пример"},"text-scroll-speed":83})
start=json.dumps(custom,ensure_ascii=False).encode()
add=json.loads((project/"patch/settings-additions.json").read_bytes())
installed=add|custom
saved=installed|{"text-alignment":"right","text-margin":26,"text-max-width":290,"text-scroll":False}
with tempfile.TemporaryDirectory(prefix="gms-test-") as tmp:
    tmp=Path(tmp)
    cfw=tmp/"TF1"/"CFW"
    app=tmp/"TF2"/"Roms"/"APPS"/"GarlicMenuSettings"
    (app/"patch").mkdir(parents=True)
    for p in (project/"patch").iterdir(): shutil.copy2(p,app/"patch")
    if args.qemu:
        cmd=[args.qemu,"-L",args.sysroot,str(project/"build/core-test-arm")]
    else:
        exe=tmp/"core"
        subprocess.run(["cc","-DGMS_TEST","-Os","-Isrc","-Ivendor/yyjson","-Ivendor/sha256",
            "tests/cli.c","src/files.c","src/transaction.c","src/config.c",
            "vendor/sha256/sha256.c","vendor/yyjson/yyjson.c","-o",exe],cwd=project,check=True)
        cmd=[str(exe)]
    def run(op,expected=0,**env):
        r=subprocess.run([*cmd,str(cfw),op],env=os.environ|env,cwd=app,capture_output=True,text=True,timeout=12)
        if expected is not None: assert r.returncode==expected,(op,r.returncode,r.stderr)
        return r
    def reset():
        if cfw.exists(): shutil.rmtree(cfw)
        (cfw/"skin").mkdir(parents=True)
        (cfw/"main").write_bytes(original); (cfw/"main").chmod(0o755)
        (cfw/"skin/settings.json").write_bytes(start)
    def snapshot():
        return {str(p.relative_to(cfw)):p.read_bytes() for p in cfw.rglob("*") if p.is_file()}
    def verify(kind):
        image=snapshot()
        expected={"main","skin/settings.json"}
        if kind=="stock":
            assert image["main"]==original and image["skin/settings.json"]==start
        else:
            expected.update(["main.bak","skin/settings.json.bak",".garlic-menu.original"])
            assert image["main"]==patched and image["main.bak"]==original and image["skin/settings.json.bak"]==start
            assert json.loads(image["skin/settings.json"])==(installed if kind=="install" else saved)
            assert image[".garlic-menu.original"].decode()==sha256(start).hexdigest()
        assert set(image)==expected and not (cfw/".garlic-menu.work").exists(),list(image)
        assert os.access(cfw/"main",os.X_OK)
    reset(); run("install"); verify("install"); run("stale",1); verify("install"); run("save"); verify("save")
    run("restore"); verify("stock"); assert run("state").stdout.strip()=="1"
    for file,content in [("main",b"unknown"),("main",patched),("main.bak",b"existing"),
                         ("skin/settings.json.bak",b"existing"),("skin/settings.json",b'{"bad":}'),
                         ("skin/settings.json",b'{"text-margin":16,"text-margin":18}')]:
        reset(); (cfw/file).write_bytes(content); before=snapshot()
        run("install",1); assert snapshot()==before
    for file in ("main","skin/settings.json"):
        reset(); (cfw/file).unlink(); before=snapshot(); run("install",1); assert snapshot()==before
    for file in ("main.bak","skin/settings.json.bak",".garlic-menu.original"):
        reset(); run("install"); (cfw/file).write_bytes(b"damaged"); before=snapshot()
        run("restore",1); assert snapshot()==before
    reset(); run("install"); (cfw/"skin/settings.json").write_bytes(b"bad")
    assert run("state").stdout.strip()=="3"; run("restore"); verify("stock")
    reset(); before=snapshot(); (cfw/"skin").chmod(0o555)
    try: run("install",1); assert snapshot()==before
    finally: (cfw/"skin").chmod(0o755)
    reset(); before=snapshot(); fd=os.open(cfw,os.O_RDONLY)
    try:
        fcntl.flock(fd,fcntl.LOCK_EX); run("install",1); assert snapshot()==before
    finally: os.close(fd)
    for name in ("main","settings-additions.json","manifest.json"):
        p=app/"patch"/name; data=p.read_bytes(); p.write_bytes(b"damaged")
        run("install",1); assert snapshot()==before; p.write_bytes(data)
    # Simulate a power cut during the body of a .bak write, before its fsync.
    for cut in (0,1,997):
        reset(); run("install",85,GMS_CRASH="7")
        (cfw/"main.bak").write_bytes(original[:cut])
        assert run("state").stdout.strip()=="1"; verify("stock")
    reset(); run("install",85,GMS_CRASH="8")
    (cfw/"skin/settings.json.bak").write_bytes(start[:21])
    assert run("state").stdout.strip()=="1"; verify("stock")
    # Interrupt recovery itself after rolling back the first live replacement.
    reset(); run("install",85,GMS_CRASH="11")
    run("state",85,GMS_CRASH="1")
    assert run("state").stdout.strip()=="1"; verify("stock")
    for op in ("install","save","restore"):
        for inject in ("GMS_CRASH","GMS_ERROR"):
            for step in range(1,60):
                reset()
                if op!="install": run("install")
                r=run(op,None,**{inject:str(step)})
                assert r.returncode in (0,1,85),r.stderr
                state=run("state").stdout.strip()
                if state=="1": verify("stock")
                else:
                    assert state=="2",state
                    kind="save" if json.loads((cfw/"skin/settings.json").read_bytes())==saved else "install"
                    verify(kind)
                if inject=="GMS_CRASH" and r.returncode!=85:
                    assert r.returncode==0,r.stderr
                    last=step-1; break
                if inject=="GMS_ERROR" and step>last: break
            print(op,inject,last,"interruption points checked")
    print("Install, save, restore, both cards, JSON preservation, backups, package integrity, I/O and lock checks passed")

