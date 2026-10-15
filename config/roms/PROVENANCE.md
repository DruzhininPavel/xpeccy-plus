# Provenance of the bundled ROM images

Every file in `config/roms/` is listed here. Nothing else is bundled, and no image
was modified - the copyright messages inside them are intact. See `LICENSE` in this
directory for the distribution terms, and `AMSTRAD.copyright` for the canonical
Sinclair/Amstrad notice (copied verbatim from the Fuse emulator; it also mentions a
few images that are not bundled here).

File names follow Fuse where Fuse carries the same image, so a set can be compared
file by file with `fuse-extra-roms`. Images that Fuse does not carry keep a plain
name of their own.

MD5 is given so a rebuild of the bundle can be checked against this table.

## Tier 1 - explicit permission

Copyright Amstrad, who allow distribution of the ROMs but retain the copyright.

| File | Machine | Rights holder | Status | Note |
| --- | --- | --- | --- | --- |
| `48.rom` | ZX Spectrum 48K | Amstrad | permitted | `4c42a2f075212361c3117015b107ff68` |
| `128-0.rom` | ZX Spectrum 128K | Amstrad | permitted | `b4d2692115a9f2924df92a3cbfb358fb` |
| `128-1.rom` | ZX Spectrum 128K | Amstrad | permitted | `6e09e5d3c4aef166601669feaaadc01c` |
| `plus2-0.rom` | ZX Spectrum +2 | Amstrad | permitted | `4ed7af4636308b8a48d7a35e6c5b546b` |
| `plus2-1.rom` | ZX Spectrum +2 | Amstrad | permitted | `b3db95931cc844efaeb82db9c171b9f3` |
| `plus3-0.rom` | ZX Spectrum +2A / +3, v4.0 | Amstrad | permitted | `9833b8b73384dd5fa3678377ff00a2bb` |
| `plus3-1.rom` | ZX Spectrum +2A / +3, v4.0 | Amstrad | permitted | `0f711ceb5ab801b4701989982e0f334c` |
| `plus3-2.rom` | ZX Spectrum +2A / +3, v4.0 | Amstrad | permitted | `3b6dd659d5e4ec97f0e2f7878152c987` |
| `plus3-3.rom` | ZX Spectrum +2A / +3, v4.0 | Amstrad | permitted | `a148bcc575e51389e84fdf5d555c3196` |
| `plus3-41.rom` | ZX Spectrum +3, v4.1 | Amstrad | permitted | later official revision, all four 16K pages in one file, `7e00ed3562abfd188d0d4da03e80bc0a` |

Source: `48.rom` .. `plus3-3.rom` taken from
<https://github.com/trufanov-nok/fuse-extra-roms>. `plus3-41.rom` comes from a
long-circulating dump of the v4.1 ROM set, which Fuse does not carry.

## Tier 2 - derivative of Amstrad code, attributed

| File | Machine | Rights holder | Status | Note |
| --- | --- | --- | --- | --- |
| `128p-0.rom` | Pentagon 128 | Amstrad + unknown authors of the Pentagon patch | derivative, distributed with every ZX emulator | `a249565f03b98d004ee7f019570069cd` |
| `128p-1.rom` | Pentagon 128 | Amstrad | permitted | byte-identical to `128-1.rom` (the 48 BASIC page), `6e09e5d3c4aef166601669feaaadc01c` |

Source: <https://github.com/trufanov-nok/fuse-extra-roms>.

## Tier 3 - clone firmware, derivative or unclear terms

Firmware of Soviet / post-Soviet ZX clones. The 128 BASIC part inside them descends
from the Amstrad code, the rest was written by the clone authors. No formal license
was ever published for any of these; they have been distributed with the machines
and with every emulator of them for decades.

