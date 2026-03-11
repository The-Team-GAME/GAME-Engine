//-----------------------------------------------------------------------------------
// file:    FirebaseLuaBindings.h
// project: Monkey Stole My GPA
//
// brief:   Exposes the Firebase Cloud Firestore API to Lua scripts via a global
//          "Cloud" table.  Handles Lua-table <-> JSON conversion and stores
//          sol::protected_function callbacks that are invoked on the main thread
//          when async HTTP requests complete.
//
// Usage from Lua:
//
//   -- Write data
//   Cloud.set("leaderboard", "player1", { name = "Alice", score = 100 },
//     function(ok, data)
//       if ok then log_message("Saved!") end
//     end)
//
//   -- Read data
//   Cloud.get("leaderboard", "player1", function(ok, data)
//     if ok then log_message("Score: " .. data.score) end
//   end)
//
//   -- Delete
//   Cloud.delete("leaderboard", "player1", function(ok, msg) end)
//
//   -- List collection
//   Cloud.query("leaderboard", function(ok, docs)
//     for i, doc in ipairs(docs) do
//       log_message(doc._id .. ": " .. doc.score)
//     end
//   end)
//
//   -- Check status
//   local ready = Cloud.is_configured()     -- true if project_id + api_key set
//   local count = Cloud.pending_count()     -- number of in-flight requests
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include "../Lua/LuaManager.h"
#include "FirebaseManager.h"

#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
#include <sol/sol.hpp>

namespace Engine
{
    namespace Firebase
    {
        namespace LuaBindings
        {
            // ================================================================
            // Lua table  ↔  JSON string conversion
            // ================================================================

            // Forward declarations
            inline void lua_value_to_json(const sol::object& obj,
                rapidjson::Value& out,
                rapidjson::Document::AllocatorType& alloc);
            inline sol::object json_value_to_lua(sol::state& lua,
                const rapidjson::Value& val);

            // ------- Lua value  →  RapidJSON value -------------------------
            inline void lua_value_to_json(const sol::object& obj,
                rapidjson::Value& out,
                rapidjson::Document::AllocatorType& alloc)
            {
                switch (obj.get_type()) {
                case sol::type::string: {
                    std::string s = obj.as<std::string>();
                    out.SetString(s.c_str(), static_cast<rapidjson::SizeType>(s.size()), alloc);
                    break;
                }
                case sol::type::number: {
                    double d = obj.as<double>();
                    // If it looks like an integer, store as int
                    if (d == static_cast<double>(static_cast<int64_t>(d))) {
                        out.SetInt64(static_cast<int64_t>(d));
                    }
                    else {
                        out.SetDouble(d);
                    }
                    break;
                }
                case sol::type::boolean:
                    out.SetBool(obj.as<bool>());
                    break;
                case sol::type::table: {
                    sol::table tbl = obj.as<sol::table>();

                    // Determine if this is an array (sequential integer keys 1..N) or a map
                    bool is_array = true;
                    size_t count = 0;
                    tbl.for_each([&](const sol::object& key, const sol::object&) {
                        ++count;
                        if (key.get_type() != sol::type::number) {
                            is_array = false;
                        }
                        });
                    // Also check that keys are 1..N
                    if (is_array && count > 0) {
                        for (size_t i = 1; i <= count; ++i) {
                            if (!tbl[i].valid() || tbl[i].get_type() == sol::type::lua_nil) {
                                is_array = false;
                                break;
                            }
                        }
                    }
                    if (count == 0) is_array = false;

                    if (is_array) {
                        out.SetArray();
                        for (size_t i = 1; i <= count; ++i) {
                            rapidjson::Value child;
                            lua_value_to_json(tbl[i], child, alloc);
                            out.PushBack(child, alloc);
                        }
                    }
                    else {
                        out.SetObject();
                        tbl.for_each([&](const sol::object& key, const sol::object& val) {
                            std::string k = key.as<std::string>();
                            rapidjson::Value child;
                            lua_value_to_json(val, child, alloc);
                            out.AddMember(
                                rapidjson::Value(k.c_str(), static_cast<rapidjson::SizeType>(k.size()), alloc),
                                child, alloc);
                            });
                    }
                    break;
                }
                default:
                    out.SetNull();
                    break;
                }
            }

