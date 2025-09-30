#include "rapidjson/document.h"
#include "rapidjson/filereadstream.h"
#include "rapidjson/istreamwrapper.h"
#include "Logger.h" // contains string and iostream already
#include "FileHandler.h"
#include "TypeDef.h"
#include <fstream>

namespace rj = rapidjson;
namespace fh = FileHandler;

namespace { // Annonymous/ local namespcae
	const constexpr char* filepath = "Assets/config/";
	const constexpr char* filename = "config.json";
	const constexpr char* container = "config";
	rj::Document m_document;


	// Config values to be stored
	s32 window_width;
	s32 window_height;
}

namespace Config {

	void load_config()
	{
		fh::set_container(container);
		std::string complete_filepath =  std::string(filepath) + std::string(filename);
		fh::parse_file(complete_filepath);
		if (fh::has_key("win_width"))
		{
			if (fh::get_s32("win_width") != fh::d_s32())
			{
				window_width = fh::get_s32("win_width");
			}
			else { Log::warning("Key: win_width does not contain a value."); }
		}
		else {
			Log::warning("Key: win_width does not exist.");
		}
		if (fh::has_key("win_height"))
		{
			if (fh::get_s32("win_height") != fh::d_s32())
			{
				window_height = fh::get_s32("win_height");
			}
			else { Log::warning("Key: win_height does not contain a value."); }
		}
		else {
			Log::warning("Key: win_height does not exist.");
		}

		fh::clear_json();
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