| File | Machine | Rights holder | Status | Note |
| --- | --- | --- | --- | --- |
| `256s-0.rom` | Scorpion ZS 256 | Sergey Zonov / ZS Scorpion authors | grey, distributed by convention | `b9fda5b6a747ff037365b0e2d8c4379a` |
| `256s-1.rom` | Scorpion ZS 256 | as above | grey | `643861ad34831b255bf2eb64e8b6ecb8` |
| `256s-2.rom` | Scorpion ZS 256 | as above | grey | service ROM, `d8ad507b1c915a9acfe0d73957082926` |
| `256s-3.rom` | Scorpion ZS 256 | Technology Research Ltd | grey | TR-DOS page of the set, `ce0723f9bc02f4948c15d3b3230ae831` |
| `scorpion295.rom` | Scorpion ZS 256 | Scorpion / MOA | grey, given away by the maker | ROM 2.95 of 26.11.1994, 64K, CRC32 99F57CE1 (the image others call 2.94), `23add41c9c4a3f7cca845d3d5cf51367` |
| `prof39f.rom` | Scorpion ZS 256 | ProfROM authors | grey | ProfROM 3.9f, 128K, `dcb8ebbe2d2f4c4afa58c43507987b9c` |
| `prof401.rom` | Scorpion ZS 256 Turbo+ | MOA (Andrew Larchenko) / Scorpion | grey | ProfROM 4.01 of 13.07.1997, 256K, ROM disk "FirmWare BonusPak", CRC32 847A66E4, `b124c52cd462ce36d79086747115896f` |
| `profi-kramis02.rom` | Profi v3 | JV "Kramis" / Kondor | grey | the v3 boards' own: BIOS JV "KRAMIS" V.02 of 10.1990 with TR-DOS 5.03, CRC32 77327F52, `720f23bec22581f37fb55bd7a928fde2` |
| `profi-bios10.rom` | Profi v5 | Micco Software / Kondor | grey | ROM Bios 1.0 of 21.09.93, read off a Kondor 5.04 board; TR-DOS 5.04T, CRC32 10DA289A, `f4ab0dd91cd7d207879767d4fe5bf30e` |
| `profi-bios20.rom` | Profi v5 | Micco Software / Kondor | grey | ROM Bios 2.0 of 17.04.94, TR-DOS 5.04T, CRC32 36F5F7BD, `02877e403f22d10d12ef0296ccb96f60` |
| `atm2.rom` | ATM Turbo 2+ | MicroART | grey | `28ce89a88089417db4d3057de942a1bb` |
| `zxevo-fe.rom` | ZX Evo (BaseConf) | NedoPC group | grey | EVO Reset Service 0.61 FE, `4d440af2f2d746094527aa6789390705` |
| `tsconf.rom` | ZX Evo (TSConf) | TS-Labs | grey | TS-BIOS, shipped elsewhere as `ts-bios.rom`, `d5f199df3832dc749fe0d12f1ce8f26f` |
| `phoenix.rom` | ZXM-Phoenix | ZXM-Phoenix authors | grey | BIOS 5.03, `892a393093f373ad3b4c9453f529b523` |
| `alf.rom` | ALF TV Game | SKB Zapad, Brest | grey | 32K: games menu in bank 0, 48 BASIC in bank 1, `f879e22ae833687ccdfc4745aa56b804` |

Source: `256s-*.rom` from <https://github.com/trufanov-nok/fuse-extra-roms>; `alf.rom`
from <https://zxbyte.ru/alf.htm>, which publishes it as a dump of the console's 27C256;
`scorpion295.rom` is `ROM2_95.512` from the maker's own download,
<https://web.archive.org/web/20060812213917/http://www.scorpion.ru:80/spectrum/files/rom2_95.zip>;
the "2.95" circulating as `scorp295.rom` (CRC 0C6C1EF6) is a different image.
`profi-*.rom` from <https://speccy4ever.speccy.org/_PR.htm> (`PJV02`, `PB10`, `PB20`, named
there by CRC). The `profi.rom` shipped before was V.02's BIOS page with someone's TR-DOS 5.04T
in place of the 5.03 the board came with.
`prof401.rom` from <https://speccy4ever.speccy.org/_SC.htm>, the CRC MAME's `scorpiontb` lists;
it is the last ProfROM MOA built for the real board (4.02 runs only in MOA's own emulator, and
the 4.xx.0NN builds are a later community fork). The rest from the images that have circulated
with these machines and with Xpeccy itself.

`alf.rom` is the original firmware, not the patched one that also circulates: it checks
it is running on a genuine console and prints "Не укради" if it is not, and this
emulator passes that check - which makes it a better image to ship and a useful test.

## Tier 4 - rights holder defunct or unknown, bundled by convention

