#pragma once

// Count arguments (supports up to 20)
#define COUNT_ARGS(...) COUNT_ARGS_IMPL(__VA_ARGS__, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1)
#define COUNT_ARGS_IMPL(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, N, ...) N

// Concatenation helper
#define CONCAT(A, B) CONCAT_IMPL(A, B)
#define CONCAT_IMPL(A, B) A##B

// Expand fields based on count
#define EXPAND_MACROS(CLASS, MACRO, N, ...) CONCAT(MACROS_EXPAND_, N)(CLASS, MACRO, __VA_ARGS__)

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
// Add more as needed...

#define FIELD_PAIR(CLASS, FIELD) std::make_pair(#FIELD, &CLASS::FIELD)

#define REFLECT_FIELDS(CLASS_NAME, ...)                                                                                              \
    template <>                                                                                                                      \
    struct ReflectionInfo<CLASS_NAME>                                                                                                \
    {                                                                                                                                \
        static constexpr const char *class_name = #CLASS_NAME;                                                                       \
        static constexpr auto fields = std::make_tuple(EXPAND_MACROS(CLASS_NAME, FIELD_PAIR, COUNT_ARGS(__VA_ARGS__), __VA_ARGS__)); \
    };

#define REFLECT_CUSTOM(CLASS_NAME, ...)                                   \
    REFLECT_FIELDS(CLASS_NAME::Reflector, __VA_ARGS__)                    \
    template <>                                                           \
    struct ReflectionInfo<CLASS_NAME>                                     \
    {                                                                     \
        using Reflector = CLASS_NAME::Reflector;                          \
        static constexpr const char *class_name = #CLASS_NAME;            \
        static constexpr auto fields = ReflectionInfo<Reflector>::fields; \
        static CLASS_NAME create(const Reflector &r)                      \
        {                                                                 \
            return CLASS_NAME::Reflector::create(r);                      \
        }                                                                 \
        static Reflector reflect(const CLASS_NAME &obj)                   \
        {                                                                 \
            return CLASS_NAME::Reflector::reflect(obj);                   \
        }                                                                 \
    };
