#pragma once

#include <bklib/json.hpp>
#include <bklib/math.hpp>

#include "languages.hpp"
#include "tile_data.hpp"

namespace tez {
namespace data {

namespace json = ::bklib::json;

// ROOT ->
//    TILE_SIZE
//  , FILE_NAME
//  , COLOR_KEY
//  , TILES
// TILE_SIZE -> "tile_size": uint8_t
// FILE_NAME -> "file_name": string
// COLOR_KEY -> "color_key": {uint8_t, uint8_t, uint8_t}
// TILES -> "tiles": [TILE_LIST]
// TILE_LIST -> [TILE*]
// TILE -> [
//    THEME
//  , TYPE
//  , VARIATION
//  , LOCATION
//  , WEIGHT
//  , NAME
//  , DESCRIPTION
// ]
// THEME -> string
// TYPE -> string
// VARIATION -> string
// LOCATION -> [uint16_t, uint16_t]
// WEIGHT -> uint16_t
// NAME -> LANGUAGE_STRING_LIST
// DESCRIPTION -> LANGUAGE_STRING_LIST
// LANGUAGE_STRING_LIST -> [LANGUAGE_STRING*]
// LANGUAGE_STRING -> [string, string]

class tile_set_parser {
public:
    using cref = bklib::json::cref;

    void rule_root(cref json_root);
    void rule_tile_size(cref json_root);
    void rule_file_name(cref json_root);
    void rule_color_key(cref json_root);
    void rule_tiles(cref json_root);
    void rule_tile_list(cref json_root);
    void rule_tile(cref json_root);
    void rule_theme(cref json_root);
    void rule_type(cref json_root);
    void rule_variation(cref json_root);
    void rule_location(cref json_root);
    void rule_weight(cref json_root);
    void rule_name(cref json_root);
    void rule_description(cref json_root);

private:
};


//==============================================================================
//
//==============================================================================
struct tile_variation {
    using location_t = tez::tile_data::offset_t;

    explicit tile_variation(json::cref value);

    tile_variation(tile_variation const&) = delete;
    tile_variation(tile_variation&&) = default;
    tile_variation& operator=(tile_variation const&) = delete;
    tile_variation& operator=(tile_variation&&) = default;;

    language_string_map name;
    language_string_map description;
    location_t   location;
    unsigned     weight;
};
//==============================================================================
//
//==============================================================================
struct tile_type {
    explicit tile_type(json::cref);

    tile_type(tile_type const&) = delete;
    tile_type(tile_type&&) = default;
    tile_type& operator=(tile_type const&) = delete;
    tile_type& operator=(tile_type&&) = default;;

    std::vector<tile_variation> variations;
};
//==============================================================================
//
//==============================================================================
struct tile_set {
    explicit tile_set(json::cref);

    tile_set(tile_set const&) = delete;
    tile_set(tile_set&&) = default;
    tile_set& operator=(tile_set const&) = delete;
    tile_set& operator=(tile_set&&) = default;;

    size_t                 size;
    utf8string             file_name;
    std::vector<tile_type> tiles;
};
//
//struct tile {
//    using location_t = tez::tile_data::offset_t;
//
//    explicit tile(Json::Value const& json);
//
//    tile(tile&& other)
//      : name{std::move(other.name)}
//      , description{std::move(other.description)}
//      , location{other.location}
//      , weight{other.weight}
//    {
//    }
//
//    tile& operator=(tile&& rhs) {
//        swap(rhs);
//        return *this;
//    }
//
//    void swap(tile& other) {
//        using std::swap;
//        swap(name, other.name);
//        swap(description, other.description);
//        swap(location, other.location);
//        swap(weight, other.weight);
//    }
//
//    language_map name;
//    language_map description;
//    location_t   location;
//    unsigned     weight;
//};
////==============================================================================
////
////==============================================================================
//struct tile_group {
//    tile_group(tile_group&& other)
//      : id{other.id}, tiles{std::move(other.tiles)}
//    {
//    }
//
//    tile_group& operator=(tile_group&& rhs) {
//        swap(rhs);
//        return *this;
//    }
//
//    void swap(tile_group& other) {
//        using std::swap;
//        swap(id, other.id);
//        swap(tiles, other.tiles);
//    }
//
//    explicit tile_group(Json::Value const& json);
//
//    utf8string        id;
//    std::vector<tile> tiles;
//};
////==============================================================================
////
////==============================================================================
//struct tile_set {   
//    tile_set();
//
//    tile_set(tile_set&& other)
//      : size{other.size}
//      , file_name{std::move(other.file_name)}
//      , tiles{std::move(other.tiles)}
//    {
//    }
//
//    tile_set& operator=(tile_set&& rhs) {
//        swap(rhs);
//        return *this;
//    }
//
//    void swap(tile_set& other) {
//        using std::swap;
//        swap(size, other.size);
//        swap(file_name, other.file_name);
//        swap(tiles, other.tiles);
//    }
//
//    size_t     size;
//    utf8string file_name;
//    bklib::flat_map<size_t, tile_group> tiles;
//};

} //namespace data
} //namespace tez
