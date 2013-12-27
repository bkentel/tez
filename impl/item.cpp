#include "item.hpp"

#include <bklib/json.hpp>

#include "languages.hpp"

using tez::item_parser;
using tez::item_definition;
using tez::utf8string;
using tez::item_attribute;
using tez::item_attribute_ref;
using tez::make_string_ref;

namespace json = bklib::json;

//ROOT              = {"items": ITEM_LIST}
//ITEM_LIST         = [ITEM*]
//ITEM              = {ITEM_ID, ITEM_CATEGORY, ITEM_TYPE, ITEM_TAG_LIST, ITEM_ATTR_LIST, ITEM_BASE_VALUE, ITEM_NAME, ITEM_DESC}
//ITEM_ID           = "id": string
//ITEM_CATEGORY     = "category": string
//ITEM_TYPE         = "type": string
//ITEM_TAG_LIST     = "tags": [ITEM_TAG*]
//ITEM_TAG          = string
//ITEM_BASE_VALUE   = "base_value": int
//ITEM_ATTR_LIST    = "attributes": [ITEM_ATTR*]
//ITEM_ATTR         = [ATTR_NAME, ATTR_VALUE]
//ATTR_NAME         = string
//ATTR_VALUE        = any
//ITEM_NAME         = LANG_STRING_LIST
//ITEM_DESC         = LANG_STRING_LIST
//LANG_STRING_LIST  = [LANG_STRING*]
//LANG_STRING       = [LANG_STRING_ID, LANG_STRING_VALUE]
//LANG_STRING_ID    = string
//LANG_STRING_VALUE = string

namespace {

auto const KEY_ITEMS            = make_string_ref("items");
auto const KEY_ITEM_ID          = make_string_ref("id");
auto const KEY_ITEM_CATEGORY    = make_string_ref("category");
auto const KEY_ITEM_TYPE        = make_string_ref("type");
auto const KEY_ITEM_TAGS        = make_string_ref("tags");
auto const KEY_ITEM_BASE_VALUE  = make_string_ref("base_value");
auto const KEY_ITEM_WEIGHT      = make_string_ref("weight");
auto const KEY_ITEM_ATTRIBUTES  = make_string_ref("attributes");
auto const KEY_ITEM_NAME        = make_string_ref("name");
auto const KEY_ITEM_DESCRIPTION = make_string_ref("description");

size_t const INDEX_ATTR_NAME  = 0;
size_t const INDEX_ATTR_VALUE = 1;

size_t const SIZE_ATTRIBUTE = 2;

} //namespace

void item_parser::rule_root(cref json_value) {
    try {
        json::require_object(json_value);
        if (json_value.size() != 1) {
            BK_DEBUG_BREAK(); //TODO
        }

        auto item_list = json::require_key(json_value, KEY_ITEMS);
        rule_item_list(item_list);
    } catch (json::error::base& e) {
        BK_JSON_ADD_TRACE(e);
    }
}

void item_parser::rule_item_list(cref json_value) {
    try  {
        json::require_array(json_value);

        json::for_each_element_skip_on_fail(json_value, [&](cref item) {
            rule_item(item);

            auto const result = items_.emplace(
                item_ref{item_.id.hash}, std::move(item_)
            );

            if (!result.second) {
                BK_DEBUG_BREAK(); //TODO
            }
        });
    } catch (json::error::base& e) {
        BK_JSON_ADD_TRACE(e);
    }
}

void item_parser::rule_item(cref json_value) {
    using json::require_key;

    try  {
        auto item_id        = require_key(json_value, KEY_ITEM_ID);
        auto item_category  = require_key(json_value, KEY_ITEM_CATEGORY);
        auto item_type      = require_key(json_value, KEY_ITEM_TYPE);
        auto item_tag_list  = require_key(json_value, KEY_ITEM_TAGS);
        auto item_weight    = require_key(json_value, KEY_ITEM_WEIGHT);
        auto item_base_val  = require_key(json_value, KEY_ITEM_BASE_VALUE);
        auto item_attr_list = require_key(json_value, KEY_ITEM_ATTRIBUTES);
        auto item_name      = require_key(json_value, KEY_ITEM_NAME);
        auto item_desc      = require_key(json_value, KEY_ITEM_DESCRIPTION);

        rule_item_id(item_id);
        rule_item_category(item_category);
        rule_item_type(item_type);
        rule_item_tag_list(item_tag_list);
        rule_item_weight(item_weight);
        rule_item_base_value(item_base_val);
        rule_item_attr_list(item_attr_list);
        rule_item_name(item_name);
        rule_item_desc(item_desc);
    } catch (json::error::base& e) {
        BK_JSON_ADD_TRACE(e);
    }
}

void item_parser::rule_item_id(cref json_value) {
    try  {
        item_.id = hashed_string {json::require_string(json_value)};
    } catch (json::error::base& e) {
        BK_JSON_ADD_TRACE(e);
    }
}

