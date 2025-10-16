#pragma once
#include "chess/board/Board.h"

namespace Evaluator {
    // Ocena pozycji z perspektywy białych (pozytywna = białe lepsze, negatywna = czarne lepsze)
    int evaluatePosition(const char board[8][8], char activeColor);
    
    // Wartości figur w centipawnach
    constexpr int PAWN_VALUE = 100;
    constexpr int KNIGHT_VALUE = 320;
    constexpr int BISHOP_VALUE = 330;
    constexpr int ROOK_VALUE = 500;
    constexpr int QUEEN_VALUE = 900;
    constexpr int KING_VALUE = 20000;
    
    // Bonusy pozycyjne
    constexpr int CENTER_CONTROL_BONUS = 30;
    constexpr int PAWN_STRUCTURE_BONUS = 10;
    constexpr int KING_SAFETY_BONUS = 5;
    constexpr int MOBILITY_BONUS = 10;
    constexpr int BISHOP_PAIR_BONUS = 50;
    constexpr int ROOK_OPEN_FILE_BONUS = 20;
    constexpr int ROOK_SEMI_OPEN_FILE_BONUS = 10;
    constexpr int DEVELOPMENT_BONUS = 15;
    
    // Piece-Square Tables (dla białych - będą odwracane dla czarnych)
    extern const int PAWN_PST[8][8];
    extern const int KNIGHT_PST[8][8];
    extern const int BISHOP_PST[8][8];
    extern const int ROOK_PST[8][8];
    extern const int QUEEN_PST[8][8];
    extern const int KING_MIDDLEGAME_PST[8][8];
    extern const int KING_ENDGAME_PST[8][8];
    
    // Funkcje pomocnicze
    int evaluatePawnStructure(const char board[8][8]);
    int evaluateCenterControl(const char board[8][8]);
    int evaluateKingSafety(const char board[8][8]);
    int evaluateMobility(const char board[8][8], char activeColor);
    int evaluatePieceSquareTables(const char board[8][8]);
    int evaluateDevelopment(const char board[8][8]);
    bool isEndgame(const char board[8][8]);
    
    // Funkcje dla końcówek
    int getMaterialBalance(const char board[8][8]);
    int evaluateEndgame(const char board[8][8]);
    int manhattanDistance(int r1, int c1, int r2, int c2);
    int getDistanceToCorner(int row, int col);
    bool isInsufficientMaterial(const char board[8][8]);
}
