#pragma once

#include <bklib/types.hpp>
#include <bklib/util.hpp>
#include <bklib/math.hpp>
#include <bklib/callback.hpp>

#include <bklib/window.hpp>

#include <bklib/impl/win/direct2d.hpp>

namespace tez {
namespace gui {

using scalar       = int;
using point        = bklib::point2d<scalar>;
using bounding_box = bklib::axis_aligned_rect<scalar>;
using renderer     = bklib::detail::d2d_renderer;

class widget_base;
class container_widget;
class canvas;

using on_mouse_move_to = bklib::on_mouse_move_to;
using on_mouse_enter   = bklib::on_mouse_enter;
using on_mouse_exit    = bklib::on_mouse_exit;

class widget_base {
public:
    explicit widget_base(bklib::utf8string name)
      : name_{std::move(name)}
      , hash_{bklib::utf8string_hash(name_)}
    {
    }

    virtual void listen(gui::on_mouse_move_to callback) { on_mouse_move_to_ = callback; }
    virtual void listen(gui::on_mouse_enter   callback) { on_mouse_enter_   = callback; }
    virtual void listen(gui::on_mouse_exit    callback) { on_mouse_exit_    = callback; }

    virtual void on_mouse_move_to(bklib::mouse& mouse, scalar x, scalar y) {
        if (on_mouse_move_to_) on_mouse_move_to_(mouse, x, y);
    }

    virtual void on_mouse_enter(bklib::mouse& mouse, scalar x, scalar y) {
        if (on_mouse_move_to_) on_mouse_move_to_(mouse, x, y);
    }

    virtual void on_mouse_exit(bklib::mouse& mouse, scalar x, scalar y) {
        if (on_mouse_move_to_) on_mouse_move_to_(mouse, x, y);
    }

    virtual void draw(renderer& r) const {
        draw(r, point{{scalar{0}, scalar{0}}});
    }

    virtual void draw(renderer& r, widget_base const& relative_to) const {
        draw(r, relative_to.bounds().top_left());
    }

    virtual void draw(renderer& r, point top_left) const = 0;

    virtual widget_base*      parent() const { return nullptr; }
    virtual bklib::string_ref name()   const { return name_; }
    virtual bklib::hash_t     hash()   const { return hash_; }

    virtual bounding_box bounds() const = 0;
protected:
    bklib::utf8string name_;
    bklib::hash_t     hash_;

    gui::on_mouse_move_to on_mouse_move_to_;
    gui::on_mouse_enter   on_mouse_enter_;
    gui::on_mouse_exit    on_mouse_exit_;
};

inline point local_to_screen(widget_base const& widget, point const p) {
    point result = p;

    for (auto w = &widget; w != nullptr; w = w->parent()) {
        auto top_left = w->bounds().top_left();
        result.x += top_left.x;
        result.y += top_left.y;
    }

    return result;
}

class container_widget : public widget_base {
public:
    using widget_base::widget_base;

    auto begin()        { return zchildren_.begin(); }
    auto end()          { return zchildren_.end(); }
    auto begin()  const { return zchildren_.begin(); }
    auto end()    const { return zchildren_.end(); }
    auto cbegin() const { return zchildren_.cbegin(); }
    auto cend()   const { return zchildren_.cend(); }

    using widget_base::draw;
    virtual void draw(renderer& r, point top_left) const override {
        for (auto const& child : zchildren_) {
            child.second->draw(r, top_left); //TODO
        }
    }

    virtual bounding_box bounds() const override {
        return bounding_box {};
    }

    template <typename T>
    T* add_child(std::unique_ptr<T> child, int zorder = 0) {
        static_assert(std::is_base_of<widget_base, T>::value, "invalid type.");

        std::unique_ptr<widget_base> base = std::move(child);
        return static_cast<T*>(add_child_(std::move(base), zorder));
    }

    std::unique_ptr<widget_base> remove_child(bklib::utf8string name) {
        return remove_child(bklib::utf8string_hash(name));
    }

