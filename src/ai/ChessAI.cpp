#include "chess/ai/ChessAI.h"
#include "chess/rules/MoveGenerator.h"
#include "chess/ai/Evaluator.h"
#include "chess/utils/Notation.h"
#include <algorithm>
#include <iostream>
#include <cmath>

ChessAI::ChessAI() : nodesVisited(0) {
    ZobristHash::initialize();
    clearHeuristics();
}

void ChessAI::clearHeuristics() {
    // Wyczyść killer moves
    for (int d = 0; d < MAX_DEPTH; d++) {
        for (int i = 0; i < MAX_KILLER_MOVES; i++) {
            killerMoves[d][i] = {0, 0, 0, 0, 0, 0, 0};
        }
    }
    
    // Wyczyść history table
    for (int fr = 0; fr < 8; fr++) {
        for (int fc = 0; fc < 8; fc++) {
            for (int tr = 0; tr < 8; tr++) {
                for (int tc = 0; tc < 8; tc++) {
                    historyTable[fr][fc][tr][tc] = 0;
                }
            }
        }
    }
}

void ChessAI::updateKillerMove(const Move& move, int depth) {
    if (depth < 0 || depth >= MAX_DEPTH) return;
    
    // Nie dodawaj bić jako killer moves (one już mają wysoki priorytet)
    if (move.capturedPiece) return;
    
    // Jeśli to już jest pierwszy killer move, nie rób nic
    if (killerMoves[depth][0].fromRow == move.fromRow &&
        killerMoves[depth][0].fromCol == move.fromCol &&
        killerMoves[depth][0].toRow == move.toRow &&
        killerMoves[depth][0].toCol == move.toCol) {
        return;
    }
    
    // Przesuń pierwszy killer move na drugą pozycję
    killerMoves[depth][1] = killerMoves[depth][0];
    // Dodaj nowy killer move na pierwszą pozycję
    killerMoves[depth][0] = move;
}

bool ChessAI::isKillerMove(const Move& move, int depth) const {
    if (depth < 0 || depth >= MAX_DEPTH) return false;
    
    for (int i = 0; i < MAX_KILLER_MOVES; i++) {
        if (killerMoves[depth][i].fromRow == move.fromRow &&
            killerMoves[depth][i].fromCol == move.fromCol &&
            killerMoves[depth][i].toRow == move.toRow &&
            killerMoves[depth][i].toCol == move.toCol) {
            return true;
        }
    }
    return false;
}

SearchResult ChessAI::findBestMove(const char board[8][8], char activeColor, 
                                  const std::string& castling, const std::string& enPassant,
                                  int maxDepth, int maxTimeMs) {
    resetNodesCount();
    searchStartTime = std::chrono::steady_clock::now();
    
    return iterativeDeepening(board, activeColor, castling, enPassant, maxDepth, maxTimeMs);
}

