"""Rebuild the two original pixel illustrations with Pillow and the bundled font."""
from pathlib import Path
from math import sin
from PIL import Image, ImageDraw, ImageFont

project = Path(__file__).resolve().parents[1]
assets = project / "assets"
def font(size):
    return ImageFont.truetype(str(assets / "font.ttf"), size)
def ridge(draw, width, base, amp, phase, color, bottom):
    points = [(x, round(base + amp * (sin(x*.064+phase)*.5 + sin(x*.021+phase)*.7)))
              for x in range(width)]
    draw.polygon([(0,bottom), *points, (width-1,bottom)], fill=color)
def pine(draw, x, y, size, color):
    draw.rectangle((x-1,y-size,x+1,y),fill=color)
    for n in (2,4,6):
        h = size*n//7
        draw.polygon([(x,y-size+h//3),(x-h//3,y-size+h),(x+h//3,y-size+h)],fill=color)
im = Image.new("RGB",(104,144),"#152d38")
d = ImageDraw.Draw(im)
for y in range(109):
    t=y/110
    d.line((0,y,104,y),fill=tuple(int(a+(b-a)*t) for a,b in zip((34,68,87),(249,175,108))))
d.ellipse((63,25,83,45),fill="#ffe2a3")
d.polygon([(0,81),(29,40),(37,50),(51,33),(92,79),(103,68),(103,113),(0,113)],fill="#6a8290")
d.polygon([(36,52),(51,33),(64,49),(54,45),(50,47),(47,43)],fill="#fff0ce")
ridge(d,104,78,11,1,"#416877",144)
ridge(d,104,90,8,4,"#274d54",144)
d.polygon([(39,89),(60,89),(47,98),(68,108),(23,135),(3,133),(53,108),(34,99)],fill="#e4b987")
for x in [0,4,11,17,83,89,96,103]:
    pine(d,x,111+(x%5),24+x%15,"#16383d")
im = im.resize((208,288),Image.Resampling.NEAREST)
d=ImageDraw.Draw(im)
d.rectangle((0,222,207,287),fill="#163237")
d.text((12,223),"NORTHERN",font=font(22),fill="#fae7bb")
d.text((12,245),"PASSAGE",font=font(31),fill="#fff2d9")
im.save(assets/"preview.png")
icon=Image.new("RGBA",(640,480),(0,0,0,0))
d=ImageDraw.Draw(icon)
d.rounded_rectangle((334,86,614,388),radius=18,fill="#142522",outline="#9dbcaa",width=3)
d.rounded_rectangle((355,111,593,268),radius=8,fill="#30483f")
d.rectangle((367,129,434,247),fill="#17302c")
for y,w in [(144,51),(164,40),(184,54),(204,35),(224,46)]:
    d.rectangle((374,y,374+w,y+4),fill="#b1d6b8" if y==184 else "#779988")
icon.paste(im.resize((77,107),Image.Resampling.NEAREST),(498,136))
for x in (446,460,474):
    d.rectangle((x,145,x+2,239),fill="#81a68e")
for x,y in [(446,165),(460,210),(474,186)]:
    d.rounded_rectangle((x-5,y,x+7,y+10),radius=2,fill="#d6ebb3")
d.text((355,287),"Garlic Menu",font=font(36),fill="#f4f4d9")
d.text((356,330),"Settings",font=font(30),fill="#a8d2ae")
icon.save(project/"Garlic Menu Settings.png")

