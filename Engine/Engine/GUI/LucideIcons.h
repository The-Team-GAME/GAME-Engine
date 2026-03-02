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

// Helper macro: converts a codepoint to a UTF-8 string literal
// Lucide codepoints fall in U+E000..U+EFFF (3-byte UTF-8: 0xEE 0x80..0xBF 0x80..0xBF)
#define LUCIDE_ICON_STR(cp) _LUCIDE_UTF8_3(cp)
#define _LUCIDE_UTF8_3(cp) _LUCIDE_MAKE_3( \
    (char)(0xE0 | (((cp) >> 12) & 0x0F)), \
    (char)(0x80 | (((cp) >> 6)  & 0x3F)), \
    (char)(0x80 | ((cp)         & 0x3F)))
#define _LUCIDE_MAKE_3(a, b, c) { a, b, c, 0 }

// ============================================================================
// Gizmo tool icons
// ============================================================================
#define LUCIDE_CP_MOVE            0xE121  // move (four-way arrow)
#define LUCIDE_CP_CROSSHAIR       0xE0AC  // crosshair (axis-constrained move)
#define LUCIDE_CP_ROTATE_CW       0xE149  // rotate-cw
#define LUCIDE_CP_SCALING         0xE2EC  // scaling (resize/scale)

// String literals for ImGui::Text / ImGui::Button
// (must be constexpr arrays, not macros expanding to initializer lists)
namespace LucideIcons {
  // Gizmo tools
  inline constexpr char Move[]      = { (char)(0xE0 | ((0xE121 >> 12) & 0x0F)), (char)(0x80 | ((0xE121 >> 6) & 0x3F)), (char)(0x80 | (0xE121 & 0x3F)), 0 };
  inline constexpr char Crosshair[] = { (char)(0xE0 | ((0xE0AC >> 12) & 0x0F)), (char)(0x80 | ((0xE0AC >> 6) & 0x3F)), (char)(0x80 | (0xE0AC & 0x3F)), 0 };
  inline constexpr char RotateCw[]  = { (char)(0xE0 | ((0xE149 >> 12) & 0x0F)), (char)(0x80 | ((0xE149 >> 6) & 0x3F)), (char)(0x80 | (0xE149 & 0x3F)), 0 };
  inline constexpr char Scaling[]   = { (char)(0xE0 | ((0xE2EC >> 12) & 0x0F)), (char)(0x80 | ((0xE2EC >> 6) & 0x3F)), (char)(0x80 | (0xE2EC & 0x3F)), 0 };

  // General purpose icons (add more as needed)
  inline constexpr char Grab[]         = { (char)(0xE0 | ((0xE1E6 >> 12) & 0x0F)), (char)(0x80 | ((0xE1E6 >> 6) & 0x3F)), (char)(0x80 | (0xE1E6 & 0x3F)), 0 };
  inline constexpr char Hand[]         = { (char)(0xE0 | ((0xE1D7 >> 12) & 0x0F)), (char)(0x80 | ((0xE1D7 >> 6) & 0x3F)), (char)(0x80 | (0xE1D7 & 0x3F)), 0 };
  inline constexpr char Maximize[]     = { (char)(0xE0 | ((0xE113 >> 12) & 0x0F)), (char)(0x80 | ((0xE113 >> 6) & 0x3F)), (char)(0x80 | (0xE113 & 0x3F)), 0 };
  inline constexpr char Scale[]        = { (char)(0xE0 | ((0xE212 >> 12) & 0x0F)), (char)(0x80 | ((0xE212 >> 6) & 0x3F)), (char)(0x80 | (0xE212 & 0x3F)), 0 };
  inline constexpr char RotateCcw[]    = { (char)(0xE0 | ((0xE148 >> 12) & 0x0F)), (char)(0x80 | ((0xE148 >> 6) & 0x3F)), (char)(0x80 | (0xE148 & 0x3F)), 0 };
  inline constexpr char ArrowUp[]      = { (char)(0xE0 | ((0xE04A >> 12) & 0x0F)), (char)(0x80 | ((0xE04A >> 6) & 0x3F)), (char)(0x80 | (0xE04A & 0x3F)), 0 };
  inline constexpr char ArrowDown[]    = { (char)(0xE0 | ((0xE042 >> 12) & 0x0F)), (char)(0x80 | ((0xE042 >> 6) & 0x3F)), (char)(0x80 | (0xE042 & 0x3F)), 0 };
  inline constexpr char ArrowLeft[]    = { (char)(0xE0 | ((0xE048 >> 12) & 0x0F)), (char)(0x80 | ((0xE048 >> 6) & 0x3F)), (char)(0x80 | (0xE048 & 0x3F)), 0 };
  inline constexpr char ArrowRight[]   = { (char)(0xE0 | ((0xE049 >> 12) & 0x0F)), (char)(0x80 | ((0xE049 >> 6) & 0x3F)), (char)(0x80 | (0xE049 & 0x3F)), 0 };
  inline constexpr char MousePointer[] = { (char)(0xE0 | ((0xE11F >> 12) & 0x0F)), (char)(0x80 | ((0xE11F >> 6) & 0x3F)), (char)(0x80 | (0xE11F & 0x3F)), 0 };
  inline constexpr char Pointer[]      = { (char)(0xE0 | ((0xE1E8 >> 12) & 0x0F)), (char)(0x80 | ((0xE1E8 >> 6) & 0x3F)), (char)(0x80 | (0xE1E8 & 0x3F)), 0 };
  inline constexpr char Expand[]       = { (char)(0xE0 | ((0xE21A >> 12) & 0x0F)), (char)(0x80 | ((0xE21A >> 6) & 0x3F)), (char)(0x80 | (0xE21A & 0x3F)), 0 };
  inline constexpr char Cross[]        = { (char)(0xE0 | ((0xE1E5 >> 12) & 0x0F)), (char)(0x80 | ((0xE1E5 >> 6) & 0x3F)), (char)(0x80 | (0xE1E5 & 0x3F)), 0 };
  inline constexpr char Move3D[]       = { (char)(0xE0 | ((0xE2E5 >> 12) & 0x0F)), (char)(0x80 | ((0xE2E5 >> 6) & 0x3F)), (char)(0x80 | (0xE2E5 & 0x3F)), 0 };
  inline constexpr char Rotate3D[]     = { (char)(0xE0 | ((0xE2EA >> 12) & 0x0F)), (char)(0x80 | ((0xE2EA >> 6) & 0x3F)), (char)(0x80 | (0xE2EA & 0x3F)), 0 };
  inline constexpr char Scale3D[]      = { (char)(0xE0 | ((0xE2EB >> 12) & 0x0F)), (char)(0x80 | ((0xE2EB >> 6) & 0x3F)), (char)(0x80 | (0xE2EB & 0x3F)), 0 };
}
