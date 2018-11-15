#ifndef DZC4_CONSTANTS_HPP_INCLUDED
#define DZC4_CONSTANTS_HPP_INCLUDED

// C++ standard library headers
#include <cstddef> // for std::size_t

constexpr unsigned NUM_COLS = 6;
constexpr unsigned NUM_ROWS = 4;

static_assert(NUM_COLS <= 8, "dzc4 only supports Connect Four boards with "
                             "up to eight columns.");
static_assert(NUM_ROWS <= 7, "dzc4 only supports Connect Four boards with "
                             "up to seven rows.");

constexpr unsigned DEPTH = 2;
constexpr std::size_t CHUNK_SIZE = 10000000;

constexpr const char *DATA_FILENAME_PREFIX = "/mnt/c/Data/C4DATA-";
constexpr const char *TABLE_FILENAME_PREFIX = "/mnt/c/Data/C4TABLE-";

#endif // DZC4_CONSTANTS_HPP_INCLUDED
