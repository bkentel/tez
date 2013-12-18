#include "gui.hpp"

namespace gui = ::tez::gui;

void gui::track_mouse_input(widget_base& widget, bool track) {
    auto root = &widget;

    while (root->parent()) {
        root = root->parent();
    }

    BK_ASSERT(dynamic_cast<gui::root*>(root) != nullptr);
    auto as_root = static_cast<gui::root*>(root);

    as_root->track_mouse_input(widget, track);
}

gui::point gui::local_to_screen(widget_base const& widget, point const p) {
    point result = p;

    for (auto w = &widget; w != nullptr; w = w->parent()) {
        auto top_left = w->bounds().top_left();
        result.x += top_left.x;
        result.y += top_left.y;
    }

    return result;
}