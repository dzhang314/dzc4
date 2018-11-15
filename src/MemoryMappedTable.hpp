#ifndef DZC4_MEMORY_MAPPED_TABLE_HPP_INCLUDED
#define DZC4_MEMORY_MAPPED_TABLE_HPP_INCLUDED

// C++ standard library headers
#include <climits>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <string>

// UNIX system headers
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

// x86 intrinsic headers
#include <x86intrin.h>

// Project-specific headers
#include "Constants.hpp"
#include "Position128.hpp"
#include "CompressedPosition64.hpp"

namespace dzc4 {

    struct MemoryMappedTable {

        static constexpr std::size_t CPOS_SIZE = sizeof(CompressedPosition64);
        static constexpr std::size_t ENTRY_SIZE = sizeof(CompressedPosition64) + 1;

        int fd;
        std::size_t size;
        std::size_t num_entries;
        char *data;

        explicit MemoryMappedTable(const char *fname) {
            struct stat st;
            if (stat(fname, &st) == -1) {
                std::cerr << "Error occurred while retrieving size of table file "
                          << fname << "." << std::endl;
                std::exit(EXIT_FAILURE);
            }
            size = static_cast<std::size_t>(st.st_size);
            num_entries = size / ENTRY_SIZE;
            fd = open(fname, O_RDONLY);
            if (fd == -1) {
                std::cerr << "Error occurred while opening table file "
                          << fname << "." << std::endl;
                std::exit(EXIT_FAILURE);
            }
            data = static_cast<char *>(mmap(
                nullptr, size, PROT_READ, MAP_SHARED, fd, 0));
        }

        explicit MemoryMappedTable(const std::string &fname) :
                MemoryMappedTable(fname.c_str()) {}

        ~MemoryMappedTable() {
            if (munmap(data, size) == -1) {
                std::cerr << "Warning: error occurred while "
                             "unmapping table file." << std::endl;
            }
            if (close(fd) == -1) {
                std::cerr << "Warning: error occurred while "
                             "closing table file." << std::endl;
            }
        }

        CompressedPosition64 getpos(off_t idx) {
            void *data_ptr = data + ENTRY_SIZE * idx;
            return *static_cast<CompressedPosition64 *>(data_ptr);
        }

        int getres(off_t idx) {
            void *data_ptr = data + (ENTRY_SIZE * idx + CPOS_SIZE);
            return static_cast<int>(*static_cast<signed char *>(data_ptr));
        }

        template <Player player>
        int find(Position128 pos) {
            const std::uint64_t comp = pos.compressed_data();
            off_t start = 0, stop = num_entries - 1;
            while (true) {
                if (start > stop) {
                    const int score = pos.score<player, DEPTH + 1>();
                    if (score == INT_MIN) {
                        std::cerr << "Error: inconclusive search" << std::endl;
                        std::exit(EXIT_FAILURE);
                    } else return score;
                }
                const off_t mid = start + (stop - start) / 2;
                const CompressedPosition64 midpos = getpos(mid);
                if      (midpos.data == comp) return getres(mid);
                else if (midpos.data > comp)  stop = mid - 1;
                else                          start = mid + 1;
            }
        }

        template <Player player>
        int eval(CompressedPosition64 comp) {
            const Position128 posn = comp.decompress();
            if (posn.won<other(player)>()) return -1;
            int neg = INT_MIN, pos = 0;
            bool candraw = false;
            for (unsigned col = 0; col < NUM_COLS; ++col) {
                if (const Position128 newpos = posn.move<player>(col)) {
                    const int newres = find<other(player)>(newpos);
                    if (newres == -1)    return +1;
                    else if (newres < 0) neg = std::max(neg, newres);
                    else if (newres > 0) pos = std::max(pos, newres);
                    else                 candraw = true;
                }
            }
            return neg > INT_MIN ? 1 - neg  :
                   candraw       ? 0        :
                   pos > 0       ? -pos - 1 : 0;
        }

    }; // struct MemoryMappedTable

} // namespace dzc4

#endif // DZC4_MEMORY_MAPPED_TABLE_HPP_INCLUDED
