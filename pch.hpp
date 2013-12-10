#pragma once

#include <algorithm>
#include <fstream>
#include <list>
#include <regex>
#include <typeindex>
#include <array>
#include <functional>
#include <locale>
#include <set>
#include <typeinfo>
#include <atomic>
#include <future>
#include <map>
#include <sstream>
#include <type_traits>
#include <bitset>
#include <initializer_list>
#include <memory>
#include <stack>
#include <unordered_map>
#include <chrono>
#include <iomanip>
#include <mutex>
#include <stdexcept>
#include <unordered_set>
#include <codecvt>
#include <ios>
#include <new>
#include <streambuf>
#include <utility>
#include <complex>
#include <iosfwd>
#include <numeric>
#include <string>
#include <valarray>
#include <condition_variable>
#include <iostream>
#include <ostream>
#include <strstream>
#include <vector>
#include <deque>
#include <istream>
#include <queue>
#include <system_error>
#include <exception>
#include <iterator>
#include <random>
#include <thread>
#include <forward_list>
#include <limits>
#include <ratio>
#include <tuple>

#include <csignal>
#include <ciso646>
#include <cstdlib>
#include <climits>
#include <cstdarg>
#include <cstring>
#include <cerrno>
#include <cstddef>
#include <cstdint>

#pragma warning(push, 3)

    #include <boost/predef.h>
    #include <boost/exception/all.hpp>
    #include <boost/container/flat_map.hpp>
    #include <boost/container/flat_set.hpp>
    #include <boost/log/trivial.hpp>
    #include <boost/variant.hpp>
    #include <boost/utility/string_ref.hpp>

    #include <jsoncpp/json.h>
    #if defined(BOOST_COMP_MSVC)
    #   pragma comment(lib, "jsoncpp_mtd.lib")
    #endif

#pragma warning(pop)
