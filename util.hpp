#pragma once

#include <bklib/config.hpp>
#include <bklib/util.hpp>

namespace tez {
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
////////////////////////////////////////////////////////////////////////////////
// parser_base
////////////////////////////////////////////////////////////////////////////////
class parser_base {
public:
    using cref       = bklib::json::cref;
    using utf8string = bklib::utf8string;

    explicit parser_base(utf8string const& file_name)
      : parser_base{std::ifstream{file_name}}
    {
    }

    explicit parser_base(std::istream&& in)
      : parser_base{in}
    {
    }

    explicit parser_base(std::istream& in) {
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

} //namespace tez
