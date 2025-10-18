#include "chess/ai/Evaluator.h"
#include "chess/rules/MoveGenerator.h"
#include <cctype>
#include <cmath>

namespace Evaluator {

// Piece-Square Tables - nagradzają figury za dobre pozycje
// Tablica dla białych (row 0 = 8th rank, row 7 = 1st rank)
// Dla czarnych będziemy odwracać tablicę (row 7-row)

const int PAWN_PST[8][8] = {
    {  0,  0,  0,  0,  0,  0,  0,  0 },  // 8th rank
    { 50, 50, 50, 50, 50, 50, 50, 50 },  // 7th rank
    { 10, 10, 20, 30, 30, 20, 10, 10 },
    {  5,  5, 10, 25, 25, 10,  5,  5 },
    {  0,  0,  0, 20, 20,  0,  0,  0 },
    {  5, -5,-10,  0,  0,-10, -5,  5 },
    {  5, 10, 10,-20,-20, 10, 10,  5 },
    {  0,  0,  0,  0,  0,  0,  0,  0 }   // 1st rank
};

const int KNIGHT_PST[8][8] = {
    { -50,-40,-30,-30,-30,-30,-40,-50 },
    { -40,-20,  0,  0,  0,  0,-20,-40 },
    { -30,  0, 10, 15, 15, 10,  0,-30 },
    { -30,  5, 15, 20, 20, 15,  5,-30 },
    { -30,  0, 15, 20, 20, 15,  0,-30 },
    { -30,  5, 10, 15, 15, 10,  5,-30 },
    { -40,-20,  0,  5,  5,  0,-20,-40 },
    { -50,-40,-30,-30,-30,-30,-40,-50 }
};

const int BISHOP_PST[8][8] = {
    { -20,-10,-10,-10,-10,-10,-10,-20 },
    { -10,  0,  0,  0,  0,  0,  0,-10 },
    { -10,  0,  5, 10, 10,  5,  0,-10 },
    { -10,  5,  5, 10, 10,  5,  5,-10 },
    { -10,  0, 10, 10, 10, 10,  0,-10 },
    { -10, 10, 10, 10, 10, 10, 10,-10 },
    { -10,  5,  0,  0,  0,  0,  5,-10 },
    { -20,-10,-10,-10,-10,-10,-10,-20 }
};

const int ROOK_PST[8][8] = {
    {  0,  0,  0,  0,  0,  0,  0,  0 },
    {  5, 10, 10, 10, 10, 10, 10,  5 },
    { -5,  0,  0,  0,  0,  0,  0, -5 },
    { -5,  0,  0,  0,  0,  0,  0, -5 },
    { -5,  0,  0,  0,  0,  0,  0, -5 },
    { -5,  0,  0,  0,  0,  0,  0, -5 },
    { -5,  0,  0,  0,  0,  0,  0, -5 },
    {  0,  0,  0,  5,  5,  0,  0,  0 }
};

const int QUEEN_PST[8][8] = {
    { -20,-10,-10, -5, -5,-10,-10,-20 },
    { -10,  0,  0,  0,  0,  0,  0,-10 },
    { -10,  0,  5,  5,  5,  5,  0,-10 },
    {  -5,  0,  5,  5,  5,  5,  0, -5 },
    {   0,  0,  5,  5,  5,  5,  0, -5 },
    { -10,  5,  5,  5,  5,  5,  0,-10 },
    { -10,  0,  5,  0,  0,  0,  0,-10 },
    { -20,-10,-10, -5, -5,-10,-10,-20 }
};

const int KING_MIDDLEGAME_PST[8][8] = {
    { -30,-40,-40,-50,-50,-40,-40,-30 },
    { -30,-40,-40,-50,-50,-40,-40,-30 },
    { -30,-40,-40,-50,-50,-40,-40,-30 },
    { -30,-40,-40,-50,-50,-40,-40,-30 },
    { -20,-30,-30,-40,-40,-30,-30,-20 },
    { -10,-20,-20,-20,-20,-20,-20,-10 },
    {  20, 20,  0,  0,  0,  0, 20, 20 },
    {  20, 30, 10,  0,  0, 10, 30, 20 }
};

const int KING_ENDGAME_PST[8][8] = {
    { -50,-40,-30,-20,-20,-30,-40,-50 },
    { -30,-20,-10,  0,  0,-10,-20,-30 },
    { -30,-10, 20, 30, 30, 20,-10,-30 },
    { -30,-10, 30, 40, 40, 30,-10,-30 },
    { -30,-10, 30, 40, 40, 30,-10,-30 },
    { -30,-10, 20, 30, 30, 20,-10,-30 },
    { -30,-30,  0,  0,  0,  0,-30,-30 },
    { -50,-30,-30,-30,-30,-30,-30,-50 }
};

bool isEndgame(const char board[8][8]) {
    int queens = 0;
    int minorPieces = 0; // Skoczki i gońce
    
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            char piece = std::toupper(board[row][col]);
            if (piece == 'Q') queens++;
            if (piece == 'N' || piece == 'B') minorPieces++;
        }
    }
    
    // Endgame jeśli: brak hetmanów lub każda strona ma hetmana + max 1 lekką figurę
    return queens == 0 || (queens <= 2 && minorPieces <= 2);
}