    std::unique_ptr<widget_base> remove_child(bklib::string_ref name) {
        return remove_child(bklib::utf8string_hash(name));
    }

    std::unique_ptr<widget_base> remove_child(bklib::hash_t hash) {
        std::unique_ptr<widget_base> result;

        auto const it = children_.find(hash);
        if (it == std::cend(children_)) {
            return result;
        }

        result = std::move(it->second);
        children_.erase(it);

        auto const zorder = it->first;
        auto const ptr    = result.get();
        auto const range  = zchildren_.equal_range(zorder);

        for (auto it = range.first; it != range.second; ++it) {
            if (it->second == ptr) {
                zchildren_.erase(it);
                break;
            }
        }

        return result;
    }

    virtual void on_mouse_move_to(bklib::mouse& mouse, scalar x, scalar y) {
        widget_base::on_mouse_move_to(mouse, x, y);

        auto old = mouse.absolute(1);

        point const p_cur {x, y};
        point const p_old {old.x, old.y};

        for (auto const& pair : zchildren_) {
            auto& child = *pair.second;

            bool const is_in  = bklib::intersects(child.bounds(), p_cur);
            bool const was_in = bklib::intersects(child.bounds(), p_old);

            if (is_in && was_in) {
                //move
                child.on_mouse_move_to(mouse, x, y);
            } else if (is_in && !was_in) {
                //enter
                child.on_mouse_enter(mouse, x, y);
            } else if (!is_in && was_in) {
                // exit
                child.on_mouse_exit(mouse, x, y);
            } else if (!is_in && !was_in) {
                // nothing
            }
        }
    }

    virtual void on_mouse_enter(bklib::mouse& mouse, scalar x, scalar y) {
        widget_base::on_mouse_enter(mouse, x, y);
        on_mouse_move_to(mouse, x, y);
    }

    virtual void on_mouse_exit(bklib::mouse& mouse, scalar x, scalar y) {
        widget_base::on_mouse_exit(mouse, x, y);
        on_mouse_move_to(mouse, x, y);
    }
private:
    widget_base* add_child_(std::unique_ptr<widget_base> child, int zorder) {
        auto const hash = child->hash();
        auto const ptr  = child.get();

        auto pair = std::make_pair(hash, std::move(child));
        auto const result = children_.insert(std::move(pair));
        if (!result.second) {
            BK_DEBUG_BREAK(); //TODO
        }

        auto where = zchildren_.upper_bound(zorder);
        zchildren_.insert(where, std::make_pair(zorder, ptr));

        return ptr;
    }

    struct compare_t {
        bool operator()(int const za, int const zb) const BK_NOEXCEPT {
            return za < zb;
        }
    };

    boost::container::flat_map<bklib::hash_t, std::unique_ptr<widget_base>> children_;
    boost::container::flat_multimap<int, widget_base*, compare_t> zchildren_;
};

class canvas : public widget_base {
public:
    using widget_base::widget_base;

    virtual void on_mouse_move_to(bklib::mouse& mouse, scalar x, scalar y) {
        widget_base::on_mouse_move_to(mouse, x, y);
    }

    virtual void on_mouse_enter(bklib::mouse& mouse, scalar x, scalar y) {
        widget_base::on_mouse_enter(mouse, x, y);
        mouse_in_ = true;
    }

    virtual void on_mouse_exit(bklib::mouse& mouse, scalar x, scalar y) {
        widget_base::on_mouse_exit(mouse, x, y);
        mouse_in_ = false;
    }

    void set_bounds(bounding_box bounds) {
        bounds_ = bounds;
    }

    virtual void draw(renderer& r, point top_left) const {
        r.draw_filled_rect(bounds_);
        if (mouse_in_) {
            r.draw_rect(bounds_);
        }
    }

    virtual bounding_box bounds() const {
        return bounds_;
    }
private:
    bounding_box bounds_;
    bool mouse_in_ = false;
};

} //namespace gui
} //namespace tez