SearchResult ChessAI::iterativeDeepening(const char board[8][8], char activeColor, 
                                        const std::string& castling, const std::string& enPassant,
                                        int maxDepth, int maxTimeMs) {
    SearchResult result;
    result.bestMove = {0, 0, 0, 0, '?', 0}; // Domyślny ruch
    
    // Wyczyść historię pozycji
    positionHistory.clear();
    
    // Dodaj pozycję startową do historii
    uint64_t startHash = ZobristHash::calculateHash(board, activeColor, castling, enPassant);
    positionHistory[startHash] = 1;
    
    // Generuj wszystkie legalne ruchy
    std::vector<Move> moves = MoveGenerator::generateLegalMoves(board, activeColor, castling, enPassant);
  
    // Sprawdź liczbę ruchów
    if (moves.empty()) {
        std::cout << "UWAGA: AI nie znalazł żadnych legalnych ruchów!" << std::endl;
        return result;
    }
    
    std::cout << "DEBUG: Wygenerowano " << moves.size() << " legalnych ruchów" << std::endl;
    
    // Sortuj ruchy dla lepszego Alfa-Beta Pruning (depth = 0 dla początkowego sortowania)
    sortMoves(moves, board, activeColor, castling, enPassant, 0);
    
    // Pokaż pierwszy ruch po sortowaniu
    std::cout << "DEBUG: Pierwszy ruch po sortowaniu: (" << moves[0].fromRow << "," << moves[0].fromCol 
              << ") -> (" << moves[0].toRow << "," << moves[0].toCol << ") figura: " << moves[0].movedPiece << std::endl;
    
    // Iterative Deepening - zaczynamy od głębokości 1
    for (int depth = 1; depth <= maxDepth; depth++) {
        if (isTimeUp()) break;
        
        SearchResult currentResult;
        currentResult.depth = depth;
        currentResult.bestMove = moves[0]; // Domyślnie pierwszy ruch
        
        // WAŻNE: Nie używaj std::numeric_limits<int>::min() bo negacja powoduje overflow!
        int bestScore = -999999;
        int alpha = -999999;
        int beta = 999999;
        
        // Wyszukaj najlepszy ruch dla aktualnej głębokości
        for (const Move& move : moves) {
            // Symuluj ruch
            char tempBoard[8][8];
            std::string tempCastling;
            std::string tempEnPassant;
            
            // Wykonaj ruch prawidłowo (z obsługą specjalnych przypadków)
            applyMove(board, move, castling, enPassant, tempBoard, tempCastling, tempEnPassant);

            // Zmień stronę do ruchu
            char tempActiveColor = (activeColor == 'w') ? 'b' : 'w';
            
            // Oblicz nowy hash
            uint64_t newHash = ZobristHash::calculateHash(tempBoard, tempActiveColor, tempCastling, tempEnPassant);
            
            // Wykonaj wyszukiwanie NegaMax
            int score = -negamax(tempBoard, tempActiveColor, tempCastling, tempEnPassant, 
                                depth - 1, -beta, -alpha, newHash);
            
            if (score > bestScore) {
                bestScore = score;
                currentResult.bestMove = move;
            }
            
            alpha = std::max(alpha, score);
            if (alpha >= beta) break; // Beta cutoff
        }
        
        currentResult.score = bestScore;
        currentResult.nodesVisited = nodesVisited;
        currentResult.timeSpent = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - searchStartTime);
        
        // Jeśli czas się skończył, przerwij
        if (isTimeUp()) break;
        
        // Zaktualizuj wynik
        result = currentResult;
        
        // Jeśli znaleźliśmy mata, nie ma potrzeby szukać głębiej
        if (std::abs(result.score) > 90000) break;
        
        // Sprawdź czy AI znalazło ruch
        if (result.bestMove.fromRow == 0 && result.bestMove.fromCol == 0 && 
            result.bestMove.toRow == 0 && result.bestMove.toCol == 0) {
            std::cout << "UWAGA: AI nie znalazło żadnego ruchu!" << std::endl;
            break;
        }

        std::cout << "Głębokość " << depth << ": " << result.score
                  << " (węzły: " << result.nodesVisited << ")" << std::endl;
    }
    
    std::cout << "DEBUG: AI wybrało ruch: (" << result.bestMove.fromRow << "," << result.bestMove.fromCol 
              << ") -> (" << result.bestMove.toRow << "," << result.bestMove.toCol 
              << ") figura: " << result.bestMove.movedPiece << std::endl;
    
    // Przed zwróceniem wyniku, sprawdź czy wybrany ruch jest w liście legalnych ruchów
    bool foundMove = false;
    for (const Move& m : moves) {
        if (m.fromRow == result.bestMove.fromRow && m.fromCol == result.bestMove.fromCol &&
            m.toRow == result.bestMove.toRow && m.toCol == result.bestMove.toCol &&
            m.promotion == result.bestMove.promotion) {
            // Znaleziono ruch w liście - upewnij się że wszystkie pola są prawidłowe
            result.bestMove = m;
            foundMove = true;
            break;
        }
    }
    
    if (!foundMove && !moves.empty()) {
        std::cout << "OSTRZEŻENIE: AI wybrał ruch spoza listy legalnych! Używam pierwszego legalnego ruchu." << std::endl;
        result.bestMove = moves[0];
    }
    
    return result;
}

