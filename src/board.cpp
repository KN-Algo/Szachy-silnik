//
// Created by jakub on 28.07.2025.
//
#include <iostream>
#include <string>
#include <sstream>
#include <cctype>
#include <algorithm>
#include "board.h"
#include "move.h"

void Board::startBoard() {
    std::string fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
    int row = 0, col = 0;
    size_t i = 0;

    while (fen[i] != ' ') {
        char c = fen[i];
        if (c == '/') {
            row++;
            col = 0;
        } else if (isdigit(c)) {
            col += c - '0';
        } else {
            board[row][col++] = c;
        }
        i++;
    }

    activeColor = fen[++i];
    i += 2;

    size_t spacePos = fen.find(' ', i);
    castling = fen.substr(i, spacePos - i);
    i = spacePos + 1;

    spacePos = fen.find(' ', i);
    enPassant = fen.substr(i, spacePos - i);
    i = spacePos + 1;

    spacePos = fen.find(' ', i);
    halfmoveClock = std::stoi(fen.substr(i, spacePos - i));
    i = spacePos + 1;

    fullmoveNumber = std::stoi(fen.substr(i));
    
    positionHistory.clear();
    positionHistory[boardToString()] = 1;
}

void Board::printBoard() const {
    for (int row = 0; row < 8; ++row) {
        for (int col = 0; col < 8; ++col) {
            std::cout << (board[row][col] ? board[row][col] : '.') << ' ';
        }
        std::cout << '\n';
    }

    std::cout << "Active color: " << activeColor << '\n';
    std::cout << "Castling: " << castling << '\n';
    std::cout << "En passant: " << enPassant << '\n';
    std::cout << "Halfmove clock: " << halfmoveClock << '\n';
    std::cout << "Fullmove number: " << fullmoveNumber << '\n';
    
    GameState state = getGameState();
    if (state != GameState::PLAYING) {
        std::cout << "Game Status: " << getGameStateString() << '\n';
    }
}

static int fileToCol(char file) { return file - 'a'; }
static int rankToRow(char rank) { return '8' - rank; }

