esxDOS v0.8.8
(c) 2005-2020 Papaya Dezign
---------------------------

This version has the following (notable) new features (for detailed info check the changelog):

* BIN variant added to SAVE command
* Variable support in GO TO command
* Parity check is disabled by default when loading .TAPs
* New/Updated Commands: .sercp, .scl2trd, .cal, .date, .strings

How to (safely) install/upgrade:
--------------------------------

1. Copy esxide.tap/esxmmc.tap/mb03plus.tap (depending on if you have a DivIDE, DivMMC or MB03+, obviously) to your CF/SD/HDD, load it and follow the instructions. Power Off and remove media.
2. Copy the SYS, BIN (and create /TMP if using DivIDE) directories to your CF/SD/HDD.
3. Insert media. Power On. Reset while keeping space pressed to reinit if needed.

Notes:
------

a) If you replace the SYS directory *before* flashing the new ROM, your previous ROM of esxDOS will not boot correctly.
b) Questions? Bugreports? Complaints about lack of LFN/instructions? Ask on the usual places or send an e-mail to: bugs at esxdos dot org

Changelog
---------

[23/02/2020] MB03+: Sped up CheckDMA routine using hardware LIFO stack (lordcoxis)
[23/02/2020] NMI: Added SAA1099 silencer on NMI entry (lordcoxis)
[04/04/2020] NMI: Silence (up to) 3x AYs on NMI entry (lordcoxis)
[04/04/2020] NMI: Added ULAPlus pallette reset on entry and restore on exit (lordcoxis)
[05/04/2020] BASIC: Added variable support in GO TO command, ie "GO TO a$" and "GOTO hd1;a$" (lordcoxis + requested by SCjoe)
[06/04/2020] Kernel: Fixed bug in M_GETSETDRV related with SYS ("$") drive (lordcoxis + reported by Dr. Slump)
[07/04/2020] BASIC: Added BIN variant to SAVE command - like CODE but doesn't write a header (lordcoxis + requested by SCjoe)
[10/04/2020] Commands: Fixed .128 command which got broken in v0.8.7 (lordcoxis + reported by nagydani)
[10/04/2020] Tape Emulator: Added "ParityCheck" parameter to ESXDOS.CFG, which determines if parity is checked when loading .TAPs (lordcoxis + requested by SCjoe)
[10/04/2020] Commands: Added .scl2trd command (Nihirash)
[01/05/2020] Commands: Updated .sercp to v0.5 (Pavel Vymetálek)
[23/05/2020] NMI: Fixed AY silencer routine (lordcoxis + reported by LMN128 & SCjoe)
[23/05/2020] Kernel: Fixed another infinite loop when ESXDOS.SYS can't be loaded (lordcoxis + reported by UB880D)
[23/02/2020] NMI: Added GS silencer on NMI entry (lordcoxis + help from moroz1999)
[24/05/2020] Commands: Added .cal, .date and .strings commands (UB880D)
[24/05/2020] MB03+: Added special MB03+ esxDOS logo (K-0s)


Thanks/Greets
-------------

Greets to all the usual peeps (y'all know who you are - just check v0.8.5's README if in doubt!).
Special thanks to all the great "hardware benefactors": Ben Versteeg, LMN128, Pavel Urbancik, Pavel Vymetalek, SpecNext Team, Tygrys, UB880D, Zaxon!

Known Bugs
----------

128K Support: Need to run an esxDOS BASIC command before .commands will work
BASIC/Tape Emulator: You can overwrite ESXDOS system using LOAD CODE that crosses $2000
Commands: Proper argument/syntax checking is not done yet on most commands
Commands/BASIC: No wildcards yet
FAT Driver: rename() isn't even remotely posix compliant
FAT Driver: Directories read-only attribute isn't always respected
BIOS: Version of system file isn't checked against the ROM version (bad things will happen if there's a mismatch)
TR-DOS: FORMAT and MOVE commands don't work yet + too many to list ;)

24.05.2020 / Papaya Dezign - All rights perversed
