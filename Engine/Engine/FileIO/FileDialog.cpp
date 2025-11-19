//-----------------------------------------------------------------------------------
// file:    FileDialog.cpp
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   Implementation of native file dialog utilities using Windows API.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#include "FileDialog.h"
#include "../Logger.h"
#include <windows.h>
#include <commdlg.h>
#include <vector>

namespace Engine {
  namespace FileIO {

    std::optional<std::string> show_save_file_dialog(
      const std::string& title,
      const std::string& default_filename,
      const char* filter,
      const std::string& default_extension,
      const std::string& initial_dir
    )
    {
      OPENFILENAMEA ofn;
      char szFile[MAX_PATH] = { 0 };

      // Copy default filename to buffer
      if (!default_filename.empty()) {
        strncpy_s(szFile, default_filename.c_str(), MAX_PATH - 1);
      }

      // Initialize OPENFILENAME structure
      ZeroMemory(&ofn, sizeof(ofn));
      ofn.lStructSize = sizeof(ofn);
      ofn.hwndOwner = NULL; // Could be set to main window handle if needed
      ofn.lpstrFile = szFile;
      ofn.nMaxFile = sizeof(szFile);
      ofn.lpstrFilter = filter;
      ofn.nFilterIndex = 1;
      ofn.lpstrFileTitle = NULL;
      ofn.nMaxFileTitle = 0;
      ofn.lpstrInitialDir = initial_dir.empty() ? NULL : initial_dir.c_str();
      ofn.lpstrTitle = title.c_str();
      ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR;

      // Set default extension if provided
      if (!default_extension.empty()) {
        ofn.lpstrDefExt = default_extension.c_str();
      }

      // Display the save dialog
      if (GetSaveFileNameA(&ofn) == TRUE) {
        Log::info("File dialog: User selected: " + std::string(ofn.lpstrFile));
        return std::string(ofn.lpstrFile);
      }
      else {
        DWORD error = CommDlgExtendedError();
        if (error != 0) {
          Log::warning("File dialog error code: " + std::to_string(error));
        }
        else {
          Log::info("File dialog: User cancelled");
        }
        return std::nullopt;
      }
    }

    std::optional<std::string> show_open_file_dialog(
      const std::string& title,
      const char* filter,
      const std::string& initial_dir
    )
    {
      OPENFILENAMEA ofn;
      char szFile[MAX_PATH] = { 0 };

      // Initialize OPENFILENAME structure
      ZeroMemory(&ofn, sizeof(ofn));
      ofn.lStructSize = sizeof(ofn);
      ofn.hwndOwner = NULL;
      ofn.lpstrFile = szFile;
      ofn.nMaxFile = sizeof(szFile);
      ofn.lpstrFilter = filter;
      ofn.nFilterIndex = 1;
      ofn.lpstrFileTitle = NULL;
      ofn.nMaxFileTitle = 0;
      ofn.lpstrInitialDir = initial_dir.empty() ? NULL : initial_dir.c_str();
      ofn.lpstrTitle = title.c_str();
      ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

      // Display the open dialog
      if (GetOpenFileNameA(&ofn) == TRUE) {
        Log::info("File dialog: User selected: " + std::string(ofn.lpstrFile));
        return std::string(ofn.lpstrFile);
      }
      else {
        DWORD error = CommDlgExtendedError();
        if (error != 0) {
          Log::warning("File dialog error code: " + std::to_string(error));
        }
        else {
          Log::info("File dialog: User cancelled");
        }
        return std::nullopt;
      }
    }

  } // namespace FileIO
} // namespace Engine
