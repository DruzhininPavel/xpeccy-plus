# Speed and time

```{index} speed
```

The machine normally runs at the speed of the real one, about 50 frames a second. The
**Machine** menu has the commands that change that: **Pause**, **Fast Mode**,
**Slow Motion** and **Fast Forward**. The same commands, and {icon}`time-rewind` **Rewind**
too, are at the right end of the toolbar. Their settings are on the **Emulation** page of
**Options**.

## Pause

```{index} Pause, Pause when inactive
```

**Machine > Pause** stops the machine, and the same command again lets it continue. The
status bar says **paused**, and a pause sign shows in the top right corner of the
picture. The key is Pause in both presets.

The machine also waits while **Options**, a file dialog or the **Debugger** is open, and
continues when that window closes. A menu does not stop it.

To stop the machine whenever you switch to another program, tick
**Pause when inactive** on the **Application** page of **Options**. The machine continues
when you come back to the window.

## Fast Mode

```{index} Fast Mode, Fast Mode render
```

**Machine > Fast Mode** runs the machine as fast as the computer can, with no sound. Use
it to get through a long wait - a slow loader, a long calculation. The title bar says
**fast** while it is on, and anti-flicker is not applied.

The **Fast Mode render** row of the **Emulation** page sets what fast mode shows. The number
at its start is how many frames a second are shown, from 1 to 100 (30 by default); the
frames between are not drawn. Fast loading of a tape shows the same number of frames a
second. The list after the number says how a frame is drawn:

- **Raster (normal)** - line by line as the machine runs, as at normal speed;
- **Video memory dump (simplified)** - the video memory as it is at the end of the frame,
  all at once. A multicolor picture does not show as it should.

Any pause ends fast mode; **Machine > Pause** first lets the machine finish the frame, so
the picture is whole. Fast mode cannot be started while the machine is paused.

## Slow Motion and Fast Forward

```{index} Slow Motion, Fast Forward
```

**Machine > Slow Motion** runs the whole machine slower, picture and sound alike:
**1/2**, **1/4** (the default) or **1/8** of the normal speed. **Machine > Fast Forward**
runs it faster, **x2**, **x4** (the default) or **x8**, and the sound plays sped up. The
speeds are set in the **Slow motion** and **Fast forward** rows of the **Emulation**
page of **Options**.

Both always start from normal speed: switching one on switches the other off, and
moving the **Speed** slider ends either of them. Opening a file or switching to another
machine brings the speed back to normal too.

A sign in the top right corner of the picture shows which mode is on.

## How the keys work

```{index} hotkeys; Toggle or Hold, Tap or hold
```

The **Slow motion**, **Fast forward** and **Fast Mode, no limit** rows of the
**Emulation** page each end in a list that says how the key of that mode works:

- **Toggle** - a press switches the mode on, the next press switches it off;
- **Hold** - the mode is on only while the key is held down;
- **Tap or hold** (the default) - a short tap switches it on or off, a longer press
  keeps it on only while the key is held.

A click on {icon}`time-slow` **Slow Motion**, {icon}`time-ffwd` **Fast Forward** or
{icon}`time-fast` **Fast Mode** in the menu or on the toolbar switches the mode on or off,
whatever the key setting.

## Rewind

```{index} Rewind, Rewind; Step and History, Allow Rewind
```

Rewind takes you back in time: hold the **Rewind (Hold)** key - Del in both presets -
or hold the {icon}`time-rewind` **Rewind** button on the toolbar down. The picture runs
backwards, with the sound played backwards too, and the sign in the top right corner shows
how many seconds back you are. Let go, and the machine continues from the picture on the
screen. Everything that came after that picture is gone.

Rewind is on by default. **Machine > Allow Rewind** switches it off and on; with it
off, the **Rewind** button is grayed out. While it is on, the program keeps a history
of the machine in memory, a snapshot every few frames. This takes 20 to 45 MB with the
default settings and costs a little emulation speed.

**Rewind** on the **Emulation** page of **Options** is the same switch, with two
settings beside it:

- **Step** - frames between two snapshots, from 1 to 50 (3 by default). When you rewind,
  each frame shown is one snapshot earlier, so a smaller step rewinds more slowly and
  smoothly, and keeps more snapshots in memory.
- **History** - how far back you can go, from 5 to 300 seconds (30 by default).

The history starts over when:

- the machine writes to a floppy disk, a hard disk or an SD card - the disk could not
  be put back as it was;
- you switch to another machine, or change a machine setting in **Options** that the
  history depends on;
- you change **Step** or **History**;
- an RZX recording starts or ends, or an RZX file starts or stops playing.

While a tape is being recorded, no history is kept and rewind does not start. Fast loading
of a tape is not kept either: when you rewind, the picture jumps over it.

## The Speed slider and Turbo Mode

```{index} Speed slider, Overclock, Turbo Mode; and Fast Forward
```

Two more controls change how fast the machine runs, and both are described in
[Turbo and speed](machines.md#turbo-and-speed):

- **Machine > Turbo Mode** is the board's own turbo, a part of the machine like on the
  real one. A program can switch it by itself.
- The **Speed** slider on the **Machine** page of **Options** belongs to the emulator.
  Its left half slows the whole machine down, like **Slow Motion**. Its right half,
  **Overclock**, makes only the CPU faster: the frame rate, the music and everything
  timed by the frame stay at normal speed, while a program that is slow because of the
  CPU - a 3D game, say - runs faster.

**Fast Forward** is different from both: it speeds up the whole machine, frames and
sound included, as if time ran faster.

While the CPU runs at a clock other than its base clock, the **CPU clock** indicator can
show that clock over the picture (see
[Indicators on the picture](picture.md#indicators-on-the-picture)). The status bar shows the
clock all the time.

## Input lag

```{index} input lag, Low latency, Run ahead
```

The **Input lag** group on the **Emulation** page sets how soon a key press shows on the
screen.

**Low latency** shows each frame as soon as it is made. In a window the desktop still
holds each frame back by one more frame; in fullscreen that delay is skipped too, but menus
then flicker as they open. With **Low latency** off, frames are buffered, for smoother
motion. It is on by default.

**Run ahead** makes keys respond sooner by one or two frames. Each frame, the program
emulates that many frames further, shows the picture from there, and goes back. So you
see the result of a key press a frame or two earlier than the machine itself would show
it. The cost is CPU time - the frames are emulated more than once - and the sound stays
a frame behind the picture. **Off** is the default. Run ahead does nothing while an RZX
recording plays, in fast mode and fast forward, while the machine is paused, and while
you rewind.

## Frames per second

```{index} FPS, frames per second
```

The status bar shows how many frames a second the machine actually runs at. With the
status bar hidden, tick **FPS** in the **Indicators** group on the **Appearance** page of
**Options** to see the number in the top right corner of the picture.
