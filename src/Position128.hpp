#ifndef DZC4_POSITION128_HPP_INCLUDED
#define DZC4_POSITION128_HPP_INCLUDED

// C++ standard library headers
#include <climits> // for INT_MIN
#include <cstdint> // for std::uint64_t
#include <ostream>

// x86 intrinsic headers
#include <x86intrin.h> // for _lzcnt_u64

// Project-specific headers
#include "BitBoard64.hpp"
#include "Constants.hpp"

// Position128 is an immutable 128-bit datatype that represents a complete
// Connect Four game state. It consists of a pair of BitBoard64s, one for each
// player's pieces.

// a score of -1 indicates that the player to move has already lost
//     (i.e., their opponent already has four pieces in a row).
// a score of zero indicates a theoretically drawn game.
// a score of +1 indicates that the player to move has a winning move.
// a score of -2 indicates that the player to move has no move that will
//     prevent their opponent from winning on their next move.
// a score of +3 indicates that the player to move has a move such that, no
//     matter how their opponent replies, they can win on their next move.
// a score of -4 indicates that the player to move has no move that will
//     prevent their opponent from winning on the move after their next.

// if the player to move can move to a position whose score is negative for
//     their opponent, then they will take the least negative such move.
// otherwise, if the player to move can move to a position whose score is
//     zero, they will take such a move.
// otherwise, if the player to move can move to a position whose score is
//     positive, they will take the most positive such move.
// otherwise, if the player to move has no available moves,
//     the game is drawn.

namespace dzc4 {

    class Position128 {

    private: // =============================================== MEMBER VARIABLES

        // Position128s are immutable. Do not allow direct modification of data.
        const BitBoard64 white;
        const BitBoard64 black;

    public: // ==================================================== CONSTRUCTORS

        // Explicitly disallow declaration of uninitialized Position128s.
        Position128() = delete;

        explicit constexpr Position128(std::uint64_t white,
                                       std::uint64_t black) :
            white(white), black(black) {}

        constexpr BitBoard64 fullboard() const {
            return BitBoard64(white.data | black.data);
        }

        constexpr operator bool() const {
            return static_cast<bool>(white.data | black.data);
        }

        template <Player player>
        constexpr std::uint64_t won() const {
            if constexpr      (player == Player::WHITE) return white.won();
            else if constexpr (player == Player::BLACK) return black.won();
        }

        template <Player player, unsigned depth>
        Evaluation eval() const {
            if (won<player>())        return Evaluation::WIN;
            if (won<other(player)>()) return Evaluation::LOSS;
            if constexpr (depth == 0) return Evaluation::UNKNOWN;
            else {
                bool hasMove = false, hasUnknown = false, hasDraw = false;
                for (unsigned col = 0; col < NUM_COLS; ++col) {
                    if (const Position128 pos = move<player>(col)) {
                        hasMove = true;
                        const Evaluation r = pos.eval<other(player),
                                                      depth - 1>();
                        if (r == Evaluation::LOSS   ) return Evaluation::WIN;
                        if (r == Evaluation::UNKNOWN) hasUnknown = true;
                        if (r == Evaluation::DRAW   ) hasDraw = true;
                    }
                }
                return hasUnknown          ? Evaluation::UNKNOWN :
                       hasDraw || !hasMove ? Evaluation::DRAW    :
                                             Evaluation::LOSS;
            }
        }

        template <Player player, unsigned depth>
        int score() const {
            if (won<other(player)>()) return -1;
            if constexpr (depth == 0) return INT_MIN;
            else {
                int neg = INT_MIN, pos = 0;
                bool hasUnknown = false, hasDraw = false;
                for (unsigned col = 0; col < NUM_COLS; ++col) {
                    if (const Position128 posn = move<player>(col)) {
                        const int s = posn.score<other(player), depth - 1>();
                        if      (s == -1)      return +1;
                        else if (s == INT_MIN) hasUnknown = true;
                        else if (s < 0)        neg = std::max(neg, s);
                        else if (s > 0)        pos = std::max(pos, s);
                        else                   hasDraw = true;
                    }
                }
                return neg > INT_MIN ? 1 - neg  :
                       hasUnknown    ? INT_MIN  :
                       hasDraw       ? 0        :
                       pos > 0       ? -pos - 1 : 0;
            }
        }

        template <Player player>
        Position128 move(unsigned col) const {
            constexpr std::uint64_t bit = 1;
            const unsigned row = fullboard().height(col);
            if (row >= NUM_ROWS) return Position128(0, 0);
            if constexpr (player == Player::WHITE) {
                return Position128(white.data | bit << (8 * col + row),
                                   black.data);
            } else if constexpr (player == Player::BLACK) {
                return Position128(white.data,
                                   black.data | bit << (8 * col + row));
            }
        }

        std::uint64_t compressed_data() const  {
            constexpr std::uint64_t bit = 1;
            const BitBoard64 full = fullboard();
            std::uint64_t comp = black.data;
            comp |= bit << full.height(0);
            comp |= bit << (full.height(1) + 8);
            comp |= bit << (full.height(2) + 16);
            comp |= bit << (full.height(3) + 24);
            comp |= bit << (full.height(4) + 32);
            comp |= bit << (full.height(5) + 40);
            comp |= bit << (full.height(6) + 48);
            comp |= bit << (full.height(7) + 56);
            return comp;
        }

    public: // ================================================== PRINT OPERATOR

        friend std::ostream &operator<<(std::ostream &os, Position128 pos) {
            constexpr std::uint64_t bit = 1;
            for (unsigned j = 8; j > 0; --j) {
                for (unsigned i = 0; i < 8; ++i) {
                    const std::uint64_t newbit = bit << (8 * i + j - 1);
                    os << ((pos.white.data & newbit) ? 'W' :
                           (pos.black.data & newbit) ? 'B' : 'O');
                }
                os << '\n';
            }
            return os;
        }

    }; // class Position128

} // namespace dzc4

#endif // DZC4_POSITION128_HPP_INCLUDED
