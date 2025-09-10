// main_mqtt.cpp
#include <iostream>
#include <cstdlib>
#include <vector>
#include <string>
#include <cctype>
#include <algorithm>

#include <nlohmann/json.hpp>
#include "chess/mqtt/mqtt_client.h"
#include "chess/mqtt/topics.h"
#include "chess/mqtt/payloads.h"

#include "chess/board/Board.h"
#include "chess/model/Move.h"
#include "chess/game/GameState.h"
#include "chess/utils/Notation.h"   // coordToAlg / algToCoord
#include "chess/ai/ChessAI.h"

using json = nlohmann::json;
using namespace notation;
using mqttwrap::Client;
using mqttwrap::MqttConfig;

// ─────────────────────────────────────────────────────────────────────────────
// ENV z defaultem
static std::string env_or(const char* key, const std::string& def) {
    if (const char* v = std::getenv(key)) return std::string(v);
    return def;
}

// Walidator „e2”
static inline bool valid_sq(const std::string& s) {
    int r, c; return algToCoord(s, r, c);
}

// Konwersje pól
static inline std::pair<int,int> to_rc(const std::string& s) {
    int r, c; algToCoord(s, r, c); return {r, c};
}
static inline std::string to_sq(int row, int col) {
    return coordToAlg(row, col);
}

// Minimalny FEN z aktualnego Board (bez żadnych cudów)
static std::string fen_from_board(const Board& b) {
    std::string out;
    for (int row = 0; row < 8; ++row) {
        int empty = 0;
        for (int col = 0; col < 8; ++col) {
            char p = b.board[row][col];
            if (p == 0) { empty++; }
            else {
                if (empty) { out += std::to_string(empty); empty = 0; }
                out += p;
            }
        }
        if (empty) out += std::to_string(empty);
        if (row != 7) out += '/';
    }
    out += ' ';
    out += (b.activeColor == 'w' ? 'w' : 'b');
    out += ' ';
    out += (b.castling.empty() ? "-" : b.castling);
    out += ' ';
    out += (b.enPassant.empty() ? "-" : b.enPassant);
    out += ' ';
    out += std::to_string(b.halfmoveClock);
    out += ' ';
    out += std::to_string(b.fullmoveNumber);
    return out;
}

// Lista TO z jednego FROM (z legalnych ruchów silnika)
static std::vector<std::string> possibleFrom(Board& board, const std::string& fromSq) {
    std::vector<std::string> moves;
    if (!valid_sq(fromSq)) return moves;

    int r0, c0;
    if (!algToCoord(fromSq, r0, c0)) return moves;

    // Jeśli na polu nie ma naszej bierki – nie ma ruchów
    if (board.board[r0][c0] == 0) return moves;

    // Weź legalne ruchy z silnika (uwzględnia: szachy, roszady, EP, promocje)
    const auto legals = board.getLegalMoves();

    for (const auto& m : legals) {
        if (m.fromRow == r0 && m.fromCol == c0) {
            moves.push_back(to_sq(m.toRow, m.toCol));
        }
    }

    std::sort(moves.begin(), moves.end());
    moves.erase(std::unique(moves.begin(), moves.end()), moves.end());
    return moves;
}


// Wypełnij Move (pola i ew. promocję)
static bool fill_move_from_to(Board& board, const std::string& from, const std::string& to, Move& out, char promo = 0) {
    if (!valid_sq(from) || !valid_sq(to)) return false;
    int r1, c1, r2, c2;
    algToCoord(from, r1, c1);
    algToCoord(to,   r2, c2);

    out.fromRow = r1; out.fromCol = c1;
    out.toRow   = r2; out.toCol   = c2;
    out.movedPiece    = board.board[r1][c1];
    out.capturedPiece = board.board[r2][c2]; // przy EP cel jest pusty – to normalne
    out.promotion     = promo;
    return true;
}

