#include "pch.hpp"
#include "commands.hpp"
#include "util.hpp"

using boost::container::flat_map;
using bklib::string_ref;
using bklib::utf8string;
using bklib::hash;
using tez::command_type;

namespace {
//------------------------------------------------------------------------------
utf8string   const NOT_FOUND_STRING  {"invalid"};
command_type const NOT_FOUND_COMMAND {command_type::NONE};
//------------------------------------------------------------------------------
namespace local_state {
    //--------------------------------------------------------------------------
    std::once_flag init_flag;

    auto const size = static_cast<size_t>(command_type::SIZE);

    std::array<string_ref, size> command_strings;
    flat_map<hash, command_type> hash_to_command_map;
    //--------------------------------------------------------------------------
    template <size_t N>
    static void add_string(command_type const type, char const (&name)[N]) {
        auto const i = static_cast<size_t>(type);
        command_strings[i] = bklib::string_ref {name, N};
    }
    //--------------------------------------------------------------------------
    static void add_hash(command_type const type, bklib::hash const hash) {
        auto result = hash_to_command_map.insert(std::make_pair(hash, type));
        BK_ASSERT(result.second && "hash collision");
    }
    //--------------------------------------------------------------------------
    static void do_init() {
        BOOST_LOG_TRIVIAL(trace) << "initializing command strings.";

        BK_ASSERT(hash_to_command_map.empty());

        hash_to_command_map.reserve(size);

        using COMMAND = command_type;
        //######################################################################
        #define BK_ADD_COMMAND_STRING(CMD)\
        add_string(CMD, #CMD);\
        add_hash(CMD, bklib::utf8string_hash(#CMD))
        //######################################################################
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
        //######################################################################
        #undef BK_ADD_COMMAND_STRING
        //######################################################################
    }   
    //--------------------------------------------------------------------------
    static void init() {
        std::call_once(init_flag, do_init);
    }
    //--------------------------------------------------------------------------
} //namespace local_state

} //namespace

bklib::string_ref tez::to_string(command_type const type) {
    local_state::init();
    auto const i = static_cast<size_t>(type);   
    return local_state::command_strings[i];
}

command_type tez::to_command(bklib::utf8string const& string) {
    return tez::to_command(bklib::utf8string_hash(string));
}

command_type tez::to_command(bklib::hash hash) {
    local_state::init();

    return bklib::find_or(
        local_state::hash_to_command_map
      , hash
      , NOT_FOUND_COMMAND
    );
}
