#include <gtest/gtest.h>
#include "gui.hpp"

#include <bklib/window.hpp>
#include <bklib/impl/win/direct2d.hpp>

TEST(Gui, Basic) {
    namespace gui = tez::gui;
    using namespace tez::gui;

    container_widget container("container");

    static char const* names[] = {
        "canvas a"
      , "canvas b"
      , "canvas c"
      , "canvas d"
    };

    static gui::bounding_box const sizes[] = {
        {10,  10, 40,  20}
      , {60,  10, 90,  30}
      , {110, 10, 140, 40}
      , {160, 10, 190, 50}
    };

    std::vector<size_t> indicies;
    indicies.reserve(bklib::elements_in(names));

    std::generate_n(
        std::back_inserter(indicies)
      , bklib::elements_in(names)
      , std::bind([](size_t& i) { return i++; }, size_t{0})
    );

    std::random_shuffle(indicies.begin(), indicies.end());

    for (auto i : indicies) {
        auto ptr = container.add_child(std::make_unique<canvas>(names[i]));
        ptr->set_bounds(sizes[i]);
    }

    size_t i = 0;
    for (auto const& pair : bklib::as_const(container)) {
        //ASSERT_STREQ(widget->name().data(), names[indicies[i++]]);
    }


    bklib::platform_window window(L"test");
    bklib::detail::d2d_renderer renderer(window.get_handle());

    auto const on_mouse_move_to = [&](bklib::mouse& mouse, int x, int y) {
        container.on_mouse_move_to(mouse, x, y);
    };

    window.listen(bklib::on_mouse_move_to{on_mouse_move_to});

    while (window.is_running()) {
        window.do_events();

        renderer.begin();

        renderer.clear();
        container.draw(renderer);

        renderer.end();
    }
}
