#ifndef DZC4_CONSTANTS_HPP_INCLUDED
#define DZC4_CONSTANTS_HPP_INCLUDED

#define NUM_COLS 7
#define NUM_ROWS 6
#define DEPTH    2

#define CHUNK_SIZE            10000000
#define DATA_FILENAME_PREFIX  "/mnt/h/C4DATA-"
#define TABLE_FILENAME_PREFIX "/mnt/h/C4TABLE-"

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
