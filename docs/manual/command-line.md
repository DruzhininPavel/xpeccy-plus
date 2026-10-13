# Command line

```{index} command line, command line; options
```

Xpeccy+ takes file names and options on the command line:

```
xpeccy-plus [options] [files]
```

On Windows the program is `xpeccy-plus.exe`. Put a name that has spaces in it in quotes.

```{index} --help, -h
```

`-h` or `--help` prints the list of options and quits without opening the window. On
Windows add `-c` to see it.

An option the program does not know, or one given last without its value, is not opened
as a file: it is a warning in the log and, once the window is up, a message over the
picture. An address or a page number that is not a number gets the same warning, and the option
is left out.

## Files

```{index} command line; files, --autostart, --no-autostart
```

Every word that is not an option is a file to open: a snapshot, a tape, a disk or anything
else **File > Open...** takes. Name several to open several, a disk and a tape for
instance.

The command line is read from left to right, and an option acts on the files after it.

A tape or a disk named here is run, as when **Run opened media** is chosen on the
**File types** page of **Options**. Only **Mount opened media** there makes it just go in.
The command line never asks which one you want. Two options decide it for one start:

- `--autostart` - run the files named here, whatever **Options** says;
- `--no-autostart` - only mount them.

A file can switch the machine, as it does when you open it from the menu (see
[Run a program](getting-started.md#run-a-program)), unless `-m` names the machine.

A file or a machine named here skips the **Start Machine** question, if **Options** asks it
at start (see [Machine at start](machines.md#machine-at-start)).

## Machine and media

```{index} -m, --machine, -p, --disk, --sdcard, --confdir
```

- `-m ID`, `--machine ID` - start on this machine. It takes the machine's name as the
  list shows it, such as `"ZX Spectrum 128K"`, or its short id: the name of its file,
  such as `zx48`, `zx128`, `zxplus2`, `zxplus2a`, `zxplus3`, `pent`, `pent1024`, `scorp`,
  `scorptp`, `atm2`, `profi3`, `profi`, `phoenix`, `evo-baseconf`, `evo-tsconf`, `alf`,
  `alf128`. `-p` is the older name of the same option. It is acted on before anything
  else, wherever it stands, so a snapshot named before it is not lost; given twice, the
  last one counts. It counts as your own pick of a machine, as in the **Machine** menu.
- `--disk X` - the floppy drive for the disks named after it: `0` to `3`, `a` to `d` or
  `A` to `D`. Without it a disk goes into drive A.
- `--sdcard PATH` - the SD card of the ZX Evolution: an image, or a folder served as a card.
- `--confdir DIR` - take the settings from another folder instead of the usual one (see
  [Where the settings are](getting-started.md#where-the-settings-are)).

```
xpeccy-plus -m pent game.trd
xpeccy-plus --disk b data.trd --disk a boot.trd
```

## Window

```{index} -s, --size, --fullscreen, -r, --ratio, -n, --noflick, --style
```

These change the setting itself, as **Options** would, and it stays for the next start.

- `-s N`, `--size N` - window size, 1 to 6 (x1 to x6).
- `--fullscreen 1` - start fullscreen; `--fullscreen 0` - in a window.
- `-r 1`, `--ratio 1` - keep the picture's shape in fullscreen; `0` stretches it.
- `-n N`, `--noflick N` - anti-flicker intensity, 0 to 100 percent.
- `--style` - macOS only: the system's own look instead of the one Xpeccy+ sets.

## Debugging

```{index} command line; debugging, -d, --debug, -l, --labels, -a, --adr, -f, --file, -b, --bank, --pc, --sp, --bp, --brk, --brk-log, --xmap
```

- `-d`, `--debug` - open the Debugger at once.
- `-l FILE`, `--labels FILE` - load labels made by SjASMPlus (its `LABELSLIST`). They win
  over the labels that come with an image.
- `-a ADR`, `--adr ADR` - where `-f` loads, 16384 (`0x4000`) if not given.
- `-f FILE`, `--file FILE` - load a binary file into memory at that address.
- `-b N`, `--bank N` - put RAM page N at `#C000`.
- `--pc ADR` - set the PC register.
- `--sp ADR` - set the SP register.
- `--bp ADR` - a breakpoint on executing that address. A label name works too.
- `--brk FILE` - load a list of breakpoints (`.xbrk`); its addresses can be labels. Give it
  more than once to load several lists.
- `--brk-log FILE` - where the [breakpoint log](debugger-breakpoints.md#the-breakpoint-log)
  goes; `-` writes it to the standard output.
- `--xmap FILE` - load a memory map saved by the Debugger (`.xmap`).

```{index} command line; numbers
```

A number in these options is hex with `#`, `$` or `0x` in front (`#8000`, `$8000`,
`0x8000`), and decimal without one (`32768`). A leading zero means nothing: `0100` is a
hundred. In a shell `#` starts a comment and `$` a variable, so there write `0x8000`, or
put the number in quotes: `"#8000"`.

`--bp` and `--brk` are set once every option is read, so the labels of `-l` are there for
them wherever `-l` stands. The others act at the moment they are read, so put them after
a snapshot they belong to: a snapshot loaded later sets its own registers and memory. A
tape or a disk that is run resets the machine after all the options are read, which undoes
`--pc` and `--sp`.

```
xpeccy-plus -m zx128 -a 0x8000 -f code.bin --pc 0x8000 -d
xpeccy-plus game.sna --bp main -l game.lab
```

## Running with no window

```{index} --bench
```

```{index} see: headless run; --bench
```

```{index} see: benchmark; --bench
```

`--bench N` runs the machine for N frames with no window, as fast as the computer can,
with no sound, and quits. It is for scripts: running a test program, measuring a routine,
taking a picture or a snapshot at a given frame. Everything else on the command line acts
as usual, so the files, the machine and the breakpoints are set first.

A breakpoint whose action is **Debugger** ends the run where it fires. One with **Counter**
counts and lets the run go on, and one with **Log** writes its line, as in the window.

What the run prints goes to the standard output; the program's log, when it goes to the
console, to the standard error. A run ends with a line of its speed:

```
bench: machine zx48, fast, 83 frames in 0.153 s: 543.5 fps, x10.85 real time, 26.14 ns/T (stopped early)
```

`(stopped early)` says a breakpoint ended it. After it comes one `SPAN` line for each span
the breakpoints measured (see [Spans](debugger-breakpoints.md#spans)).

```{index} exit code
```

The exit code tells a script how the run ended:

| Code | Meaning |
|---|---|
| `0` | it ran every frame asked for |
| `1` | something on the command line was not understood; nothing was run |
| `2` | a breakpoint stopped it |

```{index} --bench-skip, --bench-steps, --bench-full, --bench-keys, --bench-hash, --bench-shot, --bench-save, --bench-state, --bench-nodraw, --bench-heat, --bench-prof
```

More options for such a run:

- `--bench-skip N` - run N frames first, left out of the timing;
- `--bench-steps N` - then N instructions more, to stop in the middle of a frame;
- `--bench-full` - mix the sound too, as playing does;
- `--bench-keys SPEC` - press keys on given frames, see below;
- `--bench-hash` - print a check sum of the frames, the sound, the RAM and the registers,
  for telling whether two runs did the same;
- `--bench-shot FILE` - save the last frame, with the whole border, as a PPM picture;
- `--bench-save FILE` - save a snapshot at the end, in the format the extension names;
- `--bench-state FILE` - save the raw state of the machine at the end;
- `--bench-nodraw` - draw no picture, to measure the rest without it;
- `--bench-heat` - collect the memory heat map, counted in `--bench-hash`;
- `--bench-prof FILE` - Windows only: where the time of the emulation goes.

`--bench-keys` takes a list split by commas, each item `FRAME:KEYS` or `FRAME:KEYS/N`: the
keys go down on that frame of the run and are held N frames, 4 if not given. The keys of
one item are pressed together. Digits and small letters are those keys of the Spectrum,
`E` is Enter, `C` Caps Shift, `S` Symbol Shift and `_` Space. `joy:up+fire` holds the
joystick, `joyb:` the second one, `nmi` presses the NMI button, and `pc:` with a keymap
name presses a key of the PC keyboard a ZX Evolution has: `pc:F1`, `pc:UP`, `pc:ENT`.

```
xpeccy-plus -m zx48 game.tap --bench-keys "500:5,600:_/10" --bench 2000 --bench-shot end.ppm
```

`--help` lists a few more `--bench-` options. They check the emulator itself.

On Linux and macOS a shell waits for the run and has its exit code as usual. On Windows
the program does not hold up the console it is started from. In PowerShell send what it
prints into a pipe, and the exit code is in `$LASTEXITCODE`:

```
.\xpeccy-plus.exe -m zx48 --brk test.xbrk --bench 3000 | Out-String
$LASTEXITCODE
```

In the Command Prompt start it with `start /wait "" xpeccy-plus.exe ...`, and the exit
code is in `%errorlevel%`.

## Recording

```{index} --wav-out, --video-out, --video-auto, --no-video-auto, --video-autostart, --video-autostop
```

- `--wav-out FILE` - record the sound to a WAV file from the start.
- `--video-out FILE` - record a video from the start, `.mp4` or `.mkv`.
- `--video-auto`, `--no-video-auto` - start and stop recording by itself, or not, whatever
  **Options** says.
- `--video-autostart AT`, `--video-autostop AT` - when it starts and stops by itself.

See [Screenshots and recording](capture.md) for what each one does.

## Logging

```{index} log, --log, --no-log, --log-dir, --log-level, --log-groups, --log-console, -c, --console
```

What these set is for this start only: the settings in **Options** stay as they are.

- `--log` - write a log file, to `logs/<date>/` beside the program. Where the program's
  folder cannot be written to, the log goes to the settings folder.
- `--no-log` - no log file, even if **Options** says to write one.
- `--log-dir DIR` - put the `logs` folder into `DIR`.
- `--log-level LEVEL` - how much is written: `error`, `warn`, `info` (the usual),
  `debug` or `trace`.
- `--log-groups SPEC` - a level for single parts of the program, such as
  `"video:trace,net:off"`; `"all:debug"` sets them all, `-sound` turns one off. The parts
  are `app`, `conf`, `core`, `cpu`, `video`, `sound`, `tape`, `disk`, `input`, `hw`,
  `file`, `gui`, `gl`, `net` and `brk`.
- `--log-console` - write the log to the standard error as well, as **To the console** in
  **Options** does. On Windows there is one only with `-c` or a redirect.
- `-c`, `--console` - Windows only: write to the console the program was started from, or
  open a console of its own when there is none. That console waits for a key before it
  closes. The log goes there at `debug` level unless `--log-level` says otherwise. Output
  sent to a file stays in the file.
