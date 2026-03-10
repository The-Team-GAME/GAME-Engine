//-----------------------------------------------------------------------------------
// file:    FirebaseTypes.h
// project: Monkey Stole My GPA
//
// brief:   Shared types and utilities for the Firebase Cloud Firestore integration.
//          Defines callback signatures, HTTP response structures, pending request
//          tracking, and Firestore JSON value format conversion helpers.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include <string>
#include <functional>
#include <future>
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>

namespace Engine
{
  namespace Firebase
  {
    // ----------------------------------------------------------------
    // Callback type: (success, response_body_or_error_message)
    // ----------------------------------------------------------------
    using Callback = std::function<void(bool success, const std::string& data)>;

    // ----------------------------------------------------------------
    // Raw HTTP response from WinHTTP
    // ----------------------------------------------------------------
    struct HttpResponse
    {
      int    status_code = 0;
      std::string body;
      bool   success = false;   // true when status_code is 2xx
    };

    // ----------------------------------------------------------------
    // A single in-flight async request awaiting completion
    // ----------------------------------------------------------------
    struct PendingRequest
    {
      std::future<HttpResponse> future;
      Callback on_complete;     // Invoked on the main thread when the future resolves
    };

    // ================================================================
    // Firestore JSON value-format conversion
    // ================================================================
    // Firestore REST API wraps every value in a typed envelope:
    //   {"stringValue": "hello"}  {"integerValue": "42"}  {"doubleValue": 3.14}
    //   {"booleanValue": true}    {"nullValue": null}
    //   {"mapValue": {"fields": { ... }}}
    //   {"arrayValue": {"values": [ ... ]}}
    //
    // The helpers below convert between *flat* JSON that game
    // developers work with and the Firestore wire format.
    // ================================================================

    // Forward declarations
    inline void to_firestore_value(const rapidjson::Value& src,
      rapidjson::Value& dst,
      rapidjson::Document::AllocatorType& alloc);

    inline void from_firestore_value(const rapidjson::Value& src,
      rapidjson::Value& dst,
      rapidjson::Document::AllocatorType& alloc);

    // ------- flat JSON  -->  Firestore value -------------------------
    inline void to_firestore_value(const rapidjson::Value& src,
      rapidjson::Value& dst,
      rapidjson::Document::AllocatorType& alloc)
    {
      dst.SetObject();

      if (src.IsString()) {
        dst.AddMember("stringValue",
          rapidjson::Value(src.GetString(), alloc), alloc);
      }
      else if (src.IsBool()) {
        dst.AddMember("booleanValue",
          rapidjson::Value(src.GetBool()), alloc);
      }
      else if (src.IsInt()) {
        // Firestore stores integers as string
        std::string s = std::to_string(src.GetInt());
        dst.AddMember("integerValue",
          rapidjson::Value(s.c_str(), alloc), alloc);
      }
      else if (src.IsInt64()) {
        std::string s = std::to_string(src.GetInt64());
        dst.AddMember("integerValue",
          rapidjson::Value(s.c_str(), alloc), alloc);
      }
      else if (src.IsUint() || src.IsUint64()) {
        std::string s = std::to_string(src.IsUint() ? (int64_t)src.GetUint() : (int64_t)src.GetUint64());
        dst.AddMember("integerValue",
          rapidjson::Value(s.c_str(), alloc), alloc);
      }
      else if (src.IsDouble()) {
        dst.AddMember("doubleValue",
          rapidjson::Value(src.GetDouble()), alloc);
      }
      else if (src.IsNull()) {
        dst.AddMember("nullValue",
          rapidjson::Value(rapidjson::kNullType), alloc);
      }
      else if (src.IsObject()) {
        // Convert to mapValue
        rapidjson::Value map_val(rapidjson::kObjectType);
        rapidjson::Value fields(rapidjson::kObjectType);
        for (auto it = src.MemberBegin(); it != src.MemberEnd(); ++it) {
          rapidjson::Value child;
          to_firestore_value(it->value, child, alloc);
          fields.AddMember(
            rapidjson::Value(it->name, alloc),
            child, alloc);
        }
        map_val.AddMember("fields", fields, alloc);
        dst.AddMember("mapValue", map_val, alloc);
      }
      else if (src.IsArray()) {
        // Convert to arrayValue
        rapidjson::Value arr_val(rapidjson::kObjectType);
        rapidjson::Value values(rapidjson::kArrayType);
        for (rapidjson::SizeType i = 0; i < src.Size(); ++i) {
          rapidjson::Value child;
          to_firestore_value(src[i], child, alloc);
          values.PushBack(child, alloc);
        }
        arr_val.AddMember("values", values, alloc);
        dst.AddMember("arrayValue", arr_val, alloc);
      }
    }

