#!/usr/bin/env python3
from pathlib import Path
import subprocess,shlex,os
port=Path(__file__).resolve().parents[1];w=Path(os.environ.get('PVZ_WORK',str(port/'.work')));sdk=Path(os.environ.get('RG_TOOLCHAIN',str(Path.home()/'RG35XX/RG35XX-SDK/miyoo-gcc-7.3.0')))
env=dict(os.environ,LD_LIBRARY_PATH=str(sdk/'lib'),LC_ALL='C')
commands=subprocess.check_output(['ninja','-C',str(w/'build'),'-t','commands','PvZ'],text=True).splitlines()
compile=shlex.split(next(x for x in commands if ' -c ' in x and x.endswith('/src/main_miyoo.cpp')))
compile[compile.index('-c')+1]=str(port/'sources/pvz/platform/rg35xx/test-lowres.cpp')
compile[compile.index('-o')+1]=str(w/'build/test-lowres.o')
subprocess.run(compile,cwd=w/'build',env=env,check=True)
args=shlex.split(commands[-1])
if args[:2]==[":","&&"]: args=args[2:]
if args[-2:]==["&&",":"]: args=args[:-2]
args=[str(w/'build/test-lowres.o') if x.endswith('src/main_miyoo.cpp.o') else x for x in args]
args[args.index('-o')+1]=str(w/'build/test-lowres')
r=subprocess.run(args,cwd=w/'build',env=env,capture_output=True,text=True)
(w/'logs/link-test-lowres.log').write_text(r.stdout+r.stderr)
print('link exit',r.returncode)
if r.returncode:print(r.stderr[-3500:]);raise SystemExit(r.returncode)
