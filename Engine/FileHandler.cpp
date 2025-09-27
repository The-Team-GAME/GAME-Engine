#include "rapidjson/document.h"
#include "rapidjson/filereadstream.h"
#include "rapidjson/istreamwrapper.h"
#include "Logger.h" // contains string and iostream already
#include "TypeDef.h"
#include <cstdio>
#include <fstream>
#include <iostream>

namespace rj = rapidjson;

const f32 default_f32 = 0.f;
const s32 default_s32 = -1;
const std::string default_string = "";

const char* container = "entity";
rj::Document m_document;
 
namespace FileHandler {
	f32 d_f32()							{ return default_f32; }
	s32 d_s32()							{ return default_s32; }
	std::string d_string()	{ return default_string; }

	void parse_file(std::string filepath)
	{
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

		// TODO: Give the json container a name and check that it exists
		// Ensure the JSON container exists and is an object.
		if (!m_document.HasMember(container) || !m_document[container].IsObject())
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

	bool has_key(std::string const& key)
	{
		// Check that our container exists first.
		if (!m_document.HasMember(container))
		{
			return false;
		}
		return m_document[container].HasMember(key.c_str());
	}

	void delete_key(std::string const& key)
	{
		if (m_document.HasMember(container) && m_document[container].HasMember(key.c_str()))
		{
			m_document[container].RemoveMember(key.c_str());
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
		return has_key(key) ? m_document[container][key.c_str()].GetFloat() 
												: default_f32;
	}

	f32 get_s32(const std::string& key)
	{
		return has_key(key) ? m_document[container][key.c_str()].GetInt()
			: default_s32;
	}

	std::string get_string(const std::string& key)
	{
		return has_key(key) ? m_document[container][key.c_str()].GetString() 
												: default_string;
	}

}