// Czy są inne figury tego samego typu (i koloru) mogące legalnie dojść na to samo pole?
static void san_disambiguation_before_move(
    const Board& board_before,
    const Move& m,                 // wykonywany ruch
    bool& needFile, bool& needRank // wynik: dopisać plik/rząd w SAN?
){
    needFile = false; needRank = false;

    const char moved = board_before.board[m.fromRow][m.fromCol];
    const bool isPawn = std::toupper(static_cast<unsigned char>(moved))=='P';
    if (isPawn) return; // piony w SAN nie potrzebują dysambiguacji (plik przy biciu już rozróżnia)

    // Zbierz wszystkie legalne ruchy z pozycji "przed ruchem"
    auto legals = const_cast<Board&>(board_before).getLegalMoves();

    // Szukamy innych ruchów na to samo pole (ta sama figura i kolor, inny "from")
    std::vector<std::pair<int,int>> colliders;
    for (const auto& x : legals) {
        if (x.toRow==m.toRow && x.toCol==m.toCol) {
            char p = board_before.board[x.fromRow][x.fromCol];
            if (p != 0 &&
                std::toupper(static_cast<unsigned char>(p)) == std::toupper(static_cast<unsigned char>(moved)) &&
                // ten sam kolor:
                (std::isupper(static_cast<unsigned char>(p)) == std::isupper(static_cast<unsigned char>(moved))) &&
                // inny start:
                !(x.fromRow==m.fromRow && x.fromCol==m.fromCol))
            {
                colliders.emplace_back(x.fromRow, x.fromCol);
            }
        }
    }
    if (colliders.empty()) return; // brak konfliktu

    // Jeśli któryś ma inny plik (kolumnę), zwykle wystarczy podać plik.
    bool sameFileAll = true;
    bool sameRankAll = true;

    for (auto [r,c] : colliders) {
        if (c != m.fromCol) sameFileAll = false; // są różne pliki — plik rozróżni
        if (r != m.fromRow) sameRankAll = false; // są różne rzędy — rząd rozróżni
    }

    // Reguły SAN:
    // 1) jeśli plik rozróżnia jednoznacznie — podaj plik,
    // 2) else jeśli rząd rozróżnia — podaj rząd,
    // 3) else (nie rozróżnia ani plik, ani rząd) — podaj oba.
    // (Uwaga: sameFileAll==true oznacza: wszystkie kolidują W TYM SAMYM pliku -> plik NIE rozróżnia)
    bool fileDistinguishes = !sameFileAll;
    bool rankDistinguishes = !sameRankAll;

    if (fileDistinguishes) { needFile = true; needRank = false; }
    else if (rankDistinguishes) { needFile = false; needRank = true; }
    else { needFile = true; needRank = true; }
}
static std::string san_letter_for_piece(char piece) {
    switch (std::toupper(static_cast<unsigned char>(piece))) {
        case 'N': return "N";
        case 'B': return "B";
        case 'R': return "R";
        case 'Q': return "Q";
        case 'K': return "K";
        default:  return "";
    }
}
static std::string make_san_full(
    const Board& board_before, // pozycja PRZED ruchem
    const Move& m,             // ruch (z wypełnionym movedPiece/capturedPiece/promotion)
    bool isCastle, bool isCastleKingSide,
    bool isCapture, bool isEnPassant,
    bool isCheck, bool isMate
){
    // Roszada
    if (isCastle) return isCastleKingSide ? "0-0" : "0-0-0";

    const char moved = m.movedPiece;
    const bool isPawn = std::toupper(static_cast<unsigned char>(moved))=='P';

    std::string san;

    // 1) litera figury (pion bez litery)
    if (!isPawn) san += san_letter_for_piece(moved);

    // 2) dysambiguacja (tylko dla figur innych niż pion)
    bool needFile=false, needRank=false;
    if (!isPawn) san_disambiguation_before_move(board_before, m, needFile, needRank);
    if (!isPawn) {
        if (needFile) san += static_cast<char>('a' + m.fromCol);
        if (needRank) san += static_cast<char>('1' + (7 - m.fromRow));
    }

    // 3) bicie (dla piona przy biciu SAN zaczyna się od pliku piona)
    if (isPawn && isCapture) san += static_cast<char>('a' + m.fromCol);
    if (isCapture) san += 'x';

    // 4) pole docelowe
    san += coordToAlg(m.toRow, m.toCol);

    // 5) promocja
    if (m.promotion && std::toupper(static_cast<unsigned char>(m.promotion)) != '?') {
        san += '=';
        san += static_cast<char>(std::toupper(static_cast<unsigned char>(m.promotion))); // =Q/R/B/N
    }

    // 6) sufiks + / #
    if (isMate)      san += '#';
    else if (isCheck) san += '+';

    return san;
}

