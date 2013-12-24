#include "loot_table.hpp"

#include <bklib/assert.hpp>
#include <bklib/util.hpp>
#include <bklib/json.hpp>

using tez::loot_table;
using tez::loot_table_parser;
using tez::distribution;
using tez::table_ref;
using tez::item_ref;
using tez::random_t;

using bklib::utf8string;

////////////////////////////////////////////////////////////////////////////////
// File local state.
////////////////////////////////////////////////////////////////////////////////
namespace {
namespace local_state {

template <typename Key, typename Value>
using map_t = boost::container::flat_map<Key, Value>;

map_t<table_ref, loot_table> loot_tables;
map_t<item_ref,  utf8string> loot_items;

} //namespace local_state
} //namespace
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// tez::loot_table
////////////////////////////////////////////////////////////////////////////////
loot_table::loot_table(utf8string id, table_entries&& entries, weights_t const& weights)
  : entries_{std::move(entries)}
  , id_ {id}
  , ref_{{bklib::utf8string_hash(id)}}
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
    boost::container::flat_set<table_ref> history;

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

    auto const value = *boost::get<table_ref>(&entry.value);
    auto const table = to_loot_table(value);
    if (table == nullptr) {
        BK_DEBUG_BREAK();
    }

    auto const where = history.lower_bound(ref_);
    if (where != std::cend(history) && *where == ref_) {
        BK_DEBUG_BREAK();
    }

    history.emplace_hint(where, ref_);

    for (auto j = 0; j < count; ++j) {
        table->roll(random, items, history);
    }
}

loot_table* tez::to_loot_table(table_ref  ref) { return nullptr; }
loot_table* tez::to_loot_table(utf8string id) { return nullptr; }
loot_table* tez::to_loot_table(string_ref id) { return nullptr; }

////////////////////////////////////////////////////////////////////////////////
// tez::loot_table_parser
////////////////////////////////////////////////////////////////////////////////

namespace json = bklib::json;

    //try {
    //} catch (json::error::base& e) {
    //    BK_JSON_ADD_TRACE(e);
    //}

void loot_table_parser::rule_root(cref json_value) {
    try {
        json::require_object(json_value);
        if (json_value.size() > 1) {
            BK_DEBUG_BREAK(); //TODO
        }

        auto table_list = json::require_key(json_value, "tables");
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

        rule_table_id(json_value);
        rule_table_def(json_value);
    } catch (json::error::base& e) {
        BK_JSON_ADD_TRACE(e);
    }
}

void loot_table_parser::rule_table_id(cref json_value) {
    try {
        auto table_id = json::require_key(json_value, "id");
        table_id_ = json::require_string(table_id);
    } catch (json::error::base& e) {
        BK_JSON_ADD_TRACE(e);
    }
}

void loot_table_parser::rule_table_def(cref json_value) {
    try {
        auto table_def = json::require_key(json_value, "table");
        json::require_array(table_def);

        std::vector<double> weights;
        std::vector<loot_table::entry> entries;

        json::for_each_element_skip_on_fail(table_def, [&](cref entry) {
            rule_table_entry(entry);

            weights.emplace_back(entry_weight_);
            entries.emplace_back(std::move(entry_value_), std::move(entry_dist_));
        });

        tables_.emplace_back(std::move(table_id_), std::move(entries), weights);
    } catch (json::error::base& e) {
        BK_JSON_ADD_TRACE(e);
    }
}

void loot_table_parser::rule_table_entry(cref json_value) {
    try {
        json::require_array(json_value);
        if (json_value.size() > 4) {
            BK_DEBUG_BREAK(); //TODO
        }

        auto entry_weight = json::require_key(json_value, 0);
        auto entry_type   = json::require_key(json_value, 1);
        auto entry_id     = json::require_key(json_value, 2);
        auto entry_dist   = json::optional_key(json_value, 3);

        rule_entry_weight(entry_weight);
        rule_entry_type(entry_type);
        rule_entry_id(entry_id);
        rule_entry_dist(entry_dist);

        if (entry_type_ == "item") {
            entry_value_ = item_ref{bklib::utf8string_hash(entry_id_)};
        } else if (entry_type_ == "table") {
            entry_value_ = table_ref{bklib::utf8string_hash(entry_id_)};
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
        auto dist_name = json::require_key(json_value, 0);
        auto name = json::require_string(dist_name);

        auto const result =
            rule_dist_uniform(name, json_value)
         || rule_dist_normal(name, json_value)
         || rule_dist_fixed(name, json_value);

        if (!result) {
            BK_DEBUG_BREAK();
        }
    } catch (json::error::base& e) {
        BK_JSON_ADD_TRACE(e);
    }
}

bool loot_table_parser::rule_dist_uniform(utf8string const& id, cref json_value) {
    try {
        if (id != "uniform") {
            return false;
        } else if (json_value.size() > 3) {
            BK_DEBUG_BREAK();
        }

        auto dist_min  = json::require_key(json_value, 1);
        auto dist_max  = json::require_key(json_value, 2);

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
        if (id != "normal") {
            return false;
        } else if (json_value.size() > 3) {
            BK_DEBUG_BREAK();
        }


        auto dist_mean   = json::require_key(json_value, 1);
        auto dist_stddev = json::require_key(json_value, 2);

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
        if (id != "fixed") {
            return false;
        } else if (json_value.size() > 2) {
            BK_DEBUG_BREAK();
        }

        auto dist_value = json::require_key(json_value, 1);
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
