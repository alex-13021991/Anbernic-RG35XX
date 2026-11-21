"""Build both ARM/uClibc apps with Zig 0.13 or the original Linux GCC.
Pass --sdk /path/to/miyoo --zig /path/to/zig; defaults reuse local toolchain.
"""
from pathlib import Path
import argparse, os, subprocess, sys
base=Path(__file__).resolve().parent
prior=base.parents[1]/'dingux-commander-dingux-commander-rg35xx-garlicos/work'
p=argparse.ArgumentParser()
p.add_argument('--sdk',type=Path,default=prior/'toolchain/miyoo')
p.add_argument('--zig',type=Path,default=prior/'zig/zig-windows-x86_64-0.13.0/zig.exe')
a=p.parse_args()
sdk=a.sdk.resolve(); zig=a.zig.resolve()
root=sdk/'arm-miyoo-linux-uclibcgnueabi/sysroot'
gcc=sdk/'lib/gcc/arm-miyoo-linux-uclibcgnueabi/7.3.0'
out=base/'build';out.mkdir(exist_ok=True)
env=os.environ.copy();env['ZIG_GLOBAL_CACHE_DIR']=str(base/'zig-cache');env['ZIG_LOCAL_CACHE_DIR']=str(base/'zig-local-cache')
flags=['-target','arm-linux-gnueabi','-mcpu=cortex_a9','-mfloat-abi=softfp','-mfpu=neon-fp16','-marm',
       '-Os','-g','-std=gnu99','-D_GNU_SOURCE=1','-D_REENTRANT','-Wall','-Wextra','-Werror',
       '-Wno-misleading-indentation','-nostdlib','-nostdinc','--sysroot='+str(root),
       '-isystem',str(root/'usr/include'),'-isystem',str(root/'usr/include/SDL'),
       '-isystem',str(zig.parent/'lib/include')]
def run(cmd,log):
    r=subprocess.run([str(x) for x in cmd],env=env,capture_output=True,text=True)
    (out/log).write_text(r.stdout+r.stderr,encoding='utf8')
    if r.stdout or r.stderr: print(r.stdout+r.stderr)
    if r.returncode: raise SystemExit(r.returncode)
for source in ['ui','gallery','usb_manager','screenshot_viewer']:
    run([zig,'cc',*flags,'-c',base/'src'/f'{source}.c','-o',out/f'{source}.o'],source+'.log')
    print('Compiled',source,flush=True)
for app,objects in [('usb-manager',['ui','usb_manager']),('screenshot-viewer',['ui','gallery','screenshot_viewer'])]:
    libs=root/'usr/lib'
    run([zig,'ld.lld','-m','armelf_linux_eabi','--sysroot='+str(root),'-dynamic-linker','/lib/ld-uClibc.so.0',
         '-o',out/app,libs/'crt1.o',libs/'crti.o',gcc/'crtbegin.o',
         *[out/f'{s}.o' for s in objects],'-L'+str(libs),'-L'+str(root/'lib'),'-L'+str(gcc),
         '--start-group','-lSDL','-lSDL_image','-lSDL_ttf','-lpthread','-lm','-lc','-lgcc','-lgcc_s',
         '--end-group',gcc/'crtend.o',libs/'crtn.o'],app+'-link.log')
    print('Linked',app,flush=True)
