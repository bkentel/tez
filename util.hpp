#pragma once

#include <boost/container/flat_map.hpp>

#include <bklib/config.hpp>
#include <bklib/util.hpp>

#include "types.hpp"

namespace tez {

template <size_t N>
inline bklib::string_ref make_string_ref(char const (&str)[N]) {
    return bklib::string_ref{str, N - 1};
}

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
      , hash   {bklib::utf8string_hash(string)}
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

namespace detail {
    template <typename Tag> struct tag_traits;
}

template <typename Tag>
struct data_table {
    using traits      = detail::tag_traits<Tag>;
    using pointer     = typename traits::pointer;
    using container_t = boost::container::flat_map<
        typename traits::ref
      , typename traits::type
    >;

    static bool is_loaded() { return is_loaded_; }

    static void reload(bklib::utf8string filename) {
        typename traits::parser parser {filename};
        parser.parse();

        if (!is_loaded()) {
            data_ = parser.get();
        } else {
            BK_DEBUG_BREAK(); //TODO
        }

        is_loaded_ = true;
    }

    static pointer const get(hash_wrapper<Tag> ref) {
        BK_ASSERT(is_loaded_);

        auto const key = typename traits::ref {ref.value};

        auto const where = data_.find(key);
        return where != std::cend(data_) ? &where->second : nullptr;
    }
private:
    static bool        is_loaded_;
    static container_t data_;
};

////////////////////////////////////////////////////////////////////////////////
// parser_base
////////////////////////////////////////////////////////////////////////////////
class parser_base_common {
public:
    using cref = bklib::json::cref;

    parser_base_common() = default;

    explicit parser_base_common(utf8string const& file_name)
      : parser_base_common{std::ifstream{file_name}}
    {
    }

    explicit parser_base_common(platform_string const& file_name)
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

namespace detail {
    inline Json::Value parse_json(std::istream& in) {
        if (!in) {
            BK_DEBUG_BREAK(); //TODO
        }

        Json::Reader reader;
        Json::Value  root;

        auto const result = reader.parse(in, root);
        if (!result) {
            std::cout << reader.getFormattedErrorMessages();
            BK_DEBUG_BREAK(); //TODO
        }

        return root;
    }
} //namespace detail

template <typename Derived>
class parser_base_t {
public:
    using cref = bklib::json::cref;

    void parse(string_ref file_name) {
        auto in = std::ifstream {file_name};
        parse(in);
    }

    void parse(std::istream& in) {
        auto root = detail::parse_json(in);
        static_cast<Derived*>(this)->rule_root(root);
    }

    void parse(cref json_value) {
        static_cast<Derived*>(this)->rule_root(json_value);
    }    
};

} //namespace tez
