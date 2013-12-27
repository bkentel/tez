#include <gtest/gtest.h>
#include "loot_table.hpp"
#include "item.hpp"

#include <Windows.h>
#include <fcntl.h>
#include <io.h>
#include <stdio.h>

TEST(LootTable, Basic) {
    tez::loot_table_table::reload("./data/loot.def");
    tez::item_table::reload("./data/items.def");

    tez::random_t random{150123};

    auto table = tez::loot_table_table::get("orc");

    while (true) {
        std::cout << "****rolling***" << std::endl;

        auto items = table->roll(random);

        auto const en = tez::language_ref{bklib::utf8string_hash("en")};
        auto const jp = tez::language_ref{bklib::utf8string_hash("jp")};

        for (auto const item_ref : items) {
            auto const item = tez::item_table::get(item_ref);
            std::cout << item->names.get(en) << std::endl;
        }
    }
}
