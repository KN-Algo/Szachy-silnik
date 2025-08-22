#include "move_generator.h"
#include <cctype>
#include <cmath>

std::vector<Move> MoveGenerator::generateAllMoves(const char board[8][8], char activeColor, 
                                                 const std::string& castling, const std::string& enPassant) {
    std::vector<Move> moves;
    
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            char piece = board[row][col];
            if (!piece) continue;
            
            bool whitePiece = std::isupper(piece);
            if ((activeColor == 'w' && !whitePiece) || (activeColor == 'b' && whitePiece))
                continue;
            
            switch (std::toupper(piece)) {
                case 'P':
                    generatePawnMoves(board, row, col, activeColor, enPassant, moves);
                    break;
                case 'N':
                    generateKnightMoves(board, row, col, activeColor, moves);
                    break;
                case 'B':
                    generateBishopMoves(board, row, col, activeColor, moves);
                    break;
                case 'R':
                    generateRookMoves(board, row, col, activeColor, moves);
                    break;
                case 'Q':
                    generateQueenMoves(board, row, col, activeColor, moves);
                    break;
                case 'K':
                    generateKingMoves(board, row, col, activeColor, castling, moves);
                    break;
            }
        }
    }
    
    return moves;
}

std::vector<Move> MoveGenerator::generateLegalMoves(const char board[8][8], char activeColor, 
                                                   const std::string& castling, const std::string& enPassant) {
    std::vector<Move> allMoves = generateAllMoves(board, activeColor, castling, enPassant);
    std::vector<Move> legalMoves;
    
    for (const Move& move : allMoves) {
        if (!wouldKingBeInCheck(board, move, activeColor)) {
            legalMoves.push_back(move);
        }
    }
    
    return legalMoves;
}

bool MoveGenerator::hasLegalMoves(const char board[8][8], char activeColor, 
                                 const std::string& castling, const std::string& enPassant) {
    std::vector<Move> allMoves = generateAllMoves(board, activeColor, castling, enPassant);
    
    for (const Move& move : allMoves) {
        if (!wouldKingBeInCheck(board, move, activeColor)) {
            return true;
        }
    }
    
    return false;
}

bool MoveGenerator::isInCheck(const char board[8][8], char activeColor) {
    char kingChar = (activeColor == 'w') ? 'K' : 'k';
    int kingRow = -1, kingCol = -1;
    
    for (int r = 0; r < 8; r++) {
        for (int c = 0; c < 8; c++) {
            if (board[r][c] == kingChar) {
                kingRow = r;
                kingCol = c;
                break;
            }
        }
        if (kingRow != -1) break;
    }
    
    if (kingRow == -1) return false;
    
    char enemyColor = (activeColor == 'w') ? 'b' : 'w';
    return isSquareAttacked(board, kingRow, kingCol, enemyColor);
}

bool MoveGenerator::wouldKingBeInCheck(const char board[8][8], const Move& move, char activeColor) {
    char tempBoard[8][8];
    for (int r = 0; r < 8; r++) {
        for (int c = 0; c < 8; c++) {
            tempBoard[r][c] = board[r][c];
        }
    }
    
    tempBoard[move.toRow][move.toCol] = tempBoard[move.fromRow][move.fromCol];
    tempBoard[move.fromRow][move.fromCol] = 0;
    
    return isInCheck(tempBoard, activeColor);
}

