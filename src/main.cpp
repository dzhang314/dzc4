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

void assert_nonexistence(const std::filesystem::path &p) {
    if (std::filesystem::exists(p)) {
        std::cerr << "ERROR: " << p << " already exists.\n";
        std::exit(EXIT_FAILURE);
    }
}

void assert_file_exists(const std::filesystem::path &p) {
    const auto stat = std::filesystem::status(p);
    if (!std::filesystem::exists(stat)) {
        std::cerr << "ERROR: " << p << " does not exist.\n";
        std::exit(EXIT_FAILURE);
    }
    if (!std::filesystem::is_regular_file(stat)) {
        std::cerr << "ERROR: " << p
                  << " exists but is not a regular file.\n";
        std::exit(EXIT_FAILURE);
    }
}

void zerostep() {
    const std::filesystem::path filepath(plyfilename(0));
    assert_nonexistence(filepath);
    std::ofstream plyfile(filepath, std::ios::binary);
    if (!plyfile) {
        std::cout << "Could not open ply file "
                  << filepath << "." << std::endl;
        std::exit(EXIT_FAILURE);
    }
    dzc4::CompressedPosition64 start_pos;
    const char *pos_ptr = char_ptr_to(start_pos);
    if (!plyfile.write(pos_ptr, sizeof(dzc4::CompressedPosition64))) {
        std::cout << "Failed to write to ply file "
                  << filepath << "." << std::endl;
        std::exit(EXIT_FAILURE);
    }
}

void printinfo(std::ifstream &infile,
               const std::string &filename) {
    if (infile) {
        std::cout << "Successfully opened input file "
                  << filename << "." << std::endl;
        // TODO: seekg() and tellg() are implementation-defined.
        // Is there a standard C++ way to get the size of a file?
        infile.seekg(0, infile.end);
        std::streampos endpos = infile.tellg();
        infile.seekg(0);
        std::streampos startpos = infile.tellg();
        if ((endpos - startpos) % sizeof(dzc4::CompressedPosition64) != 0) {
            std::cout << "Input file " << filename
                      << " is malformed." << std::endl;
            std::exit(EXIT_FAILURE);
        } else {
            std::cout << "Found "
                      << (endpos - startpos) / sizeof(dzc4::CompressedPosition64)
                      << " positions to expand." << std::endl;
        }
    } else {
        std::cout << "Failed to open input file "
                  << filename << "." << std::endl;
        std::exit(EXIT_FAILURE);
    }
}

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
    const std::string filename = plyfilename(ply);
    std::ifstream plyfile(filename, std::ios::binary);
    printinfo(plyfile, filename);
    dzc4::CompressedPosition64 pos;
    char *pos_ptr = char_ptr_to(pos);
    std::vector<std::uint64_t> posns;
    unsigned long long int count = 0;
    unsigned chunk = 0;
    if (ply % 2 == 0) {
        while (plyfile.read(pos_ptr, sizeof(dzc4::CompressedPosition64))) {
            for (unsigned col = 0; col < NUM_COLS; ++col) {
                if (const dzc4::Position128 newpos = pos.decompress().move<Player::WHITE>(col)) {
                    if (newpos.eval<Player::BLACK, DEPTH>() == Evaluation::UNKNOWN) {
                        posns.push_back(newpos.compressed_data());
                    }
                }
            }
            if (++count % CHUNK_SIZE == 0) {
                std::cout << "Expanded " << count << " positions." << std::endl;
                writechunk(posns, ply + 1, chunk++);
            }
        }
    } else {
        while (plyfile.read(pos_ptr, sizeof(dzc4::CompressedPosition64))) {
            for (unsigned col = 0; col < NUM_COLS; ++col) {
                if (const dzc4::Position128 newpos = pos.decompress().move<Player::BLACK>(col)) {
                    if (newpos.eval<Player::WHITE, DEPTH>() == Evaluation::UNKNOWN) {
                        posns.push_back(newpos.compressed_data());
                    }
                }
            }
            if (++count % CHUNK_SIZE == 0) {
                std::cout << "Expanded " << count
                          << " positions." << std::endl;
                writechunk(posns, ply + 1, chunk++);
            }
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

bool readnext(std::vector<std::ifstream> &chunkfiles,
              std::vector<std::uint64_t> &front,
              std::vector<std::ifstream>::size_type i) {
    auto front_ptr = char_ptr_to(front[i]);
    chunkfiles[i].read(front_ptr, sizeof(std::uint64_t));
    if (chunkfiles[i].eof()) {
        chunkfiles.erase(chunkfiles.begin() + i);
        front.erase(front.begin() + i);
        std::cout << "Closed chunk file." << std::endl;
        return false;
    } else if (chunkfiles[i]) {
        return true;
    } else {
        std::cout << "Error occurred when reading from chunk file."
                  << std::endl;
        std::exit(EXIT_FAILURE);
    }
}

void merge(std::vector<std::ifstream> &chunkfiles,
           std::ofstream &plyfile) {
    std::vector<std::uint64_t> front(chunkfiles.size(), 0);
    for (std::vector<std::ifstream>::size_type i = 0;
         i < chunkfiles.size(); ++i) {
        if (!readnext(chunkfiles, front, i)) { --i; }
    }
    std::uint64_t minpos;
    char *minpos_ptr = char_ptr_to(minpos);
    while (!chunkfiles.empty()) {
        minpos = *std::min_element(front.begin(), front.end());
        if (!plyfile.write(minpos_ptr, sizeof(std::uint64_t))) {
            std::cout << "Failed to write to ply file." << std::endl;
            std::exit(EXIT_FAILURE);
        }
        for (std::vector<std::ifstream>::size_type i = 0;
             i < chunkfiles.size(); ++i) {
            if (front[i] == minpos && !readnext(chunkfiles, front, i)) { --i; }
        }
    }
}

void mergestep(unsigned ply) {
    std::vector<std::ifstream> chunkfiles;
    unsigned chunk = 0;
    unsigned long long int total = 0;
    while (true) {
        const std::string filename = chunkfilename(ply, chunk++);
        chunkfiles.emplace_back(filename, std::ios::binary);
        if (chunkfiles.back()) {
            chunkfiles.back().seekg(0, chunkfiles.back().end);
            std::streampos end = chunkfiles.back().tellg();
            chunkfiles.back().seekg(0);
            std::streampos begin = chunkfiles.back().tellg();
            std::streamoff chunksize = end - begin;
            if (chunksize % sizeof(dzc4::CompressedPosition64) != 0) {
                std::cout << "Chunk file " << filename
                          << " is malformed." << std::endl;
            }
            total += static_cast<unsigned long long int>(
                    chunksize / sizeof(dzc4::CompressedPosition64));
        } else {
            chunkfiles.pop_back();
            break;
        }
    }
    const auto numchunkfiles = chunkfiles.size();
    if (chunkfiles.empty()) {
        std::cout << "Found no chunk files to merge." << std::endl;
        std::exit(EXIT_FAILURE);
    } else {
        std::cout << "Successfully opened " << chunkfiles.size()
                  << " chunk files." << std::endl;
        std::cout << "Found " << total << " positions to merge." << std::endl;
    }
    const std::string filename = plyfilename(ply);
    std::ofstream plyfile(filename, std::ios::binary);
    if (!plyfile) {
        std::cout << "Could not open ply file "
                  << filename << "." << std::endl;
        std::exit(EXIT_FAILURE);
    }
    merge(chunkfiles, plyfile);
    for (unsigned i = 0; i < numchunkfiles; ++i) {
        const std::string chunkname = chunkfilename(ply, i);
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
