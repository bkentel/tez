#pragma once

#include <string>
#include <boost/container/flat_map.hpp>
#include "types.hpp"
#include "json.hpp"

namespace tez {

using bklib::utf8string;
using bklib::string_hasher;

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
//==============================================================================
//!
//==============================================================================
class language_map {
public:
    using hash = language_info::hash;

    explicit language_map(size_t size = 0);
    explicit language_map(Json::Value const& json);

    language_map(language_map&&) = default;
    language_map& operator=(language_map&&) = default;

    language_map(language_map const&) = delete;
    language_map& operator=(language_map const&) = delete;

    void swap(language_map& other) {
        using std::swap;
        swap(values_, other.values_);
    }

    utf8string const& operator[](language_id id) const;
    void insert(language_id id, utf8string value);
private:
    using map = boost::container::flat_map<language_id, utf8string>;

    map values_;
};

} //namespace tez
