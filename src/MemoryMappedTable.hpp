#ifndef DZC4_MEMORY_MAPPED_TABLE_HPP_INCLUDED
#define DZC4_MEMORY_MAPPED_TABLE_HPP_INCLUDED

#include <algorithm> // for std::max
#include <climits>   // for INT_MIN
#include <cstddef>   // for std::size_t
#include <iostream>  // for std::cerr, std::endl
#include <string>    // for std::string

#include <fcntl.h>    // for UNIX open, O_RDONLY
#include <sys/mman.h> // for UNIX mmap, munmap, PROT_READ, MAP_SHARED
#include <sys/stat.h> // for UNIX stat
#include <unistd.h>   // for UNIX close

#include "CompressedPosition64.hpp"
#include "Position128.hpp"
#include "Utilities.hpp"

namespace dzc4 {


struct MemoryMappedTable {


    static constexpr std::size_t POSITION_SIZE = sizeof(CompressedPosition64);
    static constexpr std::size_t ENTRY_SIZE = sizeof(CompressedPosition64) + 1;


    std::size_t file_size;
    std::size_t num_entries;
    int fd;
    char *data;


    explicit MemoryMappedTable(const char *path) {
        // Use UNIX stat to get size of table file.
        struct stat st;
        exit_if(
            stat(path, &st) == -1,
            "Error occurred while retrieving size of table file ",
            path,
            "."
        );
        // Save file size to member variables.
        file_size = static_cast<std::size_t>(st.st_size);
        num_entries = file_size / ENTRY_SIZE;
        // Use UNIX open to obtain file descriptor for table file.
        fd = open(path, O_RDONLY);
        exit_if(
            fd == -1, "Error occurred while opening table file ", path, "."
        );
        // Use UNIX mmap to load table file into memory.
        data = static_cast<char *>(
            mmap(nullptr, file_size, PROT_READ, MAP_SHARED, fd, 0)
        );
        exit_if(
            data == MAP_FAILED,
            "Error occurred while memory-mapping table file ",
            path,
            "."
        );
    }


    explicit MemoryMappedTable(const std::string &path)
        : MemoryMappedTable(path.c_str()) {}


    ~MemoryMappedTable() {
        if (munmap(data, file_size) == -1) {
            std::cerr << "Warning: error occurred while unmapping table file."
                      << std::endl;
        }
        if (close(fd) == -1) {
            std::cerr << "Warning: error occurred while closing table file."
                      << std::endl;
        }
    }


    CompressedPosition64 get_position(std::size_t index) {
        return *static_cast<CompressedPosition64 *>(
            static_cast<void *>(data + ENTRY_SIZE * index)
        );
    }


    int get_score(std::size_t index) {
        return static_cast<int>(*static_cast<signed char *>(
            static_cast<void *>(data + ENTRY_SIZE * index + POSITION_SIZE)
        ));
    }


    template <
        Player PLAYER,
        unsigned NUM_ROWS,
        unsigned NUM_COLS,
        unsigned DEPTH>
    int lookup_score(const Position128 &position) {
        const CompressedPosition64 compressed(position);
        std::size_t lower_index = 0;
        std::size_t upper_index = num_entries - 1;
        while (true) {
            if (lower_index > upper_index) {
                const int score =
                    position
                        .calculate_score<PLAYER, NUM_ROWS, NUM_COLS, DEPTH + 1>(
                        );
                exit_if(score == INT_MIN, "ERROR: Inconclusive search.");
                return score;
            }
            const std::size_t middle_index =
                lower_index + (upper_index - lower_index) / 2;
            const CompressedPosition64 center = get_position(middle_index);
            if (center < compressed) {
                lower_index = middle_index + 1;
            } else if (center > compressed) {
                upper_index = middle_index - 1;
            } else {
                return get_score(middle_index);
            }
        }
    }


    template <
        Player PLAYER,
        unsigned NUM_ROWS,
        unsigned NUM_COLS,
        unsigned DEPTH>
    int evaluate(const CompressedPosition64 &position) {
        const Position128 decompressed = position.decompress();
        if (decompressed.won<other(PLAYER)>()) { return -1; }
        int best_negative = INT_MIN;
        int best_positive = 0;
        bool has_draw = false;
        for (unsigned col = 0; col < NUM_COLS; ++col) {
            if (const Position128 next =
                    decompressed.move<PLAYER, NUM_ROWS>(col)) {
                const int score =
                    lookup_score<other(PLAYER), NUM_ROWS, NUM_COLS, DEPTH>(next
                    );
                if (score == -1) {
                    return +1;
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
               : has_draw                ? 0
               : (best_positive > 0)     ? (-best_positive - 1)
                                         : 0;
    }


}; // struct MemoryMappedTable


} // namespace dzc4

#endif // DZC4_MEMORY_MAPPED_TABLE_HPP_INCLUDED
