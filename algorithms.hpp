#pragma once

namespace bklib {

//==============================================================================
//! For every element in the container @c c matching the predicate @c test,
//! acculate via @sum.
//!
//! @returns pair<size_t, Container::value_type> = {iteration count, sum}
//!
//! @tparam Container Forward iteratable container.
//! @tparam Value Value type for the initial value for the sum.
//! @tparam Test Unary predicate(Container::value_type) -> bool.
//! @tparam Sum Binary function(Value, Container::value_type) -> Value.
//==============================================================================
template <typename Container, typename Value, typename Test, typename Sum>
auto accumulate_if(Container const& c, Value init, Test test, Sum sum)
-> std::pair<size_t, Value>
{
    auto it  = std::cbegin(c);
    auto end = std::cend(c);

    Value  value = init;
    size_t count = 0;

    while (end != (it = std::find_if(it, end, test))) {
        value = sum(value, *it++);
        ++count;
    }

    return {count, value};
}

} //namespace bklib