bool MoveGenerator::isSquareAttacked(const char board[8][8], int row, int col, char byColor) {
    int dir = (byColor == 'w') ? -1 : 1;
    char pawnChar = (byColor == 'w') ? 'P' : 'p';
    if (row + dir >= 0 && row + dir < 8) {
        if (col > 0 && board[row + dir][col - 1] == pawnChar) return true;
        if (col < 7 && board[row + dir][col + 1] == pawnChar) return true;
    }
    
    const int knightMoves[8][2] = {{-2,-1},{-2,1},{-1,-2},{-1,2},
                                   {1,-2},{1,2},{2,-1},{2,1}};
    char knightChar = (byColor == 'w') ? 'N' : 'n';
    for (auto& m : knightMoves) {
        int nr = row + m[0], nc = col + m[1];
        if (nr >= 0 && nr < 8 && nc >= 0 && nc < 8) {
            if (board[nr][nc] == knightChar) return true;
        }
    }
    
    char rookChar = (byColor == 'w') ? 'R' : 'r';
    char queenChar = (byColor == 'w') ? 'Q' : 'q';
    const int dirsRook[4][2] = {{1,0},{-1,0},{0,1},{0,-1}};
    for (auto& d : dirsRook) {
        int r = row + d[0], c = col + d[1];
        while (r >= 0 && r < 8 && c >= 0 && c < 8) {
            if (board[r][c] != 0) {
                if (board[r][c] == rookChar || board[r][c] == queenChar) return true;
                break;
            }
            r += d[0]; c += d[1];
        }
    }
    
    char bishopChar = (byColor == 'w') ? 'B' : 'b';
    const int dirsBishop[4][2] = {{1,1},{1,-1},{-1,1},{-1,-1}};
    for (auto& d : dirsBishop) {
        int r = row + d[0], c = col + d[1];
        while (r >= 0 && r < 8 && c >= 0 && c < 8) {
            if (board[r][c] != 0) {
                if (board[r][c] == bishopChar || board[r][c] == queenChar) return true;
                break;
            }
            r += d[0]; c += d[1];
        }
    }
    
    char kingChar = (byColor == 'w') ? 'K' : 'k';
    for (int dr = -1; dr <= 1; ++dr) {
        for (int dc = -1; dc <= 1; ++dc) {
            if (dr == 0 && dc == 0) continue;
            int nr = row + dr, nc = col + dc;
            if (nr >= 0 && nr < 8 && nc >= 0 && nc < 8) {
                if (board[nr][nc] == kingChar) return true;
            }
        }
    }
    
    return false;
}

bool MoveGenerator::isPathClear(const char board[8][8], int r1, int c1, int r2, int c2) {
    int dr = (r2 > r1) ? 1 : (r2 < r1) ? -1 : 0;
    int dc = (c2 > c1) ? 1 : (c2 < c1) ? -1 : 0;
    int r = r1 + dr, c = c1 + dc;
    while (r != r2 || c != c2) {
        if (board[r][c] != 0) return false;
        r += dr;
        c += dc;
    }
    return true;
}

void MoveGenerator::generatePawnMoves(const char board[8][8], int row, int col, char activeColor, 
                                     const std::string& enPassant, std::vector<Move>& moves) {
    char piece = board[row][col];
    bool whitePiece = std::isupper(piece);
    int dir = whitePiece ? -1 : 1;
    int startRow = whitePiece ? 6 : 1;
    
    if (row + dir >= 0 && row + dir < 8 && board[row + dir][col] == 0) {
        moves.push_back({row, col, row + dir, col, piece, 0});
        
        if (row == startRow && board[row + 2*dir][col] == 0) {
            moves.push_back({row, col, row + 2*dir, col, piece, 0});
        }
    }
    
    for (int dc = -1; dc <= 1; dc += 2) {
        if (col + dc >= 0 && col + dc < 8 && row + dir >= 0 && row + dir < 8) {
            char target = board[row + dir][col + dc];
            if (target && (std::isupper(target) != whitePiece)) {
                moves.push_back({row, col, row + dir, col + dc, piece, target});
            }
        }
    }
    
    if (enPassant != "-" && enPassant.length() >= 2) {
        int epCol = enPassant[0] - 'a';
        int epRow = '8' - enPassant[1];
        if (std::abs(col - epCol) == 1 && row + dir == epRow) {
            moves.push_back({row, col, epRow, epCol, piece, 0});
        }
    }
}

void MoveGenerator::generateKnightMoves(const char board[8][8], int row, int col, char activeColor, std::vector<Move>& moves) {
    char piece = board[row][col];
    bool whitePiece = std::isupper(piece);
    
    const int knightMoves[8][2] = {{-2,-1},{-2,1},{-1,-2},{-1,2},
                                   {1,-2},{1,2},{2,-1},{2,1}};
    
    for (auto& m : knightMoves) {
        int newRow = row + m[0];
        int newCol = col + m[1];
        
        if (newRow >= 0 && newRow < 8 && newCol >= 0 && newCol < 8) {
            char target = board[newRow][newCol];
            if (!target || (std::isupper(target) != whitePiece)) {
                moves.push_back({row, col, newRow, newCol, piece, target});
            }
        }
    }
}