bool Board::isPathClear(int r1, int c1, int r2, int c2) const {
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

bool Board::isSquareAttacked(int row, int col, char byColor) const {
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

bool Board::isMoveValid(const Move& move) {
    int r1 = move.fromRow;
    int c1 = move.fromCol;
    int r2 = move.toRow;
    int c2 = move.toCol;

    char piece = board[r1][c1];
    if (!piece) return false;

    bool whitePiece = std::isupper(piece);
    if ((activeColor == 'w' && !whitePiece) || (activeColor == 'b' && whitePiece))
        return false;

    if (board[r2][c2] && (std::isupper(board[r2][c2]) == whitePiece)) return false;

    int dr = r2 - r1;
    int dc = c2 - c1;
    int adr = std::abs(dr), adc = std::abs(dc);

    switch (std::toupper(piece)) {
        case 'P': {
            int dir = whitePiece ? -1 : 1;
            if (dc == 0) {
                if (dr == dir && board[r2][c2] == 0) {}
                else if ((whitePiece && r1 == 6 || !whitePiece && r1 == 1)
                         && dr == 2*dir && board[r1+dir][c1] == 0 && board[r2][c2] == 0) {}
                else return false;
            } else if (adr == 1 && dr == dir && board[r2][c2] != 0) {}
            else return false;
            break;
        }
        case 'N':
            if (!((adr == 2 && adc == 1) || (adr == 1 && adc == 2))) return false;
            break;
        case 'B':
            if (adr != adc) return false;
            if (!isPathClear(r1, c1, r2, c2)) return false;
            break;
        case 'R':
            if (!(adr == 0 || adc == 0)) return false;
            if (!isPathClear(r1, c1, r2, c2)) return false;
            break;
        case 'Q':
            if (!(adr == adc || adr == 0 || adc == 0)) return false;
            if (!isPathClear(r1, c1, r2, c2)) return false;
            break;
        case 'K':
            if (adr > 1 || adc > 1) return false;
            break;
        default:
            return false;
    }

    char captured = board[r2][c2];
    board[r2][c2] = piece;
    board[r1][c1] = 0;

    char kingChar = whitePiece ? 'K' : 'k';
    int kr=0,kc=0;
    for (int r=0;r<8;r++) for (int c=0;c<8;c++) if (board[r][c]==kingChar) { kr=r; kc=c; }

    bool inCheck = isSquareAttacked(kr, kc, whitePiece ? 'b' : 'w');

    board[r1][c1] = piece;
    board[r2][c2] = captured;

    return !inCheck;
}

void Board::makeMove(const Move& move) {
    if (!isMoveValid(move)) {
        std::cout << "Illegal move!\n";
        return;
    }
    
    char piece = board[move.fromRow][move.fromCol];
    char captured = board[move.toRow][move.toCol];
    
    if (std::toupper(piece) == 'P' || captured != 0) {
        halfmoveClock = 0;
    } else {
        halfmoveClock++;
    }
    
    board[move.toRow][move.toCol] = piece;
    board[move.fromRow][move.fromCol] = 0;
    
    if (activeColor == 'b') {
        fullmoveNumber++;
    }
    
    activeColor = (activeColor == 'w') ? 'b' : 'w';
    
    std::string position = boardToString();
    positionHistory[position]++;
}

std::vector<Move> Board::getLegalMoves() const {
    std::vector<Move> legalMoves;
    
    for (int fromRow = 0; fromRow < 8; fromRow++) {
        for (int fromCol = 0; fromCol < 8; fromCol++) {
            char piece = board[fromRow][fromCol];
            if (!piece) continue;
            
            bool whitePiece = std::isupper(piece);
            if ((activeColor == 'w' && !whitePiece) || (activeColor == 'b' && whitePiece))
                continue;
            
            for (int toRow = 0; toRow < 8; toRow++) {
                for (int toCol = 0; toCol < 8; toCol++) {
                    Move move = {fromRow, fromCol, toRow, toCol, piece, board[toRow][toCol]};
                    
                    Board* nonConstThis = const_cast<Board*>(this);
                    if (nonConstThis->isMoveValid(move)) {
                        legalMoves.push_back(move);
                    }
                }
            }
        }
    }
    
    return legalMoves;
}

bool Board::hasLegalMoves() const {
    for (int fromRow = 0; fromRow < 8; fromRow++) {
        for (int fromCol = 0; fromCol < 8; fromCol++) {
            char piece = board[fromRow][fromCol];
            if (!piece) continue;
            
            bool whitePiece = std::isupper(piece);
            if ((activeColor == 'w' && !whitePiece) || (activeColor == 'b' && whitePiece))
                continue;
            
            for (int toRow = 0; toRow < 8; toRow++) {
                for (int toCol = 0; toCol < 8; toCol++) {
                    Move move = {fromRow, fromCol, toRow, toCol, piece, board[toRow][toCol]};
                    
                    Board* nonConstThis = const_cast<Board*>(this);
                    if (nonConstThis->isMoveValid(move)) {
                        return true;
                    }
                }
            }
        }
    }
    return false;
}

bool Board::isInCheck() const {
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
    return isSquareAttacked(kingRow, kingCol, enemyColor);
}

GameState Board::getGameState() const {
    if (halfmoveClock >= 100) {
        return GameState::DRAW_50_MOVES;
    }
    
    std::string currentPosition = boardToString();
    auto it = positionHistory.find(currentPosition);
    if (it != positionHistory.end() && it->second >= 3) {
        return GameState::DRAW_REPETITION;
    }
    
    if (hasInsufficientMaterial()) {
        return GameState::DRAW_INSUFFICIENT_MATERIAL;
    }
    
    if (!hasLegalMoves()) {
        if (isInCheck()) {
            return GameState::CHECKMATE;
        } else {
            return GameState::STALEMATE;
        }
    }
    
    return GameState::PLAYING;
}

std::string Board::getGameStateString() const {
    switch (getGameState()) {
        case GameState::PLAYING:
            return "Game in progress";
        case GameState::CHECKMATE:
            return (activeColor == 'w') ? "Czarny wygrywa przez mata!" : "Biały wygrywa przez mata!";
        case GameState::STALEMATE:
            return "Remis przez pata!";
        case GameState::DRAW_50_MOVES:
            return "Remis przez zasadę 50 ruchów!";
        case GameState::DRAW_REPETITION:
            return "Remis przez trzykrotne powtórzenie pozycji!";
        case GameState::DRAW_INSUFFICIENT_MATERIAL:
            return "Remis przez niewystarczający materiał!";
        default:
            return "Unknown state";
    }
}

std::string Board::boardToString() const {
    std::string result;
    for (int r = 0; r < 8; r++) {
        for (int c = 0; c < 8; c++) {
            result += (board[r][c] ? board[r][c] : '.');
        }
    }
    result += activeColor;
    result += castling;
    result += enPassant;
    return result;
}

bool Board::hasInsufficientMaterial() const {
    // Count pieces
    int wKing = 0, bKing = 0;
    int wQueen = 0, bQueen = 0;
    int wRook = 0, bRook = 0;
    int wBishop = 0, bBishop = 0;
    int wKnight = 0, bKnight = 0;
    int wPawn = 0, bPawn = 0;
    
    for (int r = 0; r < 8; r++) {
        for (int c = 0; c < 8; c++) {
            switch (board[r][c]) {
                case 'K': wKing++; break;
                case 'k': bKing++; break;
                case 'Q': wQueen++; break;
                case 'q': bQueen++; break;
                case 'R': wRook++; break;
                case 'r': bRook++; break;
                case 'B': wBishop++; break;
                case 'b': bBishop++; break;
                case 'N': wKnight++; break;
                case 'n': bKnight++; break;
                case 'P': wPawn++; break;
                case 'p': bPawn++; break;
            }
        }
    }
    
    if (wPawn || bPawn || wQueen || bQueen || wRook || bRook) {
        return false;
    }
    
    if (wBishop == 0 && bBishop == 0 && wKnight == 0 && bKnight == 0) {
        return true;
    }
    
    if ((wBishop == 1 && bBishop == 0 && wKnight == 0 && bKnight == 0) ||
        (wBishop == 0 && bBishop == 1 && wKnight == 0 && bKnight == 0)) {
        return true;
    }
    
    if ((wKnight == 1 && bKnight == 0 && wBishop == 0 && bBishop == 0) ||
        (wKnight == 0 && bKnight == 1 && wBishop == 0 && bBishop == 0)) {
        return true;
    }
    
    return false;
}

int Board::countPieces(char piece) const {
    int count = 0;
    for (int r = 0; r < 8; r++) {
        for (int c = 0; c < 8; c++) {
            if (board[r][c] == piece) count++;
        }
    }
    return count;
}
