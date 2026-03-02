"""
gen_lucide_icons.py  –  Generate Engine/Engine/GUI/LucideIcons.h from lucide.ttf

Usage:
    python gen_lucide_icons.py

Requires: fonttools  (pip install fonttools)
"""

import re
from pathlib import Path

from fontTools.ttLib import TTFont

# ── Paths ──────────────────────────────────────────────────────────────────────
FONT_PATH = Path(__file__).parent / "Engine" / "Assets" / "fonts" / "lucide.ttf"
OUTPUT_PATH = Path(__file__).parent / "Engine" / "Engine" / "GUI" / "LucideIcons.h"


def kebab_to_pascal(name: str) -> str:
    """Convert a kebab-case glyph name to PascalCase C++ identifier.

    Examples:
        'arrow-down-left' -> 'ArrowDownLeft'
        'align-horizontal-distribute-center' -> 'AlignHorizontalDistributeCenter'
        'rotate-3d' -> 'Rotate3D'
    """
    parts = name.split("-")
    result = []
    for part in parts:
        # If the part is purely digits (e.g. '3d'), uppercase it
        if part.isdigit() or re.match(r"^\d+[a-zA-Z]+$", part):
            result.append(part.upper())
        else:
            result.append(part.capitalize())
    return "".join(result)


def cp_to_utf8_literal(cp: int) -> str:
    """Convert a codepoint to a C hex escape string literal.

    For U+E000..U+FFFF (3-byte UTF-8):
        byte0 = 0xE0 | (cp >> 12) & 0x0F
        byte1 = 0x80 | (cp >>  6) & 0x3F
        byte2 = 0x80 | (cp      ) & 0x3F

    Returns e.g. '"\\xee\\x84\\xa1"'
    """
    b0 = 0xE0 | ((cp >> 12) & 0x0F)
    b1 = 0x80 | ((cp >> 6) & 0x3F)
    b2 = 0x80 | (cp & 0x3F)
    return f'"\\x{b0:02x}\\x{b1:02x}\\x{b2:02x}"'


def generate_header(icons: list[tuple[int, str, str]]) -> str:
    """Return the full header file content.

    icons: list of (codepoint, original_name, PascalName)
    """
    min_cp = min(cp for cp, _, _ in icons)
    max_cp = max(cp for cp, _, _ in icons)

    # Compute alignment width for names
    max_name_len = max(len(pascal) for _, _, pascal in icons)

    lines = []
    lines.append(
        "//-----------------------------------------------------------------------------------"
    )
    lines.append("// file:    LucideIcons.h")
    lines.append("// project: Monkey Stole My GPA")
    lines.append("//")
    lines.append("// brief:   Auto-generated C++ bindings for Lucide icon font codepoints.")
    lines.append("//          DO NOT EDIT MANUALLY — regenerate with gen_lucide_icons.py")
    lines.append("//")
    lines.append(f"//          Total icons: {len(icons)}")
    lines.append(f"//          Codepoint range: U+{min_cp:04X} .. U+{max_cp:04X}")
    lines.append("//")
    lines.append("//          Usage:")
    lines.append("//            ImGui::PushFont(icon_font);")
    lines.append('//            ImGui::Text(LucideIcons::Move);')
    lines.append("//            ImGui::PopFont();")
    lines.append("//")
    lines.append(
        "// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved."
    )
    lines.append(
        "//-----------------------------------------------------------------------------------"
    )
    lines.append("#pragma once")
    lines.append("")
    lines.append("// Lucide font glyph range (for ImGui font loading)")
    lines.append(f"#define LUCIDE_ICON_MIN 0x{min_cp:04X}")
    lines.append(f"#define LUCIDE_ICON_MAX 0x{max_cp:04X}")
    lines.append("")
    lines.append("// clang-format off")
    lines.append("namespace LucideIcons {")

    # Group by first letter for readability
    current_letter = None
    for cp, original, pascal in icons:
        first = original[0].upper()
        if first != current_letter:
            current_letter = first
            lines.append(f"  // ── {first} {'─' * 60}")
        padding = " " * (max_name_len - len(pascal))
        utf8 = cp_to_utf8_literal(cp)
        lines.append(
            f"  inline const char {pascal}[]{padding} = {utf8};  // {original} (U+{cp:04X})"
        )

    lines.append("}")
    lines.append("// clang-format on")
    lines.append("")

    return "\n".join(lines)


def main():
    font = TTFont(str(FONT_PATH))
    cmap = font.getBestCmap()

    # Collect only PUA icons (U+E000+)
    icons = []
    seen_names: set[str] = set()

    for cp, glyph_name in sorted(cmap.items()):
        if cp < 0xE000:
            continue
        pascal = kebab_to_pascal(glyph_name)
        # Handle potential duplicates
        if pascal in seen_names:
            pascal = f"{pascal}_{cp:04X}"
        seen_names.add(pascal)
        icons.append((cp, glyph_name, pascal))

    header = generate_header(icons)
    OUTPUT_PATH.write_text(header, encoding="utf-8")
    print(f"Generated {OUTPUT_PATH} with {len(icons)} icons")


if __name__ == "__main__":
    main()
