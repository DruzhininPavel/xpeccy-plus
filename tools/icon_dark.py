#!/usr/bin/env python3
"""Make the dark-ground variant of a line icon: images/x.png -> images/x-dark.png.

The icon engine (src/xgui/iconengine.cpp) draws x-dark.png instead of x.png while the
style's ground is dark. This is for icons drawn as dark strokes on nothing, which vanish
there: every pixel's lightness is turned over (HLS, 1 - L) and its hue, saturation and
alpha kept - black strokes come out light, a red stays red. x-16.png and x-24.png, where
the icon has them, get x-dark-16.png and x-dark-24.png the same way.

usage: python tools/icon_dark.py images/filter.png [images/target.png ...]
"""
import colorsys
import os
import sys

from PIL import Image

TOP = 0.92		# the lightest a turned pixel gets: light gray, not a glare of white


def turn(src, dst):
    im = Image.open(src).convert("RGBA")
    px = im.load()
    for y in range(im.size[1]):
        for x in range(im.size[0]):
            r, g, b, a = px[x, y]
            if a == 0:
                continue
            h, l, s = colorsys.rgb_to_hls(r / 255.0, g / 255.0, b / 255.0)
            r2, g2, b2 = colorsys.hls_to_rgb(h, min(TOP, 1.0 - l), s)
            px[x, y] = (round(r2 * 255), round(g2 * 255), round(b2 * 255), a)
    im.save(dst, optimize=True)
    print(dst)


def main():
    if len(sys.argv) < 2:
        sys.exit(__doc__)
    for path in sys.argv[1:]:
        base = path[:-4]
        for sfx in ("", "-16", "-24"):
            src = base + sfx + ".png"
            if os.path.exists(src):
                turn(src, base + "-dark" + sfx + ".png")


if __name__ == "__main__":
    main()
