#include "../Dependencies/rapidjson/document.h"
#include "../Dependencies/rapidjson/filereadstream.h"
#include "Logger.h" // contains string and iostream already
#include "../Dependencies/rapidjson/istreamwrapper.h"
#include <cstdio>
#include <fstream>
#include <iostream>
namespace rj = rapidjson;

std::string test_savefile = "test_save.json";
std::string test_loadfile = "Assets/objects/enemy.json";

rj::Document m_document;

namespace FileHandler {
	void load_file(std::string filepath)
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

		// At this point the filestream has been successfully opened and the json has been parsed
		ifs.close();
		Log::info("File successfully loaded from " + filepath);
	}

	void clear_json() // clears the json document so more things can be parsed in
	{
		m_document.RemoveAllMembers();
		Log::info("Json Document Cleared");
	}
}

