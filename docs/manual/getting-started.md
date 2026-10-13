# Getting started

## Download

```{index} download, installation, AppImage
```

Every release is on the [releases page](https://github.com/dotkoval/xpeccy-plus/releases).
Pick the file for your system:

| System | File | |
|---|---|---|
| Windows 10 or 11 | `xpeccy-plus-<version>-win-x64-qt6.zip` | |
| Older Windows | `xpeccy-plus-<version>-win-x64.zip` | 64-bit |
| | `xpeccy-plus-<version>-win-x86.zip` | 32-bit |
| Linux | `xpeccy-plus-<version>-linux-x86_64.AppImage` | |
| macOS | `xpeccy-plus-<version>-macos-arm64-qt6.dmg` | Apple silicon |

Nothing has to be installed besides. Machines, ROM images and settings come with the
program, so it runs right away.

**Windows.** Unpack the archive anywhere you can write to, and run `xpeccy-plus.exe`.

**Linux.** Allow the file to run (`chmod +x`, or the file's properties), then run it. It
needs a distribution no older than Ubuntu 22.04 (glibc 2.35).

**macOS.** Open the disk image and drag **xpeccy-plus** into Applications. The program is not
signed by a registered developer, so the first start is refused: open
**System Settings > Privacy & Security**, find the message about Xpeccy+ and click
**Open Anyway**.

## The first start

```{index} first start, Which machine to start?
```

The first start asks which machine to run:

- **Which machine to start?** - any of the built-in machines.
- **Next time** - **Start the last one chosen**, **Ask again**, or **Start this one**.

You can change the answer later on the **Application** page of **Options**.

## Run a program

```{index} Open, Run, Mount, Remember my choice, drag and drop, media; run or mount
```

Open a file in any of these ways:

- **File > Open...**;
- drop the file onto the window;
- give its name on the command line: `xpeccy-plus game.tzx`.

A snapshot (`.szx`, `.z80`, `.sna`) continues straight away from where it was saved.

A tape or a disk can be **run** or just **mounted**. Run resets the machine and starts
the program on it; Mount only puts the tape or disk in, as if into a real recorder or
drive. The first time, Xpeccy+ asks which one you want. Tick **Remember my choice** to stop
the question; the **File types** page of **Options** sets it again. To be asked about one
file only, hold Shift as you open or drop it.

```{index} machine; switched by a file
```

If the current machine cannot run the file (a +3 disk on a Pentagon, for instance),
Xpeccy+ switches to one that can - the machine the file names, when it names one. The next
file your own machine can run switches back to it.

```{index} Choose a Machine
```

The **File types** page of **Options** sets this for each kind of file:

- **Auto** - switch only when needed, then back to your machine;
- **Ask** - when this machine cannot run the file, the **Choose a Machine** window lists
  the machines that can: **Switch** to the one picked, **Keep** the current one, or
  **Cancel** and not open the file;
- **Keep current** - never switch the machine;
- or one machine that always opens this kind of file.

## Where the settings are

```{index} settings folder, config.conf, roms folder, --confdir
```

```{index} see: configuration folder; settings folder
```

| System | Folder |
|---|---|
| Windows | `config` beside `xpeccy-plus.exe` |
| Linux, macOS | `~/.config/xpeccy-plus` (or `$XDG_CONFIG_HOME/xpeccy-plus`) |

Everything you change is kept there: `config.conf` holds the settings, `roms` holds the
ROM images. A file put in `roms` under the name of a built-in image replaces that image.

On Windows the whole folder can be moved or copied, settings and all. To run with a
different settings folder, start the program with `--confdir <folder>`.

## This manual

```{index} Manual
```

**Help > Manual** opens this manual from the program's own files, so it needs no network.
It opens in the language of your system when the manual has that language, and in English
when it does not. Switch the language at the top of the left column.
