#ifndef DZC4_COMPRESSED_POSITION_64_HPP_INCLUDED
#define DZC4_COMPRESSED_POSITION_64_HPP_INCLUDED

#include <compare> // for operator<=>
#include <cstdint> // for std::uint64_t

#include "BitBoard64.hpp"  // for BitBoard64
#include "Position128.hpp" // for Position128

namespace dzc4 {


struct CompressedPosition64 {


    std::uint64_t data;


    explicit constexpr CompressedPosition64() noexcept
        : data(0x0101010101010101) {}


    explicit constexpr CompressedPosition64(std::uint64_t compressed) noexcept
        : data(compressed) {}


    explicit constexpr CompressedPosition64(const Position128 &position
    ) noexcept
        : data(position.black.data) {
        constexpr std::uint64_t bit = 1;
        const BitBoard64 full = position.full_board();
        data |= bit << full.height(0);
        data |= bit << (full.height(1) + 8);
        data |= bit << (full.height(2) + 16);
        data |= bit << (full.height(3) + 24);
        data |= bit << (full.height(4) + 32);
        data |= bit << (full.height(5) + 40);
        data |= bit << (full.height(6) + 48);
        data |= bit << (full.height(7) + 56);
    }


    constexpr auto
    operator<=>(const CompressedPosition64 &) const noexcept = default;


    constexpr unsigned offset(unsigned col) const noexcept {
        constexpr std::uint64_t mask = 0xFF;
        const std::uint64_t x = data & mask << (8 * col);
        return 7 - static_cast<unsigned>(std::countl_zero(x)) % 8;
    }


    constexpr Position128 decompress() const noexcept {
        constexpr std::uint64_t bit = 1;
        std::uint64_t mask = (bit << offset(0)) - 1;
        mask |= ((bit << offset(1)) - 1) << 8;
        mask |= ((bit << offset(2)) - 1) << 16;
        mask |= ((bit << offset(3)) - 1) << 24;
        mask |= ((bit << offset(4)) - 1) << 32;
        mask |= ((bit << offset(5)) - 1) << 40;
        mask |= ((bit << offset(6)) - 1) << 48;
        mask |= ((bit << offset(7)) - 1) << 56;
        const BitBoard64 white(mask & ~data);
        const BitBoard64 black(mask & data);
        return Position128(white, black);
    }


}; // struct CompressedPosition64


} // namespace dzc4

#endif // DZC4_COMPRESSED_POSITION_64_HPP_INCLUDED
