#include <gtest/gtest.h>
#include "loot_table.hpp"
#include "item.hpp"

TEST(LootTable, Basic) {   
    tez::reload_items("./data/items.def");


    auto in = std::ifstream{"./data/loot.def"};

    auto parser = tez::loot_table_parser{in};
    parser.parse();

    auto const table = tez::to_loot_table(bklib::string_ref("common"));

    tez::random_t random{150123};

    while (true) {
        std::cout << "****rolling***" << std::endl;

        auto items = table->roll(random);

        for (auto const item : items) {
            auto const name = tez::to_item(item);
            std::cout << name << std::endl;
        }
    }
}