int ChessAI::quiescence(const char board[8][8], char activeColor, const std::string& castling,
                        const std::string& enPassant, int alpha, int beta) {
    nodesVisited++;
    
    // Stand pat - ocena statyczna pozycji
    int standPat = Evaluator::evaluatePosition(board, activeColor);
    
    // Beta cutoff - jeśli nasza pozycja jest już na tyle dobra
    if (standPat >= beta) {
        return beta;
    }
    
    // Aktualizuj alfa
    if (standPat > alpha) {
        alpha = standPat;
    }
    
    // Generuj tylko "głośne" ruchy: bicia, promocje, i w końcówkach także szachy
    std::vector<Move> allMoves = MoveGenerator::generateLegalMoves(board, activeColor, castling, enPassant);
    std::vector<Move> loudMoves;
    
    bool isEndgame = Evaluator::isEndgame(board);
    int materialBalance = Evaluator::getMaterialBalance(board);
    bool hasAdvantage = std::abs(materialBalance) > 200; // Przewaga > 2 piony
    
    for (const Move& move : allMoves) {
        // Zawsze dodaj bicia i promocje
        if (move.capturedPiece || move.promotion) {
            loudMoves.push_back(move);
            continue;
        }
        
        // W końcówce z przewagą: dodaj szachy (ruchy które dają szach)
        if (isEndgame && hasAdvantage) {
            // Symuluj ruch i sprawdź czy daje szach
            char tempBoard[8][8];
            std::string tempCastling;
            std::string tempEnPassant;
            
            applyMove(board, move, castling, enPassant, tempBoard, tempCastling, tempEnPassant);
            
            char enemyColor = (activeColor == 'w') ? 'b' : 'w';
            bool givesCheck = MoveGenerator::isInCheck(tempBoard, enemyColor);
            
            if (givesCheck) {
                loudMoves.push_back(move);
            }
        }
    }
    
    // Jeśli brak "głośnych" ruchów, zwróć statyczną ocenę
    if (loudMoves.empty()) {
        return standPat;
    }
    
    // Sortuj "głośne" ruchy (MVV-LVA dla bić, szachy na końcu)
    std::sort(loudMoves.begin(), loudMoves.end(), [](const Move& a, const Move& b) {
        int scoreA = 0, scoreB = 0;
        
        if (a.capturedPiece) {
            switch (std::toupper(a.capturedPiece)) {
                case 'P': scoreA += 100; break;
                case 'N': scoreA += 320; break;
                case 'B': scoreA += 330; break;
                case 'R': scoreA += 500; break;
                case 'Q': scoreA += 900; break;
            }
            // Odejmij wartość atakującego (MVV-LVA)
            switch (std::toupper(a.movedPiece)) {
                case 'P': scoreA -= 1; break;
                case 'N': scoreA -= 3; break;
                case 'B': scoreA -= 3; break;
                case 'R': scoreA -= 5; break;
                case 'Q': scoreA -= 9; break;
            }
        }
        
        if (b.capturedPiece) {
            switch (std::toupper(b.capturedPiece)) {
                case 'P': scoreB += 100; break;
                case 'N': scoreB += 320; break;
                case 'B': scoreB += 330; break;
                case 'R': scoreB += 500; break;
                case 'Q': scoreB += 900; break;
            }
            switch (std::toupper(b.movedPiece)) {
                case 'P': scoreB -= 1; break;
                case 'N': scoreB -= 3; break;
                case 'B': scoreB -= 3; break;
                case 'R': scoreB -= 5; break;
                case 'Q': scoreB -= 9; break;
            }
        }
        
        // Priorytetyzuj promocje
        if (a.promotion) scoreA += 800;
        if (b.promotion) scoreB += 800;
        
        return scoreA > scoreB;
    });
    
    // Przeszukaj "głośne" ruchy
    for (const Move& move : loudMoves) {
        char tempBoard[8][8];
        std::string tempCastling;
        std::string tempEnPassant;
        
        applyMove(board, move, castling, enPassant, tempBoard, tempCastling, tempEnPassant);
        
        char tempActiveColor = (activeColor == 'w') ? 'b' : 'w';
        
        int score = -quiescence(tempBoard, tempActiveColor, tempCastling, tempEnPassant, -beta, -alpha);
        
        if (score >= beta) {
            return beta;
        }
        
        if (score > alpha) {
            alpha = score;
        }
    }
    
    return alpha;
}

