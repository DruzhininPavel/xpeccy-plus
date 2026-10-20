esxDOS v0.8.6
(c) 2005-2018 Papaya Dezign
---------------------------

This version has the following (notable) new features (for detailed info checkout the changelog):

* 128K BASIC Support: For now only using a specific ROM (check the changelog) and still very experimental (use .128 command to test).
* Auto-BOOT Support: Supports loading /SYS/AUTOBOOT.BAS on cold/warm boot (configured in /SYS/CONFIG/ESXDOS.CFG).
* TR-DOS Navigator: If 'AlwaysBoot=1' is set in TRDOS.CFG, TR-DOS Navigator will be loaded whenever there is no BOOT file in the TR-DOS disk.
* Beta 48 (CAS and CBI) Support: Experimental and activated by setting 'Beta48=1' in TRDOS.CFG.
* New/Updated Commands: .playtfm, .playpt3, .ls, .tapein, .tapeout, .partinfo, .launcher, .rm, .hexview, .drives, .mktrd and .128

How to (safely) install/upgrade:
--------------------------------

1. Copy esxide.tap/esxmmc.tap (depending on if you have a DivIDE or DivMMC, obviously) to your CF/SD/HDD, load it and follow the instructions. Power Off and remove media.
2. Copy the SYS, BIN (and create /TMP if using DivIDE) directories to your CF/SD/HDD.
3. Insert media. Power On. Reset while keeping space pressed to reinit if needed.

Notes:
------

a) If you replace the SYS directory *before* flashing the new ROM, your previous ROM of esxDOS will not boot correctly.
b) Questions? Bugreports? Complaints about lack of LFN/instructions? Ask on the usual places or send an e-mail to: bugs at esxdos dot org

Changelog
---------

[12/06/2013] auto-LOAD: Changed auto-LOAD mechanism to be more transparent, and fixed bug that affected SQ-Tracker, RETURN from TR-DOS (lordcoxis + reported by Factor 6/Velesoft)
[12/06/2013] TR-DOS: Fixed a bug that affected TR-DOS Navigator (code version), related to TR-DOS version detection (lordcoxis + reported by Velesoft)
[14/06/2013] NMI: Fixed bug when pressing NMI without system loaded (lordcoxis + reported by Velesoft)
[14/06/2013] TR-DOS: Added a workaround to make Outrun Europa .TRD load as it calls 48k ROM from interrupt (lordcoxis + reported by Ilyad)
[20/06/2013] TR-DOS: Added a few missing routines to add more compatibility (lordcoxis based on several bug-reports)
[20/06/2013] TR-DOS: Added a new option "AlwaysBoot", which can be set to enable the loading of a custom TR-DOS Boot file when none is present in the disk. Included TR-DOS Navigator (code version) with the authors permission (lordcoxis + thanks to Velesoft)
[25/06/2013] TR-DOS: Fixed long delay when detecting drive/disk using #3d2f calls (lordcoxis)
[08/07/2013] NMI: Changed 128kB RAM bank detection to use random bytes instead of a fixed string (lordcoxis + reported by Velesoft)
[08/07/2013] Kernel: Fixed a crash when a .command returned to BASIC with a custom error string (lordcoxis + reported by John Barker)
[09/07/2013] NMI: Fixed a bug in NMI entry which could corrupt AF and R in some circumstances (lordcoxis)
[09/07/2013] NMI: TR-DOS paged byte was being set with an invalid value in 128kB snapshots, which led to the snapshots not loading on certain emulators (ub880d + reported by ZXHistory on WOS)
[13/07/2013] TR-DOS: Added experimental Beta 48 (CAS and CBI) support (lordcoxis + thanks to Flavio Matsumoto)
[21/09/2013] FAT Driver: Fixed a bug in the handling of FAT32 volumes - when the last cluster was set to $FFFFFFFF in the FSINFO sector, writing was not possible (lordcoxis)
[21/11/2013] Kernel: Added RTC.SYS support (lordcoxis)
[13/08/2016] auto-LOAD: Added POKE 23388,16 to auto-LOAD syscall to improve 128k compatibility (lordcoxis + reported by ub880d and velesoft)
[30/06/2017] Kernel: Removed v6Z80P Turbo I/O routine as it caused COVOX interference (lordcoxis + reported by various)
[07/10/2017] BASIC: Loading a BASIC program sometimes wouldn't clear the existing one (lordcoxis + reported by Andrew Owen)
[07/10/2017] auto-LOAD: Fixed FLAGS variable (ub880d + reported by ellvis)
[07/10/2017] Kernel: Fixed wrong calculation of parameters pointer in M_EXECCMD (lordcoxis + reported by ub880d)
[10/10/2017] FAT Driver: Fixed carry flag set on return from F_FGETPOS (lordcoxis + reported by ub880d)
[17/02/2018] Commands: Added .128 command to test 128K mode using Velesoft's "zx128rom-for-divide" ROM (lordcoxis)
[17/02/2018] auto-BOOT: Added preliminary auto-BOOT support (lordcoxis)
[07/03/2018] Commands: Added .playpt3 (replaces old command since this one also plays 6 channel tunes) and .playtfm commands (tygrys)
[07/03/2018] Kernel: M_EXECCMD was still broken when there were no parameters for executed command (ub880d)
[07/03/2018] auto-BOOT: Caps Shift changes the autoboot setting with a XOR 3 (lordcoxis + suggested by Carlos Sanchez)
[07/03/2018] Kernel: Added possibility to use full path to command in M_EXECCMD if path begins with "/" (ub880d)
[09/03/2018] Commands: Added .rm, .hexview and .drives commands (Dr. Slump)
[09/03/2018] Commands: Added improved .ls, .tapein, .tapeout, fixed .partinfo and new .launcher (ub880d)
[09/03/2018] Commands: Added .mktrd (laesq)
[10/03/2018] Commands: Added improved .partinfo (ub880d)
[12/05/2018] auto-BOOT: Load /SYS/AUTOBOOT.BAS directly instead of changing CWD to /SYS (lordcoxis + reported by various)
[13/05/2018] FAT Driver: Fixed a long standing bug, where the last directory entry created in a sector was corrupted (lordcoxis + reported by Carlos Sanchez)
[13/05/2018] NMI: Added empty TMP folder to archive, for lazy people ;) (lordcoxis)
[15/05/2018] FAT: Clear whole clusters when creating directory entries, instead of just the next sector (lordcoxis/ub880d + reported by cygnus, z00m)

Thanks/Greets
-------------

Greets to all the usual peeps (y'all know who you are - just check v0.8.5's README if in doubt!).
Special thanks to all the great "hardware benefactors": Ben Versteeg, Pavel Urbancik, SpecNext Team, Tygrys, UB880D, Zaxon!

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

18.05.2018 / Papaya Dezign - All rights perversed
