// s
//  Created by jakub on 28.07.2025.
//

#pragma once
#include <string>
#include "move.h"
class Board
{
private:
    char board[8][8] = {};
    char activeColor;
    std::string castling;
    std::string enPassant;
    int halfmoveClock;
    int fullmoveNumber;

    bool isPathClear(int r1, int c1, int r2, int c2) const;
    bool isSquareAttacked(int row, int col, char byColor) const;

public:
    void startBoard();
    void printBoard() const;
    bool isMoveValid(const Move &move);
    void makeMove(const Move &move);
    std::string checkCastlingReason(const Move &move) const; // To dodane
};

//
// Created by  chatGPT Macieja Soroki
//
// board.h
