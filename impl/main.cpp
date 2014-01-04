#include <bklib/window.hpp>
#include <bklib/math.hpp>
#include <bklib/renderer2d.hpp>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "room.hpp"
#include "algorithms.hpp"
#include "hotkeys.hpp"

#include "types.hpp"

namespace tez {
using random_t  = std::mt19937;
}

template <typename Container, typename Function>
inline void for_each_i(Container& container, Function function) {
    using value_t = typename Container::value_type;

    size_t i = 0;

    std::for_each(std::begin(container), std::end(container), [&](value_t& value) {
        function(value, i++);
    });
}

template <typename Test, typename Fail>
using if_void_t = typename std::conditional<
    std::is_void<Test>::value
  , Fail
  , Test
>::type;

template <typename T>
inline T clamp(T value, T min, T max) {
    return (value < min)
      ? min
      : (value > max)
        ? max
        : value
    ;
}

template <typename Test, typename Result = void>
using enable_if_integral_t = typename std::enable_if<
    std::is_integral<Test>::value
  , Result
>::type;

template <typename Test, typename Result = void>
using enable_if_float_t = typename std::enable_if<
    std::is_floating_point<Test>::value
  , Result
>::type;


template <typename T, enable_if_integral_t<T>* = nullptr>
inline T ceil_div(T const dividend, T const divisor) {
    return (dividend / divisor) + ((dividend % divisor) ? 1 : 0);
}

template <typename T, enable_if_float_t<T>* = nullptr>
inline T ceil_div(T const dividend, T const divisor) {
    return std::ceil(dividend / divisor);
}

class room_rect_set {
public:
    ////////////////////////////////////////////////////////////////////////////
    // types
    ////////////////////////////////////////////////////////////////////////////
    using rect_t  = bklib::axis_aligned_rect<int>;
    using point_t = bklib::point2d<int>;

    struct value_t {
        value_t(rect_t base) : base {base} {}

        rect_t base     = rect_t {};
        rect_t hole     = rect_t {};
        bool   has_hole = false;
    };

    using container_t    = std::vector<value_t>;
    using iterator       = container_t::iterator;
    using const_iterator = container_t::const_iterator;
    ////////////////////////////////////////////////////////////////////////////
    // traversal
    ////////////////////////////////////////////////////////////////////////////
    const_iterator begin() const { return rects_.begin(); }
    const_iterator end()   const { return rects_.end(); }

    iterator begin() { return rects_.begin(); }
    iterator end()   { return rects_.end(); }

    ////////////////////////////////////////////////////////////////////////////
    // properties
    ////////////////////////////////////////////////////////////////////////////
    bool   empty() const BK_NOEXCEPT { return rects_.empty(); }
    size_t size()  const BK_NOEXCEPT { return rects_.size(); }

    int px(int dx, int dy) const BK_NOEXCEPT {
        BK_ASSERT(!empty());
        auto const r = rects_.begin()->base;

        if (dx < 0)      return r.left();
        else if (dx > 0) return r.right();
        else             return r.left() + r.width() / 2;
    }

    int py(int dx, int dy) const BK_NOEXCEPT {
        BK_ASSERT(!empty());
        auto const r = rects_.begin()->base;

        if (dy < 0)      return r.top();
        else if (dy > 0) return r.bottom();
        else             return r.top() + r.width() / 2;

    }
    ////////////////////////////////////////////////////////////////////////////
    // lifetime
    ////////////////////////////////////////////////////////////////////////////
    explicit room_rect_set(rect_t rect) {
        add(rect);
    }
    ////////////////////////////////////////////////////////////////////////////
    // operations
    ////////////////////////////////////////////////////////////////////////////

    //--------------------------------------------------------------------------
    //! Geometrically add a rectangle to the set.
    //--------------------------------------------------------------------------
    void add(rect_t rect) {
        rects_.emplace_back(rect);
    }

    //--------------------------------------------------------------------------
    //! Geometrically subtract a rectangle to the set.
    //! @param where
    //!     An iterator to the existing rectangle to add a "hole" to.
    //! @param hole
    //!     The rectangle to subtract.
    //! @pre @p where must be dereferenceable.
    //! @pre @p hole must be strictly smaller than the rectangle at @p where.
    //! @pre @p hole must strictly intersect the rectangle at @p where.
    //--------------------------------------------------------------------------
    void subtract(iterator where, rect_t hole) {
        //must be a valid position
        BK_ASSERT(where != end());
        auto& value = *where;

        //hole must be strictly smaller in width and height
        BK_ASSERT(hole.width()  < value.base.width());
        BK_ASSERT(hole.height() < value.base.height());

        //hole must be entirely contained within base
        BK_ASSERT(bklib::intersection_of(value.base, hole).result == hole);

        value.has_hole = true;
        value.hole     = hole;
    }

