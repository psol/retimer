#!/usr/bin/env python3
"""Génère components/render/font.c à partir de tools/font_source.txt."""
import pathlib

root = pathlib.Path(__file__).resolve().parent.parent
lines = (root / "tools/font_source.txt").read_text(encoding="utf-8").splitlines()

glyphs = []
i = 0
while i < len(lines):
    line = lines[i]
    if line.startswith("= "):
        char = line[2:].strip()
        char = " " if char == "' '" else char
        rows = lines[i + 1:i + 10]
        assert len(rows) == 9 and all(len(r) == 5 for r in rows), char
        values = [int(r.replace("#", "1").replace(".", "0"), 2) for r in rows]
        glyphs.append((ord(char), char, values))
        i += 10
    else:
        i += 1

glyphs.sort()
out = [
    "/* Généré par tools/font.py à partir de tools/font_source.txt. */",
    "/* Ne pas modifier à la main. */",
    '#include "font.h"',
    "",
    "#include <stddef.h>",
    "",
    "typedef struct {",
    "    uint32_t code;",
    "    uint8_t rows[FONT_HEIGHT];",
    "} glyph_t;",
    "",
    "static const glyph_t GLYPHS[] = {",
]
for code, char, values in glyphs:
    name = char if char not in "'\\" else "\\" + char
    out.append("    {0x%04X, {%s}}, /* %s */" % (
        code, ", ".join("0x%02X" % v for v in values), name))
out += [
    "};",
    "",
    "const uint8_t *font_glyph(uint32_t code)",
    "{",
    "    for (size_t i = 0; i < sizeof GLYPHS / sizeof GLYPHS[0]; i++) {",
    "        if (GLYPHS[i].code == code) {",
    "            return GLYPHS[i].rows;",
    "        }",
    "    }",
    "    return NULL;",
    "}",
    "",
]
(root / "components/render/font.c").write_text("\n".join(out), encoding="utf-8")
print(len(glyphs), "glyphes")