int manhattanDistance(int r1, int c1, int r2, int c2) {
    return std::abs(r1 - r2) + std::abs(c1 - c2);
}

int getDistanceToCorner(int row, int col) {
    // Odległość do najbliższego rogu
    int dist1 = manhattanDistance(row, col, 0, 0);
    int dist2 = manhattanDistance(row, col, 0, 7);
    int dist3 = manhattanDistance(row, col, 7, 0);
    int dist4 = manhattanDistance(row, col, 7, 7);
    return std::min({dist1, dist2, dist3, dist4});
}

int getMaterialBalance(const char board[8][8]) {
    int balance = 0;
    
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            char piece = board[row][col];
            if (!piece || std::toupper(piece) == 'K') continue;
            
            bool isWhite = std::isupper(piece);
            int multiplier = isWhite ? 1 : -1;
            
            switch (std::toupper(piece)) {
                case 'P': balance += PAWN_VALUE * multiplier; break;
                case 'N': balance += KNIGHT_VALUE * multiplier; break;
                case 'B': balance += BISHOP_VALUE * multiplier; break;
                case 'R': balance += ROOK_VALUE * multiplier; break;
                case 'Q': balance += QUEEN_VALUE * multiplier; break;
            }
        }
    }
    
    return balance;
}

bool isInsufficientMaterial(const char board[8][8]) {
    // Sprawdź czy jest za mało materiału do mata
    int whitePieces = 0, blackPieces = 0;
    int whiteKnights = 0, blackKnights = 0;
    int whiteBishops = 0, blackBishops = 0;
    bool hasWhitePawn = false, hasBlackPawn = false;
    bool hasWhiteRook = false, hasBlackRook = false;
    bool hasWhiteQueen = false, hasBlackQueen = false;
    
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            char piece = board[row][col];
            if (!piece) continue;
            
            bool isWhite = std::isupper(piece);
            char upperPiece = std::toupper(piece);
            
            if (upperPiece == 'K') continue;
            
            if (isWhite) {
                whitePieces++;
                if (upperPiece == 'N') whiteKnights++;
                if (upperPiece == 'B') whiteBishops++;
                if (upperPiece == 'P') hasWhitePawn = true;
                if (upperPiece == 'R') hasWhiteRook = true;
                if (upperPiece == 'Q') hasWhiteQueen = true;
            } else {
                blackPieces++;
                if (upperPiece == 'N') blackKnights++;
                if (upperPiece == 'B') blackBishops++;
                if (upperPiece == 'P') hasBlackPawn = true;
                if (upperPiece == 'R') hasBlackRook = true;
                if (upperPiece == 'Q') hasBlackQueen = true;
            }
        }
    }
    
    // K vs K
    if (whitePieces == 0 && blackPieces == 0) return true;
    
    // K+N vs K lub K+B vs K
    if ((whitePieces == 1 && blackPieces == 0 && (whiteKnights == 1 || whiteBishops == 1)) ||
        (blackPieces == 1 && whitePieces == 0 && (blackKnights == 1 || blackBishops == 1))) {
        return true;
    }
    
    // K+B vs K+B (same color bishops)
    if (whitePieces == 1 && blackPieces == 1 && whiteBishops == 1 && blackBishops == 1) {
        // Sprawdź czy gońce są na tym samym kolorze pól
        int whiteBishopColor = -1, blackBishopColor = -1;
        for (int row = 0; row < 8; row++) {
            for (int col = 0; col < 8; col++) {
                if (board[row][col] == 'B') whiteBishopColor = (row + col) % 2;
                if (board[row][col] == 'b') blackBishopColor = (row + col) % 2;
            }
        }
        if (whiteBishopColor == blackBishopColor) return true;
    }
    
    return false;
}

