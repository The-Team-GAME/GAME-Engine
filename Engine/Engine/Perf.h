//-----------------------------------------------------------------------------------
// file:    Perf.h
// project: Monkey Stole My GPA
//
// author:  How Hong Chin Ryan (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   This file defines a performance profiling utility that measures and
//          tracks execution time for labeled code sections using high-resolution
//          timers.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once
#include <string>
#include <unordered_map>
#include <chrono>
#include "typedef.h"

namespace Engine
{
  namespace Perf
  {
    /*=================================================================================
    Function Brief:
    Starts a performance timer for a labeled code section. Records the current
    high-resolution timestamp.

    Parameters:
    label - A string identifier for the code section being timed.

    Where to call it:
    At the beginning of a code section you want to profile, before the operations
    you wish to measure.
    ==================================================================================*/
    void start(const std::string& label);

    /*=================================================================================
    Function Brief:
    Stops the performance timer for a labeled code section and calculates the
    elapsed time since the corresponding start() call.

    Parameters:
    label - A string identifier for the code section being timed (must match the
            label used in start()).

    Where to call it:
    At the end of the code section you want to profile, after all operations have
    completed.
    ==================================================================================*/
    void stop(const std::string& label);

    /*=================================================================================
    Function Brief:
    Retrieves the elapsed time for a labeled code section and prints it to the
    console in both seconds and milliseconds.

    Parameters:
    label - A string identifier for the code section whose elapsed time to retrieve.

    Returns:
    The elapsed time in seconds as a float. Returns 0.0f if the label doesn't exist.
    ==================================================================================*/
    f32 get_elapsed(const std::string& label);

    /*=================================================================================
    Function Brief:
    Retrieves the entire map of all recorded elapsed times.

    Returns:
    A constant reference to the unordered map containing all label-time pairs.
    ==================================================================================*/
    const std::unordered_map<std::string, f64>& get_elapsed_time_map();

    /*=================================================================================
    Function Brief:
    Calculates the total elapsed time across all labeled code sections.

    Returns:
    The sum of all recorded elapsed times in seconds as a double.
    ==================================================================================*/
    f64 get_total_time();
  }
}
