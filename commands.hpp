#pragma once

#include <bklib/types.hpp>

namespace tez {
//==============================================================================
//! Game commands.
//==============================================================================
enum class command_type {
    NONE      = 0
  , NOT_FOUND = 0

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

  , SIZE           //!< not a command; size of the enum only.
};
//------------------------------------------------------------------------------
bklib::string_ref to_string(command_type type);
//------------------------------------------------------------------------------
command_type      to_command(bklib::utf8string const& string);
command_type      to_command(bklib::hash_t hash);
//------------------------------------------------------------------------------
} //namespace tez