int evaluateEndgame(const char board[8][8]) {
    int score = 0;
    
    // Znajdź królów i policz figury
    int whiteKingRow = -1, whiteKingCol = -1;
    int blackKingRow = -1, blackKingCol = -1;
    bool whiteHasQueen = false, blackHasQueen = false;
    bool whiteHasRook = false, blackHasRook = false;
    int whitePieces = 0, blackPieces = 0;
    
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            char piece = board[row][col];
            if (piece == 'K') {
                whiteKingRow = row; whiteKingCol = col;
            } else if (piece == 'k') {
                blackKingRow = row; blackKingCol = col;
            } else if (piece == 'Q') {
                whiteHasQueen = true;
                whitePieces++;
            } else if (piece == 'q') {
                blackHasQueen = true;
                blackPieces++;
            } else if (piece == 'R') {
                whiteHasRook = true;
                whitePieces++;
            } else if (piece == 'r') {
                blackHasRook = true;
                blackPieces++;
            } else if (piece && piece != ' ') {
                if (std::isupper(piece)) whitePieces++;
                else blackPieces++;
            }
        }
    }
    
    if (whiteKingRow == -1 || blackKingRow == -1) return 0;
    
    // Oblicz przewagę materialną
    int materialBalance = getMaterialBalance(board);
    
    // SPECJALNE ROZPOZNAWANIE PODSTAWOWYCH MATÓW
    // K+Q vs K lub K+R vs K - to są najłatwiejsze maty
    if ((whiteHasQueen || whiteHasRook) && blackPieces == 0) {
        // Białe mają hetmana/wieżę vs samego króla - SUPER WAŻNE!
        // Zwiększ drastycznie bonusy
        int distanceToEdge = std::min({blackKingRow, 7 - blackKingRow, blackKingCol, 7 - blackKingCol});
        score += (3 - distanceToEdge) * 100; // x2.5 większy bonus!
        
        if (distanceToEdge == 0) {
            // Król przy brzegu - OGROMNY bonus, blisko mata!
            score += 300;
            // Dodatkowy bonus jeśli blisko rogu
            int distanceToCorner = getDistanceToCorner(blackKingRow, blackKingCol);
            score += (7 - distanceToCorner) * 50;
        }
        
        // Zbliżanie własnego króla jest KRYTYCZNE
        int kingDistance = manhattanDistance(whiteKingRow, whiteKingCol, blackKingRow, blackKingCol);
        score += (14 - kingDistance) * 25;
        
        return score; // Wróć wcześniej - to jest najważniejsze
    }
    
    if ((blackHasQueen || blackHasRook) && whitePieces == 0) {
        int distanceToEdge = std::min({whiteKingRow, 7 - whiteKingRow, whiteKingCol, 7 - whiteKingCol});
        score -= (3 - distanceToEdge) * 100;
        
        if (distanceToEdge == 0) {
            score -= 300;
            int distanceToCorner = getDistanceToCorner(whiteKingRow, whiteKingCol);
            score -= (7 - distanceToCorner) * 50;
        }
        
        int kingDistance = manhattanDistance(whiteKingRow, whiteKingCol, blackKingRow, blackKingCol);
        score -= (14 - kingDistance) * 25;
        
        return score;
    }
    
    // Jeśli ktoś ma przewagę materialną, pomóż mu dać mata
    if (std::abs(materialBalance) > 200) { // Przewaga większa niż 2 piony
        // Zwiększone bonusy dla końcówek - AI musi być bardziej agresywne
        int aggressionMultiplier = (std::abs(materialBalance) > 500) ? 2 : 1; // 2x dla dużej przewagi
        
        if (materialBalance > 0) {
            // Białe mają przewagę - powinny:
            // 1. Przybliżyć swego króla do króla przeciwnika
            int kingDistance = manhattanDistance(whiteKingRow, whiteKingCol, blackKingRow, blackKingCol);
            score += (14 - kingDistance) * 15 * aggressionMultiplier; // Zwiększone z 10 do 15
            
            // 2. Przycisnąć króla przeciwnika do brzegu/rogu
            int distanceToEdge = std::min({blackKingRow, 7 - blackKingRow, blackKingCol, 7 - blackKingCol});
            score += (3 - distanceToEdge) * 40 * aggressionMultiplier; // Zwiększone z 20 do 40
            
            // 3. Bonus za przyparcie do rogu (gdy już jest przy brzegu)
            if (distanceToEdge <= 1) {
                int distanceToCorner = getDistanceToCorner(blackKingRow, blackKingCol);
                score += (7 - distanceToCorner) * 30 * aggressionMultiplier; // Zwiększone z 15 do 30
            }
            
            // 4. Ogranicz ruchy króla przeciwnika (opposition)
            int filesApart = std::abs(whiteKingCol - blackKingCol);
            int ranksApart = std::abs(whiteKingRow - blackKingRow);
            if ((filesApart == 2 && ranksApart == 0) || (filesApart == 0 && ranksApart == 2)) {
                score += 50; // Zwiększone z 30 do 50
            }
            
            // 5. Dodatkowy bonus jeśli król jest już przy brzegu - szybciej kończyć
            if (distanceToEdge == 0) {
                score += 100 * aggressionMultiplier; // Duży bonus za przypięcie do brzegu
            }
        } else {
            // Czarne mają przewagę
            int kingDistance = manhattanDistance(whiteKingRow, whiteKingCol, blackKingRow, blackKingCol);
            score -= (14 - kingDistance) * 15 * aggressionMultiplier;
            
            int distanceToEdge = std::min({whiteKingRow, 7 - whiteKingRow, whiteKingCol, 7 - whiteKingCol});
            score -= (3 - distanceToEdge) * 40 * aggressionMultiplier;
            
            if (distanceToEdge <= 1) {
                int distanceToCorner = getDistanceToCorner(whiteKingRow, whiteKingCol);
                score -= (7 - distanceToCorner) * 30 * aggressionMultiplier;
            }
            
            int filesApart = std::abs(whiteKingCol - blackKingCol);
            int ranksApart = std::abs(whiteKingRow - blackKingRow);
            if ((filesApart == 2 && ranksApart == 0) || (filesApart == 0 && ranksApart == 2)) {
                score -= 50;
            }
            
            if (distanceToEdge == 0) {
                score -= 100 * aggressionMultiplier;
            }
        }
    }
    
    return score;
}

