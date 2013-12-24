#include "hotkeys.hpp"
#include <bklib/json.hpp>

using bklib::utf8string;
using bklib::hash_t;
using bklib::key_combo;
using tez::command_type;
using tez::hotkeys;

namespace json = bklib::json;
namespace error = json::error;

////////////////////////////////////////////////////////////////////////////////
utf8string const tez::hotkeys::DEFAULT_FILE_NAME = {"./data/bindings.def"};
//------------------------------------------------------------------------------
// private local state for tez::hotkeys
//------------------------------------------------------------------------------
namespace {
namespace local_state {
////////////////////////////////////////////////////////////////////////////////
template <typename K, typename V>
using flat_map = boost::container::flat_map<K, V>;

bool initialized = false;

flat_map<key_combo, command_type> combo_to_command;
flat_map<hash_t,    command_type> hash_to_command;
////////////////////////////////////////////////////////////////////////////////

void init() {
    if (initialized) return;
    initialized = true;

    hotkeys::reload();
}

} //namespace local_state
} //namespace
//------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////
// tez::hotkeys
////////////////////////////////////////////////////////////////////////////////
//------------------------------------------------------------------------------
command_type hotkeys::translate(key_combo const& combo) {
    if (!local_state::initialized) local_state::init();
    return bklib::find_or(local_state::combo_to_command, combo, command_type::NONE);
}
//------------------------------------------------------------------------------
command_type hotkeys::translate(bklib::hash_t name) {
    if (!local_state::initialized) local_state::init();
    return bklib::find_or(local_state::hash_to_command, name, command_type::NONE);
}
//------------------------------------------------------------------------------
command_type hotkeys::translate(utf8string const& name) {
    return translate(bklib::utf8string_hash(name));
}
//------------------------------------------------------------------------------
void hotkeys::reload(utf8string const& file) {
    std::ifstream in {file};
    reload(in);
}
//------------------------------------------------------------------------------
void hotkeys::reload(std::istream& in) {
    if (!in) {
        BK_DEBUG_BREAK(); //TODO
    }

    bindings_parser parser {in};

    local_state::combo_to_command.clear();
    local_state::combo_to_command = std::move(parser);
}
//------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////
// tez::bindings_parser
////////////////////////////////////////////////////////////////////////////////
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
tez::bindings_parser::bindings_parser(utf8string const& file_name)
  : bindings_parser {std::ifstream {file_name}}
{    
}
//------------------------------------------------------------------------------
tez::bindings_parser::bindings_parser(std::istream& in)
  : bindings_parser {std::move(in)}
{
}
//------------------------------------------------------------------------------
tez::bindings_parser::bindings_parser(std::istream&& in) {
    if (!in) {
        BK_DEBUG_BREAK(); //TODO
    }

    Json::Reader reader;
    Json::Value  root;

    auto const result = reader.parse(in, root);
    if (!result) {
        BK_DEBUG_BREAK(); //TODO
    }

    rule_root(root);
}
//------------------------------------------------------------------------------
void tez::bindings_parser::rule_root(cref json_root) {
    try {
        static utf8string const root_key {"bindings"};

        json::require_object(json_root);
        cref json_binding_list = json::require_key(json_root, root_key);

        rule_binding_list(json_binding_list);
    } catch (json::error::base& e) {
        BK_JSON_ADD_TRACE(e);
    }
}
//------------------------------------------------------------------------------
void tez::bindings_parser::rule_binding_list(cref json_binding_list) {
    try {
        json::require_array(json_binding_list);
    
        json::for_each_element_skip_on_fail(json_binding_list, [&](cref json_binding) {
            rule_binding(json_binding);
        });
    } catch (json::error::base& e) {
        BK_JSON_ADD_TRACE(e);
    }
}
//------------------------------------------------------------------------------
void tez::bindings_parser::rule_binding(cref json_binding) {
    try {
        json::require_array(json_binding);

        auto const size = json_binding.size();
        if (size < 2) {
            BK_DEBUG_BREAK(); //TODO
        } else if (size > 2) {
            BK_DEBUG_BREAK(); //TODO
        }

        cref json_command    = json::require_key(json_binding, 0);
        cref json_combo_list = json::require_key(json_binding, 1);

        cur_command_ = rule_command(json_command);
        if (cur_command_ == command_type::NONE) {
            BK_DEBUG_BREAK();
            return; //TODO
        }

        rule_combo_list(json_combo_list);
    } catch (json::error::base& e) {
        BK_JSON_ADD_TRACE(e);
    }
}
//------------------------------------------------------------------------------
tez::command_type tez::bindings_parser::rule_command(cref json_command) {
    try {
        auto const command_string = json::require_string(json_command);
        auto const command = to_command(command_string);;

        return command;
    } catch (json::error::base& e) {
        BK_JSON_ADD_TRACE(e);
    }
}
//------------------------------------------------------------------------------
void tez::bindings_parser::rule_combo_list(cref json_combo_list) {
    try {
        json::require_array(json_combo_list);

        json::for_each_element_skip_on_fail(json_combo_list, [&](cref json_combo) {
            cur_combo_.clear();
            cur_combo_ = rule_combo(json_combo);
            if (cur_combo_.empty()) {
                BK_DEBUG_BREAK(); //TOOO
            }

            auto const result = bindings_.emplace(
                std::make_pair(std::move(cur_combo_), cur_command_)
            );

            if (!result.second) {
                BK_DEBUG_BREAK(); //TOOO
            }
        });
    } catch (json::error::base& e) {
        BK_JSON_ADD_TRACE(e);
    }
}
//------------------------------------------------------------------------------
bklib::key_combo tez::bindings_parser::rule_combo(cref json_combo) {
    try {
        json::require_array(json_combo);

        key_combo result;

        json::for_each_element_skip_on_fail(json_combo, [&](cref json_key) {
            auto const key = rule_key(json_key);
            if (key == bklib::keycode::NONE) {
                BK_DEBUG_BREAK(); //TOOO
            }

            auto const added = result.add(key);
            if (!added) {
                BK_DEBUG_BREAK(); //TOOO
            }
        });

        return result;
    } catch (json::error::base& e) {
        BK_JSON_ADD_TRACE(e);
    }
}
//------------------------------------------------------------------------------
bklib::keycode tez::bindings_parser::rule_key(cref json_key) {
    try {
        auto const key_string = json::require_string(json_key);
        auto const key = bklib::keyboard::key_code(key_string);

        return key;
    } catch (json::error::base& e) {
        BK_JSON_ADD_TRACE(e);
    }
}
//------------------------------------------------------------------------------