    //--------------------------------------------------------------------------
    //! Intersection tests.
    //--------------------------------------------------------------------------
    bool intersects(rect_t const& r) const {
        using namespace std::placeholders;
        return std::any_of(begin(), end(), std::bind(intersects_<rect_t>, _1, std::cref(r)));
    }
    //--------------------------------------------------------------------------
    bool intersects(point_t const& p) const {
        using namespace std::placeholders;
        return std::any_of(begin(), end(), std::bind(intersects_<point_t>, _1, std::cref(p)));
    }
    //--------------------------------------------------------------------------
    bool intersects(room_rect_set const& other) const {
        auto const end1 = end();
        auto const end2 = other.end();

        for (auto it1 = begin(); it1 != end1; ++it1) {
            for (auto it2 = other.begin(); it2 != end2; ++it2) {
                if (intersects_(*it1, *it2)) {
                    return true;
                }
            }
        }

        return false;
    }

    //--------------------------------------------------------------------------
    //! Translate by the vector (@p dx, @p dy).
    //--------------------------------------------------------------------------
    void translate(int dx, int dy) {
        for (auto& value : rects_) {
            value.base.translate(dx, dy);
            if (value.has_hole) {
                value.hole.translate(dx, dy);
            }
        }
    }
public:
    static room_rect_set merge(room_rect_set&& a, room_rect_set&& b) {
        auto const size_a = a.rects_.capacity();
        auto const size_b = b.rects_.capacity();

        auto& out = size_a > size_b ? a : b;
        auto& in  = size_a > size_b ? b : a;

        std::move(in.begin(), in.end(), std::back_inserter(out.rects_));
        in.rects_.clear();

        room_rect_set result = std::move(out);

        return result;
    }
private:
    template <typename T>
    static bool intersects_(value_t const& a, T const& b);

    template <>
    static inline bool intersects_<value_t>(value_t const& a, value_t const& b) {
        return bklib::intersects(a.base, b.base);
    }

    template <>
    static inline bool intersects_<rect_t>(value_t const& a, rect_t const& b) {
        return bklib::intersects(a.base, b)
            && a.has_hole
            && !bklib::intersects(a.hole, b)
        ;
    }

    template <>
    static inline bool intersects_<point_t>(value_t const& a, point_t const& b) {
        return bklib::intersects(a.base, b)
            && a.has_hole
            && !bklib::intersects(a.hole, b)
        ;
    }

    container_t rects_;
};

class grid_layout {
public:
    struct params_t {
        //! grid cell size
        int cell_size = 10;
        
        //! min size for rectangles generated; must be >= 3.
        int rect_min_size = 3;
        //! max size for rectangles generated; must be < cell_size.
        int rect_max_size = cell_size - 1;

        //! mean size of generated rectangles.
        float rect_size_mean   = 5.0f;
        //! stddev for the size of generated rectangles.
        float rect_size_stddev = 3.0f;

        //! mean number of generated rectangles per "room".
        float rects_per_cell_mean   = 1.0f;
        //! stddev for the number of generated rectangles per "room".
        float rects_per_cell_stddev = 1.0f;

        //! probability that a given rect will have a "hole" in it.
        float hole_probability = 0.25f;

        //! width of the play field.
        int field_w = 100;
        //! height of the play field.
        int field_h = 100;

        int cells_w = field_w / cell_size;
        int cells_h = field_h / cell_size;

        bool validate() {
            static int const MIN_RECT_SIZE = 3;

            if (cell_size < MIN_RECT_SIZE + 1) {
                cell_size = MIN_RECT_SIZE + 1;
            }

            if (rect_min_size < MIN_RECT_SIZE) {
                rect_min_size = MIN_RECT_SIZE;
            }

            if (rect_max_size > cell_size - 1) {
                rect_max_size = cell_size - 1;
            }

            if (rect_max_size > cell_size - 1) {
                rect_min_size = cell_size - 1;
            }
        }
    };
    //--------------------------------------------------------------------------
    using rect         = bklib::axis_aligned_rect<int>;
    using cell_t       = std::vector<room_rect_set>;
    using random_t     = tez::random_t;
    using dist_normal  = std::normal_distribution<float>;
    using dist_uniform = std::uniform_int_distribution<int>;
    //--------------------------------------------------------------------------

