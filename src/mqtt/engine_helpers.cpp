#include "chess/mqtt/engine_helpers.h"
#include <cstdlib>
#include <algorithm>
#include <cctype>
#include "chess/utils/Notation.h"

using namespace notation;

// ─────────────────────────────────────────────────────────────────────────────
// ENV z defaultem
std::string env_or(const char *key, const std::string &def)
{
    if (const char *v = std::getenv(key))
        return std::string(v);
    return def;
}

// Walidator „e2”
  bool valid_sq(const std::string &s)
{
    int r, c;
    return algToCoord(s, r, c);
}

// Konwersje pól
  std::pair<int, int> to_rc(const std::string &s)
{
    int r, c;
    algToCoord(s, r, c);
    return {r, c};
}
  std::string to_sq(int row, int col)
{
    return coordToAlg(row, col);
}

// Minimalny FEN z aktualnego Board (bez żadnych cudów)
 std::string fen_from_board(const Board &b)
{
    std::string out;
    for (int row = 0; row < 8; ++row)
    {
        int empty = 0;
        for (int col = 0; col < 8; ++col)
        {
            char p = b.board[row][col];
            if (p == 0)
            {
                empty++;
            }
            else
            {
                if (empty)
                {
                    out += std::to_string(empty);
                    empty = 0;
                }
                out += p;
            }
        }
        if (empty)
            out += std::to_string(empty);
        if (row != 7)
            out += '/';
    }
    out += ' ';
    out += (b.activeColor == 'w' ? 'w' : 'b');
    out += ' ';
    out += (b.castling.empty() ? "-" : b.castling);
    out += ' ';
    out += (b.enPassant.empty() ? "-" : b.enPassant);
    out += ' ';
    out += std::to_string(b.halfmoveClock);
    out += ' ';
    out += std::to_string(b.fullmoveNumber);
    return out;
}

// Lista TO z jednego FROM (z legalnych ruchów silnika)
 std::vector<std::string> possibleFrom(Board &board, const std::string &fromSq)
{
    std::vector<std::string> moves;
    if (!valid_sq(fromSq))
        return moves;

    int r0, c0;
    if (!algToCoord(fromSq, r0, c0))
        return moves;

    // Jeśli na polu nie ma naszej bierki – nie ma ruchów
    if (board.board[r0][c0] == 0)
        return moves;

    // Weź legalne ruchy z silnika (uwzględnia: szachy, roszady, EP, promocje)
    const auto legals = board.getLegalMoves();
    const char mover = board.board[r0][c0];
    const bool white = std::isupper(static_cast<unsigned char>(mover));

    for (const auto &m : legals)
    {
        if (m.fromRow == r0 && m.fromCol == c0)
        {
            char target = board.board[m.toRow][m.toCol];
            if (target && ((std::isupper(static_cast<unsigned char>(target)) != 0) == white))
                continue; // odfiltruj bicie własnych figur

            // opcjonalnie: dodatkowe sito
            if (!board.isMoveValid(m)) continue;

            moves.push_back(to_sq(m.toRow, m.toCol));
        }
    }

    std::sort(moves.begin(), moves.end());
    moves.erase(std::unique(moves.begin(), moves.end()), moves.end());
    return moves;
}


// Wypełnij Move (pola i ew. promocję)
 bool fill_move_from_to(Board &board, const std::string &from, const std::string &to, Move &out, char promo)
{
    if (!valid_sq(from) || !valid_sq(to))
        return false;
    int r1, c1, r2, c2;
    algToCoord(from, r1, c1);
    algToCoord(to, r2, c2);

    out.fromRow = r1;
    out.fromCol = c1;
    out.toRow = r2;
    out.toCol = c2;
    out.movedPiece = board.board[r1][c1];
    out.capturedPiece = board.board[r2][c2]; // przy EP cel jest pusty – to normalne
    out.promotion = promo;
    return true;
}