            // ------- RapidJSON value  →  Lua object ------------------------
            inline sol::object json_value_to_lua(sol::state& lua,
                const rapidjson::Value& val)
            {
                if (val.IsString()) return sol::make_object(lua, std::string(val.GetString()));
                if (val.IsBool())   return sol::make_object(lua, val.GetBool());
                if (val.IsInt())    return sol::make_object(lua, val.GetInt());
                if (val.IsInt64())  return sol::make_object(lua, static_cast<double>(val.GetInt64()));
                if (val.IsDouble()) return sol::make_object(lua, val.GetDouble());
                if (val.IsNull())   return sol::make_object(lua, sol::lua_nil);

                if (val.IsObject()) {
                    sol::table tbl = lua.create_table();
                    for (auto it = val.MemberBegin(); it != val.MemberEnd(); ++it) {
                        tbl[std::string(it->name.GetString())] = json_value_to_lua(lua, it->value);
                    }
                    return tbl;
                }

                if (val.IsArray()) {
                    sol::table tbl = lua.create_table();
                    for (rapidjson::SizeType i = 0; i < val.Size(); ++i) {
                        tbl[static_cast<int>(i + 1)] = json_value_to_lua(lua, val[i]);
                    }
                    return tbl;
                }

                return sol::make_object(lua, sol::lua_nil);
            }

            // ------- High level: Lua table  →  JSON string -----------------
            inline std::string lua_table_to_json(const sol::table& tbl)
            {
                rapidjson::Document doc;
                doc.SetObject();
                auto& alloc = doc.GetAllocator();

                tbl.for_each([&](const sol::object& key, const sol::object& val) {
                    std::string k = key.as<std::string>();
                    rapidjson::Value child;
                    lua_value_to_json(val, child, alloc);
                    doc.AddMember(
                        rapidjson::Value(k.c_str(), static_cast<rapidjson::SizeType>(k.size()), alloc),
                        child, alloc);
                    });

                rapidjson::StringBuffer buf;
                rapidjson::Writer<rapidjson::StringBuffer> writer(buf);
                doc.Accept(writer);
                return buf.GetString();
            }

            // ------- High level: JSON string  →  Lua object ----------------
            inline sol::object json_to_lua(sol::state& lua, const std::string& json)
            {
                rapidjson::Document doc;
                doc.Parse(json.c_str());
                if (doc.HasParseError()) {
                    return sol::make_object(lua, sol::lua_nil);
                }

                // If the root is an array, convert to Lua array table
                if (doc.IsArray()) {
                    sol::table arr = lua.create_table();
                    for (rapidjson::SizeType i = 0; i < doc.Size(); ++i) {
                        arr[static_cast<int>(i + 1)] = json_value_to_lua(lua, doc[i]);
                    }
                    return arr;
                }

                // Otherwise it's an object
                return json_value_to_lua(lua, doc);
            }

