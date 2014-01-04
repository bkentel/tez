#pragma once

#include <boost/iterator/filter_iterator.hpp>
#include <boost/iterator/transform_iterator.hpp>

#include <bklib/keyboard.hpp>
#include <bklib/json_forward.hpp>

#include "types.hpp"
#include "commands.hpp"
#include "util.hpp"

namespace tez {

using bklib::key_combo;
using bklib::keycode;

namespace detail {
    using key_bindings_map_t = flat_map<key_combo, command_t>;
} //namespace detail

//==============================================================================
//==============================================================================
class bindings_parser : public parser_base_t<bindings_parser> {
public:
    using map_t = detail::key_bindings_map_t;

    bindings_parser();

    void rule_root(cref json_value);
    void rule_key_binding_list(cref json_value);
    void rule_key_binding(cref json_value);
    void rule_command_name(cref json_value);
    void rule_binding_list(cref json_value);
    void rule_binding(cref json_value);
    void rule_key(cref json_value);

    map_t&& get() { return std::move(bindings_); }
private:
    keycode   cur_key_;
    command_t cur_command_;
    key_combo cur_combo_;
    map_t     bindings_;
};

////////////////////////////////////////////////////////////////////////////////
// key_bindings
////////////////////////////////////////////////////////////////////////////////
namespace detail {
//==============================================================================
//! iterator details for key_bindings.
//==============================================================================
struct key_bindings_detail {
    using container_t      = key_bindings_map_t;
    using value_t          = container_t::value_type const&;
    using const_iterator_t = container_t::const_iterator;

    struct predicate : public std::unary_function<value_t, bool> {
        using ref = std::reference_wrapper<key_combo const>;

        predicate(ref combo) : combo {combo} {}

        bool operator()(value_t value) const {
            return combo.get().includes(value.first);
        }

        ref combo;
    };

    struct transform : public std::unary_function<value_t, command_t> {
        command_t operator()(value_t value) const {
            return value.second;
        }
    };

    using const_filter_iterator = boost::filter_iterator<predicate, const_iterator_t>;
    using const_iterator = boost::transform_iterator<transform, const_filter_iterator>;

    static const_iterator make_begin(container_t const& container, predicate::ref combo) {
        auto filter = const_filter_iterator {
            predicate {combo}
          , std::cbegin(container)
          , std::cend(container)
        };

        return {filter, transform{}};
    }

    static const_iterator make_end(container_t const& container, predicate::ref combo) {
        auto filter = const_filter_iterator {
            predicate {combo}
          , std::cend(container)
          , std::cend(container)
        };

        return {filter, transform{}};
    }
};

} //namespace detail

//==============================================================================
//! mappings between a key_combo and command_t.
//==============================================================================
class key_bindings {
    BK_NO_COPY_ASSIGN(key_bindings);
public:
    using detail = detail::key_bindings_detail;

    static string_ref const DEFAULT_FILE_NAME;

    key_bindings(string_ref file = DEFAULT_FILE_NAME);

    void reload(string_ref file = DEFAULT_FILE_NAME);
    void reload(std::istream& in);

    //--------------------------------------------------------------------------
    //! Iterate through all bindings that are a subset of @p keys.
    //--------------------------------------------------------------------------
    detail::const_iterator match_subset_begin(key_combo const& keys) const {
        return detail::make_begin(mappings_, keys);
    }

    detail::const_iterator match_subset_end(key_combo const& keys) const {
        return detail::make_end(mappings_, keys);
    }

    //--------------------------------------------------------------------------
    //! Return the binding matching @p keys; otherwise return command_t::NOT_FOUND.
    //--------------------------------------------------------------------------
    command_t match(key_combo const& keys) const;
private:
    detail::container_t mappings_;
};

} //namespace tez

