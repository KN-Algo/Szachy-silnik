// Board.cpp
#include "chess/board/Board.h"
#include <iostream>
#include <sstream>
#include <string>
#include <cctype>

void Board::startBoard() {
    std::string fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
    int row = 0, col = 0;
    size_t i = 0;

    while (fen[i] != ' ') {
        char c = fen[i];
        if (c == '/') { row++; col = 0; }
        else if (isdigit(c)) { col += c - '0'; }
        else { board[row][col++] = c; }
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

    gameStateManager.clearHistory();
    gameStateManager.addPosition(board, activeColor, castling, enPassant);

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
}

std::vector<Move> Board::getLegalMoves() const {
    return MoveGenerator::generateLegalMoves(board, activeColor, castling, enPassant);
}
bool Board::hasLegalMoves() const {
    return MoveGenerator::hasLegalMoves(board, activeColor, castling, enPassant);
}
bool Board::isInCheck() const {
    return MoveGenerator::isInCheck(board, activeColor);
}
GameState Board::getGameState() const {
    return gameStateManager.checkGameState(board, activeColor, castling, enPassant,
                                          halfmoveClock, hasLegalMoves(), isInCheck());
}
std::string Board::getGameStateString() const {
    return gameStateManager.getGameStateString(getGameState(), activeColor);
}

