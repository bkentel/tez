#include <bklib/window.hpp>
#include <bklib/math.hpp>
#include <bklib/renderer2d.hpp>

#include <glm/glm.hpp>

#include "room.hpp"
#include "algorithms.hpp"
#include "hotkeys.hpp"

////

class game_main {
public:
    game_main()
      : window_     {L"tez"}
      , renderer2d_ {window_.get_handle()}
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

        tez::generator::layout_random layout { };
        tez::generator::room_simple   simple {tez::generator::room_simple::range{{3, 10}}, tez::generator::room_simple::range{{3, 10}}};

        tez::random rand {10};

        for (size_t i = 0; i < 20; ++i) {
            auto room = simple.generate(rand);
            layout.insert(rand, std::move(room));
        }

        layout.normalize();

        map_ = layout.to_grid();
    }

    void render() {
        renderer2d_.begin();
        renderer2d_.clear();

        for (auto const& cell : map_) {
            if (cell.value.type == tez::tile_type::empty) continue;

            auto const ix = cell.i.x;
            auto const iy = cell.i.y;

            bklib::renderer2d::rect const r = {
                ix * 32.0f
              , iy * 32.0f
              , ix * 32.0f + 32.0f
              , iy * 32.0f + 32.0f
            };

            renderer2d_.fill_rect(r);
        }

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
    }
    void on_mouse_down(bklib::mouse& mouse, unsigned button) {
    }
    void on_mouse_up(bklib::mouse& mouse, unsigned button) {
    }

    void on_keydown(bklib::keyboard& keyboard, bklib::keycode key) {
        auto const combo   = keyboard.state();
        auto const command = keymap_.translate(combo);

        switch (command) {
        case tez::command_type::DIR_NORTH :
            std::cout << "north" << std::endl;
            break;
        }
    }
    void on_keyup(bklib::keyboard& keyboard, bklib::keycode key) {
    }
    void on_keyrepeat(bklib::keyboard& keyboard, bklib::keycode key) {
    }

private:
    bklib::platform_window window_;
    bklib::renderer2d      renderer2d_;
    tez::grid2d<tez::tile_data> map_;

    tez::hotkeys keymap_;
};

int main(int argc, char const* argv[]) try {
    game_main game;    
    return game.run();
} catch (bklib::exception_base const&) {
    BK_DEBUG_BREAK(); //TODO
} catch (std::exception const&) {
    BK_DEBUG_BREAK(); //TODO
} catch (...) {
    BK_DEBUG_BREAK(); //TODO
}
