//-----------------------------------------------------------------------------------
// file:    FileHandler.cpp
// project: Monkey Stole My GPA
//
// author:  Cheng Ming Xian (100%)
//          Primary author
//					Shee Yang (?%)
//
// email:   c.mingxian@digipen.edu
//
// brief:   This file uses the rapidjson library to parse a filestream into a 
//					rapidjson::Document type and then use the other functions in this file
//					to read from and write to(wip) the json file.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#include "rapidjson/document.h"
#include "rapidjson/filereadstream.h"
#include "rapidjson/istreamwrapper.h"
#include "Logger.h"
#include "TypeDef.h"
#include <fstream>

namespace rj = rapidjson;

const f32 default_f32 = 0.f;
const s32 default_s32 = -1;
const std::string default_string = "";

std::string container = "";

rj::Document m_document;

namespace FileHandler {
  f32 d_f32() { return default_f32; }
  s32 d_s32() { return default_s32; }
  std::string d_string() { return default_string; }

	void parse_file(std::string filepath)
	{
		//clear_json(); // clearing the json stream incase there was something left over
		std::ifstream ifs(filepath);
		if (!ifs.is_open())
		{
			Log::warning("Failed to open file at " + filepath);
			return;
		}

    rj::IStreamWrapper isw(ifs);
    m_document.ParseStream(isw);
    if (m_document.HasParseError())
    {
      Log::warning("Failed to parse json file.");
      return;
    }

		// Ensure the JSON container exists and is an object.
		if (!m_document.HasMember(container.c_str()) || !m_document[container.c_str()].IsObject())
		{
			Log::warning("Container is either missing or incomplete.");
		}

    // At this point the filestream has been successfully opened and the json has been parsed
    ifs.close();
    Log::info("File successfully loaded from " + filepath);
  }

  void clear_json() // clears the json document so more things can be parsed in
  {
    m_document.RemoveAllMembers();
    Log::info("Json Document Cleared");
  }

	void set_container(std::string str)
	{
		container = str;
		return;
	}

	bool has_key(std::string const& key)
	{
		// Check that our container exists first.
		if (!m_document.HasMember(container.c_str()))
		{
			return false;
		}
		return m_document[container.c_str()].HasMember(key.c_str());
	}

	void delete_key(std::string const& key)
	{
		if (m_document.HasMember(container.c_str()) && m_document[container.c_str()].HasMember(key.c_str()))
		{
			m_document[container.c_str()].RemoveMember(key.c_str());
		}
		else
		{
			Log::warning("Attempted to delete a key that does not exist: " + key);
		}
	} 

	// Uses the same concept of value_or in std::optional but doing it this way so theres no need
	// to make a new type just for one value
	f32 get_f32(const std::string& key)
	{
		return has_key(key) ? m_document[container.c_str()][key.c_str()].GetFloat() 
												: default_f32;
	}

	s32 get_s32(const std::string& key)
	{
		return has_key(key) ? m_document[container.c_str()][key.c_str()].GetInt()
			: default_s32;
	}

	std::string get_string(const std::string& key)
	{
		return has_key(key) ? m_document[container.c_str()][key.c_str()].GetString() 
												: default_string;
	}

  // Array support
  bool is_array(const std::string& key)
  {
    if (!has_key(key)) return false;

    const rj::Value& entity = m_document[container.c_str()];
    return entity[key.c_str()].IsArray();
  }

  s32 get_array_size(const std::string& key)
  {
    if (!is_array(key)) return 0;

    const rj::Value& entity = m_document[container.c_str()];
    return static_cast<s32>(entity[key.c_str()].Size());
  }

  std::string get_string_from_array(const std::string& key, s32 index)
  {
    if (!is_array(key)) return default_string;

    const rj::Value& entity = m_document[container.c_str()];
    const rj::Value& array = entity[key.c_str()];

    if (index < 0 || index >= static_cast<s32>(array.Size())) {
      return default_string;
    }

    if (array[index].IsString()) {
      return array[index].GetString();
    }
    return default_string;
  }

  f32 get_f32_from_array(const std::string& key, s32 index)
  {
    if (!is_array(key)) return default_f32;

    const rj::Value& entity = m_document[container.c_str()];
    const rj::Value& array = entity[key.c_str()];

    if (index < 0 || index >= static_cast<s32>(array.Size())) {
      return default_f32;
    }

    if (array[index].IsNumber()) {
      return static_cast<f32>(array[index].GetFloat());
    }
    return default_f32;
  }

  s32 get_s32_from_array(const std::string& key, s32 index)
  {
    if (!is_array(key)) return default_s32;

    const rj::Value& entity = m_document[container.c_str()];
    const rj::Value& array = entity[key.c_str()];

    if (index < 0 || index >= static_cast<s32>(array.Size())) {
      return default_s32;
    }

    if (array[index].IsInt()) {
      return array[index].GetInt();
    }
    return default_s32;
  }

  bool has_key_in_array_object(const std::string& arrayKey, s32 index, const std::string& objectKey)
  {
    if (!is_array(arrayKey)) return false;

    const rj::Value& entity = m_document[container.c_str()];
    const rj::Value& array = entity[arrayKey.c_str()];

    if (index < 0 || index >= static_cast<s32>(array.Size())) {
      return false;
    }

    if (array[index].IsObject()) {
      return array[index].HasMember(objectKey.c_str());
    }
    return false;
  }

  std::string get_string_from_array_object(const std::string& arrayKey, s32 index, const std::string& objectKey)
  {
    if (!has_key_in_array_object(arrayKey, index, objectKey)) return default_string;

    const rj::Value& entity = m_document[container.c_str()];
    const rj::Value& array = entity[arrayKey.c_str()];
    const rj::Value& value = array[index][objectKey.c_str()];

    if (value.IsString()) {
      return value.GetString();
    }
    return default_string;
  }

  f32 get_f32_from_array_object(const std::string& arrayKey, s32 index, const std::string& objectKey)
  {
    if (!has_key_in_array_object(arrayKey, index, objectKey)) return default_f32;

    const rj::Value& entity = m_document[container.c_str()];
    const rj::Value& array = entity[arrayKey.c_str()];
    const rj::Value& value = array[index][objectKey.c_str()];

    if (value.IsNumber()) {
      return static_cast<f32>(value.GetFloat());
    }
    return default_f32;
  }

  s32 get_s32_from_array_object(const std::string& arrayKey, s32 index, const std::string& objectKey)
  {
    if (!has_key_in_array_object(arrayKey, index, objectKey)) return default_s32;

    const rj::Value& entity = m_document[container.c_str()];
    const rj::Value& array = entity[arrayKey.c_str()];
    const rj::Value& value = array[index][objectKey.c_str()];

    if (value.IsInt()) {
      return value.GetInt();
    }
    return default_s32;
  }
}

