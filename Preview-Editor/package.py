"""Build the install and standalone source ZIPs, preserving Unix execute bits."""
import argparse
import hashlib
import stat
import zipfile
from pathlib import Path

project = Path(__file__).resolve().parent
release = "PreviewEditor-1.1.0"

def add(archive, source, member, executable=False):
    info = zipfile.ZipInfo(member, (2026, 11, 27, 0, 0, 0))
    info.create_system = 3
    info.external_attr = (stat.S_IFREG | (0o755 if executable else 0o644)) << 16
    info.compress_type = zipfile.ZIP_DEFLATED
    archive.writestr(info, source.read_bytes())

def tree(archive, folder, prefix):
    for path in sorted(folder.rglob("*")):
        if path.is_file() and "__pycache__" not in path.parts:
            add(archive, path, prefix + path.relative_to(folder).as_posix())

def build(out):
    out.mkdir(parents=True, exist_ok=True)
    install = out / (release + "-GarlicOS-1.4.9.zip")
    source = out / (release + "-source.zip")
    base = "Roms/APPS/"
    app = base + "PreviewEditor/"
    with zipfile.ZipFile(install, "w") as z:
        add(z, project / "Preview Editor.sh", base + "Preview Editor.sh", True)
        add(z, project / "assets/preview.png", base + "Imgs/Preview Editor.png")
        add(z, project / "build/preview-editor", app + "preview-editor", True)
        add(z, project / "build/libpng16.so.16", app + "lib/libpng16.so.16")
        add(z, project / "build/libpng-license.txt", app + "licenses/libpng-license.txt")
        for directory in ("assets", "licenses"):
            tree(z, project / directory, app + directory + "/")
        add(z, project / "Установка.txt", "Установка.txt")
        add(z, project / "README.md", app + "README.md")
        add(z, project / "CHANGELOG.md", app + "CHANGELOG.md")
        tree(z, project / "docs", app + "docs/")
    with zipfile.ZipFile(source, "w") as z:
        for directory in ("src", "tests", "assets", "licenses", "vendor", "docs"):
            tree(z, project / directory, release + "/" + directory + "/")
        for name in ("README.md", "CHANGELOG.md", "Установка.txt", "Preview Editor.sh", "package.py", "build.sh", ".gitattributes", ".gitignore"):
            add(z, project / name, release + "/" + name, name.endswith(".sh"))
    sums = []
    for archive in (install, source):
        with zipfile.ZipFile(archive) as z:
            assert z.testzip() is None
        sums.append(hashlib.sha256(archive.read_bytes()).hexdigest() + "  " + archive.name)
    (out / "SHA256SUMS.txt").write_bytes(("\n".join(sums) + "\n").encode())
    print(*sums, sep="\n")

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("out", type=Path, nargs="?", default=project / "dist")
    build(parser.parse_args().out)
