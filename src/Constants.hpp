#ifndef DZC4_CONSTANTS_HPP_INCLUDED
#define DZC4_CONSTANTS_HPP_INCLUDED

#include <cstddef> // for std::size_t

constexpr unsigned NUM_COLS = 6;
constexpr unsigned NUM_ROWS = 4;
constexpr unsigned DEPTH = 2;
constexpr std::size_t CHUNK_SIZE = 10000000;

constexpr const char *DATA_FILENAME_PREFIX = "/mnt/c/Data/C4DATA-";
constexpr const char *TABLE_FILENAME_PREFIX = "/mnt/c/Data/C4TABLE-";

// As in chess, white plays first.
enum class Player { WHITE, BLACK };
enum class Result { WIN, LOSS, DRAW, UNKNOWN };

constexpr Player other(Player p) {
    switch (p) {
        case Player::WHITE: return Player::BLACK;
        case Player::BLACK: return Player::WHITE;
    }
}

#endif // DZC4_CONSTANTS_HPP_INCLUDED
