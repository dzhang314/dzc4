#ifndef DZC4_POSITION_128_HPP_INCLUDED
#define DZC4_POSITION_128_HPP_INCLUDED

#include <climits> // for INT_MIN
#include <cstdint> // for std::uint64_t

#include "BitBoard64.hpp" // for BitBoard64

namespace dzc4 {


// As in chess, white goes first.
enum class Player { WHITE, BLACK };
enum class Evaluation { WIN, LOSS, DRAW, UNKNOWN };


constexpr Player other(Player player) noexcept {
    switch (player) {
        case Player::WHITE: return Player::BLACK;
        case Player::BLACK: return Player::WHITE;
    }
}


// Position128 is a 128-bit structure that represents a game state in Connect
// Four. It consists of a pair of BitBoard64s, one for each player's pieces.


struct Position128 {


    BitBoard64 white;
    BitBoard64 black;


    explicit constexpr Position128(
        const BitBoard64 &white_board, const BitBoard64 &black_board
    ) noexcept
        : white(white_board)
        , black(black_board) {}


    constexpr BitBoard64 full_board() const noexcept {
        return BitBoard64(white.data | black.data);
    }


    constexpr operator bool() const noexcept {
        return static_cast<bool>(white.data | black.data);
    }


    template <Player PLAYER, unsigned NUM_ROWS>
    constexpr Position128 move(unsigned col) const noexcept {
        constexpr std::uint64_t bit = 1;
        const unsigned row = full_board().height(col);
        if (row < NUM_ROWS) {
            const std::uint64_t new_piece = bit << (8 * col + row);
            if constexpr (PLAYER == Player::WHITE) {
                return Position128(BitBoard64(white.data | new_piece), black);
            } else if constexpr (PLAYER == Player::BLACK) {
                return Position128(white, BitBoard64(black.data | new_piece));
            } else {
                static_assert(false);
            }
        } else {
            return Position128(BitBoard64(0), BitBoard64(0));
        }
    }


    template <Player PLAYER>
    constexpr std::uint64_t won() const noexcept {
        if constexpr (PLAYER == Player::WHITE) {
            return white.won();
        } else if constexpr (PLAYER == Player::BLACK) {
            return black.won();
        } else {
            static_assert(false);
        }
    }


    template <
        Player PLAYER,
        unsigned NUM_ROWS,
        unsigned NUM_COLS,
        unsigned DEPTH>
    constexpr Evaluation evaluate() const noexcept {
        if (won<PLAYER>()) { return Evaluation::WIN; }
        if (won<other(PLAYER)>()) { return Evaluation::LOSS; }
        if constexpr (DEPTH == 0) {
            return Evaluation::UNKNOWN;
        } else {
            bool has_move = false;
            bool has_unknown = false;
            bool has_draw = false;
            for (unsigned col = 0; col < NUM_COLS; ++col) {
                if (const Position128 next = move<PLAYER, NUM_ROWS>(col)) {
                    has_move = true;
                    const Evaluation eval = next.evaluate<
                        other(PLAYER),
                        NUM_ROWS,
                        NUM_COLS,
                        DEPTH - 1>();
                    if (eval == Evaluation::LOSS) { return Evaluation::WIN; }
                    if (eval == Evaluation::UNKNOWN) { has_unknown = true; }
                    if (eval == Evaluation::DRAW) { has_draw = true; }
                }
            }
            return has_unknown               ? Evaluation::UNKNOWN
                   : (has_draw || !has_move) ? Evaluation::DRAW
                                             : Evaluation::LOSS;
        }
    }


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


    template <
        Player PLAYER,
        unsigned NUM_ROWS,
        unsigned NUM_COLS,
        unsigned DEPTH>
    constexpr int calculate_score() const noexcept {
        if (won<other(PLAYER)>()) { return -1; }
        if constexpr (DEPTH == 0) {
            return INT_MIN;
        } else {
            int best_negative = INT_MIN;
            int best_positive = 0;
            bool has_unknown = false;
            bool has_draw = false;
            for (unsigned col = 0; col < NUM_COLS; ++col) {
                if (const Position128 next = move<PLAYER, NUM_ROWS>(col)) {
                    const int score = next.calculate_score<
                        other(PLAYER),
                        NUM_ROWS,
                        NUM_COLS,
                        DEPTH - 1>();
                    if (score == -1) {
                        return +1;
                    } else if (score == INT_MIN) {
                        has_unknown = true;
                    } else if (score < 0) {
                        best_negative = std::max(best_negative, score);
                    } else if (score > 0) {
                        best_positive = std::max(best_positive, score);
                    } else {
                        has_draw = true;
                    }
                }
            }
            return (best_negative > INT_MIN) ? (1 - best_negative)
                   : has_unknown             ? INT_MIN
                   : has_draw                ? 0
                   : (best_positive > 0)     ? (-best_positive - 1)
                                             : 0;
        }
    }


}; // struct Position128


} // namespace dzc4

#endif // DZC4_POSITION_128_HPP_INCLUDED
