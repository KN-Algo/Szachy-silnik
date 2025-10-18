#include <iostream>
#include <string>
#include <vector>
#include "chess/board/Board.h"
#include "chess/model/Move.h"
#include "chess/utils/Notation.h"
#include "chess/rules/MoveGenerator.h"

using namespace notation;

// Pomocnicza funkcja do konwersji stringa ruchu "a1b2" do struktury Move
Move stringToMove(const std::string& moveStr) {
    Move move;
    if (moveStr.length() < 4) {
        std::cerr << "Nieprawidlowy format ruchu: " << moveStr << std::endl;
        return move;
    }
    
    int fromRow, fromCol, toRow, toCol;
    if (!algToCoord(moveStr.substr(0, 2), fromRow, fromCol) || 
        !algToCoord(moveStr.substr(2, 2), toRow, toCol)) {
        std::cerr << "Nieprawidlowe wspolrzedne w ruchu: " << moveStr << std::endl;
        return move;
    }
    
    move.fromRow = fromRow;
    move.fromCol = fromCol;
    move.toRow = toRow;
    move.toCol = toCol;
    
    return move;
}

void testCaptureOwnPieceWhite() {
    std::cout << "\n=== Test: Bicie swojego koloru (białe) ===" << std::endl;
    Board board;
    // Ustawienie pozycji: biały pion na e4, biały pion na d5
    board.loadFEN("4k3/8/8/3P4/4P3/8/8/4K3 w - - 0 1");
    board.printBoard();
    
    Move move = stringToMove("e4d5");
    bool isValid = board.isMoveValid(move);
    std::cout << "Ruch e4d5 (bicie swojego pionka): " << (isValid ? "DOZWOLONY" : "ZAKAZANY") << std::endl;
    
    // Wykonaj ruch mimo że jest nieprawidłowy, aby zobaczyć efekt
    std::cout << "Wykonuje ruch mimo że jest nieprawidłowy..." << std::endl;
    board.makeMove(move);
    board.printBoard();
}

void testCaptureOwnPieceBlack() {
    std::cout << "\n=== Test: Bicie swojego koloru (czarne) ===" << std::endl;
    Board board;
    // Ustawienie pozycji: czarny pion na e5, czarny pion na d4
    board.loadFEN("4k3/8/8/8/3p4/4p3/8/4K3 b - - 0 1");
    board.printBoard();
    
    Move move = stringToMove("e3d4");
    bool isValid = board.isMoveValid(move);
    std::cout << "Ruch e3d4 (bicie swojego pionka): " << (isValid ? "DOZWOLONY" : "ZAKAZANY") << std::endl;
    
    // Wykonaj ruch mimo że jest nieprawidłowy, aby zobaczyć efekt
    std::cout << "Wykonuje ruch mimo że jest nieprawidłowy..." << std::endl;
    board.makeMove(move);
    board.printBoard();
}

void testEnPassantWhite() {
    std::cout << "\n=== Test: Bicie w przelocie (białe) ===" << std::endl;
    Board board;
    // Ustawienie pozycji: biały pion na e5, czarny pion na f7
    // Czarny ruch f7f5, potem biały exf6
    board.loadFEN("4k3/5p2/8/4P3/8/8/8/4K3 b - e6 0 1");
    board.printBoard();
    
    // Najpierw wykonaj ruch f7f5
    Move moveF5 = stringToMove("f7f5");
    if (board.isMoveValid(moveF5)) {
        board.makeMove(moveF5);
        std::cout << "Wykonano ruch f7f5" << std::endl;
        board.printBoard();
        
        // Teraz sprawdź ruch exf6 (en passant)
        Move moveEP = stringToMove("e5f6");
        bool isValid = board.isMoveValid(moveEP);
        std::cout << "Ruch e5f6 (en passant): " << (isValid ? "DOZWOLONY" : "ZAKAZANY") << std::endl;
        
        if (isValid) {
            board.makeMove(moveEP);
            board.printBoard();
        }
    } else {
        std::cout << "Ruch f7f5 jest nieprawidlowy!" << std::endl;
    }
}