void item_parser::rule_item_category(cref json_value) {
    try  {
        auto category = json::require_string(json_value);
        item_.category = item_category_ref {bklib::utf8string_hash(category)};
    } catch (json::error::base& e) {
        BK_JSON_ADD_TRACE(e);
    }
}

void item_parser::rule_item_type(cref json_value) {
    try  {
        auto type = json::require_string(json_value);
        item_.type = item_type_ref {bklib::utf8string_hash(type)};
    } catch (json::error::base& e) {
        BK_JSON_ADD_TRACE(e);
    }
}

void item_parser::rule_item_tag_list(cref json_value) {
    try  {
        json::require_array(json_value);

        json::for_each_element_skip_on_fail(json_value, [&](cref tag) {
            rule_item_tag(tag);
        });
    } catch (json::error::base& e) {
        BK_JSON_ADD_TRACE(e);
    }
}

void item_parser::rule_item_tag(cref json_value) {
    try  {
        auto tag = json::require_string(json_value);
        auto const result = item_.tags.insert(item_tag_ref{bklib::utf8string_hash(tag)});
        if (!result.second) {
            BK_DEBUG_BREAK(); //TODO
        }
    } catch (json::error::base& e) {
        BK_JSON_ADD_TRACE(e);
    }
}

void item_parser::rule_item_weight(cref json_value) {
    try  {
        auto base_value = json::require_int(json_value);
        if (base_value < 0) {
            BK_DEBUG_BREAK(); //TODO
        }

        item_.weight = base_value;
    } catch (json::error::base& e) {
        BK_JSON_ADD_TRACE(e);
    }
}

void item_parser::rule_item_base_value(cref json_value) {
    try  {
        auto base_value = json::require_int(json_value);
        if (base_value < 0) {
            BK_DEBUG_BREAK(); //TODO
        }

        item_.base_value = base_value;
    } catch (json::error::base& e) {
        BK_JSON_ADD_TRACE(e);
    }
}

void item_parser::rule_item_attr_list(cref json_value) {
    try  {
        json::require_array(json_value);

        json::for_each_element_skip_on_fail(json_value, [&](cref attr) {
            rule_item_attr(attr);
        });
    } catch (json::error::base& e) {
        BK_JSON_ADD_TRACE(e);
    }
}

void item_parser::rule_item_attr(cref json_value) {
    try  {
        json::require_array(json_value);

        auto const size = json_value.size();
        if (size < SIZE_ATTRIBUTE) {
            BK_DEBUG_BREAK(); //TODO
        } else if (size > SIZE_ATTRIBUTE) {
            BK_DEBUG_BREAK(); //TODO
        }

        auto name  = json::require_key(json_value, INDEX_ATTR_NAME);
        auto value = json::require_key(json_value, INDEX_ATTR_VALUE);

        auto attr_name = rule_attr_name(name);
        auto attribute = rule_attr_value(value, item_attribute_ref{bklib::utf8string_hash(attr_name)});

        auto const result = item_.attributes.insert(attribute);
        if (!result.second) {
            BK_DEBUG_BREAK(); //TODO
        }
    } catch (json::error::base& e) {
        BK_JSON_ADD_TRACE(e);
    }
}

utf8string item_parser::rule_attr_name(cref json_value) {
    try  {
        return json::require_string(json_value);
    } catch (json::error::base& e) {
        BK_JSON_ADD_TRACE(e);
    }
}

item_attribute item_parser::rule_attr_value(cref json_value, item_attribute_ref name) {
    try  {
        return [&] {
            switch (json_value.type()) {
            case Json::ValueType::intValue :
            case Json::ValueType::uintValue :
                return item_attribute{name, json_value.asInt()};
                break;
            case Json::ValueType::stringValue :
                return item_attribute{name, json_value.asString()};
                break;
            case Json::ValueType::realValue :
                return item_attribute{name, json_value.asFloat()};
                break;
            default :
                BOOST_THROW_EXCEPTION(json::error::bad_type{}
                    << json::error::info_actual_type{{json_value.type()}}
                );

                break;
            }
        }();
    } catch (json::error::base& e) {
        BK_JSON_ADD_TRACE(e);
    }
}

void item_parser::rule_item_name(cref json_value) {
    try {
        language_string_parser parser;
        parser.parse(json_value);
        item_.names = parser.get();
    } catch (json::error::base& e) {
        BK_JSON_ADD_TRACE(e);
    }
}

void item_parser::rule_item_desc(cref json_value) {
    try  {
        language_string_parser parser;
        parser.parse(json_value);
        item_.descriptions = parser.get();
    } catch (json::error::base& e) {
        BK_JSON_ADD_TRACE(e);
    }
}

////////////////////////////////////////////////////////////////////////////////
// item_table
////////////////////////////////////////////////////////////////////////////////

bool                         tez::item_table::is_loaded_ {false};
tez::item_table::container_t tez::item_table::data_      {};
