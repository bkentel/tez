#include "hotkeys.hpp"

#include <bklib/json.hpp>

namespace json = bklib::json;

using tez::key_bindings;
using tez::bindings_parser;
using tez::command_t;
using tez::string_ref;
using tez::key_combo;
using tez::keycode;
using tez::make_string_ref;

////////////////////////////////////////////////////////////////////////////////
// tez::key_bindings
////////////////////////////////////////////////////////////////////////////////
string_ref const key_bindings::DEFAULT_FILE_NAME = make_string_ref("./data/bindings.def");

key_bindings::key_bindings(string_ref file) {
    reload(file);
}

void key_bindings::reload(string_ref file) {
    std::ifstream in {file.to_string()};
    reload(in);
}

void key_bindings::reload(std::istream& in) {
    bindings_parser parser;
    parser.parse(in);
    mappings_ = parser.get();
}

command_t key_bindings::match(key_combo const& keys) const {
    auto const result = mappings_.find(keys);
    return result != std::cend(mappings_)
      ? result->second
      : command_t::NOT_FOUND
    ;
}
////////////////////////////////////////////////////////////////////////////////
// tez::bindings_parser
////////////////////////////////////////////////////////////////////////////////
namespace {
    auto const KEY_BINDINGS = make_string_ref("bindings");

    size_t const INDEX_COMMAND_NAME = 0;
    size_t const INDEX_BINDING_LIST = 1;

    size_t const SIZE_BINDING = 2;
} //namespace

bindings_parser::bindings_parser()
  : cur_key_{keycode::NONE}
  , cur_command_{command_t::NONE}
  , cur_combo_{}
  , bindings_{}
{
}

//------------------------------------------------------------------------------
// ROOT
//   {"bindings": KEY_BINDING_LIST}
//------------------------------------------------------------------------------
void bindings_parser::rule_root(cref json_root) {
    try {
        json::require_object(json_root);
        cref key_binding_list = json::require_key(json_root, KEY_BINDINGS);

        rule_key_binding_list(key_binding_list);
    } catch (json::error::base& e) {
        BK_JSON_ADD_TRACE(e);
    }
}
//------------------------------------------------------------------------------
// KEY_BINDING_LIST
//   [KEY_BINDING*]
//------------------------------------------------------------------------------
void bindings_parser::rule_key_binding_list(cref json_value) {
    try {
        json::require_array(json_value);

        json::for_each_element_skip_on_fail(json_value, [&](cref key_binding) {
            rule_key_binding(key_binding);
        });
    } catch (json::error::base& e) {
        BK_JSON_ADD_TRACE(e);
    }
}
//------------------------------------------------------------------------------
// KEY_BINDING
//   [COMMAND_NAME, BINDING_LIST]
//------------------------------------------------------------------------------
void bindings_parser::rule_key_binding(cref json_value) {
    try {
        json::require_array(json_value);
        json::require_size(json_value, SIZE_BINDING);

        cref command_name = json::require_key(json_value, INDEX_COMMAND_NAME);
        cref binding_list = json::require_key(json_value, INDEX_BINDING_LIST);

        rule_command_name(command_name);
        rule_binding_list(binding_list);
    } catch (json::error::base& e) {
        BK_JSON_ADD_TRACE(e);
    }
}
//------------------------------------------------------------------------------
// COMMAND_NAME
//   string
//------------------------------------------------------------------------------
void bindings_parser::rule_command_name(cref json_value) {
    try {
        auto const command_string = json::require_string(json_value);
        cur_command_ = command::translate(command_string);

        if (cur_command_ == command_t::NOT_FOUND) {
            BOOST_THROW_EXCEPTION(json::error::bad_value{}
             << json::error::info_value{command_string}
            );
        }
    } catch (json::error::base& e) {
        BK_JSON_ADD_TRACE(e);
    }
}
//------------------------------------------------------------------------------
// BINDING_LIST
//   [BINDING*]
//------------------------------------------------------------------------------
void bindings_parser::rule_binding_list(cref json_value) {
    try {
        json::require_array(json_value);

        json::for_each_element_skip_on_fail(json_value, [&](cref binding) {
            rule_binding(binding);
        });
    } catch (json::error::base& e) {
        BK_JSON_ADD_TRACE(e);
    }
}
//------------------------------------------------------------------------------
// BINDING
//   [KEY*]
//------------------------------------------------------------------------------
void bindings_parser::rule_binding(cref json_value) {
    try {
        json::require_array(json_value);

        cur_combo_.clear();

        json::for_each_element_skip_on_fail(json_value, [&](cref key) {
            rule_key(key);
            cur_combo_.add(cur_key_);
        });

        if (cur_combo_.empty()) {
            BK_DEBUG_BREAK(); //TOOO
            return;
        }

        auto const result = bindings_.emplace(
            std::make_pair(std::move(cur_combo_), cur_command_)
        );

        if (!result.second) {
            BK_DEBUG_BREAK(); //TOOO
        }
    } catch (json::error::base& e) {
        BK_JSON_ADD_TRACE(e);
    }
}
//------------------------------------------------------------------------------
// KEY
//   string
//------------------------------------------------------------------------------
void bindings_parser::rule_key(cref json_key) {
    try {
        auto const key_string = json::require_string(json_key);
        cur_key_ = bklib::keyboard::translate(key_string);

        if (cur_key_ == keycode::NONE) {
            BOOST_THROW_EXCEPTION(json::error::bad_value{}
             << json::error::info_value{key_string}
            );
        }
    } catch (json::error::base& e) {
        BK_JSON_ADD_TRACE(e);
    }
}
//------------------------------------------------------------------------------
