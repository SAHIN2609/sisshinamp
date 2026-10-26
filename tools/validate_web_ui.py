#!/usr/bin/env python3
"""Checks that the embedded WebView interface remains syntactically valid.

Runs without npm dependencies. CI uses Node to parse every inline script as
well as the native JUCE event bridge, and verifies the original amp markup,
right-click suppression and studio hardware visuals.
"""
from html.parser import HTMLParser
from pathlib import Path
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]
HTML = ROOT / "Assets" / "interface.html"
BRIDGE = ROOT / "Assets" / "bridge.js"

class Scripts(HTMLParser):
    def __init__(self):
        super().__init__(convert_charrefs=False)
        self.parts = []
        self.active = False

    def handle_starttag(self, tag, attrs):
        if tag == "script":
            attrs = dict(attrs)
            self.active = "src" not in attrs
            if self.active:
                self.parts.append([])

    def handle_data(self, data):
        if self.active and self.parts:
            self.parts[-1].append(data)

    def handle_endtag(self, tag):
        if tag == "script":
            self.active = False

def main():
    markup = HTML.read_text(encoding="utf8")
    checks = {
        "Original amp models": "const AMPS=[['Sakura','CLEAN'],['Taki','METAL'],['Take','THICK']]" in markup,
        "Right-click disabled in DOM": 'oncontextmenu="return false;"' in markup,
        "Contextmenu capture installed": "document.addEventListener('contextmenu'" in markup,
        "No browser traffic lights": 'style="background:#ff5f57"' not in markup,
        "Realistic amp hardware": "amp-plaque" in markup and "amp-screw" in markup,
        "Selector mini-amps match full heads": all(
            name in markup for name in (
                "mini-grill", "mini-plate", "mini-handle", "mini-knobs",
                ".sel3 .m0 .mini-grill", ".sel3 .m1 .mini-grill",
                ".sel3 .m2 .mini-grill"
            )
        ),
        "Native bridge loaded": '<script src="bridge.js"></script>' in markup,
        "Factory cabinet library": 'id="irbanklist"' in markup,
        "HTML logo retained": "DEFLOGO=" in markup
    }
    for label, okay in checks.items():
        print(f"{'PASS' if okay else 'FAIL'}: {label}")
    if not all(checks.values()):
        return 1
    parser = Scripts()
    parser.feed(markup)
    with tempfile.TemporaryDirectory(prefix="sishhin-webtest-") as work:
        for i, fragments in enumerate(parser.parts):
            script = Path(work) / f"inline-{i}.js"
            script.write_text("".join(fragments), encoding="utf8")
            subprocess.run(["node", "--check", str(script)], check=True)
        subprocess.run(["node", "--check", str(BRIDGE)], check=True)
    print(f"PASS: Node parsed {len(parser.parts)} inline script(s) plus bridge.js")
    return 0

if __name__ == "__main__":
    sys.exit(main())
