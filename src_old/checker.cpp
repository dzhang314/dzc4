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

using dzc4::Player, dzc4::Evaluation;

int main() {

    constexpr unsigned ply = 10;

    dzc4::TableFileReader reader(ply);

    dzc4::CompressedPosition64 comp;
    int score;
    std::size_t total = 0;
    std::size_t num_unknown = 0;

    while (reader.read(comp, score)) {
        const dzc4::Position128 posn = comp.decompress();
        const Evaluation ev = ply % 2 == 0
            ? posn.eval<Player::WHITE, DEPTH + 4>()
            : posn.eval<Player::BLACK, DEPTH + 4>();
        switch (ev) {
            case Evaluation::WIN:
                dzc4::exit_if(score <= 0, "INCONSISTENT WIN ", total);
                break;
            case Evaluation::LOSS:
                dzc4::exit_if(score >= 0, "INCONSISTENT LOSS ", total);
                break;
            case Evaluation::DRAW:
                dzc4::exit_if(score != 0, "INCONSISTENT DRAW ", total);
                break;
            case Evaluation::UNKNOWN:
                ++num_unknown;
                break;
        }
        ++total;
    }
    std::cout << (total - num_unknown) << '/' << total << std::endl;

    return EXIT_SUCCESS;

}
