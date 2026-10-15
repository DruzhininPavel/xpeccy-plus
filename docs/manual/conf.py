# Sphinx configuration of the user manual; built by tools/manual.py, which
# passes the language (-D language=xx). English is the source text, the other
# languages are .po files under locale/.

import json
import os
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parent

project = "Xpeccy+"
version = release = (ROOT.parent.parent / "VERSION").read_text().split()[0]
html_title = "Xpeccy+ Manual"

extensions = ["myst_parser"]
myst_enable_extensions = ["colon_fence", "deflist"]
myst_heading_anchors = 3

exclude_patterns = ["README.md", "locale", "shots", "requirements.txt"]
templates_path = ["_templates"]
html_static_path = ["_static"]
html_css_files = ["manual.css"]

language = "en"
locale_dirs = ["locale/"]
gettext_compact = False     # one .po per page, as the pages are split
gettext_location = False    # no line numbers: moving a paragraph is not a change to translate
# the index is sorted by the translated words. A picture's alt text is translated without
# "image", which would add each picture's path too: one set of pictures serves every language
gettext_additional_targets = ["index"]

html_theme = "furo"
html_logo = "../../images/icons/xpeccy-plus-128.png"
html_favicon = "../../images/icons/xpeccy-plus-32.png"
# the blue of the logo's stripes, darker on white to stay readable
html_theme_options = {
    "light_css_variables": {
        "color-brand-primary": "#00708c",
        "color-brand-content": "#00708c",
    },
    "dark_css_variables": {
        "color-brand-primary": "#3fc7e6",
        "color-brand-content": "#3fc7e6",
    },
}
html_show_copyright = False
html_show_sourcelink = False
html_copy_source = False
html_sidebars = {
    "**": [
        "sidebar/brand.html",
        "sidebar/search.html",
        "languages.html",
        "sidebar/scroll-start.html",
        "sidebar/navigation.html",
        "index-link.html",
        "sidebar/scroll-end.html",
    ]
}

# every language there is, by its own name, for the switcher in the sidebar
LANG_NAMES = {"en": "English", "ru": "Русский", "es": "Español", "uk": "Українська"}
LANGS = ["en"] + sorted(d.name for d in (ROOT / "locale").glob("*") if (d / "LC_MESSAGES").is_dir())


def page_context(app, pagename, templatename, context, doctree):
    # {{ release }} in a raw html line of a page: the version is no text to translate,
    # which would go out of date with every release
    if "body" in context:
        context["body"] = context["body"].replace("{{ release }}", release)
    # each language is a site of its own beside this one: <out>/<lang>/<page>.html
    up = "../" * (pagename.count("/") + 1)
    context["lang_links"] = [
        (LANG_NAMES.get(l, l), f"{up}{l}/{pagename}.html", l == app.config.language)
        for l in LANGS
    ]
    if pagename == "search" and app.builder.name == "html":
        app.add_js_file(PAGES_JS)
        app.add_js_file("search-offline.js")
    if "genindexentries" in context:
        link_see(context["genindexentries"])


# Sphinx writes an index "see" entry as plain text, "see floppy disk", which leaves the
# reader to look the term up. It gets the link of the term it names: the first place the
# index sends that term to.
def link_see(groups):
    from sphinx.locale import _
    forms = [re.escape(_(f)).replace(re.escape("%s"), "(.+)") for f in ("see %s", "see also %s")]
    see = re.compile("|".join(forms))
    first = {}
    for _letter, entries in groups:
        for name, (links, subs, _key) in entries:
            uri = links[0][1] if links else next((s[1][0][1] for s in subs if s[1]), None)
            if uri:
                first.setdefault(name, uri)
    for _letter, entries in groups:
        for _name, (_links, subs, _key) in entries:
            for i, (sub, sublinks) in enumerate(subs):
                m = not sublinks and see.fullmatch(sub)
                target = m and first.get(next(g for g in m.groups() if g))
                if target:
                    subs[i] = (sub, [(False, target)])


# The text under a search hit is the found page fetch()ed, which a browser refuses to a
# page opened from disk - the copy Help > Manual opens. So every page's text also goes
# into one script for search.html (_static/search-offline.js serves it).
PAGES_JS = "xp-pages.js"
MAIN = re.compile(r'<article role="main".*?</article>', re.S)


def write_pages(app, exception):
    if exception or app.builder.name != "html":
        return
    pages = {}
    for doc in sorted(app.env.found_docs):
        m = MAIN.search(Path(app.outdir, doc + ".html").read_text(encoding="utf-8"))
        if m:
            pages[doc] = m.group(0)
    js = "window.XP_PAGES = " + json.dumps(pages, ensure_ascii=False) + ";\n"
    Path(app.outdir, "_static", PAGES_JS).write_text(js, encoding="utf-8")


# {icon}`stop` - a button's icon as the program has it (images/stop.png of the repository),
# in the line before the button's name: a picture finds the button faster than its tooltip
def icon_role(name, rawtext, text, lineno, inliner, options=None, content=None):
    from docutils import nodes
    if not (ICONS / f"{text}.png").is_file():
        msg = inliner.reporter.error(f"no icon images/{text}.png", line=lineno)
        return [inliner.problematic(rawtext, rawtext, msg)], [msg]
    # the image path is relative to the page, and every page is in this folder
    return [nodes.image(rawtext, uri=f"../../images/{text}.png", alt="", classes=["xp-icon"])], []


ICONS = ROOT.parent.parent / "images"


# the title over the sidebar and in the tab is configuration, not text, so no .po has it
TITLES = {"ru": "Руководство Xpeccy+", "uk": "Посібник Xpeccy+", "es": "Manual de Xpeccy+"}


# the manual of main, on Pages under dev/, says so over every page: it may describe what
# the newest release does not have yet
DEV_NOTES = {
    "en": "This manual is for the nightly build: it may describe what the latest release does not have yet.",
    "ru": "Это руководство к ночной сборке: в нём может быть то, чего ещё нет в последнем релизе.",
    "uk": "Це посібник до нічної збірки: у ньому може бути те, чого ще немає в останньому релізі.",
    "es": "Este manual es de la versión nocturna: puede describir lo que la última versión publicada aún no tiene.",
}


def set_title(app, config):
    config.html_title = TITLES.get(config.language, html_title)
    if os.environ.get("XPECCY_MANUAL_DEV"):
        config.html_theme_options = dict(config.html_theme_options,
                                         announcement=DEV_NOTES.get(config.language, DEV_NOTES["en"]))


def setup(app):
    app.add_role("icon", icon_role)
    app.connect("config-inited", set_title)
    app.connect("html-page-context", page_context)
    app.connect("build-finished", write_pages)