int ChessAI::negamax(const char board[8][8], char activeColor, const std::string& castling, 
                     const std::string& enPassant, int depth, int alpha, int beta, 
                     uint64_t zobristHash) {
    nodesVisited++;
    
    // Sprawdź powtórzenia pozycji (threefold repetition)
    if (positionHistory[zobristHash] >= 2) {
        // Pozycja wystąpiła już 2 razy, więc to będzie 3rd repetition = remis
        // Ale jeśli mamy przewagę materialną, to jest dla nas złe
        int materialBalance = Evaluator::getMaterialBalance(board);
        if (std::abs(materialBalance) > 200) {
            // Mamy znaczącą przewagę - repetycja to strata
            return materialBalance > 0 ? -50 : 50; // Kara za repetycję
        }
        return 0; // Standardowy remis
    }
    
    // Sprawdź tablicę transpozycji
    int ttScore;
    NodeType ttNodeType;
    if (transpositionTable.probe(zobristHash, depth, ttScore, ttNodeType)) {
        if (ttNodeType == NodeType::EXACT) {
            return ttScore;
        } else if (ttNodeType == NodeType::ALPHA && ttScore <= alpha) {
            return alpha;
        } else if (ttNodeType == NodeType::BETA && ttScore >= beta) {
            return beta;
        }
    }
    
    // Sprawdź czy czas się skończył
    if (isTimeUp()) {
        return 0;
    }
    
    // Sprawdź czy osiągnęliśmy maksymalną głębokość
    if (depth == 0) {
        // Zamiast zwracać ocenę statyczną, użyj quiescence search
        return quiescence(board, activeColor, castling, enPassant, alpha, beta);
    }
    
    // Sprawdź stan gry
    bool hasLegalMoves = MoveGenerator::hasLegalMoves(board, activeColor, castling, enPassant);
    bool isInCheck = MoveGenerator::isInCheck(board, activeColor);
    
    if (!hasLegalMoves) {
        if (isInCheck) {
            // Mat - im bliżej mata, tym większa wartość (dodaj depth żeby preferować szybsze maty)
            int mateScore = 100000 - depth;
            int score = (activeColor == 'w') ? -mateScore : mateScore;
            transpositionTable.store(zobristHash, depth, score, NodeType::EXACT);
            return score;
        } else {
            // Pat - remis, ale jeśli mamy przewagę to jest dla nas złe
            int materialBalance = Evaluator::getMaterialBalance(board);
            int patScore = 0;
            
            // Jeśli mamy dużą przewagę materialną, pat to dla nas duża strata
            if (std::abs(materialBalance) > 300) {
                // Przewaga > 3 piony - pat to prawie jak przegrana
                patScore = (materialBalance > 0) ? -500 : 500;
            } else if (std::abs(materialBalance) > 200) {
                // Przewaga > 2 piony - pat to znacząca strata
                patScore = (materialBalance > 0) ? -200 : 200;
            }
            
            transpositionTable.store(zobristHash, depth, patScore, NodeType::EXACT);
            return patScore;
        }
    }
    
    // Generuj wszystkie legalne ruchy
    std::vector<Move> moves = MoveGenerator::generateLegalMoves(board, activeColor, castling, enPassant);
    
    // Sortuj ruchy dla lepszego Alfa-Beta Pruning
    sortMoves(moves, board, activeColor, castling, enPassant, depth);
    
    // WAŻNE: Nie używaj std::numeric_limits<int>::min() bo negacja powoduje overflow!
    int bestScore = -999999;
    NodeType bestNodeType = NodeType::ALPHA;
    int originalAlpha = alpha;
    
    for (const Move& move : moves) {
        // Symuluj ruch
        char tempBoard[8][8];
        std::string tempCastling;
        std::string tempEnPassant;
        
        // Wykonaj ruch prawidłowo (z obsługą specjalnych przypadków)
        applyMove(board, move, castling, enPassant, tempBoard, tempCastling, tempEnPassant);
      
        // Zmień stronę do ruchu
        char tempActiveColor = (activeColor == 'w') ? 'b' : 'w';
        
        // Oblicz nowy hash
        uint64_t newHash = ZobristHash::calculateHash(tempBoard, tempActiveColor, tempCastling, tempEnPassant);
        
        // Dodaj pozycję do historii
        positionHistory[newHash]++;
        
        // Rekurencyjne wywołanie NegaMax
        int score = -negamax(tempBoard, tempActiveColor, tempCastling, tempEnPassant, 
                            depth - 1, -beta, -alpha, newHash);
        
        // Usuń pozycję z historii
        positionHistory[newHash]--;
        
        if (score > bestScore) {
            bestScore = score;
        }
        
        // Alfa-Beta Pruning
        if (score >= beta) {
            // Beta cutoff - ten ruch jest tak dobry że przeciwnik go nie dopuści
            // Zapisz jako killer move (jeśli to nie było bicie)
            if (!move.capturedPiece) {
                updateKillerMove(move, depth);
                // Zwiększ wartość w history table
                historyTable[move.fromRow][move.fromCol][move.toRow][move.toCol] += depth * depth;
            }
            
            transpositionTable.store(zobristHash, depth, score, NodeType::BETA);
            return score;
        }
        
        if (score > alpha) {
            alpha = score;
            bestNodeType = NodeType::EXACT;
        }
    }
    
    // Określ typ węzła dla tablicy transpozycji
    if (bestScore <= originalAlpha) {
        bestNodeType = NodeType::ALPHA;
    } else if (bestScore >= beta) {
        bestNodeType = NodeType::BETA;
    } else {
        bestNodeType = NodeType::EXACT;
    }
    
    // Zapisz wynik w tablicy transpozycji
    transpositionTable.store(zobristHash, depth, bestScore, bestNodeType);
    
    return bestScore;
}

