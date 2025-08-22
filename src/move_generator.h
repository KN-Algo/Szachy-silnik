#pragma once
#include <vector>
#include "move.h"

class MoveGenerator {
private:
    static bool isSquareAttacked(const char board[8][8], int row, int col, char byColor);
    static bool isPathClear(const char board[8][8], int r1, int c1, int r2, int c2);
    static bool wouldKingBeInCheck(const char board[8][8], const Move& move, char activeColor);
    
    static void generatePawnMoves(const char board[8][8], int row, int col, char activeColor, 
                                 const std::string& enPassant, std::vector<Move>& moves);
    static void generateKnightMoves(const char board[8][8], int row, int col, char activeColor, std::vector<Move>& moves);
    static void generateBishopMoves(const char board[8][8], int row, int col, char activeColor, std::vector<Move>& moves);
    static void generateRookMoves(const char board[8][8], int row, int col, char activeColor, std::vector<Move>& moves);
    static void generateQueenMoves(const char board[8][8], int row, int col, char activeColor, std::vector<Move>& moves);
    static void generateKingMoves(const char board[8][8], int row, int col, char activeColor, 
                                 const std::string& castling, std::vector<Move>& moves);

public:
    static std::vector<Move> generateAllMoves(const char board[8][8], char activeColor, 
                                            const std::string& castling, const std::string& enPassant);
    static std::vector<Move> generateLegalMoves(const char board[8][8], char activeColor, 
                                              const std::string& castling, const std::string& enPassant);
    static bool hasLegalMoves(const char board[8][8], char activeColor, 
                             const std::string& castling, const std::string& enPassant);
    static bool isInCheck(const char board[8][8], char activeColor);
};