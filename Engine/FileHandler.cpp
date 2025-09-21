#include "../Dependencies/rapidjson/document.h"
#include "../Dependencies/rapidjson/filereadstream.h"
#include "Logger.h" // contains string and iostream already
#include <cstdio>
#include <fstream>

namespace rj = rapidjson;

namespace {
	std::string test_savefile = "test_save.json";
	std::string test_loadfile = "Assets/objects/enemy.json";
}

namespace FileHandler {
	void load_file(std::string filepath)
	{
		std::ifstream ifs(filepath);
		if (!ifs.is_open())
		{
			Log::warning("Failed to open file at " + filepath);
			return;
		}

		rj::Document m_document;
		m_document.ParseStream(ifs);
		if (m_document.HasParseError())
		{
			Log::warning("Failed to parse json file.");
			return;
		}

		// At this point the filestream has been successfully opened and the json has been parsed
		ifs.close();
		Log::Info("File successfully loaded from " + filepath);
	}
}