| File | Machine | Rights holder | Status | Note |
| --- | --- | --- | --- | --- |
| `trdos.rom` | Beta Disk / TR-DOS 5.03 | Technology Research Ltd (UK) | proprietary, holder long gone | last official release, used by the Sinclair profiles; shipped by Fuse and ZEsarUX as well, `0da70a5d2a0e733398e005b96b7e4ba6` |
| `trdos504t.rom` | Beta Disk / TR-DOS 5.04T | Technology Research Ltd + unknown patch authors | as above | the version the clones actually shipped with, used by the Pentagon profiles, `b4c9634312b796063015450daef13dfa` |
| `gluck.rom` | Pentagon service ROM | Gluck service ROM authors | unknown | `d5869034604dbfd2c1d54170e874fd0a` |
| `sgen.rom` | ATM Turbo 2+, ZX Evo | ATM / General Sound authors | unknown | 2K character generator for the text mode, `bd4e0f6f9177f38a71f081882ba3c454` |
| `gs104.rom` | General Sound 1.04 | General Sound authors | unknown | `6cb34f369d32b2788bd631937b1cf3eb` |
| `gs105a.rom` | General Sound 1.05a | General Sound authors | unknown | `1801e94780ad2a914b070b7128d86c39` |
| `gs105b.rom` | General Sound 1.05b | General Sound authors | unknown | latest release, used by the bundled romsets, `c6244f36ce2e782d4a4be9d72b0b4dd4` |
| `gs105a.txt` | - | General Sound authors | unknown | release notes shipped with the firmware |
| `gs105b.txt` | - | General Sound authors | unknown | release notes shipped with the firmware |

The General Sound firmware sources are published at
<https://github.com/psbhlw/gs-firmware> (sources only, no built images); the binaries
bundled here are the original firmware releases.

## esxDOS - permission of the rights holder

`../esxdos/` holds esxDOS 0.8.9 by Papaya Dezign, the divMMC firmware (`ESXMMC.BIN`, the
image its EEPROM starts with) and the `SYS` and `BIN` folders a card needs, plus the
release's `README.TXT` and `ESX089.NFO` - unmodified, from the official
<http://www.esxdos.org/files/esxdos089.zip>. Bundled with the permission of Papaya Dezign;
thanks to Miguel Guerreiro / Papaya Dezign. Its terms are in `../esxdos/LICENSE`.

