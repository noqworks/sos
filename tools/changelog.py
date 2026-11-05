#!/usr/bin/env python3
"""Release notes for NOQ SOS, from releases.json (the single source).

    python3 tools/changelog.py                # writes CHANGELOG.md and CHANGELOG.tr.md
    python3 tools/changelog.py --check        # fails if firmware/noq-sos.yaml and releases.json disagree
    python3 tools/changelog.py --notes 1.0.0  # prints the GitHub release text for one version (English, then Turkish)

Add a release: put a new entry at the top of "releases" (version, date, summary, changes, compatibility,
credits), move the shipped items out of "next", bump "version" in firmware/noq-sos.yaml, run this script.
"""
import json
import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parent.parent
DATA = json.loads((ROOT / "releases.json").read_text(encoding="utf-8"))
AREAS = ("hardware", "firmware", "setup", "docs")

T = {
    "en": {
        "title": "Changelog",
        "intro": "All changes to {name}, newest first. Versions are MAJOR.MINOR.PATCH:",
        "rules": [
            "**Patch (1.0.x)**: fixes. Parts and firmware stay compatible.",
            "**Minor (1.x.0)**: new features or parts, compatible with earlier builds.",
            "**Major (2.0.0)**: changes that need new parts or new wiring.",
        ],
        "source": "Generated from `releases.json` by `tools/changelog.py`. Edit that file, not this one.",
        "next": "Next: {version}",
        "next_intro": "Planned, from requests and questions. Suggest something or tell us what you hit: [issues]({repo}/issues).",
        "areas": {"hardware": "Hardware", "firmware": "Firmware", "setup": "Setup", "docs": "Documentation"},
        "compat": "Compatibility",
        "credits": "Thanks",
        "files": "Files",
        "zip": "source and print files (zip)",
        "release": "GitHub release",
        "other": "Türkçe: [CHANGELOG.tr.md](CHANGELOG.tr.md)",
    },
    "tr": {
        "title": "Sürüm notları",
        "intro": "{name} için bütün değişiklikler, en yenisi üstte. Sürümler ANA.ARA.YAMA biçiminde:",
        "rules": [
            "**Yama (1.0.x)**: düzeltme. Parçalar ve yazılım uyumlu kalır.",
            "**Ara sürüm (1.x.0)**: yeni özellik ya da parça, önceki yapımlarla uyumlu.",
            "**Ana sürüm (2.0.0)**: yeni parça ya da yeni kablolama gerektiren değişiklik.",
        ],
        "source": "`releases.json` dosyasından `tools/changelog.py` ile üretilir. Bu dosyayı değil, onu düzenle.",
        "next": "Sıradaki: {version}",
        "next_intro": "Planlananlar; talep ve sorulardan. Önerin ya da takıldığın bir yer varsa: [konular]({repo}/issues).",
        "areas": {"hardware": "Donanım", "firmware": "Yazılım", "setup": "Kurulum", "docs": "Belgeler"},
        "compat": "Uyumluluk",
        "credits": "Teşekkürler",
        "files": "Dosyalar",
        "zip": "kaynak ve baskı dosyaları (zip)",
        "release": "GitHub sürümü",
        "other": "English: [CHANGELOG.md](CHANGELOG.md)",
    },
}


def by_area(items, lang):
    out = []
    for area in AREAS:
        rows = [i for i in items if i["area"] == area]
        if rows:
            out.append((T[lang]["areas"][area], [line(i, lang) for i in rows]))
    return out


def line(item, lang):
    text = item[lang]
    extra = []
    if item.get("by"):
        extra.append(", ".join(item["by"]))
    if item.get("ref"):
        extra.append(f"[#{item['ref'].rstrip('/').split('/')[-1]}]({item['ref']})")
    return text + (f" ({' · '.join(extra)})" if extra else "")


def release(r, lang, level="##"):
    t = T[lang]
    v = r["version"]
    md = [f"{level} {v} · {r['date']}", "", r["summary"][lang], ""]
    for title, rows in by_area(r["changes"], lang):
        md += [f"{level}# {title}", "", *[f"- {x}" for x in rows], ""]
    md.append(f"**{t['compat']}:** {r['compatibility'][lang]}")
    if r.get("credits"):
        md.append(f"**{t['credits']}:** {', '.join(r['credits'])}")
    md += [f"**{t['files']}:** [{t['zip']}]({DATA['files']}{DATA['product']}-{v}-source.zip) · "
           f"[{t['release']}]({DATA['repo']}/releases/tag/v{v})", ""]
    return md


def changelog(lang):
    t = T[lang]
    md = [f"# {t['title']}", "", t["other"], "", t["intro"].format(name=DATA["name"]), "", *[f"- {x}" for x in t["rules"]], "",
          f"_{t['source']}_", ""]
    nxt = DATA.get("next") or {}
    if nxt.get("items"):
        md += [f"## {t['next'].format(version=nxt['version'])}", "", t["next_intro"].format(repo=DATA["repo"]), ""]
        for title, rows in by_area(nxt["items"], lang):
            md += [f"### {title}", "", *[f"- {x}" for x in rows], ""]
    for r in DATA["releases"]:
        md += release(r, lang)
    return "\n".join(md).rstrip() + "\n"


def firmware_version():
    m = re.search(r'^\s*version:\s*"([^"]+)"', (ROOT / "firmware" / "noq-sos.yaml").read_text(encoding="utf-8"), re.M)
    return m.group(1) if m else None


def main():
    latest = DATA["releases"][0]["version"]
    if "--check" in sys.argv:
        fw = firmware_version()
        if fw != latest:
            sys.exit(f"firmware/noq-sos.yaml is {fw}, releases.json is {latest}")
        print(f"version {latest}")
        return
    if "--notes" in sys.argv:
        v = sys.argv[sys.argv.index("--notes") + 1]
        r = next((x for x in DATA["releases"] if x["version"] == v), None)
        if r is None:
            sys.exit(f"{v} is not in releases.json")
        print("\n".join(release(r, "en")[2:] + ["---", "", "**Türkçe**", ""] + release(r, "tr")[2:]).rstrip())
        return
    (ROOT / "CHANGELOG.md").write_text(changelog("en"), encoding="utf-8")
    (ROOT / "CHANGELOG.tr.md").write_text(changelog("tr"), encoding="utf-8")
    print(f"CHANGELOG.md, CHANGELOG.tr.md · {len(DATA['releases'])} release(s), latest {latest}")


if __name__ == "__main__":
    main()
