#include <gtest/gtest.h>
#include "loot_table.hpp"

TEST(LootTable, Basic) {
    auto in = std::ifstream{"./data/loot.def"};

    auto parser = tez::loot_table_parser{in};
    parser.parse();
}
