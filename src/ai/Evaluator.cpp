#include "chess/ai/Evaluator.h"
#include <cctype>
#include <cmath>

namespace Evaluator {

int evaluatePosition(const char board[8][8], char activeColor) {
    int score = 0;
    
    // Ocena materialna
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            char piece = board[row][col];
            if (!piece) continue;
            
            bool isWhite = std::isupper(piece);
            int multiplier = isWhite ? 1 : -1;
            
            switch (std::toupper(piece)) {
                case 'P': score += PAWN_VALUE * multiplier; break;
                case 'N': score += KNIGHT_VALUE * multiplier; break;
                case 'B': score += BISHOP_VALUE * multiplier; break;
                case 'R': score += ROOK_VALUE * multiplier; break;
                case 'Q': score += QUEEN_VALUE * multiplier; break;
                case 'K': score += KING_VALUE * multiplier; break;
            }
        }
    }
    
    // Bonusy pozycyjne
    score += evaluatePawnStructure(board);
    score += evaluateCenterControl(board);
    score += evaluateKingSafety(board);
    
    // Zwróć ocenę z perspektywy strony do ruchu
    return (activeColor == 'w') ? score : -score;
}

int evaluatePawnStructure(const char board[8][8]) {
    int score = 0;
    
    // Bonus za podwójne piony
    for (int col = 0; col < 8; col++) {
        int whitePawns = 0, blackPawns = 0;
        for (int row = 0; row < 8; row++) {
            if (board[row][col] == 'P') whitePawns++;
            if (board[row][col] == 'p') blackPawns++;
        }
        
        if (whitePawns > 1) score -= PAWN_STRUCTURE_BONUS * (whitePawns - 1);
        if (blackPawns > 1) score += PAWN_STRUCTURE_BONUS * (blackPawns - 1);
    }
    
    return score;
}

int evaluateCenterControl(const char board[8][8]) {
    int score = 0;
    
    // Bonus za kontrolę centrum (pola e4, e5, d4, d5)
    const int centerSquares[4][2] = {{3,3}, {3,4}, {4,3}, {4,4}};
    
    for (auto& square : centerSquares) {
        int row = square[0], col = square[1];
        char piece = board[row][col];
        
        if (piece) {
            if (std::isupper(piece)) {
                score += CENTER_CONTROL_BONUS;
            } else {
                score -= CENTER_CONTROL_BONUS;
            }
        }
    }
    
    return score;
}

int evaluateKingSafety(const char board[8][8]) {
    int score = 0;
    
    // Znajdź pozycje królów
    int whiteKingRow = -1, whiteKingCol = -1;
    int blackKingRow = -1, blackKingCol = -1;
    
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            if (board[row][col] == 'K') {
                whiteKingRow = row; whiteKingCol = col;
            } else if (board[row][col] == 'k') {
                blackKingRow = row; blackKingCol = col;
            }
        }
    }
    
    // Bonus za bezpieczeństwo króla (im dalej od centrum, tym lepiej)
    if (whiteKingRow != -1) {
        int centerDistance = std::abs(whiteKingRow - 3.5) + std::abs(whiteKingCol - 3.5);
        score += KING_SAFETY_BONUS * (7 - centerDistance);
    }
    
    if (blackKingRow != -1) {
        int centerDistance = std::abs(blackKingRow - 3.5) + std::abs(blackKingCol - 3.5);
        score -= KING_SAFETY_BONUS * (7 - centerDistance);
    }
    
    return score;
}

} // namespace Evaluator
