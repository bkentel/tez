#pragma once

#include "types.hpp"

namespace tez {
//==============================================================================
//! Command types.
//==============================================================================
enum class command_t {
    NONE      = 0
  , NOT_FOUND = 0
    //--------------------------------------------------------------------------
  , USE
  , DIR_NORTH_WEST
  , DIR_NORTH
  , DIR_NORTH_EAST
  , DIR_WEST
  , DIR_HERE
  , DIR_EAST
  , DIR_SOUTH_WEST
  , DIR_SOUTH
  , DIR_SOUTH_EAST
  , DIR_UP
  , DIR_DOWN
    //--------------------------------------------------------------------------
  , SIZE           //!< not a command; size of the enum only.
};

//==============================================================================
//! Command translation.
//! @note not thread safe.
//==============================================================================
class command {
public:
    //! command_t <-> string
    static string_ref translate(command_t cmd);
    static command_t  translate(utf8string const& string);
    static command_t  translate(string_ref string);
    static command_t  translate(hash_t hash);
};

} //namespace tez
