//
//  Created by jakub on 28.07.2025.
//
#include <iostream>
#include <string>
#include <sstream>
#include <cctype>
#include "board.h"
#include "move.h"
#include <algorithm> //Dodane przez MS

void Board::startBoard()
{
    std::string fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
    int row = 0, col = 0;
    size_t i = 0;

    while (fen[i] != ' ')
    {
        char c = fen[i];
        if (c == '/')
        {
            row++;
            col = 0;
        }
        else if (isdigit(c))
        {
            col += c - '0';
        }
        else
        {
            board[row][col++] = c;
        }
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
}

void Board::printBoard() const
{
    for (int row = 0; row < 8; ++row)
    {
        for (int col = 0; col < 8; ++col)
        {
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

static int fileToCol(char file) { return file - 'a'; }
static int rankToRow(char rank) { return '8' - rank; }

bool Board::isPathClear(int r1, int c1, int r2, int c2) const
{
    int dr = (r2 > r1) ? 1 : (r2 < r1) ? -1
                                       : 0;
    int dc = (c2 > c1) ? 1 : (c2 < c1) ? -1
                                       : 0;
    int r = r1 + dr, c = c1 + dc;
    while (r != r2 || c != c2)
    {
        if (board[r][c] != 0)
            return false;
        r += dr;
        c += dc;
    }
    return true;
}

bool Board::isSquareAttacked(int row, int col, char byColor) const
{
    int dir = (byColor == 'w') ? -1 : 1;
    char pawnChar = (byColor == 'w') ? 'P' : 'p';
    if (row + dir >= 0 && row + dir < 8)
    {
        if (col > 0 && board[row + dir][col - 1] == pawnChar)
            return true;
        if (col < 7 && board[row + dir][col + 1] == pawnChar)
            return true;
    }
    const int knightMoves[8][2] = {{-2, -1}, {-2, 1}, {-1, -2}, {-1, 2}, {1, -2}, {1, 2}, {2, -1}, {2, 1}};
    char knightChar = (byColor == 'w') ? 'N' : 'n';
    for (auto &m : knightMoves)
    {
        int nr = row + m[0], nc = col + m[1];
        if (nr >= 0 && nr < 8 && nc >= 0 && nc < 8)
        {
            if (board[nr][nc] == knightChar)
                return true;
        }
    }
    char rookChar = (byColor == 'w') ? 'R' : 'r';
    char queenChar = (byColor == 'w') ? 'Q' : 'q';
    const int dirsRook[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
    for (auto &d : dirsRook)
    {
        int r = row + d[0], c = col + d[1];
        while (r >= 0 && r < 8 && c >= 0 && c < 8)
        {
            if (board[r][c] != 0)
            {
                if (board[r][c] == rookChar || board[r][c] == queenChar)
                    return true;
                break;
            }
            r += d[0];
            c += d[1];
        }
    }
    char bishopChar = (byColor == 'w') ? 'B' : 'b';
    const int dirsBishop[4][2] = {{1, 1}, {1, -1}, {-1, 1}, {-1, -1}};
    for (auto &d : dirsBishop)
    {
        int r = row + d[0], c = col + d[1];
        while (r >= 0 && r < 8 && c >= 0 && c < 8)
        {
            if (board[r][c] != 0)
            {
                if (board[r][c] == bishopChar || board[r][c] == queenChar)
                    return true;
                break;
            }
            r += d[0];
            c += d[1];
        }
    }
    char kingChar = (byColor == 'w') ? 'K' : 'k';
    for (int dr = -1; dr <= 1; ++dr)
    {
        for (int dc = -1; dc <= 1; ++dc)
        {
            if (dr == 0 && dc == 0)
                continue;
            int nr = row + dr, nc = col + dc;
            if (nr >= 0 && nr < 8 && nc >= 0 && nc < 8)
            {
                if (board[nr][nc] == kingChar)
                    return true;
            }
        }
    }
    return false;
}

bool Board::isMoveValid(const Move &move)
{

    int r1 = move.fromRow;
    int c1 = move.fromCol;
    int r2 = move.toRow;
    int c2 = move.toCol;

    char piece = board[r1][c1];
    if (!piece)
        return false;

    bool whitePiece = std::isupper(piece);
    if ((activeColor == 'w' && !whitePiece) || (activeColor == 'b' && whitePiece))
        return false;

    if (board[r2][c2] && (std::isupper(board[r2][c2]) == whitePiece))
        return false;

    int dr = r2 - r1;
    int dc = c2 - c1;
    int adr = std::abs(dr), adc = std::abs(dc);

    switch (std::toupper(piece))
    {
    case 'P':
    {
        int dir = whitePiece ? -1 : 1;
        if (dc == 0)
        {
            if (dr == dir && board[r2][c2] == 0)
            {
            }
            else if ((whitePiece && r1 == 6 || !whitePiece && r1 == 1) && dr == 2 * dir && board[r1 + dir][c1] == 0 && board[r2][c2] == 0)
            {
            }
            else
                return false;
        }
        else if (adr == 1 && dr == dir && board[r2][c2] != 0)
        {
        }
        else
            return false;
        break;
    }
    case 'N':
        if (!((adr == 2 && adc == 1) || (adr == 1 && adc == 2)))
            return false;
        break;
    case 'B':
        if (adr != adc)
            return false;
        if (!isPathClear(r1, c1, r2, c2))
            return false;
        break;
    case 'R':
        if (!(adr == 0 || adc == 0))
            return false;
        if (!isPathClear(r1, c1, r2, c2))
            return false;
        break;
    case 'Q':
        if (!(adr == adc || adr == 0 || adc == 0))
            return false;
        if (!isPathClear(r1, c1, r2, c2))
            return false;
        break;
    case 'K':
        if (adr > 1 || adc > 1)
            return false;
        break;
    default:
        return false;
    }

    char captured = board[r2][c2];
    board[r2][c2] = piece;
    board[r1][c1] = 0;

    char kingChar = whitePiece ? 'K' : 'k';
    int kr = 0, kc = 0;
    for (int r = 0; r < 8; r++)
        for (int c = 0; c < 8; c++)
            if (board[r][c] == kingChar)
            {
                kr = r;
                kc = c;
            }

    bool inCheck = isSquareAttacked(kr, kc, whitePiece ? 'b' : 'w');

    board[r1][c1] = piece;
    board[r2][c2] = captured;

    return !inCheck;
}

void Board::makeMove(const Move &move)
{
    if (!isMoveValid(move))
    {
        std::cout << "Illegal move!\n";
        return;
    }
    board[move.toRow][move.toCol] = board[move.fromRow][move.fromCol];
    board[move.fromRow][move.fromCol] = 0;
    activeColor = (activeColor == 'w') ? 'b' : 'w';
}

std::string Board::checkCastlingReason(const Move &move) const // -> Dodane MS
{
    // 0) Czy to w ogóle wygląda jak roszada? (król o 2 w bok, bez zmiany rzędu)
    char piece = board[move.fromRow][move.fromCol];
    if (!piece || std::toupper(piece) != 'K')
        return "To nie jest roszada: z pola startowego nie rusza sie krol.";
    if ((move.toRow - move.fromRow) != 0 || std::abs(move.toCol - move.fromCol) != 2)
        return "To nie jest roszada: krol nie przesuwa sie o 2 pola w bok.";

    bool white = std::isupper(piece);
    int startRow = white ? 7 : 0;
    int startCol = 4;

    if (move.fromRow != startRow || move.fromCol != startCol)
        return "Krol nie stoi na polu startowym (e1/e8).";

    int dc = move.toCol - move.fromCol;
    bool shortSide = (dc == 2); // O-O
    bool longSide = (dc == -2); // O-O-O
    if (!shortSide && !longSide)
        return "To nie jest roszada: zly wektor ruchu.";

    // 1) Prawa do roszady z FEN (KQkq)
    auto hasRight = [&](char r)
    {
        return castling != "-" && castling.find(r) != std::string::npos;
    };
    if (white)
    {
        if (shortSide && !hasRight('K'))
            return "Brak prawa do krotkiej roszady (krol lub wieza ruszali sie wczesniej).";
        if (longSide && !hasRight('Q'))
            return "Brak prawa do dlugiej roszady (krol lub wieza ruszali sie wczesniej).";
    }
    else
    {
        if (shortSide && !hasRight('k'))
            return "Brak prawa do krótkiej roszady (krol lub wieza ruszali sie wczesniej).";
        if (longSide && !hasRight('q'))
            return "Brak prawa do dlugiej roszady (krol lub wieza ruszali sie wczesniej).";
    }

    // 2) Czy odpowiednia wieża stoi na miejscu
    int rookFromCol = shortSide ? 7 : 0;
    char rookChar = white ? 'R' : 'r';
    if (board[startRow][rookFromCol] != rookChar)
        return "Brak wlasciwej wiezy na rogu (a1/h1 lub a8/h8).";

    // 3) Czy między królem a wieżą jest pusto
    int c1 = std::min(startCol, rookFromCol) + 1;
    int c2 = std::max(startCol, rookFromCol) - 1;
    for (int c = c1; c <= c2; ++c)
    {
        if (board[startRow][c] != 0)
            return "Pomiedzy krolem a wieza stoja figury.";
    }

    // 4) Szach na królu teraz?
    char opp = white ? 'b' : 'w';
    if (isSquareAttacked(startRow, startCol, opp))
        return "Krol jest w szachu – roszada zakazana.";

    // 5) Pole pośrednie (f1/d1 lub f8/d8) nie może być atakowane
    int midCol = shortSide ? 5 : 3;
    if (isSquareAttacked(startRow, midCol, opp))
        return "Pole, przez które przechodzi krol, jest atakowane.";

    // 6) Pole docelowe króla (g1/c1 lub g8/c8) nie może być atakowane
    if (isSquareAttacked(startRow, move.toCol, opp))
        return "Pole docelowe krola jest atakowane.";

    return "Roszada jest mozliwa.";
}
