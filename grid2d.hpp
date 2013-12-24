#pragma once

#include <vector>

#include <boost/iterator/iterator_adaptor.hpp>

#include <bklib/config.hpp>
#include <bklib/assert.hpp>
#include <bklib/math.hpp>

namespace tez {

//==============================================================================
template <typename T>
struct index2d {
    static_assert(std::is_integral<T>::value, "indicies must be integral.");

    size_t as_size_t(size_t const stride, T dx = 0, T dy = 0) const BK_NOEXCEPT {
        BK_ASSERT(y + dy >= 0);
        BK_ASSERT(x + dx >= 0);

        auto const yy = static_cast<size_t(y + dy);
        auto const xx = static_cast<size_t(x + dx);

        return yy * stride + xx;
    }

    T x, y;
};
//==============================================================================
template <typename T> index2d<T> north_of(index2d<T> i) { return {i.x + 0, i.y - 1}; }
template <typename T> index2d<T> south_of(index2d<T> i) { return {i.x + 0, i.y + 1}; }
template <typename T> index2d<T> east_of(index2d<T> i)  { return {i.x + 1, i.y + 0}; }
template <typename T> index2d<T> west_of(index2d<T> i)  { return {i.x - 1, i.y + 0}; }
//==============================================================================


namespace detail {
    //==========================================================================
    template <typename T>
    struct grid_iterator_value {
        using index_t = index2d<size_t>;

        grid_iterator_value operator=(grid_iterator_value const&) = delete;

        grid_iterator_value(T& value, index_t i) BK_NOEXCEPT
          : value {value}
          , i {i}
        {
        }

        T& operator=(T rhs) {
            return (value = rhs);
        }

        operator T&() BK_NOEXCEPT { return value; }
        operator T const&() const BK_NOEXCEPT { return value; }

        T&      value;
        index_t i;
    };

    template <typename T>
    bool operator==(grid_iterator_value<T> const& lhs, T const& rhs) { return lhs.value == rhs; }

    template <typename T>
    bool operator==(T const& lhs, grid_iterator_value<T> const& rhs) { return lhs == rhs.value; }

    //==========================================================================
    template <typename T>
    struct grid_iterator_traits {
        using base      = typename std::vector<T>::iterator;
        using value     = grid_iterator_value<T>;
        using traversal = boost::random_access_traversal_tag;
        using reference = value;
    };

    template <typename T>
    struct grid_iterator_traits<T const> {
        using base      = typename std::vector<T>::const_iterator;
        using value     = grid_iterator_value<T const>;
        using traversal = boost::random_access_traversal_tag;
        using reference = value;
    };
    //==========================================================================
} //namespace detail

//==============================================================================
template <typename T>
class grid_iterator : public boost::iterator_adaptor<
    grid_iterator<T>                                    // Derived
  , typename detail::grid_iterator_traits<T>::base      // Base
  , typename detail::grid_iterator_traits<T>::value     // Value
  , typename detail::grid_iterator_traits<T>::traversal // CategoryOrTraversal
  , typename detail::grid_iterator_traits<T>::reference // Reference
> {
public:
    grid_iterator() BK_NOEXCEPT
      : grid_iterator::iterator_adaptor_ {}
      , width_{0}
      , height_{0}
      , pos_{}
    {
    }

    grid_iterator(base_type it, size_t w, size_t h, size_t pos = 0)
      : grid_iterator::iterator_adaptor_ {it}
      , width_{w}
      , height_{h}
      , pos_{pos}
    {
    }

    template <typename U>
    grid_iterator(
        grid_iterator<U> const& other
      , typename std::enable_if<std::is_convertible<U*,T*>::value>::type* = nullptr
    )
      : grid_iterator(other.base(), width_, height_, pos_)
    {
    }
 private:
    friend class boost::iterator_core_access;

    typename iterator_adaptor::reference dereference() const {
        BK_ASSERT(width_ > 0 && height_ > 0);

        auto const d = std::div(
            static_cast<intmax_t>(pos_), static_cast<intmax_t>(width_)
        );

        return value_type(
            *base()
          , {static_cast<size_t>(d.rem), static_cast<size_t>(d.quot)}
        );
    }

    void advance(typename iterator_adaptor::difference_type n) {
        BK_ASSERT(pos_ + n <= width_ * height_);

        base_reference() = base() + n;
        pos_ += n;
    }

    void decrement() {
        advance(-1);
    }

    void increment() {
        advance(1);
    }

    size_t pos_;
    size_t width_;
    size_t height_;
};
//==============================================================================
template <typename T>
using const_grid_iterator = grid_iterator<T const>;
//==============================================================================
//
//template <typename T, typename IndexType>
//class grid_sub_itererator
//  : public boost::iterator_adaptor<
//        grid_sub_itererator<T, IndexType>                            // Derived
//      , typename grid_itererator_traits<T, IndexType>::base      // Base
//      , typename grid_itererator_traits<T, IndexType>::value     // Value
//      , typename grid_itererator_traits<T, IndexType>::traversal // CategoryOrTraversal
//      , typename grid_itererator_traits<T, IndexType>::reference // Reference
//    >
//{
//public:
//    grid_sub_itererator() BK_NOEXCEPT
//      : grid_sub_itererator::iterator_adaptor_ {}
//      , width_ {0}, height_ {0}, pos_ {}
//    {
//    }
//
//    grid_sub_itererator(base_type it, size_t offset, size_t stride, IndexType w, IndexType h)
//      : grid_sub_itererator::iterator_adaptor_ {it + offset}
//        , width_ {w}, height_ {h}, pos_ {0}, offset_ {offset}, stride_ {stride}
//    {
//    }
//
//    template <typename U>
//    grid_sub_itererator(
//        grid_sub_itererator<U, IndexType> const& other
//      , typename std::enable_if<std::is_convertible<U*,T*>::value>::type* = nullptr
//    )
//        : grid_sub_itererator::iterator_adaptor_(other.base())
//    {
//    }
// private:
//    friend class boost::iterator_core_access;
//
//    typename iterator_adaptor::reference dereference() const {
//        auto const x = (pos_ % width_) + (stride_ - width_);
//        auto const y = (pos_ + offset_) / stride_;
//
//        return grid_itererator_value<T, IndexType>(
//            *base(), {static_cast<IndexType>(x), static_cast<IndexType>(y) }
//        );
//    }
//
//    void advance(typename iterator_adaptor::difference_type n) {
//        auto dx = (pos_ + n) % width_;
//        auto dy = ((pos_ + n) / width_) - (pos_ / width_);
//
//        BK_ASSERT(pos_ + n >= 0);
//        BK_ASSERT(pos_ + n <= width_ + height_);
//
//        if (pos_ + n == width_ + height_) {
//            base_reference() = base() + n;
//        } else {
//            base_reference() = base() + n + dy*(stride_ - width_ + 1);
//        }
//
//        pos_ += n;
//    }
//
//    void decrement() {
//        advance(-1);
//    }
//
//    void increment() {
//        advance(+1);
//    }
//
//    size_t pos_;
//    size_t offset_;
//    size_t stride_;
//
//    IndexType width_;
//    IndexType height_;
//};
//==============================================================================
template <typename T>
class grid2d {
public:
    using index_t = size_t;
    using index   = index2d<index_t>;

