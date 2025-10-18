#pragma once
#include <vector>
#include <chrono>
#include <unordered_map>
#include "chess/model/Move.h"
#include "chess/ai/TranspositionTable.h"
#include "chess/ai/ZobristHash.h"

struct SearchResult {
    Move bestMove;
    int score;
    int depth;
    uint64_t nodesVisited;
    std::chrono::milliseconds timeSpent;
    
    SearchResult() : score(0), depth(0), nodesVisited(0), timeSpent(0) {}
};

class ChessAI {
private:
    // Parametry wyszukiwania - MUSZĄ BYĆ PIERWSZE!
    static constexpr int MAX_DEPTH = 50;
    static constexpr int MAX_TIME_MS = 30000; // 30 sekund
    static constexpr int MAX_KILLER_MOVES = 2;
    
    TranspositionTable transpositionTable;
    uint64_t nodesVisited;
    std::chrono::steady_clock::time_point searchStartTime;
    std::unordered_map<uint64_t, int> positionHistory; // Śledzenie powtórzeń pozycji
    
    // Killer moves - przechowuje 2 najlepsze "ciche" ruchy dla każdej głębokości
    Move killerMoves[MAX_DEPTH][MAX_KILLER_MOVES];
    
    // History heuristic - tablica [from][to] zliczająca jak dobre były ruchy
    int historyTable[8][8][8][8];
    
    // NegaMax z Alfa-Beta Pruning
    int negamax(const char board[8][8], char activeColor, const std::string& castling, 
                const std::string& enPassant, int depth, int alpha, int beta, 
                uint64_t zobristHash);
    
    // Quiescence Search - przeszukiwanie tylko bić (unika horizon effect)
    int quiescence(const char board[8][8], char activeColor, const std::string& castling,
                   const std::string& enPassant, int alpha, int beta);
    
    // Iterative Deepening
    SearchResult iterativeDeepening(const char board[8][8], char activeColor, 
                                   const std::string& castling, const std::string& enPassant,
                                   int maxDepth, int maxTimeMs);
    
    // Sprawdzenie czy czas się skończył
    bool isTimeUp() const;
    
    // Sortowanie ruchów dla lepszego Alfa-Beta Pruning
    void sortMoves(std::vector<Move>& moves, const char board[8][8], 
                   char activeColor, const std::string& castling, 
                   const std::string& enPassant, int depth);
    
    // Aktualizacja killer moves
    void updateKillerMove(const Move& move, int depth);
    
    // Sprawdzenie czy ruch jest killer move
    bool isKillerMove(const Move& move, int depth) const;
    
    // Czyszczenie heurystyk
    void clearHeuristics();
    
    // Pomocnicza funkcja do prawidłowego wykonywania ruchów (z obsługą specjalnych przypadków)
    void applyMove(const char boardIn[8][8], const Move& move, 
                   const std::string& castlingIn, const std::string& enPassantIn,
                   char boardOut[8][8], std::string& castlingOut, std::string& enPassantOut);
    
public:
    ChessAI();
    
    // Główna funkcja AI - zwraca najlepszy ruch
    SearchResult findBestMove(const char board[8][8], char activeColor, 
                             const std::string& castling, const std::string& enPassant,
                             int maxDepth = 20, int maxTimeMs = 5000);
    
    // Reset licznika węzłów
    void resetNodesCount() { nodesVisited = 0; }
    
    // Pobierz liczbę odwiedzonych węzłów
    uint64_t getNodesVisited() const { return nodesVisited; }
    
    // Wyczyść tablicę transpozycji
    void clearTranspositionTable() { transpositionTable.clear(); }
};
