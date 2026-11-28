#!/usr/bin/env python3
from pathlib import Path
import tempfile,shutil,subprocess
r=Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory(prefix='pvz-launcher-') as d:
 p=Path(d);shutil.copy2(r/'Package/Roms/PORTS/PlantsVsZombies/launch.sh',p/'launch.sh')
 (p/'main_mmp.pak').touch();(p/'PvZ').write_text('#!/bin/sh\nprintf "TEST_SELECTED=%s\\n" "$PVZ_RENDER_WIDTH"\n');(p/'PvZ').chmod(0o755)
 for value,expected in [(None,640),('320\n',320),('400\r\n',400),('480\n',480),('640\n',640),('800\n',800),('600\n',640),('400junk\n',640),('',640)]:
  cfg=p/'render-width.txt'
  if value is None:cfg.unlink(missing_ok=True)
  else:cfg.write_bytes(value.encode())
  subprocess.run(['/bin/sh',str(p/'launch.sh')],check=True)
  assert f'TEST_SELECTED={expected}' in (p/'run.log').read_text(),repr(value)
 print('PASS: launcher default 640, all five supported widths, CRLF, invalid and missing configuration')
