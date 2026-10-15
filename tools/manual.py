#!/usr/bin/env python3
"""Build the user manual (docs/manual) and keep its translations.

    python tools/manual.py update [--add LANG]  English text -> .po of every language
    python tools/manual.py html [--dev]         build/manual/<lang>/ for every language;
                                                --dev: a note on every page that it is main's
    python tools/manual.py pdf                  build/manual-pdf/manual-<lang>.pdf
    python tools/manual.py check                offline search finds docs/manual/checks.txt
    python tools/manual.py status               how much of each language is translated
    python tools/manual.py shots [NAME...]      screenshots from docs/manual/shots/*.json

Needs the packages in docs/manual/requirements.txt; pdf and check need Chrome,
Chromium or Edge (or CHROME=<path>). English is the source, every other language
is a directory under docs/manual/locale.
"""

import argparse
import os
import re
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path
from urllib.parse import quote

ROOT = Path(__file__).resolve().parent.parent
SRC = ROOT / "docs" / "manual"
LOCALE = SRC / "locale"
BUILD = ROOT / "build"
OUT = BUILD / "manual"


def languages():
    langs = ["en"]
    if LOCALE.is_dir():
        langs += sorted(d.name for d in LOCALE.iterdir() if (d / "LC_MESSAGES").is_dir())
    return langs


def run(*cmd):
    print("+", " ".join(str(c) for c in cmd), flush=True)
    if subprocess.run([str(c) for c in cmd]).returncode:
        sys.exit(f"failed: {cmd[2] if cmd[1] == '-m' else cmd[0]}")


def sphinx(builder, lang, dst):
    # -W: a warning is a broken link or a missing picture, not something to ship
    run(sys.executable, "-m", "sphinx", "-q", "-W", "--keep-going", "-b", builder,
        "-D", f"language={lang}", "-d", BUILD / "manual-doctrees" / builder / lang, SRC, dst)


def find_browser():
    env = os.environ.get("CHROME")
    if env:
        return env
    names = ["google-chrome", "google-chrome-stable", "chromium", "chromium-browser", "chrome", "msedge"]
    for name in names:
        path = shutil.which(name)
        if path:
            return path
    for path in [
        r"C:\Program Files\Google\Chrome\Application\chrome.exe",
        r"C:\Program Files (x86)\Google\Chrome\Application\chrome.exe",
        r"C:\Program Files (x86)\Microsoft\Edge\Application\msedge.exe",
        "/Applications/Google Chrome.app/Contents/MacOS/Google Chrome",
        "/Applications/Chromium.app/Contents/MacOS/Chromium",
    ]:
        if os.path.exists(path):
            return path
    sys.exit("no Chrome, Chromium or Edge found; set CHROME to one")


