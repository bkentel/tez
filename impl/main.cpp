#include <bklib/window.hpp>

class game_main {
public:
    game_main()
      : window_(L"tez")
    {
        using namespace std::placeholders;
        using std::bind; 

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
    }

    int run() {
        while (window_.is_running()) {
            window_.do_events();
        }

        return window_.get_result().get() ? 0 : -1;
    }

    void on_mouse_move(bklib::mouse& mouse, int dx, int dy) {
        std::cout << "on_mouse_move:" << " dx = " << dx << " dy = " << dy << std::endl;
    }
    void on_mouse_move_to(bklib::mouse& mouse, int x, int y) {
        std::cout << "on_mouse_move_to:" << " x = " << x << " y = " << y << std::endl;
    }
    void on_mouse_down(bklib::mouse& mouse, unsigned button) {
        std::cout << "on_mouse_down:" << " button = " << button << std::endl;
    }
    void on_mouse_up(bklib::mouse& mouse, unsigned button) {
        std::cout << "on_mouse_up:" << " button = " << button << std::endl;
    }
private:
    bklib::platform_window window_;
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
