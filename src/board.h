//
// Created by jakub on 28.07.2025.
//

#pragma once
#include <string>
#include <vector>
#include "move.h"
#include "game_state.h"
#include "move_generator.h"

class Board {
private:
    char board[8][8] = {};
    char activeColor;
    std::string castling;
    std::string enPassant;
    int halfmoveClock;
    int fullmoveNumber;
    
    GameStateManager gameStateManager;
    
    bool isPathClear(int r1, int c1, int r2, int c2) const;
    bool isSquareAttacked(int row, int col, char byColor) const;

public:
    void startBoard();
    void printBoard() const;
    bool isMoveValid(const Move& move);
    void makeMove(const Move& move);
    
    std::vector<Move> getLegalMoves() const;
    bool hasLegalMoves() const;
    bool isInCheck() const;
    GameState getGameState() const;
    std::string getGameStateString() const;
};