            // ================================================================
            // Register "Cloud" global table in Lua
            // ================================================================
            inline void register_bindings()
            {
                Engine::LuaManager::register_binding([](sol::state& lua) {

                    sol::table cloud = lua.create_table();

                    // Cloud.is_configured() -> bool
                    cloud.set_function("is_configured", []() -> bool {
                        return Engine::Firebase::is_configured();
                        });

                    // Cloud.pending_count() -> number
                    cloud.set_function("pending_count", []() -> int {
                        return static_cast<int>(Engine::Firebase::pending_count());
                        });

                    // Cloud.get(collection, doc_id, callback)
                    // callback(success: bool, data: table|string)
                    cloud.set_function("get",
                        [&lua](const std::string& collection,
                            const std::string& doc_id,
                            sol::protected_function callback) {

                                // Store the Lua callback so it survives until the async response
                                auto shared_cb = std::make_shared<sol::protected_function>(std::move(callback));

                                Engine::Firebase::get_document(collection, doc_id,
                                    [&lua, shared_cb](bool success, const std::string& data) {
                                        if (*shared_cb) {
                                            if (success) {
                                                sol::object lua_data = json_to_lua(lua, data);
                                                (*shared_cb)(success, lua_data);
                                            }
                                            else {
                                                (*shared_cb)(success, data);
                                            }
                                        }
                                    });
                        });

                    // Cloud.set(collection, doc_id, data_table, callback)
                    cloud.set_function("set",
                        [&lua](const std::string& collection,
                            const std::string& doc_id,
                            sol::table data,
                            sol::optional<sol::protected_function> callback) {

                                std::string json_data = lua_table_to_json(data);

                                std::shared_ptr<sol::protected_function> shared_cb;
                                if (callback.has_value()) {
                                    shared_cb = std::make_shared<sol::protected_function>(std::move(callback.value()));
                                }

                                Engine::Firebase::set_document(collection, doc_id, json_data,
                                    [&lua, shared_cb](bool success, const std::string& data) {
                                        if (shared_cb && *shared_cb) {
                                            if (success) {
                                                sol::object lua_data = json_to_lua(lua, data);
                                                (*shared_cb)(success, lua_data);
                                            }
                                            else {
                                                (*shared_cb)(success, data);
                                            }
                                        }
                                    });
                        });

                    // Cloud.delete(collection, doc_id, callback)
                    cloud.set_function("delete",
                        [&lua](const std::string& collection,
                            const std::string& doc_id,
                            sol::optional<sol::protected_function> callback) {

                                std::shared_ptr<sol::protected_function> shared_cb;
                                if (callback.has_value()) {
                                    shared_cb = std::make_shared<sol::protected_function>(std::move(callback.value()));
                                }

                                Engine::Firebase::delete_document(collection, doc_id,
                                    [shared_cb](bool success, const std::string& data) {
                                        if (shared_cb && *shared_cb) {
                                            (*shared_cb)(success, data);
                                        }
                                    });
                        });

                    // Cloud.query(collection, callback)
                    // callback(success: bool, docs: table[])
                    cloud.set_function("query",
                        [&lua](const std::string& collection,
                            sol::protected_function callback) {

                                auto shared_cb = std::make_shared<sol::protected_function>(std::move(callback));

                                Engine::Firebase::query_collection(collection,
                                    [&lua, shared_cb](bool success, const std::string& data) {
                                        if (*shared_cb) {
                                            if (success) {
                                                sol::object lua_data = json_to_lua(lua, data);
                                                (*shared_cb)(success, lua_data);
                                            }
                                            else {
                                                (*shared_cb)(success, data);
                                            }
                                        }
                                    });
                        });

                    // Cloud.raw(method, url, body, callback)
                    cloud.set_function("raw",
                        [&lua](const std::string& method,
                            const std::string& url,
                            const std::string& body,
                            sol::optional<sol::protected_function> callback) {

                                std::shared_ptr<sol::protected_function> shared_cb;
                                if (callback.has_value()) {
                                    shared_cb = std::make_shared<sol::protected_function>(std::move(callback.value()));
                                }

                                Engine::Firebase::raw_request(method, url, body,
                                    [&lua, shared_cb](bool success, const std::string& data) {
                                        if (shared_cb && *shared_cb) {
                                            (*shared_cb)(success, data);
                                        }
                                    });
                        });

                    lua["Cloud"] = cloud;

                    std::cout << "[Firebase] Cloud API bound to Lua\n";
                    });
            }

        } // namespace LuaBindings
    } // namespace Firebase
} // namespace Engine