void testEnPassantBlack() {
    std::cout << "\n=== Test: Bicie w przelocie (czarne) ===" << std::endl;
    Board board;
    // Ustawienie pozycji: czarny pion na e4, biały pion na d2
    // Biały ruch d2d4, potem czarny exd3
    board.loadFEN("4k3/8/8/8/4p3/8/3P4/4K3 w - e3 0 1");
    board.printBoard();
    
    // Najpierw wykonaj ruch d2d4
    Move moveD4 = stringToMove("d2d4");
    if (board.isMoveValid(moveD4)) {
        board.makeMove(moveD4);
        std::cout << "Wykonano ruch d2d4" << std::endl;
        board.printBoard();
        
        // Teraz sprawdź ruch exd3 (en passant)
        Move moveEP = stringToMove("e4d3");
        bool isValid = board.isMoveValid(moveEP);
        std::cout << "Ruch e4d3 (en passant): " << (isValid ? "DOZWOLONY" : "ZAKAZANY") << std::endl;
        
        if (isValid) {
            board.makeMove(moveEP);
            board.printBoard();
        }
    } else {
        std::cout << "Ruch d2d4 jest nieprawidlowy!" << std::endl;
    }
}

void testCastlingWhite() {
    std::cout << "\n=== Test: Roszada (białe) ===" << std::endl;
    Board board;
    // Ustawienie pozycji: biały król na e1, wieże na a1 i h1, bez ruchu króla i wież
    board.loadFEN("4k3/8/8/8/8/8/8/R3K2R w KQ - 0 1");
    board.printBoard();
    
    // Sprawdź krótką roszadę
    Move moveOO = stringToMove("e1g1");
    bool isValidOO = board.isMoveValid(moveOO);
    std::cout << "Ruch e1g1 (krotka roszada): " << (isValidOO ? "DOZWOLONY" : "ZAKAZANY") << std::endl;
    
    if (isValidOO) {
        std::string reason = board.checkCastlingReason(moveOO);
        std::cout << "Powod: " << reason << std::endl;
    }
    
    // Sprawdź długą roszadę
    Move moveOOO = stringToMove("e1c1");
    bool isValidOOO = board.isMoveValid(moveOOO);
    std::cout << "Ruch e1c1 (dluga roszada): " << (isValidOOO ? "DOZWOLONY" : "ZAKAZANY") << std::endl;
    
    if (isValidOOO) {
        std::string reason = board.checkCastlingReason(moveOOO);
        std::cout << "Powod: " << reason << std::endl;
    }
}

void testCastlingBlack() {
    std::cout << "\n=== Test: Roszada (czarne) ===" << std::endl;
    Board board;
    // Ustawienie pozycji: czarny król na e8, wieże na a8 i h8, bez ruchu króla i wież
    board.loadFEN("r3k2r/8/8/8/8/8/8/4K3 b kq - 0 1");
    board.printBoard();
    
    // Sprawdź krótką roszadę
    Move moveOO = stringToMove("e8g8");
    bool isValidOO = board.isMoveValid(moveOO);
    std::cout << "Ruch e8g8 (krotka roszada): " << (isValidOO ? "DOZWOLONY" : "ZAKAZANY") << std::endl;
    
    if (isValidOO) {
        std::string reason = board.checkCastlingReason(moveOO);
        std::cout << "Powod: " << reason << std::endl;
    }
    
    // Sprawdź długą roszadę
    Move moveOOO = stringToMove("e8c8");
    bool isValidOOO = board.isMoveValid(moveOOO);
    std::cout << "Ruch e8c8 (dluga roszada): " << (isValidOOO ? "DOZWOLONY" : "ZAKAZANY") << std::endl;
    
    if (isValidOOO) {
        std::string reason = board.checkCastlingReason(moveOOO);
        std::cout << "Powod: " << reason << std::endl;
    }
}

