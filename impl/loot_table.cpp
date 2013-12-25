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

using bklib::utf8string;
using bklib::string_ref;

////////////////////////////////////////////////////////////////////////////////
// File local state.
////////////////////////////////////////////////////////////////////////////////
namespace {

    template <size_t N>
    bklib::string_ref make_string_ref(char const (&str)[N]) {
        return bklib::string_ref{str, N - 1};
    }

namespace local_state {

template <typename Key, typename Value>
using map_t = boost::container::flat_map<Key, Value>;

map_t<loot_table_ref, loot_table> loot_tables;
map_t<item_ref,  utf8string> loot_items;

loot_table* get_table(loot_table_ref table) {
    auto const where = loot_tables.find(table);

    return (where != std::cend(loot_tables))
      ? &where->second
      : nullptr
    ;
}

bool add_table(loot_table&& table) {
    auto const ref   = table.reference();
    auto const where = loot_tables.lower_bound(ref);

    if (where != std::cend(loot_tables) && where->first == ref) {
        return false;
    }

    loot_tables.emplace_hint(where, ref, std::move(table));
    return true;
}

void add_loot() {
    string_ref const items[] = {
          make_string_ref("cheese")
        , make_string_ref("apple")
        , make_string_ref("meat")
        , make_string_ref("candy")
        , make_string_ref("axe")
        , make_string_ref("sword")
        , make_string_ref("dagger")
        , make_string_ref("hammer")
        , make_string_ref("bow")
        , make_string_ref("staff")
        , make_string_ref("wand")
    };

    for (auto const item : items) {
        item_ref const ref {bklib::utf8string_hash(item.data())};
        auto const result = loot_items.emplace(ref, item);
        if (!result.second) {
            BK_DEBUG_BREAK(); //TODO
        }
    }
}

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

loot_table* tez::to_loot_table(loot_table_ref ref) {
    return local_state::get_table(ref);
}

loot_table* tez::to_loot_table(utf8string id) { return nullptr; }

loot_table* tez::to_loot_table(string_ref id) {
    loot_table_ref const ref {bklib::utf8string_hash(id.data())};
    return to_loot_table(ref);
}

string_ref tez::to_item(item_ref item) {
    static string_ref const none {"{invalid}"};

    auto const where = local_state::loot_items.find(item);

    return where == std::cend(local_state::loot_items) ? none : where->second;
}

////////////////////////////////////////////////////////////////////////////////
// tez::loot_table_parser
////////////////////////////////////////////////////////////////////////////////

namespace json = bklib::json;

namespace {
    auto   const LOOT_ROOT_KEY  = make_string_ref("tables");
    size_t const LOOT_ROOT_SIZE = 1;

    size_t const LOOT_TABLE_SIZE = 2;
    auto   const LOOT_TABLE_ID   = make_string_ref("id");
    auto   const LOOT_TABLE_DEF  = make_string_ref("table");

    size_t const LOOT_ENTRY_MIN_SIZE     = 3;
    size_t const LOOT_ENTRY_MAX_SIZE     = 4;
    auto   const LOOT_ENTRY_TYPE_ITEM    = make_string_ref("item");
    auto   const LOOT_ENTRY_TYPE_TABLE   = make_string_ref("table");
    size_t const LOOT_ENTRY_WEIGHT_INDEX = 0;
    size_t const LOOT_ENTRY_TYPE_INDEX   = 1;
    size_t const LOOT_ENTRY_ID_INDEX     = 2;
    size_t const LOOT_ENTRY_DIST_INDEX   = 3;

