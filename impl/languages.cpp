#include "pch.hpp"
#include "languages.hpp"

using bklib::utf8string;
using tez::language_id;
using tez::language_info;
using tez::language_map;

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
    return get_info(string_hasher{}(lang));
}

bool language_info::is_defined(hash const lang) {
    std::call_once(once_flag, init);

    return lang_info.find(lang) != lang_info.end();
}

language_id language_info::fallback() { return lang_fallback_id; }
language_id language_info::default()  { return lang_default_id; }
utf8string  language_info::substitute()  { return lang_substitute_string; }

////////////////////////////////////////////////////////////////////////////////
language_map::language_map(size_t const size)
{
    if (size) {
        values_.reserve(size);
    }
}

language_map::language_map(Json::Value const& json)
{

}

utf8string const& language_map::operator[](language_id const id) const {
    auto it = values_.find(id);
    if (it != values_.end()) {
        return it->second;
    }

    std::cout << "Warning: requested language string not found.";

    it = values_.find(language_info::fallback());
    if (it != values_.end()) {
        return it->second;
    }

    std::cout << "Warning: fallback language string not found.";

    static auto const substitute = language_info::substitute();
    return substitute;
}

void language_map::insert(language_id const id, utf8string value) {
    auto result = values_.emplace(id, std::move(value));

    if (!result.second) {
        BK_DEBUG_BREAK();
        std::cout << "Warning: duplicate language.";
    }
}
