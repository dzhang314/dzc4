#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>

#include "Constants.hpp"
#include "FileNames.hpp"
#include "BitBoard64.hpp"
#include "Position128.hpp"
#include "MemoryMappedTable.hpp"

void writechunk(std::vector<dzc4::CompressedPosition64> &posns,
                unsigned ply, unsigned chunk) {
    std::sort(posns.begin(), posns.end());
    posns.erase(std::unique(posns.begin(), posns.end()), posns.end());
    dzc4::DataFileWriter(ply, chunk) << posns;
    posns.clear();
}

using dzc4::Player, dzc4::Evaluation;

void chunkstep(unsigned ply) {
    dzc4::DataFileReader reader(ply);
    dzc4::CompressedPosition64 posn;
    std::vector<dzc4::CompressedPosition64> posns;
    unsigned long long int count = 0;
    unsigned chunk = 0;
    while (reader >> posn) {
        for (unsigned col = 0; col < NUM_COLS; ++col) {
            if (const dzc4::Position128 next_posn = ply % 2 == 0
                    ? posn.decompress().move<Player::WHITE>(col)
                    : posn.decompress().move<Player::BLACK>(col)) {
                const Evaluation ev = ply % 2 == 0
                    ? next_posn.eval<Player::BLACK, DEPTH>()
                    : next_posn.eval<Player::WHITE, DEPTH>();
                if (ev == Evaluation::UNKNOWN) posns.emplace_back(next_posn);
            }
        }
        if (++count % CHUNK_SIZE == 0) {
            std::cout << "Expanded " << count << " positions." << std::endl;
            writechunk(posns, ply + 1, chunk++);
        }
    }
    if (!posns.empty()) {
        std::cout << "Expanded " << count << " positions." << std::endl;
        writechunk(posns, ply + 1, chunk);
    }
}

// ========================================================================== //
// |||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||| //
// ========================================================================== //

bool readnext(std::vector<dzc4::DataFileReader> &chunk_readers,
              std::vector<dzc4::CompressedPosition64> &front_buffer,
              std::size_t index) {
    chunk_readers[index] >> front_buffer[index];
    if (chunk_readers[index].eof()) {
        std::cout << "Closed chunk file" << chunk_readers[index].get_path()
                  << "." << std::endl;
        chunk_readers.erase(chunk_readers.begin() + index);
        front_buffer.erase(front_buffer.begin() + index);
        return false;
    } else if (chunk_readers[index]) return true;
    else dzc4::exit_if(true, "Error occurred when reading from chunk file.");
}

void merge(std::vector<dzc4::DataFileReader> &chunkfiles,
           dzc4::DataFileWriter &plyfile) {
    std::vector<dzc4::CompressedPosition64> front(chunkfiles.size());
    for (std::size_t i = 0; i < chunkfiles.size(); ++i) {
        if (!readnext(chunkfiles, front, i)) --i;
    }
    while (!chunkfiles.empty()) {
        const dzc4::CompressedPosition64 minpos =
            *std::min_element(front.begin(), front.end());
        plyfile << minpos;
        for (std::size_t i = 0; i < chunkfiles.size(); ++i) {
            if (front[i] == minpos && !readnext(chunkfiles, front, i)) --i;
        }
    }
}

unsigned count_chunks(unsigned ply) {
    for (unsigned count = 0; true; ++count) {
        const std::filesystem::path chunk_path = chunkfilename(ply, count);
        const auto stat = std::filesystem::status(chunk_path);
        if (!std::filesystem::exists(stat)
            || !std::filesystem::is_regular_file(stat)) return count;
    }
}

void mergestep(unsigned ply) {
    std::vector<dzc4::DataFileReader> chunkfiles;
    const unsigned count = count_chunks(ply);
    dzc4::exit_if(count == 0, "ERROR: Found no chunk files to merge.");
    // TODO: Track total?
    for (unsigned chunk = 0; chunk < count; ++chunk) {
        chunkfiles.emplace_back(ply, chunk);
    }
    dzc4:: DataFileWriter writer(ply);
    merge(chunkfiles, writer);
    for (unsigned chunk = 0; chunk < count; ++chunk) {
        const std::string chunkname = chunkfilename(ply, chunk);
        std::remove(chunkname.c_str());
    }
}

// ========================================================================== //
// |||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||| //
// ========================================================================== //

void endstep() {
    constexpr unsigned ply = NUM_ROWS * NUM_COLS - DEPTH;
    {
        dzc4::DataFileReader reader(ply);
        dzc4::TableFileWriter writer(ply);
        dzc4::CompressedPosition64 posn;
        unsigned long long int count = 0;
        while (reader >> posn) {
            const int score = ply % 2 == 0
                    ? posn.decompress().score<Player::WHITE, DEPTH + 1>()
                    : posn.decompress().score<Player::BLACK, DEPTH + 1>();
            writer.write(posn, score);
            if (++count % CHUNK_SIZE == 0) {
                std::cout << "Evaluated " << count
                            << " positions." << std::endl;
            }
        }
        std::cout << "Evaluated " << count << " positions." << std::endl;
    }
    const std::string plyname = plyfilename(ply);
    std::remove(plyname.c_str());
}

void backstep(unsigned ply) {
    std::cout << "Back-propagating from ply " << ply
              << " to ply " << ply - 1 << "." << std::endl;
    {
        dzc4::DataFileReader reader(ply - 1);
        dzc4::TableFileWriter writer(ply - 1);
        dzc4::MemoryMappedTable tabfile(tabfilename(ply));
        // TODO: Check and handle file opening errors.
        dzc4::CompressedPosition64 posn;
        unsigned long long int count = 0;
        while (reader >> posn) {
            const int score = ply % 2 == 0
                ? tabfile.eval<Player::BLACK>(posn)
                : tabfile.eval<Player::WHITE>(posn);
            writer.write(posn, score);
            if (++count % CHUNK_SIZE == 0) {
                std::cout << "Evaluated " << count
                            << " positions." << std::endl;
            }
        }
        std::cout << "Evaluated " << count << " positions." << std::endl;
    }
    const std::string plyname = plyfilename(ply - 1);
    std::remove(plyname.c_str());
}

// ========================================================================== //
// |||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||| //
// ========================================================================== //

int main() {

    dzc4::DataFileWriter(0) << dzc4::CompressedPosition64();

    for (unsigned ply = 0; ply < NUM_ROWS * NUM_COLS - DEPTH; ++ply) {
        chunkstep(ply);
        mergestep(ply + 1);
    }

    endstep();

    for (unsigned ply = NUM_ROWS * NUM_COLS - DEPTH; ply > 0; --ply) {
        backstep(ply);
    }

    // for (unsigned ply = 3; ply > 0; --ply) {
    //     dzc4::MemoryMappedTable table(tabfilename(ply - 1));
    //     for (std::size_t i = 0; i < table.num_entries; ++i) {
    //         std::cout << table.getpos(i) << std::flush;
    //         std::cout << table.getres(i) << std::endl;
    //     }
    // }

    return EXIT_SUCCESS;

}
