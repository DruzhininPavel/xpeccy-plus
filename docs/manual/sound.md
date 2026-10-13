# Sound

```{index} sound
```

What you hear comes from the machine's own sound devices - the beeper, the tape, and
whatever sound chips and cards the machine has. Which devices a machine has is set on
the **Machine** page of **Options**; how the sound is played and how loud each device is
are on the **Sound** page.

## Volume and mute

```{index} Mute, volume
```

{icon}`speaker` **Mute** on the toolbar, or **Machine > Mute**, silences the sound and
brings it back. The arrow beside the **Mute** button opens a slider for the volume (see
[Toolbar](main-window.md#toolbar)).

Alt with the mouse wheel over the picture turns the volume up or down by 5%. The slider
under the **Mute** button shows the new volume for a moment; when the toolbar is hidden,
the picture shows it. While the mouse is grabbed, the wheel goes to the machine instead.

This volume is the same as **Master** on the **Sound** page, and both mute and volume
are kept for the next start.

## The Sound page

```{index} Sound page, Driver, Latency, Auto adjust, sound; latency
```

**Options > Sound** has three groups.

**Output** - how the sound reaches your speakers:

- **Driver** - **SDL** plays the sound; **NULL** plays nothing.
- **Rate** - the sample rate: **Auto** takes the rate your computer's sound device
  uses and shows which one it is, or **48000** or **44100**.
- **Latency** - how much sound is kept ready ahead of playback, from 10 to 150 ms. Less
  makes the sound follow the picture and the keys more closely; too little and the
  sound clicks. With **Auto adjust** ticked (the default) the program finds the value
  by itself: it raises the latency when the sound runs short and lowers it again after a
  long clean spell. The slider then shows the value it has settled on.

```{index} Low cut (DC), High cut (anti-alias), anti-alias filter
```

The group beside it has three switches:

- **Enabled** - with it off, the program plays silence. The machine still runs at its
  normal speed.
- **Low cut (DC)** - takes out the lowest frequencies. The chips' output sits above zero
  rather than around it, and that offset only takes loudness away from the rest of the
  sound. On out of the box.
- **High cut (anti-alias)** - takes out the frequencies too high for the output rate.
  Without it they come back as false notes, which is heard most in beeper music as a
  harsh crunch. On out of the box; it costs a little CPU time.

```{index} Volumes, Master
```

**Volumes** - one slider for each device, from 0 to 100:

| Volume | What it sets |
|---|---|
| **Master** | everything together; the same as the toolbar's volume |
| **Beeper** | the machine's own speaker |
| **Tape** | the tape, while it plays or records |
| **PSG** | the AY or YM sound chips |
| **DAC** | Covox and Soundrive |
| **General Sound** | the General Sound card |
| **SAA1099** | the SAA1099 chip |

The **Sound** page is the same for every machine.

## Sound devices of a machine

```{index} sound devices
```

The **Sound** group on the **Machine** page of **Options** lists the sound devices of
the running machine. Each machine keeps its own: a change here goes with this machine
only. The button with the {icon}`settings` gear at the end of a row opens the device's
further settings. See [The Machine page](machines.md#the-machine-page) for the rest of the
page.

### PSG

```{index} PSG, AY, YM, TurboSound, TurboSound FM
```

```{index} see: sound chip; PSG
```

The sound chips most music on the Spectrum is written for. The **PSG** row says how many:

| Choice | What you get |
|---|---|
| **None** | no sound chip |
| **x1 - AY/YM** | one chip, three voices - the 128K machines and most clones |
| **x2 - TurboSound** | two chips, six voices, for music written for TurboSound |
| **x2 - TurboSound FM** | two YM2203 chips: TurboSound with FM synthesis, for music written for TurboSound FM |
| **x3 - ZX Next** | three chips, nine voices |

A program written for one chip plays on the first chip of two or three, and sounds the
same.

```{index} Machine: PSG, AY-3-8910, YM2149, YM2203, PSG; clock, PSG; stereo, Separation
```

The {icon}`settings` gear opens **Machine: PSG**:

- **Chip** - **AY-3-8910** or **YM2149**. The two sound alike; the YM2149 plays volume
  envelopes in finer steps. **YM2203** is the chip of TurboSound FM and is chosen with it.
- **Clock** - the chip's clock in MHz, which sets the pitch. **Auto** is half the
  machine's base CPU clock, as the boards divide it, and the list shows what that comes
  to. **1.773447** is the clock of the ZX Spectrum 128, +2 and +3; **1.75** that of
  most clones. Music written for one of them sounds a little out of tune on the other.
  You can also type a clock of your own.
- **Stereo** - **Mono**, or the order of the three voices from left to right: **ABC**,
  **ACB**, **BAC**, **BCA**, **CAB**, **CBA**. **ABC** puts the second voice in the
  middle; **ACB** puts the third voice there.
- **Separation** - how far apart the left and right sides are: 100% keeps them fully
  apart, 0% is mono. It is grayed out with **Mono**.

### DAC

```{index} DAC, Covox, Soundrive
```

A DAC plays digitized sound - speech, drums and samples - written to a port byte by
byte:

- **None** - no DAC;
- **Covox only** - one mono channel on port #FB;
- **Soundrive 1.05 mode 1** - four channels, two on each side, on ports #0F, #1F (left)
  and #4F, #5F (right);
- **Soundrive 1.05 mode 2** - the same four channels on ports #F1, #F3 (left) and #F9,
  #FB (right).

Pick the one the program is written for.

### General Sound

```{index} General Sound
```

A sound card with a processor and memory of its own, which plays music modules and
samples for the programs written for it. **On** puts it in the machine. The
{icon}`settings` gear opens **Machine: General Sound**:

- **ROM** - the card's own ROM, picked from the list or from a file with the
  {icon}`fileopen` folder button;
- **Reset** - the card is reset together with the machine.

### SAA1099

```{index} SAA1099
```

A sound chip with six voices, found on some sound cards for the Spectrum. **On** puts it
in the machine.

## Beeper

```{index} beeper, MIC in the beeper
```

The beeper is the machine's own speaker, and every machine has it. Its volume is
**Beeper** on the **Sound** page. **MIC in the beeper**, in the **ULA** group of
**Machine: Advanced Settings**, mixes in the signal the machine sends to the tape too,
a tenth as loud, as a machine with a ULA does (see
[Advanced settings](machines.md#advanced-settings)).

## Recording the sound

```{index} Record Sound to WAV, sound; recording
```

**Capture > Record Sound to WAV...** writes what you hear to a file; see
[Screenshots and recording](capture.md).
