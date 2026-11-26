#!/usr/bin/env python3
"""Запуск ARM ELF с частными ресурсами, моделью fb0, событиями evdev и OpenAL null."""
import argparse
import hashlib
import os
from pathlib import Path
import struct
import resource
import shutil
import subprocess
import tempfile
import time

parser = argparse.ArgumentParser()
parser.add_argument("work", type=Path)
parser.add_argument("content", type=Path)
parser.add_argument("--level")
parser.add_argument("--bpp", type=int, choices=[16,24,32], default=32)
parser.add_argument("--padded", action="store_true")
parser.add_argument("--garlic", action="store_true", help="Actual device RGB offsets 0/8/16, no alpha")
parser.add_argument("--seconds", type=int, default=15)
parser.add_argument("--binary", type=Path)
parser.add_argument("--library-dir", type=Path)
args = parser.parse_args()
resource.setrlimit(resource.RLIMIT_CORE, (resource.getrlimit(resource.RLIMIT_CORE)[1],)*2)
work = args.work.resolve()
content = args.content.resolve()
assert (content / "Prebaked.pak").is_file()
with tempfile.TemporaryDirectory(dir=work, prefix="qemu-") as tmp:
    root = Path(tmp)
    for part in ["usr", "lib"]:
        (root / part).symlink_to(work / "sysroot" / part)
    dev = root / "dev"
    (dev / "input").mkdir(parents=True)
    fb = dev / "fb0"
    fb.write_bytes(bytes((640+(16 if args.padded else 0))*(480+(4 if args.padded else 0))*(args.bpp//8)))
    pad = dev / "input/event7"
    os.mkfifo(pad)
    pipe = os.open(pad, os.O_RDWR | os.O_NONBLOCK)
    (root / "Content").symlink_to(content)
    (root / "Saves").mkdir()
    name = args.level.replace("/", "-") if args.level else "intro"
    name += f"-{args.bpp}" + ("-padded" if args.padded else "")
    if args.binary: name += "-package"
    if args.garlic: name += "-garlic"
    logpath = work / "reports" / f"qemu-{name}.log"
    env = dict(SDL_VIDEODRIVER="dummy", JAZZ2_RESOLUTION="640x480", LANG="ru_RU.UTF-8",
               ALSOFT_DRIVERS="null", HOME=str(root / "Saves"), TEST_FB=str(fb), TEST_PAD=str(pad),
               LD_PRELOAD=str(work / "tests/simulated-io.so"))
    env["TEST_BPP"] = str(args.bpp)
    if args.garlic: env['TEST_GARLIC_RGB']='1'
    if args.library_dir: env["LD_LIBRARY_PATH"] = str(args.library_dir.resolve())
    if args.padded:
        env["TEST_PADDED"] = "1"
    cmd = ["qemu-arm", "-cpu", "cortex-a9", "-L", str(root)]
    for k,v in env.items():
        cmd.extend(["-E",k+"="+v])
    cmd += [str(args.binary.resolve() if args.binary else work / "build/jazz2"), "/config", "Saves/Jazz2.config", "/max-fps:30"]
    if args.level:
        cmd += ["/level", args.level]
    with logpath.open("wb") as log, subprocess.Popen(cmd, cwd=root, stdout=log, stderr=subprocess.STDOUT) as process:
        try:
            if args.bpp == 24:
                rc = process.wait(timeout=15)
                if rc != 1:
                    raise RuntimeError(f"Framebuffer failure exit={rc}, expected 1")
                print("unsupported framebuffer: clean exit=1")
                continue_test = False
            else:
                continue_test = True
            images = set()
            for i in range(args.seconds*5 if continue_test else 0):
                time.sleep(0.2)
                if process.poll() is not None:
                    raise RuntimeError(f"Ранний выход {process.returncode}. Журнал {logpath}")
                pixels = fb.read_bytes()
                if any(pixels):
                    images.add(hashlib.sha256(pixels).digest())
                    if i == 35:
                        (work / "reports" / f"{name}.raw").write_bytes(pixels)
            if continue_test and len(images) < 2:
                raise RuntimeError(f"Нет смены кадров. Журнал {logpath}")
            # Linux ARM32 input_event: timeval32, type, code, value.
            for value in ([1,0] if continue_test else []):
                os.write(pipe,struct.pack("<iiHHi",0,0,1,0x13c,value))
            rc = process.wait(timeout=8)
            config = root / "Saves/Jazz2.config"
            if continue_test and (rc or not config.is_file() or not config.stat().st_size):
                raise RuntimeError(f"Выход {rc}, настройки {config.exists()}. Журнал {logpath}")
            if continue_test:
                print(f"{name}: кадров с разным SHA-256 {len(images)}, MENU exit={rc}, config={config.stat().st_size} B")
        finally:
            for core in root.glob("qemu*.core"):
                shutil.copyfile(core, work / "reports" / "qemu-crash.core")
            os.close(pipe)
            if process.poll() is None:
                process.kill()
                process.wait()
