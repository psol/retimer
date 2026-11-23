#!/usr/bin/env python3
"""Convertit les écrans de référence (test/host/references/*.pbm) en PNG.

Les images vont dans docs/ecrans/, agrandies ×2 et entourées d'un cadre gris
qui figure le bord de l'écran, pour être relues sans carte, sur téléphone.
Une planche docs/ecrans/planche.png les rassemble toutes.

    python3 tools/screens.py            (nécessite Pillow : pip install pillow)
"""
import pathlib
from PIL import Image, ImageDraw

root = pathlib.Path(__file__).resolve().parent.parent
source = root / "test/host/references"
destination = root / "docs/ecrans"
destination.mkdir(parents=True, exist_ok=True)

SCALE = 2
FRAME = 12
BACKGROUND = (205, 205, 200)  # bord du boîtier
PAPER = (238, 236, 228) # blanc de l'e-ink
INK = (30, 30, 30)

thumbnails = []
for pbm in sorted(source.glob("*.pbm")):
    screen = Image.open(pbm).convert("1")
    w, h = screen.size
    colour = Image.new("RGB", (w, h), PAPER)
    colour.paste(INK, mask=screen.point(lambda v: 255 - v))
    colour = colour.resize((w * SCALE, h * SCALE), Image.NEAREST)
    framed = Image.new("RGB", (w * SCALE + 2 * FRAME, h * SCALE + 2 * FRAME), BACKGROUND)
    framed.paste(colour, (FRAME, FRAME))
    framed.save(destination / (pbm.stem + ".png"), optimize=True)
    thumbnails.append((pbm.stem, framed))

if thumbnails:
    columns = 3
    tw, th = thumbnails[0][1].size
    title = 24
    rows = (len(thumbnails) + columns - 1) // columns
    sheet = Image.new("RGB", (columns * tw, rows * (th + title)), (255, 255, 255))
    drawing = ImageDraw.Draw(sheet)
    for i, (name, image) in enumerate(thumbnails):
        x = (i % columns) * tw
        y = (i // columns) * (th + title)
        drawing.text((x + FRAME, y + 6), name, fill=(0, 0, 0))
        sheet.paste(image, (x, y + title))
    sheet.save(destination / "planche.png", optimize=True)
print(len(thumbnails), "écrans")
