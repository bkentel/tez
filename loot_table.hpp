#pragma once

#include <random>

#include <boost/variant.hpp>
#include <boost/container/flat_set.hpp>

#include <bklib/config.hpp>
#include <bklib/assert.hpp>
#include <bklib/json_forward.hpp>

#include "types.hpp"

namespace tez {

template <typename T, typename Tag>
struct tagged_value {
    T value;
};

template <typename T, typename Tag>
inline bool operator<(tagged_value<T, Tag> const& a, tagged_value<T, Tag> const& b) {
    return a.value < b.value;
}

template <typename T, typename Tag>
inline bool operator>(tagged_value<T, Tag> const& a, tagged_value<T, Tag> const& b) {
    return a.value > b.value;
}

template <typename T, typename Tag>
inline bool operator==(tagged_value<T, Tag> const& a, tagged_value<T, Tag> const& b) {
    return a.value == b.value;
}

template <typename T, typename Tag>
inline bool operator!=(tagged_value<T, Tag> const& a, tagged_value<T, Tag> const& b) {
    return a.value != b.value;
}

using random_t  = std::mt19937;

namespace detail {
    struct tag_item_ref;
    struct tag_table_ref;
} //namespace detail

using item_ref  = tagged_value<size_t, detail::tag_item_ref>;
using table_ref = tagged_value<size_t, detail::tag_table_ref>;

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
            return static_cast<int>(dist(random)); //TODO
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
        bool operator()(table_ref) const BK_NOEXCEPT { return false; }
    };

    struct entry {
        using value_t = boost::variant<item_ref, table_ref>;

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
    using history_t     = boost::container::flat_set<table_ref>;
    using weights_t     = std::vector<double>;

    loot_table(utf8string id, table_entries&& entries, weights_t const& weights);
    loot_table() = default;

    item_list roll(random_t& random);
    void      roll(random_t& random, item_list& items, history_t& history);
private:
    table_entries                entries_ {{}};
    std::discrete_distribution<> distribution_ = std::discrete_distribution<>{};
    std::string                  id_ {{""}};
    table_ref                    ref_ {{0}};
};

loot_table* to_loot_table(table_ref  ref);
loot_table* to_loot_table(utf8string id);
loot_table* to_loot_table(string_ref id);

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

class parser_base {
public:
    using cref       = bklib::json::cref;
    using utf8string = bklib::utf8string;

    explicit parser_base(utf8string const& file_name)
      : parser_base{std::ifstream{file_name}}
    {
    }

    explicit parser_base(std::istream&& in)
      : parser_base{in}
    {
    }

    explicit parser_base(std::istream& in) {
        if (!in) {
            BK_DEBUG_BREAK(); //TODO
        }

        Json::Reader reader;

        auto const result = reader.parse(in, json_root_);
        if (!result) {
            std::cout << reader.getFormattedErrorMessages();
            BK_DEBUG_BREAK(); //TODO
        }
    }
protected:
    Json::Value json_root_;
};

struct loot_table_parser : public parser_base {
    using cref = bklib::json::cref;
    using utf8string = bklib::utf8string;

    using parser_base::parser_base;

    void parse() {
        rule_root(json_root_);
    }

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

    utf8string   table_id_     {{""}};
    int          entry_weight_ {0};
    utf8string   entry_type_   {{""}};
    utf8string   entry_id_     {{""}};
    distribution entry_dist_   {{}};
    loot_table::entry::value_t entry_value_ {{}};
    int          dist_min_     {0};
    int          dist_max_     {0};
    int          dist_mean_    {0};
    int          dist_stddev_  {0};
    int          dist_value_   {0};

    std::vector<loot_table> tables_ {{0}};
};

} //namespace tez
