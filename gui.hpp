#pragma once

#include <bklib/types.hpp>
#include <bklib/util.hpp>
#include <bklib/math.hpp>
#include <bklib/callback.hpp>

#include <bklib/window.hpp>

#include <bklib/renderer2d.hpp>

namespace tez {
namespace gui {

using scalar       = int;
using point        = bklib::point2d<scalar>;
using bounding_box = bklib::axis_aligned_rect<scalar>;
using renderer     = bklib::renderer2d;
using color        = renderer::color;
using color_alpha  = renderer::color_alpha;

class widget_base;
class container_widget;
class canvas;

using on_mouse_move    = bklib::on_mouse_move;
using on_mouse_move_to = bklib::on_mouse_move_to;
using on_mouse_enter   = bklib::on_mouse_enter;
using on_mouse_exit    = bklib::on_mouse_exit;
using on_mouse_down    = bklib::on_mouse_down;
using on_mouse_up      = bklib::on_mouse_up;

template <typename Container, typename Predicate>
inline auto find_if(Container&& container, Predicate&& predicate) {
    auto const where = std::find_if(
        std::begin(container)
      , std::end(container)
      , std::forward<Predicate>(predicate)
    );

    return where;
}

//==============================================================================
#define BK_GUI_DECLARE_LISTENER(event)\
public:\
    virtual void listen(::tez::gui::event callback) override { event##_ = callback; }\
protected:\
    ::tez::gui::event event##_

//==============================================================================
void  track_mouse_input(widget_base& widget, bool track = true);
point local_to_screen(widget_base const& widget, point p);

//==============================================================================

template <typename T>
inline void unused(T const&) {}

template <typename Head, typename... Tail>
inline void unused(Head const&, Tail const&... tail) {
    unused(tail...);
}

//==============================================================================
class widget_base {
public:
    widget_base() = delete;
    widget_base(widget_base const&) = delete;
    widget_base& operator=(widget_base const&) = delete;

    widget_base(widget_base&&) = default;
    widget_base& operator=(widget_base&&) = default;

    widget_base(bounding_box bounds, widget_base* parent, bklib::utf8string name)
      : bounds_ {bounds}
      , parent_ {parent}
      , name_   {std::move(name)}
      , hash_   {bklib::utf8string_hash(name_)}
    {
    }

    explicit widget_base(bklib::utf8string name)
      : widget_base(bounding_box{}, nullptr, std::move(name))
    {
    }

    virtual ~widget_base() = default;

    virtual void listen(gui::on_mouse_move_to callback) { unused(callback); }
    virtual void listen(gui::on_mouse_move    callback) { unused(callback); }
    virtual void listen(gui::on_mouse_enter   callback) { unused(callback); }
    virtual void listen(gui::on_mouse_exit    callback) { unused(callback); }
    virtual void listen(gui::on_mouse_down    callback) { unused(callback); }
    virtual void listen(gui::on_mouse_up      callback) { unused(callback); }

    virtual void on_mouse_move    (bklib::mouse& mouse, scalar dx, scalar dy) { unused(mouse, dx, dy); }
    virtual void on_mouse_move_to (bklib::mouse& mouse, scalar x,  scalar y)  { unused(mouse, x, y); }
    virtual void on_mouse_enter   (bklib::mouse& mouse, scalar x,  scalar y)  { unused(mouse, x, y); }
    virtual void on_mouse_exit    (bklib::mouse& mouse, scalar x,  scalar y)  { unused(mouse, x, y); }
    virtual void on_mouse_down    (bklib::mouse& mouse, scalar x,  scalar y, unsigned button) { unused(mouse, x, y, button); }
    virtual void on_mouse_up      (bklib::mouse& mouse, scalar x,  scalar y, unsigned button) { unused(mouse, x, y, button); }

    virtual void draw(renderer& r) const { unused(r); };

    virtual widget_base*      parent() const BK_NOEXCEPT { return parent_; }
    virtual bklib::string_ref name()   const BK_NOEXCEPT { return name_; }
    virtual bklib::hash_t     hash()   const BK_NOEXCEPT { return hash_; }
    virtual bounding_box      bounds() const BK_NOEXCEPT { return bounds_;}

    virtual widget_base* set_parent(widget_base* parent) BK_NOEXCEPT {
        using std::swap;
        swap(parent, parent_);
        return parent;
    }

    virtual void resize(bounding_box size) BK_NOEXCEPT { bounds_ = size; }
protected:
    bounding_box      bounds_;
    widget_base*      parent_;
    bklib::utf8string name_;
    bklib::hash_t     hash_;
};

//==============================================================================
class container_widget : public widget_base {
public:
    using element_t   = std::unique_ptr<widget_base>;
    using container_t = std::vector<element_t>;
    using iterator    = std::vector<element_t>::iterator;

    auto begin()         { return children_.begin(); }
    auto end()           { return children_.end(); }
    auto begin()   const { return children_.begin(); }
    auto end()     const { return children_.end(); }
    auto cbegin()  const { return children_.cbegin(); }
    auto cend()    const { return children_.cend(); }

    auto rbegin()        { return children_.begin(); }
    auto rend()          { return children_.end(); }
    auto rbegin()  const { return children_.begin(); }
    auto rend()    const { return children_.end(); }
    auto crbegin() const { return children_.crbegin(); }
    auto crend()   const { return children_.crend(); }

    using widget_base::widget_base;

    template <typename T>
    T* add_child(std::unique_ptr<T> child) {
        static_assert(std::is_base_of<widget_base, T>::value, "invalid type.");

        BK_ASSERT(!!child);
        T* const ptr = child.get();

        std::unique_ptr<widget_base> base = std::move(child);
        base->set_parent(this);

        children_.emplace_back(std::move(base));

        return ptr;
    }

    element_t remove_child(bklib::string_ref const name) {
        return remove_child(bklib::utf8string_hash(name));
    }

    element_t remove_child(bklib::hash_t const hash) {
        element_t result;

        auto const where = gui::find_if(children_, [hash](element_t const& child) {
            return child->hash() == hash;
        });

        //auto where = std::find_if(std::begin(children_), std::end(children_), [hash](element_t const& child) {
        //    return child->hash() == hash;
        //});

        if (where == std::end(children_)) {
            return result;
        }

        result = std::move(*where);
        children_.erase(where);
        return result;
    }

    widget_base* find_topmost_at(scalar x, scalar y) const {
        point const p {x, y};

        auto where = std::find_if(std::cbegin(children_), std::cend(children_), [&](element_t const& child) {
            return bklib::intersects(child->bounds(), p);
        });

        return (where != std::cend(children_)) ? where->get() : nullptr;
    }

    void move_to_top(iterator where) {
        using std::swap;

        while (where != std::begin(children_)) {
            element_t& a = *where;
            element_t& b = *where--;

            swap(a, b);
        }
    }

    virtual void draw(renderer& r) const override {
        std::for_each(std::crbegin(children_), std::crend(children_), [&](element_t const& child) {
            child->draw(r);
        });
    }

    virtual void on_mouse_move(bklib::mouse& mouse, scalar dx, scalar dy) override {
        auto const pos = mouse.absolute();
        point const p {pos.x, pos.y};

        for (auto& child : children_) {
            if (bklib::intersects(child->bounds(), p)) {
                child->on_mouse_move(mouse, dx, dy);
                break;
            }
        }
    }

    virtual void on_mouse_move_to(bklib::mouse& mouse, scalar x, scalar y) override {
        auto old = mouse.absolute(1);

        auto const cur_target = find_topmost_at(x, y);
        auto const old_target = find_topmost_at(old.x, old.y);

        if (cur_target && cur_target == old_target) {
            cur_target->on_mouse_move_to(mouse, x, y);
        } else {
            if (old_target && old_target != cur_target) {
                old_target->on_mouse_exit(mouse, x, y);
            }

            if (cur_target && cur_target != old_target) {
                cur_target->on_mouse_enter(mouse, x, y);
            }
        }
    }

    virtual void on_mouse_enter(bklib::mouse& mouse, scalar x, scalar y) override {
        on_mouse_move_to(mouse, x, y);
    }

    virtual void on_mouse_exit(bklib::mouse& mouse, scalar x, scalar y) override {
        on_mouse_move_to(mouse, x, y);
    }

    virtual void on_mouse_down(bklib::mouse& mouse, scalar x, scalar y, unsigned button) override {
        if (auto const target = find_topmost_at(x, y)) {
            target->on_mouse_down(mouse, x, y, button);
        }
 
    }

    virtual void on_mouse_up(bklib::mouse& mouse, scalar x, scalar y, unsigned button) override {
        if (auto const target = find_topmost_at(x, y)) {
            target->on_mouse_up(mouse, x, y, button);
        }
    }
private:
    container_t children_;
};
//==============================================================================
class root : public container_widget {
public:
    using container_widget::container_widget;

    virtual void on_mouse_move_to(bklib::mouse& mouse, scalar x, scalar y) {
        if (track_mouse_input_) {
            track_mouse_input_->on_mouse_move_to(mouse, x, y);
        } else {
            container_widget::on_mouse_move_to(mouse, x, y);
        }
    }

    virtual void on_mouse_move(bklib::mouse& mouse, scalar dx, scalar dy) {
        if (track_mouse_input_) {
            track_mouse_input_->on_mouse_move(mouse, dx, dy);
        } else {
            container_widget::on_mouse_move(mouse, dx, dy);
        }
    }

    virtual void on_mouse_down(bklib::mouse& mouse, scalar x, scalar y, unsigned button) {
        if (track_mouse_input_) {
            track_mouse_input_->on_mouse_down(mouse, x, y, button);
        } else {
            container_widget::on_mouse_down(mouse, x, y, button);
        }
    }

    virtual void on_mouse_up(bklib::mouse& mouse, scalar x, scalar y, unsigned button) {
        if (track_mouse_input_) {
            track_mouse_input_->on_mouse_up(mouse, x, y, button);
        } else {
            container_widget::on_mouse_up(mouse, x, y, button);
        }
    }

    void track_mouse_input(widget_base& widget, bool const track = true) {
        if (!track) {
            BK_ASSERT(&widget == track_mouse_input_);
            track_mouse_input_ = nullptr;
        } else {
            BK_ASSERT(track_mouse_input_ == nullptr);
            track_mouse_input_ = &widget;
        }
    }
private:
    widget_base* track_mouse_input_ = nullptr;
};

//==============================================================================
class sizing_frame {
public:
    using mouse = bklib::mouse;

    enum class state : unsigned {
        none         = 0
      , left         = 1 << 0
      , top          = 1 << 1
      , right        = 1 << 2
      , bottom       = 1 << 3
      , top_left     = top    | left
      , top_right    = top    | right
      , botttom_left = bottom | left
      , bottom_right = bottom | right
    };

    static BK_CONSTEXPR scalar const WIDTH = 8;

    explicit sizing_frame(widget_base& widget)
      : widget_ {widget}
      , state_  {state::none}
    {
    }

    bool is_sizing() const {
        return state_ != state::none;
    }

    bool state_contains(state const s) const {
        using type = std::underlying_type_t<state>;
        return (static_cast<type>(state_) & static_cast<type>(s)) != 0;
    }

    void on_mouse_down(mouse& m, scalar x, scalar y, unsigned button) {
        if (button != 0) return;

        BK_ASSERT(state_ == state::none);

 
        auto const& bounds = widget_.bounds();

        scalar const dl =  (x - bounds.left()   );
        scalar const dt =  (y - bounds.top()    );
        scalar const dr = -(x - bounds.right()  );
        scalar const db = -(y - bounds.bottom() );

        bool const is_l = dl >= 0 && dl < WIDTH;
        bool const is_t = dt >= 0 && dt < WIDTH;
        bool const is_r = dr >= 0 && dr < WIDTH;
        bool const is_b = db >= 0 && db < WIDTH;

        BK_ASSERT(!(is_l || is_r) || is_l ^ is_r);
        BK_ASSERT(!(is_t || is_b) || is_t ^ is_b);

        size_t const value =
 
            (is_l ? bklib::get_enum_value(state::left)   : 0)
          | (is_t ? bklib::get_enum_value(state::top)    : 0)
          | (is_r ? bklib::get_enum_value(state::right)  : 0)
          | (is_b ? bklib::get_enum_value(state::bottom) : 0);

        state_ = static_cast<state>(value);

        if (state_ != state::none) {
            gui::track_mouse_input(widget_, true);
        }
    }

    void on_mouse_up(mouse&, scalar, scalar, unsigned button) {
        if (button != 0) return;
        if (state_ == state::none) return;

        state_ = state::none;
        gui::track_mouse_input(widget_, false);
    }

    void on_mouse_move_to(mouse& m, scalar x, scalar y) {
        if (state_ == state::none) return;

        auto BK_CONSTEXPR MIN_W = 16;
        auto BK_CONSTEXPR MIN_H = 16;

        auto const last_mouse = m.absolute(1);

        auto const old = bklib::make_point2d<scalar, scalar>(last_mouse.x, last_mouse.y);
        auto const cur = bklib::make_point2d<scalar, scalar>(x, y);
        auto const delta = cur - old;

        auto const bounds = widget_.bounds();
        auto l = bounds.left();
        auto t = bounds.top();
        auto r = bounds.right();
        auto b = bounds.bottom();

        auto const off_l = x - l;
        auto const off_t = y - t;
        auto const off_r = x - r;
        auto const off_b = y - b;

        auto const adjust_l = [&] { if (r - l < MIN_W) l = r - MIN_W; };
        auto const adjust_t = [&] { if (b - t < MIN_H) t = b - MIN_H; };
        auto const adjust_r = [&] { if (r - l < MIN_W) r = l + MIN_W; };
        auto const adjust_b = [&] { if (b - t < MIN_W) b = t + MIN_H; };

        auto const update_side = [](scalar& side, scalar const delta, scalar const offset) {
            if      (delta > 0 && offset > 0) side += delta;
            else if (delta < 0 && offset < 0) side += delta;
 
        };

        if (state_contains(state::left)) {
            update_side(l, delta.x, off_l);
            adjust_l();
        } else if (state_contains(state::right)) {
            update_side(r, delta.x, off_r);
            adjust_r();
        }

        if (state_contains(state::top)) {
            update_side(t, delta.y, off_t);
            adjust_t();
        } else if (state_contains(state::bottom)) {
            update_side(b, delta.y, off_b);
            adjust_b();
        }
 

        auto const new_bounds = bounding_box{l, t, r, b};
        widget_.resize(new_bounds);
    }
private:
    widget_base& widget_;
    state        state_;
};

class mover {
public:
    using mouse = bklib::mouse;

    explicit mover(widget_base& widget)
        : widget_{widget}
    {
    }

    bool is_moving() const {
        return moving_;
    }

    void on_mouse_down(mouse&, scalar, scalar, unsigned button) {
        if (button != 0) return;
        BK_ASSERT(!moving_);

        moving_ = true;
        gui::track_mouse_input(widget_, true);
    }

    void on_mouse_up(mouse&, scalar, scalar, unsigned button) {
        if (button != 0) return;
        if (moving_ == false) return;

        moving_ = false;
        gui::track_mouse_input(widget_, false);
    }

    void on_mouse_move_to(mouse& m, scalar x, scalar y) {
        if (!moving_) return;

        auto const last_mouse = m.absolute(1);

        auto const old = bklib::make_point2d<scalar, scalar>(last_mouse.x, last_mouse.y);
        auto const cur = bklib::make_point2d<scalar, scalar>(x, y);
        auto const delta = cur - old;

        auto const bounds = widget_.bounds();
        auto const p = bounding_box::tl_point{
            bounds.top_left() + delta
        };

        bounding_box const new_bounds {p, bounds.width(), bounds.height()};
        widget_.resize(new_bounds);
    }
private:
    widget_base& widget_;
    bool         moving_ = false;
};

class canvas : public widget_base {
public:
    using widget_base::widget_base;

    virtual void on_mouse_down(bklib::mouse& mouse, scalar x, scalar y, unsigned button) {
        if (!mover_.is_moving()) {
            frame_.on_mouse_down(mouse, x, y, button);
        }
        if (!frame_.is_sizing()) {
            mover_.on_mouse_down(mouse, x, y, button);
        }
    }

    virtual void on_mouse_up(bklib::mouse& mouse, scalar x, scalar y, unsigned button) {
        if (!mover_.is_moving()) {
            frame_.on_mouse_up(mouse, x, y, button);
        }

        if (!frame_.is_sizing()) {
            mover_.on_mouse_up(mouse, x, y, button);
        }
    }

    virtual void on_mouse_move_to(bklib::mouse& mouse, scalar x, scalar y) {
        if (!mover_.is_moving()) {
            frame_.on_mouse_move_to(mouse, x, y);
        }

        if (!frame_.is_sizing()) {
            mover_.on_mouse_move_to(mouse, x, y);
        }
    }

    virtual void on_mouse_enter(bklib::mouse& mouse, scalar x, scalar y) {
    }

    virtual void on_mouse_exit(bklib::mouse& mouse, scalar x, scalar y) {
    }

    virtual void draw(renderer& r) const override {
        r.set_color_brush(bklib::gfx::color3f {{1.0f, 1.0f, 1.0f}});
        r.fill_rect(bounds_);

        r.set_color_brush(bklib::gfx::color3f {{0.0f, 0.0f, 0.0f}});
        r.draw_rect(bounds_, 8.0f);
    }

private:
    sizing_frame frame_ {*this};
    mover        mover_ {*this};
};
//==============================================================================
class icon_grid : public widget_base
{
    BK_GUI_DECLARE_LISTENER(on_mouse_move);
    BK_GUI_DECLARE_LISTENER(on_mouse_down);
    BK_GUI_DECLARE_LISTENER(on_mouse_up);
public:
    static size_t const GRID_ITEM_SIZE   = 32;
    static size_t const GRID_ITEM_BORDER = 2;
    static size_t const GRID_SIZE        = GRID_ITEM_SIZE + GRID_ITEM_BORDER;

    static bounding_box const& check_bounds_(bounding_box const& bounds) {
        BK_ASSERT(bounds.width() >= GRID_SIZE);
        return bounds;
    }

    static size_t check_count_(size_t const count) {
        BK_ASSERT(count > 0);
        return count;
    }

    icon_grid(bklib::utf8string name, bounding_box bounds, size_t count)
      : widget_base{bounds, nullptr, std::move(name)}
      , count_ {check_count_(count)}
      , items_ (count)
    {
        calculate_rows_cols_();
    }

    struct grid_item {
        virtual uint32_t color() const = 0;
        virtual bklib::hash_t hash() const BK_NOEXCEPT { return reinterpret_cast<bklib::hash_t>(this); }
    };

    virtual void on_mouse_move_to(bklib::mouse& mouse, scalar x, scalar y) {
        index_ = index_from_pos(x, y);
    }

    virtual void on_mouse_down(bklib::mouse& mouse, scalar x, scalar y, unsigned button) {
    }

    virtual void on_mouse_up(bklib::mouse& mouse, scalar x, scalar y, unsigned button) {
    }

    virtual void on_mouse_enter(bklib::mouse& mouse, scalar x, scalar y) {
        BK_UNUSED(mouse); BK_UNUSED(x); BK_UNUSED(y);
        mouse_in_ = true;
    }

    virtual void on_mouse_exit(bklib::mouse& mouse, scalar x, scalar y) {
        BK_UNUSED(mouse); BK_UNUSED(x); BK_UNUSED(y);
        mouse_in_ = false;
    }

    size_t columns() const BK_NOEXCEPT { return col_count_; }
    size_t rows()    const BK_NOEXCEPT { return row_count_; }

    bklib::point2d<size_t> index_to_xy(size_t i) const {
        auto const cols = columns();
        return {i / cols, i % cols};
    }

    bounding_box index_to_rect(size_t i) const {
        BK_ASSERT(i < count_);

        auto const row = i / columns();
        auto const col = i % columns();

        auto const origin = bounds_.top_left();

        scalar const left   = origin.x + col * GRID_SIZE;
        scalar const top    = origin.y + row * GRID_SIZE;
        scalar const right  = left + GRID_SIZE;
        scalar const bottom = top + GRID_SIZE;

        return bounding_box{{left, top, right, bottom}};
    }

    size_t index_from_pos(scalar x, scalar y) {
        auto const origin = bounds_.top_left();

        auto const ox = x - origin.x;
        auto const oy = y - origin.y;

        BK_ASSERT(ox >= 0 && oy >= 0);

        auto const col = static_cast<size_t>(ox / GRID_SIZE);
        auto const row = static_cast<size_t>(oy / GRID_SIZE);

        if (col < columns() && row < rows()) {
            return col + row * columns();
        } else {
            return count_;
        }
    }

    virtual void draw(renderer& r) const override {
        auto const cols = columns();
        auto const rows = columns();

        auto const origin = bounding_box::tl_point{bounds_.top_left()};

        auto const gs = static_cast<scalar>(GRID_SIZE);
        auto const bs = static_cast<scalar>(GRID_ITEM_BORDER);

        bounding_box const cell_back  {origin, gs, gs};
        bounding_box const cell_front {
            cell_back.left()   + bs
          , cell_back.top()    + bs
          , cell_back.right()  - bs
          , cell_back.bottom() - bs
        };

        color const border_color         {0.0f, 0.0f, 0.0f};
        color const cell_backcolor       {0.0f, 0.0f, 0.0f};
        color const cell_forecolor       {0.5f, 0.5f, 0.5f};
        color const cell_highlight_color {0.0f, 1.0f, 0.0f};

        r.set_color_brush(border_color);
        r.draw_rect(bounds());

        for (size_t i = 0; i < count_; ++i) {
            scalar const yi = i / cols;
            scalar const xi = i % cols;
            auto const    v = bklib::make_vector2d(xi*gs, yi*gs);

            r.set_color_brush(cell_backcolor);
            r.fill_rect(cell_back + v);

            if (mouse_in_ && i == index_) {
                r.set_color_brush(cell_highlight_color);
            } else {
                r.set_color_brush(cell_forecolor);
            }
            r.fill_rect(cell_front + v);
        }
    }

private:
    void calculate_rows_cols_() {
        auto const width = bounds_.width();
        auto const cols  = width / GRID_SIZE;
        auto const rows  = (count_ / cols) + ((count_ % cols) ? 1: 0);

        col_count_ = cols;
        row_count_ = rows;
    }

    size_t count_;
    std::vector<std::unique_ptr<grid_item>> items_;

    unsigned col_count_;
    unsigned row_count_;

    bool mouse_in_ = false;
    size_t index_ = 0;
};

} //namespace gui
} //namespace tez

#undef BK_GUI_DECLARE_LISTENER