bool ChessAI::isTimeUp() const {
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - searchStartTime);
    return elapsed.count() >= MAX_TIME_MS;
}

void ChessAI::sortMoves(std::vector<Move>& moves, const char board[8][8], 
                        char activeColor, const std::string& castling, 
                        const std::string& enPassant, int depth) {
    // Sortuj ruchy według priorytetu (BARDZO WAŻNE dla alfa-beta pruning):
    // 1. Bicia (z wartością bicia - MVV/LVA)
    // 2. Ruchy promocji  
    // 3. Killer moves (historycznie dobre "ciche" ruchy)
    // 4. History heuristic
    // 5. W końcówce: ruchy króla które ograniczają przeciwnika
    // 6. Ruchy do centrum
    // 7. Pozostałe ruchy
    
    bool isEndgame = Evaluator::isEndgame(board);
    int materialBalance = Evaluator::getMaterialBalance(board);
    
    std::sort(moves.begin(), moves.end(), [&](const Move& a, const Move& b) {
        int scoreA = 0, scoreB = 0;
        
        // Bicia
        if (a.capturedPiece) {
            scoreA += 1000 + Evaluator::PAWN_VALUE; // Podstawowy bonus za bicie
            switch (std::toupper(a.capturedPiece)) {
                case 'P': scoreA += Evaluator::PAWN_VALUE; break;
                case 'N': scoreA += Evaluator::KNIGHT_VALUE; break;
                case 'B': scoreA += Evaluator::BISHOP_VALUE; break;
                case 'R': scoreA += Evaluator::ROOK_VALUE; break;
                case 'Q': scoreA += Evaluator::QUEEN_VALUE; break;
            }
        }
        if (b.capturedPiece) {
            scoreB += 1000 + Evaluator::PAWN_VALUE;
            switch (std::toupper(b.capturedPiece)) {
                case 'P': scoreB += Evaluator::PAWN_VALUE; break;
                case 'N': scoreB += Evaluator::KNIGHT_VALUE; break;
                case 'B': scoreB += Evaluator::BISHOP_VALUE; break;
                case 'R': scoreB += Evaluator::ROOK_VALUE; break;
                case 'Q': scoreB += Evaluator::QUEEN_VALUE; break;
            }
        }
        
        // Promocje
        if (a.promotion) scoreA += 500;
        if (b.promotion) scoreB += 500;
        
        // Killer moves - historycznie dobre "ciche" ruchy na tej głębokości
        if (!a.capturedPiece && isKillerMove(a, depth)) {
            scoreA += 400;
        }
        if (!b.capturedPiece && isKillerMove(b, depth)) {
            scoreB += 400;
        }
        
        // History heuristic - ruchy które były dobre w innych pozycjach
        if (!a.capturedPiece) {
            scoreA += historyTable[a.fromRow][a.fromCol][a.toRow][a.toCol] / 10;
        }
        if (!b.capturedPiece) {
            scoreB += historyTable[b.fromRow][b.fromCol][b.toRow][b.toCol] / 10;
        }
        
        // W końcówce z przewagą materialną: priorytet ruchów króla które atakują
        if (isEndgame && std::abs(materialBalance) > 200) {
            char movedPieceA = board[a.fromRow][a.fromCol];
            char movedPieceB = board[b.fromRow][b.fromCol];
            
            // Znajdź króla przeciwnika
            int enemyKingRow = -1, enemyKingCol = -1;
            char enemyKing = (activeColor == 'w') ? 'k' : 'K';
            for (int r = 0; r < 8; r++) {
                for (int c = 0; c < 8; c++) {
                    if (board[r][c] == enemyKing) {
                        enemyKingRow = r; enemyKingCol = c;
                        break;
                    }
                }
                if (enemyKingRow != -1) break;
            }
            
            if (enemyKingRow != -1) {
                // Bonus dla ruchów króla które zbliżają się do króla przeciwnika
                if (std::toupper(movedPieceA) == 'K') {
                    int distBefore = Evaluator::manhattanDistance(a.fromRow, a.fromCol, enemyKingRow, enemyKingCol);
                    int distAfter = Evaluator::manhattanDistance(a.toRow, a.toCol, enemyKingRow, enemyKingCol);
                    if (distAfter < distBefore) scoreA += 200;
                }
                if (std::toupper(movedPieceB) == 'K') {
                    int distBefore = Evaluator::manhattanDistance(b.fromRow, b.fromCol, enemyKingRow, enemyKingCol);
                    int distAfter = Evaluator::manhattanDistance(b.toRow, b.toCol, enemyKingRow, enemyKingCol);
                    if (distAfter < distBefore) scoreB += 200;
                }
            }
        }
        
        // Ruchy do centrum (w otwarciu/middlegame)
        if (!isEndgame) {
            int centerDistanceA = std::abs(a.toRow - 3.5) + std::abs(a.toCol - 3.5);
            int centerDistanceB = std::abs(b.toRow - 3.5) + std::abs(b.toCol - 3.5);
            scoreA += (7 - centerDistanceA) * 10;
            scoreB += (7 - centerDistanceB) * 10;
        }
        
        return scoreA > scoreB;
    });
}