    size_t const LOOT_DIST_TYPE_INDEX          = 0;
    auto   const LOOT_DIST_UNIFORM             = make_string_ref("uniform");
    size_t const LOOT_DIST_UNIFORM_PARAMS      = 3;
    size_t const LOOT_DIST_UNIFORM_MIN_INDEX   = 1;
    size_t const LOOT_DIST_UNIFORM_MAX_INDEX   = 2;
    auto   const LOOT_DIST_NORMAL              = make_string_ref("normal");
    size_t const LOOT_DIST_NORMAL_PARAMS       = 3;
    size_t const LOOT_DIST_NORMAL_MEAN_INDEX   = 1;
    size_t const LOOT_DIST_NORMAL_STDDEV_INDEX = 2;
    auto   const LOOT_DIST_FIXED               = make_string_ref("fixed");
    size_t const LOOT_DIST_FIXED_PARAMS        = 2;
    size_t const LOOT_DIST_FIXED_VALUE_INDEX   = 1;
}

void loot_table_parser::rule_root(cref json_value) {
    local_state::add_loot(); //TODO delete me

    try {
        json::require_object(json_value);
        if (json_value.size() != LOOT_ROOT_SIZE) {
            BK_DEBUG_BREAK(); //TODO
        }

        auto table_list = json::require_key(json_value, LOOT_ROOT_KEY);
        rule_table_list(table_list);
    } catch (json::error::base& e) {
        BK_JSON_ADD_TRACE(e);
    }

    for (auto& table : tables_) {
        local_state::add_table(std::move(table));
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
        if (json_value.size() != LOOT_TABLE_SIZE) {
            BK_DEBUG_BREAK(); //TODO
        }

        rule_table_id(json_value);
        rule_table_def(json_value);
    } catch (json::error::base& e) {
        BK_JSON_ADD_TRACE(e);
    }
}

void loot_table_parser::rule_table_id(cref json_value) {
    try {
        auto table_id = json::require_key(json_value, LOOT_TABLE_ID);
        table_id_ = json::require_string(table_id);
    } catch (json::error::base& e) {
        BK_JSON_ADD_TRACE(e);
    }
}

void loot_table_parser::rule_table_def(cref json_value) {
    try {
        auto table_def = json::require_key(json_value, LOOT_TABLE_DEF);
        json::require_array(table_def);

        loot_table::weights_t     weights;
        loot_table::table_entries entries;      

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
        if (json_value.size() < LOOT_ENTRY_MIN_SIZE) {
            BK_DEBUG_BREAK(); //TODO
        } else if (json_value.size() > LOOT_ENTRY_MAX_SIZE) {
            BK_DEBUG_BREAK(); //TODO
        }

        auto entry_weight = json::require_key(json_value, LOOT_ENTRY_WEIGHT_INDEX);
        auto entry_type   = json::require_key(json_value, LOOT_ENTRY_TYPE_INDEX);
        auto entry_id     = json::require_key(json_value, LOOT_ENTRY_ID_INDEX);
        auto entry_dist   = json::optional_key(json_value, LOOT_ENTRY_DIST_INDEX);

        rule_entry_weight(entry_weight);
        rule_entry_type(entry_type);
        rule_entry_id(entry_id);
        rule_entry_dist(entry_dist);

        if (entry_type_ == LOOT_ENTRY_TYPE_ITEM) {
            entry_value_ = item_ref{bklib::utf8string_hash(entry_id_)};
        } else if (entry_type_ == LOOT_ENTRY_TYPE_TABLE) {
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
        auto dist_type  = json::require_key(json_value, LOOT_DIST_TYPE_INDEX);
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
        if (id != LOOT_DIST_UNIFORM) {
            return false;
        } else if (json_value.size() != LOOT_DIST_UNIFORM_PARAMS) {
            BK_DEBUG_BREAK();
        }

        auto dist_min  = json::require_key(json_value, LOOT_DIST_UNIFORM_MIN_INDEX);
        auto dist_max  = json::require_key(json_value, LOOT_DIST_UNIFORM_MAX_INDEX);

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
        if (id != LOOT_DIST_NORMAL) {
            return false;
        } else if (json_value.size() != LOOT_DIST_NORMAL_PARAMS) {
            BK_DEBUG_BREAK();
        }

        auto dist_mean   = json::require_key(json_value, LOOT_DIST_NORMAL_MEAN_INDEX);
        auto dist_stddev = json::require_key(json_value, LOOT_DIST_NORMAL_STDDEV_INDEX);

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
        if (id != LOOT_DIST_FIXED) {
            return false;
        } else if (json_value.size() != LOOT_DIST_FIXED_PARAMS) {
            BK_DEBUG_BREAK();
        }

        auto dist_value = json::require_key(json_value, LOOT_DIST_FIXED_VALUE_INDEX);

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
