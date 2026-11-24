"""Run the ARM UI with a dummy SDL screen. All writes go to a synthetic TF1."""
import argparse,json,os,shutil,struct,subprocess,tempfile
from pathlib import Path

p=argparse.ArgumentParser()
p.add_argument("stock",type=Path)
p.add_argument("qemu"); p.add_argument("sysroot")
p.add_argument("--frames",type=Path)
a=p.parse_args()
project=Path(__file__).resolve().parents[1]
original=(a.stock/"main").read_bytes(); config=(a.stock/"settings.json").read_bytes()
with tempfile.TemporaryDirectory(prefix="gms-ui-") as tmp:
    tmp=Path(tmp)
    cfw=tmp/"TF1"/"CFW"; (cfw/"skin").mkdir(parents=True)
    main=cfw/"main"; cfg=cfw/"skin/settings.json"
    main.write_bytes(original); main.chmod(0o755); cfg.write_bytes(config)
    app=tmp/"TF2"/"Roms/APPS/GarlicMenuSettings"; app.mkdir(parents=True)
    for directory in ("assets","patch"): shutil.copytree(project/directory,app/directory)
    frames=(a.frames or (tmp/"frames")).resolve()
    def run(name,keys,pages):
        capture=frames/name; capture.mkdir(parents=True,exist_ok=True)
        env=os.environ|{"SDL_VIDEODRIVER":"dummy","SDL_AUDIODRIVER":"dummy","GMS_KEYS":keys,
                        "GMS_FIXTURE":str(cfw),"GMS_FRAMES":str(capture)}
        subprocess.run([a.qemu,"-L",a.sysroot,str(project/"build/ui-test-arm")],cwd=app,env=env,check=True,timeout=30)
        paths=sorted(capture.glob("*.bmp"))
        got=[int(path.stem.split("-")[1]) for path in paths]
        assert got==pages,(name,pages,got)
        for path in paths:
            assert struct.unpack_from("<ii",path.read_bytes(),18)==(640,480)
        return paths
    run("exit-first","a",[0]); assert main.read_bytes()==original and cfg.read_bytes()==config
    run("install","daab",[0,0,1,2])
    assert json.loads(cfg.read_bytes())["text-max-width"]==313
    baseline=cfg.read_bytes()
    run("shoulders","shla",[2]*4)
    assert cfg.read_bytes()==baseline
    run("scroll-off","sssda",[2]*5)
    assert json.loads(cfg.read_bytes())["text-scroll"] is False
    run("scroll-reset","xa",[2,2]); assert cfg.read_bytes()==baseline
    frames1=run("preview","ybybssssdybyb",[2,3,2,3,2,2,2,2,2,2,3,2,3,2])
    assert frames1[1].read_bytes()!=frames1[10].read_bytes()  # Cover off
    assert cfg.read_bytes()==baseline
    frames2=run("side","sssssdyyb",[2]*7+[3,2])
    assert frames2[7].read_bytes()!=frames1[1].read_bytes() # Cover on left
    run("save","dslsqsa",[2]*7)
    changed=json.loads(cfg.read_bytes())
    assert (changed["text-alignment"],changed["text-margin"],changed["text-max-width"])==("center",26,312)
    run("cancel-reset","xb",[2,2]); assert json.loads(cfg.read_bytes())==changed
    run("reset","xa",[2,2]); assert cfg.read_bytes()==baseline
    run("cancel-restore","ssssssab b".replace(" ",""),[2]*7+[4,2])
    assert main.read_bytes()!=original
    run("restore","ssssssadaa",[2]*7+[4,4,5])
    assert main.read_bytes()==original and cfg.read_bytes()==config
    assert not (cfw/"main.bak").exists() and not (cfw/"skin/settings.json.bak").exists()
    run("first-again","b",[0])
    print("13 ARM SDL UI scenarios: 640x480, install OK, A/B/X/Y, L1/R1, covers, confirm/cancel and next launch passed")