    //--------------------------------------------------------------------------
    //
    //--------------------------------------------------------------------------
    std::vector<room_rect_set> operator()(random_t& random) {
        return (*this)(params_t{}, random);
    }

    //--------------------------------------------------------------------------
    // using params, generate a random layout.
    //--------------------------------------------------------------------------
    std::vector<room_rect_set> operator()(
        params_t const params, random_t& random
    ) {
        params_ = params;

        make_generators_();

        size_t const cell_count = params_.cells_h * params_.cells_w;

        cells_.clear();
        cells_.resize(cell_count);

        //----------------------------------------------------------------------
        // for each cell, generate a random number of rooms and merge.
        //----------------------------------------------------------------------
        auto hole_gen = std::uniform_real_distribution<float>{};

        for_each_i(cells_, [&](cell_t& cell, size_t const i) {
            auto const cell_rect = get_cell_rect_(i);

            auto const count = count_gen_(random);
            for (auto n = 0; n < count; ++n) {
                auto const room_rect = generate_rect_(cell_rect, random);
                cell.emplace_back(room_rect);

                if ( room_rect.width() >= 5 && room_rect.height() >= 5 &&
                    hole_gen(random) <= params_.hole_probability
                ) {
                    auto const hole_rect = generate_hole_(room_rect, random);
                    auto& room = cell.back();
                    room.subtract(room.begin(), hole_rect);
                }
            }

            merge_cell_rects_(cell);
        });

        shift_cell_rects_(random);

        //----------------------------------------------------------------------
        // merge the contents of all the cells into one vector.
        //----------------------------------------------------------------------
        std::vector<room_rect_set> result;
        result.reserve(cell_count);

        for (auto& cell : cells_) {
            for (auto& rect : cell) {
                if (!rect.empty()) {
                    result.emplace_back(rect);
                }
            }
        }

        return result;
    }
private:
    //--------------------------------------------------------------------------
    params_t params_;

    std::function<int (random_t&)> size_gen_;
    std::function<int (random_t&)> count_gen_;

    std::vector<cell_t> cells_;
    //--------------------------------------------------------------------------

    void make_generators_() {
        auto& p = params_;

        dist_normal size_dist  {p.rect_size_mean,      p.rect_size_stddev};
        dist_normal count_dist {p.rects_per_cell_mean, p.rects_per_cell_stddev};

        size_gen_ = [=](random_t& random) mutable -> int {
            auto const value   = size_dist(random);
            auto const rounded = static_cast<int>(std::round(value));
            return clamp(rounded, p.rect_min_size, p.rect_max_size);
        };

        count_gen_ = [=](random_t& random) mutable -> int {
            auto const value   = count_dist(random);
            auto const rounded = static_cast<int>(std::round(value));
            return rounded < 0 ? 0 : rounded;
        };
    }

    rect generate_hole_(rect const& base_rect, random_t& random) const {
        auto& p = params_;

        auto const width  = base_rect.width();
        auto const height = base_rect.height();

        auto const w_max = width  - 4;
        auto const h_max = height - 4;

        auto const w = dist_uniform{1, w_max}(random);
        auto const h = dist_uniform{1, h_max}(random);

        auto const x_max = width  - w - 4;
        auto const y_max = height - h - 4;

        auto const dx = x_max < 1 ? 0 : dist_uniform(0, x_max)(random);
        auto const dy = y_max < 1 ? 0 : dist_uniform(0, y_max)(random);

        auto const x0 = base_rect.left() + dx + 2;
        auto const y0 = base_rect.top()  + dy + 2;
        auto const x1 = x0 + w;
        auto const y1 = y0 + h;

        return rect {x0, y0, x1, y1};
    }

    //--------------------------------------------------------------------------
    // generate a rectangle that fits inside cell_rect.
    //--------------------------------------------------------------------------
    rect generate_rect_(rect const& cell_rect, random_t& random) const {
        auto& p = params_;

        auto const w = size_gen_(random);
        auto const h = size_gen_(random);

        auto const dx = dist_uniform(0, p.cell_size - w - 1)(random);
        auto const dy = dist_uniform(0, p.cell_size - h - 1)(random);

        auto const x0 = cell_rect.left() + dx;
        auto const y0 = cell_rect.top()  + dy;
        auto const x1 = x0 + w;
        auto const y1 = y0 + h;

        return rect {x0, y0, x1, y1};
    }

