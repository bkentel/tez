#include "commands.hpp"

#include <bklib/util.hpp>

using tez::string_ref;
using tez::utf8string;
using tez::command_t;
using tez::command;
using tez::hash_t;

namespace {
namespace local_state {
//------------------------------------------------------------------------------
bool                                 initialized       {false};
tez::flat_map<hash_t, command_t>     hash_to_command   { };
tez::flat_map<command_t, string_ref> command_to_string { };
//------------------------------------------------------------------------------
void add_command(command_t const command, string_ref const string) {
    hash_t const hash = bklib::utf8string_hash(string);

    if (!hash_to_command.insert(std::make_pair(hash, command)).second) {
        BK_BREAK_AND_EXIT(); //TODO
    }

    if (!command_to_string.insert(std::make_pair(command, string)).second) {
        BK_BREAK_AND_EXIT(); //TODO
    }
}

void init() {
    if (initialized) {
        return;
    }

    auto const size = bklib::get_enum_value(command_t::SIZE);

    hash_to_command.reserve(size);
    command_to_string.reserve(size);

    using COMMAND = command_t;
    //##########################################################################
    #define BK_ADD_COMMAND_STRING(CMD) add_command(CMD, #CMD)
    //##########################################################################
    BK_ADD_COMMAND_STRING(COMMAND::USE);
    BK_ADD_COMMAND_STRING(COMMAND::DIR_NORTH_WEST);
    BK_ADD_COMMAND_STRING(COMMAND::DIR_NORTH);
    BK_ADD_COMMAND_STRING(COMMAND::DIR_NORTH_EAST);
    BK_ADD_COMMAND_STRING(COMMAND::DIR_WEST);
    BK_ADD_COMMAND_STRING(COMMAND::DIR_HERE);
    BK_ADD_COMMAND_STRING(COMMAND::DIR_EAST);
    BK_ADD_COMMAND_STRING(COMMAND::DIR_SOUTH_WEST);
    BK_ADD_COMMAND_STRING(COMMAND::DIR_SOUTH);
    BK_ADD_COMMAND_STRING(COMMAND::DIR_SOUTH_EAST);
    //##########################################################################
    #undef BK_ADD_COMMAND_STRING
    //##########################################################################

    initialized = true;
}
//------------------------------------------------------------------------------
} //namespace local_state
} //namespace

string_ref command::translate(command_t cmd) {
    local_state::init();

    auto const result = local_state::command_to_string.find(cmd);

    return (result != std::cend(local_state::command_to_string))
      ? result->second
      : string_ref {"invalid command_t"}
    ;
}

command_t command::translate(hash_t hash) {
    local_state::init();

    auto const result = local_state::hash_to_command.find(hash);

    return (result != std::cend(local_state::hash_to_command))
      ? result->second
      : command_t::NOT_FOUND
    ;
}

command_t command::translate(utf8string const& string) {
    auto const hash = bklib::utf8string_hash(string);
    return translate(hash);
}

command_t command::translate(string_ref string) {
    auto const hash = bklib::utf8string_hash(string);
    return translate(hash);
}
