

#include <string>
#include <tuple>
#include <utility>
#include <iostream>

// Helper macro to generate a single (string, pointer-to-member) pair.
// Note: We MUST pass the CLASS_NAME for robust member pointer generation.
#define FIELD_PAIR(CLASS, FIELD) \
  std::make_pair(#FIELD, &CLASS::FIELD)

// --- Variadic Field Expansion Helpers (Supports up to 5 fields) ---
// These helpers use fixed arguments to correctly comma-separate the list.

#define FIELDS_EXPAND_1(CLASS, F1) FIELD_PAIR(CLASS, F1)

#define FIELDS_EXPAND_2(CLASS, F1, F2) \
  FIELDS_EXPAND_1(CLASS, F1), FIELD_PAIR(CLASS, F2)

#define FIELDS_EXPAND_3(CLASS, F1, F2, F3) \
  FIELDS_EXPAND_2(CLASS, F1, F2), FIELD_PAIR(CLASS, F3)

#define FIELDS_EXPAND_4(CLASS, F1, F2, F3, F4) \
  FIELDS_EXPAND_3(CLASS, F1, F2, F3), FIELD_PAIR(CLASS, F4)

#define FIELDS_EXPAND_5(CLASS, F1, F2, F3, F4, F5) \
  FIELDS_EXPAND_4(CLASS, F1, F2, F3, F4), FIELD_PAIR(CLASS, F5)

// Macro to select the correct expansion helper based on argument count.
// By having 5 placeholders (_1 to _5), the 6th argument (NAME) is correctly
// selected when 1 to 5 fields are passed in __VA_ARGS__.
#define GET_FIELD_MACRO(_1, _2, _3, _4, _5, NAME, ...) NAME

// Main macro: Takes the Class Name, followed by a list of field names.
#define GENERATE_FIELDS(CLASS_NAME, ...)           \
  static constexpr auto _fields = std::make_tuple( \
      GET_FIELD_MACRO(__VA_ARGS__, FIELDS_EXPAND_5, FIELDS_EXPAND_4, FIELDS_EXPAND_3, FIELDS_EXPAND_2, FIELDS_EXPAND_1)(CLASS_NAME, __VA_ARGS__))

struct Person
{
  std::string name;
  int age;

  GENERATE_FIELDS(Person, name, age);

  const std::string& _type = "Person";
};

// https://stackoverflow.com/questions/1198260/how-can-you-iterate-over-the-elements-of-an-stdtuple
template <typename T>
void print_struct_fields(const T& obj)
{
  // The type T must have a static constexpr member named _fields.
  std::apply([&obj](const auto &...pairs)
    {
      // Unpack 'pairs' (std::pair<const char*, member_pointer>)
      // using a fold expression with the comma operator.
      ((
        // pairs.first is the field name (e.g., "name")
        // (obj.*pairs.second) uses the pointer-to-member operator to
        // access the actual value of the member in the 'obj' instance.
        std::cout << "- " << pairs.first << ": "
        << (obj.*pairs.second) << "\n"),
        ...); // Fold expression: sequences the print statements
    },
    T::_fields); // Apply the lambda to the static _fields tuple
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