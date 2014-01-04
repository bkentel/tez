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

//template <typename T>
//struct rect_union {
//    using rect = bklib::axis_aligned_rect<T>;
//    using container_t = std::vector<rect>;
//    using iterator = typename container_t::iterator;
//    using const_iterator = typename container_t::const_iterator;
//
//    rect_union() = default;
//
//    explicit rect_union(rect r) {
//        add(r);
//    }
//
//    const_iterator begin() const { return rects.begin(); }
//    const_iterator end()   const { return rects.end(); }
//
//    iterator begin() { return rects.begin(); }
//    iterator end()   { return rects.end(); }
//
//    template <typename U>
//    bool intersects(U const& other) const {
//        return std::any_of(std::cbegin(rects), std::cend(rects), [&](rect const& r) {
//            return bklib::intersects(other, r);
//        });
//    }
//    
//    bool intersects(rect_union const& other) const {
//        return std::any_of(std::cbegin(other), std::cend(other), [&](rect const& r) {
//            return intersects(r);
//        });
//    }
//
//    bool empty() const { return rects.empty(); }
//
//    size_t size() const { return rects.size(); }
//
//    void add(rect const& r) {
//        rects.push_back(r);
//    }
//
//    rect bounds() const {
//        BK_ASSERT(!empty());
//
//        auto x0 = std::numeric_limits<T>::max();
//        auto y0 = std::numeric_limits<T>::max();
//        auto x1 = std::numeric_limits<T>::min();
//        auto y1 = std::numeric_limits<T>::min();
//
//        for (auto const& r : rects) {
//            x0 = std::min(x0, r.left());
//            y0 = std::min(y0, r.top());
//            x1 = std::min(x1, r.right());
//            y1 = std::min(y1, r.bottom());
//        }
//
//        return {x0, y0, x1, y1};
//    }
//
//    void move(T dx, T dy) {
//        for (auto& rect : rects) {
//            rect.move(dx, dy);
//        }
//    }
//
//    static rect_union merge(rect_union&& a, rect_union&& b) {
//        rect_union& from = a.rects.capacity() <   b.rects.capacity() ? a : b;
//        rect_union& to   = a.rects.capacity() >=  b.rects.capacity() ? a : b;
//
//        to.rects.reserve(from.size() + to.size());
//
//        for (auto const& r : from) {
//            to.add(r);
//        }
//        
//        from.rects.resize(0);
//        rect_union result = std::move(to);
//
//        return result;
//    }
//
//    container_t rects;
//};

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

class grid_layout {
public:
    struct params_t {
        int cell_size = 10;

        int rect_min_size = 3;
        int rect_max_size = 9;

        float rect_size_mean   = 5.0f;
        float rect_size_stddev = 3.0f;

        float rects_per_cell_mean   = 1.0f;
        float rects_per_cell_stddev = 1.0f;

        float hole_probability = 0.25f;

        int field_w = 100;
        int field_h = 100;

        int cells_w = field_w / cell_size;
        int cells_h = field_h / cell_size;
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

                if (hole_gen(random) <= params_.hole_probability) {
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

        auto const w = dist_uniform{1, width  - 2}(random);
        auto const h = dist_uniform{1, height - 2}(random);

        auto const x_max = width  - w - 2;
        auto const y_max = height - h - 2;

        auto const dx = x_max <= 1 ? 1 : dist_uniform(1, x_max)(random);
        auto const dy = y_max <= 1 ? 1 : dist_uniform(1, y_max)(random);

        auto const x0 = base_rect.left() + dx;
        auto const y0 = base_rect.top()  + dy;
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
        invalid, empty, floor, wall,
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

    size_t width_;
    size_t height_;

    std::vector<element_t> tiles_;
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
            tile_floor.room_id = i + 1;

            for (auto const& rect : room_defs_[i]) {
                grid_.fill_rect(rect.base, tile_floor);
            }

            for (auto const& rect : room_defs_[i]) {
                if (rect.has_hole) {
                    grid_.fill_rect(rect.hole, tile_hole);
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
