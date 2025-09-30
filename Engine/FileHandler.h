#pragma once
#include <string>
#include <vector>
#include "TypeDef.h"

namespace FileHandler {

	f32 d_f32();
	s32 d_s32();
	std::string d_string();

	void parse_file(std::string filepath);

	void clear_json(); // clears the js::Document so another file can be parsed in

	void set_container(std::string str);

	bool has_key(std::string const& key); // Checks if a key is present

	void delete_key(std::string const& key); // Deletes the key

	f32 get_f32(const std::string& key);

	s32 get_s32(const std::string& key);

	std::string get_string(const std::string& key);

	// Array support
	bool is_array(const std::string& key);

	s32 get_array_size(const std::string& key);

	std::string get_string_from_array(const std::string& key, s32 index);

	f32 get_f32_from_array(const std::string& key, s32 index);

	s32 get_s32_from_array(const std::string& key, s32 index);

	// Object in array support
	bool has_key_in_array_object(const std::string& arrayKey, s32 index, const std::string& objectKey);

	std::string get_string_from_array_object(const std::string& arrayKey, s32 index, const std::string& objectKey);

	f32 get_f32_from_array_object(const std::string& arrayKey, s32 index, const std::string& objectKey);

	s32 get_s32_from_array_object(const std::string& arrayKey, s32 index, const std::string& objectKey);
}
