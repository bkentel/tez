#pragma once

#include <random>

#include <boost/variant.hpp>
#include <boost/container/flat_set.hpp>

#include <bklib/config.hpp>
#include <bklib/assert.hpp>
#include <bklib/json_forward.hpp>

#include "types.hpp"
#include "util.hpp"

#include <bklib/types.hpp>
#include <bklib/util.hpp>

namespace tez {

using random_t  = std::mt19937;

namespace detail {
    struct tag_item;
    struct tag_loot_table;
} //namespace detail

using item_ref       = tagged_value<hash_t, detail::tag_item>;
using loot_table_ref = tagged_value<hash_t, detail::tag_loot_table>;

//==============================================================================
//==============================================================================
class distribution {
public:
    template <typename T>
    static inline distribution make_uniform_int(T a, T b) {
        return distribution{std::uniform_int_distribution<T>{a, b}};
    }

    template <typename T>
    static inline distribution make_normal(T mean, T stddev) {
        return distribution{std::normal_distribution<>{
            static_cast<double>(mean)
          , static_cast<double>(stddev)
        }};
    }

    template <typename T>
    static inline distribution make_fixed(T value) {
        return distribution{value};
    }
public:
    distribution()
      : impl_{[](random_t& random) { return 0; }}
    {
    }

    template <typename T>
    explicit distribution(std::uniform_int_distribution<T> dist)
      : impl_{[dist](random_t& random) {
            return static_cast<int>(dist(random)); //TODO
        }}
    {
    }

    template <typename T>
    explicit distribution(std::normal_distribution<T> dist)
      : impl_{[dist](random_t& random) mutable {
            return static_cast<int>(std::round(dist(random))); //TODO
        }}
    {
    }

    template <typename T, typename std::enable_if<std::is_arithmetic<T>::value>::type* = 0>
    explicit distribution(T value)
      : impl_{[value](random_t& random) {
            return static_cast<int>(value); //TODO
        }}
    {
    }

    int operator()(random_t& random) const {
        return impl_(random);
    }
private:
    std::function<int (random_t&)> impl_;
};

//==============================================================================
//==============================================================================
class loot_table {
public:
    struct visitor : public boost::static_visitor<bool> {
        bool operator()(item_ref)  const BK_NOEXCEPT { return true; }
        bool operator()(loot_table_ref) const BK_NOEXCEPT { return false; }
    };

    struct entry {
        using value_t = boost::variant<item_ref, loot_table_ref>;

        entry() = default;
        entry(value_t value, distribution count)
          : value{std::move(value)}
          , count{std::move(count)}
        {
        }

        value_t      value = item_ref{0};
        distribution count = distribution{};
    };

    using table_entries = std::vector<entry>;
    using item_list     = std::vector<item_ref>;
    using history_t     = boost::container::flat_set<loot_table_ref>;
    using weights_t     = std::vector<double>;

    loot_table(utf8string id, table_entries&& entries, weights_t const& weights);
    loot_table() = default;

    item_list roll(random_t& random);
    void      roll(random_t& random, item_list& items, history_t& history);

    loot_table_ref reference() const { return loot_table_ref {id_.hash}; }
    string_ref id() const { return {id_.string}; }
private:
    table_entries                entries_ {{}};
    std::discrete_distribution<> distribution_ = std::discrete_distribution<>{};
    tez::hashed_string id_ {{""}};
};

// ROOT          -> {"tables": TABLE_LIST}
// TABLE_LIST    -> [TABLE*]
// TABLE         -> {TABLE_ID, TABLE_DEF}
// TABLE_ID      -> "id": string
// TABLE_DEF     -> "table": [TABLE_ENTRY*]
// TABLE_ENTRY   -> [ENTRY_WEIGHT, ENTRY_TYPE, ENTRY_ID, ENTRY_DIST | null]
// ENTRY_WEIGHT  -> unsigned
// ENTRY_TYPE    -> string
// ENTRY_ID      -> string
// ENTRY_DIST    -> DIST_UNIFORM | DIST_NORMAL | DIST_FIXED
// DIST_UNIFORM  -> ["uniform", DIST_MIN, DIST_MAX]
// DIST_MIN      -> unsigned
// DIST_MAX      -> unsigned
// DIST_NORMAL   -> ["normal", DIST_MEAN, DIST_STDDEV]
// DIST_MEAN     -> unsigned
// DIST_STDDEV   -> unsigned
// DIST_FIXED    -> ["fixed", DIST_VALUE]
// DIST_VALUE    -> unsigned

struct loot_table_parser : public parser_base<loot_table_parser> {
    using cref = bklib::json::cref;
    using utf8string = bklib::utf8string;

    using parser_base::parser_base;
    using parser_base::parse;

    void rule_root(cref json_value);

    void rule_table_list(cref json_value);
    void rule_table(cref json_value);
    void rule_table_id(cref json_value);
    void rule_table_def(cref json_value);
    void rule_table_entry(cref json_value);

    void rule_entry_weight(cref json_value);
    void rule_entry_type(cref json_value);
    void rule_entry_id(cref json_value);
    void rule_entry_dist(cref json_value);

    bool rule_dist_uniform(utf8string const& id, cref json_value);
    bool rule_dist_normal(utf8string const& id ,cref json_value);
    bool rule_dist_fixed(utf8string const& id, cref json_value);

    void rule_dist_min(cref json_value);
    void rule_dist_max(cref json_value);
    void rule_dist_mean(cref json_value);
    void rule_dist_stddev(cref json_value);
    void rule_dist_value(cref json_value);

    using map_t = boost::container::flat_map<loot_table_ref, loot_table>;

    map_t&& get() { return std::move(tables_); }

    utf8string                 table_id_     {{""}};
    int                        entry_weight_ {0};
    utf8string                 entry_type_   {{""}};
    utf8string                 entry_id_     {{""}};
    distribution               entry_dist_   {{}};
    loot_table::entry::value_t entry_value_  {{}};
    int                        dist_min_     {0};
    int                        dist_max_     {0};
    int                        dist_mean_    {0};
    int                        dist_stddev_  {0};
    int                        dist_value_   {0};

    map_t tables_ = {{}};
};

////////////////////////////////////////////////////////////////////////////////
// loot table functions
////////////////////////////////////////////////////////////////////////////////
namespace detail {
    template <> struct tag_traits<tag_loot_table> {
        using type    = loot_table;
        using pointer = loot_table*;
        using ref     = loot_table_ref;
        using parser  = loot_table_parser;
    };
} //namespace detail

extern template data_table<detail::tag_loot_table>;
using loot_table_table = data_table<detail::tag_loot_table>;

inline string_ref ref_to_id(loot_table_ref ref) {
    auto const loot_table = loot_table_table::get(ref);

    if (loot_table) return {loot_table->id()};
    else            return {""};
}


} //namespace tez
