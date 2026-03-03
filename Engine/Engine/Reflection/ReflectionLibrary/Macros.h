//-----------------------------------------------------------------------------------
// file:    Macros.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   Macro definitions for the reflection library's code generation system.
//          Provides variadic macro expansion capabilities to automatically generate
//          reflection information for classes with up to 40 fields. Enables the
//          REFLECT macro system used throughout the engine.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

// Count arguments (supports 0 to 40)
// This version properly handles empty __VA_ARGS__
#define COUNT_ARGS(...) COUNT_ARGS_IMPL(0, ##__VA_ARGS__, 40, 39, 38, 37, 36, 35, 34, 33, 32, 31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0)
#define COUNT_ARGS_IMPL(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, N, ...) N

// Concatenation helper
#define CONCAT(A, B) CONCAT_IMPL(A, B)
#define CONCAT_IMPL(A, B) A##B

// Expand fields based on count
#define EXPAND_MACROS(CLASS, MACRO, N, ...) CONCAT(MACROS_EXPAND_, N)(CLASS, MACRO, ##__VA_ARGS__)

#define MACROS_EXPAND_0(C, M) /* Empty expansion for zero fields */
#define MACROS_EXPAND_1(C, M, F1) M(C, F1)
#define MACROS_EXPAND_2(C, M, F1, F2) MACROS_EXPAND_1(C, M, F1), M(C, F2)
#define MACROS_EXPAND_3(C, M, F1, F2, F3) MACROS_EXPAND_2(C, M, F1, F2), M(C, F3)
#define MACROS_EXPAND_4(C, M, F1, F2, F3, F4) MACROS_EXPAND_3(C, M, F1, F2, F3), M(C, F4)
#define MACROS_EXPAND_5(C, M, F1, F2, F3, F4, F5) MACROS_EXPAND_4(C, M, F1, F2, F3, F4), M(C, F5)
#define MACROS_EXPAND_6(C, M, F1, F2, F3, F4, F5, F6) MACROS_EXPAND_5(C, M, F1, F2, F3, F4, F5), M(C, F6)
#define MACROS_EXPAND_7(C, M, F1, F2, F3, F4, F5, F6, F7) MACROS_EXPAND_6(C, M, F1, F2, F3, F4, F5, F6), M(C, F7)
#define MACROS_EXPAND_8(C, M, F1, F2, F3, F4, F5, F6, F7, F8) MACROS_EXPAND_7(C, M, F1, F2, F3, F4, F5, F6, F7), M(C, F8)
#define MACROS_EXPAND_9(C, M, F1, F2, F3, F4, F5, F6, F7, F8, F9) MACROS_EXPAND_8(C, M, F1, F2, F3, F4, F5, F6, F7, F8), M(C, F9)
#define MACROS_EXPAND_10(C, M, F1, F2, F3, F4, F5, F6, F7, F8, F9, F10) MACROS_EXPAND_9(C, M, F1, F2, F3, F4, F5, F6, F7, F8, F9), M(C, F10)
#define MACROS_EXPAND_11(C, M, F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11) MACROS_EXPAND_10(C, M, F1, F2, F3, F4, F5, F6, F7, F8, F9, F10), M(C, F11)
#define MACROS_EXPAND_12(C, M, F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12) MACROS_EXPAND_11(C, M, F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11), M(C, F12)
#define MACROS_EXPAND_13(C, M, F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12, F13) MACROS_EXPAND_12(C, M, F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12), M(C, F13)
#define MACROS_EXPAND_14(C, M, F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12, F13, F14) MACROS_EXPAND_13(C, M, F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12, F13), M(C, F14)
#define MACROS_EXPAND_15(C, M, F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12, F13, F14, F15) MACROS_EXPAND_14(C, M, F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12, F13, F14), M(C, F15)
#define MACROS_EXPAND_16(C, M, F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12, F13, F14, F15, F16) MACROS_EXPAND_15(C, M, F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12, F13, F14, F15), M(C, F16)
#define MACROS_EXPAND_17(C, M, F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12, F13, F14, F15, F16, F17) MACROS_EXPAND_16(C, M, F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12, F13, F14, F15, F16), M(C, F17)
#define MACROS_EXPAND_18(C, M, F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12, F13, F14, F15, F16, F17, F18) MACROS_EXPAND_17(C, M, F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12, F13, F14, F15, F16, F17), M(C, F18)
#define MACROS_EXPAND_19(C, M, F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12, F13, F14, F15, F16, F17, F18, F19) MACROS_EXPAND_18(C, M, F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12, F13, F14, F15, F16, F17, F18), M(C, F19)
#define MACROS_EXPAND_20(C, M, F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12, F13, F14, F15, F16, F17, F18, F19, F20) MACROS_EXPAND_19(C, M, F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12, F13, F14, F15, F16, F17, F18, F19), M(C, F20)
#define MACROS_EXPAND_21(C, M, F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12, F13, F14, F15, F16, F17, F18, F19, F20, F21) MACROS_EXPAND_20(C, M, F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12, F13, F14, F15, F16, F17, F18, F19, F20), M(C, F21)
#define MACROS_EXPAND_22(C, M, F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12, F13, F14, F15, F16, F17, F18, F19, F20, F21, F22) MACROS_EXPAND_21(C, M, F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12, F13, F14, F15, F16, F17, F18, F19, F20, F21), M(C, F22)
#define MACROS_EXPAND_23(C, M, F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12, F13, F14, F15, F16, F17, F18, F19, F20, F21, F22, F23) MACROS_EXPAND_22(C, M, F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12, F13, F14, F15, F16, F17, F18, F19, F20, F21, F22), M(C, F23)
#define MACROS_EXPAND_24(C, M, F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12, F13, F14, F15, F16, F17, F18, F19, F20, F21, F22, F23, F24) MACROS_EXPAND_23(C, M, F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12, F13, F14, F15, F16, F17, F18, F19, F20, F21, F22, F23), M(C, F24)
#define MACROS_EXPAND_25(C, M, F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12, F13, F14, F15, F16, F17, F18, F19, F20, F21, F22, F23, F24, F25) MACROS_EXPAND_24(C, M, F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12, F13, F14, F15, F16, F17, F18, F19, F20, F21, F22, F23, F24), M(C, F25)
#define MACROS_EXPAND_26(C, M, F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12, F13, F14, F15, F16, F17, F18, F19, F20, F21, F22, F23, F24, F25, F26) MACROS_EXPAND_25(C, M, F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12, F13, F14, F15, F16, F17, F18, F19, F20, F21, F22, F23, F24, F25), M(C, F26)
#define MACROS_EXPAND_27(C, M, F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12, F13, F14, F15, F16, F17, F18, F19, F20, F21, F22, F23, F24, F25, F26, F27) MACROS_EXPAND_26(C, M, F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12, F13, F14, F15, F16, F17, F18, F19, F20, F21, F22, F23, F24, F25, F26), M(C, F27)
#define MACROS_EXPAND_28(C, M, F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12, F13, F14, F15, F16, F17, F18, F19, F20, F21, F22, F23, F24, F25, F26, F27, F28) MACROS_EXPAND_27(C, M, F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12, F13, F14, F15, F16, F17, F18, F19, F20, F21, F22, F23, F24, F25, F26, F27), M(C, F28)
#define MACROS_EXPAND_29(C, M, F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12, F13, F14, F15, F16, F17, F18, F19, F20, F21, F22, F23, F24, F25, F26, F27, F28, F29) MACROS_EXPAND_28(C, M, F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12, F13, F14, F15, F16, F17, F18, F19, F20, F21, F22, F23, F24, F25, F26, F27, F28), M(C, F29)
#define MACROS_EXPAND_30(C, M, F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12, F13, F14, F15, F16, F17, F18, F19, F20, F21, F22, F23, F24, F25, F26, F27, F28, F29, F30) MACROS_EXPAND_29(C, M, F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12, F13, F14, F15, F16, F17, F18, F19, F20, F21, F22, F23, F24, F25, F26, F27, F28, F29), M(C, F30)
#define MACROS_EXPAND_31(C, M, F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12, F13, F14, F15, F16, F17, F18, F19, F20, F21, F22, F23, F24, F25, F26, F27, F28, F29, F30, F31) MACROS_EXPAND_30(C, M, F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12, F13, F14, F15, F16, F17, F18, F19, F20, F21, F22, F23, F24, F25, F26, F27, F28, F29, F30), M(C, F31)
#define MACROS_EXPAND_32(C, M, F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12, F13, F14, F15, F16, F17, F18, F19, F20, F21, F22, F23, F24, F25, F26, F27, F28, F29, F30, F31, F32) MACROS_EXPAND_31(C, M, F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12, F13, F14, F15, F16, F17, F18, F19, F20, F21, F22, F23, F24, F25, F26, F27, F28, F29, F30, F31), M(C, F32)
#define MACROS_EXPAND_33(C, M, F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12, F13, F14, F15, F16, F17, F18, F19, F20, F21, F22, F23, F24, F25, F26, F27, F28, F29, F30, F31, F32, F33) MACROS_EXPAND_32(C, M, F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12, F13, F14, F15, F16, F17, F18, F19, F20, F21, F22, F23, F24, F25, F26, F27, F28, F29, F30, F31, F32), M(C, F33)
#define MACROS_EXPAND_34(C, M, F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12, F13, F14, F15, F16, F17, F18, F19, F20, F21, F22, F23, F24, F25, F26, F27, F28, F29, F30, F31, F32, F33, F34) MACROS_EXPAND_33(C, M, F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12, F13, F14, F15, F16, F17, F18, F19, F20, F21, F22, F23, F24, F25, F26, F27, F28, F29, F30, F31, F32, F33), M(C, F34)
#define MACROS_EXPAND_35(C, M, F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12, F13, F14, F15, F16, F17, F18, F19, F20, F21, F22, F23, F24, F25, F26, F27, F28, F29, F30, F31, F32, F33, F34, F35) MACROS_EXPAND_34(C, M, F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12, F13, F14, F15, F16, F17, F18, F19, F20, F21, F22, F23, F24, F25, F26, F27, F28, F29, F30, F31, F32, F33, F34), M(C, F35)
#define MACROS_EXPAND_36(C, M, F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12, F13, F14, F15, F16, F17, F18, F19, F20, F21, F22, F23, F24, F25, F26, F27, F28, F29, F30, F31, F32, F33, F34, F35, F36) MACROS_EXPAND_35(C, M, F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12, F13, F14, F15, F16, F17, F18, F19, F20, F21, F22, F23, F24, F25, F26, F27, F28, F29, F30, F31, F32, F33, F34, F35), M(C, F36)
#define MACROS_EXPAND_37(C, M, F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12, F13, F14, F15, F16, F17, F18, F19, F20, F21, F22, F23, F24, F25, F26, F27, F28, F29, F30, F31, F32, F33, F34, F35, F36, F37) MACROS_EXPAND_36(C, M, F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12, F13, F14, F15, F16, F17, F18, F19, F20, F21, F22, F23, F24, F25, F26, F27, F28, F29, F30, F31, F32, F33, F34, F35, F36), M(C, F37)
#define MACROS_EXPAND_38(C, M, F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12, F13, F14, F15, F16, F17, F18, F19, F20, F21, F22, F23, F24, F25, F26, F27, F28, F29, F30, F31, F32, F33, F34, F35, F36, F37, F38) MACROS_EXPAND_37(C, M, F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12, F13, F14, F15, F16, F17, F18, F19, F20, F21, F22, F23, F24, F25, F26, F27, F28, F29, F30, F31, F32, F33, F34, F35, F36, F37), M(C, F38)
#define MACROS_EXPAND_39(C, M, F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12, F13, F14, F15, F16, F17, F18, F19, F20, F21, F22, F23, F24, F25, F26, F27, F28, F29, F30, F31, F32, F33, F34, F35, F36, F37, F38, F39) MACROS_EXPAND_38(C, M, F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12, F13, F14, F15, F16, F17, F18, F19, F20, F21, F22, F23, F24, F25, F26, F27, F28, F29, F30, F31, F32, F33, F34, F35, F36, F37, F38), M(C, F39)
#define MACROS_EXPAND_40(C, M, F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12, F13, F14, F15, F16, F17, F18, F19, F20, F21, F22, F23, F24, F25, F26, F27, F28, F29, F30, F31, F32, F33, F34, F35, F36, F37, F38, F39, F40) MACROS_EXPAND_39(C, M, F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12, F13, F14, F15, F16, F17, F18, F19, F20, F21, F22, F23, F24, F25, F26, F27, F28, F29, F30, F31, F32, F33, F34, F35, F36, F37, F38, F39), M(C, F40)