void MoveGenerator::generateBishopMoves(const char board[8][8], int row, int col, char activeColor, std::vector<Move>& moves) {
    char piece = board[row][col];
    bool whitePiece = std::isupper(piece);
    
    const int directions[4][2] = {{1,1},{1,-1},{-1,1},{-1,-1}};
    
    for (auto& dir : directions) {
        for (int i = 1; i < 8; i++) {
            int newRow = row + dir[0] * i;
            int newCol = col + dir[1] * i;
            
            if (newRow < 0 || newRow >= 8 || newCol < 0 || newCol >= 8) break;
            
            char target = board[newRow][newCol];
            if (!target) {
                moves.push_back({row, col, newRow, newCol, piece, 0});
            } else {
                if (std::isupper(target) != whitePiece) {
                    moves.push_back({row, col, newRow, newCol, piece, target});
                }
                break;
            }
        }
    }
}

void MoveGenerator::generateRookMoves(const char board[8][8], int row, int col, char activeColor, std::vector<Move>& moves) {
    char piece = board[row][col];
    bool whitePiece = std::isupper(piece);
    
    const int directions[4][2] = {{1,0},{-1,0},{0,1},{0,-1}};
    
    for (auto& dir : directions) {
        for (int i = 1; i < 8; i++) {
            int newRow = row + dir[0] * i;
            int newCol = col + dir[1] * i;
            
            if (newRow < 0 || newRow >= 8 || newCol < 0 || newCol >= 8) break;
            
            char target = board[newRow][newCol];
            if (!target) {
                moves.push_back({row, col, newRow, newCol, piece, 0});
            } else {
                if (std::isupper(target) != whitePiece) {
                    moves.push_back({row, col, newRow, newCol, piece, target});
                }
                break;
            }
        }
    }
}

void MoveGenerator::generateQueenMoves(const char board[8][8], int row, int col, char activeColor, std::vector<Move>& moves) {
    generateBishopMoves(board, row, col, activeColor, moves);
    generateRookMoves(board, row, col, activeColor, moves);
}

void MoveGenerator::generateKingMoves(const char board[8][8], int row, int col, char activeColor, 
                                     const std::string& castling, std::vector<Move>& moves) {
    char piece = board[row][col];
    bool whitePiece = std::isupper(piece);
    
    for (int dr = -1; dr <= 1; dr++) {
        for (int dc = -1; dc <= 1; dc++) {
            if (dr == 0 && dc == 0) continue;
            
            int newRow = row + dr;
            int newCol = col + dc;
            
            if (newRow >= 0 && newRow < 8 && newCol >= 0 && newCol < 8) {
                char target = board[newRow][newCol];
                if (!target || (std::isupper(target) != whitePiece)) {
                    moves.push_back({row, col, newRow, newCol, piece, target});
                }
            }
        }
    }
    
    if (whitePiece && row == 7 && col == 4) {
        if (castling.find('K') != std::string::npos && 
            board[7][5] == 0 && board[7][6] == 0 && board[7][7] == 'R' &&
            !isSquareAttacked(board, 7, 4, 'b') && 
            !isSquareAttacked(board, 7, 5, 'b') && 
            !isSquareAttacked(board, 7, 6, 'b')) {
            moves.push_back({7, 4, 7, 6, 'K', 0});
        }
        if (castling.find('Q') != std::string::npos && 
            board[7][3] == 0 && board[7][2] == 0 && board[7][1] == 0 && board[7][0] == 'R' &&
            !isSquareAttacked(board, 7, 4, 'b') && 
            !isSquareAttacked(board, 7, 3, 'b') && 
            !isSquareAttacked(board, 7, 2, 'b')) {
            moves.push_back({7, 4, 7, 2, 'K', 0});
        }
    }
    
    if (!whitePiece && row == 0 && col == 4) {
        if (castling.find('k') != std::string::npos && 
            board[0][5] == 0 && board[0][6] == 0 && board[0][7] == 'r' &&
            !isSquareAttacked(board, 0, 4, 'w') && 
            !isSquareAttacked(board, 0, 5, 'w') && 
            !isSquareAttacked(board, 0, 6, 'w')) {
            moves.push_back({0, 4, 0, 6, 'k', 0});
        }
        if (castling.find('q') != std::string::npos && 
            board[0][3] == 0 && board[0][2] == 0 && board[0][1] == 0 && board[0][0] == 'r' &&
            !isSquareAttacked(board, 0, 4, 'w') && 
            !isSquareAttacked(board, 0, 3, 'w') && 
            !isSquareAttacked(board, 0, 2, 'w')) {
            moves.push_back({0, 4, 0, 2, 'k', 0});
        }
    }
}