#pragma once

#include <bklib/config.hpp>
#include <bklib/assert.hpp>

#include <bklib/math.hpp>

#include "algorithms.hpp"
#include "tile_data.hpp"
#include "grid2d.hpp"

namespace tez {

using random = std::mt19937;

//==============================================================================
//! A single room.
//==============================================================================
class room : public grid2d<tile_data> {
public:
    using rect  = bklib::axis_aligned_rect<int>;
    using point = bklib::point2d<int>;

    //No implicit copies
    room(room const&) = delete;
    room& operator=(room const&) = delete;

    //Move operators
    room(room&& other) = default;
    room& operator=(room&& rhs) = default;

    room(index_t w, index_t h, tile_data value = tile_data{});
};

class map : public grid2d<tile_data> {
    using rect = bklib::axis_aligned_rect<int>;

    //No implicit copies
    map(map const&) = delete;
    map& operator=(map const&) = delete;

    map(index_t w, index_t h) : grid2d(w, h) {}

    void write(room const& src, rect const& room_rect) {
        auto const x = room_rect.left(); BK_ASSERT(x >= 0);
        auto const y = room_rect.top();  BK_ASSERT(y >= 0);

        auto& dest = *this;

        for (auto const& tile : src) {
            auto const& pos   = tile.i;
            auto const& value = tile.value;

            auto const xi = pos.x + x; BK_ASSERT(xi < width());
            auto const yi = pos.y + y; BK_ASSERT(yi < height());

            auto& t = dest[{xi, yi}];
            BK_ASSERT(t.type == tile_type::empty);

            t = value;
        }
    }
};

//==============================================================================
//! Procedural generation.
//==============================================================================
namespace generator {
//==============================================================================
//! Generator for simple rectangular rooms of fixed size.
//==============================================================================
struct room_simple_fixed {
    room_simple_fixed(unsigned w, unsigned h)
      : width_{w}, height_{h} {}

    room generate(random& rand) const;

    unsigned width_;
    unsigned height_;
};
//==============================================================================
//! Generator for simple rectangular rooms of random size.
//==============================================================================
struct room_simple {
    using distribution = std::uniform_int_distribution<size_t>;
    using range = std::pair<size_t, size_t>;

    room_simple(range w, range h)
      : width_{w.first, w.second}, height_{h.first, h.second} {}

    room generate(random& rand);

    distribution width_;
    distribution height_;
};

//==============================================================================
//! Generator for simple rectangular rooms of random size.
//==============================================================================
struct layout_random {
    using distribution = std::uniform_int_distribution<int>;
    using rect = bklib::axis_aligned_rect<int>;

    bklib::min_max<int> range_x_;
    bklib::min_max<int> range_y_;

    layout_random()
    {
    }

    struct looped_index {
        using dist = std::uniform_int_distribution<size_t>;

        looped_index(random& rand, size_t const size)
          : cur{dist(0, size ? size - 1 : 0)(rand)}
          , size{size ? size : 1}
        {
        }

        size_t operator++() { return (++cur % size); }
        operator size_t() const { return (cur % size); }

        size_t cur, size;
    };

    void update_ranges(rect const& r) {
        range_x_(r.left());
        range_x_(r.right());
        range_y_(r.top());
        range_y_(r.bottom());
    }

    void normalize() {
        auto v = bklib::make_vector2d(range_x_.min, range_y_.min);

        for (auto& r : rects_) {
            r -= v;
        }

        range_x_.max -= range_x_.min;
        range_x_.min = 0;

        range_y_.max -= range_y_.min;
        range_y_.min = 0;
    }

    bool verify() const {
        for (auto i = 0u; i < rects_.size(); ++i) {
            for (auto j = i + 1; j < rects_.size(); ++j) {
                auto const& a = rects_[i];
                auto const& b = rects_[j];
                if (bklib::intersects(a, b)) {
                    return false;
                }
            }
        }

        return true;
    }

    bool intersects(rect const r) const {
        return std::cend(rects_) != std::find_if(
            std::cbegin(rects_), std::cend(rects_)
          , [r](rect const ir) {
                return bklib::intersects(r, ir);
            }
        );
    }

    static int const MAX_ITERATIONS = 10;
    static int const MIN_SEPARATION = 1;

    void insert(random& rand, room&& new_room) {
        using namespace bklib;
        static auto const zero = make_vector2d(0.0f, 0.0f);

        auto const w = static_cast<int>(new_room.width());
        auto const h = static_cast<int>(new_room.height());
        auto const r = magnitude(make_vector2d(w/2.0f, h/2.0f));

        auto test_rect = rect{
            -MIN_SEPARATION
          , -MIN_SEPARATION
          , w + MIN_SEPARATION
          , h + MIN_SEPARATION
        };

        bool inserted  = rects_.empty();

        //calculate a displacement vector that is the sum of the vectors between
        //the centers of the existing rects which intersect test_rect.
        auto get_correction = [&](rect const& test_rect) {
            auto const c = bounding_circle(test_rect);
            return accumulate_if(rects_, zero
                , [&](rect const& r) { return bklib::intersects(r, test_rect); }
                , [&](vector2d<float> const& v, rect const& ir) {
                    return v + separation_vector(c, bounding_circle(ir));
                }
            );
        };

        //loop through existing rects with a random starting index.
        for (auto i = looped_index {rand, rects_.size()}; !inserted; ++i) {
            auto const cur_rect   = rects_[i];
            auto const cur_circle = bounding_circle(cur_rect);
            
            //try to place the new room at a location randomly rotated around
            //cur_rect.
            for (auto n = 0; !inserted && n < MAX_ITERATIONS; ++n) {
                auto const v = random_direction(rand) * (r + cur_circle.r);
                auto const p = round_toward<int>(cur_circle.p + v, v);

                test_rect =  rect(rect::center_point(p), w + 2*MIN_SEPARATION, h + 2*MIN_SEPARATION);

                auto const correction = get_correction(test_rect);
                inserted = (correction.first == 0);

                if (!inserted) {
                    test_rect += round_toward<int>(correction.second);
                    inserted  =  !intersects(test_rect);
                }
            }
        }

        test_rect = rect{
            test_rect.left()   + MIN_SEPARATION
          , test_rect.top()    + MIN_SEPARATION
          , test_rect.right()  - MIN_SEPARATION
          , test_rect.bottom() - MIN_SEPARATION
        };

        update_ranges(test_rect);
        rects_.emplace_back(test_rect);
        data_.emplace_back(std::move(new_room));
    }

    grid2d<tile_data> to_grid() const {
        BK_ASSERT(!rects_.empty());
        BK_ASSERT(rects_.size() == data_.size());

        auto result = grid2d<tile_data>(range_x_.range(), range_y_.range());

        for (size_t i = 0; i < rects_.size(); ++i) {
            auto const& rect = rects_[i];
            auto const& room = data_[i];

            for (auto const& tile_info : room) {
                auto pos = tile_info.i;
                pos.x += rect.left();
                pos.y += rect.top();

                result[pos] = tile_info.value;
            }
        }

        return result;
    }

    std::vector<rect> rects_;
    std::vector<room> data_;
};
//==============================================================================
} //namespace genertor

} //namespace tez
