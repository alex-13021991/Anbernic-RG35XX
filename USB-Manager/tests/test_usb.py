from pathlib import Path
import os, subprocess, shutil, tempfile
base=Path(__file__).resolve().parent
root=Path(tempfile.mkdtemp(prefix='usb-fixture-',dir=base))
root.mkdir(exist_ok=True)
misc=root/'misc';mmc=root/'mmc';lock=root/'lock'
for d in [misc,mmc/'CFW/autostart',mmc/'CFW/mtp']:d.mkdir(parents=True,exist_ok=True)
shell=Path('C:/Users/admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/native/git/usr/bin/sh.exe')
env=os.environ.copy();env['PATH']=str(shell.parent)+os.pathsep+env['PATH']
source=(base.parent/'src/apply-usb.sh').read_text(encoding='utf8')
for name,value in [('MISC',misc),('MMC',mmc),('LOCK',lock)]:
    import re
    source=re.sub(r'^'+name+r'=.*$',name+"='"+value.as_posix()+"'",source,flags=re.M)
mock='''
mount() {
  case "$*" in
    *remount,rw*) [ "${FAIL_RW:-0}" = 0 ] ;;
    *remount,ro*) [ "${FAIL_RO:-0}" = 0 ] ;;
    *) return 1 ;;
  esac
}
sync() { :; }
touch() {
  if [ "${FAIL_TOUCH:-0}" = 1 ]; then
    case "$*" in *enableADB*) FAIL_TOUCH=0; return 1 ;; esac
  fi
  command touch "$@"
}
'''
source=source.replace('#!/bin/sh\n','#!/bin/sh\n'+mock,1)
script=root/'apply-test.sh';script.write_text(source,encoding='utf8',newline='\n')
subprocess.run([str(shell),'-n',str(script)],check=True,env=env)
names=['enableMTP','enableADB','waitForUSB']
def state():return tuple(int((misc/n).is_file()) for n in names)
def setstate(s):
    for n,v in zip(names,s):
        p=misc/n
        if p.exists():p.unlink()
        if v:p.write_text('original '+n)
def run(s,expected=0,**fail):
    result=subprocess.run([str(shell),str(script),*map(str,s)],env={**env,**{k:str(v) for k,v in fail.items()}},capture_output=True)
    assert result.returncode==expected,(s,expected,result.returncode,result.stdout,result.stderr)
    return result
setstate((0,1,0));run((1,1,0),11);assert state()==(0,1,0)
for p in [misc/'usb.sh',mmc/'CFW/autostart/mtp.sh',mmc/'CFW/mtp/run.sh',mmc/'CFW/mtp/mtp-server']:p.write_text('#!/bin/sh\n',newline='\n')
run((1,1,0),11)
(misc/'dmenu.bin').write_text('  /misc/usb.sh\n')
valid=[(0,0,0),(1,0,0),(0,1,0),(1,1,0),(0,0,1)]
for before in valid:
    for after in valid:
        setstate(before);run(after);assert state()==after
        assert not lock.exists() and not (misc/'.usb-manager-backup').exists()
setstate((1,1,0));run((1,1,1),10);assert state()==(1,1,0)
run(('1;echo BAD',0,0),10)
run((0,0,1),13,FAIL_RW=1);assert state()==(1,1,0);assert not lock.exists()
setstate((0,0,1));run((1,1,0),14,FAIL_TOUCH=1);assert state()==(0,0,1)
assert (misc/'waitForUSB').read_text()=='original waitForUSB'
assert not (misc/'.usb-manager-backup').exists()
run((1,1,0),16,FAIL_RO=1);assert state()==(1,1,0)
lock.mkdir();run((0,0,0),12);assert state()==(1,1,0);lock.rmdir()
backup=misc/'.usb-manager-backup';backup.mkdir();(backup/'ready').touch();(backup/'waitForUSB').write_text('recover')
run((0,1,0),17);assert state()==(0,0,1);assert not backup.exists()
# Post-commit cleanup interruption must keep new settings.
backup.mkdir();(backup/'enableADB').write_text('old');run((0,1,0),17);assert state()==(0,0,1)
print('PASS: 25 saved-mode transitions; invalid args; missing MTP/hook; mount failures; rollback with original contents; lock; interrupted-save recovery; post-commit cleanup')