    // ------- Firestore value  -->  flat JSON -------------------------
    inline void from_firestore_value(const rapidjson::Value& src,
      rapidjson::Value& dst,
      rapidjson::Document::AllocatorType& alloc)
    {
      if (src.HasMember("stringValue")) {
        dst.SetString(src["stringValue"].GetString(), alloc);
      }
      else if (src.HasMember("integerValue")) {
        // Firestore sends integers as strings
        const char* s = src["integerValue"].GetString();
        dst.SetInt64(std::stoll(s));
      }
      else if (src.HasMember("doubleValue")) {
        dst.SetDouble(src["doubleValue"].GetDouble());
      }
      else if (src.HasMember("booleanValue")) {
        dst.SetBool(src["booleanValue"].GetBool());
      }
      else if (src.HasMember("nullValue")) {
        dst.SetNull();
      }
      else if (src.HasMember("mapValue")) {
        dst.SetObject();
        if (src["mapValue"].HasMember("fields")) {
          const auto& fields = src["mapValue"]["fields"];
          for (auto it = fields.MemberBegin(); it != fields.MemberEnd(); ++it) {
            rapidjson::Value child;
            from_firestore_value(it->value, child, alloc);
            dst.AddMember(
              rapidjson::Value(it->name, alloc),
              child, alloc);
          }
        }
      }
      else if (src.HasMember("arrayValue")) {
        dst.SetArray();
        if (src["arrayValue"].HasMember("values")) {
          const auto& values = src["arrayValue"]["values"];
          for (rapidjson::SizeType i = 0; i < values.Size(); ++i) {
            rapidjson::Value child;
            from_firestore_value(values[i], child, alloc);
            dst.PushBack(child, alloc);
          }
        }
      }
      else {
        dst.SetNull(); // Unknown type
      }
    }

    // ----------------------------------------------------------------
    // High-level string helpers
    // ----------------------------------------------------------------

    // Convert flat JSON string  -->  Firestore document body
    // Input:  {"name":"Alice","score":100}
    // Output: {"fields":{"name":{"stringValue":"Alice"},"score":{"integerValue":"100"}}}
    inline std::string to_firestore_document(const std::string& flat_json)
    {
      rapidjson::Document src;
      src.Parse(flat_json.c_str());
      if (src.HasParseError() || !src.IsObject()) return "{}";

      rapidjson::Document dst;
      dst.SetObject();
      auto& alloc = dst.GetAllocator();

      rapidjson::Value fields(rapidjson::kObjectType);
      for (auto it = src.MemberBegin(); it != src.MemberEnd(); ++it) {
        rapidjson::Value val;
        to_firestore_value(it->value, val, alloc);
        fields.AddMember(
          rapidjson::Value(it->name, alloc),
          val, alloc);
      }
      dst.AddMember("fields", fields, alloc);

      rapidjson::StringBuffer buf;
      rapidjson::Writer<rapidjson::StringBuffer> writer(buf);
      dst.Accept(writer);
      return buf.GetString();
    }

    // Convert Firestore document response  -->  flat JSON string
    // Strips the "name", "createTime", "updateTime" metadata fields,
    // keeps only the user data under "fields".
    inline std::string from_firestore_document(const std::string& firestore_json)
    {
      rapidjson::Document src;
      src.Parse(firestore_json.c_str());
      if (src.HasParseError()) return "{}";

      // Single document
      if (src.IsObject() && src.HasMember("fields")) {
        rapidjson::Document dst;
        dst.SetObject();
        auto& alloc = dst.GetAllocator();

        const auto& fields = src["fields"];
        for (auto it = fields.MemberBegin(); it != fields.MemberEnd(); ++it) {
          rapidjson::Value val;
          from_firestore_value(it->value, val, alloc);
          dst.AddMember(
            rapidjson::Value(it->name, alloc),
            val, alloc);
        }

        rapidjson::StringBuffer buf;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buf);
        dst.Accept(writer);
        return buf.GetString();
      }

      return "{}";
    }

    // Convert a Firestore list-documents response into a JSON array of flat objects.
    // Each element also gets an injected "_id" field with the document ID.
    // Input: {"documents":[ {Firestore doc}, ... ]}
    // Output: [{"_id":"doc1","name":"Alice"}, ...]
    inline std::string from_firestore_collection(const std::string& firestore_json)
    {
      rapidjson::Document src;
      src.Parse(firestore_json.c_str());
      if (src.HasParseError()) return "[]";

      rapidjson::Document dst;
      dst.SetArray();
      auto& alloc = dst.GetAllocator();

      if (src.IsObject() && src.HasMember("documents") && src["documents"].IsArray()) {
        for (rapidjson::SizeType i = 0; i < src["documents"].Size(); ++i) {
          const auto& doc = src["documents"][i];
          rapidjson::Value obj(rapidjson::kObjectType);

          // Extract document ID from the full path
          if (doc.HasMember("name") && doc["name"].IsString()) {
            std::string full_path = doc["name"].GetString();
            auto last_slash = full_path.rfind('/');
            std::string doc_id = (last_slash != std::string::npos)
              ? full_path.substr(last_slash + 1)
              : full_path;
            obj.AddMember("_id", rapidjson::Value(doc_id.c_str(), alloc), alloc);
          }

          if (doc.HasMember("fields") && doc["fields"].IsObject()) {
            const auto& fields = doc["fields"];
            for (auto it = fields.MemberBegin(); it != fields.MemberEnd(); ++it) {
              rapidjson::Value val;
              from_firestore_value(it->value, val, alloc);
              obj.AddMember(
                rapidjson::Value(it->name, alloc),
                val, alloc);
            }
          }

          dst.PushBack(obj, alloc);
        }
      }

      rapidjson::StringBuffer buf;
      rapidjson::Writer<rapidjson::StringBuffer> writer(buf);
      dst.Accept(writer);
      return buf.GetString();
    }

  } // namespace Firebase
} // namespace Engine
