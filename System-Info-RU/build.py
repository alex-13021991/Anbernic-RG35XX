"""Build the original RG35XX executable with Zig 0.13 and the Miyoo uClibc SDK."""
import argparse
import os
import subprocess
from pathlib import Path

base = Path(__file__).resolve().parent
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--sdk", type=Path, required=True, help="SDK's miyoo directory")
parser.add_argument("--zig", type=Path, required=True, help="Zig 0.13 executable")
args = parser.parse_args()
zig, sdk = args.zig.resolve(), args.sdk.resolve()
target = "arm-miyoo-linux-uclibcgnueabi"
root = sdk / target / "sysroot"
libs = root / "usr/lib"
gcc = sdk / "lib/gcc" / target / "7.3.0"
build = base / "build"
build.mkdir(exist_ok=True)
env = {**os.environ, "ZIG_GLOBAL_CACHE_DIR": str(build / "zig-global"),
       "ZIG_LOCAL_CACHE_DIR": str(build / "zig-local")}
sources = ["main", "ui", "screens", "system", "storage", "cache", "diag"]
flags = ["-triple", "armv7-unknown-linux-gnueabi", "-target-cpu", "cortex-a9",
         "-target-abi", "aapcs-linux", "-mfloat-abi", "soft", "-target-feature", "+vfp3",
         "-emit-obj", "-Os", "-std=gnu99", "-fgnuc-version=4.2.1", "-D_GNU_SOURCE",
         "-D_FILE_OFFSET_BITS=64", "-D_REENTRANT", "-Wall", "-Wextra", "-Werror",
         "-nostdsysteminc", "-nobuiltininc", "-isystem", root / "usr/include",
         "-isystem", root / "usr/include/SDL", "-isystem", zig.parent / "lib/include"]
objects = []
for name in sources:
    obj = build / (name + ".o")
    subprocess.run([zig, "-cc1", *flags, base / "src" / (name + ".c"), "-o", obj],
                   env=env, check=True)
    objects.append(obj)
subprocess.run([zig, "ld.lld", "-s", "-m", "armelf_linux_eabi", "--sysroot=" + str(root),
                "-dynamic-linker", "/lib/ld-uClibc.so.0", "-o", build / "system-info",
                libs / "crt1.o", libs / "crti.o", gcc / "crtbegin.o", *objects,
                "-L" + str(libs), "-L" + str(root / "lib"), "-L" + str(gcc),
                "--start-group", "--as-needed", "-lSDL", "-lSDL_ttf", "-lpthread", "-lm",
                "-lc", "-lgcc", "-lgcc_s", "--end-group", gcc / "crtend.o", libs / "crtn.o"],
               check=True, env=env)
print(build / "system-info")

