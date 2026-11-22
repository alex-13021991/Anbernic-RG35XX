"""Package a collected RG35XX-Utilities source tree."""
import argparse
import hashlib
import zipfile
from pathlib import Path

base = Path(__file__).resolve().parent
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--out", type=Path, default=base.parent / "outputs")
out = parser.parse_args().out.resolve()
out.mkdir(parents=True, exist_ok=True)
version = "0.1.5"


def pack(name, files, include_manifest=False):
    payload = {path: file.read_bytes() for path, file in files.items()}
    if include_manifest:
        payload["SHA256.txt"] = "".join(
            f"{hashlib.sha256(data).hexdigest()}  {path}\n"
            for path, data in sorted(payload.items())
        ).encode()
    with zipfile.ZipFile(out / name, "w") as archive:
        for path, data in sorted(payload.items()):
            info = zipfile.ZipInfo(path, (2026, 11, 22, 0, 0, 0))
            info.create_system = 3
            executable = path.endswith((".sh", ".so", "/usb-manager", "/screenshot-viewer"))
            info.external_attr = (0o100755 if executable else 0o100644) << 16
            if path.endswith(".sh") and (b"\r" in data or not data.startswith(b"#!")):
                raise ValueError(f"Invalid shell script: {path}")
            info.compress_type = zipfile.ZIP_DEFLATED
            archive.writestr(info, data)
    with zipfile.ZipFile(out / name) as archive:
        assert archive.testzip() is None and set(archive.namelist()) == set(payload)
        assert all(archive.read(path) == data for path, data in payload.items())
    print(name)


for title, folder, binary, helper in [
    ("Screenshot Viewer", "ScreenshotViewer", "screenshot-viewer", "screenshots.cfg"),
    ("USB Manager", "USBManager", "usb-manager", "apply-usb.sh"),
]:
    app = f"APPS/{folder}"
    files = {
        f"APPS/{title}.sh": base / "src" / f"{title}.sh",
        f"APPS/Imgs/{title}.png": base / "resources/Imgs" / f"{title}.png",
        f"{app}/{binary}": base / "build" / binary,
        f"{app}/{helper}": base / "src" / helper,
        f"{app}/j2k.so": base / "resources/j2k.so",
        f"{app}/res/FreeSans.ttf": base / "resources/FreeSans.ttf",
        "README_RU.txt": base / "README_RU.txt",
        "TESTS.txt": base / "TESTS.txt",
    }
    files.update({f"licenses/{p.name}": p for p in (base / "licenses").glob("*.txt")})
    pack(f"{folder}-{version}-GarlicOS-1.4.9.zip", files, include_manifest=True)

source = {}
for pattern in ("src/*", "tests/*", "resources/*", "resources/Imgs/*", "licenses/*.txt"):
    for path in (p for p in base.glob(pattern) if p.is_file()):
        source[path.relative_to(base).as_posix()] = path
for name in ("build-wsl.sh", "build.py", "package.py", "verify.py", "run_tests.py",
             "README_RU.txt", "BUILD.md", "TESTS.txt", "LICENSE.txt", "font-license.txt"):
    source[name] = base / name
pack(f"RG35XX-Utilities-{version}-source.zip", source)
