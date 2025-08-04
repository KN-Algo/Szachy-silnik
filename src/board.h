//
// Created by jakub on 28.07.2025.
//

#pragma once
#include <string>

class Board {
private:
    char board[8][8] = {};
    char activeColor;
    std::string castling;
    std::string enPassant;
    int halfmoveClock;
    int fullmoveNumber;

public:
    void startBoard();
    void printBoard() const;
};