void ChessAI::applyMove(const char boardIn[8][8], const Move& move, 
                        const std::string& castlingIn, const std::string& enPassantIn,
                        char boardOut[8][8], std::string& castlingOut, std::string& enPassantOut) {
    using namespace notation;
    
    // Skopiuj planszę
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            boardOut[i][j] = boardIn[i][j];
        }
    }
    
    castlingOut = castlingIn;
    enPassantOut = "-";
    
    char moved = boardIn[move.fromRow][move.fromCol];
    char captured = boardIn[move.toRow][move.toCol];
    bool whiteMoved = std::isupper(moved);
    
    // Helper do usuwania praw roszady
    auto dropRight = [&](char r) {
        if (castlingOut == "-") return;
        size_t p = castlingOut.find(r);
        if (p != std::string::npos) {
            castlingOut.erase(p, 1);
            if (castlingOut.empty()) castlingOut = "-";
        }
    };
    
    // 1) Poruszył się KRÓL -> tracimy oba prawa tej strony
    if (std::toupper(moved) == 'K') {
        if (whiteMoved) { dropRight('K'); dropRight('Q'); }
        else            { dropRight('k'); dropRight('q'); }
    }
    
    // 2) Poruszyła się WIEŻA ze startowego rogu -> tracimy odpowiednie prawo
    if (moved == 'R') {
        if (move.fromRow == 7 && move.fromCol == 7) dropRight('K'); // h1
        if (move.fromRow == 7 && move.fromCol == 0) dropRight('Q'); // a1
    }
    if (moved == 'r') {
        if (move.fromRow == 0 && move.fromCol == 7) dropRight('k'); // h8
        if (move.fromRow == 0 && move.fromCol == 0) dropRight('q'); // a8
    }
    
    // 3) Zbicie wieży przeciwnika ze startowego rogu -> oni tracą prawo
    if (captured == 'R') {
        if (move.toRow == 7 && move.toCol == 7) dropRight('K'); // białe O-O
        if (move.toRow == 7 && move.toCol == 0) dropRight('Q'); // białe O-O-O
    }
    if (captured == 'r') {
        if (move.toRow == 0 && move.toCol == 7) dropRight('k'); // czarne O-O
        if (move.toRow == 0 && move.toCol == 0) dropRight('q'); // czarne O-O-O
    }
    
    // Sprawdź specjalne przypadki
    bool isCastle = (std::toupper(moved) == 'K' && std::abs(move.toCol - move.fromCol) == 2);
    bool isPawn   = (std::toupper(moved) == 'P');
    
    // En passant: czy to EP?
    bool isEnPassantCapture = false;
    int dir = whiteMoved ? -1 : 1;
    if (isPawn) {
        int dr = move.toRow - move.fromRow;
        int dc = move.toCol - move.fromCol;
        int adc = std::abs(dc);
        if (adc == 1 && dr == dir && captured == 0 && enPassantIn != "-") {
            int epRow, epCol;
            if (algToCoord(enPassantIn, epRow, epCol) && epRow == move.toRow && epCol == move.toCol) {
                isEnPassantCapture = true;
            }
        }
    }
    
    // Jeśli EP – zdejmij piona z „miniętego" pola
    if (isEnPassantCapture) {
        int capRow = move.toRow - dir;
        int capCol = move.toCol;
        boardOut[capRow][capCol] = 0;
    }
    
    // Wykonanie ruchu figury
    boardOut[move.toRow][move.toCol] = moved;
    boardOut[move.fromRow][move.fromCol] = 0;
    
    // Roszada: przesuń wieżę
    if (isCastle) {
        int row = move.toRow;
        if (move.toCol == 6) {            // O-O
            boardOut[row][5] = boardOut[row][7];
            boardOut[row][7] = 0;
        } else if (move.toCol == 2) {     // O-O-O
            boardOut[row][3] = boardOut[row][0];
            boardOut[row][0] = 0;
        }
    }
    
    // Promocja piona
    if (isPawn && (move.toRow == 0 || move.toRow == 7)) {
        char promo = move.promotion ? move.promotion : 'Q';
        if (!whiteMoved) promo = std::tolower(promo);
        boardOut[move.toRow][move.toCol] = promo;
    }
    
    // Ustawianie enPassant po podwójnym ruchu piona
    if (isPawn) {
        if ((whiteMoved && move.fromRow == 6 && move.toRow == 4) ||
            (!whiteMoved && move.fromRow == 1 && move.toRow == 3))
        {
            int passRow = move.fromRow + (whiteMoved ? -1 : 1);
            int passCol = move.fromCol;
            enPassantOut = coordToAlg(passRow, passCol);
        }
    }
}
