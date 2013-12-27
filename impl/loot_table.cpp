#include "loot_table.hpp"

#include <bklib/assert.hpp>
#include <bklib/util.hpp>
#include <bklib/json.hpp>

using tez::loot_table;
using tez::loot_table_parser;
using tez::distribution;
using tez::loot_table_ref;
using tez::item_ref;
using tez::random_t;
using tez::make_string_ref;

using bklib::utf8string;
using bklib::string_ref;

////////////////////////////////////////////////////////////////////////////////
// tez::loot_table
////////////////////////////////////////////////////////////////////////////////
loot_table::loot_table(utf8string id, table_entries&& entries, weights_t const& weights)
  : entries_{std::move(entries)}
  , id_ {id}
  , distribution_(
        weights.size()
      , double {0}
      , static_cast<double>(weights.size())
      , [&](double d) {
            return weights[static_cast<size_t>(d)];
        }
    )
{
}

loot_table::item_list loot_table::roll(random_t& random) {
    std::vector<item_ref>  items;
    boost::container::flat_set<loot_table_ref> history;

    roll(random, items, history);

    return items;
}

void loot_table::roll(random_t& random, item_list& items, history_t& history) {
    auto const i = distribution_(random);
    BK_ASSERT(i >= 0 && i < entries_.size());

    auto const& entry   = entries_[i];
    auto const  is_item = boost::apply_visitor(visitor(), entry.value);
    auto const  count   = entry.count(random);

    if (is_item) {
        auto const value = *boost::get<item_ref>(&entry.value);
        for (auto j = 0; j < count; ++j) {
            items.emplace_back(value);
        }

        return;
    }

    auto const value = *boost::get<loot_table_ref>(&entry.value);
    auto const table = tez::loot_table_table::get(value);
    if (table == nullptr) {
        BK_DEBUG_BREAK();
    }

    //auto const where = history.lower_bound(reference());
    //if (where != std::cend(history) && *where == reference()) {
    //    BK_DEBUG_BREAK();
    //}

    //history.emplace_hint(where, reference());

    for (auto j = 0; j < count; ++j) {
        table->roll(random, items, history);
    }
}

////////////////////////////////////////////////////////////////////////////////
// tez::loot_table_parser
////////////////////////////////////////////////////////////////////////////////
namespace json = bklib::json;

namespace {

auto const KEY_ROOT             = make_string_ref("tables");
auto const KEY_TABLE_ID         = make_string_ref("id");
auto const KEY_TABLE_DEF        = make_string_ref("table");
auto const KEY_ENTRY_TYPE_ITEM  = make_string_ref("item");
auto const KEY_ENTRY_TYPE_TABLE = make_string_ref("table");
auto const KEY_DIST_UNIFORM     = make_string_ref("uniform");
auto const KEY_DIST_NORMAL      = make_string_ref("normal");
auto const KEY_DIST_FIXED       = make_string_ref("fixed");

size_t const SIZE_ROOT         = 1;
size_t const SIZE_TABLE        = 2;
size_t const SIZE_ENTRY_MIN    = 3;
size_t const SIZE_ENTRY_MAX    = 4;
size_t const SIZE_DIST_UNIFORM = 3;
size_t const SIZE_DIST_NORMAL  = 3;
size_t const SIZE_DIST_FIXED   = 2;

size_t const INDEX_DIST_TYPE          = 0;
size_t const INDEX_DIST_UNIFORM_MIN   = 1;
size_t const INDEX_DIST_UNIFORM_MAX   = 2;
size_t const INDEX_DIST_NORMAL_MEAN   = 1;
size_t const INDEX_DIST_NORMAL_STDDEV = 2;
size_t const INDEX_DIST_FIXED_VALUE   = 1;
size_t const INDEX_ENTRY_WEIGHT       = 0;
size_t const INDEX_ENTRY_TYPE         = 1;
size_t const INDEX_ENTRY_ID           = 2;
size_t const INDEX_ENTRY_DIST         = 3;

}