def browser(*args, capture=False):
    with tempfile.TemporaryDirectory() as prof:
        cmd = [find_browser(), "--headless=new", "--disable-gpu", f"--user-data-dir={prof}"]
        if sys.platform.startswith("linux"):
            cmd.append("--no-sandbox")
        res = subprocess.run(cmd + list(args), check=True, timeout=120,
            stdout=subprocess.PIPE if capture else subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        return res.stdout.decode("utf-8", "replace") if capture else None


def cmd_update(opts):
    pot = BUILD / "manual-gettext"
    if pot.exists():
        shutil.rmtree(pot)
    sphinx("gettext", "en", pot)
    langs = [l for l in languages() if l != "en"]
    if opts.add and opts.add not in langs:
        langs.append(opts.add)
    for lang in langs:
        old = translations(lang)
        run(sys.executable, "-m", "sphinx_intl", "update", "-p", pot, "-d", LOCALE, "-l", lang)
        carry_icons(lang, old)


# An icon is part of its paragraph's text, so a button that gets another icon would put every
# paragraph naming it out of date in each language, with nothing to translate. A paragraph
# whose English changed only in its {icon} roles takes the same change in its translation.
ICON = re.compile(r"\{icon\}`([^`]+)` ?")


def translations(lang):
    """{English with the icons taken out: (English, translation)} of what is translated."""
    from babel.messages.pofile import read_po
    found = {}
    for po in (LOCALE / lang / "LC_MESSAGES").glob("*.po"):
        with open(po, "rb") as f:
            for msg in read_po(f):
                if msg.id and msg.string and not msg.fuzzy:
                    found[ICON.sub("", msg.id)] = (msg.id, msg.string)
    return found


def icons_moved(old_id, new_id, text):
    """text with the icons of old_id changed as new_id has them, or None."""
    was, now = ICON.findall(old_id), ICON.findall(new_id)
    if sorted(ICON.findall(text)) != sorted(was):
        return None
    if len(was) == len(now):
        # the same places: each old name becomes the new name in its place
        swap = {}
        for a, b in zip(was, now):
            if swap.setdefault(a, b) != b:
                return None
        return ICON.sub(lambda m: f"{{icon}}`{swap[m.group(1)]}`" + m.group(0)[len(m.group(1)) + 8:],
                        text)
    # an icon added or dropped: each one goes before the same words as in the English,
    # which a screen name in bold, English in every language, makes possible
    bare = ICON.sub("", text)
    for m in ICON.finditer(new_id):
        rest = new_id[m.end():]
        anchor = re.match(r"\*\*[^*]+\*\*|\S+", rest)
        if not anchor or bare.count(anchor.group(0)) != 1:
            return None
        bare = bare.replace(anchor.group(0), m.group(0) + anchor.group(0), 1)
    return bare


def carry_icons(lang, old):
    from babel.messages.pofile import read_po, write_po
    carried = 0
    for po in (LOCALE / lang / "LC_MESSAGES").glob("*.po"):
        with open(po, "rb") as f:
            cat = read_po(f)
        changed = False
        for msg in cat:
            if not msg.id or (msg.string and not msg.fuzzy):
                continue
            prev = old.get(ICON.sub("", msg.id))
            text = prev and icons_moved(prev[0], msg.id, prev[1])
            if text:
                msg.string = text
                msg.flags.discard("fuzzy")
                msg.previous_id = []
                changed = True
                carried += 1
        if changed:
            with open(po, "wb") as f:
                write_po(f, cat, width=76)
    if carried:
        print(f"{lang}: {carried} translations kept, their English changed only in icons")


def cmd_html(opts):
    if opts.dev:
        os.environ["XPECCY_MANUAL_DEV"] = "1"     # conf.py's note, read by each sphinx run
    # what is here goes into the package as it is, so nothing left from an older build;
    # the folder itself stays, as a window open on it would refuse its removal
    if OUT.exists():
        for p in OUT.iterdir():
            shutil.rmtree(p) if p.is_dir() else p.unlink()
    for lang in languages():
        dst = OUT / lang
        sphinx("html", lang, dst)
    # the online copy is served from the top of this folder: the reader's language
    # when the manual has it, as Help > Manual does with the system's; English without JS
    langs = ",".join(f'"{l}"' for l in languages())
    (OUT / "index.html").write_text(
        '<!doctype html><meta charset="utf-8"><title>Xpeccy+ Manual</title>\n'
        f'<script>var l=[{langs}],w=(navigator.languages||[navigator.language||"en"])'
        '.map(function(s){return s.slice(0,2).toLowerCase()}).filter(function(s){'
        'return l.indexOf(s)>=0})[0]||"en";location.replace(w+"/")</script>\n'
        '<meta http-equiv="refresh" content="1; url=en/"><a href="en/">Xpeccy+ Manual</a>\n',
        encoding="utf-8")


# not in the package: a long read, kept for whoever wants one at hand
def cmd_pdf(opts):
    for lang in languages():
        single = BUILD / "manual-single" / lang
        if single.exists():
            shutil.rmtree(single)
        sphinx("singlehtml", lang, single)
        (BUILD / "manual-pdf").mkdir(parents=True, exist_ok=True)
        pdf = BUILD / "manual-pdf" / f"manual-{lang}.pdf"
        browser("--no-pdf-header-footer", "--generate-pdf-document-outline",
            f"--print-to-pdf={pdf}", (single / "index.html").as_uri())
        print(f"  {pdf} ({pdf.stat().st_size // 1024} KB)")


def cmd_check(opts):
    # what a reader types into the search box, run from disk as a reader opens it
    bad = 0
    for ln in (SRC / "checks.txt").read_text(encoding="utf-8").splitlines():
        ln = ln.split("#")[0].strip()
        if not ln:
            continue
        lang, word = ln.split(None, 1)
        page = OUT / lang / "search.html"
        dom = browser("--virtual-time-budget=10000", "--dump-dom", page.as_uri() + "?q=" + quote(word), capture=True)
        found = dom.count('<li class="kind-')
        # the text around a hit in a page, which a page opened from disk only has from
        # xp-pages.js; a hit in the index names its page instead
        pages = dom.count('<li class="kind-text"') + dom.count('<li class="kind-title"')
        text = dom.count('<p class="context"')
        print(f"  {lang} {word!r}: {found} found, {pages} in pages, {text} with text")
        bad += found == 0 or text < pages
    if bad:
        sys.exit(f"{bad} searches found nothing")


def cmd_status(opts):
    from babel.messages.pofile import read_po
    for lang in languages()[1:]:
        done = fuzzy = todo = 0
        for po in sorted((LOCALE / lang / "LC_MESSAGES").rglob("*.po")):
            with open(po, "rb") as f:
                cat = read_po(f)
            for msg in cat:
                if not msg.id:
                    continue
                if msg.fuzzy:
                    fuzzy += 1
                elif msg.string:
                    done += 1
                else:
                    todo += 1
        total = done + fuzzy + todo
        print(f"{lang}: {done}/{total} translated, {fuzzy} out of date, {todo} not translated")


def cmd_shots(opts):
    sys.path.insert(0, str(ROOT / "tools"))
    sys.dont_write_bytecode = True      # no __pycache__ left in tools/
    import manual_shots
    manual_shots.main(SRC, opts.names)


def main():
    # the words checked and the pages are not all Latin; a Windows console is cp1252
    sys.stdout.reconfigure(encoding="utf-8", errors="replace")
    ap =argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = ap.add_subparsers(dest="cmd", required=True)
    p = sub.add_parser("update")
    p.add_argument("--add", metavar="LANG", help="start a new language")
    p = sub.add_parser("html")
    p.add_argument("--dev", action="store_true", help="the manual of main, not of a release")
    for name in ("pdf", "check", "status"):
        sub.add_parser(name)
    p = sub.add_parser("shots")
    p.add_argument("names", nargs="*")
    opts = ap.parse_args()
    globals()["cmd_" + opts.cmd](opts)


if __name__ == "__main__":
    main()
