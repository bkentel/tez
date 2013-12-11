#include "room.hpp"
#include <bklib/macros.hpp>

//==============================================================================
using random = tez::random;
//==============================================================================
using room = tez::room;

room::room(
    index_t w
  , index_t h
  , tez::tile_data value
)
  : grid2d(w, h, value)
{
}

//==============================================================================
using room_simple = tez::generator::room_simple;

room room_simple::generate(random& rand) {
    auto const w = width_(rand);
    auto const h = height_(rand);

    return room_simple_fixed{w, h}.generate(rand);
}
//==============================================================================
using room_simple_fixed = tez::generator::room_simple_fixed;

room room_simple_fixed::generate(random& rand) const {
    BK_UNUSED(rand);

    auto const value = tez::tile_data{tez::tile_type::floor};

    auto result = room {width_, height_, value};

    auto edge_y = [&](unsigned const y) {
        for (auto x = 0u; x < width_; ++x) {
            result[{x, y}].type = tile_type::wall;
        }
    };

    auto edge_x = [&](unsigned const y) {
        for (auto const x : {0u, width_ - 1}) {
            result[{x, y}].type = tile_type::wall;
        }
    };

    edge_y(0);
    for (auto y = 1u; y < height_ - 1; ++y) edge_x(y);
    edge_y(height_ - 1);

    return result;
}
