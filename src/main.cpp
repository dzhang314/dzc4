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

void zerostep() { dzc4::DataFileWriter(0) << dzc4::CompressedPosition64(); }

void writechunk(std::vector<std::uint64_t> &v,
                unsigned ply, unsigned chunk) {
    std::sort(v.begin(), v.end());
    v.erase(std::unique(v.begin(), v.end()), v.end());
    const std::string filename = chunkfilename(ply, chunk);
    std::ofstream chunk_file(filename, std::ios::binary);
    if (chunk_file) {
        std::cout << "Writing chunk file " << filename << "." << std::endl;
        auto v_ptr = char_ptr_to(v.data());
        chunk_file.write(v_ptr, v.size() * sizeof(std::uint64_t));
        if (chunk_file) {
            std::cout << "Successfully wrote chunk file "
                      << filename << "." << std::endl;
            v.clear();
        } else {
            std::cout << "Could not write to chunk file "
                      << filename << "." << std::endl;
            std::exit(EXIT_FAILURE);
        }
    } else {
        std::cout << "Could not open chunk file "
                  << filename << "." << std::endl;
        std::exit(EXIT_FAILURE);
    }
}

void chunkstep(unsigned ply) {
    dzc4::DataFileReader reader(ply);
    dzc4::CompressedPosition64 pos;
    std::vector<std::uint64_t> posns;
    unsigned long long int count = 0;
    unsigned chunk = 0;
    while (reader >> pos) {
        for (unsigned col = 0; col < NUM_COLS; ++col) {
            if (ply % 2 == 0) {
                if (const dzc4::Position128 newpos =
                        pos.decompress().move<Player::WHITE>(col)) {
                    if (newpos.eval<Player::BLACK, DEPTH>() == Evaluation::UNKNOWN) {
                        posns.push_back(newpos.compressed_data());
                    }
                }
            } else {
                if (const dzc4::Position128 newpos =
                        pos.decompress().move<Player::BLACK>(col)) {
                    if (newpos.eval<Player::WHITE, DEPTH>() == Evaluation::UNKNOWN) {
                        posns.push_back(newpos.compressed_data());
                    }
                }
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
        chunk_readers.erase(chunk_readers.begin() + index);
        front_buffer.erase(front_buffer.begin() + index);
        std::cout << "Closed chunk file." << std::endl;
        return false;
    } else if (chunk_readers[index]) {
        return true;
    } else {
        std::cout << "Error occurred when reading from chunk file."
                  << std::endl;
        std::exit(EXIT_FAILURE);
    }
}

void merge(std::vector<dzc4::DataFileReader> &chunkfiles,
           dzc4::DataFileWriter &plyfile) {
    std::vector<dzc4::CompressedPosition64> front(chunkfiles.size());
    for (std::size_t i = 0; i < chunkfiles.size(); ++i) {
        if (!readnext(chunkfiles, front, i)) { --i; }
    }
    while (!chunkfiles.empty()) {
        const dzc4::CompressedPosition64 minpos =
            *std::min_element(front.begin(), front.end());
        plyfile << minpos;
        for (std::size_t i = 0; i < chunkfiles.size(); ++i) {
            if (front[i] == minpos && !readnext(chunkfiles, front, i)) { --i; }
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
    std::cout << "Successfully opened " << count
              << " chunk files." << std::endl;
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
    unsigned ply = NUM_ROWS * NUM_COLS - DEPTH;
    const std::string plyname = plyfilename(ply);
    const std::string resname = tabfilename(ply);
    {
        std::ifstream plyfile(plyname, std::ios::binary);
        std::ofstream resfile(resname, std::ios::binary);
        // TODO: Check and handle file opening errors.
        dzc4::CompressedPosition64 pos;
        auto pos_ptr = static_cast<char *>(static_cast<void *>(&pos));
        unsigned long long int count = 0;
        if (ply % 2 == 0) {
            while (plyfile.read(pos_ptr, sizeof(dzc4::CompressedPosition64))) {
                const signed char result = static_cast<signed char>(
                        pos.decompress().score<Player::WHITE, DEPTH + 1>());
                auto res_ptr = static_cast<const char *>(
                        static_cast<const void *>(&result));
                resfile.write(pos_ptr, sizeof(dzc4::CompressedPosition64));
                resfile.write(res_ptr, sizeof(signed char));
                if (++count % CHUNK_SIZE == 0) {
                    std::cout << "Evaluated " << count
                              << " positions." << std::endl;
                }
            }
        } else {
            while (plyfile.read(pos_ptr, sizeof(dzc4::CompressedPosition64))) {
                const signed char result = static_cast<signed char>(
                        pos.decompress().score<Player::BLACK, DEPTH + 1>());
                auto res_ptr = static_cast<const char *>(
                        static_cast<const void *>(&result));
                resfile.write(pos_ptr, sizeof(dzc4::CompressedPosition64));
                resfile.write(res_ptr, sizeof(signed char));
                if (++count % CHUNK_SIZE == 0) {
                    std::cout << "Evaluated " << count
                              << " positions." << std::endl;
                }
            }
        }
        std::cout << "Evaluated " << count << " positions." << std::endl;
    }
    std::remove(plyname.c_str());
}

void backstep(unsigned ply) {
    std::cout << "Back-propagating from ply " << ply
              << " to ply " << ply - 1 << "." << std::endl;
    const std::string plyname = plyfilename(ply - 1);
    const std::string resname = tabfilename(ply - 1);
    const std::string tabname = tabfilename(ply);
    {
        std::ifstream plyfile(plyname, std::ios::binary);
        std::ofstream resfile(resname, std::ios::binary);
        dzc4::MemoryMappedTable tabfile(tabname);
        // TODO: Check and handle file opening errors.
        dzc4::CompressedPosition64 pos;
        auto pos_ptr = static_cast<char *>(static_cast<void *>(&pos));
        unsigned long long int count = 0;
        if (ply % 2 == 0) {
            while (plyfile.read(pos_ptr, sizeof(dzc4::CompressedPosition64))) {
                const auto result = static_cast<signed char>(tabfile.eval<Player::BLACK>(pos));
                auto res_ptr = static_cast<const char *>(static_cast<const void *>(&result));
                resfile.write(pos_ptr, sizeof(dzc4::CompressedPosition64));
                resfile.write(res_ptr, sizeof(signed char));
                if (++count % CHUNK_SIZE == 0) {
                    std::cout << "Evaluated " << count
                              << " positions." << std::endl;
                }
            }
        } else {
            while (plyfile.read(pos_ptr, sizeof(dzc4::CompressedPosition64))) {
                const auto result = static_cast<signed char>(tabfile.eval<Player::WHITE>(pos));
                auto res_ptr = static_cast<const char *>(static_cast<const void *>(&result));
                resfile.write(pos_ptr, sizeof(dzc4::CompressedPosition64));
                resfile.write(res_ptr, sizeof(signed char));
                if (++count % CHUNK_SIZE == 0) {
                    std::cout << "Evaluated " << count
                              << " positions." << std::endl;
                }
            }
        }
        std::cout << "Evaluated " << count << " positions." << std::endl;
    }
    std::remove(plyname.c_str());
}

// ========================================================================== //
// |||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||| //
// ========================================================================== //

int main() {

    zerostep();

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
