# Picture

How big the picture is, how much border it shows, and how it is drawn: the colors, a
shader, anti-flicker. The quick switches are in the **View** menu; all the settings are on
the **Video** page of **Options**.

## Window size

```{index} window size, scale
```

**View > Size** has **x1** to **x6**: each pixel of the machine is drawn as a square of
1 x 1 to 6 x 6 screen pixels. The scale is always a whole number, so every pixel of the
picture is the same size and nothing is blurred. At **x1** the picture is drawn
as it is, without a shader.

The window cannot be resized by dragging its edges. Its size follows from the scale and
the border size, and changes when either of them changes.

## Fullscreen

```{index} Fullscreen, Keep Aspect Ratio in Fullscreen, aspect ratio
```

**View > Fullscreen** fills the whole screen with the picture. In fullscreen the
picture is fitted to the screen rather than kept to whole pixels. How to reach the menu
from there is in [Fullscreen](main-window.md#fullscreen).

**View > Keep Aspect Ratio in Fullscreen** matters only in fullscreen:

- ticked - the picture keeps its shape. On a screen wider than the picture, more border
  is shown at the sides instead of black bars, as far as the machine's frame goes;
- not ticked - the picture is stretched to fill the whole screen.

Fullscreen is remembered: if the program is closed in fullscreen, it starts in
fullscreen next time.

## Border

```{index} Border slider, border; size, overscan
```

The **Border** slider in the **Picture** group of **Options > Video** sets how much
border is shown around the screen. The label beside it names the size and gives it in
machine pixels:

| Size | Picture |
|---|---|
| **no** | 256 x 192, the screen alone |
| **tiny** | 272 x 208 |
| **small** | 288 x 224 |
| **medium** | 320 x 240 |
| **full** | 352 x 288 |
| **overscan** | the whole frame the machine draws |

**View > Border** has the same sizes, taken at once, with the machine running or paused.

The setting is the same for every machine, but some machines need more of their frame
than others: ATM Turbo 2+ and ZX Evolution (BaseConf) show at least **medium**, and ZX
Evolution (TSConf) always shows its whole frame - the slider and **View > Border** are
grayed out there.

## Palette

```{index} palette, ZX Palette, Palette Editor
```

```{index} see: color; palette
```

The 16 colors of the ZX picture can be taken from several palettes. Pick one in
**View > ZX Palette**, or in the **Palette** list of **Options > Video**. **Xpeccy+** is
the built-in palette; the others are listed by their file names, without the extension.

To change the colors of a palette, pick it in **Options > Video** and click the
{icon}`edit` edit button beside the list. The **Palette Editor** shows the 16 colors in four
rows: the eight normal colors in the top two rows, the eight bright ones below them. Click a
color to choose another one, then click **OK**. The change takes effect only after
**Apply** or {icon}`ok-apply` **OK** in **Options**; {icon}`cancel` **Cancel** there
discards it. The
edited palette is saved under the same name in the `palettes` folder of the settings folder
(see [Where the settings are](getting-started.md#where-the-settings-are)), and from then on
that file is used instead of the bundled one. The built-in **Xpeccy+** palette cannot
be edited: the edit button is grayed out for it.

When a bundled palette has been edited, a second button, {icon}`refresh`, appears beside the
edit button. After asking, it deletes your edited copy, and the palette is again the one
that came with the program.

```{index} palette; own
```

A palette of your own is a text file in the same `palettes` folder: 16 lines, one color
on each, written as `#RRGGBB` - black, blue, red, magenta, green, cyan, yellow, white,
then the same eight colors bright. A file with fewer than 16 colors is not used; the
built-in palette is shown instead.

```{index} Gray scale
```

**Gray scale** in the **Picture** group draws the picture in shades of gray.

```{index} ULA+, ULA plus
```

A machine with ULA+ can show 64 colors when a program asks for them. ULA+ is switched on
with **ULA plus** in the machine's advanced settings (see
[Advanced settings](machines.md#advanced-settings)).

## Shaders

```{index} shader
```

```{index} see: scan lines; shader
```

A shader changes the look of the picture after it is drawn: scan lines, the mask of a
CRT tube, a curved screen, the blur of a TV signal. Pick one in **View > Shaders**, or in
the **Shader** list of **Options > Video**; **None** turns it off. The program comes with
twenty shaders built in, and one of them is on by default.

The shader is not used at **x1**, where there is no room for its effect. When the
computer's graphics cannot run shaders, the list holds **None** only.

```{index} shader; own, GLSL
```

Your own shaders go in the `shaders` folder of the settings folder, as `.txt` files.
A file there with the name of a built-in shader replaces that shader. The file holds two
parts in GLSL: a line `vertex:` followed by the vertex shader, and a line `fragment:` followed by
the fragment shader. The picture comes in as the texture `rubyTexture`, with its size in
`rubyInputSize` and `rubyTextureSize`; the size of the picture on the screen is in
`rubyOutputSize`. When a shader is picked, the message **shader compiled** appears on the
picture. If the shader does not work, the message is **shader error**, and the picture is
shown without a shader.

```{index} Reload Shader
```

The **Reload Shader** action reads the shader file again, so you can see a change you
have just saved. It has no key by default; give it one on the **Hotkeys** page of
**Options** (see [Hotkeys](hotkeys.md)).

## Anti-flicker

```{index} Anti-flicker, Anti-flicker; Intensity, Anti-flicker; Mode, Anti-flicker; Look ahead
```

Some programs show more colors than the machine has by switching between two or three
pictures every frame. On a CRT television this looked like one steady picture; on a
modern screen it flickers. Anti-flicker blends the frames together again. Its settings are
in the **Anti-flicker** group of **Options > Video**:

- **Intensity** - how much of the previous frames is mixed into the picture. **0% (off)**
  turns anti-flicker off; it is off by default.
- **Mode** - which pixels are blended, and how many frames; see the table below.
- **Gamma** - how the colors are mixed. 2.2 is close to a CRT; 1.0 mixes them linearly.
  Values from 1.8 to 2.4 are usual.
- **Look ahead** - only for the adaptive modes: the machine runs two frames ahead in the
  background, so a flicker is recognized sooner after it starts. It costs some extra
  CPU time.

"Fullscreen" in the names of the modes means the whole picture, not fullscreen view:

| Mode | What it does |
|---|---|
| **2-frames (fullscreen)** | Every pixel that changed since the last frame is mixed with its color in that frame, all over the picture. Good for a still two-frame picture; a moving picture gets trails. |
| **2-frames (adaptive)** | Only the pixels that keep alternating between the same two colors for several frames are mixed. Moving parts stay sharp. |
| **3-frames (fullscreen)** | The last three frames are mixed everywhere, for pictures made of three frames in turn (red, green and blue). |
| **2-/3-frames (adaptive)** | Finds both kinds by itself - three-frame pictures and two-frame flicker - and leaves everything else alone. The default. |

The **Anti-flicker** action switches the intensity in turn between 50%, 100% and off,
and the picture shows which one. Its key is set on the **Hotkeys** page of **Options**.

Anti-flicker is not applied in fast mode.

## Indicators on the picture

```{index} Indicators, CPU clock, HALT indicator, Fade out
```

The **Indicators** group on the **Appearance** page of **Options** sets what is drawn
over the picture. Tick the ones you want:

| Indicator | What it shows |
|---|---|
| **Keyboard** | the rows of keys the program is reading (ZX machines only) |
| **Joystick** | blinks while the joystick is read |
| **Mouse** | blinks while the mouse is read |
| **Tape** | on while the tape plays, red while it records |
| **Disk** | green when a disk is read, red when it is written |
| **FPS** | frames per second, top right |
| **HALT** | the T-states up to the HALT and in the whole frame, bottom right |
| **CPU clock** | the clock in MHz, bottom right, while the CPU is not on its base clock - turbo or overclock |
| **Messages** | what Xpeccy+ has just done, bottom left: "size x3", "quick save" and so on |
| **Fade out** | signs and messages fade away instead of vanishing at once |

**Joystick**, **Mouse**, **Tape**, **Disk**, **FPS** and **CPU clock** are drawn on the
picture only while the status bar is hidden; with the status bar shown, they are in the
status bar instead (see [Status bar](main-window.md#status-bar)).

In the top right corner a sign also shows the speed mode, in the manner of a video
recorder: pause, rewind with how many seconds back, fast mode, fast forward or slow
motion (see [Speed and time](time.md)).

## The Video page

```{index} Video page
```

The **Video** page of **Options** has four groups:

- **Picture** - **Border**, **Palette** with the {icon}`edit` button for the
  **Palette Editor**, **Shader**, and on the right the window scale (**Scale x1** to
  **Scale x6**), **Fullscreen**, **Keep ratio** and **Gray scale**. The scale,
  **Fullscreen**, **Keep ratio**, the palette and the shader are the same settings as in
  the **View** menu.
- **Anti-flicker** - see [Anti-flicker](#anti-flicker).
- **Screenshots** and **Recording** - see [Screenshots and recording](capture.md).
