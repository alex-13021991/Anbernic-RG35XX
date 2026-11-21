from pathlib import Path
import subprocess, os, re, concurrent.futures
base=Path(__file__).resolve().parent
zig=base/'zig/zig-windows-x86_64-0.13.0/zig.exe'
tc=base/'toolchain/miyoo'
sysroot=tc/'arm-miyoo-linux-uclibcgnueabi/sysroot'
cpp=tc/'arm-miyoo-linux-uclibcgnueabi/include/c++/7.3.0'
gcc=tc/'lib/gcc/arm-miyoo-linux-uclibcgnueabi/7.3.0'
src=base/'src'
out=base/'build'
out.mkdir(exist_ok=True)
env=os.environ.copy()
env['ZIG_GLOBAL_CACHE_DIR']=str(base/'zig-cache')
env['ZIG_LOCAL_CACHE_DIR']=str(base/'zig-local-cache')
flags=['-target','arm-linux-gnueabi','-mcpu=cortex_a9','-mfloat-abi=softfp','-mfpu=neon-fp16','-marm','-Os','-std=gnu++11','-nostdinc++','-D_GNU_SOURCE=1','-D_REENTRANT','--sysroot='+str(sysroot),'-isystem',str(cpp),'-isystem',str(cpp/'arm-miyoo-linux-uclibcgnueabi'),'-isystem',str(cpp/'backward'),'-isystem',str(sysroot/'usr/include/SDL')]
flags += ['-nostdlib','-nostdinc','-isystem',str(sysroot/'usr/include'),'-isystem',str(zig.parent/'lib/include')]
make=(src/'Makefile').read_text()
for line in make.splitlines():
    if line.startswith('CXXFLAGS += -D'):
        define=line.split('+= ',1)[1].split('\t')[0].strip().replace('\\"','"')
        if define.startswith('-DFONTS='): define=define.replace("'",'')
        if define.startswith('-DRESDIR='): define='-DRESDIR="res"'
        flags.append(define)
objects=re.search(r'OBJS :=(.*?)\n\n',make,re.S)[1].replace('\\','').split()
def compile(obj):
    stem=Path(obj).stem
    source=src/'src'/(stem+'.cpp')
    if not source.exists(): source=source.with_suffix('.c')
    cmd=[str(zig),'cc',*flags,'-x','c++','-c',str(source),'-o',str(out/obj)]
    p=subprocess.run(cmd,env=env,capture_output=True,text=True)
    (out/(stem+'.log')).write_text(p.stdout+p.stderr)
    return obj,p.returncode,p.stderr
results=list(concurrent.futures.ThreadPoolExecutor(4).map(compile,objects))
for obj,code,err in results:
    print(obj, 'OK' if not code else 'FAILED', flush=True)
    if code: print(err[:5000], flush=True)
if any(code for _,code,_ in results): raise SystemExit(1)
libs=sysroot/'usr/lib'
cmd=[str(zig),'ld.lld','-s','-m','armelf_linux_eabi','--sysroot='+str(sysroot),'-dynamic-linker','/lib/ld-uClibc.so.0','-o',str(out/'DinguxCommander'),str(libs/'crt1.o'),str(libs/'crti.o'),str(gcc/'crtbegin.o'),*[str(out/o) for o in objects],'-L'+str(libs),'-L'+str(sysroot/'lib'),'-L'+str(gcc),'--start-group','-lstdc++','-lSDL','-lpthread','-lSDL_image','-lSDL_ttf','-lm','-lc','-lgcc','-lgcc_s','--end-group',str(gcc/'crtend.o'),str(libs/'crtn.o')]
p=subprocess.run(cmd,env=env,capture_output=True,text=True)
(out/'link.log').write_text(p.stdout+p.stderr)
print(p.stdout+p.stderr)
raise SystemExit(p.returncode)