void loot_table_parser::rule_root(cref json_value) {
    try {
        json::require_object(json_value);
        json::require_size(json_value, SIZE_ROOT);

        auto table_list = json::require_key(json_value, KEY_ROOT);
        rule_table_list(table_list);
    } catch (json::error::base& e) {
        BK_JSON_ADD_TRACE(e);
    }
}

void loot_table_parser::rule_table_list(cref json_value) {
    try {
        json::require_array(json_value);

        json::for_each_element_skip_on_fail(json_value, [&](cref table) {
            rule_table(table);
        });
    } catch (json::error::base& e) {
        BK_JSON_ADD_TRACE(e);
    }
}

void loot_table_parser::rule_table(cref json_value) {
    try {
        json::require_object(json_value);
        json::require_size(json_value, SIZE_TABLE);

        rule_table_id(json_value);
        rule_table_def(json_value);
    } catch (json::error::base& e) {
        BK_JSON_ADD_TRACE(e);
    }
}

void loot_table_parser::rule_table_id(cref json_value) {
    try {
        auto table_id = json::require_key(json_value, KEY_TABLE_ID);
        table_id_ = json::require_string(table_id);
    } catch (json::error::base& e) {
        BK_JSON_ADD_TRACE(e);
    }
}

void loot_table_parser::rule_table_def(cref json_value) {
    try {
        auto table_def = json::require_key(json_value, KEY_TABLE_DEF);
        json::require_array(table_def);

        loot_table::weights_t     weights;
        loot_table::table_entries entries;

        json::for_each_element_skip_on_fail(table_def, [&](cref entry) {
            rule_table_entry(entry);

            weights.emplace_back(entry_weight_);
            entries.emplace_back(std::move(entry_value_), std::move(entry_dist_));
        });

        auto const ref = loot_table_ref{bklib::utf8string_hash(table_id_)};
        tables_.emplace(ref, loot_table{std::move(table_id_), std::move(entries), weights});
    } catch (json::error::base& e) {
        BK_JSON_ADD_TRACE(e);
    }
}

void loot_table_parser::rule_table_entry(cref json_value) {
    try {
        json::require_array(json_value);
        json::require_size(json_value, SIZE_ENTRY_MIN, SIZE_ENTRY_MAX);

        auto entry_weight = json::require_key(json_value, INDEX_ENTRY_WEIGHT);
        auto entry_type   = json::require_key(json_value, INDEX_ENTRY_TYPE);
        auto entry_id     = json::require_key(json_value, INDEX_ENTRY_ID);
        auto entry_dist   = json::optional_key(json_value, INDEX_ENTRY_DIST);

        rule_entry_weight(entry_weight);
        rule_entry_type(entry_type);
        rule_entry_id(entry_id);
        rule_entry_dist(entry_dist);

        if (entry_type_ == KEY_ENTRY_TYPE_ITEM) {
            entry_value_ = item_ref{bklib::utf8string_hash(entry_id_)};
        } else if (entry_type_ == KEY_ENTRY_TYPE_TABLE) {
            entry_value_ = loot_table_ref{bklib::utf8string_hash(entry_id_)};
        } else {
            BK_DEBUG_BREAK(); //TODO
        }
    } catch (json::error::base& e) {
        BK_JSON_ADD_TRACE(e);
    }
}

void loot_table_parser::rule_entry_weight(cref json_value) {
    try {
        entry_weight_ = json::require_int(json_value);
    } catch (json::error::base& e) {
        BK_JSON_ADD_TRACE(e);
    }
}

void loot_table_parser::rule_entry_type(cref json_value) {
    try {
        entry_type_ = json::require_string(json_value);
    } catch (json::error::base& e) {
        BK_JSON_ADD_TRACE(e);
    }
}

void loot_table_parser::rule_entry_id(cref json_value) {
    try {
        entry_id_ = json::require_string(json_value);
    } catch (json::error::base& e) {
        BK_JSON_ADD_TRACE(e);
    }
}

