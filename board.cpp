//
// Created by jakub on 28.07.2025.
//
#include <iostream>
#include <string>
#include <sstream>
#include <cctype>
#include "board.h"
class Board {


private:
	char board[8][8];
	char activeColor;
	std::string castling;
	std::string enPassant;
	int halfmoveNumber;
	int fullmoveNumber;
public:
	void startBoard() {
		/*
		 The board at the beggining.
		 It uses the FEN notation, chceck repository to read more.
		 */
		std::string fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
		std::istringstream iss(fen);
		std::string piecePlacement;

		iss >> piecePlacement >> activeColor >> castling >> enPassant >> halfmoveNumber >> fullmoveNumber;

		int row = 0, col = 0;
		for (char c : piecePlacement) {
			if (c == '/') {
				row++;
				col = 0;
			} else if (isdigit(c)) {
				col += c - '0';
			} else {
				board[row][col] = c;
				col++;
			}
		}
	}

	void printBoard() {
		/*
		printing the state of game as the 8x8 board
		 */
		for (int row = 0; row < 8; row++) {
			for (int col = 0; col < 8; col++) {
				char piece = board[row][col];
				std::cout << (piece ? piece : '.') << " ";
			}
			std::cout << std::endl;
		}
		std::cout << "\nActive color: " << activeColor
                  << "\nCastling rights: " << castling
                  << "\nEn passant target: " << enPassant
                  << "\nHalfmove clock: " << halfmoveNumber
                  << "\nFullmove number: " << fullmoveNumber
                  << std::endl;
	}
};
int main() {
    Board board;            // Tworzymy obiekt klasy Board
    board.startBoard();     // Inicjalizujemy szachownicę domyślnym FEN-em
    board.printBoard();     // Wyświetlamy szachownicę i informacje o grze

    return 0;
}