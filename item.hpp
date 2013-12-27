#pragma once

#include <boost/container/flat_set.hpp>

#include <bklib/json_forward.hpp>

#include "util.hpp"
#include "types.hpp"
#include "languages.hpp"

namespace tez {

namespace detail {
    struct tag_item;
    struct tag_item_category;
    struct tag_item_type;
    struct tag_item_tag;
    struct tag_item_attribute;
} //namespace detail

using item_ref           = tagged_value<hash_t, detail::tag_item>;
using item_category_ref  = tagged_value<hash_t, detail::tag_item_category>;
using item_type_ref      = tagged_value<hash_t, detail::tag_item_type>;
using item_tag_ref       = tagged_value<hash_t, detail::tag_item_tag>;
using item_attribute_ref = tagged_value<hash_t, detail::tag_item_attribute>;

////////////////////////////////////////////////////////////////////////////////
// item_attribute
////////////////////////////////////////////////////////////////////////////////
class item_attribute {
public:
    enum class type {
        integer, string, floating_point
    };

    item_attribute(item_attribute_ref name, int32_t int_val)
      : int_val_ {int_val}
      , name_    {name}
      , type_    {type::integer}
    {
    }

    item_attribute(item_attribute_ref name, float float_val)
      : float_val_ {float_val}
      , name_      {name}
      , type_      {type::floating_point}
    {
    }

    item_attribute(item_attribute_ref name, utf8string const& str_val)
      : name_ {name}
      , type_ {type::string}
    {
        auto const count = std::min(sizeof(str_val_) - 1, str_val.size());
        std::strncpy(str_val_, str_val.c_str(), sizeof(str_val_));
        str_val_[sizeof(str_val_) - 1] = 0;
    }

    item_attribute_ref name() const BK_NOEXCEPT { return name_; }
private:
    union {
        char    str_val_[32];
        int32_t int_val_;
        float   float_val_;
    };

    item_attribute_ref name_;
    type               type_;
};

inline bool operator<(item_attribute const& lhs, item_attribute const& rhs) {
    return lhs.name() < rhs.name();
}

#define BK_COPY_DELETE(name)\
name(name const&) = delete;\
name& operator=(name const&) = delete

#define BK_MOVE_DEFAULT(name)\
name(name&&) = default;\
name& operator=(name&&) = default


////////////////////////////////////////////////////////////////////////////////
// item_definition
////////////////////////////////////////////////////////////////////////////////
struct item_definition {
    using hash_t = bklib::hash_t;

    template <typename T>
    using set_t = boost::container::flat_set<T>;

    item_definition() = default;

    BK_COPY_DELETE(item_definition);
    BK_MOVE_DEFAULT(item_definition);

    hashed_string         id;
    item_category_ref     category;
    item_type_ref         type;
    unsigned              weight;
    unsigned              base_value;
    set_t<item_tag_ref>   tags;
    set_t<item_attribute> attributes;
    language_string_map   names;
    language_string_map   descriptions;
};

////////////////////////////////////////////////////////////////////////////////
// item_parser
////////////////////////////////////////////////////////////////////////////////
class item_parser : public parser_base<item_parser> {
    friend parser_base<item_parser>;
public:
    using parser_base::parser_base;
    using parser_base::parse;

    void rule_root(cref json_value);
    void rule_item_list(cref json_value);
    void rule_item(cref json_value);
    void rule_item_id(cref json_value);
    void rule_item_category(cref json_value);
    void rule_item_type(cref json_value);
    void rule_item_tag_list(cref json_value);
    void rule_item_tag(cref json_value);
    void rule_item_weight(cref json_value);
    void rule_item_base_value(cref json_value);
    void rule_item_attr_list(cref json_value);
    void rule_item_attr(cref json_value);
    utf8string rule_attr_name(cref json_value);
    item_attribute rule_attr_value(cref json_value, item_attribute_ref name);
    void rule_item_name(cref json_value);
    void rule_item_desc(cref json_value);

    using map_t = boost::container::flat_map<item_ref, item_definition>;

    map_t&& get() {
        return std::move(items_);
    }

    item_definition item_;
    map_t           items_;
};

////////////////////////////////////////////////////////////////////////////////
// item functions
////////////////////////////////////////////////////////////////////////////////
namespace detail {
    template <> struct tag_traits<tag_item> {
        using type    = item_definition;
        using pointer = item_definition*;
        using ref     = item_ref;
        using parser  = item_parser;
    };
} //namespace detail

extern template data_table<detail::tag_item>;
using item_table = data_table<detail::tag_item>;

inline string_ref ref_to_id(item_ref ref) {
    auto const item = item_table::get(ref);

    if (item) return {item->id.string};
    else      return {""};
}

} //namespace tez