    //--------------------------------------------------------------------------
    // get the rectangle corresponding to the cell with index i.
    //--------------------------------------------------------------------------
    rect get_cell_rect_(size_t const i) const {
        auto const sz = params_.cell_size;
        auto const w  = params_.cells_w;
        auto const h  = params_.cells_h;

        auto const div = std::div(i, w);

        auto const xi = div.rem;
        auto const yi = div.quot;

        auto const x0 = xi * sz;
        auto const y0 = yi * sz;
        auto const x1 = x0 + sz;
        auto const y1 = y0 + sz;

        return {x0, y0, x1, y1};
    }

    //--------------------------------------------------------------------------
    // for all cells, merge all intersecting rectangles into rectangle unions.
    //--------------------------------------------------------------------------
    void merge_cell_rects_(cell_t& cell) {
        auto const n = cell.size();

        //for all pairs
        for (size_t i = 0 ; (n != 0) && (i < n - 1); ++i) {
            for (size_t j = i + 1 ; j < n; ++j) {
                auto& u0 = cell[i];
                auto& u1 = cell[j];

                if (u0.intersects(u1)) {
                    u1 = room_rect_set::merge(std::move(u0), std::move(u1));
                    break;
                }
            }
        }

        auto const is_empty = [](room_rect_set const& u) {
            return u.empty();
        };

        cell.erase(
            std::remove_if(std::begin(cell), std::end(cell), is_empty)
            , std::end(cell)
        );
    }

    //--------------------------------------------------------------------------
    // once, for all empty cells, shift a neighboring cell's contents toward the
    // the empty cell by a random amount.
    //--------------------------------------------------------------------------
    void shift_cell_rects_(random_t& random) {
        tez::flat_set<size_t> used;

        //for each cell
        for_each_i(cells_, [&](cell_t& cell, size_t const cell_index) {
            //only empty cells
            if (!cell.empty()) {
                return;
            }

            auto const here = static_cast<int>(cell_index);
            auto const w    = params_.cells_w;

            //indicies of cardinal neighbors
            int const neighbor_indicies[4] {
                here % w != 0     ? here - 1 : -1 //mind the edges
              , here % w != w - 1 ? here + 1 : -1 //mind the edges
              , here - w
              , here + w
            };

            //start from a random neighbor
            size_t const start = dist_uniform{0, 3}(random);

            //for each neighbor...
            for (size_t i = 0; i < 4; ++i) {
                auto const neighbor = (start + i) % 4;
                auto const j = neighbor_indicies[neighbor];

                if (j < 0 || j >= cells_.size()) {
                    //bad index; try next
                    continue;
                } else if (used.find(j) != std::cend(used)) {
                    //already used index; try next
                    continue;
                }

                auto& target = cells_[j];

                if (target.empty()) {
                    //empty neighbor; try next
                    continue;
                }

                //ok; mark neighbor as used
                used.insert(j);

                int const delta_min = 1;
                int const delta_max = params_.cell_size - 1;
                int const delta = dist_uniform {delta_min, delta_max}(random);

                //for all rect unions...
                for (auto& u : target) {
                    switch (neighbor) {
                    case 0 : u.translate( delta,  0);     break;
                    case 1 : u.translate(-delta,  0);     break;
                    case 2 : u.translate(0,       delta); break;
                    case 3 : u.translate(0,      -delta); break;
                    }
                }

                //done this cell
                break;
            }
        });
    }
};

struct tile_data {
    enum class tile_type : uint16_t {
        invalid, empty, corridor, floor, wall,
    };

    using room_id_t = uint16_t;
    using tile_sub_type = uint16_t;

    static room_id_t const ROOM_ID_NONE = 0;

    tile_type     type     = tile_type::empty;
    tile_sub_type sub_type = 0;
    room_id_t     room_id  = ROOM_ID_NONE;
};

struct tile_grid {
    using element_t = tile_data;
    using rect_t = bklib::axis_aligned_rect<int>;

    tile_grid(size_t width, size_t height, element_t value = element_t{})
      : width_{width}
      , height_{height}
    {
        tiles_.resize(width*height, value);
    }

    element_t& at(size_t x, size_t y) {
        BK_ASSERT(x < width_ && y < height_);
        auto const i = y * width_ + x;
        return tiles_[i];
    }

    element_t const& at(size_t x, size_t y) const {
        return const_cast<tile_grid*>(this)->at(x, y);
    }

