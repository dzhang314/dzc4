#ifndef DZC4_UTILITIES_HPP_INCLUDED
#define DZC4_UTILITIES_HPP_INCLUDED

// C++ standard library headers
#include <iostream> // for std::cerr, std::endl
#include <memory> // for std::addressof

namespace dzc4 {

    template <typename T>
    constexpr char *char_ptr_to(T &obj) {
        return static_cast<char *>(static_cast<void *>(std::addressof(obj)));
    }

    template <typename T>
    constexpr char *char_ptr_to(T *ptr) {
        return static_cast<char *>(static_cast<void *>(ptr));
    }

    template <typename T>
    constexpr const char *char_ptr_to(const T &obj) {
        return static_cast<const char *>(static_cast<const void *>(
            std::addressof(obj)));
    }

    template <typename T>
    constexpr const char *char_ptr_to(const T *ptr) {
        return static_cast<const char *>(static_cast<const void *>(ptr));
    }



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

} // namespace dzc4

#endif // DZC4_UTILITIES_HPP_INCLUDED
