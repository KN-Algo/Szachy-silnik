//
// Created by siepk on 5.08.2025.
//

#include <iostream>
#include <string>
#include <sstream>
#include <cctype>
#include "board.h"

Move parseMove(const std::string& input) {
    Move move = {-1, -1, -1, -1, 0, 0};
    
    if (input.length() < 4) return move;
    
    std::string cleanInput = input;
    size_t toPos = cleanInput.find(" to ");
    if (toPos != std::string::npos) {
        cleanInput = cleanInput.substr(0, toPos) + cleanInput.substr(toPos + 4);
    }
    
    cleanInput.erase(std::remove(cleanInput.begin(), cleanInput.end(), ' '), cleanInput.end());
    
    if (cleanInput.length() < 4) return move;
    
    char fromFile = cleanInput[0];
    char fromRank = cleanInput[1];
    
    char toFile = cleanInput[2];
    char toRank = cleanInput[3];
    
    if (fromFile < 'a' || fromFile > 'h' || fromRank < '1' || fromRank > '8' ||
        toFile < 'a' || toFile > 'h' || toRank < '1' || toRank > '8') {
        return move;
    }
    
    move.fromCol = fromFile - 'a';
    move.fromRow = '8' - fromRank;
    move.toCol = toFile - 'a';
    move.toRow = '8' - toRank;
    
    return move;
}

int main() {
    Board board;
    board.startBoard();
    board.printBoard();

    std::string input;
    while (true) {
        GameState state = board.getGameState();
        if (state != GameState::PLAYING) {
            std::cout << "\nGra zakończona: " << board.getGameStateString() << std::endl;
            std::cout << "Wpisz 'exit' aby wyjść lub 'restart' aby rozpocząć nową grę: ";
            std::getline(std::cin, input);
            
            if (input == "exit") break;
            if (input == "restart") {
                board.startBoard();
                board.printBoard();
                continue;
            }
            continue;
        }
        
        std::cout << "\nPodaj ruch (np. e2e4 lub e2 to e4, 'exit' aby zakończyć): ";
        std::getline(std::cin, input);

        if (input == "exit") break;
        
        if (input == "help") {
            std::cout << "Dostępne komendy:\n";
            std::cout << "- Ruch: e2e4 lub e2 to e4\n";
            std::cout << "- help: pokaż tę pomoc\n";
            std::cout << "- moves: pokaż wszystkie legalne ruchy\n";
            std::cout << "- restart: rozpocznij nową grę\n";
            std::cout << "- exit: zakończ program\n";
            continue;
        }
        
        if (input == "moves") {
            auto legalMoves = board.getLegalMoves();
            std::cout << "Legalne ruchy (" << legalMoves.size() << "):\n";
            for (const auto& move : legalMoves) {
                char fromFile = 'a' + move.fromCol;
                char fromRank = '8' - move.fromRow;
                char toFile = 'a' + move.toCol;
                char toRank = '8' - move.toRow;
                std::cout << fromFile << fromRank << toFile << toRank << " ";
            }
            std::cout << std::endl;
            continue;
        }
        
        if (input == "restart") {
            board.startBoard();
            board.printBoard();
            continue;
        }

        Move move = parseMove(input);
        if (move.fromRow == -1) {
            std::cout << "Niepoprawny format ruchu! Użyj np. e2e4 lub e2 to e4\n";
            continue;
        }
        
        if (board.isMoveValid(move)) {
            board.makeMove(move);
            board.printBoard();
            
            GameState newState = board.getGameState();
            if (newState != GameState::PLAYING) {
                std::cout << "\n" << board.getGameStateString() << std::endl;
            } else if (board.isInCheck()) {
                std::cout << "\nSzach!" << std::endl;
            }
        } else {
            std::cout << "Ruch niepoprawny!\n";
        }
    }

    return 0;
}
