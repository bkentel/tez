#pragma once

#include <bklib/config.hpp>
#include <bklib/util.hpp>

#include "types.hpp"

namespace tez {
////////////////////////////////////////////////////////////////////////////////
// zip
////////////////////////////////////////////////////////////////////////////////
template <typename T1, typename T2, typename F>
inline void zip(T1 first1, T1 last1, T2 first2, T2 last2, F function) {
    while (first1 != last1 && first2 != last2) {
        function(*first1++, *first2++);
    }
}

////////////////////////////////////////////////////////////////////////////////
// tagged_value
////////////////////////////////////////////////////////////////////////////////
template <typename T, typename Tag>
struct tagged_value {
    T value;

    explicit tagged_value(T value = T{0}) BK_NOEXCEPT : value {value} {}
    explicit operator T() const BK_NOEXCEPT {
        return value;
    }
};

template <typename T, typename Tag>
inline bool operator<(tagged_value<T, Tag> const& lhs, tagged_value<T, Tag> const& rhs) {
    return lhs.value < rhs.value;
}

template <typename T, typename Tag>
inline bool operator>(tagged_value<T, Tag> const& lhs, tagged_value<T, Tag> const& rhs) {
    return lhs.value > rhs.value;
}

template <typename T, typename Tag>
inline bool operator==(tagged_value<T, Tag> const& lhs, tagged_value<T, Tag> const& rhs) {
    return lhs.value == rhs.value;
}

template <typename T, typename Tag>
inline bool operator!=(tagged_value<T, Tag> const& lhs, tagged_value<T, Tag> const& rhs) {
    return lhs.value != rhs.value;
}

////////////////////////////////////////////////////////////////////////////////
// hashed_string
////////////////////////////////////////////////////////////////////////////////
struct hashed_string {
    hashed_string()
      : string {}
      , hash   {0}
    {
    }

    hashed_string(char const* str)
      : string {str}
      , hash   {bklib::utf8string_hash(str)}
    {
    }

    hashed_string(bklib::utf8string str)
      : string {std::move(str)}
      , hash   {bklib::utf8string_hash(str)}
    {
    }

    hashed_string(bklib::string_ref str)
      : hashed_string{str}
    {
    }

    bklib::utf8string string;
    bklib::hash_t     hash;
};

inline bool operator<(hashed_string const& lhs, hashed_string const& rhs) {
    return lhs.hash < rhs.hash;
}

struct hash_wrapper_base {
protected:
    explicit hash_wrapper_base(hash_t hash)
      : value {hash} {}
public:
    hash_wrapper_base(utf8string const& string)
      : value {bklib::utf8string_hash(string)} {}
    hash_wrapper_base(char const* string)
      : value {bklib::utf8string_hash(string)} {}
    hash_wrapper_base(string_ref string)
      : value {bklib::utf8string_hash(string)} {}

    hash_t value;
};

template <typename Tag>
struct hash_wrapper : public hash_wrapper_base {
    using hash_wrapper_base::hash_wrapper_base;

    hash_wrapper(tagged_value<hash_t, Tag> hash)
      : hash_wrapper_base{hash.value} {}
};

////////////////////////////////////////////////////////////////////////////////
// parser_base
////////////////////////////////////////////////////////////////////////////////
class parser_base_common {
public:
    using cref       = bklib::json::cref;
    using utf8string = bklib::utf8string;

    parser_base_common() = default;

    explicit parser_base_common(bklib::utf8string const& file_name)
      : parser_base_common{std::ifstream{file_name}}
    {
    }

    explicit parser_base_common(bklib::platform_string const& file_name)
      : parser_base_common{std::ifstream{file_name}} //TODO
    {
    }

    explicit parser_base_common(std::istream&& in)
      : parser_base_common{in}
    {
    }

    explicit parser_base_common(std::istream& in) {
        if (!in) {
            BK_DEBUG_BREAK(); //TODO
        }

        Json::Reader reader;

        auto const result = reader.parse(in, json_root_);
        if (!result) {
            std::cout << reader.getFormattedErrorMessages();
            BK_DEBUG_BREAK(); //TODO
        }
    }
protected:
    Json::Value json_root_;
};

template <typename Derived>
class parser_base : public parser_base_common {
public :
    using parser_base_common::parser_base_common;

    void parse() {
        static_cast<Derived*>(this)->rule_root(json_root_);
    }

    void parse(cref json_value) {
        static_cast<Derived*>(this)->rule_root(json_value);
    }
};

} //namespace tez
