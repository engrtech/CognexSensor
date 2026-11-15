from pathlib import Path
from html import escape
from PIL import Image, ImageDraw, ImageFont

OUT = Path(__file__).resolve().parent
WIDTH, HEIGHT, SCALE = 1280, 1040, 3
image = Image.new('RGB', (WIDTH * SCALE, HEIGHT * SCALE), 'white')
draw = ImageDraw.Draw(image)
parts = [f'<svg xmlns="http://www.w3.org/2000/svg" width="{WIDTH}" height="{HEIGHT}" viewBox="0 0 {WIDTH} {HEIGHT}">',
         '<title>75 kiloohm sensor input divider using the carrier R31 pulldown</title>',
         '<desc>X070 remains connected to the PLC. A new 75 kiloohm resistor connects X070 to J20 pin 1. Existing carrier R31, 10 kiloohms, connects this node to common ground at J20 pin 3. Existing R10, 100 ohms, connects the node to MCU PA9. Remove the external 15 kiloohm resistor. J20 pin 2 is unused.</desc>',
         '<rect width="1280" height="1040" fill="white"/>']
INK, WIRE, MUTED, BLUE = '#142e44', '#244456', '#587084', '#1268a5'


def line(points, color=WIRE, width=4):
    draw.line([(round(x*SCALE), round(y*SCALE)) for x, y in points], fill=color,
              width=round(width*SCALE), joint='curve')
    xy = ' '.join(f'{x},{y}' for x, y in points)
    parts.append(f'<polyline points="{xy}" fill="none" stroke="{color}" stroke-width="{width}" stroke-linejoin="round"/>')


def box(x, y, w, h, fill='white', stroke=WIRE, width=2):
    draw.rectangle((x*SCALE,y*SCALE,(x+w)*SCALE,(y+h)*SCALE), fill=fill,
                   outline=stroke, width=round(width*SCALE))
    parts.append(f'<rect x="{x}" y="{y}" width="{w}" height="{h}" fill="{fill}" stroke="{stroke}" stroke-width="{width}"/>')


def text(x, y, label, size=26, color=INK, bold=False, anchor='start'):
    font = ImageFont.truetype('C:/Windows/Fonts/seguisb.ttf' if bold else 'C:/Windows/Fonts/segoeui.ttf', round(size*SCALE))
    pa = {'start':'ls','middle':'ms','end':'rs'}[anchor]
    draw.text((x*SCALE,y*SCALE), label, fill=color, font=font, anchor=pa)
    parts.append(f'<text x="{x}" y="{y}" font-family="Segoe UI, Arial, sans-serif" font-size="{size}" font-weight="{600 if bold else 400}" text-anchor="{anchor}" fill="{color}">{escape(label)}</text>')


def dot(x, y, r=6, color=WIRE):
    draw.ellipse(((x-r)*SCALE,(y-r)*SCALE,(x+r)*SCALE,(y+r)*SCALE),fill=color)
    parts.append(f'<circle cx="{x}" cy="{y}" r="{r}" fill="{color}"/>')


def dashed_box(x, y, w, h):
    for xx in range(x,x+w,18):
        line([(xx,y),(min(xx+9,x+w),y)],'#9db3c9',2)
        line([(xx,y+h),(min(xx+9,x+w),y+h)],'#9db3c9',2)
    for yy in range(y,y+h,18):
        line([(x,yy),(x,min(yy+9,y+h))],'#9db3c9',2)
        line([(x+w,yy),(x+w,min(yy+9,y+h))],'#9db3c9',2)


text(60,62,'Sensor input: low-current divider',40,bold=True)
text(60,105,'One added resistor; the carrier provides the lower divider resistor.',25,color=MUTED)

dashed_box(620,205,600,545)
text(645,248,'FuSa1 carrier — existing components',25,color=MUTED,bold=True)

# Preserve the existing sensor-to-PLC branch; add only a parallel sensing tap.
box(70,165,230,82,fill='#f3f6f8',stroke='#bccbd6')
text(185,199,'PLC input',26,bold=True,anchor='middle')
text(185,230,'existing wire stays',21,color=MUTED,anchor='middle')
line([(260,350),(260,247)])
dot(260,350)
text(70,299,'Sensor OUT',27,bold=True)
text(70,332,'X070',23,color=MUTED)
line([(70,350),(310,350)])

# New, external upper resistor.
box(310,329,145,42,stroke=BLUE,width=4)
line([(455,350),(800,350)])
text(382,296,'75 kΩ · 1%',29,color=BLUE,bold=True,anchor='middle')
text(382,410,'ADD externally',23,color=BLUE,bold=True,anchor='middle')

dot(620,350,7,BLUE)
text(640,300,'J20 pin 1',27,bold=True)
text(640,395,'2.68–2.85 V*',25,color=BLUE,bold=True)
dot(800,350)

# Existing series protection resistor and the MCU GPIO.
line([(800,350),(900,350)])
box(900,333,95,34,width=3)
line([(995,350),(1050,350)])
text(947,305,'R10 · 100 Ω',23,anchor='middle')
box(1050,315,140,83,fill='#f3f6f8',stroke='#bccbd6')
text(1120,350,'MCU',23,color=MUTED,anchor='middle')
text(1120,382,'PA9',29,bold=True,anchor='middle')

# R31 is physically fitted on the carrier, at the J20 side of R10.
line([(800,350),(800,470)])
box(780,470,40,112,width=4)
line([(800,582),(800,660)])
text(850,515,'R31 · 10 kΩ',29,bold=True)
text(850,551,'Already on the carrier',22,color=MUTED)
text(850,582,'Do not add a second one',21,color=MUTED)

# Shared reference: sensor 0 V to carrier ground, not protective earth.
line([(70,660),(800,660)])
dot(620,660,7,BLUE)
dot(800,660)
text(70,606,'Sensor 0 V',27,bold=True)
text(70,639,'N24-14-C (traced return)',22,color=MUTED)
text(640,735,'J20 pin 3 · GND',25,bold=True)
line([(800,660),(800,680)])
line([(775,680),(825,680)],width=3)
line([(783,690),(817,690)],width=3)
line([(791,700),(809,700)],width=3)

box(60,785,1160,67,fill='#fff4e9',stroke='#edc79f')
text(82,827,'REMOVE the external 15 kΩ resistor. J20 pin 2 is unused.',27,color='#8a4314',bold=True)

text(70,904,'No board',27,bold=True)
text(310,904,'X070 = 22.8–24.2 V  →  PA9 ≈ 2.68–2.85 V',25)
text(1110,904,'LED OFF',25,color=MUTED,bold=True,anchor='middle')
text(70,949,'Board present',27,bold=True)
text(310,949,'X070 ≈ 0 V  →  PA9 ≈ 0 V',25)
text(1110,949,'LED ON',25,color=BLUE,bold=True,anchor='middle')
text(60,1000,'*Nominal calculation, 0.27–0.28 mA. Signal range assumed: 21.6–26.4 V. Verify on the connected carrier.',20,color=MUTED)
text(60,1029,'Simplified circuit; other carrier parts omitted. LED behavior requires the corrected active-LOW firmware.',19,color=MUTED)

parts.append('</svg>')
(OUT/'sensor_divider_75k.svg').write_text('\n'.join(parts)+'\n',encoding='utf-8')
image.resize((WIDTH,HEIGHT),Image.Resampling.LANCZOS).save(OUT/'sensor_divider_75k.png')
print(OUT/'sensor_divider_75k.png')
print(OUT/'sensor_divider_75k.svg')