#define FIELD_PAIR(CLASS, FIELD) std::make_pair(#FIELD, &CLASS::FIELD)

// Helper to expand fields - uses a conditional based on argument count
#define MAKE_FIELD_TUPLE_0(CLASS_NAME, ...) std::make_tuple()
#define MAKE_FIELD_TUPLE_N(CLASS_NAME, ...) std::make_tuple(EXPAND_MACROS(CLASS_NAME, FIELD_PAIR, COUNT_ARGS(__VA_ARGS__), __VA_ARGS__))
#define MAKE_FIELD_TUPLE_SELECT_0 MAKE_FIELD_TUPLE_0
#define MAKE_FIELD_TUPLE_SELECT_1 MAKE_FIELD_TUPLE_N
#define MAKE_FIELD_TUPLE_SELECT_2 MAKE_FIELD_TUPLE_N
#define MAKE_FIELD_TUPLE_SELECT_3 MAKE_FIELD_TUPLE_N
#define MAKE_FIELD_TUPLE_SELECT_4 MAKE_FIELD_TUPLE_N
#define MAKE_FIELD_TUPLE_SELECT_5 MAKE_FIELD_TUPLE_N
#define MAKE_FIELD_TUPLE_SELECT_6 MAKE_FIELD_TUPLE_N
#define MAKE_FIELD_TUPLE_SELECT_7 MAKE_FIELD_TUPLE_N
#define MAKE_FIELD_TUPLE_SELECT_8 MAKE_FIELD_TUPLE_N
#define MAKE_FIELD_TUPLE_SELECT_9 MAKE_FIELD_TUPLE_N
#define MAKE_FIELD_TUPLE_SELECT_10 MAKE_FIELD_TUPLE_N
#define MAKE_FIELD_TUPLE_SELECT_11 MAKE_FIELD_TUPLE_N
#define MAKE_FIELD_TUPLE_SELECT_12 MAKE_FIELD_TUPLE_N
#define MAKE_FIELD_TUPLE_SELECT_13 MAKE_FIELD_TUPLE_N
#define MAKE_FIELD_TUPLE_SELECT_14 MAKE_FIELD_TUPLE_N
#define MAKE_FIELD_TUPLE_SELECT_15 MAKE_FIELD_TUPLE_N
#define MAKE_FIELD_TUPLE_SELECT_16 MAKE_FIELD_TUPLE_N
#define MAKE_FIELD_TUPLE_SELECT_17 MAKE_FIELD_TUPLE_N
#define MAKE_FIELD_TUPLE_SELECT_18 MAKE_FIELD_TUPLE_N
#define MAKE_FIELD_TUPLE_SELECT_19 MAKE_FIELD_TUPLE_N
#define MAKE_FIELD_TUPLE_SELECT_20 MAKE_FIELD_TUPLE_N
#define MAKE_FIELD_TUPLE_SELECT_21 MAKE_FIELD_TUPLE_N
#define MAKE_FIELD_TUPLE_SELECT_22 MAKE_FIELD_TUPLE_N
#define MAKE_FIELD_TUPLE_SELECT_23 MAKE_FIELD_TUPLE_N
#define MAKE_FIELD_TUPLE_SELECT_24 MAKE_FIELD_TUPLE_N
#define MAKE_FIELD_TUPLE_SELECT_25 MAKE_FIELD_TUPLE_N
#define MAKE_FIELD_TUPLE_SELECT_26 MAKE_FIELD_TUPLE_N
#define MAKE_FIELD_TUPLE_SELECT_27 MAKE_FIELD_TUPLE_N
#define MAKE_FIELD_TUPLE_SELECT_28 MAKE_FIELD_TUPLE_N
#define MAKE_FIELD_TUPLE_SELECT_29 MAKE_FIELD_TUPLE_N
#define MAKE_FIELD_TUPLE_SELECT_30 MAKE_FIELD_TUPLE_N
#define MAKE_FIELD_TUPLE_SELECT_31 MAKE_FIELD_TUPLE_N
#define MAKE_FIELD_TUPLE_SELECT_32 MAKE_FIELD_TUPLE_N
#define MAKE_FIELD_TUPLE_SELECT_33 MAKE_FIELD_TUPLE_N
#define MAKE_FIELD_TUPLE_SELECT_34 MAKE_FIELD_TUPLE_N
#define MAKE_FIELD_TUPLE_SELECT_35 MAKE_FIELD_TUPLE_N
#define MAKE_FIELD_TUPLE_SELECT_36 MAKE_FIELD_TUPLE_N
#define MAKE_FIELD_TUPLE_SELECT_37 MAKE_FIELD_TUPLE_N
#define MAKE_FIELD_TUPLE_SELECT_38 MAKE_FIELD_TUPLE_N
#define MAKE_FIELD_TUPLE_SELECT_39 MAKE_FIELD_TUPLE_N
#define MAKE_FIELD_TUPLE_SELECT_40 MAKE_FIELD_TUPLE_N
#define MAKE_FIELD_TUPLE_SELECTOR(N) CONCAT(MAKE_FIELD_TUPLE_SELECT_, N)
#define MAKE_FIELD_TUPLE(CLASS_NAME, ...) MAKE_FIELD_TUPLE_SELECTOR(COUNT_ARGS(__VA_ARGS__))(CLASS_NAME, ##__VA_ARGS__)

