# The virtual keyboard

- `zx-black.json`, `zx-white.json` - the skins, drawn for Xpeccy+ by Volutar in the format of
  his `layout_svg.py` (canvas, gradient, contour, colors, icons, rows of keys with their
  labels). The program draws them itself at the size the window has (`src/xgui/vkbdskin.cpp`);
  the keys' places in the rows say which Spectrum key each one is.
- Two fields of the program's own: `"ground"` in `canvas` - what shows through the contours
  around the keys (the picture's `bg` if it is not there); `"eb"` in a label - that many
  canvas pixels thicker, a number or `[x, y]`.
- `keys-font.ttf` - the lettering, made by `tools/vkbd_font.py` from Liberation Sans Bold
  2.1.5; see LICENSE_ICONS and LICENSE_LIBERATION.

A file of the same name in `vkbd/` of the settings folder is drawn instead of the built-in
one, and again each time it is saved.
