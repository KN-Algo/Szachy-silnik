#include <iostream>
#include <string>
#include <cctype>
#include <optional>
#include "board.h"
#include "move.h"

static int fileToCol(char f) { return f - 'a'; } // a..h -> 0..7
static int rankToRow(char r) { return '8' - r; } // 8->0, 1->7
static bool inRange(int x) { return 0 <= x && x < 8; }

// e2e4 lub e7e8Qs
std::optional<Move> parseLAN(const std::string &s)
{
    if (s.size() != 4 && s.size() != 5)
        return std::nullopt;
    char f1 = std::tolower(s[0]), r1 = s[1], f2 = std::tolower(s[2]), r2 = s[3];
    if (f1 < 'a' || f1 > 'h' || f2 < 'a' || f2 > 'h')
        return std::nullopt;
    if (r1 < '1' || r1 > '8' || r2 < '1' || r2 > '8')
        return std::nullopt;

    Move m{};
    m.fromCol = fileToCol(f1);
    m.fromRow = rankToRow(r1);
    m.toCol = fileToCol(f2);
    m.toRow = rankToRow(r2);

    // Na tym etapie movedPiece i capturedPiece ustawi Board
    m.movedPiece = '?';
    m.capturedPiece = '.';
    m.promotion = 0;

    if (s.size() == 5)
    {
        char promo = std::toupper(s[4]); // N,B,R,Q
        if (promo == 'N' || promo == 'B' || promo == 'R' || promo == 'Q')
            m.promotion = promo;
        else
            return std::nullopt;
    }
    if (!inRange(m.fromCol) || !inRange(m.toCol) || !inRange(m.fromRow) || !inRange(m.toRow))
        return std::nullopt;

    return m;
}

int main()
{
    Board board;
    board.startBoard(); // ustawienie pozycji startowej
    // board.printBoard(); // (opcjonalnie) podgląd szachownicy

    std::cout << "Podaj ruch (np. e2e4, e7e8Q). 'quit' aby wyjsc.\n";

    std::string s;
    while (true)
    {
        std::cout << "> ";
        if (!(std::cin >> s))
            break;
        if (s == "quit" || s == "exit")
            break;

        auto m = parseLAN(s);
        if (!m)
        {
            std::cout << "Niepoprawny format ruchu.\n";
            continue;
        }

        if (board.isMoveValid(*m))
        {
            board.makeMove(*m);
            std::cout << "Ruch wykonany: " << s << "\n";
            board.printBoard(); // (opcjonalnie) podgląd po ruchu
        }
        else
        {
            std::cout << "Ruch nielegalny: " << s << "\n";
            std::cout << board.checkCastlingReason(*m) << "\n"; // dodane MS
        }
        
    }
    return 0;
}
