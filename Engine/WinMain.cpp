#include <string>
#include <tuple>
#include <utility>
#include <iostream>

#define FIELD_PAIR(CLASS, FIELD) std::make_pair(#FIELD, &CLASS::FIELD)

// Count arguments (supports up to 20)
#define COUNT_ARGS(...) COUNT_ARGS_IMPL(__VA_ARGS__, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1)
#define COUNT_ARGS_IMPL(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, N, ...) N

// Concatenation helper
#define CONCAT(A, B) CONCAT_IMPL(A, B)
#define CONCAT_IMPL(A, B) A##B

// Expand fields based on count
#define EXPAND_FIELDS(CLASS, N, ...) CONCAT(FIELDS_EXPAND_, N)(CLASS, __VA_ARGS__)

#define FIELDS_EXPAND_1(C, F1) FIELD_PAIR(C, F1)
#define FIELDS_EXPAND_2(C, F1, F2) FIELDS_EXPAND_1(C, F1), FIELD_PAIR(C, F2)
#define FIELDS_EXPAND_3(C, F1, F2, F3) FIELDS_EXPAND_2(C, F1, F2), FIELD_PAIR(C, F3)
#define FIELDS_EXPAND_4(C, F1, F2, F3, F4) FIELDS_EXPAND_3(C, F1, F2, F3), FIELD_PAIR(C, F4)
#define FIELDS_EXPAND_5(C, F1, F2, F3, F4, F5) FIELDS_EXPAND_4(C, F1, F2, F3, F4), FIELD_PAIR(C, F5)
// Add more as needed...

#define GENERATE_FIELDS(CLASS_NAME, ...)                                                     \
  template <>                                                                                \
  constexpr auto reflection_fields<CLASS_NAME>()                                             \
  {                                                                                          \
    return std::make_tuple(EXPAND_FIELDS(CLASS_NAME, COUNT_ARGS(__VA_ARGS__), __VA_ARGS__)); \
  }

// Forward declaration of the reflection function template
template <typename T>
constexpr auto reflection_fields();

struct Person
{
  std::string name;
  int age;
  static constexpr const char* _type = "Person";
};

// Generate fields OUTSIDE the struct
GENERATE_FIELDS(Person, name, age)

template <typename T>
void print_struct_fields(const T& obj)
{
  std::apply([&obj](const auto &...pairs)
    { ((std::cout << "- " << pairs.first << ": "
      << (obj.*pairs.second) << "\n"),
      ...); },
    reflection_fields<T>());
}

int main()
{
  Person person{ "Alice", 30 };
  Person person2{ "bobby", 23 };

  std::cout << "Type: " << person._type << " has fields:\n";
  print_struct_fields(person);

  std::cout << "Type: " << person2._type << " has fields:\n";
  print_struct_fields(person2);

  return 0;
}

// This is not needed, just for supressing intellisense warnings.
template <typename T>
constexpr auto reflection_fields()
{
}
