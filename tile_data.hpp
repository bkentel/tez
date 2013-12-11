#pragma once

#include <bklib/types.hpp>
#include <bklib/math.hpp>

namespace tez {

enum class tile_type : uint8_t {
    empty
  , floor
  , wall
  , ceiling
  , door

  , COUNT
};

enum class direction {
    here, north, north_east, east, south_east, south, south_west, west, north_west
};

static char as_char(tile_type const t) {
    switch (t) {
    case tile_type::empty   : return ' ';
    case tile_type::floor   : return '.';
    case tile_type::wall    : return '#';
    case tile_type::ceiling : return '%';
    case tile_type::door    : return '+';
    }
    
    return '?';
}

struct tile_data {
    using offset_t = bklib::point2d<uint16_t>;

    explicit tile_data(tile_type type = tile_type::empty)
      : data{0}
      , offset{{0, 0}}
      , sub_type{0}
      , type{type}
      , variation{0}
    {
    }

    uint64_t  data;      //type specific data use.
    offset_t  offset;    //image offset in the tile map.
    uint16_t  sub_type;  //subtype of the tile.
    tile_type type;      //main type of the tile.
    uint8_t   variation; //cosmetic variation.
};

static_assert(sizeof(tile_data) == 16, "unexpected size");



} //namespace tez
