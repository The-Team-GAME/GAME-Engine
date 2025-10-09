//-----------------------------------------------------------------------------------
// file:    FileExplorer.h
// project: Monkey Stole My GPA
//
// author:  Ryan Ang
//          Primary author
//
// email:   a.ryanankiat@digipen.edu
//
// brief:   This file aids in creating the functions that help 
//					implement the FileExplorer utility function. It provides a
//          graphical interface for the user to browse the filesystem, primarily
//          to select image files (.jpg, .png, .jpeg). It handles directory traversal,
//          path history, and filtering to return the selected file path.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------

#pragma once

#include <string>
#include <filesystem>
#include <vector>
#include <algorithm>
#include <cstdlib>
#include <stack>

namespace fs = std::filesystem;

// image file path
extern std::string selected_file_path;

// file directory (for manual file changing if needed)
extern std::string root_path; //currently unused

// my file directory <3
void FileExplorer(bool* p_open);