    void fill_rect(rect_t const rect, element_t value) {
        BK_ASSERT(rect.left()  >= 0 && rect.top()    >= 0);
        BK_ASSERT(rect.right() >= 0 && rect.bottom() >= 0);

        for (auto y = rect.top(); y < rect.bottom(); ++y) {
            for (auto x = rect.left(); x < rect.right(); ++x) {
                at(x, y) = value;
            }
        }
    }

    template <typename Function>
    inline void for_each_xy(Function function) const {
        for (size_t y = 0; y < height_; ++y) {
            for (size_t x = 0; x < width_; ++x) {
                function(x, y, at(x, y));
            }
        }
    }

    inline bool is_valid_index(int const x, int const y) const BK_NOEXCEPT {
        return x >= 0
            && static_cast<size_t>(x) < width_
            && y >= 0
            && static_cast<size_t>(y) < height_
        ;
    }

    using block = std::array<
        std::array<element_t*, 3>, 3
    >;

    block block_at(int const x, int const y) {
        block b;

        for (auto const yi : {-1, 0, 1}) {
            for (auto const xi : {-1, 0, 1}) {
                auto const xx = x + xi;
                auto const yy = y + yi;

                b[yi + 1][xi + 1] = is_valid_index(xx, yy) ? &at(xx, yy) : &at(x, y);
            }
        }

        return b;
    }


    template <typename Function>
    void for_each_neighbor(int const x, int const y, Function function) {
        for (auto const yi : {-1, 0, 1}) {
            for (auto const xi : {-1, 0, 1}) {
                if ((xi | yi) == 0 || !is_valid_index(x + xi, y + yi)) continue;
                function(xi, yi, at(x + xi, y + yi));
            }
        }
    }

    size_t width()  const BK_NOEXCEPT { return width_; }
    size_t height() const BK_NOEXCEPT { return height_; }

    size_t width_;
    size_t height_;

    std::vector<element_t> tiles_;
};

struct directed_walk {
    using random_t = tez::random_t;

    bool rule(tile_grid::block const& b) const {
        //auto const is_floor = [](tile_data const* data) {
        //    return data->type == tile_data::tile_type::floor ? 1 : 0;
        //};

        //auto const is_corridor = [](tile_data const* data) {
        //    return data->type == tile_data::tile_type::corridor ? 1 : 0;
        //};

        //int const floor_ns =
        //    is_floor(b[0][0]) + is_floor(b[0][1]) + is_floor(b[0][2])
        //  + is_floor(b[2][0]) + is_floor(b[2][1]) + is_floor(b[2][2]);

        //int const floor_ew =
        //    is_floor(b[0][0]) + is_floor(b[0][2])
        //  + is_floor(b[1][0]) + is_floor(b[1][2])
        //  + is_floor(b[2][0]) + is_floor(b[2][2]);

        //int const corridor_ns = is_corridor(b[0][1]) + is_corridor(b[2][1]);
        //int const corridor_ew = is_corridor(b[1][0]) + is_corridor(b[1][2]);

        //return (floor_ns == 0 && floor_ew == 0)
        //    || (floor_ns == 3 && floor_ew == 2 && corridor_ew == 0)
        //    || (floor_ns == 6 && floor_ew == 4 && corridor_ew == 0)
        //    || (floor_ns == 2 && floor_ew == 3 && corridor_ns == 0)
        //    || (floor_ns == 4 && floor_ew == 6 && corridor_ns == 0)
        //;

        auto const is_floor = [](tile_data const* data) {
            return data->type == tile_data::tile_type::floor ? 1 : 0;
        };

        auto const is_corridor = [](tile_data const* data) {
            return data->type == tile_data::tile_type::corridor;
        };

        int const floor_n = is_floor(b[0][0]) + is_floor(b[0][1]) + is_floor(b[0][2]);
        int const floor_s = is_floor(b[2][0]) + is_floor(b[2][1]) + is_floor(b[2][2]);
        int const floor_e = is_floor(b[0][2]) + is_floor(b[1][2]) + is_floor(b[2][2]);
        int const floor_w = is_floor(b[0][0]) + is_floor(b[1][0]) + is_floor(b[2][0]);

        bool const corridor_ew = is_corridor(b[1][0]) || is_corridor(b[1][2]);
        bool const corridor_ns = is_corridor(b[0][1]) || is_corridor(b[2][1]);

        if (floor_n == 0 && floor_s == 0 && floor_e == 0 && floor_w == 0) {
            return true;
        } else if (!corridor_ew && floor_e != 3 && floor_e == floor_w) {
            return (floor_n == 3 && floor_s == 0)
                || (floor_n == 0 && floor_s == 3)
                || (floor_n == 3 && floor_s == 3);
        } else if (!corridor_ns && floor_n != 3 && floor_n == floor_s) {
            return (floor_e == 3 && floor_w == 0)
                || (floor_e == 0 && floor_w == 3)
                || (floor_e == 3 && floor_w == 3);
        } else {
            return false;
        }
    }

