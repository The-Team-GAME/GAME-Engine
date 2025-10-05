#include "Reflection/Reflect.h"
#include "Reflection/Helpers.h"

struct Book
{
  std::string title;
  std::string author;
  int pages;
};
REFLECT(Book, title, author, pages)

struct Person
{
  std::string name;
  int age;
  double height;
  bool is_student;
  Book favorite_book;
};
REFLECT(Person, name, age, height, is_student, favorite_book)

int main()
{
  Book book{ "1984", "George Orwell", 328 };

  // Serialize
  Person person{ "Alice", 30, 5.7, false, book };
  std::string json = serialize_to_json(person);
  std::cout << "Serialized: " << json << "\n\n";

  // Deserialize
  Person person2;
  if (deserialize_from_json(person2, json))
  {
    std::cout << "Deserialized successfully:\n";
    //print_struct_fields(person2);
  }

  return 0;
}