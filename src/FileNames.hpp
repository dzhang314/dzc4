#ifndef DZC4_FILENAMES_HPP_INCLUDED
#define DZC4_FILENAMES_HPP_INCLUDED

// C++ standard library headers
#include <iomanip> // for std::setw and std::setfill
#include <ios>
#include <iostream> // for std::ifstream, std::ofstream
#include <filesystem>
#include <sstream> // for std::ostringstream
#include <string>

// Project-specific headers
#include "Constants.hpp"
#include "CompressedPosition64.hpp"

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

namespace dzc4 {

    [[noreturn]] void error_exit() {
        std::cerr << std::endl;
        std::exit(EXIT_FAILURE);
    }

    template <typename T, typename... Ts>
    [[noreturn]] void error_exit(const T &msg, const Ts &... msgs) {
        std::cerr << msg;
        error_exit(msgs...);
    }

    template <typename... Ts>
    void exit_if(bool b, const Ts &... msgs) { if (b) error_exit(msgs...); }

    void assert_nonexistence(const std::filesystem::path &p) {
        exit_if(std::filesystem::exists(p),
                "ERROR: ", p, " already exists.");
    }

    void assert_file_exists(const std::filesystem::path &p) {
        const auto stat = std::filesystem::status(p);
        exit_if(!std::filesystem::exists(stat),
                "ERROR: ", p, " does not exist.");
        exit_if(!std::filesystem::is_regular_file(stat),
                "ERROR: ", p, " exists but is not a regular file.");
    }



    class DataFileWriter {

    private: // =============================================== MEMBER VARIABLES

        std::filesystem::path data_path;
        std::ofstream data_stream;

    public: // ===================================================== CONSTRUCTOR

        explicit DataFileWriter(unsigned ply) : data_path(plyfilename(ply)),
                                                data_stream() {
            assert_nonexistence(data_path);
            data_stream.open(data_path, std::ios::binary | std::ios::out
                                                         | std::ios::trunc);
            exit_if(data_stream.fail(),
                    "ERROR: Failed to create data file ", data_path, ".");
        }

    public: // ======================================= STREAM INSERTION OPERATOR

        DataFileWriter &operator<<(CompressedPosition64 pos) {
            const char *pos_ptr = char_ptr_to(pos);
            data_stream.write(pos_ptr, sizeof(CompressedPosition64));
            exit_if(data_stream.fail(),
                    "ERROR: Failed to write to data file ", data_path, ".");
            return *this;
        }

    }; // class DataFileWriter



    class DataFileReader {

    private: // =============================================== MEMBER VARIABLES

        std::filesystem::path data_path;
        std::uintmax_t data_size;
        std::ifstream data_stream;

    public: // ===================================================== CONSTRUCTOR

        explicit DataFileReader(const std::string &path_str) :
                data_path(path_str), data_stream() {
            assert_file_exists(data_path);
            data_size = std::filesystem::file_size(data_path);
            exit_if(data_size % sizeof(CompressedPosition64) != 0,
                    "ERROR: Data file ", data_path, " is malformed.");
            data_size /= sizeof(CompressedPosition64);
            data_stream.open(data_path, std::ios::binary | std::ios::in);
            exit_if(data_stream.fail(),
                    "ERROR: Failed to open data file ", data_path, ".");
            std::cout << "Successfully opened data file " << data_path
                      << ". Found " << data_size / sizeof(CompressedPosition64)
                      << " positions." << std::endl;
        }

        explicit DataFileReader(unsigned ply) :
                DataFileReader(plyfilename(ply)) {}

        explicit DataFileReader(unsigned ply, unsigned chunk) :
                DataFileReader(chunkfilename(ply, chunk)) {}

    public: // =================================================== STATE TESTING

        operator bool() const { return !data_stream.fail(); }

        bool eof() const { return data_stream.eof(); }

    public: // ======================================= STREAM INSERTION OPERATOR

        DataFileReader &operator>>(CompressedPosition64 &pos) {
            char *pos_ptr = char_ptr_to(pos);
            data_stream.read(pos_ptr, sizeof(CompressedPosition64));
            return *this;
        }

    }; // class DataFileWriter

} // namespace dzc4

#endif // DZC4_FILENAMES_HPP_INCLUDED