| File | Size | MD5 |
| --- | ---: | --- |
| `ESXMMC.BIN` | 8192 | `fa50b0258e52b8d72bd83cc2fb6e1013` |
| `BIN/128` | 512 | `9632dc64cbe81eb10b2028a925621dae` |
| `BIN/CAL` | 603 | `5b858161928fc91e6563c25e67a7d5c5` |
| `BIN/CD` | 512 | `50c40f96a11161cbad1b312d7517e8e3` |
| `BIN/CHMOD` | 512 | `fb04875adc42097837b7839432f5103c` |
| `BIN/CP` | 512 | `1e5a00d1a8acb2f5d46f8ff1b1347e80` |
| `BIN/DATE` | 388 | `49281f0d780bcee65a8697d54ea12549` |
| `BIN/DFWLOAD` | 1024 | `6c9fec59ad152fb55db55b3fc20f49c1` |
| `BIN/DIVIDEO` | 917 | `1e3a152f884d9a310496870a3bc58766` |
| `BIN/DRIVES` | 1008 | `9a8bc288f48b399974e5676a1dbd6bdc` |
| `BIN/DSKPROBE` | 2837 | `d5336db574873f68edbfdbec72c2f404` |
| `BIN/DUMPMEM` | 512 | `e9d02ffab7560f07adfc0739a1e5baf0` |
| `BIN/FILE` | 1024 | `3dc7b278ed9f52ff6df9122066149791` |
| `BIN/GRAMON` | 2560 | `930ec960019d5802b957d591ddcd7164` |
| `BIN/GSC` | 4896 | `2e4f5e414e0b620f2ce566e5a32afeb6` |
| `BIN/HEXDUMP` | 512 | `cd4f68ae03402de1a1299fcc91edde83` |
| `BIN/HEXVIEW` | 2767 | `489543ce2372ebe07e3f292379660f7c` |
| `BIN/LAUNCHER` | 1160 | `76a10020fffc4f672690f5496baaadfa` |
| `BIN/LS` | 1189 | `8ceef180fe87f9a0de000721b7e01fc0` |
| `BIN/LSTAP` | 1024 | `90c937f5d8e847b286dfb5fbb9dd850d` |
| `BIN/MKDIR` | 512 | `7c2505a761948bf221f5083924eff968` |
| `BIN/MKTRD` | 512 | `c9449b0a8cb9b3ad7f6f11123681bb50` |
| `BIN/MORE` | 417 | `f414a55ee44f00286a50a231bd29054a` |
| `BIN/MV` | 512 | `48ebee514da2b7e803fafa72507a5fe6` |
| `BIN/PARTINFO` | 512 | `a35fbbf8a4b54e5dfbb4604a0dcb5b36` |
| `BIN/PLAYPT3` | 3642 | `bfabb96b23412808f561e7666b1811a5` |
| `BIN/PLAYSQT` | 1759 | `db65aad44667f1d3bd606abcf97e1ff6` |
| `BIN/PLAYSTC` | 1475 | `fadead590fd9b02f32e20c1fd35664b7` |
| `BIN/PLAYTFM` | 2232 | `c2fc51b9e118a57423bc9cf925908be8` |
| `BIN/PLAYWAV` | 1536 | `9d617d24638646cc43cccba9f2c8c445` |
| `BIN/RM` | 1289 | `b6d06591057859e061ac0a382f6ab3e8` |
| `BIN/SCL2TRD` | 4075 | `601cd775f8646cdcaaeaf7d0e63b3e05` |
| `BIN/SERCP` | 3072 | `94673dbd6a9d4275d99f7f9f15f0bff3` |
| `BIN/SNAPLOAD` | 6656 | `e587f0ad0e05eb9c6aa0da38f53e2b24` |
| `BIN/SPEAKCZ` | 1536 | `cd3b8cf7d0daa65051cb2877ef1a7031` |
| `BIN/STRINGS` | 1045 | `e4d1d1ff1608736d82a07d7ea9442418` |
| `BIN/TAPEIN` | 512 | `5db0a65f475f9d9eb91b0eda452bae8e` |
| `BIN/TAPEOUT` | 512 | `b30b9f2fd6fdc78eed514fb15dcb3bfd` |
| `BIN/TAR` | 2691 | `dcab1baa4907675ee3a845986fd53685` |
| `BIN/TE` | 1024 | `bc73a3c7299b9e57a74b49a698343f49` |
| `BIN/TSPLAY` | 3584 | `bbefe2eefd9adac8c6051741ce1977ba` |
| `BIN/VDISK` | 512 | `3fb12c5773ae39c77a47c8d8bf9b077b` |
| `ESX089.NFO` | 2965 | `8a17f0a54c0d1af43d38be5d14e1100c` |
| `README.TXT` | 4320 | `740928f5527378845ef25b49c0bb1e18` |
| `SYS/BDIR.KO` | 1024 | `d1a602cbc49f2200c0de7e9f855894eb` |
| `SYS/BETADISK.SYS` | 2233 | `64a7ea5a4690c76bedcaed67874a0505` |
| `SYS/BFILE.KO` | 1024 | `1f6c192703e8ef104f4efe5831eb042d` |
| `SYS/CONFIG/ESXDOS.CFG` | 247 | `0eea6ed61b759a6c7a8285d2859c0bdf` |
| `SYS/CONFIG/TRDOS.CFG` | 875 | `3e4870421391bdf18759eab6ae77aa3b` |
| `SYS/ERRMSG.KO` | 512 | `62a5a9dd6ac8d1e048baa70073b7fecd` |
| `SYS/ESXDOS.SYS` | 4090 | `484135f5803e399e09ef01b290f2f39e` |
| `SYS/NMI.SYS` | 3539 | `7744e8f88ad5a0a38ae16132cdcbca1c` |
| `SYS/NMI_HELP.SCR` | 6912 | `7c1fbd5d11172b79a8042334479a34c4` |
| `SYS/TAPE.KO` | 1024 | `0dc2206e6ceaffa0810e7f99e2ac7c9a` |
| `SYS/TRDBOOT.BIN` | 15360 | `313590e963124f498a5b3763aea66c7d` |
| `SYS/TRDOS54T.KO` | 16412 | `2e0ef5b18368c4d432ec983cb34c6616` |
| `SYS/VDISK.KO` | 512 | `7f8f7ed83b6c7ad1eee4aadc27c68f22` |

## Other files

| File | What it is |
| --- | --- |
| `AMSTRAD.copyright` | canonical Amstrad notice, verbatim copy from the Fuse distribution |
| `LICENSE` | distribution terms for this directory |
| `PROVENANCE.md` | this file |
| `../esxdos/LICENSE` | terms of the bundled esxDOS |
| `../boot.$B` | Dimon boot 2024 by Dmitry Yurinov, appended to TR-DOS images that carry no boot file - <https://zxart.ee/prod/540777> |