int evaluatePieceSquareTables(const char board[8][8]) {
    int score = 0;
    bool endgame = isEndgame(board);
    
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            char piece = board[row][col];
            if (!piece) continue;
            
            bool isWhite = std::isupper(piece);
            int multiplier = isWhite ? 1 : -1;
            int tableRow = isWhite ? row : (7 - row);
            
            int pstValue = 0;
            switch (std::toupper(piece)) {
                case 'P': pstValue = PAWN_PST[tableRow][col]; break;
                case 'N': pstValue = KNIGHT_PST[tableRow][col]; break;
                case 'B': pstValue = BISHOP_PST[tableRow][col]; break;
                case 'R': pstValue = ROOK_PST[tableRow][col]; break;
                case 'Q': pstValue = QUEEN_PST[tableRow][col]; break;
                case 'K': 
                    pstValue = endgame ? KING_ENDGAME_PST[tableRow][col] 
                                       : KING_MIDDLEGAME_PST[tableRow][col];
                    break;
            }
            
            score += pstValue * multiplier;
        }
    }
    
    return score;
}

int evaluatePosition(const char board[8][8], char activeColor) {
    int score = 0;
    
    // Sprawdź remis przez brak materiału
    if (isInsufficientMaterial(board)) {
        return 0;
    }
    
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
    score += evaluatePieceSquareTables(board);
    score += evaluatePawnStructure(board);
    score += evaluateCenterControl(board);
    score += evaluateKingSafety(board);
    score += evaluateDevelopment(board);
    
    // Specjalna ocena dla końcówek z przewagą materialną
    if (isEndgame(board)) {
        int endgameScore = evaluateEndgame(board);
        // W końcówce ocena postępu jest BARDZO ważna - zwiększ wagę 2x
        score += endgameScore * 2;
    }
    
    // Zwróć ocenę z perspektywy strony do ruchu
    return (activeColor == 'w') ? score : -score;
}

