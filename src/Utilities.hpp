#ifndef DZC4_UTILITIES_HPP_INCLUDED
#define DZC4_UTILITIES_HPP_INCLUDED

#include <cstdlib>    // for std::exit, EXIT_FAILURE
#include <filesystem> // for std::filesystem
#include <iostream>   // for std::cerr, std::endl
#include <memory>     // for std::addressof

namespace dzc4 {


template <typename T>
constexpr char *char_ptr_to(T &obj) {
    return static_cast<char *>(static_cast<void *>(std::addressof(obj)));
}


template <typename T>
constexpr const char *char_ptr_to(const T &obj) {
    return static_cast<const char *>(
        static_cast<const void *>(std::addressof(obj))
    );
}


template <typename T>
constexpr char *char_ptr_to(T *ptr) {
    return static_cast<char *>(static_cast<void *>(ptr));
}


template <typename T>
constexpr const char *char_ptr_to(const T *ptr) {
    return static_cast<const char *>(static_cast<const void *>(ptr));
}


[[noreturn]] inline void error_exit() {
    std::cerr << std::endl;
    std::exit(EXIT_FAILURE);
}


template <typename T, typename... Ts>
[[noreturn]] void error_exit(const T &message, const Ts &...messages) {
    std::cerr << message;
    error_exit(messages...);
}


template <typename... Ts>
void exit_if(bool condition, const Ts &...messages) {
    if (condition) error_exit(messages...);
}


inline void assert_nonexistence(const std::filesystem::path &p) {
    exit_if(std::filesystem::exists(p), "ERROR: ", p, " already exists.");
}


inline void assert_file_exists(const std::filesystem::path &p) {
    const auto status = std::filesystem::status(p);
    exit_if(!std::filesystem::exists(status), "ERROR: ", p, " does not exist.");
    exit_if(
        !std::filesystem::is_regular_file(status),
        "ERROR: ",
        p,
        " exists but is not a regular file."
    );
}


} // namespace dzc4

#endif // DZC4_UTILITIES_HPP_INCLUDED
