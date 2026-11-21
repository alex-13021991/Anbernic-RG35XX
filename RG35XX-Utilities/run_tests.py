from pathlib import Path
import os,subprocess,sys
base=Path(__file__).resolve().parent
prior=base.parents[2]/'2026-11-17/dingux-commander-dingux-commander-rg35xx-garlicos/work'
zig=prior/'zig/zig-windows-x86_64-0.13.0/zig.exe'
node=Path('C:/Users/admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/node/bin/node.exe')
shell=Path('C:/Users/admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/native/git/usr/bin/sh.exe')
env=os.environ.copy();env['PYTHONIOENCODING']='utf-8';env['ZIG_GLOBAL_CACHE_DIR']=str(base/'zig-cache');env['ZIG_LOCAL_CACHE_DIR']=str(base/'zig-local-cache')
log=[]
def run(args):
    r=subprocess.run([str(s) for s in args],env=env,capture_output=True,encoding='utf8',errors='replace')
    text=r.stdout+r.stderr;print(text,end='');log.append(text)
    if r.returncode:raise SystemExit(r.returncode)
run([sys.executable,base/'build.py'])
run([sys.executable,base/'tests/test_usb.py'])
run([zig,'cc','-target','wasm32-wasi','-D_GNU_SOURCE','-O0','-g','-Wl,-z,stack-size=1048576',
     '-include',base/'tests/wasi_compat.h',base/'tests/test_core.c',base/'src/gallery.c',base/'tests/wasi_compat.c',
     '-o',base/'build/test-core.wasm'])
run([sys.executable,base/'tests/fixtures.py'])
run([node,base/'tests/run-wasi.cjs',base/'build/test-core.wasm',base/'tests/fixtures'])
run([sys.executable,base/'verify.py'])
for script in (base/'src').glob('*.sh'):
    b=script.read_bytes();assert b'\r' not in b and not b.startswith(b'\xef\xbb\xbf'),script
    run([shell,'-n',script])
log.append('PASS: launcher/helper shell syntax, LF and no BOM.\n')
log.append('NOT TESTED: target hardware, SDL framebuffer, actual mount/reboot, live USB role.\n')
(base/'TESTS.txt').write_text(''.join(log),encoding='utf8')
print('All checks passed; report saved to work/TESTS.txt')
