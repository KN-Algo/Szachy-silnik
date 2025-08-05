//
// Created by siepk on 5.08.2025.
//

#include <iostream>
#include <string>
#include "board.h"

int main() {
    Board board;
    board.startBoard();
    board.printBoard();

    std::string move;
    while (true) {
        std::cout << "\nPodaj ruch (np. e2 to e4, 'exit' aby zakończyć): ";
        std::getline(std::cin, move);

        if (move == "exit") break;

        if (board.isMoveValid(move)) {
            board.makeMove(move);
            board.printBoard();
        } else {
            std::cout << "Ruch niepoprawny!\n";
        }
    }

    return 0;
}
