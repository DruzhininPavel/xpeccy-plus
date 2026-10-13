# The user manual

The text a user reads from Help > Manual, from the package or on GitHub Pages. English
is the source: one Markdown (MyST) page per chapter here. Every other language is a set
of `.po` files under `locale/<lang>/LC_MESSAGES/`, one entry per paragraph.

```
pip install -r docs/manual/requirements.txt
python tools/manual.py html      # build/manual/<lang>/index.html
python tools/manual.py pdf       # build/manual-pdf/manual-<lang>.pdf, not packaged (needs Chrome or Edge)
python tools/manual.py check     # offline search finds the words in checks.txt
```

**Changing the English text.** Edit the page, then `python tools/manual.py update`: every
paragraph that changed is marked out of date (`#, fuzzy`) in each language and shows in
English there until it is translated again. `python tools/manual.py status` counts them.
A paragraph whose English changed only in its `{icon}` roles (a button got another icon, or
an icon was put before a name) is not: `update` makes the same change in its translations.

**Translating.** Fill `msgstr` of each entry; remove `#, fuzzy` once an entry is right.
Names of windows, menus and buttons stay as on screen - the program itself is English.
A new language: `python tools/manual.py update --add xx` (the ISO 639-1 code, the one
the system reports - Ukrainian is `uk`), and its name and title in `LANG_NAMES` and
`TITLES` of `conf.py`. Help > Manual and the Pages front page pick it by that code.

**Before a release.** The English is kept up to date with each change; the translations
catch up once: `update`, then translate what `status` counts as out of date or not
translated in every language, keeping each entry's markup (`**`, `` ` ``, links,
`{icon}`) as the English has it, then `html` and `check`.

**Pictures.** `images/` holds the pictures the pages show, one set for every language: the
numbers on them are explained in the text. Each is made by `python tools/manual.py shots`
from its description in `shots/<name>.json` (Windows; see `tools/manual_shots.py`).
Some show a game that may be downloaded (Exolon and Cybernoid from Spectrum Computing, Fire
'n Ice too, Uwol from the TS-Labs catalogue): the repository does not carry those, so `shots`
downloads them once into `build/manual-files` with curl and checks their sha256. A shot runs
with the master volume at 0, as the hidden desktop hides the window but not the sound.

**Icons.** A button the reader sees as an icon gets its icon before its name:
{icon}`stop` **Breakpoints** shows `images/stop.png` of the repository, the file the program
uses (find it in the button's `.ui` or `setIcon` call), as tall as the text. Put it in the
row or list item that describes the button and at its first mention in a section.

**Index.** A block like this after a heading puts the terms that section explains into
the index - commas between terms, `;` before a sub-entry. Name a control as the program
does, a concept in lower case.

````
```{index} Rewind, Rewind; Step and History
```
````

**Style.** American English. Say what a thing does and where it is, in short sentences.
Name controls in bold, exactly as the program shows them: **Tape Player**, **Debugger**.
