#include "languages.hpp"

#include <bklib/types.hpp>
#include <bklib/macros.hpp>
#include <bklib/util.hpp>
#include <bklib/json.hpp>

using bklib::utf8string;
using tez::language_id;
using tez::language_info;
//using tez::language_map;

utf8string const language_info::FILE_NAME = R"(./data/language.def)";

////////////////////////////////////////////////////////////////////////////////
namespace {

std::once_flag once_flag;

utf8string lang_fallback_string   = {"en"};
utf8string lang_default_string    = lang_fallback_string;
utf8string lang_substitute_string = {"<substitute>"};

language_id lang_fallback_id = 1;
language_id lang_default_id  = 1;

boost::container::flat_map<size_t, language_info::info> lang_info;

static utf8string const FIELD_FILE_ID    = {"file_id"};
static utf8string const FIELD_DEFAULT    = {"default"};
static utf8string const FIELD_SUBSTITUTE = {"substitute"};
static utf8string const FIELD_FALLBACK   = {"fallback"};
static utf8string const FIELD_LANGUAGE   = {"language"};


void init() {
}

} //namespace

language_info::info const& language_info::get_info(
    hash const lang
) {
    static info const NOT_FOUND {
        0, "", ""
    };

    std::call_once(once_flag, init);

    auto it = lang_info.find(lang);
    if (it != lang_info.end()) {
        return it->second;
    }

    return NOT_FOUND;
}

language_info::info const& language_info::get_info(
    utf8string const& lang
) {
    return get_info(bklib::utf8string_hash(lang));
}

bool language_info::is_defined(hash const lang) {
    std::call_once(once_flag, init);

    return lang_info.find(lang) != lang_info.end();
}

language_id language_info::fallback() { return lang_fallback_id; }
language_id language_info::default()  { return lang_default_id; }
utf8string  language_info::substitute()  { return lang_substitute_string; }

////////////////////////////////////////////////////////////////////////////////
// tez::language_string_parser
////////////////////////////////////////////////////////////////////////////////
namespace json = bklib::json;
using tez::language_string_parser;

namespace {

size_t const INDEX_LANG_ID     = 0;
size_t const INDEX_LANG_STRING = 1;
size_t const SIZE_LANG_STRING = 2;

} //namespace

void language_string_parser::rule_root(cref json_value) {
    try {
        rule_lang_string_list_(json_value);
    } catch (json::error::base& e) {
        BK_JSON_ADD_TRACE(e);
    }
}

void language_string_parser::rule_lang_string_list_(cref json_value) {
    try {
        json::require_array(json_value);

        json::for_each_element_skip_on_fail(json_value, [&](cref lang_string) {
            rule_lang_string_(lang_string);

            //add the pair to the map
            map_.insert(std::move(lang_id_), std::move(string_));
        });
    } catch (json::error::base& e) {
        BK_JSON_ADD_TRACE(e);
    }
}

void language_string_parser::rule_lang_string_(cref json_value) {
    try {
        json::require_array(json_value);

        auto const size = json_value.size();
        if (size < SIZE_LANG_STRING) {
            BK_DEBUG_BREAK(); // TODO
        } else if (size > SIZE_LANG_STRING) {
            BK_DEBUG_BREAK(); // TODO
        }

        auto string_id    = json::require_key(json_value, INDEX_LANG_ID);
        auto string_value = json::require_key(json_value, INDEX_LANG_STRING);

        rule_lang_string_id_(string_id);
        rule_lang_string_value_(string_value);
    } catch (json::error::base& e) {
        BK_JSON_ADD_TRACE(e);
    }
}

void language_string_parser::rule_lang_string_id_(cref json_value) {
    try {
        lang_id_ = json::require_string(json_value);
    } catch (json::error::base& e) {
        BK_JSON_ADD_TRACE(e);
    }
}

void language_string_parser::rule_lang_string_value_(cref json_value) {
    try {
        string_ = json::require_string(json_value);
    } catch (json::error::base& e) {
        BK_JSON_ADD_TRACE(e);
    }
}
