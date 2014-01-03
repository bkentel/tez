#include <gtest/gtest.h>

#include <boost/iterator/filter_iterator.hpp>
#include <boost/iterator/transform_iterator.hpp>

#include "loot_table.hpp"
#include "item.hpp"

#include <bklib/math.hpp>

template <typename T>
class grid_block {
public:
    explicit grid_block(T const& value = T{}) {
        data_.fill(value);
    }

    T const& operator[](size_t const i) const {
        BK_ASSERT((i & 0xFF) == i);
        return data_[i];
    }

    T& operator[](size_t const i) {
        BK_ASSERT((i & 0xFF) == i);
        return data_[i];
    }
private:
    std::array<T, 0xFF> data_;
};

template <typename T>
class grid {
public:
    grid(size_t const width, size_t const height, T const& value = T{})
      : width_  {(width  | 0x10) & 0x10}
      , height_ {(height | 0x10) & 0x10}
      , blocks_((width_ >> 4) * (height_ >> 4), grid_block<T> {value})
    {
    }

    size_t block_w() const { return width_  >> 4; }
    size_t block_h() const { return height_ >> 4; }

    T const& at(size_t x, size_t y) const {
        return const_cast<grid*>(this)->at(x, y);
    }

    T& at(size_t x, size_t y) {
        BK_ASSERT(x < width_);
        BK_ASSERT(y < height_);

        auto const x0 = x >> 4;
        auto const y0 = y >> 4;

        auto const x1 = x & 0xF;
        auto const y1 = y & 0xF;

        auto const i0 = y0 * block_w() + x0;
        auto const i1 = (y1 << 4) + x1;

        return blocks_[i0][i1];
    }
private:
    size_t         width_;
    size_t         height_;
    std::vector<grid_block<T>> blocks_;
};

namespace generator {

struct grid_layout {
    using rect = bklib::axis_aligned_rect<int>;

    static size_t ceil_div(size_t dividend, size_t divisor) {
        return (dividend / divisor) + ((dividend % divisor) ? 1 : 0);
    }

    grid_layout(size_t width, size_t height, size_t cell_size)
      : width_     {ceil_div(width,  cell_size)}
      , height_    {ceil_div(height, cell_size)}
      , cell_size_ {cell_size}
    {
    }

    std::vector<rect> operator()(tez::random_t& random) {
        using dist_t = std::uniform_int_distribution<>;

        auto const n = width_*height_;

        cells_.clear();
        cells_.resize(n);

        auto const min_size  = 3;
        auto const size_dist = dist_t(min_size, cell_size_);

        for (size_t i = 0; i < cells_.size(); ++i) {
            auto const cr = cell_rect(i);

            auto const w = size_dist(random);
            auto const h = size_dist(random);

            auto const dx = dist_t(0, cell_size_ - w)(random);
            auto const dy = dist_t(0, cell_size_ - h)(random);

            auto const x0 = cr.left() + dx;
            auto const y0 = cr.top()  + dy;
            auto const x1 = x0 + w;
            auto const y1 = y0 + h;

            cells_[i].push_back(rect {x0, y0, x1, y1});
        }

        std::vector<rect> result;
        result.reserve(n);

        for (auto& cell : cells_) {
            for (auto& rect : cell) {
                result.push_back(rect);
            }
        }

        return result;
    }

    rect cell_rect(size_t i) const {
        return cell_rect(i % width_, i / width_);
    }

    rect cell_rect(size_t x, size_t y) const {
        BK_ASSERT(x < width_);
        BK_ASSERT(y < height_);

        auto const x0 = static_cast<int>(x  * cell_size_);
        auto const y0 = static_cast<int>(y  * cell_size_);
        auto const x1 = static_cast<int>(x0 + cell_size_);
        auto const y1 = static_cast<int>(y0 + cell_size_);

        return {x0, y0, x1, y1};
    }

    size_t cell_size_;
    size_t width_;
    size_t height_;

    std::vector<std::vector<rect>> cells_;
};

//void generate() {
//    tez::random_t random {100};
//
//    using rect_t = bklib::axis_aligned_rect<int>;
//
//    std::vector<rect_t> input_stack;
//    std::vector<rect_t> output_stack;
//
//    input_stack.push_back(rect_t{0, 0, 100, 100});
//
//    int const min_w = 3;
//    int const min_h = 3;
//
//    for (size_t i = 0; i < input_stack.size(); ++i) {
//        auto const& rect = input_stack[i];
//
//        auto const w = rect.width();
//        auto const h = rect.height();
//
//        bool split_w = (w / 2) >= min_w;
//        bool split_h = (h / 2) >= min_h;
//
//        if (split_w && split_h) {
//            if (i % 2) {
//                split_w = false;
//                split_h = true;
//            } else {
//                split_w = true;
//                split_h = false;
//            }
//        }
//
//        if (split_w) {
//            auto const range = (w - 2*min_w) / 2;
//            auto const delta = std::uniform_int_distribution<>{0, range}(random);
//            auto const w0 = w / 2 + delta;
//            auto const w1 = w - w0;
//
//            rect_t r0 {rect.left(), rect.top(), rect.left() + w0, rect.bottom()};
//            rect_t r1 {r0.right(),  r0.top(),   r0.right() + w1,  rect.bottom()};
//
//            input_stack.push_back(r0);
//            input_stack.push_back(r1);
//        } else if (split_h) {
//            auto const range = (h - 2*min_h) / 2;
//            auto const delta = std::uniform_int_distribution<>{0, range}(random);
//            auto const h0 = h / 2 + delta;
//            auto const h1 = h - h0;
//
//            rect_t r0 {rect.left(), rect.top(),  rect.right(), rect.top() + h0};
//            rect_t r1 {rect.left(), r0.bottom(), rect.right(), r0.bottom() + h1};
//
//            input_stack.push_back(r0);
//            input_stack.push_back(r1);
//        } else {
//            output_stack.push_back(rect);
//            continue;
//        }
//    }
//
//}

} //namespace generator


TEST(LootTable, Basic) {
    tez::random_t random {1984};

    generator::grid_layout grid {100, 100, 10};

    auto rects = grid(random);

    //tez::random_t random{150123};

    //tez::loot_table_table::reload("./data/loot.def");
    //tez::item_table::reload("./data/items.def");

    //auto table = tez::loot_table_table::get("orc");

    //while (true) {
    //    std::cout << "****rolling***" << std::endl;

    //    auto items = table->roll(random);

    //    auto const en = tez::language_ref{bklib::utf8string_hash("en")};
    //    auto const jp = tez::language_ref{bklib::utf8string_hash("jp")};

    //    for (auto const item_ref : items) {
    //        auto const item = tez::item_table::get(item_ref);
    //        std::cout << item->names.get(en) << std::endl;
    //    }
    //}
}
