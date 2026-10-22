# FatFs R0.15a

ChaN's FAT file system module (http://elm-chan.org/fsw/ff/), the guest of
`tests/core/vfat`: it drives a folder volume through its sectors as a Spectrum's
DOS would. Used by the bench only, never by the app. License: `LICENSE.txt`, a
1-clause BSD.

Taken from `ff15a.zip` as released, but for:

- `ffconf.h`: long names on (`FF_USE_LFN 1`, UTF-8 API `FF_LFN_UNICODE 2`), code
  page 866 for 8.3 names, `f_mkfs`, `f_expand`, `f_utime` and labels on, no RTC.
- `ffunicode.c`: the DBCS tables (932, 936, 949, 950) left out, 2 MB of the
  file's 2 MB; the bench is built for code page 866 only.

`diskio.c` and `ffsystem.c` are not taken: the bench has its own disk functions,
and with a static long name buffer and no reentrancy FatFs needs no system ones.
