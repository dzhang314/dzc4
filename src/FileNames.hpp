#ifndef DZC4_FILENAMES_HPP_INCLUDED
#define DZC4_FILENAMES_HPP_INCLUDED

// C++ standard library headers
#include <iomanip> // for std::setw and std::setfill
#include <filesystem>
#include <sstream> // for std::ostringstream
#include <string>

// Project-specific headers
#include "Constants.hpp"

inline std::string plyfilename(unsigned ply) {
    std::ostringstream filename;
    filename << DATA_FILENAME_PREFIX;
    filename << std::setw(2) << std::setfill('0') << NUM_COLS;
    filename << '-';
    filename << std::setw(2) << std::setfill('0') << NUM_ROWS;
    filename << '-';
    filename << std::setw(4) << std::setfill('0') << ply;
    return filename.str();
}

inline std::string tabfilename(unsigned ply) {
    std::ostringstream filename;
    filename << TABLE_FILENAME_PREFIX;
    filename << std::setw(2) << std::setfill('0') << NUM_COLS;
    filename << '-';
    filename << std::setw(2) << std::setfill('0') << NUM_ROWS;
    filename << '-';
    filename << std::setw(4) << std::setfill('0') << ply;
    return filename.str();
}

inline std::string chunkfilename(unsigned ply, unsigned chunk) {
    std::ostringstream filename;
    filename << DATA_FILENAME_PREFIX;
    filename << std::setw(2) << std::setfill('0') << NUM_COLS;
    filename << '-';
    filename << std::setw(2) << std::setfill('0') << NUM_ROWS;
    filename << '-';
    filename << std::setw(4) << std::setfill('0') << ply;
    filename << '-';
    filename << std::setw(8) << std::setfill('0') << chunk;
    return filename.str();
}

#endif // DZC4_FILENAMES_HPP_INCLUDED