    using reference       = T&;
    using const_reference = T const&;

    using iterator       = grid_iterator<T>;
    using const_iterator = grid_iterator<T const>;

    //using sub_iterator       = grid_sub_itererator<T,       IndexType>;
    //using const_sub_iterator = grid_sub_itererator<T const, IndexType>;

    grid2d(grid2d const&) = delete;
    grid2d& operator=(grid2d const&) = delete;

    grid2d(grid2d&& other)
      : width_(other.width_)
      , height_(other.height_)
      , data_(std::move(other.data_))
    {
    }

    grid2d& operator=(grid2d&& rhs) {
        rhs.swap(*this);
        return *this;
    }

    void swap(grid2d& other) {
        using std::swap;
        swap(width_, other.width_);
        swap(height_, other.height_);
        swap(data_, other.data_);
    }

    grid2d(index_t const w, index_t const h, T const value = T {})
      : width_{w}
      , height_{h}
      , data_(w*h, value)
    {
    }

    grid2d() : grid2d(0, 0) {}

    size_t width()    const BK_NOEXCEPT { return width_; }
    size_t height()   const BK_NOEXCEPT { return height_; }
    size_t size()     const BK_NOEXCEPT { return width() * height(); }

    size_t mem_size() const BK_NOEXCEPT {
        auto const c = sizeof(grid2d);
        auto const x = sizeof(T);
        auto const n = size();

        return n*x + c;
    }

    reference operator[](index i) {
        return data_[index2d_to_index_(i)];
    }

    const_reference operator[](index i) const {
        return data_[index2d_to_index_(i)];
    }

    bool is_valid(index i) const BK_NOEXCEPT {
        return (i.x < width_) && (i.y < height_);
    }

    //sub_iterator begin(point pa, point pb) {
    //    reorder(pa, pb);

    //    BK_ASSERT(pa.x > 0);
    //    BK_ASSERT(pa.y > 0);

    //    BK_ASSERT(pb.x < width_);
    //    BK_ASSERT(pb.y < height_);
    //
    //    auto const w = pb.x - pa.x;
    //    auto const h = pb.y - pa.y;

    //    return sub_iterator(data_.begin(), index2d_to_index_(pa), width_, w, h);
    //}
    //
    //sub_iterator end(point pa, point pb) {
    //    reorder(pa, pb);

    //    BK_ASSERT(pa.x > 0);
    //    BK_ASSERT(pa.y > 0);

    //    BK_ASSERT(pb.x < width_);
    //    BK_ASSERT(pb.y < height_);

    //    //the recct formed by pa and pb is exclusive of pb; make the iterator
    //    //point to one element past the last element.
    //    --pb.y;

    //    return sub_iterator(data_.begin(), index2d_to_index_(pb), 0, 0, 0);
    //}
    //
    iterator begin() { return iterator(data_.begin(), width_, height_); }
    iterator end()   { return iterator(data_.end(), width_, height_); }

    const_iterator begin() const { return const_iterator(data_.begin(), width_, height_); }
    const_iterator end()   const { return const_iterator(data_.end(), width_, height_); }

    const_iterator cbegin() const { return begin(); }
    const_iterator cend()   const { return end(); }
private:
    size_t index2d_to_index_(index i) const BK_NOEXCEPT {
        BK_ASSERT(is_valid(i));
        return i.y * width_ + i.x;
    }

    index_t width_;
    index_t height_;
    std::vector<T> data_;
};
//==============================================================================

} //namespace tez