// Mapowanie char promocji -> nazwa dla backendu
static const char* promo_name(char p) {
    switch (std::toupper(static_cast<unsigned char>(p))) {
        case 'Q': return "queen";
        case 'R': return "rook";
        case 'B': return "bishop";
        case 'N': return "knight";
        default:  return "";
    }
}

// ─────────────────────────────────────────────────────────────────────────────

int main() {
    // Inicjalizacja planszy
    Board board;
    board.startBoard();

    // Konfiguracja MQTT
    MqttConfig cfg;
    cfg.host      = env_or("MQTT_HOST", "localhost");
    cfg.port      = std::stoi(env_or("MQTT_PORT", "1883"));
    cfg.client_id = env_or("MQTT_CLIENT_ID", "chess-engine");
    cfg.qos       = 1;

    Client client(cfg);
    if (!client.connect()) {
        std::cerr << "[MQTT] Connection failed\n";
        return 1;
    }

    auto publish_engine_status = [&](const std::string& status, const std::string& msg = "") {
        json j = { {"status", status} };
        if (!msg.empty()) j["message"] = msg;
        client.publish(topics::STATUS_ENGINE, j);
    };

    client.set_message_handler([&](const std::string& topic, const std::string& payload) {
        try {
            // =========================================================================
            // POSSIBLE MOVES
            // =========================================================================
            if (topic == topics::POSSIBLE_MOVES_REQ) {
                publish_engine_status("thinking", "generating moves");

                auto req = PossibleMovesReq::parse(payload); // { position, fen }

                // Opcjonalna synchronizacja pozycji
                if (!req.fen.empty()) {
                    if (!board.loadFEN(req.fen)) {
                        publish_engine_status("error", "bad FEN in possible_moves/request");
                        return;
                    }
                }

                // Zwróć ruchy wyłącznie z generatora silnika
                auto moves = possibleFrom(board, req.position);

                client.publish(
                    topics::POSSIBLE_MOVES_RES,
                    make_possible_moves_response(req.position, moves)
                );

                publish_engine_status("ready", "moves ready");
                return;
            }

            // =========================================================================
            // MOVE ENGINE (walidacja i wykonanie ruchu)
            // =========================================================================
            if (topic == topics::MOVE_ENGINE_REQ) {
                publish_engine_status("analyzing", "validating move");

                auto req = MoveEngineReq::parse(payload); // { from, to, current_fen, type, physical }

                if (!req.current_fen.empty()) {
                    if (!board.loadFEN(req.current_fen)) {
                        publish_engine_status("error", "bad current_fen");
                        return;
                    }
                }
                const std::string fen_before = fen_from_board(board);

                // ewentualna figura promocji z requestu (tylko z MoveEngineReq)
                char promo = 0;
                if (req.type == "promotion" && !req.promotion_piece.empty()) {
                    // "queen"/"rook"/"bishop"/"knight" -> 'Q'/'R'/'B'/'N'
                    char p = std::toupper(static_cast<unsigned char>(req.promotion_piece[0]));
                    // nadaj wielkość litery wg koloru wykonującego ruch
                    const auto [r_from, c_from] = to_rc(req.from);
                    const char movedHere = board.board[r_from][c_from];
                    const bool whiteMove = std::isupper(static_cast<unsigned char>(movedHere));
                    promo = whiteMove ? p : static_cast<char>(std::tolower(static_cast<unsigned char>(p)));
                }


                Move m{};
                // jeżeli podano promocję – ustaw literę w odpowiedniej wielkości
                if (promo) {
                    const auto [r, c] = to_rc(req.from);
                    const char movedHere = board.board[r][c];
                    const bool whiteMove = std::isupper(static_cast<unsigned char>(movedHere));
                    promo = whiteMove
                        ? static_cast<char>(std::toupper(static_cast<unsigned char>(promo)))
                        : static_cast<char>(std::tolower(static_cast<unsigned char>(promo)));
                }

                if (!fill_move_from_to(board, req.from, req.to, m, promo)) {
                    client.publish(
                        topics::MOVE_REJECTED,
                        make_move_rejected(req.from, req.to, fen_before, req.physical, "Bad algebraic square")
                    );
                    publish_engine_status("ready", "validation done");
                    return;
                }

                // Walidacja silnikiem (zero równoległej logiki)
                const bool legal = board.isMoveValid(m);
                if (!legal) {
                    client.publish(
                        topics::MOVE_REJECTED,
                        make_move_rejected(req.from, req.to, fen_before, req.physical, "Illegal move")
                    );
                    publish_engine_status("ready", "validation done");
                    return;
                }

                // ── ZBIÓR DANYCH DO OZNAKOWANIA RUCHU (przed wykonaniem)
                const std::string prevEP = board.enPassant; // np. "e6" / "-"
                const bool whiteMoved = std::isupper(static_cast<unsigned char>(m.movedPiece));
                const bool isPawn     = (std::toupper(static_cast<unsigned char>(m.movedPiece)) == 'P');
                const bool targetEmptyBefore = (m.capturedPiece == 0);

                // czy to roszada (wg wewnętrznego kształtu ruchu króla: delta kolumny == 2)
                const bool isCastle = (std::toupper(static_cast<unsigned char>(m.movedPiece)) == 'K'
                                       && std::abs(m.toCol - m.fromCol) == 2
                                       && m.toRow == m.fromRow);
                const bool isCastleKingSide = isCastle && (m.toCol > m.fromCol);

                // EP gdy: pion, ruch po skosie, cel pusty, a TO == poprzednie enPassant
                const std::string toAlg = to_sq(m.toRow, m.toCol);
                const bool isEnPassant = (isPawn && m.fromCol != m.toCol && targetEmptyBefore
                                          && !prevEP.empty() && prevEP != "-" && toAlg == prevEP);

                // czy było bicie (z uwzględnieniem EP)
                const bool isCapture = (!targetEmptyBefore) || isEnPassant;
                Board board_before = board;
                // ── Wykonanie
                board.makeMove(m);

                // ── Po ruchu: stan, FEN, „kto następny”, check/checkmate
                const std::string fen_after = fen_from_board(board);
                const std::string next_player = (board.activeColor == 'w') ? "white" : "black";
                const bool gives_check = board.isInCheck(); // strona na posunięciu jest w szachu? (czyli ruch dał szacha) :contentReference[oaicite:2]{index=2}
                const GameState gs = board.getGameState();  // CHECKMATE/STALEMATE/PLAYING itd. :contentReference[oaicite:3]{index=3}

                // ── Notation/Special/Additional
                json j = {
                    {"from", req.from},
                    {"to",   req.to},
                    {"fen",  fen_after},
                    {"next_player", next_player},
                    {"physical", req.physical}
                };

                // special_move + dodatkowy ruch wieży przy roszadzie
                if (isCastle) {
                    j["special_move"] = isCastleKingSide ? "castling_kingside" : "castling_queenside";
                    // additional_moves: ruch wieży wg rzędu króla
                    const int row = m.fromRow;
                    if (isCastleKingSide) {
                        j["additional_moves"] = json::array({ json{{"from", to_sq(row,7)}, {"to", to_sq(row,5)}, {"piece","rook"}} });
                    } else {
                        j["additional_moves"] = json::array({ json{{"from", to_sq(row,0)}, {"to", to_sq(row,3)}, {"piece","rook"}} });
                    }
                    j["notation"] = isCastleKingSide ? "0-0" : "0-0-0";
                } else if (isEnPassant) {
                    j["special_move"] = "en_passant";
                }

                // promocja
                if (m.promotion && std::toupper(static_cast<unsigned char>(m.promotion)) != '?') {
                    j["special_move"]    = "promotion";
                    j["promotion_piece"] = promo_name(m.promotion);
                }

                // SAN (lekki) + sufiksy
                const bool isMate = (gs == GameState::CHECKMATE);
                if (!j.contains("notation")) {
                    j["notation"] = make_san_full(board_before, m,
                                  isCastle, isCastleKingSide,
                                  isCapture, isEnPassant,
                                  gives_check, isMate);
                }

                // sufiksy check/mate + status gry
                j["gives_check"] = gives_check;
                if (gs == GameState::CHECKMATE) {
                    j["game_status"] = "checkmate";
                    j["winner"] = whiteMoved ? "white" : "black";
                } else if (gs == GameState::STALEMATE) {
                    j["game_status"] = "stalemate";
                } else {
                    j["game_status"] = "playing";
                }

                client.publish(topics::MOVE_CONFIRMED, j);
                publish_engine_status("ready", "validation done");
                return;
            }

            // =========================================================================
            // MOVE AI (szukanie + publikacja ruchu AI)
            // =========================================================================
            if (topic == topics::AI_THINK_REQ) {
                publish_engine_status("thinking", "ai thinking");

                try {
                    auto j = json::parse(payload);
                    if (j.contains("fen") && j["fen"].is_string()) {
                        const auto fen = j["fen"].get<std::string>();
                        (void)board.loadFEN(fen); // jeżeli FEN zły, AI po prostu nic nie znajdzie
                    }
                } catch (...) {}

                // zrzut tablicy dla AI
                char arr[8][8];
                for (int r=0;r<8;++r) for (int c=0;c<8;++c) arr[r][c]=board.board[r][c];
                char side = board.activeColor;              // 'w'/'b'
                std::string cast = board.castling;          // "KQkq"/"-"
                std::string ep   = board.enPassant;         // "e3"/"-"

                ChessAI ai;
                auto res = ai.findBestMove(arr, side, cast, ep, /*depth*/5, /*timeMs*/5000);

                // brak ruchu?
                if (res.bestMove.fromRow == 0 && res.bestMove.fromCol == 0 &&
                    res.bestMove.toRow == 0 && res.bestMove.toCol == 0 && res.bestMove.promotion == '?') {
                    publish_engine_status("error", "no legal moves");
                    return;
                }

                Move exec = res.bestMove;

                if (!board.isMoveValid(exec)) {
                    publish_engine_status("error", "ai produced illegal move");
                    return;
                }

                // dane „przed” (do klasyfikacji EP/roszady/itd.)
                const std::string prevEP = board.enPassant;
                const bool whiteMoved = std::isupper(static_cast<unsigned char>(exec.movedPiece));
                const bool isPawn     = (std::toupper(static_cast<unsigned char>(exec.movedPiece)) == 'P');
                const bool targetEmptyBefore = (exec.capturedPiece == 0);

                const bool isCastle = (std::toupper(static_cast<unsigned char>(exec.movedPiece)) == 'K'
                                       && std::abs(exec.toCol - exec.fromCol) == 2
                                       && exec.toRow == exec.fromRow);
                const bool isCastleKingSide = isCastle && (exec.toCol > exec.fromCol);
                const std::string toAlg = to_sq(exec.toRow, exec.toCol);
                const bool isEnPassant = (isPawn && exec.fromCol != exec.toCol && targetEmptyBefore
                                          && !prevEP.empty() && prevEP != "-" && toAlg == prevEP);
                const bool isCapture = (!targetEmptyBefore) || isEnPassant;
                Board board_before = board;
                // wykonaj
                board.makeMove(exec);

                const std::string fen_after = fen_from_board(board);
                const std::string from = to_sq(exec.fromRow, exec.fromCol);
                const std::string to   = to_sq(exec.toRow,   exec.toCol);
                const std::string next_player = (board.activeColor == 'w') ? "white" : "black";
                const bool gives_check = board.isInCheck();
                const GameState gs = board.getGameState();

                json j = {
                    {"from", from},
                    {"to",   to},
                    {"fen",  fen_after},
                    {"next_player", next_player}
                };

                if (isCastle) {
                    j["special_move"] = isCastleKingSide ? "castling_kingside" : "castling_queenside";
                    const int row = exec.fromRow;
                    if (isCastleKingSide) {
                        j["additional_moves"] = json::array({ json{{"from", to_sq(row,7)}, {"to", to_sq(row,5)}, {"piece","rook"}} });
                    } else {
                        j["additional_moves"] = json::array({ json{{"from", to_sq(row,0)}, {"to", to_sq(row,3)}, {"piece","rook"}} });
                    }
                    j["notation"] = isCastleKingSide ? "0-0" : "0-0-0";
                } else if (isEnPassant) {
                    j["special_move"] = "en_passant";
                }

                if (exec.promotion && std::toupper(static_cast<unsigned char>(exec.promotion)) != '?') {
                    j["special_move"]    = "promotion";
                    j["promotion_piece"] = promo_name(exec.promotion);
                }

                const bool isMate = (gs == GameState::CHECKMATE);
                if (!j.contains("notation")) {
                    j["notation"] = make_san_full(board_before, exec,
                                  isCastle, isCastleKingSide,
                                  isCapture, isEnPassant,
                                  gives_check, isMate);
                }
                j["gives_check"] = gives_check;
                if (gs == GameState::CHECKMATE) {
                    j["game_status"] = "checkmate";
                    j["winner"] = whiteMoved ? "white" : "black";
                } else if (gs == GameState::STALEMATE) {
                    j["game_status"] = "stalemate";
                } else {
                    j["game_status"] = "playing";
                }

                client.publish(topics::MOVE_AI, j);
                publish_engine_status("ready", "ai move published");
                return;
            }

            // =========================================================================
            // RESTART (start / fen)
            // =========================================================================
            if (topic == "control/restart/external") {
                try {
                    auto j = json::parse(payload);
                    if (j.contains("fen") && j["fen"].is_string()) {
                        const auto fen = j["fen"].get<std::string>();
                        if (!board.loadFEN(fen)) {
                            client.publish(topics::STATUS_ENGINE, json{{"status","error"},{"message","bad FEN"}});
                            return;
                        }
                    } else {
                        board.startBoard();
                    }
                    client.publish(topics::STATUS_ENGINE, json{{"status","ready"},{"message","board restarted"}});
                } catch (...) {
                    board.startBoard();
                    client.publish(topics::STATUS_ENGINE, json{{"status","ready"},{"message","board restarted"}});
                }
                return;
            }

            // Nieznany temat
            publish_engine_status("warning", std::string("unhandled topic: ") + topic);
        }
        catch (const std::exception& e) {
            std::cerr << "[Engine] Failed to handle message on " << topic << ": " << e.what() << "\n";
            try { publish_engine_status("error", e.what()); } catch (...) {}
        }
    });

    // Subskrypcje wymagane przez backend
    client.subscribe(topics::POSSIBLE_MOVES_REQ, 1);
    client.subscribe(topics::MOVE_ENGINE_REQ, 1);
    client.subscribe("control/restart/external", 1);
    client.subscribe(topics::AI_THINK_REQ, 1);

    std::cout << "Engine MQTT bridge up. Listening… (Ctrl+C to exit)\n";
    client.loop_forever();
    return 0;
}
