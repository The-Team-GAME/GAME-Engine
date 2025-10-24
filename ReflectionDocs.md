# Reflection Documentation

This readme covers the basic usage of reflection  that is relevant to this engine. For detailed documentation of the ReflectionLibrary, visit [https://github.com/sheeyang/Reflection](https://github.com/sheeyang/Reflection).

## Basic Reflection

```cpp
#include <ReflectionLibrary.h>

// Define a simple struct
struct Person
{
    std::string name;
    int age;
    double height;
};

// Make it reflectable with a single macro
REFLECT_FIELDS(Person, name, age, height)

int main()
{
    Person person{"Alice", 30, 5.6};
    
    // Print the struct
    ReflectionLibrary::print(person);
    
    // Get class name
    auto className = ReflectionLibrary::get_class_name(person);
    std::cout << "Class: " << className << std::endl;
    
    // Get field count
    auto fieldCount = ReflectionLibrary::get_field_count(person);
    std::cout << "Fields: " << fieldCount << std::endl;
    
    return 0;
}
```

**Output:**
```
Type: Person
  name = "Alice"
  age = 30
  height = 5.6
Class: Person
Fields: 3
```

### JSON Serialization

```cpp
#include <ReflectionLibrary.h>

struct Product
{
    std::string name;
    double price;
    int quantity;
    bool inStock;
};
REFLECT_FIELDS(Product, name, price, quantity, inStock)

int main()
{
    Product product{"Laptop", 999.99, 5, true};
    
    // Serialize to JSON
    std::string json = ReflectionLibrary::to_json(product, true);
    std::cout << "JSON:\n" << json << std::endl;
    
    // Deserialize from JSON
    std::string jsonStr = R"({
        "name": "Phone",
        "price": 599.99,
        "quantity": 10,
        "inStock": true
    })";
    
    auto result = ReflectionLibrary::from_json<Product>(jsonStr);
    if (result.has_value())
    {
        Product p = result.value();
        std::cout << "Loaded: " << p.name << ", $" << p.price << std::endl;
    }
    
    return 0;
}
```

**Output:**
```
JSON:
{
    "name": "Laptop",
    "price": 999.99,
    "quantity": 5,
    "inStock": true
}
Loaded: Phone, $599.99
```

## Custom Reflection

For types you cannot modify (third-party libraries, legacy code), use custom reflectors:

```cpp
#include <ReflectionLibrary.h>

// Third-party struct we cannot modify
struct LegacyPoint
{
    int x_coord;
    int y_coord;
};

// Wrapper with custom reflector
struct Point
{
    LegacyPoint point;
    
    struct Reflector
    {
        int x;
        int y;
        
        
        // Transforms the reflectable fields (x, y) into the actual Point object
        // Example: Called during deserialization (JSON/GenericValue -> Point)
        static Point create(const Reflector& r)
        {
            Point p;
            p.point.x_coord = r.x;
            p.point.y_coord = r.y;
            return p;
        }
        
        // Extracts the reflectable fields (x, y) from the Point object
        // Example: Called during serialization (Point -> JSON/GenericValue)
        static Reflector reflect(const Point& p)
        {
            Reflector r;
            r.x = p.point.x_coord;
            r.y = p.point.y_coord;
            return r;
        }
    };
};
REFLECT_CUSTOM(Point, x, y)

int main()
{
    Point p;
    p.point.x_coord = 10;
    p.point.y_coord = 20;
    
    // Works with all reflection features
    ReflectionLibrary::print(p);
    std::string json = ReflectionLibrary::to_json(p, true);
    std::cout << json << std::endl;
    
    return 0;
}
```



## How Settings Work

Settings are automatically serialized to and from JSON files using reflection.

- When you add a new member to a settings struct (e.g., `EngineSettings` in `Engine/FileIO/EngineSettings.h` or `UserSettings` in `Engine/FileIO/UserSettings.h`), the new field is **automatically included in the JSON** on the next engine run. **No manual JSON editing is required!!**
- To add a new setting:
  1. Add the member to the struct.
  2. Include it in the `REFLECT_FIELDS` macro at the bottom of the file.

The `SettingsManager` handles loading, merging defaults for missing fields, and saving automatically.