#!/usr/bin/env python3
"""Generate the K6WP placeholder tray/app icons (Todo 9).

Worker-placeholder artwork: a simple "K6" glyph on a rounded-square
background. Replaceable without any code changes - drop in new
packaging/app.ico + packaging/app_paused.ico (same filenames) and rebuild.

Method: Pillow (PIL) draws each size (16/32/48/256) from a 256 px base and
writes a multi-size ICO. Requires: pip install pillow.

Usage: python packaging/make_app_icon.py
"""

import os

from PIL import Image, ImageDraw, ImageFont

SIZES = [16, 32, 48, 256]

# Output next to this script, regardless of the caller's CWD.
OUT_DIR = os.path.dirname(os.path.abspath(__file__))


def _font(size):
    for name in ("arialbd.ttf", "segoeuib.ttf", "arial.ttf"):
        try:
            return ImageFont.truetype("C:/Windows/Fonts/" + name, size)
        except OSError:
            continue
    return ImageFont.load_default(size)


def _base(bg_top, bg_bottom, glyph):
    """256 px rounded-square base with a vertical gradient + centered glyph."""
    size = 256
    img = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)
    radius = int(size * 0.22)
    for y in range(size):
        t = y / (size - 1)
        color = tuple(int(bg_top[c] + (bg_bottom[c] - bg_top[c]) * t) for c in range(3)) + (255,)
        d.line([(radius, y), (size - radius, y)], fill=color)
    mask = Image.new("L", (size, size), 0)
    md = ImageDraw.Draw(mask)
    md.rounded_rectangle([0, 0, size - 1, size - 1], radius=radius, fill=255)
    img.putalpha(mask)
    d = ImageDraw.Draw(img)
    font = _font(int(size * 0.52))
    bbox = d.textbbox((0, 0), glyph, font=font)
    w = bbox[2] - bbox[0]
    h = bbox[3] - bbox[1]
    d.text(((size - w) / 2 - bbox[0], (size - h) / 2 - bbox[1]), glyph,
           font=font, fill=(255, 255, 255, 255))
    return img


def _paused_base():
    """Amber variant with two pause bars under the glyph (distinct at 16 px)."""
    img = _base((245, 158, 11), (180, 83, 9), "K6")
    d = ImageDraw.Draw(img)
    bar_w = int(256 * 0.10)
    bar_h = int(256 * 0.16)
    gap = int(256 * 0.06)
    y = int(256 * 0.66)
    x0 = (256 - 2 * bar_w - gap) / 2
    d.rounded_rectangle([x0, y, x0 + bar_w, y + bar_h], radius=bar_w // 2,
                        fill=(255, 255, 255, 255))
    d.rounded_rectangle([x0 + bar_w + gap, y, x0 + 2 * bar_w + gap, y + bar_h],
                        radius=bar_w // 2, fill=(255, 255, 255, 255))
    return img


def main():
    _base((99, 102, 241), (67, 56, 202), "K6").save(
        os.path.join(OUT_DIR, "app.ico"), sizes=[(s, s) for s in SIZES])
    _paused_base().save(os.path.join(OUT_DIR, "app_paused.ico"),
                        sizes=[(s, s) for s in SIZES])
    print("wrote app.ico + app_paused.ico sizes=%s" % SIZES)


if __name__ == "__main__":
    main()