int evaluatePawnStructure(const char board[8][8]) {
    int score = 0;
    
    // Kara za podwójne piony (doubled pawns)
    for (int col = 0; col < 8; col++) {
        int whitePawns = 0, blackPawns = 0;
        for (int row = 0; row < 8; row++) {
            if (board[row][col] == 'P') whitePawns++;
            if (board[row][col] == 'p') blackPawns++;
        }
        
        if (whitePawns > 1) score -= PAWN_STRUCTURE_BONUS * (whitePawns - 1);
        if (blackPawns > 1) score += PAWN_STRUCTURE_BONUS * (blackPawns - 1);
    }
    
    // Bonus za passed pawns (piony przeszłe)
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            char piece = board[row][col];
            if (std::toupper(piece) != 'P') continue;
            
            bool isWhite = std::isupper(piece);
            bool isPassed = true;
            
            // Sprawdź czy pion jest przeszły (passed)
            if (isWhite) {
                // Sprawdź czy są czarne piony przed nim
                for (int r = row - 1; r >= 0; r--) {
                    int leftCol = (col > 0) ? col - 1 : col;
                    int rightCol = (col < 7) ? col + 1 : col;
                    for (int c = leftCol; c <= rightCol; c++) {
                        if (board[r][c] == 'p') {
                            isPassed = false;
                            break;
                        }
                    }
                    if (!isPassed) break;
                }
                if (isPassed) {
                    score += (7 - row) * 10; // Im bliżej promocji, tym więcej punktów
                }
            } else {
                // Sprawdź czy są białe piony przed czarnym pionem
                for (int r = row + 1; r < 8; r++) {
                    int leftCol = (col > 0) ? col - 1 : col;
                    int rightCol = (col < 7) ? col + 1 : col;
                    for (int c = leftCol; c <= rightCol; c++) {
                        if (board[r][c] == 'P') {
                            isPassed = false;
                            break;
                        }
                    }
                    if (!isPassed) break;
                }
                if (isPassed) {
                    score -= row * 10; // Im bliżej promocji, tym więcej punktów
                }
            }
        }
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
    
    // Bonus za extended centrum (c3-c6, d3-d6, e3-e6, f3-f6)
    for (int row = 2; row <= 5; row++) {
        for (int col = 2; col <= 5; col++) {
            char piece = board[row][col];
            if (piece && std::toupper(piece) == 'P') {
                if (std::isupper(piece)) {
                    score += CENTER_CONTROL_BONUS / 3;
                } else {
                    score -= CENTER_CONTROL_BONUS / 3;
                }
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
    
    bool endgame = isEndgame(board);
    
    if (!endgame) {
        // W middlegame: bonus za osłonę pionów przed królem
        if (whiteKingRow != -1) {
            // Sprawdź piony przed białym królem
            int pawnShield = 0;
            if (whiteKingRow < 7) {
                for (int dc = -1; dc <= 1; dc++) {
                    int c = whiteKingCol + dc;
                    if (c >= 0 && c < 8) {
                        if (board[whiteKingRow + 1][c] == 'P') pawnShield++;
                        if (whiteKingRow < 6 && board[whiteKingRow + 2][c] == 'P') pawnShield++;
                    }
                }
            }
            score += pawnShield * KING_SAFETY_BONUS;
        }
        
        if (blackKingRow != -1) {
            // Sprawdź piony przed czarnym królem
            int pawnShield = 0;
            if (blackKingRow > 0) {
                for (int dc = -1; dc <= 1; dc++) {
                    int c = blackKingCol + dc;
                    if (c >= 0 && c < 8) {
                        if (board[blackKingRow - 1][c] == 'p') pawnShield++;
                        if (blackKingRow > 1 && board[blackKingRow - 2][c] == 'p') pawnShield++;
                    }
                }
            }
            score -= pawnShield * KING_SAFETY_BONUS;
        }
    }
    
    return score;
}

int evaluateDevelopment(const char board[8][8]) {
    int score = 0;
    
    // Kara za nierozwinięte figury w początkowej fazie gry
    // Białe: jeśli figury nadal na początkowych polach
    if (board[7][1] == 'N') score -= DEVELOPMENT_BONUS; // b1 knight
    if (board[7][6] == 'N') score -= DEVELOPMENT_BONUS; // g1 knight
    if (board[7][2] == 'B') score -= DEVELOPMENT_BONUS; // c1 bishop
    if (board[7][5] == 'B') score -= DEVELOPMENT_BONUS; // f1 bishop
    
    // Czarne
    if (board[0][1] == 'n') score += DEVELOPMENT_BONUS; // b8 knight
    if (board[0][6] == 'n') score += DEVELOPMENT_BONUS; // g8 knight
    if (board[0][2] == 'b') score += DEVELOPMENT_BONUS; // c8 bishop
    if (board[0][5] == 'b') score += DEVELOPMENT_BONUS; // f8 bishop
    
    // Bonus za parę gońców
    int whiteBishops = 0, blackBishops = 0;
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            if (board[row][col] == 'B') whiteBishops++;
            if (board[row][col] == 'b') blackBishops++;
        }
    }
    if (whiteBishops >= 2) score += BISHOP_PAIR_BONUS;
    if (blackBishops >= 2) score -= BISHOP_PAIR_BONUS;
    
    // Bonus za wieże na otwartych/półotwartych kolumnach
    for (int col = 0; col < 8; col++) {
        bool hasWhitePawn = false, hasBlackPawn = false;
        int whiteRookRow = -1, blackRookRow = -1;
        
        for (int row = 0; row < 8; row++) {
            if (board[row][col] == 'P') hasWhitePawn = true;
            if (board[row][col] == 'p') hasBlackPawn = true;
            if (board[row][col] == 'R') whiteRookRow = row;
            if (board[row][col] == 'r') blackRookRow = row;
        }
        
        // Białe wieże
        if (whiteRookRow != -1) {
            if (!hasWhitePawn && !hasBlackPawn) {
                score += ROOK_OPEN_FILE_BONUS; // Otwarta kolumna
            } else if (!hasWhitePawn) {
                score += ROOK_SEMI_OPEN_FILE_BONUS; // Półotwarta kolumna
            }
        }
        
        // Czarne wieże
        if (blackRookRow != -1) {
            if (!hasWhitePawn && !hasBlackPawn) {
                score -= ROOK_OPEN_FILE_BONUS;
            } else if (!hasBlackPawn) {
                score -= ROOK_SEMI_OPEN_FILE_BONUS;
            }
        }
    }
    
    return score;
}

} // namespace Evaluator
