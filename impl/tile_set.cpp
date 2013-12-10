#include "pch.hpp"
#include "tile_set.hpp"

using namespace tez::data;
using bklib::utf8string;

namespace {
    static utf8string const FIELD_NAME        = {"name"};
    static utf8string const FIELD_DESCRIPTION = {"description"};
    static utf8string const FIELD_LOCATION    = {"location"};
    static utf8string const FIELD_WEIGHT      = {"weight"};
    static utf8string const FIELD_FILE_ID     = {"file_id"};
    static utf8string const FIELD_TILE_SIZE   = {"tile_size"};
    static utf8string const FIELD_FILE_NAME   = {"file_name"};
    static utf8string const FIELD_COLOR_KEY   = {"color_key"};
    static utf8string const FIELD_TILES       = {"tiles"};

    //tile_variation::location_t required_location(json::cref value, json::field_t field) {
    //    auto array = json::required_array(value, field, 2, 2);
    //    auto const x = json::required_integer(array, 0);
    //    auto const y = json::required_integer(array, 1);
    //    return {x, y};
    //}
}

//tile_variation::tile_variation(json::cref value)
//  : name{json::required_array(value, FIELD_NAME)}
//  , description{json::required_array(value, FIELD_DESCRIPTION)}
//  , location{required_location(value, FIELD_LOCATION)}
//  , weight{json::required_integer(value, FIELD_WEIGHT)}
//{
//    
//
//    auto val_name   = json::required_string( value, FIELD_NAME);
//    auto val_desc   = json::required_string( value, FIELD_DESCRIPTION);
//    auto val_loc    = json::required_array(  value, FIELD_LOCATION, 2, 2);
//    auto val_weight = json::required_integer(value, FIELD_WEIGHT);
//
//    
//}

//tile::tile(Json::Value const& json) {
//    using namespace bklib::json;
//
//    static utf8string const FIELD_NAME        = {"name"};
//    static utf8string const FIELD_DESCRIPTION = {"description"};
//    static utf8string const FIELD_LOCATION    = {"location"};
//    static utf8string const FIELD_WEIGHT      = {"weight"};
//
//    required_object(json);
//
//    cref var_name   = json[FIELD_NAME];
//    cref var_desc   = json[FIELD_DESCRIPTION];
//    cref var_loc    = json[FIELD_LOCATION];
//    cref var_weight = json[FIELD_WEIGHT];
//
//    name        = tez::language_map(var_name);
//    description = tez::language_map(var_desc);
//
//    required_array(var_loc, 2, 2);
//    location.x = required_integer(var_loc[0]);
//    location.y = required_integer(var_loc[1]);
//
//    weight = required_integer(var_weight);
//}
//
////////////////////////////
//tile_group::tile_group(Json::Value const& json) {
//    using namespace bklib::json;
//
//    static utf8string const FIELD_ID        = {"id"};
//    static utf8string const FIELD_VARIATION = {"variation"};
//
//    required_object(json);
//
//    for (cref var : required_array(json[FIELD_VARIATION])) {
//        tiles.emplace_back(tile{var});
//    }
//
//    id = required_string(json[FIELD_ID]);
//}
//
//
////////////////////////////////
//
//tile_set::tile_set() {
//    using namespace bklib::json;
//
//    static utf8string const FIELD_FILE_ID   = {"file_id"};
//    static utf8string const FIELD_TILE_SIZE = {"tile_size"};
//    static utf8string const FIELD_FILE_NAME = {"file_name"};
//    static utf8string const FIELD_COLOR_KEY = {"color_key"};
//    static utf8string const FIELD_TILES     = {"tiles"};
//
//    Json::Value  json_root;
//    Json::Reader json_reader;
//    
//    auto json_in = std::ifstream{R"(./data/tiles.def)"};
//    if (!json_in) {
//        //failed to open the file
//        BK_DEBUG_BREAK();
//    }
//
//    if (!json_reader.parse(json_in, json_root)) {
//        //failed to parse the file
//        BK_DEBUG_BREAK();
//        std::cout << json_reader.getFormattedErrorMessages();
//    }
//
//    required_object(json_root);
//
//    if (required_string(json_root[FIELD_FILE_ID]) != FIELD_TILES) {
//        //bad file id
//        BK_DEBUG_BREAK();
//    }
//
//    size = required_integer<size_t>(json_root[FIELD_TILE_SIZE]);
//    file_name = required_string(json_root[FIELD_FILE_NAME]);
//
//    cref val_color_key = required_array(json_root[FIELD_COLOR_KEY], 3, 3);
//    required_integer<uint8_t>(val_color_key[0]);
//    required_integer<uint8_t>(val_color_key[1]);
//    required_integer<uint8_t>(val_color_key[2]);
//
//    auto hasher = tez::string_hasher{};
//    for (cref val_group : required_array(json_root[FIELD_TILES])) {
//        auto group = tile_group{val_group};
//        auto const hash = hasher(group.id);
//
//        tiles.emplace(hash, std::move(group));
//    }
//}