    boost::container::flat_set<int> operator()(
        random_t& random, tile_grid& grid
      , int const start_room
      , int const start_x,  int const start_y
      , int const dir_x, int const dir_y
    ) {
        BK_ASSERT(std::abs(dir_x) <= 1);
        BK_ASSERT(std::abs(dir_y) <= 1);

        static float const forward  = 80;
        static float const left     = 20;
        static float const right    = 20;
        static float const backward = 5;

        static float const segment_length_mean   = 5.0f;
        static float const segment_length_stddev = 3.0f;

        std::discrete_distribution<> direction_gen {
            forward, left, right, backward
        };

        std::normal_distribution<float> length_gen {segment_length_mean, segment_length_stddev};

        auto x = start_x;
        auto y = start_y;

        boost::container::flat_set<int> connections;
        connections.reserve(10);
        connections.insert(start_room);

        for (int iterations = 0; iterations < 10; ++iterations) {
            auto const dir    = direction_gen(random);
            auto const length = static_cast<int>(std::round(length_gen(random)));

            auto dx = dir_x;
            auto dy = dir_y;

            switch (dir) {
            case 0 : break;
            case 1 : std::tie(dx, dy) = std::make_pair(dy,  dx); break;
            case 2 : std::tie(dx, dy) = std::make_pair(-dy, -dx); break;
            case 3 : std::tie(dx, dy) = std::make_pair(-dx, -dy); break;
            }

            for (int i = 0; i < length; ++i) {
                if (!grid.is_valid_index(x, y)) {
                    break; //try another direction TODO
                }

                auto&      data = grid.at(x, y);
                auto const type = data.type;

                if (type == tile_data::tile_type::empty) {
                    auto const block = grid.block_at(x, y);

                    if (rule(block)) {
                        data.type = tile_data::tile_type::corridor;
                        data.room_id = start_room;
                    } else {
                        break; //try another direction TODO
                    }
                } else if (type == tile_data::tile_type::corridor) {
                    connections.insert(data.room_id);
                } else if (type == tile_data::tile_type::floor) {
                    if (data.room_id != start_room) {
                        connections.insert(data.room_id);
                        return connections;
                    }
                }

                x += dx;
                y += dy;
            }
        }

        return connections;
    }
};

struct level {
    using random_t = tez::random_t;

    explicit level(random_t& random)
      : grid_ {100, 100}
    {
        generate(random);
    }

    void generate(random_t& random) {
        room_defs_ = grid_layout{}(random);

        tile_data tile_floor;
        tile_floor.type = tile_data::tile_type::floor;

        tile_data tile_hole;
        tile_hole.type = tile_data::tile_type::empty;

        for (size_t i = 0; i < room_defs_.size(); ++i) {
            auto const& cell = room_defs_[i];
            tile_floor.room_id = i + 1;

            for (auto const& rect : cell) {
                grid_.fill_rect(rect.base, tile_floor);
            }

            for (auto const& rect : cell) {
                if (rect.has_hole) {
                    grid_.fill_rect(rect.hole, tile_hole);
                }
            }
        }

        static int const vx[] {0,  0, 1, -1};
        static int const vy[] {1, -1, 0,  0};
        std::uniform_int_distribution<> dir_gen {0, 3};

        for (size_t i = 0; i < room_defs_.size(); ++i) {
            auto const& cell = room_defs_[i];

            auto const start_dir = dir_gen(random);

            for (int j = 0; j < 4; ++j) {
                auto const dir = (start_dir + j) % 4;

                auto const dx = vx[dir];
                auto const dy = vy[dir];

                auto result = directed_walk {}(
                    random, grid_, i+1
                  , cell.px(dx, dy), cell.py(dx, dy)
                  , dx, dy
                );

                if (result.size() > 1) {
                    break;
                }
            }
        }
    }

