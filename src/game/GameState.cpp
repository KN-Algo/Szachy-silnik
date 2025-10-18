
#include "chess/game/GameState.h"
#include <cctype>

void GameStateManager::addPosition(const char board[8][8], char activeColor, 
                                  const std::string& castling, const std::string& enPassant) {
    std::string position = boardToString(board, activeColor, castling, enPassant);
    positionHistory[position]++;
}

void GameStateManager::clearHistory() {
    positionHistory.clear();
}

GameState GameStateManager::checkGameState(const char board[8][8], char activeColor, 
                                         const std::string& castling, const std::string& enPassant,
                                         int halfmoveClock, bool hasLegalMoves, bool isInCheck) const {
    if (halfmoveClock >= 100) {
        return GameState::DRAW_50_MOVES;
    }
    
    std::string currentPosition = boardToString(board, activeColor, castling, enPassant);
    auto it = positionHistory.find(currentPosition);
    if (it != positionHistory.end() && it->second >= 3) {
        return GameState::DRAW_REPETITION;
    }
    
    if (hasInsufficientMaterial(board)) {
        return GameState::DRAW_INSUFFICIENT_MATERIAL;
    }
    
    if (!hasLegalMoves) {
        if (isInCheck) {
            return GameState::CHECKMATE;
        } else {
            return GameState::STALEMATE;
        }
    }
    
    return GameState::PLAYING;
}

std::string GameStateManager::getGameStateString(GameState state, char activeColor) const {
    switch (state) {
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

std::string GameStateManager::boardToString(const char board[8][8], char activeColor, 
                                           const std::string& castling, const std::string& enPassant) const {
    std::string result;
    for (int r = 0; r < 8; r++)
        for (int c = 0; c < 8; c++)
            result += (board[r][c] ? board[r][c] : '.');

    // normalizacja roszad
    std::string cst = castling.empty() ? "-" : castling;
    if (cst != "-") {
        std::string order = "KQkq";
        std::string norm;
        for (char ch : order) if (cst.find(ch) != std::string::npos) norm.push_back(ch);
        cst = norm.empty() ? "-" : norm;
    }

    // normalizacja en passant
    std::string ep = enPassant.empty() ? "-" : enPassant;

    result += activeColor;
    result += cst;
    result += ep;
    return result;
}

bool GameStateManager::hasInsufficientMaterial(const char board[8][8]) const {
    int wQueen=0,bQueen=0,wRook=0,bRook=0,wBishop=0,bBishop=0,wKnight=0,bKnight=0,wPawn=0,bPawn=0;
    int wB_r=-1,wB_c=-1, bB_r=-1,bB_c=-1; // do koloru pól gońców

    for (int r=0;r<8;r++) for (int c=0;c<8;c++) {
        switch (board[r][c]) {
            case 'Q': wQueen++; break; case 'q': bQueen++; break;
            case 'R': wRook++;  break; case 'r': bRook++;  break;
            case 'B': wBishop++; wB_r=r; wB_c=c; break;
            case 'b': bBishop++; bB_r=r; bB_c=c; break;
            case 'N': wKnight++; break; case 'n': bKnight++; break;
            case 'P': wPawn++;   break; case 'p': bPawn++;   break;
            default: break;
        }
    }

    // jakikolwiek pion/hetman/wieża => materiał wystarczający
    if (wPawn || bPawn || wQueen || bQueen || wRook || bRook) return false;

    // TYLKO królowie
    if (wBishop==0 && bBishop==0 && wKnight==0 && bKnight==0) return true;

    // K + (B lub N) vs K
    if ((wBishop + wKnight) == 1 && (bBishop + bKnight) == 0) return true;
    if ((bBishop + bKnight) == 1 && (wBishop + wKnight) == 0) return true;

    // K + NN vs K (dwa skoczki nie wymuszają mata)
    if (wKnight==2 && wBishop==0 && bKnight==0 && bBishop==0) return true;
    if (bKnight==2 && bBishop==0 && wKnight==0 && wBishop==0) return true;

    // K+B vs K+B — tylko gdy OBIE strony mają gońca na TYM SAMYM kolorze pól
    if (wBishop==1 && bBishop==1 && wKnight==0 && bKnight==0) {
        if (wB_r!=-1 && bB_r!=-1) {
            int wColor = (wB_r + wB_c) & 1; // 0 = jasne, 1 = ciemne
            int bColor = (bB_r + bB_c) & 1;
            if (wColor == bColor) return true;
        }
    }

    return false;
}