void testPromotionWhite() {
    std::cout << "\n=== Test: Promocja (białe) ===" << std::endl;
    Board board;
    // Ustawienie pozycji: biały pion na e7, puste pole na e8
    board.loadFEN("8/4P3/8/8/8/8/8/4K3 w - - 0 1");
    board.printBoard();
    
    // Sprawdź ruch e7e8 z promocją do hetmana
    Move movePromo;
    movePromo.fromRow = 1; // e7
    movePromo.fromCol = 4;
    movePromo.toRow = 0; // e8
    movePromo.toCol = 4;
    movePromo.movedPiece = 'P';
    movePromo.capturedPiece = 0; // brak figury do zbicia
    movePromo.promotion = 'Q'; // promocja do hetmana
    
    bool isValid = board.isMoveValid(movePromo);
    std::cout << "Ruch e7e8Q (promocja do hetmana): " << (isValid ? "DOZWOLONY" : "ZAKAZANY") << std::endl;
    
    // Sprawdź ruch e7e8 z promocją do skoczka (bez litery promocji)
    Move movePromoNoLetter;
    movePromoNoLetter.fromRow = 1; // e7
    movePromoNoLetter.fromCol = 4;
    movePromoNoLetter.toRow = 0; // e8
    movePromoNoLetter.toCol = 4;
    movePromoNoLetter.movedPiece = 'P';
    movePromoNoLetter.capturedPiece = 0; // brak figury do zbicia
    movePromoNoLetter.promotion = 0; // brak litery promocji
    
    bool isValidNoLetter = board.isMoveValid(movePromoNoLetter);
    std::cout << "Ruch e7e8 (bez litery promocji): " << (isValidNoLetter ? "DOZWOLONY" : "ZAKAZANY") << std::endl;
}

void testPromotionBlack() {
    std::cout << "\n=== Test: Promocja (czarne) ===" << std::endl;
    Board board;
    // Ustawienie pozycji: czarny pion na e2, puste pole na e1
    board.loadFEN("4k3/8/8/8/8/8/4p3/8 b - - 0 1");
    board.printBoard();
    
    // Sprawdź ruch e2e1 z promocją do hetmana
    Move movePromo;
    movePromo.fromRow = 6; // e2
    movePromo.fromCol = 4;
    movePromo.toRow = 7; // e1
    movePromo.toCol = 4;
    movePromo.movedPiece = 'p';
    movePromo.capturedPiece = 0; // brak figury do zbicia
    movePromo.promotion = 'q'; // promocja do hetmana (mała litera dla czarnych)
    
    bool isValid = board.isMoveValid(movePromo);
    std::cout << "Ruch e2e1q (promocja do hetmana): " << (isValid ? "DOZWOLONY" : "ZAKAZANY") << std::endl;
    
    // Sprawdź ruch e2e1 z promocją do skoczka (bez litery promocji)
    Move movePromoNoLetter;
    movePromoNoLetter.fromRow = 6; // e2
    movePromoNoLetter.fromCol = 4;
    movePromoNoLetter.toRow = 7; // e1
    movePromoNoLetter.toCol = 4;
    movePromoNoLetter.movedPiece = 'p';
    movePromoNoLetter.capturedPiece = 0; // brak figury do zbicia
    movePromoNoLetter.promotion = 0; // brak litery promocji
    
    bool isValidNoLetter = board.isMoveValid(movePromoNoLetter);
    std::cout << "Ruch e2e1 (bez litery promocji): " << (isValidNoLetter ? "DOZWOLONY" : "ZAKAZANY") << std::endl;
}

int main() {
    std::cout << "Testowanie reguł szachowych..." << std::endl;
    
    testCaptureOwnPieceWhite();
    testCaptureOwnPieceBlack();
    
    testEnPassantWhite();
    testEnPassantBlack();
    
    testCastlingWhite();
    testCastlingBlack();
    
    testPromotionWhite();
    testPromotionBlack();
    
    std::cout << "\nTesty zakonczone." << std::endl;
    return 0;
}