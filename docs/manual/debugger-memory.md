# Memory

The Debugger shows memory byte by byte in the **DUMP** panel, and the bytes the registers
point at in **REG-DUMP**. **MEMMAP** shows which memory the CPU sees, and lets you page
in another. The tools around them find bytes, fill memory, show it as pictures, save it
to a file and load it back, and the **Heat map** shows which memory the program uses and
how.

## The DUMP panel

```{index} DUMP panel, Bytes per row, Code page
```

```{index} see: hex dump; DUMP panel
```

![The DUMP panel](images/dbg-dump.png)

1. **View** - **CPU**, **RAM** or **ROM**, and for the last two the page, its address
   and how much of it to show.
2. **Bytes per row** - **Auto**, **8/row**, **12/row** or **16/row**.
3. **Code page** - **WIN1251**, **CP866** or **KOI8R**: how bytes from 128 to 255 show as
   letters in the text column.
4. **Dump** - the address, the bytes in groups of four, and the same bytes as text.
5. **Scrollbar** - drag it to move through memory.

The title of the panel shows the address of the byte under the cursor: **DUMP | 00C000**.

In the text column a byte below 32 shows as a dot. **Auto** puts as many groups of four
bytes on a row as the panel is wide enough for.

### Moving around

```{index} Go to address, DUMP panel; go to address
```

| Key | What it does |
|---|---|
| Up, Down | the cursor one row; at the edge the dump scrolls by one row |
| PageUp, PageDown | a page back or on |
| mouse wheel | scroll by one row |