// Czy są inne figury tego samego typu (i koloru) mogące legalnie dojść na to samo pole?
 void san_disambiguation_before_move(
    const Board &board_before,
    const Move &m,                 // wykonywany ruch
    bool &needFile, bool &needRank // wynik: dopisać plik/rząd w SAN?
)
{
    needFile = false;
    needRank = false;

    const char moved = board_before.board[m.fromRow][m.fromCol];
    const bool isPawn = std::toupper(static_cast<unsigned char>(moved)) == 'P';
    if (isPawn)
        return; // piony w SAN nie potrzebują dysambiguacji (plik przy biciu już rozróżnia)

    // Zbierz wszystkie legalne ruchy z pozycji "przed ruchem"
    auto legals = const_cast<Board &>(board_before).getLegalMoves();

    // Szukamy innych ruchów na to samo pole (ta sama figura i kolor, inny "from")
    std::vector<std::pair<int, int>> colliders;
    for (const auto &x : legals)
    {
        if (x.toRow == m.toRow && x.toCol == m.toCol)
        {
            char p = board_before.board[x.fromRow][x.fromCol];
            if (p != 0 &&
                std::toupper(static_cast<unsigned char>(p)) == std::toupper(static_cast<unsigned char>(moved)) &&
                // ten sam kolor:
                (std::isupper(static_cast<unsigned char>(p)) == std::isupper(static_cast<unsigned char>(moved))) &&
                // inny start:
                !(x.fromRow == m.fromRow && x.fromCol == m.fromCol))
            {
                colliders.emplace_back(x.fromRow, x.fromCol);
            }
        }
    }
    if (colliders.empty())
        return; // brak konfliktu

    // Jeśli któryś ma inny plik (kolumnę), zwykle wystarczy podać plik.
    bool sameFileAll = true;
    bool sameRankAll = true;

    for (auto [r, c] : colliders)
    {
        if (c != m.fromCol)
            sameFileAll = false; // są różne pliki — plik rozróżni
        if (r != m.fromRow)
            sameRankAll = false; // są różne rzędy — rząd rozróżni
    }

    // Reguły SAN:
    // 1) jeśli plik rozróżnia jednoznacznie — podaj plik,
    // 2) else jeśli rząd rozróżnia — podaj rząd,
    // 3) else (nie rozróżnia ani plik, ani rząd) — podaj oba.
    // (Uwaga: sameFileAll==true oznacza: wszystkie kolidują W TYM SAMYM pliku -> plik NIE rozróżnia)
    bool fileDistinguishes = !sameFileAll;
    bool rankDistinguishes = !sameRankAll;

    if (fileDistinguishes)
    {
        needFile = true;
        needRank = false;
    }
    else if (rankDistinguishes)
    {
        needFile = false;
        needRank = true;
    }
    else
    {
        needFile = true;
        needRank = true;
    }
}
 std::string san_letter_for_piece(char piece)
{
    switch (std::toupper(static_cast<unsigned char>(piece)))
    {
    case 'N':
        return "N";
    case 'B':
        return "B";
    case 'R':
        return "R";
    case 'Q':
        return "Q";
    case 'K':
        return "K";
    default:
        return "";
    }
}
 std::string make_san_full(
    const Board &board_before, // pozycja PRZED ruchem
    const Move &m,             // ruch (z wypełnionym movedPiece/capturedPiece/promotion)
    bool isCastle, bool isCastleKingSide,
    bool isCapture, bool isEnPassant,
    bool isCheck, bool isMate)
{
    // Roszada
    if (isCastle)
        return isCastleKingSide ? "0-0" : "0-0-0";

    const char moved = m.movedPiece;
    const bool isPawn = std::toupper(static_cast<unsigned char>(moved)) == 'P';

    std::string san;

    // 1) litera figury (pion bez litery)
    if (!isPawn)
        san += san_letter_for_piece(moved);

    // 2) dysambiguacja (tylko dla figur innych niż pion)
    bool needFile = false, needRank = false;
    if (!isPawn)
        san_disambiguation_before_move(board_before, m, needFile, needRank);
    if (!isPawn)
    {
        if (needFile)
            san += static_cast<char>('a' + m.fromCol);
        if (needRank)
            san += static_cast<char>('1' + (7 - m.fromRow));
    }

    // 3) bicie (dla piona przy biciu SAN zaczyna się od pliku piona)
    if (isPawn && isCapture)
        san += static_cast<char>('a' + m.fromCol);
    if (isCapture)
        san += 'x';

    // 4) pole docelowe
    san += coordToAlg(m.toRow, m.toCol);

    // 5) promocja
    if (m.promotion && std::toupper(static_cast<unsigned char>(m.promotion)) != '?')
    {
        san += '=';
        san += static_cast<char>(std::toupper(static_cast<unsigned char>(m.promotion))); // =Q/R/B/N
    }

    // 6) sufiks + / #
    if (isMate)
        san += '#';
    else if (isCheck)
        san += '+';

    return san;
}

// Mapowanie char promocji -> nazwa dla backendu
 const char *promo_name(char p)
{
    switch (std::toupper(static_cast<unsigned char>(p)))
    {
    case 'Q':
        return "queen";
    case 'R':
        return "rook";
    case 'B':
        return "bishop";
    case 'N':
        return "knight";
    default:
        return "";
    }
}

 bool promo_char_from_name(std::string s, char& out) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c){ return std::tolower(c); });
    if      (s == "queen")  { out = 'Q'; return true; }
    else if (s == "rook")   { out = 'R'; return true; }
    else if (s == "bishop") { out = 'B'; return true; }
    else if (s == "knight") { out = 'N'; return true; }
    return false;
}

