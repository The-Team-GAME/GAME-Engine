# Reflection Documentation

This readme covers the basic usage of reflection that is relevant to this engine. For detailed documentation of the ReflectionLibrary, visit [https://github.com/sheeyang/Reflection](https://github.com/sheeyang/Reflection).

## Table of Contents

- [Reflection Documentation](#reflection-documentation)
  - [Table of Contents](#table-of-contents)
  - [Reflection examples](#reflection-examples)
    - [Basic Reflection](#basic-reflection)
    - [JSON Serialization](#json-serialization)
  - [Custom Reflection](#custom-reflection)
  - [Registering ECS Components](#registering-ecs-components)
    - [Basic Component Registration](#basic-component-registration)
    - [Custom Reflector Component Registration](#custom-reflector-component-registration)
    - [When to Use Each Macro](#when-to-use-each-macro)
  - [How Settings Work](#how-settings-work)


## Reflection examples

### Basic Reflection

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
REFLECT_CUSTOM(Point, x, y) // only list the Reflector's fields, not the Struct's fields!! (In this case the fields are int 'x' and int 'y', not LegacyPoint 'point')

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

## Registering ECS Components

Components in the ECS must be registered to enable serialization and automatic type management. The engine provides two macros for registering components:

### Basic Component Registration

Use `REGISTER_COMPONENT` for components with simple fields that can be directly reflected:

```cpp
#include "../ECS/Macros.h"

struct Health
{
    float current;
    float max;
};

// Register the component - automatically reflects AND registers with ECS
REGISTER_COMPONENT(Health, current, max)
```

**Important:** Use `REGISTER_COMPONENT` instead of `REFLECT_FIELDS` for ECS components. `REGISTER_COMPONENT` both reflects the type AND registers it with the ECS automatically.

### Custom Reflector Component Registration

Use `REGISTER_COMPONENT_CUSTOM` for components that need custom serialization logic:

```cpp
#include "../ECS/Macros.h"

struct Texture
{
    Engine::Graphics::Texture texture_handle; // Internal handle we don't want to serialize
    
    struct Reflector
    {
        std::string path; // Only serialize the file path
        
        static Reflector reflect(const Texture& tex)
        {
            Reflector r;
            if (tex.texture_handle)
            {
                r.path = Engine::Resources::get_relative_path(tex.texture_handle->path);
            }
            return r;
        }
        
        static Texture create(const Reflector& r)
        {
            return Engine::Resources::load_texture(r.path);
        }
    };
};

// Register with custom reflector - uses the Reflector's fields for serialization
REGISTER_COMPONENT_CUSTOM(Texture, path)
```

### When to Use Each Macro

- **`REGISTER_COMPONENT`**: Use for simple data components where all fields should be serialized as-is
  - Examples: Position, Velocity, Health, Score
  
- **`REGISTER_COMPONENT_CUSTOM`**: Use when you need control over serialization
  - Loading resources from paths instead of raw data
  - Converting between internal and serialized representations
  - Excluding certain fields from serialization
  - Handling complex initialization logic

## How Settings Work

Settings are automatically serialized to and from JSON files using reflection.

- When you add a new member to a settings struct (e.g., `EngineSettings` in `Engine/FileIO/EngineSettings.h` or `UserSettings` in `Engine/FileIO/UserSettings.h`), the new field is **automatically included in the JSON** on the next engine run. **No manual JSON editing is required!!**
- To add a new setting:
  1. Add the member to the struct.
  2. Include it in the `REFLECT_FIELDS` macro at the bottom of the file.

The `SettingsManager` handles loading, merging defaults for missing fields, and saving automatically.