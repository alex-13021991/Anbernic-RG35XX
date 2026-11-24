"""Run the ARM UI with a dummy SDL screen. All writes go to a synthetic TF1."""
import argparse,json,os,shutil,struct,subprocess,tempfile
from pathlib import Path

p=argparse.ArgumentParser()
p.add_argument("stock",type=Path)
p.add_argument("qemu"); p.add_argument("sysroot")
p.add_argument("--frames",type=Path)
a=p.parse_args()
project=Path(__file__).resolve().parents[1]

def region(path,x,y,w,h):
    bmp=path.read_bytes()
    offset=struct.unpack_from("<I",bmp,10)[0]
    assert struct.unpack_from("<H",bmp,28)[0]==24
    return b"".join(bmp[offset+((479-r)*640+x)*3:offset+((479-r)*640+x+w)*3] for r in range(y,y+h))

original=(a.stock/"main").read_bytes(); config=(a.stock/"settings.json").read_bytes()
with tempfile.TemporaryDirectory(prefix="gms-ui-") as tmp:
    tmp=Path(tmp)
    cfw=tmp/"TF1"/"CFW"; (cfw/"skin").mkdir(parents=True)
    main=cfw/"main"; cfg=cfw/"skin/settings.json"
    main.write_bytes(original); main.chmod(0o755); cfg.write_bytes(config)
    app=tmp/"TF2"/"Roms/APPS/GarlicMenuSettings"; app.mkdir(parents=True)
    for directory in ("assets","patch"): shutil.copytree(project/directory,app/directory)
    frames=(a.frames or (tmp/"frames")).resolve()
    cases=[]
    def run(name,keys,pages):
        cases.append(name)
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
    # A v1.0 config can specify width 0 and still have scroll=true.
    legacy=json.loads(baseline)
    legacy["text-max-width"]=0
    cfg.write_text(json.dumps(legacy))
    original_legacy=cfg.read_bytes()
    zero=run("width-zero-cancel","ssswb",[2]*5)
    assert cfg.read_bytes()==original_legacy
    def focused(frame,row):
        return region(frame,12,70+46*row,1,1)!=region(frame,10,70+46*row,1,1)
    assert focused(zero[2],2) and focused(zero[3],4) and focused(zero[4],2)
    run("width-zero-save","a",[2])
    assert json.loads(cfg.read_bytes())["text-scroll"] is False
    run("reenable-scroll","ssdsda",[2]*6)
    current=json.loads(cfg.read_bytes())
    assert current["text-max-width"]==1 and current["text-scroll"] is True
    run("width-to-zero","ssqa",[2]*4)
    assert json.loads(cfg.read_bytes())["text-scroll"] is False
    run("width-reset","xa",[2,2]); assert cfg.read_bytes()==baseline
    hidden=run("cover-only","ssssda",[2]*6)
    assert cfg.read_bytes()==baseline
    assert region(hidden[5],18,210,278,24)==region(zero[0],18,210,278,24)
    alignment=run("center-right","ddb",[2]*3)
    for f in alignment[1:]:
        assert region(f,18,210,278,24)==region(zero[0],18,210,278,24)
    assert region(alignment[0],18,210,278,24)!=region(zero[0],18,210,278,24)
    # Sample unselected text in the SDL framebuffer. Both its label and its
    # value must switch from their active colours to the dim palette.
    def dimmed(frame,row):
        y=75+46*row
        peaks=[max(region(frame,x,y,w,25)) for x,w in ((21,145),(175,118))]
        return all(110 < peak < 170 for peak in peaks)
    for frame,margin_dim,width_dim in zip(alignment,(False,True,False),(False,True,True)):
        assert dimmed(frame,1)==margin_dim and dimmed(frame,2)==width_dim
    assert region(alignment[0],18,119,276,26)==region(alignment[2],18,119,276,26)
    assert region(alignment[1],18,165,276,26)==region(alignment[2],18,165,276,26)
    assert dimmed(hidden[5],5) and not dimmed(hidden[0],5)

    keys="dswdslswwdssb"
    nav=run("alignment-navigation",keys,[2]*len(keys))
    expected_rows=[0,0,4,0,0,1,1,4,1,0,0,1,2]
    assert all(focused(frame,row) for frame,row in zip(nav,expected_rows,strict=True))
    assert cfg.read_bytes()==baseline
    keys="sssssqwdswdsb"
    side=run("cover-side-memory",keys,[2]*len(keys))
    assert focused(side[9],6) and focused(side[10],4) and focused(side[12],5)
    assert region(side[5],18,301,276,28)!=region(side[6],18,301,276,28)
    assert region(side[6],18,301,276,28)==region(side[12],18,301,276,28)
    assert cfg.read_bytes()==baseline
    keys="dsdswwwwb"
    combined=run("center-without-cover",keys,[2]*len(keys))
    assert all(focused(frame,row) for frame,row in zip(combined,[0,0,4,4,6,4,0,6,4],strict=True))
    assert cfg.read_bytes()==baseline
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
    run("save","slsqwwda",[2]*8)
    changed=json.loads(cfg.read_bytes())
    assert changed["text-scroll"] is False
    assert (changed["text-alignment"],changed["text-margin"],changed["text-max-width"])==("center",26,312)
    run("save-remembered-values","qa",[2]*2)
    assert json.loads(cfg.read_bytes())==changed|{"text-alignment":"left"}
    run("save-center-again","da",[2]*2)
    run("cancel-reset","xb",[2,2]); assert json.loads(cfg.read_bytes())==changed
    run("reset","xa",[2,2]); assert cfg.read_bytes()==baseline
    hint=run("cancel-restore","ssssssab b".replace(" ",""),[2]*7+[4,2])
    assert region(hint[0],18,446,170,28)!=region(hint[6],18,446,170,28)
    assert region(hint[0],191,446,449,28)==region(hint[6],191,446,449,28)
    # SDL writes BGR. Confirm the restore action is amber.
    ink=region(hint[6],18,446,170,28)
    assert any(r>220 and g>155 and b<155 and r>g for b,g,r in zip(ink[::3],ink[1::3],ink[2::3]))
    assert main.read_bytes()!=original
    run("restore","ssssssadaa",[2]*7+[4,4,5])
    assert main.read_bytes()==original and cfg.read_bytes()==config
    assert not (cfw/"main.bak").exists() and not (cfw/"skin/settings.json.bak").exists()
    run("first-again","b",[0])
    print(f"{len(cases)} ARM SDL UI scenarios passed, including disabled scroll and the contextual restore hint")

