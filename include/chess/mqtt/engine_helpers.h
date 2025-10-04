#pragma once
#include <string>
#include <vector>
#include "chess/board/Board.h"
#include "chess/model/Move.h"

// Konwersje i walidacja
bool valid_sq(const std::string &s);
std::pair<int, int> to_rc(const std::string &s);
std::string to_sq(int row, int col);

// Generacja ruchów z danego pola
std::vector < std::string> possibleFrom(Board &board, const std::string &fromSq);

// FEN generowany z aktualnej planszy
std::string fen_from_board(const Board &b);

// Wypełnienie obiektu Move
bool fill_move_from_to(Board &board, const std::string &from, const std::string &to, Move &out, char promo = 0);

// Konwersje promocji
const char* promo_name(char p);
bool promo_char_from_name(std::string s, char &out);

// Generacja zapisu SAN (pełna notacja)
std::string make_san_full(
    const Board &board_before,
    const Move &m,
    bool isCastle, bool isCastleKingSide,
    bool isCapture, bool isEnPassant,
    bool isCheck, bool isMate);

// Pomocnicze: odczyt ENV z domyślną wartością
std::string env_or(const char* key, const std::string &def);
