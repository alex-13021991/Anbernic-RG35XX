from pathlib import Path
from PIL import Image
import struct
base=Path(__file__).resolve().parent/'fixtures'
for folder in ['tf1/Screenshots','tf2/Screenshots','tf1/CFW/retroarch/.retroarch','tf2/custom']:
    (base/folder).mkdir(parents=True,exist_ok=True)
for file in ['tf1/Screenshots/same.png','tf1/Screenshots/changed.png','tf2/Screenshots/same.png','tf2/custom/extra.png']:
    Image.new('RGB',(16,12),(80,100,150)).save(base/file)
(base/'tf1/Screenshots/bad.png').write_bytes(b'not a png')
(base/'tf1/Screenshots/ignored.txt').write_text('ignore')
(base/'tf1/CFW/retroarch/.retroarch/retroarch.cfg').write_text('screenshot_directory = "/fixtures/tf1/Screenshots"\n')
(base/'extra.cfg').write_text('/fixtures/tf2/custom\n/fixtures/tf1/Screenshots\n')
(base/'huge.png').write_bytes(b'\x89PNG\r\n\x1a\n'+struct.pack('>I',13)+b'IHDR'+struct.pack('>II',100000,100000))
print(base)