void loot_table_parser::rule_entry_dist(cref json_value) {
    try {
        if (json_value.isNull()) {
            entry_dist_ = distribution::make_fixed(1);
            return;
        }

        json::require_array(json_value);
        auto dist_type  = json::require_key(json_value, INDEX_DIST_TYPE);
        auto const type = json::require_string(dist_type);

        if (!rule_dist_uniform(type, json_value)
         && !rule_dist_normal(type, json_value)
         && !rule_dist_fixed(type, json_value)
        ) {
            BK_DEBUG_BREAK(); //TODO
        }
    } catch (json::error::base& e) {
        BK_JSON_ADD_TRACE(e);
    }
}

bool loot_table_parser::rule_dist_uniform(utf8string const& id, cref json_value) {
    try {
        if (id != KEY_DIST_UNIFORM) {
            return false;
        }

        json::require_size(json_value, SIZE_DIST_UNIFORM);

        auto dist_min  = json::require_key(json_value, INDEX_DIST_UNIFORM_MIN);
        auto dist_max  = json::require_key(json_value, INDEX_DIST_UNIFORM_MAX);

        rule_dist_min(dist_min);
        rule_dist_max(dist_max);

        entry_dist_ = distribution::make_normal(dist_min_, dist_max_);
    } catch (json::error::base& e) {
        BK_JSON_ADD_TRACE(e);
    }

    return true;
}

bool loot_table_parser::rule_dist_normal(utf8string const& id, cref json_value) {
    try {
        if (id != KEY_DIST_NORMAL) {
            return false;
        }

        json::require_size(json_value, SIZE_DIST_NORMAL);

        auto dist_mean   = json::require_key(json_value, INDEX_DIST_NORMAL_MEAN);
        auto dist_stddev = json::require_key(json_value, INDEX_DIST_NORMAL_STDDEV);

        rule_dist_mean(dist_mean);
        rule_dist_stddev(dist_stddev);

        entry_dist_ = distribution::make_normal(dist_mean_, dist_stddev_);
    } catch (json::error::base& e) {
        BK_JSON_ADD_TRACE(e);
    }

    return true;
}

bool loot_table_parser::rule_dist_fixed(utf8string const& id, cref json_value) {
    try {
        if (id != KEY_DIST_FIXED) {
            return false;
        }

        json::require_size(json_value, SIZE_DIST_FIXED);

        auto dist_value = json::require_key(json_value, INDEX_DIST_FIXED_VALUE);

        rule_dist_value(dist_value);

        entry_dist_ = distribution::make_fixed(dist_value_);
    } catch (json::error::base& e) {
        BK_JSON_ADD_TRACE(e);
    }

    return true;
}

void loot_table_parser::rule_dist_min(cref json_value) {
    try {
        dist_min_ = json::require_int(json_value);
    } catch (json::error::base& e) {
        BK_JSON_ADD_TRACE(e);
    }
}

void loot_table_parser::rule_dist_max(cref json_value) {
    try {
        dist_max_ = json::require_int(json_value);
    } catch (json::error::base& e) {
        BK_JSON_ADD_TRACE(e);
    }
}

void loot_table_parser::rule_dist_mean(cref json_value) {
    try {
        dist_mean_ = json::require_int(json_value);
    } catch (json::error::base& e) {
        BK_JSON_ADD_TRACE(e);
    }
}

void loot_table_parser::rule_dist_stddev(cref json_value) {
    try {
        dist_stddev_ = json::require_int(json_value);
    } catch (json::error::base& e) {
        BK_JSON_ADD_TRACE(e);
    }
}

void loot_table_parser::rule_dist_value(cref json_value) {
    try {
        dist_value_ = json::require_int(json_value);
    } catch (json::error::base& e) {
        BK_JSON_ADD_TRACE(e);
    }
}

////////////////////////////////////////////////////////////////////////////////
// loot_table_table
////////////////////////////////////////////////////////////////////////////////

bool                               tez::loot_table_table::is_loaded_ {false};
tez::loot_table_table::container_t tez::loot_table_table::data_      {};
