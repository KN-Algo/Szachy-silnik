// Notation.h
#pragma once
#include <string>
#include <cctype>

namespace notation {

    /**
     * Funkcje pomocnicze do konwersji między notacją algebraiczną (np. "e4")
     * a współrzędnymi planszy (row, col) używanymi wewnętrznie przez silnik.
     */

    // Zamiana współrzędnych (row, col) -> notacja algebraiczna "a1..h8"
    inline std::string coordToAlg(int row, int col) {
        char file = 'a' + col;
        char rank = '8' - row;
        return std::string{file} + rank;
    }

    // Zamiana notacji algebraicznej "e2" -> współrzędne planszy (row, col)
    inline bool algToCoord(const std::string& s, int& row, int& col) {
        if (s.size() != 2) return false;
        char f = std::tolower(s[0]), r = s[1];
        if (f < 'a' || f > 'h' || r < '1' || r > '8') return false;
        col = f - 'a';
        row = '8' - r;
        return true;
    }

    // Dodatkowe konwersje pomocnicze dla pojedynczych znaków
    inline int fileToCol(char file) { return file - 'a'; }
    inline int rankToRow(char rank) { return '8' - rank; }

} // namespace notation
