#ifndef DZC4_COMPRESSEDPOSITION64_HPP_INCLUDED
#define DZC4_COMPRESSEDPOSITION64_HPP_INCLUDED

// C++ standard library headers
#include <cstdint> // for std::uint64_t
#include <ostream>

// x86 intrinsic headers
#include <x86intrin.h> // for _lzcnt_u64

// Project-specific headers
#include "Position128.hpp"

namespace dzc4 {

    struct CompressedPosition64 {

        std::uint64_t data;

        explicit constexpr CompressedPosition64() : data(0x0101010101010101) {}
        explicit constexpr CompressedPosition64(std::uint64_t c) : data(c) {}

        explicit CompressedPosition64(dzc4::Position128 pos) :
            data(pos.compressed_data()) {}

        constexpr bool operator==(CompressedPosition64 rhs) const { return data == rhs.data; }
        constexpr bool operator!=(CompressedPosition64 rhs) const { return data != rhs.data; }
        constexpr bool operator< (CompressedPosition64 rhs) const { return data <  rhs.data; }
        constexpr bool operator> (CompressedPosition64 rhs) const { return data >  rhs.data; }
        constexpr bool operator<=(CompressedPosition64 rhs) const { return data <= rhs.data; }
        constexpr bool operator>=(CompressedPosition64 rhs) const { return data >= rhs.data; }

        unsigned offset(unsigned col) const  {
            constexpr std::uint64_t mask = 0xFF;
            const std::uint64_t x = data & mask << (8 * col);
            return 7 - static_cast<unsigned>(_lzcnt_u64(x)) % 8;
        }

        std::uint64_t bitmask() const {
            constexpr std::uint64_t bit = 1;
            std::uint64_t mask = (bit << offset(0)) - 1;
            mask |= ((bit << offset(1)) - 1) <<  8;
            mask |= ((bit << offset(2)) - 1) << 16;
            mask |= ((bit << offset(3)) - 1) << 24;
            mask |= ((bit << offset(4)) - 1) << 32;
            mask |= ((bit << offset(5)) - 1) << 40;
            mask |= ((bit << offset(6)) - 1) << 48;
            mask |= ((bit << offset(7)) - 1) << 56;
            return mask;
        }

        dzc4::Position128 decompress() const {
            const std::uint64_t mask = bitmask();
            return Position128(mask & ~data, mask & data);
        }

        friend std::ostream &operator<<(std::ostream &os,
                                        CompressedPosition64 pos) {
            return os << pos.decompress();
        }

    }; // struct CompressedPosition64

} // namespace dzc4

#endif // DZC4_COMPRESSEDPOSITION64_HPP_INCLUDED