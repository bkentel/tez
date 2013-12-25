#pragma once

#include "util.hpp"
#include "types.hpp"

namespace tez {

namespace detail {
    struct tag_item_category;
    struct tag_item_type;
    struct tag_item_tag;
    struct tag_item_attribute;
} //namespace detail

using item_category_ref  = tagged_value<hash_t, detail::tag_item_category>;
using item_type_ref      = tagged_value<hash_t, detail::tag_item_type>;
using item_tag_ref       = tagged_value<hash_t, detail::tag_item_tag>;
using item_attribute_ref = tagged_value<hash_t, detail::tag_item_attribute>;

struct item_attribute {
    
};

struct item_definition {
    using hash_t = bklib::hash_t;

    template <typename T>
    using set_t = boost::container::flat_set<T>;

    hashed_string             id;
    item_category_ref         category;
    item_type_ref             type;
    unsigned                  value;
    set_t<item_tag_ref>       tags;
    set_t<item_attribute_ref> attributes;
    set_t<hashed_string>      descriptions;
};

namespace detail {
    struct tag_language;
} //namespace detail

using language_ref = tagged_value<hash_t, detail::tag_language>;

class language_string_map {
public:
    using key_t       = language_ref;
    using value_t     = utf8string;
    using container_t = boost::container::flat_map<key_t, value_t>;

    template <typename T1, typename T2, typename F>
    static void zip(T1 first1, T1 last1, T2 first2, T2 last2, F function) {
        while (first1 != last1 && first2 != last2) {
            function(*first1++, *first2++);
        }
    }

    template <typename LangIter, typename StringIter>
    void insert(LangIter first_l, LangIter last_l, StringIter first_s, StringIter last_s) {
        strings_.reserve(last_l  - first_l);

        zip(first_l, last_l, first_s, last_s, [&](utf8string const& lang_id, utf8string& string) {
            language_ref const ref {bklib::utf8string_hash(lang_id)};
            auto const result = strings_.emplace(ref, std::move(string));
            if (!result.second) {
                BK_DEBUG_BREAK();
            }
        });   
    }
private:
    container_t strings_;
};

struct language_parser : public parser_base {
};

struct item_parser : public parser_base {
    using parser_base::parser_base;

    void parse(cref json_value) { rule_root(json_value); }
    void parse() { parse(json_root_); }

    void rule_root(cref json_value);
    void rule_item_list(cref json_value);
    void rule_item(cref json_value);
    void rule_item_id(cref json_value);
    void rule_item_category(cref json_value);
    void rule_item_type(cref json_value);
    void rule_item_tag_list(cref json_value);
    void rule_item_tag(cref json_value);
    void rule_item_base_value(cref json_value);
    void rule_item_attr_list(cref json_value);
    void rule_item_attr(cref json_value);
    void rule_attr_name(cref json_value);
    void rule_attr_value(cref json_value);
    void rule_item_name(cref json_value);
    void rule_item_desc(cref json_value);
    void rule_lang_string_list(cref json_value);
    void rule_lang_string(cref json_value);
    void rule_lang_string_id(cref json_value);
    void rule_lang_string_value(cref json_value);

    item_definition item_;
};

} //namespace tez