#define REFLECT_FIELDS(CLASS_NAME, ...)                                                                                        \
    template <>                                                                                                                \
    struct ReflectionLibrary::ReflectionInfo<CLASS_NAME>                                                                       \
    {                                                                                                                          \
        static constexpr const char *class_name = #CLASS_NAME;                                                                 \
        static constexpr auto fields = MAKE_FIELD_TUPLE(CLASS_NAME, ##__VA_ARGS__);                                            \
        static CLASS_NAME reflect(CLASS_NAME &obj) { return obj; }                                                             \
        static CLASS_NAME create(CLASS_NAME &obj) { return obj; }                                                              \
    };

#define REFLECT_CUSTOM(CLASS_NAME, ...)                                                                                        \
    template <>                                                                                                                \
    struct ReflectionLibrary::ReflectionInfo<CLASS_NAME>                                                                       \
    {                                                                                                                          \
        using Reflector = CLASS_NAME::Reflector;                                                                               \
        static constexpr const char *class_name = #CLASS_NAME;                                                                 \
        static constexpr auto fields = MAKE_FIELD_TUPLE(Reflector, ##__VA_ARGS__);                                             \
        static Reflector reflect(CLASS_NAME &obj) { return Reflector::reflect(obj); }                                          \
        static CLASS_NAME create(Reflector &r) { return Reflector::create(r); }                                                \
    };

// Enum reflection macros
#define ENUM_VALUE_PAIR(ENUM_TYPE, VALUE) std::make_pair(ENUM_TYPE::VALUE, #VALUE)

#define MAKE_ENUM_TUPLE_0(ENUM_TYPE, ...) std::make_tuple()
#define MAKE_ENUM_TUPLE_N(ENUM_TYPE, ...) std::make_tuple(EXPAND_MACROS(ENUM_TYPE, ENUM_VALUE_PAIR, COUNT_ARGS(__VA_ARGS__), __VA_ARGS__))
#define MAKE_ENUM_TUPLE_SELECT_0 MAKE_ENUM_TUPLE_0
#define MAKE_ENUM_TUPLE_SELECT_1 MAKE_ENUM_TUPLE_N
#define MAKE_ENUM_TUPLE_SELECT_2 MAKE_ENUM_TUPLE_N
#define MAKE_ENUM_TUPLE_SELECT_3 MAKE_ENUM_TUPLE_N
#define MAKE_ENUM_TUPLE_SELECT_4 MAKE_ENUM_TUPLE_N
#define MAKE_ENUM_TUPLE_SELECT_5 MAKE_ENUM_TUPLE_N
#define MAKE_ENUM_TUPLE_SELECT_6 MAKE_ENUM_TUPLE_N
#define MAKE_ENUM_TUPLE_SELECT_7 MAKE_ENUM_TUPLE_N
#define MAKE_ENUM_TUPLE_SELECT_8 MAKE_ENUM_TUPLE_N
#define MAKE_ENUM_TUPLE_SELECT_9 MAKE_ENUM_TUPLE_N
#define MAKE_ENUM_TUPLE_SELECT_10 MAKE_ENUM_TUPLE_N
#define MAKE_ENUM_TUPLE_SELECT_11 MAKE_ENUM_TUPLE_N
#define MAKE_ENUM_TUPLE_SELECT_12 MAKE_ENUM_TUPLE_N
#define MAKE_ENUM_TUPLE_SELECT_13 MAKE_ENUM_TUPLE_N
#define MAKE_ENUM_TUPLE_SELECT_14 MAKE_ENUM_TUPLE_N
#define MAKE_ENUM_TUPLE_SELECT_15 MAKE_ENUM_TUPLE_N
#define MAKE_ENUM_TUPLE_SELECT_16 MAKE_ENUM_TUPLE_N
#define MAKE_ENUM_TUPLE_SELECT_17 MAKE_ENUM_TUPLE_N
#define MAKE_ENUM_TUPLE_SELECT_18 MAKE_ENUM_TUPLE_N
#define MAKE_ENUM_TUPLE_SELECT_19 MAKE_ENUM_TUPLE_N
#define MAKE_ENUM_TUPLE_SELECT_20 MAKE_ENUM_TUPLE_N
#define MAKE_ENUM_TUPLE_SELECT_21 MAKE_ENUM_TUPLE_N
#define MAKE_ENUM_TUPLE_SELECT_22 MAKE_ENUM_TUPLE_N
#define MAKE_ENUM_TUPLE_SELECT_23 MAKE_ENUM_TUPLE_N
#define MAKE_ENUM_TUPLE_SELECT_24 MAKE_ENUM_TUPLE_N
#define MAKE_ENUM_TUPLE_SELECT_25 MAKE_ENUM_TUPLE_N
#define MAKE_ENUM_TUPLE_SELECT_26 MAKE_ENUM_TUPLE_N
#define MAKE_ENUM_TUPLE_SELECT_27 MAKE_ENUM_TUPLE_N
#define MAKE_ENUM_TUPLE_SELECT_28 MAKE_ENUM_TUPLE_N
#define MAKE_ENUM_TUPLE_SELECT_29 MAKE_ENUM_TUPLE_N
#define MAKE_ENUM_TUPLE_SELECT_30 MAKE_ENUM_TUPLE_N
#define MAKE_ENUM_TUPLE_SELECT_31 MAKE_ENUM_TUPLE_N
#define MAKE_ENUM_TUPLE_SELECT_32 MAKE_ENUM_TUPLE_N
#define MAKE_ENUM_TUPLE_SELECT_33 MAKE_ENUM_TUPLE_N
#define MAKE_ENUM_TUPLE_SELECT_34 MAKE_ENUM_TUPLE_N
#define MAKE_ENUM_TUPLE_SELECT_35 MAKE_ENUM_TUPLE_N
#define MAKE_ENUM_TUPLE_SELECT_36 MAKE_ENUM_TUPLE_N
#define MAKE_ENUM_TUPLE_SELECT_37 MAKE_ENUM_TUPLE_N
#define MAKE_ENUM_TUPLE_SELECT_38 MAKE_ENUM_TUPLE_N
#define MAKE_ENUM_TUPLE_SELECT_39 MAKE_ENUM_TUPLE_N
#define MAKE_ENUM_TUPLE_SELECT_40 MAKE_ENUM_TUPLE_N
#define MAKE_ENUM_TUPLE_SELECTOR(N) CONCAT(MAKE_ENUM_TUPLE_SELECT_, N)
#define MAKE_ENUM_TUPLE(ENUM_TYPE, ...) MAKE_ENUM_TUPLE_SELECTOR(COUNT_ARGS(__VA_ARGS__))(ENUM_TYPE, ##__VA_ARGS__)

#define REFLECT_ENUM(ENUM_TYPE, ...)                                                                                           \
    template <>                                                                                                                \
    struct ReflectionLibrary::EnumInfo<ENUM_TYPE>                                                                              \
    {                                                                                                                          \
        static constexpr const char *enum_name = #ENUM_TYPE;                                                                   \
        static constexpr auto values = MAKE_ENUM_TUPLE(ENUM_TYPE, ##__VA_ARGS__);                                              \
    };