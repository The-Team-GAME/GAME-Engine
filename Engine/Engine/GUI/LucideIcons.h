//-----------------------------------------------------------------------------------
// file:    LucideIcons.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   C++ binding for Lucide icon font codepoints. Maps icon names to
//          Unicode codepoints for use with ImGui and the Lucide TTF font.
//          Generated from the Lucide font (lucide.ttf) glyph table.
//
//          Usage:
//            ImGui::PushFont(icon_font);
//            ImGui::Text(LUCIDE_ICON_MOVE);
//            ImGui::PopFont();
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

// Lucide font glyph range (for ImGui font loading)
#define LUCIDE_ICON_MIN 0xE000
#define LUCIDE_ICON_MAX 0xE6B1

// Helper: compute UTF-8 bytes for a 3-byte codepoint (U+0800..U+FFFF) at compile time.
// Uses unsigned char to avoid MSVC C4310 truncation warnings on signed char.
#define _LUCIDE_B0(cp) static_cast<unsigned char>(0xE0 | (((cp) >> 12) & 0x0F))
#define _LUCIDE_B1(cp) static_cast<unsigned char>(0x80 | (((cp) >> 6)  & 0x3F))
#define _LUCIDE_B2(cp) static_cast<unsigned char>(0x80 | ((cp)         & 0x3F))

// Produce a constexpr const char[4] from a codepoint
#define LUCIDE_ICON(name, cp) \
  inline const char name[] = { \
    static_cast<char>(_LUCIDE_B0(cp)), \
    static_cast<char>(_LUCIDE_B1(cp)), \
    static_cast<char>(_LUCIDE_B2(cp)), \
    '\0' }

// ============================================================================
// Gizmo tool icons
// ============================================================================
#define LUCIDE_CP_MOVE            0xE121  // move (four-way arrow)
#define LUCIDE_CP_CROSSHAIR       0xE0AC  // crosshair (axis-constrained move)
#define LUCIDE_CP_ROTATE_CW       0xE149  // rotate-cw
#define LUCIDE_CP_SCALING         0xE2EC  // scaling (resize/scale)

// Icon string constants for ImGui::Text / ImGui::Button
#pragma warning(push)
#pragma warning(disable: 4310) // cast truncates constant value (unsigned -> signed char is intentional for UTF-8)
namespace LucideIcons {
  // Gizmo tools
  LUCIDE_ICON(Move,         0xE121);
  LUCIDE_ICON(Crosshair,    0xE0AC);
  LUCIDE_ICON(RotateCw,     0xE149);
  LUCIDE_ICON(Scaling,      0xE2EC);

  // General purpose icons (add more as needed)
  LUCIDE_ICON(Grab,         0xE1E6);
  LUCIDE_ICON(Hand,         0xE1D7);
  LUCIDE_ICON(Maximize,     0xE113);
  LUCIDE_ICON(Scale,        0xE212);
  LUCIDE_ICON(RotateCcw,    0xE148);
  LUCIDE_ICON(ArrowUp,      0xE04A);
  LUCIDE_ICON(ArrowDown,    0xE042);
  LUCIDE_ICON(ArrowLeft,    0xE048);
  LUCIDE_ICON(ArrowRight,   0xE049);
  LUCIDE_ICON(MousePointer, 0xE11F);
  LUCIDE_ICON(Pointer,      0xE1E8);
  LUCIDE_ICON(Expand,       0xE21A);
  LUCIDE_ICON(Cross,        0xE1E5);
  LUCIDE_ICON(Move3D,       0xE2E5);
  LUCIDE_ICON(Rotate3D,     0xE2EA);
  LUCIDE_ICON(Scale3D,      0xE2EB);
}
#pragma warning(pop)
