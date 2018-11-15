#ifndef DZC4_FILENAMES_HPP_INCLUDED
#define DZC4_FILENAMES_HPP_INCLUDED

// C++ standard library headers
#include <iomanip> // for std::setw and std::setfill
#include <ios>
#include <iostream> // for std::ifstream, std::ofstream
#include <filesystem>
#include <sstream> // for std::ostringstream
#include <string>
#include <vector>

// Project-specific headers
#include "Constants.hpp"
#include "Utilities.hpp"
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

        explicit DataFileWriter(const std::string &path_str) {
            data_path = path_str;
            assert_nonexistence(data_path);
            data_stream.open(data_path, std::ios::binary | std::ios::out
                                                         | std::ios::trunc);
            exit_if(data_stream.fail(),
                    "ERROR: Failed to create data file ", data_path, ".");
        }

        explicit DataFileWriter(unsigned ply) :
                DataFileWriter(plyfilename(ply)) {}

        explicit DataFileWriter(unsigned ply, unsigned chunk) :
                DataFileWriter(chunkfilename(ply, chunk)) {}

    public: // ======================================= STREAM INSERTION OPERATOR

        DataFileWriter &operator<<(CompressedPosition64 posn) {
            const char *posn_ptr = char_ptr_to(posn);
            data_stream.write(posn_ptr, sizeof(CompressedPosition64));
            exit_if(data_stream.fail(),
                    "ERROR: Failed to write to data file ", data_path, ".");
            return *this;
        }

        DataFileWriter &operator<<(
                const std::vector<CompressedPosition64> &posns) {
            const char * posns_ptr = char_ptr_to(posns.data());
            data_stream.write(posns_ptr,
                              posns.size() * sizeof(CompressedPosition64));
            exit_if(data_stream.fail(),
                    "ERROR: Failed to write to data file ", data_path, ".");
            return *this;
        }

    }; // class DataFileWriter



    class TableFileWriter {

    private: // =============================================== MEMBER VARIABLES

        std::filesystem::path table_path;
        std::ofstream table_stream;

    public: // ===================================================== CONSTRUCTOR

        explicit TableFileWriter(const std::string &path_str) {
            table_path = path_str;
            assert_nonexistence(table_path);
            table_stream.open(table_path, std::ios::binary | std::ios::out
                                                           | std::ios::trunc);
            exit_if(table_stream.fail(),
                    "ERROR: Failed to create table file ", table_path, ".");
        }

        explicit TableFileWriter(unsigned ply) :
                TableFileWriter(tabfilename(ply)) {}

    public: // ======================================= STREAM INSERTION OPERATOR

        TableFileWriter &write(CompressedPosition64 posn, int score) {
            const char *posn_ptr = char_ptr_to(posn);
            const signed char score_char = static_cast<signed char>(score);
            const char *score_ptr = char_ptr_to(score_char);
            table_stream.write(posn_ptr, sizeof(CompressedPosition64));
            table_stream.write(score_ptr, 1);
            exit_if(table_stream.fail(),
                    "ERROR: Failed to write to table file ", table_path, ".");
            return *this;
        }

    }; // class TableFileWriter



    class DataFileReader {

    private: // =============================================== MEMBER VARIABLES

        std::filesystem::path data_path;
        std::uintmax_t data_size;
        std::ifstream data_stream;

    public: // ===================================================== CONSTRUCTOR

        explicit DataFileReader(const std::string &path_str) {
            data_path = path_str;
            assert_file_exists(data_path);
            data_size = std::filesystem::file_size(data_path);
            exit_if(data_size % sizeof(CompressedPosition64) != 0,
                    "ERROR: Data file ", data_path, " is malformed.");
            data_size /= sizeof(CompressedPosition64);
            data_stream.open(data_path, std::ios::binary | std::ios::in);
            exit_if(data_stream.fail(),
                    "ERROR: Failed to open data file ", data_path, ".");
            std::cout << "Successfully opened data file " << data_path
                      << ". Found " << data_size << " positions." << std::endl;
        }

        explicit DataFileReader(unsigned ply) :
                DataFileReader(plyfilename(ply)) {}

        explicit DataFileReader(unsigned ply, unsigned chunk) :
                DataFileReader(chunkfilename(ply, chunk)) {}

    public: // =================================================== STATE TESTING

        operator bool() const { return !data_stream.fail(); }

        bool eof() const { return data_stream.eof(); }

        const std::filesystem::path &get_path() const { return data_path; }

    public: // ======================================= STREAM INSERTION OPERATOR

        DataFileReader &operator>>(CompressedPosition64 &posn) {
            char *posn_ptr = char_ptr_to(posn);
            data_stream.read(posn_ptr, sizeof(CompressedPosition64));
            return *this;
        }

    }; // class DataFileWriter

} // namespace dzc4

#endif // DZC4_FILENAMES_HPP_INCLUDED
