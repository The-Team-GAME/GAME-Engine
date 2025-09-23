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