#pragma once
#include <string>
#include "TypeDef.h"
namespace FileHandler {

	f32 d_f32();
	s32 d_s32();
	std::string d_string();

	void parse_file(std::string filepath);

	void clear_json(); // clears the js::Document so another file can be parsed in

	void set_container(std::string str);

	bool has_key(std::string const& key); // Checks if a key is present

	void delete_key(std::string const& key); // Delets the key

	f32 get_f32(const std::string& key);

	s32 get_s32(const std::string& key);

	std::string get_string(const std::string& key);
}
