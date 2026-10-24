# Disks and drives

```{index} disk
```

A machine can have floppy drives, hard disks, an SD card or a cartridge slot, depending on
the hardware it copies. A divMMC can be added to the Sinclair models and the Pentagons. The
**Media** menu lists only what the running machine has.

## Which machine has what

```{index} Beta Disk, TR-DOS, +3DOS, disk interface, NemoIDE, SMUC
```

| Machine | Floppy | Hard disk | Other |
|---|---|---|---|
| ZX Spectrum 48K | - | - | cartridge, divMMC |
| ZX Spectrum 128K, +2, +2A | - | - | divMMC |
| ZX Spectrum +3 | +3 drives | - | divMMC |
| ZX Spectrum Next | - | - | SD card, divMMC built in |
| Pentagon, Pentagon 1024 SL | Beta Disk | - | divMMC |
| Scorpion ZS 256 | Beta Disk | - | |
| Scorpion ZS 256 Turbo+ | Beta Disk | SMUC | |
| ATM Turbo 2+ | Beta Disk | ATM Turbo | |
| Profi v3 | Beta Disk | - | |
| Profi v5 | Beta Disk | Profi | |
| ZX Evolution, both | Beta Disk | NemoIDE (ZX Evo) | SD card |
| ZXM-Phoenix | Beta Disk | - | |
| ALF TV Game, ALF TV Game 128K | - | - | cartridge |

**Beta Disk** is the TR-DOS disk interface with up to four drives, A to D. The **+3**
interface has two drives, A and B, and runs +3DOS.

A machine with no disk interface of its own can be given one. Open the **Machine** page of
**Options** and pick it in the **Disk** row: **None**, **Beta Disk (WD1793)** or
**+3 (uPD765)**. The **Hard disk** row does the same for an IDE interface: **NemoIDE**,
**NemoIDE A8**, **NemoIDE (ZX Evo)**, **SMUC**, **ATM Turbo** or **Profi**. An interface that
is part of the machine's board cannot be changed: the list is grayed out, and its tooltip
says **Built into this board**. A Beta Disk cannot be added to the +2A and +3.

## Floppy disks

### Putting a disk in

```{index} floppy disk, Floppy Drives, Write Protect
```

```{index} see: floppy; floppy disk
```