    void draw(bklib::renderer2d& renderer) {
        using rect_t  = bklib::renderer2d::rect;
        using color_t = bklib::renderer2d::color;

        static float const TILE_SIZE = 32.0f;

        grid_.for_each_xy([&](size_t x, size_t y, tile_data const& data) {
            auto const x0 = x * TILE_SIZE;
            auto const y0 = y * TILE_SIZE;
            auto const x1 = x0 + TILE_SIZE;
            auto const y1 = y0 + TILE_SIZE;

            rect_t const r {x0, y0, x1, y1};

            if (data.type == tile_data::tile_type::empty) {
                renderer.set_color_brush(color_t{{1.0f, 0.0f, 0.0f}});
            } else if (data.type == tile_data::tile_type::corridor) {
                renderer.set_color_brush(color_t{{0.2f, 0.2f, 0.5f}});
            } else {
                //auto const color_val = data.room_id * 123;
                //float const r = ((color_val & 0x0000FF) >> 0)  / 255.0f;
                //float const g = ((color_val & 0x00FF00) >> 8)  / 255.0f;
                //float const b = ((color_val & 0xFF0000) >> 16) / 255.0f;

                renderer.set_color_brush(color_t{{0.0f, 0.5f, 0.0f}});
            }

            renderer.fill_rect(r);
        });
    }

    std::vector<room_rect_set> room_defs_;
    tile_grid                  grid_;
};

////

class game_main {
public:
    game_main()
      : random_{10}
      , window_     {L"tez"}
      , renderer2d_ {window_.get_handle()}
      , scale_{1.0f}
      , translate_{1.0f}
      , level_ {random_}
    {
        using namespace std::placeholders;
        using std::bind;

        window_.listen(bklib::on_resize{
            bind(&game_main::on_resize, this, _1, _2)
        });

        window_.listen(bklib::on_mouse_move{
            bind(&game_main::on_mouse_move, this, _1, _2, _3)
        });

        window_.listen(bklib::on_mouse_move_to{
            bind(&game_main::on_mouse_move_to, this, _1, _2, _3)
        });

        window_.listen(bklib::on_mouse_down{
            bind(&game_main::on_mouse_down, this, _1, _2)
        });

        window_.listen(bklib::on_mouse_wheel_v{
            bind(&game_main::on_mouse_wheel_v, this, _1, _2)
        });

        window_.listen(bklib::on_mouse_up{
            bind(&game_main::on_mouse_up, this, _1, _2)
        });

        window_.listen(bklib::on_keydown{
            bind(&game_main::on_keydown, this, _1, _2)
        });
        window_.listen(bklib::on_keyup{
            bind(&game_main::on_keyup, this, _1, _2)
        });
        window_.listen(bklib::on_keyrepeat{
            bind(&game_main::on_keyrepeat, this, _1, _2)
        });
    }

    void render() {
        glm::mat3 m {1.0f};
        m = translate_*scale_*m;

        renderer2d_.begin();
        renderer2d_.set_transform(m);
        renderer2d_.clear();

        level_.draw(renderer2d_);

        //auto const c1 = bklib::renderer2d::color {{1.0f, 1.0f, 1.0f}};
        //auto const c2 = bklib::renderer2d::color {{1.0f, 0.0f, 1.0f}};

        //renderer2d_.set_color_brush(c1);

        //for (size_t i = 0; i < rects_.size(); ++i) {
        //    auto const& u = rects_[i];

        //    if (i != index_) {
        //        renderer2d_.set_color_brush(c1);
        //    } else {
        //        renderer2d_.set_color_brush(c2);
        //    }

        //    for (auto const& rect : u) {
        //         auto const r = bklib::renderer2d::rect(
        //            rect.left() * 32
        //          , rect.top() * 32
        //          , rect.right() * 32
        //          , rect.bottom() * 32
        //        );

        //        renderer2d_.fill_rect(r);
        //    }
        //}

        renderer2d_.set_color_brush(bklib::renderer2d::color {{0.0f, 0.0f, 0.0f}});

        for (float y = 0.0f; y < 100.0f; y += 10.0f) {
            for (float x = 0.0f; x < 100.0f; x += 10.0f) {
                bklib::renderer2d::rect const r = {x*32, y*32, (x + 10) *32, (y + 10) *32};
                renderer2d_.draw_rect(r, 1.0f / scale_[0][0]);
            }
        }

        //for (auto const& cell : map_) {
        //    if (cell.value.type == tez::tile_type::empty) continue;

        //    auto const ix = cell.i.x;
        //    auto const iy = cell.i.y;

        //    bklib::renderer2d::rect const r = {
        //        ix * 32.0f
        //      , iy * 32.0f
        //      , ix * 32.0f + 32.0f
        //      , iy * 32.0f + 32.0f
        //    };

        //    renderer2d_.fill_rect(r);
        //}

        renderer2d_.end();
    }

