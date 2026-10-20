# FreeType in Xpeccy+

The on-screen keyboard draws its lettering with FreeType, built from this copy on every
platform so the keyboard looks the same everywhere (on Linux the copy is linked with
`--exclude-libs,ALL`, so Qt keeps the system's FreeType).

- Release: FreeType 2.14.3, `freetype-2.14.3.tar.xz` from
  https://download.savannah.gnu.org/releases/freetype/, sha256
  `36bc4f1cc413335368ee656c42afca65c5a3987e8768cc28cf11ba775e785a5f`, signature checked
  against Werner Lemberg's key `E306 7470 7856 409F F194 8010 BE6C 3AAC 63AD 8E3F`.
- Kept, unchanged: `LICENSE.TXT`, `docs/FTL.TXT`, `docs/GPLv2.TXT`, `include/`, and the
  modules used, whole: `src/base`, `src/sfnt`, `src/truetype`, `src/autofit`,
  `src/smooth`. Everything else of the release is left out.
- Ours: `xpeccy/freetype/config/ftoption.h` (the release's, with the options marked
  "xpeccy" turned off: compressed and variable fonts, PostScript names, SVG, Mac fonts, the
  `FREETYPE_PROPERTIES` variable, CJK/Indic hinting) and `xpeccy/freetype/config/ftmodule.h`
  (TrueType, sfnt, autofit, smooth). `xpeccy/` comes first on the include path, so these
  stand in for the ones under `include/`.
- Subpixel text is FreeType's default (Harmony) LCD rendering:
  `FT_CONFIG_OPTION_SUBPIXEL_RENDERING` stays off.
- License: the FreeType License (`docs/FTL.TXT`), credited in `LICENSE_FREETYPE`.

To update: unpack the new release, replace the kept files, carry the "xpeccy" changes over
to the new `ftoption.h`, check the keyboard against the old one.
