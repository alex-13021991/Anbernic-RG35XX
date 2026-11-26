#!/usr/bin/env python3
"""Restore the unchanged SDK archive from Git-friendly volumes, verifying hashes."""
import argparse
import hashlib
import json
from pathlib import Path
import os

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('output', type=Path, help='Destination env_toolchain.7z (outside repository recommended)')
args = parser.parse_args()
base = Path(__file__).resolve().parent
manifest = json.loads((base / 'manifest.json').read_text(encoding='utf-8'))
output = args.output.expanduser().resolve()
if output.exists():
    raise SystemExit('Destination already exists; choose another path: ' + str(output))
output.parent.mkdir(parents=True, exist_ok=True)
temporary = output.with_name(output.name + '.assembling')
combined = hashlib.sha256()
created = False
try:
    with temporary.open('xb') as dest:
        created = True
        for part in manifest['parts']:
            source = base / part['name']
            digest = hashlib.sha256()
            count = 0
            with source.open('rb') as src:
                while True:
                    data = src.read(1024 * 1024)
                    if not data:
                        break
                    count += len(data)
                    digest.update(data)
                    combined.update(data)
                    dest.write(data)
            if count != part['bytes'] or digest.hexdigest() != part['sha256']:
                raise ValueError('Corrupt or incomplete volume: ' + part['name'])
            print(part['name'] + ': OK', flush=True)
    if temporary.stat().st_size != manifest['bytes'] or combined.hexdigest() != manifest['sha256']:
        raise ValueError('Combined archive checksum mismatch')
    # Hard-link installation avoids overwriting a destination created concurrently.
    os.link(str(temporary), str(output))
    temporary.unlink()
except Exception:
    # Never remove a temporary file which predated this invocation.
    if created:
        temporary.unlink(missing_ok=True)
    raise
print('Archive: ' + str(output))
print('SHA256: ' + combined.hexdigest())
