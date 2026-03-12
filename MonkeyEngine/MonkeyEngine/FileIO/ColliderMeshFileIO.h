//-----------------------------------------------------------------------------------
// file:    ColliderMeshFileIO.h
// project: Monkey Stole My GPA
//
// author:  Antigravity
//
// brief:   This file declares functions for serializing and deserializing collider
//          mesh data (polygon vertices) to/from JSON format. Enables saving and
//          loading of custom collider shapes for entities.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include <string>
#include <vector>
#include <optional>
#include "../Math/Vector2D.h"

namespace Engine {
  namespace FileIO {

    /*=================================================================================
    Structure:
    Represents collider mesh data that can be serialized to/from JSON files.
    Contains the polygon vertices that define a custom collider shape.
    ==================================================================================*/
    struct ColliderMeshData {
      std::vector<Math::Vec2D> vertices;
      Math::Vec2D offset = { 0.0f, 0.0f };
      float width = 0.0f;   // Bounding width (for reference)
      float height = 0.0f;  // Bounding height (for reference)
    };

    /*=================================================================================
    Function Brief:
    Serializes collider mesh data to a JSON string.

    Parameters:
    mesh_data - The collider mesh data to serialize.

    Returns:
    JSON string containing the serialized mesh data.

    Where to call it:
    When exporting collider data to files from the editor.
    ==================================================================================*/
    std::string serialize_collider_mesh_to_json(const ColliderMeshData& mesh_data);

    /*=================================================================================
    Function Brief:
    Deserializes collider mesh data from a JSON string.

    Parameters:
    json_str - The JSON string containing the serialized mesh data.

    Returns:
    std::optional containing the mesh data if successful, std::nullopt on failure.

    Where to call it:
    When loading collider data from files.
    ==================================================================================*/
    std::optional<ColliderMeshData> deserialize_collider_mesh_from_json(const std::string& json_str);

    /*=================================================================================
    Function Brief:
    Serializes collider mesh data to a JSON file.

    Parameters:
    filepath  - Path to the output JSON file.
    mesh_data - The collider mesh data to serialize.

    Returns:
    True if the file was written successfully, false on error.

    Where to call it:
    When exporting collider data to files from the editor.
    ==================================================================================*/
    bool serialize_collider_mesh_to_json_file(const std::string& filepath, const ColliderMeshData& mesh_data);

    /*=================================================================================
    Function Brief:
    Deserializes collider mesh data from a JSON file.

    Parameters:
    filepath - Path to the JSON file containing the mesh data.

    Returns:
    std::optional containing the mesh data if successful, std::nullopt on failure.

    Where to call it:
    When loading collider data from files (e.g., in Polygon constructor).
    ==================================================================================*/
    std::optional<ColliderMeshData> deserialize_collider_mesh_from_json_file(const std::string& filepath);

  } // namespace FileIO
} // namespace Engine
