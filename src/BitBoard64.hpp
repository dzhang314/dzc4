#ifndef DZC4_BITBOARD64_HPP_INCLUDED
#define DZC4_BITBOARD64_HPP_INCLUDED

// C++ standard library headers
#include <cstdint> // for std::uint64_t

// x86 intrinsic headers
#include <x86intrin.h> // for _lzcnt_u64

// BitBoard64 is a 64-bit datatype that represents the configuration of one
// player's pieces in a Connect Four board. Each bit represents a playable
// space on a 7x8 Connect Four board, as follows:

//       X  X  X  X  X  X  X  X (MSB)  (All Connect Four boards in comments
//       6 14 22 30 38 46 54 62         are drawn so that pieces are dropped
//       5 13 21 29 37 45 53 61         into the board from the top, and
//       4 12 20 28 36 44 52 60         gravity pulls them toward the bottom.)
//       3 11 19 27 35 43 51 59
//       2 10 18 26 34 42 50 58        (A set bit indicates that a piece is
//       1  9 17 25 33 41 49 57         present; a clear bit represents an
// (LSB) 0  8 16 24 32 40 48 56         empty space.)

// The eight bits in the top row (7, 15, 23, 31, 39, 47, 55, 63) must NEVER
// BE SET; they must always be zero for the win-checking algorithm employed
// in won() to work. In particular, they prevent vertical and diagonal
// four-in-a-row configurations from spilling between columns.

namespace dzc4 {

    struct BitBoard64 {

    public: // ================================================ MEMBER VARIABLES

        // BitBoard64s are immutable. Do not allow direct modification of data.
        const std::uint64_t data;

    public: // ==================================================== CONSTRUCTORS

        // Explicitly disallow declaration of uninitialized BitBoard64s.
        BitBoard64() = delete;

        explicit constexpr BitBoard64(std::uint64_t data) : data(data) {}

    public: // =================================================== STATE TESTING

        constexpr std::uint64_t won() const {
            const std::uint64_t check_1 = data    & (data    >>  1);
            const std::uint64_t check_7 = data    & (data    >>  7);
            const std::uint64_t check_8 = data    & (data    >>  8);
            const std::uint64_t check_9 = data    & (data    >>  9);
            const std::uint64_t match_1 = check_1 & (check_1 >>  2);
            const std::uint64_t match_7 = check_7 & (check_7 >> 14);
            const std::uint64_t match_8 = check_8 & (check_8 >> 16);
            const std::uint64_t match_9 = check_9 & (check_9 >> 18);
            return match_1 | match_7 | match_8 | match_9;
        }

        // Note: Logically, this method should be constexpr, but _lzcnt_u64
        // comes from a C intrinsic header and is not marked constexpr.
        unsigned height(unsigned col) const {
            constexpr std::uint64_t mask = 0xFF;
            const std::uint64_t x = data & (mask << (8 * col));
            return 7 - static_cast<unsigned>(_lzcnt_u64(x) - 1) % 8;
        }

    }; // struct BitBoard64

} // namespace dzc4

#endif // DZC4_BITBOARD64_HPP_INCLUDED