    int run() {
        while (window_.is_running()) {
            window_.do_events();
            render();
        }

        return window_.get_result().get() ? 0 : -1;
    }

    void on_resize(unsigned w, unsigned h) {
        renderer2d_.resize(w, h);
    }

    void on_mouse_move(bklib::mouse& mouse, int dx, int dy) {
    }

    void on_mouse_move_to(bklib::mouse& mouse, int x, int y) {
        auto const& left_button = mouse.button(0);

        if (left_button) {
            auto const time = std::chrono::duration_cast<std::chrono::milliseconds>(bklib::clock_t::now() - left_button.time);

            if (time > std::chrono::milliseconds{100}) {
                auto const& last = mouse.absolute(1);

                auto const dx = x - last.x;
                auto const dy = y - last.y;

                translate_[2].x += dx;
                translate_[2].y += dy;
            }
        }
    }

    void on_mouse_down(bklib::mouse& mouse, unsigned button) {
        glm::mat3 m {1.0f};

        auto inv_trans = translate_;
        auto inv_scale = scale_;

        inv_trans[2].x = -inv_trans[2].x;
        inv_trans[2].y = -inv_trans[2].y;

        inv_scale[0][0] = 1.0f / inv_scale[0][0];
        inv_scale[1][1] = 1.0f / inv_scale[1][1];

        m = inv_scale*inv_trans*m;

        glm::vec3 v {
            mouse.absolute().x
          , mouse.absolute().y
          , 1.0f
        };

        v = m*v;

        bklib::point2d<int> const  p {v.x / 32, v.y / 32};

        std::cout << "x: " << p.x << "y: " << p.y << std::endl;

        //index_ = -1;
        //for (size_t i = 0; i < rects_.size(); ++i) {
        //    if (rects_[i].intersects(p)) {
        //        index_ = i;
        //        break;
        //    }
        //}
    }

    void on_mouse_up(bklib::mouse& mouse, unsigned button) {
    }

    void on_mouse_wheel_v(bklib::mouse& mouse, int delta) {
        auto scale = scale_[0][0];
        scale *= delta < 0 ? 0.9f : 1.1f;

        scale_[0][0] = scale;
        scale_[1][1] = scale;
    }

    void on_keydown(bklib::keyboard& keyboard, bklib::keycode key) {
        auto const combo   = keyboard.state();
        auto const command = bindings_.match(combo);

        switch (command) {
        case tez::command_t::USE :
            level_.generate(random_); break;
        case tez::command_t::DIR_NORTH :
            translate_[2].y -= 5.0f; break;
        case tez::command_t::DIR_SOUTH :
            translate_[2].y += 5.0f; break;
        case tez::command_t::DIR_EAST :
            translate_[2].x += 5.0f; break;
        case tez::command_t::DIR_WEST :
            translate_[2].x -= 5.0f; break;
        }

    }

    void on_keyup(bklib::keyboard& keyboard, bklib::keycode key) {
    }

    void on_keyrepeat(bklib::keyboard& keyboard, bklib::keycode key) {
        auto const combo = keyboard.state();

        for (
            auto it = bindings_.match_subset_begin(combo),
                end = bindings_.match_subset_end(combo)
          ; it != end
          ; ++it
        ) {
            auto const command = *it;

            switch (command) {
            case tez::command_t::DIR_NORTH :
                translate_[2].y -= 5.0f; break;
            case tez::command_t::DIR_SOUTH :
                translate_[2].y += 5.0f; break;
            case tez::command_t::DIR_EAST :
                translate_[2].x += 5.0f; break;
            case tez::command_t::DIR_WEST :
                translate_[2].x -= 5.0f; break;
            }
        }
    }

private:
    tez::random_t random_;

    bklib::platform_window window_;
    bklib::renderer2d      renderer2d_;
    tez::grid2d<tez::tile_data> map_;

    tez::key_bindings bindings_;

    glm::mat3 scale_;
    glm::mat3 translate_;

    level level_;

    int index_ = -1;
};

int main(int argc, char const* argv[]) try {
    game_main game;
 
    return game.run();
} catch (bklib::exception_base const&) {
    BK_DEBUG_BREAK(); //TODO
} catch (std::exception const&) {
    BK_DEBUG_BREAK(); //TODO
} catch (...) {
    BK_DEBUG_BREAK(); //TODO
}
