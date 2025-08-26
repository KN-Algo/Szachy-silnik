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
    constexpr int CENTER_CONTROL_BONUS = 10;
    constexpr int PAWN_STRUCTURE_BONUS = 5;
    constexpr int KING_SAFETY_BONUS = 20;
    
    // Funkcje pomocnicze
    int evaluatePawnStructure(const char board[8][8]);
    int evaluateCenterControl(const char board[8][8]);
    int evaluateKingSafety(const char board[8][8]);
}
