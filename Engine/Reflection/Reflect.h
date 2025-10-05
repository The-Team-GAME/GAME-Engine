#pragma once

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

#define REFLECT(CLASS_NAME, ...)                                                     \
  template <>                                                                                \
  constexpr auto reflection_fields<CLASS_NAME>()                                             \
  {                                                                                          \
    return std::make_pair(#CLASS_NAME,                                                       \
                          std::make_tuple(EXPAND_FIELDS(CLASS_NAME, COUNT_ARGS(__VA_ARGS__), __VA_ARGS__))); \
  }

template <typename T>
constexpr auto reflection_fields()
{
}


/* Example usage */
/*

struct Person
{
  std::string name;
  int age;
};
REFLECT(Person, name, age)

struct Book
{
  std::string title;
  std::string author;
  int pages;
};
REFLECT(Book, title, author, pages)


template <typename T>
void print_struct_fields(const T& obj)
{
  auto [class_name, fields_tuple] = reflection_fields<T>();

  std::cout << "Type: " << class_name << " has fields:\n";
  std::apply([&obj](const auto &...pairs)
    {
      ((std::cout << "- " << pairs.first << ": " << (obj.*pairs.second) << "\n"), ...);
    },
    fields_tuple
  );
}

int main()
{
  Person person{ "Alice", 30 };
  Person person2{ "bobby", 23 };
  Book book{ "1984", "George Orwell", 328 };

  print_struct_fields(person);
  std::cout << "\n";

  print_struct_fields(person2);
  std::cout << "\n";

  print_struct_fields(book);
  std::cout << "\n";

  return 0;
}
*/