#include "rapidjson/document.h"
#include "rapidjson/filereadstream.h"
#include "rapidjson/istreamwrapper.h"
#include "Logger.h" // contains string and iostream already
#include "TypeDef.h"
#include <fstream>

namespace rj = rapidjson;

namespace { // Annonymous/ local namespcae
	const constexpr char* filepath = "Assets/config/";
	const constexpr char* filename = "config.json";
	const constexpr char* container = "entity";
	rj::Document m_document;


	// Config values to be stored
	s32 window_width;
	s32 window_height;


	// Parse config handles the parsing of the data from the json file into the config variables
	// After the data has been read from the actual config file and passed into the js::document
	void parse_config(void);

	void parse_config()
	{

	}
}
namespace Config {

	void load_config(void)
	{
		std::string complete_filepath =  std::string(filepath) + std::string(filename);
		std::ifstream ifs(complete_filepath);
		if (!ifs.is_open())
		{
			Log::warning("Failed to open file at " + complete_filepath);
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
		if (!m_document.HasMember(container) || !m_document[container].IsObject())
		{
			Log::warning("Container is either missing or incomplete.");
		}

		// At this point the filestream has been successfully opened and the json has been parsed
		ifs.close();
		Log::info("File successfully loaded from " + complete_filepath);

		// Here we read the values from the config file into the variables
		parse_config();

		// Now we clear the json document after the data has been parsed in
		m_document.RemoveAllMembers();
		Log::info("Json Document Cleared");
	}

	// Accessors for config values
	s32 get_win_width()
	{
		return window_width;
	}

	s32 get_win_height()
	{
		return window_height;
	}
}