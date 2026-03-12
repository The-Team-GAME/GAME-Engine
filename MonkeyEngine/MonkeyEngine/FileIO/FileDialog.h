//-----------------------------------------------------------------------------------
// file:    FileDialog.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   Header file for native file dialog utilities using Windows API.
//          Provides functions to open save/load file dialogs.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include <string>
#include <optional>

namespace Engine {
  namespace FileIO {

    /*=================================================================================
    Function Brief:
    Opens a native Windows save file dialog and returns the selected path.

    Parameters:
    title - Dialog window title
    default_filename - Default filename to show in the dialog
    filter - File type filter (e.g., "Level Files (*.json)\0*.json\0All Files\0*.*\0")
    default_extension - Default file extension to append if user doesn't specify one
    initial_dir - Initial directory to open the dialog in (optional)

    Returns:
    std::optional<std::string> containing the selected file path if user confirmed,
    std::nullopt if user cancelled.

    Where to call it:
    When you need the user to choose where to save a file.
    ==================================================================================*/
    std::optional<std::string> show_save_file_dialog(
      const std::string& title = "Save File",
      const std::string& default_filename = "untitled",
      const char* filter = "All Files\0*.*\0",
      const std::string& default_extension = "",
      const std::string& initial_dir = ""
    );

    /*=================================================================================
    Function Brief:
    Opens a native Windows open file dialog and returns the selected path.

    Parameters:
    title - Dialog window title
    filter - File type filter (e.g., "Level Files (*.json)\0*.json\0All Files\0*.*\0")
    initial_dir - Initial directory to open the dialog in (optional)

    Returns:
    std::optional<std::string> containing the selected file path if user confirmed,
    std::nullopt if user cancelled.

    Where to call it:
    When you need the user to choose a file to open.
    ==================================================================================*/
    std::optional<std::string> show_open_file_dialog(
      const std::string& title = "Open File",
      const char* filter = "All Files\0*.*\0",
      const std::string& initial_dir = ""
    );

  } // namespace FileIO
} // namespace Engine