These keys are the same in both presets, and can be changed on the **Hotkeys** page of
**Options** (see [Debugger keys](hotkeys.md#debugger-keys)):

| Command | Key | Goes to |
|---|---|---|
| **Go to address** | G | the address cell of the top row opens for typing |
| **Go to (PC)** | Ctrl+P | the address in PC |
| **Go to (SP)** | Ctrl+S | the top of the stack |
| **Go to (BC)** | Ctrl+B | the address in BC |
| **Go to (DE)** | Ctrl+D | the address in DE |
| **Go to (HL)** | Ctrl+H | the address in HL |
| **Go to (IX)** | Ctrl+X | the address in IX |
| **Go to (IY)** | Ctrl+Y | the address in IY |

A right-click on a register's name in the **CPU** panel moves the dump to the address the
register holds.

To go to an address, double-click the address of any row, or select it and press Enter, and
type it. The dump scrolls so that the address lands on that row. An address can be hex
(`8000`, `#8000`, `0x8000`), a label, or a register with a dot in front, as `.HL`. The
labels are offered as you type, as in the listing (see
[Labels offered as you type](debugger-code.md#labels-offered-as-you-type)).

### Editing

```{index} DUMP panel; editing, Allow ROM wr
```

Double-click a byte, or select it and press Enter, and type the new value: two hex digits.
Enter writes it. The text column cannot be edited.

ROM is protected: a byte in ROM is written only with **Allow ROM wr** ticked in the
{icon}`settings` **Options** menu of the **Disasm** panel. A cartridge is never written.

### CPU, RAM and ROM

```{index} memory page, DUMP panel; RAM and ROM view
```

**CPU** shows the 64K the CPU sees now, with whatever pages are in it. **RAM** and **ROM**
show one 16K page, whether the CPU sees it or not:

- **Pg** - the number of the page;
- the address field - the address the first byte of the page is shown at: C000 for RAM
  and 0000 for ROM, as they are set when you switch the view;
- the size - how much of the page the dump goes through, from 256 bytes to the whole 16KB.

The rows then show the page and the address, as `07:C000`.

### Selecting and breakpoints

```{index} Fetch break, Read break, Write break
```

Drag over the bytes to select a block, or Ctrl+click its first byte and Shift+click its
last; a click with the middle button clears it. The **DUMP** and the **Disasm** listing
share one selection, so a block selected here is ready for **View**, **Fill...** and the
other block commands of the listing (see
[Selecting a block](debugger-code.md#selecting-a-block)). The selection shows in the
**CPU** view only.

A right-click on a byte opens a menu of **Fetch break**, **Read break** and **Write break**:
tick one to stop when the CPU runs, reads or writes that byte. See
[Breakpoints and watching](debugger-breakpoints.md).

## REG-DUMP

```{index} REG-DUMP panel
```

![The REG-DUMP panel](images/dbg-regdump.png)

**REG-DUMP** shares a place with the **DUMP**: click its tab. It has a row for each
register that can point into memory - PC, BC, DE, HL, SP, BC', DE', HL', IX and IY. Each
row starts with the register and its value, `HL (5CB0):`, and goes on with the bytes from
that address, as many as fit. It changes with every step, so you see at once what an
instruction is about to read. It cannot be edited.

## MEMMAP

```{index} MEMMAP panel; paging in, Restore mapping, paging
```

![The MEMMAP panel](images/dbg-memmap.png)

The four rows at the top of the **MEMMAP** panel are the four 16K windows of the CPU's
memory: 0000-3FFF, 4000-7FFF, 8000-BFFF and C000-FFFF. Hold the mouse over a row to see
its addresses. Each row says what is in that window now, **ROM** or **RAM**, and the
number of the page.

Change a row to put another page in: pick **ROM** or **RAM** and type the page number. The
**DUMP**, the listing and the CPU itself then see that page there. A window you changed has
its type in bold and its page number lit. It stays that way until the program pages that
window itself.

Right-click the rows, or the **MEMMAP** title, and choose **Restore mapping** to put back
all four windows as they were when the Debugger opened.

The rest of the panel - ports, signals, frames and the beam - is described in
[The Debugger](debugger.md).

## Memory Finder

```{index} Memory Finder, Search, Find pattern
```

![The Memory Finder](images/dbg-finder.png)

**Search** in the {icon}`other` **Tools** menu of the **Disasm** panel opens it, as does the
**Find pattern** key: Ctrl+F in both presets.

- **Bytes** - up to 8 bytes in hex, separated by colons: `3E:05:C9`.
- **Mask** - for each byte, the bits that must match: `FF` all of them, `00` none, so that
  any byte will do there, `0F` only the low four bits. Bytes with no mask must match whole.
- **Text** - up to 8 characters. Typing text fills **Bytes** with its codes, and typing
  bytes shows them here as text. Upper and lower case count.

{icon}`search` **Find** goes through the 64K the CPU sees, starting from the address at the
top of the listing. The line under the fields says **Found @** and the address, or
**Not found**. The listing and the **DUMP** go to the place found.

## Memory Filler

```{index} Memory Filler, Fill
```

![The Memory Filler](images/dbg-filler.png)

**Fill...** in the {icon}`other` **Tools** menu opens it. With a block selected, it starts
with the block's addresses.

- **Start address**, **End address** - the memory to fill, ends included.
- **Pattern** - up to 8 bytes, separated by colons. They repeat to the end.
- **Mask** - for **Mask** only: the bits of each byte that the pattern sets.
- How the pattern goes in:

| Method | Each byte becomes |
|---|---|
| **Mask** | the pattern in the bits of the mask, the old byte in the rest |
| **Put** | the pattern |
| **Or** | the old byte OR the pattern |
| **And** | the old byte AND the pattern |
| **Xor** | the old byte XOR the pattern |

{icon}`fill` **Fill** writes the memory the CPU sees and closes the window. ROM is written
only with **Allow ROM wr** ticked.

## Memory Sprite Scanner

```{index} Memory Sprite Scanner, Sprite scanner, sprite; finding
```

![The Memory Sprite Scanner](images/dbg-sprite-scanner.png)

**Sprite scanner** in the {icon}`other` **Tools** menu opens it. It shows memory as a
picture, one bit a pixel, so that sprites, fonts and screens can be found by eye. In the
picture it shows Exolon (Raffaele Cecco, Hewson, 1987): with **Width** 3, the frames of its
hero running stand one under another, and **Columns** puts the memory after them side by
side.

- **Page** - the RAM page read for the addresses from C000 to FFFF. It starts at the page
  the CPU has there.
- **Addr** - the first byte shown.
- **Width** - how many bytes make one line of the picture, 8 pixels each.
- **Height** - how many rows of 8 lines the picture has.
- **Inversion** - swaps black and white.
- **Columns** - when **Width** is narrow, the memory that follows is drawn in more columns
  side by side, in turn in another color, to fill the picture.
- **Grid** - shades every other 8 x 8 square, to see where the bytes and rows fall.
- **ZX screen** - reads the memory the way the ZX screen lays it out: 6144 bytes from
  **Addr**.

The scrollbar at the right and the mouse wheel move the address by one row of 8 lines.
The {icon}`floppy` button with the floppy disk saves the bytes shown - **Width** x
**Height** x 8 of them, or 6144 with **ZX screen** - to a file, as they are in memory.

The scanner stays open when you leave the Debugger, and comes back with it.

## Saving and loading memory

```{index} Save dump, Load dump, Save RAM Dump, Open RAM Dump, Hobeta
```

**Save dump** and **Load dump** are in the {icon}`floppy` floppy disk menu at the right of
the **Disasm** toolbar. Each has a key of its own: see
[Debugger keys](hotkeys.md#debugger-keys).

![The Save RAM Dump window](images/dbg-save-dump.png)

**Save RAM Dump** saves a part of the memory the CPU sees. Fill in **Start** and **End**,
or **Start** and **Length**, in hex: the third follows. Then pick from
{icon}`save_all` **Save as...**:

- **...as bin** - the bytes as they are, into a file you name;
- **...as Hobeta** - a `.$C` file: a TR-DOS code file named after the start address,
  that loads back to the same address;
- **...to disk A** to **...to disk D** - a code file on the TR-DOS disk in that drive.
  A drive with no disk gets a new, formatted one. Up to FF00 bytes go on a disk.

![The Open RAM Dump window](images/dbg-open-dump.png)

**Open RAM Dump** loads a file into memory. The {icon}`fileopen` button beside **File**
picks the file; a file can be up to FF00 bytes long. **Start** is where it goes, C000 to
begin with, and **Length** and **End** show how far it reaches. A file that would run past
FFFF is moved down to end there. {icon}`ok-apply` **OK** writes it into the memory the CPU
sees, ROM only with **Allow ROM wr** ticked.

## Heat map

```{index} Heat map, Heat map; CSV export, Collect
```

![The Heat map panel](images/dbg-heatmap.png)

1. What the picture covers - **CPU**, **RAM** or **ROM** - and the page for the last two.
2. The counter shown - **All**, **Read**, **Write** or **Exec**.
3. **Collect**, and the buttons that {icon}`refresh` reset the counters and
   {icon}`save_all` export them.
4. The picture.
5. The address under the mouse, and the legend with its counts.

The **Heat map** counts, for every byte of RAM and ROM, how many times the program has
read it, written it and run it. Click its tab, under the **DUMP**, and tick **Collect**:
counting starts and goes on while the machine runs, also with the Debugger closed, until
you untick it. With **Collect** off the picture says **collecting is off**.

Each counter has its color: **Read** green, **Write** red, **Exec** blue. A byte nothing
has touched is gray. With **All**, each cell takes the color of the counter that wins:
blue when it was run more often than read and written together, else red when written
more than read, else green. **Exec** counts every byte the CPU takes as part of an
instruction, operands included; **Read** counts only the data it reads.

So the picture reads like a map of the program: blue is code, red is what it changes - the
screen, the variables, the stack - and green is what it only reads, such as graphics,
fonts and tables.

**CPU** shows the 64K the CPU sees now as four squares, one for each 16K window, each cell
4 bytes. **RAM** and **ROM** show one 16K page, a cell a byte, and scroll when the panel is
too small.

Hold the mouse over a cell to see its address, the memory and page it is in, and its
counts: **Read:**, **Write:**, **Exec:**. Double-click a cell and the listing goes there,
when the CPU can see it.

The {icon}`refresh` button with the circular arrows resets all counters, after asking.
Loading a snapshot resets them too. The {icon}`save_all` button beside it saves all counters
to a CSV file: one line per byte of RAM and then of ROM, with the columns
`type,page,offset,read,write,exec`, untouched bytes included, for a spreadsheet or a script
of your own.

## Example: find where the score is kept

1. Play the game until the score shows a number you can recognize, such as 150.
2. Open the Debugger: **Debug > Debugger**.
3. Open the **Memory Finder** (**Tools > Search**). Many games keep the score as digits:
   type the digits in **Text** as the game shows them, leading zeros included, such as
   `000150`. Click {icon}`search` **Find**.
4. **Found @** gives the address, and the **DUMP** shows the digits there.
5. To check it, change a digit in the **DUMP** - `31` to `39` turns a 1 into a 9 - and
   close the Debugger. The next time the game draws its score, it shows the change.

If the text is not found, the game keeps the score in another form. Try the number as
bytes: `01:50` for 150 kept as decimal digits two to a byte, or `96:00` for 150 as a word,
low byte first. To find the code that adds the points, put a **Write break** on the score
(see [Breakpoints and watching](debugger-breakpoints.md)).

## Example: save the screen memory to a file

1. Open the Debugger: **Debug > Debugger**.
2. In the {icon}`floppy` floppy disk menu of the **Disasm** toolbar, choose **Save dump**.
3. Type `4000` in **Start** and `1B00` in **Length**. **End** shows `5AFF`.
4. Click {icon}`save_all` **Save as...** and choose **...as bin**. Name the file, such as
   `picture.scr`.

The file holds the 6912 bytes of the screen, the picture and then its colors, in the same
layout as a `.scr` screenshot (see [Screenshots and recording](capture.md)).

The second screen of a 128K is in RAM page 7. To save it, put page 7 in the C000 window in
**MEMMAP** and save from C000 instead; **Restore mapping** puts the memory back.
