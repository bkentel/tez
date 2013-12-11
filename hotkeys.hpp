#pragma once

#include <bklib/keyboard.hpp>
#include <bklib/json_forward.hpp>
#include <bklib/types.hpp>
#include "commands.hpp"

namespace tez {

using bklib::utf8string;

//==============================================================================
// ROOT         -> {"bindings" : BINDING_LIST}
// BINDING_LIST -> [BINDING*]
// BINDING      -> [COMMAND, COMBO_LIST]
// COMMAND      -> string
// COMBO_LIST   -> [COMBO*]
// COMBO        -> [KEY+]
// KEY          -> string
//==============================================================================
class bindings_parser {
public:
    using map_t = boost::container::flat_map<bklib::key_combo, command_type>;
    using cref  = bklib::json::cref;

    bindings_parser(utf8string const& file_name);
    bindings_parser(std::istream&& in);
    bindings_parser(std::istream& in);

    void             rule_root(         cref json_root);
    void             rule_binding_list( cref json_binding_list);
    void             rule_binding(      cref json_binding);
    command_type     rule_command(      cref json_command);
    void             rule_combo_list(   cref json_combo_list);
    bklib::key_combo rule_combo(        cref json_combo);
    bklib::keycode   rule_key(          cref json_key);

    operator map_t&&() && {
        return std::move(bindings_);
    }
private:
    command_type     cur_command_;
    bklib::key_combo cur_combo_;
    map_t            bindings_;
};

//==============================================================================
//TODO
//WARNING
//not thread safe
//==============================================================================
struct hotkeys {
    using key_combo  = bklib::key_combo;
    using utf8string = bklib::utf8string;

    static utf8string const DEFAULT_FILE_NAME;

    static void reload(utf8string const& file = DEFAULT_FILE_NAME);
    static void reload(std::istream& in);

    static command_type translate(key_combo const& combo);
    static command_type translate(utf8string const& name);
    static command_type translate(bklib::hash_t name);
};

} //namespace tez

