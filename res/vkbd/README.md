# The virtual keyboard

- `zx-black.json`, `zx-white.json` - the skins, drawn for Xpeccy+ by Volutar in the format of
  his `layout_svg.py` (canvas, gradient, contour, colors, icons, rows of keys with their
  labels). The program draws them itself at the size the window has (`src/xgui/vkbdskin.cpp`);
  the keys' places in the rows say which Spectrum key each one is.
- `keys-font.ttf` - the lettering, made by `tools/vkbd_font.py` from Liberation Sans Bold
  2.1.5; see LICENSE_ICONS and LICENSE_LIBERATION.

A file of the same name in `vkbd/` of the settings folder is drawn instead of the built-in
one, and again each time it is saved. `zx-128.json` there adds the 128 skin to the menu.

## Fields of the program's own

All of them may be left out; a layout without them is drawn as before.

In `canvas`:
- `"ground"` - what shows through the contours around the keys (the picture's `bg` if not
  given).
- `"lit"` - the color a pressed key's contour shows (`#00c8ff`), `"hover"` - the one under
  the pointer (the ground, 30% of the way to white).

In a label: `"eb"` - that many canvas pixels thicker, a number or `[x, y]`.

In a key:
- `"zx"` - what it presses: `"1"`..`"0"`, `"A"`..`"Z"`, `"CS"`, `"SS"`, `"ENTER"`, `"SPACE"`,
  or several at once, `["CS", "1"]`; `[]` presses nothing (lettering). Without it the key's
  place in the 48K's four rows of ten says what it is.
- `"y"`, `"h"` - its own, over the row's.
- `"outline"` - `[[x, y], ...]`, a polygon of right angles in canvas units drawn instead of
  the box (an Enter over two rows); its labels are placed from the middle of its bounds.
- `"corner"` - its own corner radius, a number or `[tl, tr, br, bl]`. On an outline the
  four go to the corners that jut out by the side they face; the corners that cut in take
  the smallest.

- `"gradient"` - its own, as the layout's below; `"dome"` - its own (over the layout's), or
  `false` for none.

`"gradient"` (the caps', and a key's own): the stops, or `{"stops", "h", "from"}` - `"h"` its
length in canvas units (the whole body if not given) from the body's top, or its bottom with
`"from": "bottom"`; past it the end colors go on. A key two rows tall keeps a one-row shading.

`"dome"` - a shape laid on every cap (the 128's rounded pad): `"inset"` `[left, top, right,
bottom]` from the key's box (a number for all four; negative goes past it), `"corner"` a
number or `[tl, tr, br, bl]`, `"cut"` - cut flat that far below the key's top, and
`"gradient"` (as above) or `"fill"` a color. It never leaves the cap.

In `rainbow`: `"bands"` - `[[color, x], ...]`, each band's x at `"y0"`; the bands run from
`"y0"` to `"y1"` (by default the four of the 48K, over the whole height); `"bands": []` for
none.
