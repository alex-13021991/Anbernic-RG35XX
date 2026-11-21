"""Recreate the original combined source tree from the three project folders."""
from pathlib import Path
import shutil
import sys

shared = Path(__file__).resolve().parent
target = Path(sys.argv[1]).resolve()
if any(parent == folder for parent in (target, *target.parents)
       for folder in (shared, shared.parent / "USB-Manager", shared.parent / "Screenshot-Viewer")):
    raise SystemExit("Choose a destination outside the source folders")
target.mkdir(parents=True, exist_ok=False)
for folder in (shared, shared.parent / "USB-Manager", shared.parent / "Screenshot-Viewer"):
    for src in folder.rglob("*"):
        if not src.is_file() or src in (folder / "README.md", shared / "collect.py") or "__pycache__" in src.parts:
            continue
        dest = target / src.relative_to(folder)
        dest.parent.mkdir(parents=True, exist_ok=True)
        if dest.exists():
            raise SystemExit(f"Duplicate file: {dest}")
        shutil.copy2(src, dest)
