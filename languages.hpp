#pragma once

#include <boost/container/flat_map.hpp>

#include <bklib/json_forward.hpp>

#include "types.hpp"
#include "util.hpp"

namespace tez {

using language_id = uint8_t;

BK_CONSTEXPR static language_id const INVALID_LANG_ID {0};

//==============================================================================
//!
//==============================================================================
struct language_info {
    using hash = size_t;

    static utf8string const FILE_NAME;

    //! tuple<id, string_id, name>
    using info = std::tuple<
        language_id, utf8string, utf8string
    >;

    static info const& get_info(hash lang);
    static info const& get_info(utf8string const& lang);

    static bool is_defined(hash lang);
    static bool is_defined(utf8string const& lang);

    static language_id fallback();
    static language_id default();
    static utf8string  substitute();
};
//==============================================================================
// ROOT -> {
//    SUBSTITUTE
//  , FALLBACK
//  , DEFAULT
//  , LANGUAGE_LIST
// }
// SUBSTITUTE -> string
// FALLBACK -> string
// DEFAULT -> string
// LANGUAGE_LIST -> [LANGUAGE*]
// LANGUAGE -> [LANGUAGE_NAME, LANGUAGE_STRING]
// LANGUAGE_NAME -> string
// LANGUAGE_STRING -> string
//==============================================================================
class languages_parser {
public:
    using cref = bklib::json::cref;

    void rule_root(cref json_root);
    void rule_substitute(cref json_substitute);
    void rule_fallback(cref json_fallback);
    void rule_default(cref json_default);
    void rule_language_list(cref json_language_list);
    void rule_language(cref json_language);
    void rule_language_name(cref json_language_name);
    void rule_language_string(cref json_language_string);
private:
};

////////////////////////////////////////////////////////////////////////////////
// language_string_map
////////////////////////////////////////////////////////////////////////////////
namespace detail {
    struct tag_language;
} //namespace detail

using language_ref = tagged_value<hash_t, detail::tag_language>;

//==============================================================================
//!
//==============================================================================
class language_string_map {
public:
    using key_t       = language_ref;
    using value_t     = utf8string;
    using container_t = boost::container::flat_map<key_t, value_t>;

    template <typename LangIter, typename StringIter>
    void insert(LangIter first_l, LangIter last_l, StringIter first_s, StringIter last_s) {
        strings_.reserve(last_l  - first_l);

        zip(first_l, last_l, first_s, last_s, [&](utf8string const& lang_id, utf8string& string) {
            insert(lang_id, std::move(string));
        });   
    }

    void insert(utf8string const& id, utf8string&& value) {
        language_ref const ref {bklib::utf8string_hash(id)};
        auto const result = strings_.emplace(ref, std::move(value));
        if (!result.second) {
            BK_DEBUG_BREAK();
        }
    }

    string_ref get(language_ref language) const {
        static char const FAIL_STRING[] = {"{undefined}"};

        auto const where = strings_.find(language);

        if (where == std::cend(strings_)) {
            return {FAIL_STRING};
        }

        return {where->second};
    }
private:
    container_t strings_;
};

//==============================================================================
//! Json parser for language string lists.
//
// ROOT              = LANG_STRING_LIST
// LANG_STRING_LIST  = [LANG_STRING*]
// LANG_STRING       = [LANG_STRING_ID, LANG_STRING_VALUE]
// LANG_STRING_ID    = ascii_string
// LANG_STRING_VALUE = utf8string
//==============================================================================
class language_string_parser : public parser_base<language_string_parser> {
    friend parser_base<language_string_parser>;
public:
    using parser_base::parser_base;
    using parser_base::parse;

    language_string_map&& get() {
        return std::move(map_);
    }
private:
    void rule_root(cref json_value);
    void rule_lang_string_list_(cref json_value);
    void rule_lang_string_(cref json_value);
    void rule_lang_string_id_(cref json_value);
    void rule_lang_string_value_(cref json_value);

    utf8string lang_id_;
    utf8string string_;

    language_string_map map_;
};

} //namespace tez
