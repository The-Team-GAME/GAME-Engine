//-----------------------------------------------------------------------------------
// file:    ColliderMeshFileIO.cpp
// project: Monkey Stole My GPA
//
// author:  Antigravity
//
// brief:   This file implements serialization and deserialization functions for
//          collider mesh data. Provides JSON-based save/load functionality for
//          polygon collider vertices.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#include "ColliderMeshFileIO.h"
#include "Helpers.h"
#include "../Reflection/ReflectionLibrary.h"
#include "../Logger.h"

namespace Engine {
  namespace FileIO {

    std::string serialize_collider_mesh_to_json(const ColliderMeshData& mesh_data) {
      ReflectionLibrary::GenericValue root = ReflectionLibrary::Object();

      root["dataType"] = "ColliderMesh";
      root["version"] = 1;

      // Serialize vertices array
      ReflectionLibrary::Array vertices_array;
      for (const auto& vertex : mesh_data.vertices) {
        ReflectionLibrary::Object vertex_obj;
        vertex_obj["x"] = static_cast<double>(vertex.x);
        vertex_obj["y"] = static_cast<double>(vertex.y);
        vertices_array.push_back(ReflectionLibrary::GenericValue(std::move(vertex_obj)));
      }
      root["vertices"] = ReflectionLibrary::GenericValue(std::move(vertices_array));

      // Serialize offset
      ReflectionLibrary::Object offset_obj;
      offset_obj["x"] = static_cast<double>(mesh_data.offset.x);
      offset_obj["y"] = static_cast<double>(mesh_data.offset.y);
      root["offset"] = ReflectionLibrary::GenericValue(std::move(offset_obj));

      // Serialize bounding dimensions
      root["width"] = static_cast<double>(mesh_data.width);
      root["height"] = static_cast<double>(mesh_data.height);

      return ReflectionLibrary::generic_value_to_json(root, true);
    }

    std::optional<ColliderMeshData> deserialize_collider_mesh_from_json(const std::string& json_str) {
      std::optional<ReflectionLibrary::GenericValue> root_opt = ReflectionLibrary::generic_value_from_json(json_str);

      if (!root_opt.has_value()) {
        Log::warning("Failed to parse collider mesh JSON");
        return std::nullopt;
      }

      ReflectionLibrary::GenericValue root = root_opt.value();

      // Validate data type
      if (!root.isObject()) {
        Log::warning("Collider mesh JSON root is not an object");
        return std::nullopt;
      }

      const auto& obj = root.getObject();

      if (obj.find("dataType") != obj.end()) {
        if (root["dataType"].getString() != "ColliderMesh") {
          Log::warning("Invalid collider mesh data type");
          return std::nullopt;
        }
      }

      ColliderMeshData mesh_data;

      // Deserialize vertices
      if (obj.find("vertices") != obj.end() && root["vertices"].isArray()) {
        const auto& vertices_array = root["vertices"].getArray();
        for (const auto& vertex_val : vertices_array) {
          if (vertex_val.isObject()) {
            const auto& vertex_obj = vertex_val.getObject();
            float x = 0.0f, y = 0.0f;

            if (vertex_obj.find("x") != vertex_obj.end()) {
              x = static_cast<float>(vertex_val["x"].getDouble());
            }
            if (vertex_obj.find("y") != vertex_obj.end()) {
              y = static_cast<float>(vertex_val["y"].getDouble());
            }

            mesh_data.vertices.push_back(Math::Vec2D(x, y));
          }
        }
      }

      // Deserialize offset
      if (obj.find("offset") != obj.end() && root["offset"].isObject()) {
        const auto& offset_obj = root["offset"].getObject();
        if (offset_obj.find("x") != offset_obj.end()) {
          mesh_data.offset.x = static_cast<float>(root["offset"]["x"].getDouble());
        }
        if (offset_obj.find("y") != offset_obj.end()) {
          mesh_data.offset.y = static_cast<float>(root["offset"]["y"].getDouble());
        }
      }

      // Deserialize bounding dimensions
      if (obj.find("width") != obj.end()) {
        mesh_data.width = static_cast<float>(root["width"].getDouble());
      }
      if (obj.find("height") != obj.end()) {
        mesh_data.height = static_cast<float>(root["height"].getDouble());
      }

      return mesh_data;
    }

    bool serialize_collider_mesh_to_json_file(const std::string& filepath, const ColliderMeshData& mesh_data) {
      std::string json_content = serialize_collider_mesh_to_json(mesh_data);

      if (!write_string_to_file(filepath, json_content)) {
        Log::error("Failed to write collider mesh to file: {}", filepath);
        return false;
      }

      Log::info("Successfully exported collider mesh to: {}", filepath);
      return true;
    }

    std::optional<ColliderMeshData> deserialize_collider_mesh_from_json_file(const std::string& filepath) {
      auto json_str = read_string_from_file(filepath);

      if (!json_str.has_value()) {
        Log::warning("Failed to read collider mesh file: {}", filepath);
        return std::nullopt;
      }

      return deserialize_collider_mesh_from_json(json_str.value());
    }

  } // namespace FileIO
} // namespace Engine
