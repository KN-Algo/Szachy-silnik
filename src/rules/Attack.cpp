// Attack.cpp
#include "chess/rules/Attack.h"

bool Board::isPathClear(int r1, int c1, int r2, int c2) const {
    int dr = (r2 > r1) ? 1 : (r2 < r1) ? -1 : 0;
    int dc = (c2 > c1) ? 1 : (c2 < c1) ? -1 : 0;
    int r = r1 + dr, c = c1 + dc;
    while (r != r2 || c != c2) {
        if (board[r][c] != 0) return false;
        r += dr; c += dc;
    }
    return true;
}

bool Board::isSquareAttacked(int row, int col, char byColor) const {
    int dir = (byColor == 'w') ? -1 : 1;
    char pawnChar = (byColor == 'w') ? 'P' : 'p';
    if (row + dir >= 0 && row + dir < 8) {
        if (col > 0 && board[row + dir][col - 1] == pawnChar) return true;
        if (col < 7 && board[row + dir][col + 1] == pawnChar) return true;
    }
    const int knightMoves[8][2] = {{-2,-1},{-2,1},{-1,-2},{-1,2},{1,-2},{1,2},{2,-1},{2,1}};
    char knightChar = (byColor == 'w') ? 'N' : 'n';
    for (auto &m : knightMoves) {
        int nr = row + m[0], nc = col + m[1];
        if (nr >= 0 && nr < 8 && nc >= 0 && nc < 8) {
            if (board[nr][nc] == knightChar) return true;
        }
    }
    char rookChar = (byColor == 'w') ? 'R' : 'r';
    char queenChar = (byColor == 'w') ? 'Q' : 'q';
    const int dirsRook[4][2] = {{1,0},{-1,0},{0,1},{0,-1}};
    for (auto &d : dirsRook) {
        int r = row + d[0], c = col + d[1];
        while (r >= 0 && r < 8 && c >= 0 && c < 8) {
            if (board[r][c] != 0) {
                if (board[r][c] == rookChar || board[r][c] == queenChar) return true;
                break;
            }
            r += d[0]; c += d[1];
        }
    }
    char bishopChar = (byColor == 'w') ? 'B' : 'b';
    const int dirsBishop[4][2] = {{1,1},{1,-1},{-1,1},{-1,-1}};
    for (auto &d : dirsBishop) {
        int r = row + d[0], c = col + d[1];
        while (r >= 0 && r < 8 && c >= 0 && c < 8) {
            if (board[r][c] != 0) {
                if (board[r][c] == bishopChar || board[r][c] == queenChar) return true;
                break;
            }
            r += d[0]; c += d[1];
        }
    }
    char kingChar = (byColor == 'w') ? 'K' : 'k';
    for (int dr = -1; dr <= 1; ++dr) {
        for (int dc = -1; dc <= 1; ++dc) {
            if (dr == 0 && dc == 0) continue;
            int nr = row + dr, nc = col + dc;
            if (nr >= 0 && nr < 8 && nc >= 0 && nc < 8) {
                if (board[nr][nc] == kingChar) return true;
            }
        }
    }
    return false;
}
