//-----------------------------------------------------------------------------------
// file:    FileHandler.h
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
#pragma once
#include <string>
#include <vector>
#include "TypeDef.h"

namespace FileHandler {

	/*=================================================================================
	Function Brief:
	The functions below are a set of accessors for the default values for each type to
	be returned when the getter functions are called and there is no value returned.
	Think of it as my own implementation of value_or in std::optional
	==================================================================================*/
	f32 d_f32();
	s32 d_s32();
	std::string d_string();
	// End of default value accessors

	/*=================================================================================
	Function Brief:
	The function take in a json file and deseralises it into a readable format that can
	allow its objects/ members to be parsed into respective variables using the 
	rapidjson library.
	==================================================================================*/
	void parse_file(std::string filepath);

	/*=================================================================================
	Function Brief:
	The function clears the rapidjson::Document variables that holds the parsed file
	stream.

	Where to call it:
	Usually after a json file has been read and the values parsed into their appropriate
	variables or before another json file is about to be read. Clearing the 
	json Document basically ensures that nothing foreign ends up in the new stream that
	you are going to read.
	==================================================================================*/
	void clear_json(); // clears the js::Document so another file can be parsed in

	void set_container(std::string str);

	bool has_key(std::string const& key); // Checks if a key is present

	void delete_key(std::string const& key); // Deletes the key

	/*=================================================================================
	Function Brief:
	The functions below are a set of getters for the values of the corresponding key 
	found in the json files. They return a default value for each type where a value
	value cannot be obtained.
	==================================================================================*/
	f32 get_f32(const std::string& key);

	s32 get_s32(const std::string& key);

	std::string get_string(const std::string& key);
	// End of getter functions


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