Open a disk image with **File > Open...**, or drop it onto the window: it goes into drive A
(see [Run a program](getting-started.md#run-a-program)). To put it into another drive, pick
**Disk B**, **Disk C** or **Disk D** as the file type in the open dialog.

![Media > Floppy Drives](images/menu-floppy.png)

**Media > Floppy Drives** has a submenu for each drive the machine has. The submenu is named
after the disk in the drive, or **(empty)**; the drive letter is on its icon. A star after
the name means the disk has been changed and not saved yet. Each submenu has:

- **Open...** - put a disk image into this drive. It is only mounted: nothing starts, and
  the machine stays the same.
- **New** - put an empty disk into this drive. On a Beta Disk machine you are asked
  **Format for TR-DOS?**: {icon}`ok-apply` **Yes** gives an empty TR-DOS disk ready for
  files, {icon}`cancel` **No** gives an unformatted one, for a program that formats it
  itself.
- **Save** - write the disk back to the file it came from.
- **Save As...** - write the disk to a new file; you pick the name and the format.
- **Eject** - take the disk out.
- **Write Protect** - the machine cannot write to this disk, as with the tab on a real one.

A right-click on a drive's {icon}`diskGreen` light in the status bar opens the same menu.

### Disk formats

```{index} TRD, SCL, UDI, DSK, TD0, FDI, IMA, Hobeta, RAW file to disk
```

| Format | Opens | Saves | What it is |
|---|---|---|---|
| `.trd` | yes | yes | TR-DOS disk, sector by sector |
| `.scl` | yes | yes | TR-DOS files packed together without the empty space |
| `.udi` | yes | yes | the whole disk track by track, as the drive reads it |
| `.dsk` | yes | yes | +3 disk (extended DSK too) |
| `.td0` | yes | - | Teledisk image |
| `.fdi` | yes | - | FDI image |
| `.ima`, `.img` | yes | - | 1.44 MB PC floppy |
| `.$B`, `.$C`... | yes | - | Hobeta: one TR-DOS file |

A **Hobeta** file is not a disk but a single TR-DOS file. Opening it into a drive adds the
file to the disk that is there; an empty drive gets a new TR-DOS disk with the file on it.

Any other file can be opened as **RAW file to disk**: it is copied onto the TR-DOS disk in
the drive as one file, named after the host file. A file longer than 65280 bytes is split
into several parts.

### The boot file

```{index} boot file, Add boot loader
```

Most TR-DOS disks start with a file named `boot`, which the machine runs when it boots from
the disk. When a TR-DOS disk has no `boot` of its own, Xpeccy+ adds one as the disk is opened,
so the disk starts by itself and shows a menu of its files. Adding this file does not count
as a change to the disk, and the image file is not touched unless you save the disk.

To stop this, open the **Machine** page of **Options**, click the {icon}`settings` gear in
the **Disk** row and clear **Add boot loader**. The file added is `boot.$B` in the
[settings folder](getting-started.md#where-the-settings-are); put another Hobeta file there
under the same name to use your own.

### Drive settings

```{index} Machine: Disk, Interleave, Number of drives, TR-DOS ROM
```

The {icon}`settings` gear in the **Disk** row of the **Machine** page opens
**Machine: Disk**:

- **Fast disk access** - no waiting for the head to move or the disk to turn; see below.
- **Add boot loader** - see [The boot file](#the-boot-file).
- **TR-DOS ROM** - which ROM image the Beta Disk runs.
- **Interleave** - the order of sectors on each track of a `.trd` or `.scl` image, set when
  the image is opened. TR-DOS formats its own disks as 1, 9, 2, 10...
- **Number of drives** - from one (A) to four (A to D); a +3 has two at most. A drive left
  out is not there at all, and programs find it missing.
- **Drives** - for each drive, **80 cylinders** and **Double side**.

### Fast Disk Access

```{index} Fast Disk Access
```

Real drives spend time moving the head and waiting for the sector to come around.
**Media > Fast Disk Access** skips those waits, so disks load much faster. The same switch
is in the **Disk Manager** and in **Machine: Disk**.

### Changed disks

```{index} disk; saving, Save Changed Disks, Remember mounted media
```

What the machine writes to a disk stays in memory: the image file changes only when you
save the disk.

**File > Save Changed Disks** asks about each drive whose disk was changed:
**Disk A: game.trd has been changed. Do you want to save it?** **Yes** writes the disk to its
file, **No** goes on without saving it, **Cancel** stops asking. For a new disk that has no
file yet you are asked for a file name. The same happens for a disk opened from a format
Xpeccy+ reads but does not write, such as `.td0`, `.fdi` or `.ima`. When a save fails, a
message says why, and the disk stays changed.

Before a changed disk is ejected or replaced, and when you close Xpeccy+, you are asked the
same question. **Cancel** keeps the disk in the drive, or keeps the program open.

With **Remember mounted media** on the **File types** page of **Options**, the disks in the
drives are opened again at the next start.

## Disk Manager

```{index} Disk Manager, Copy to tape, Save as Hobeta, Save as RAW
```

![The Disk Manager](images/disk-manager.png)

**Media > Disk Manager** (also {icon}`fdd_disk` on the toolbar, and a click on a drive
{icon}`diskGreen` light in the status bar) shows what is in the drives and on the disks. In
the picture drive A holds Fire 'n Ice (n-Discovery, 2003).

- **Tabs** - one for each drive, named after the disk's own TR-DOS label or the file name.
  **Fast disk access** beside them is the same switch as in the **Media** menu.
- **The line under the tabs** - the file in the drive, then buttons to {icon}`fileopen` open
  a disk image, {icon}`doc-new` insert a new disk, {icon}`save_all` save to the file it came
  from, {icon}`floppy` save as, and {icon}`tape-eject` eject, and the **Write protect** box.
  They do what the **Floppy Drives** menu does.
- **The catalog** - the files on a TR-DOS disk: **Name**, **Ext**, **Start**, **Length**,
  **SecLen** (length in sectors), **Trk** and **Sec** (where the file begins). A +3DOS disk
  has columns of its own (see below); any other disk shows no catalog.
- **The line at the bottom** - the disk's label, how many files and deleted files it has,
  free sectors, and **modified** when it has unsaved changes. While the drive's motor runs,
  **Head on track** shows where the head is, and the file under it is shown in bold.

Select one or more files in the catalog (Ctrl or Shift for several) and use the buttons on
the right:

- {icon}`tape` **Copy to tape** - add the files to the tape in the [Tape Player](tapes.md),
  as BASIC programs or code blocks. A file whose length does not match the number of sectors
  it occupies (several files joined into one) is skipped.
- {icon}`dollar` **Save as Hobeta** - write each file to a folder you pick as a Hobeta file
  (`name.$C`).
- {icon}`filebin` **Save as RAW** - write each file's bare data, with no header (`name.C`).

The window is kept up to date as the machine runs: a file the program writes shows up in
the catalog straight away.

### +3DOS disks

```{index} +3DOS; Disk Manager, CPC disk
```

A +3 disk shows its files too, and so do the CPC system and data disks, which the +3 also
reads. The columns are **Name**, **Ext**, **Type** (**Program**, **Numbers**, **Chars** or
**Bytes**), **Start** (a program's autostart line, or the address of code), **Length**,
**Size** (the space the file takes on the disk), **Trk** and **Sec**. A file in a user area
other than 0 has its number before the name, as `3:GAME`. The line at the bottom says which
kind of disk it is, how many files it has and how much space is free.

The buttons work as on TR-DOS. {icon}`tape` **Copy to tape** and {icon}`dollar`
**Save as Hobeta** need the file's +3DOS header, which gives its type, address and length; a
file without one is skipped. {icon}`filebin` **Save as RAW** writes the data without the
header, or the whole file when it has none.

## SD card

```{index} SD card, --sdcard
```

The two ZX Evolution machines and the ZX Spectrum Next have an SD card slot. The Next
starts from a card with System/Next on it, and its menu has **System/Next Card...** too:
see [The SD card](zx-next.md#the-sd-card). **Media > SD Card** shows the card in
the slot, or **(no card)**, and has:

- **Open Image...** - insert a card from an image file.
- **Open Folder...** - use a folder on your computer as the card. See
  [A folder as a disk](#a-folder-as-a-disk).
- **Eject** - take the card out.
- **Write Protect** - the card's lock switch: the machine cannot write to the card. On a
  folder card it makes that one folder read only.

What the machine writes to a card image goes into the image file at once. To start with a
card, use `--sdcard <file or folder>` on the [command line](command-line.md).

## divMMC

```{index} divMMC, esxDOS, EEPROM, MAPRAM, Hard Reset
```

```{index} see: DivMMC EnJOY!; divMMC
```

The divMMC is an SD card interface for the ZX Spectrum 48K, 128K, +2, +2A and +3, and the
Pentagons take it on their ZX-BUS. The one emulated follows Mario Prato's original design,
which the boards sold since, such as the DivMMC EnJOY!, are built on: 8K of EEPROM for the
firmware, 128K or 512K of RAM, and two SD card slots. Its SD card port has a clock of its
own, so the card works at any usual CPU speed. Its firmware is esxDOS, which loads `.tap`,
`.sna`, `.z80` and `.trd` files from the card and adds dot commands to BASIC. esxDOS 0.8.5
to 0.8.9 come with the emulator, by permission of its authors, Papaya Dezign, and 0.8.9 is
in the board's EEPROM from the start.

### Setting it up

1. Switch the board on with **Media > divMMC**, or on the **Machine** page of **Options**
   set the **divMMC** row to **divMMC**. The {icon}`settings` gear opens **Machine: divMMC**:
   - **RAM** - **128K** or **512K**.
   - **EEPROM writable** - the board's EEPROM jumper. Leave it off. When it is on, the
     Spectrum can write new firmware into the EEPROM, and the firmware does not start at a
     reset.
   - **Firmware** - what the EEPROM holds: one of **esxDOS 0.8.9** to **esxDOS 0.8.5**,
     **(erased)**, or **Custom** for anything else, with the esxDOS version in it when it has one. The
     {icon}`fileopen` button picks a firmware file. **Apply** writes the choice into the
     EEPROM; before it replaces a **Custom** firmware, which may exist only in the EEPROM,
     it asks.
2. Make a card: **Media > divMMC** has a menu for each slot, **Card 1** and **Card 2**,
   with the name of the card in it. **New esxDOS Folder...** asks for a folder - an empty
   one, or a new one made in the same window - copies the `SYS` and `BIN` folders of the
   esxDOS release the EEPROM holds into it, adds `TMP`, puts it in the slot and powers the
   machine off and on. With no bundled release in the EEPROM it asks which one to copy. esxDOS starts
   from the card; put your programs into the folder.
3. Or use a card of your own. Each slot's menu also has **Open Image...**, **Open
   Folder...**, **Eject** and **Write Protect**, as the ZX Evolution's card has. esxDOS
   needs an image with an MBR and one FAT16 or FAT32 partition, or a folder, holding the
   `SYS`, `BIN` and `TMP` folders of the same esxDOS release as the EEPROM. When a folder
   in slot 1 holds another bundled release than the EEPROM, Xpeccy+ says so in a window
   after the machine is switched on: esxDOS itself does not check, and fails in odd ways.

Another esxDOS version, or a firmware of your own, goes in with the {icon}`fileopen` button,
or the way the real board is flashed: turn **EEPROM writable** on, load the flasher, such as
`ESXMMC.TAP`, and follow it, turn the option off, then use **Machine > Reset > Hard Reset**.
The **Firmware** row then shows what was flashed. With the EEPROM erased and the option
off, the machine stops at its first interrupt, as it does with a real board that has no
firmware.

![Machine: divMMC](images/options-divmmc.png)

A real board is fitted and removed with the power off, and so is this one: **Apply**
after a change in the **divMMC** row, in **RAM** or in **Firmware** powers the machine off
and on. There is one board, as if the same one
went from machine to machine: the RAM size and the EEPROM jumper are set for each machine,
but the EEPROM is the same on all of them, and is kept for the next start of the emulator.

esxDOS looks for the card and loads its system files only when the machine is switched
on. A card put in later, or changed, is seen after **Machine > Reset > Hard Reset**, which
also reads a folder used as a card again, with the files copied into it since;
started with no card, esxDOS ignores the NMI button until then. A plain reset is not enough:
then the firmware skips the search and keeps what it found before.

### Switching it on and off

**Media > divMMC** switches the board on and off in one go. It is in the right-click menu
too, it can be a toolbar button - a click switches, its arrow lists the cards - or have a
hotkey (**divMMC On/Off**). While the board is on, **divMMC** is a menu: the same item,
ticked, to switch it off, then **Card 1** and **Card 2**. It works as the **divMMC** row of
**Options** does: the machine is powered off and on, and the setting is the machine's, kept
for the next start. Since what is in the machine's memory is lost, a window asks first.
The board's RAM, its jumper and its EEPROM stay as they were.

While the board is on, the floppy drives and the hard disk are out of the machine: the
board takes the TR-DOS entry, so on a real machine the drives would sit there unused. Their
lights in the status bar are dim and struck through, the **Disk Manager** and **Drives**
menus show the drives grayed under a line that says why, and **Options** shows **Disk** and
**Hard disk** dimmed.
Nothing is ejected: switch the board off, and the drives come back with their disks.
Opening a disk while the board is on asks to switch it off first.

### Which esxDOS

The releases differ mostly in their TR-DOS emulator, which runs `.trd` images from the
card:

- **0.8.5** - the first with the TR-DOS emulator.
- **0.8.6** - TR-DOS Navigator boots a disk with no boot file, Beta 48 support, the `.128`
  command.
- **0.8.7** - saving the EEPROM to the card works, Headball GS loads.
- **0.8.8** - `SAVE ... BIN`, `GO TO` with a string variable, no parity check when loading
  `.tap` files, the NMI silences the sound chips.
- **0.8.9** - `.scl` images, TR-DOS `MOVE` and `FORMAT`, and a fix for a full card whose
  root folder got damaged.

Each release's own `README.TXT` is in `config/esxdos/<version>/`.

### Using it

- The firmware starts on every reset, then goes to 48K BASIC. Type `.ls` for the files on
  the card and `.cd` to go into a folder; the dot commands are the files in `BIN`.
- **Machine > NMI** (Shift+F10 in the Modern preset, F10 in the Classic one) is the board's
  NMI button: esxDOS opens its file browser.
- A folder used as a card takes what esxDOS saves, unless folders are set to read only
  (see [Writing to a folder](#writing-to-a-folder)): then esxDOS reports `I/O ERROR` when
  it tries to write.
- **Machine > Reset** (the plain one) is the board's reset button, with one difference
  from the original board: it also clears the board's MAPRAM setting, as the ZX Spectrum
  Next's board does. On the original, only the power clears it, and a program that crashes
  into setting it (BREAK held during a load from the card is one way) leaves the machine
  with garbage or stopped until it is switched off. Here a reset brings the firmware back,
  warm: esxDOS keeps what it had, a tape attached in its browser included, and goes
  straight to 48K BASIC. After `ESXMMC.TAP` has flashed the EEPROM, the firmware runs from
  a copy in the board's RAM until that reset.
- **Machine > Reset > Hard Reset** is the power switch: the board's RAM goes with the
  machine's, and esxDOS starts cold, with its logo and the card read again.
- A tape or disk opened to be run ([Run a program](getting-started.md#run-a-program))
  starts from a Hard Reset while the board is on, so that a tape attached in esxDOS does
  not take the `LOAD ""` instead; the keys go into the 48K BASIC the firmware leaves the
  machine in. A disk cannot be started this way: the drives are out while the board is on,
  so Xpeccy+ offers to switch the board off first. A `.trd` on the card starts from the
  esxDOS browser.
- The window title names the card while esxDOS reads it, by its image file or folder, as it
  names a disk while its drive runs.
- esxDOS runs in 48K BASIC. 128 BASIC and +3 BASIC stop working with the board on, as they
  do on the real one.
- On a machine with a Beta Disk, the board answers the calls to TR-DOS (`#3Dxx`) first:
  `RANDOMIZE USR 15616` starts the TR-DOS that esxDOS runs on the card's `.trd` images, not
  the machine's own. **Machine > NMI** is the board's button then, not the machine's own.
- A snapshot saved as SZX keeps the board's state; other formats leave it out.

## Hard disks

```{index} hard disk, IDE, HDI, LBA
```

```{index} see: HDD; hard disk
```

A machine with an IDE interface (see [Which machine has what](#which-machine-has-what)) has
two places for a drive, **Master** and **Slave**. Connect them first: on the **Machine** page
of **Options**, click the {icon}`settings` gear in the **Hard disk** row and set **Master**
(and **Slave**, if you want two) to **HDD (ATA)**.

**Media > Drives** then has a submenu for each connected device, named **Master:** or
**Slave:** followed by the name of the image in it:

- **Open Image...** - open a hard disk image: `.hdi`, or a plain image of the disk with any
  other extension.
- **Open Folder...** - use a folder on your computer as the disk. See
  [A folder as a disk](#a-folder-as-a-disk).
- **Eject** - take the image out.
- **Properties...** - the window **Hard Disk: Master** (or **Slave**): the image, its
  cylinders, heads and sectors (**C / H / S**), its **Capacity**, and the **LBA** box. LBA
  lets the system address the disk by sector number; clear it for an old system that only
  knows cylinders, heads and sectors.

What the machine writes to a hard disk image goes into the file at once. The images stay
mounted when you switch machines, and are opened again at the next start.

## A folder as a disk

```{index} folder as a disk, Open Folder, FAT32, NedoOS
```

**Open Folder...** in **Media > SD Card**, in a divMMC's card menus and in **Media > Drives**
lets the machine use a folder of your computer as if it were a FAT32 card or disk. There is
no image to make: copy programs into the folder and open it. The window that picks the
folder is **Folder to serve as the card** or **Folder to serve as the disk**.

- The folder is read when it is opened, and again at **Machine > Reset > Hard Reset**,
  when you switch machines or click **Apply** in **Options**. Files added later show up
  only then.
- The disk is made big enough for the folder, from 512 MB up to 32 GB. A folder that takes
  writes gets as much free room again as its files take, and 256 MB more. A larger folder
  is not opened, and the message **folder too big for a disk** says so.
- These are left out: files of 4 GB and over, folders more than 16 levels deep, entries
  past 65536 in one folder, and anything past 200000 entries in all. The message
  **folder: not all of it fits** says when something was left out.
- A link to a folder (a junction on Windows) is followed once: one that leads back into
  the folder, or to a place already shown, is left out.
- On a hard disk, a folder that holds NedoOS with its `ZXLDR` file in the top level can
  boot NedoOS.

### Writing to a folder

```{index} folder as a disk; writing, Writing to a folder, Folders, Deleted files
```

A folder takes what the machine writes: a program it saves, a file it changes or deletes
there changes the files in the folder itself. The folder follows about half a second after
the machine stops writing. A file the machine saves with no date keeps the time it was
written on your computer.

The first time the machine writes to a folder, it stops and **Writing to a folder** asks:

- **Allow writing** - the folder takes this write and every one after it.
- **Read-only** - the folder stays as it is. What the machine wrote stays in memory, and
  the machine sees it, until the folder is read again. Folders opened after that are
  read only.

It asks once: the answer becomes the **Folders** setting. Both settings are in the
**Folder as a disk** group on the [Emulation](options.md#emulation) page of **Options**:

- **Folders** - **R/W**, read/write (the default), or **R/O**, read-only. The machine cannot
  write to a read-only folder: esxDOS reports `I/O ERROR`, a hard disk an error.
- **Deleted files** - **Move to trash** (the default) puts a file the machine deletes in
  your system's trash; **Delete permanently** removes it. A file the trash will not take
  stays in the folder.

**Write Protect** in a card's menu makes that one folder read only, whatever **Folders**
says.

What the machine wrote is never thrown away on the way. When something cannot be carried
over - a file another program has open, a name your system refuses - a message says so
once, naming the files, and a file's content that could not be put in its place is kept in
the folder's top level with **(recovered)** in its name. A file you change on your computer
while the machine has the folder keeps your change: the machine's version is saved beside it
as **name (2)**, and the machine deleting it leaves it where it is.

Keep a copy of anything you cannot afford to lose. Formatting the card or disk, or
deleting most of the folder at once, does not reach your files: the folder is left alone
from then on, what the machine writes stays in memory, and a message says so.

## Cartridge

```{index} cartridge, Interface 2
```

The ZX Spectrum 48K has a slot for an Interface 2 ROM cartridge, and the ALF TV Game consoles
have a slot for their own game cartridges. **Media > Cartridge** shows the cartridge in the
slot, or **(empty)**, and has:

- **Open...** - a cartridge image, `.rom` or `.bin`, up to 4 MB. The machine is reset with
  the cartridge in.
- **Eject** - take the cartridge out; the machine is reset if it was running from it.
