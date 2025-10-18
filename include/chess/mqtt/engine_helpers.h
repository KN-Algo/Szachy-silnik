#pragma once
#include <string>
#include <vector>
#include "chess/board/Board.h"
#include "chess/model/Move.h"

/**
 * @brief Odczytuje wartość zmiennej środowiskowej lub zwraca wartość domyślną.
 * 
 * @param key Nazwa zmiennej środowiskowej.
 * @param def Wartość domyślna zwracana, jeśli zmienna nie istnieje.
 * @return std::string Wartość zmiennej środowiskowej lub wartość domyślna.
 */
std::string env_or(const char* key, const std::string &def);

/**
 * @brief Sprawdza poprawność zapisu pola w notacji algebraicznej (np. "e2").
 * 
 * @param s Dwuznakowy zapis pola, np. "a1"–"h8".
 * @return true Jeśli zapis jest poprawny.
 * @return false Jeśli zapis jest niepoprawny.
 */
bool valid_sq(const std::string &s);

/**
 * @brief Konwertuje zapis algebraiczny pola (np. "e2") na współrzędne planszy.
 * 
 * @param s Pole w notacji algebraicznej.
 * @return std::pair<int,int> Para (wiersz, kolumna) odpowiadająca polu.
 */
std::pair<int, int> to_rc(const std::string &s);

/**
 * @brief Konwertuje współrzędne planszy na zapis algebraiczny (np. (6,4) → "e2").
 * 
 * @param row Numer wiersza (0–7, gdzie 0 = rząd 8).
 * @param col Numer kolumny (0–7, gdzie 0 = kolumna 'a').
 * @return std::string Pole w notacji algebraicznej.
 */
std::string to_sq(int row, int col);

/**
 * @brief Generuje zapis FEN (Forsyth–Edwards Notation) dla aktualnego stanu planszy.
 * 
 * @param b Obiekt klasy Board reprezentujący bieżący stan gry.
 * @return std::string Pełny zapis FEN bieżącej pozycji.
 */
std::string fen_from_board(const Board &b);

/**
 * @brief Zwraca listę możliwych ruchów (pól docelowych) dla figury z danego pola.
 * 
 * @param board Bieżący stan planszy.
 * @param fromSq Pole początkowe w notacji algebraicznej.
 * @return std::vector<std::string> Lista możliwych pól docelowych w notacji algebraicznej.
 */
std::vector<std::string> possibleFrom(Board &board, const std::string &fromSq);

/**
 * @brief Wypełnia strukturę Move na podstawie pól początkowego i końcowego.
 * 
 * @param board Plansza, na której wykonywany jest ruch.
 * @param from Pole początkowe (np. "e2").
 * @param to Pole docelowe (np. "e4").
 * @param out [out] Struktura Move, do której zostanie zapisany wynik.
 * @param promo Znak figury promocji (np. 'Q', 'N'); domyślnie 0.
 * @return true Jeśli ruch został poprawnie zbudowany.
 * @return false Jeśli dane wejściowe są niepoprawne.
 */
bool fill_move_from_to(Board &board, const std::string &from, const std::string &to, Move &out, char promo = 0);

/**
 * @brief Mapuje znak figury promocji (np. 'Q') na nazwę tekstową używaną przez backend.
 * 
 * @param p Znak figury promocji (np. 'Q', 'R', 'B', 'N').
 * @return const char* Nazwa figury w formacie tekstowym (np. "queen", "rook").
 */
const char* promo_name(char p);

/**
 * @brief Odwrotne mapowanie: konwertuje nazwę figury promocji na znak.
 * 
 * @param s Nazwa figury (np. "queen", "bishop").
 * @param out [out] Zmienna, do której zostanie zapisany znak figury (np. 'Q', 'B').
 * @return true Jeśli nazwa została rozpoznana.
 * @return false Jeśli nie znaleziono odpowiadającej figury.
 */
bool promo_char_from_name(std::string s, char &out);

/**
 * @brief Generuje pełny zapis ruchu w notacji SAN (Standard Algebraic Notation).
 * 
 * @param board_before Plansza przed wykonaniem ruchu.
 * @param m Struktura opisująca ruch.
 * @param isCastle Czy ruch jest roszadą.
 * @param isCastleKingSide Czy jest to roszada królewska (0-0).
 * @param isCapture Czy ruch jest biciem.
 * @param isEnPassant Czy ruch jest biciem w przelocie.
 * @param isCheck Czy po ruchu król przeciwnika jest szachowany.
 * @param isMate Czy ruch kończy partię matem.
 * @return std::string Zapis ruchu w formacie SAN (np. "Nxe5+", "0-0", "e8=Q#").
 */
std::string make_san_full(
    const Board &board_before,
    const Move &m,
    bool isCastle, bool isCastleKingSide,
    bool isCapture, bool isEnPassant,
    bool isCheck, bool isMate);
