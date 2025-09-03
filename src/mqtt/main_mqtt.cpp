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

// >>> Twoje nagłówki silnika:
#include "chess/board/Board.h"
#include "chess/model/Move.h"
#include "chess/game/GameState.h"
#include "chess/utils/Notation.h"   // coordToAlg / algToCoord

using json = nlohmann::json;
using namespace notation;
using mqttwrap::Client;
using mqttwrap::MqttConfig;

// ─────────────────────────────────────────────────────────────────────────────
// Util: ENV z defaultem
static std::string env_or(const char* key, const std::string& def) {
    if (const char* v = std::getenv(key)) return std::string(v);
    return def;
}

// Walidator „e2” etc.
static inline bool valid_sq(const std::string& s) {
    int r, c; return algToCoord(s, r, c);
}

// Konwersje pól (ale mamy już w Notation.h – zostawiamy wygodne aliasy)
static inline std::pair<int,int> to_rc(const std::string& s) {
    int r, c; algToCoord(s, r, c); return {r, c};
}
static inline std::string to_sq(int row, int col) {
    return coordToAlg(row, col);
}

// Minimalny FEN z Board (plansza + podstawowe pola stanu z Twojej klasy)
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

// Zwróć listę TO-squares z jednego FROM
static std::vector<std::string> possibleFrom(Board& board, const std::string& fromSq) {
    std::vector<std::string> moves;
    if (!valid_sq(fromSq)) return moves;

    auto legals = board.getLegalMoves();
    int r0, c0; algToCoord(fromSq, r0, c0);
    moves.reserve(8);
    for (const auto& m : legals) {
        if (m.fromRow == r0 && m.fromCol == c0) {
            moves.push_back(to_sq(m.toRow, m.toCol));
        }
    }
    std::sort(moves.begin(), moves.end());
    moves.erase(std::unique(moves.begin(), moves.end()), moves.end());
    return moves;
}

// Składanie Move pod Twoją strukturę
static bool fill_move_from_to(Board& board, const std::string& from, const std::string& to, Move& out, char promo = 0) {
    if (!valid_sq(from) || !valid_sq(to)) return false;
    int r1, c1, r2, c2;
    algToCoord(from, r1, c1);
    algToCoord(to,   r2, c2);

    out.fromRow = r1; out.fromCol = c1;
    out.toRow   = r2; out.toCol   = c2;
    out.movedPiece    = board.board[r1][c1];
    out.capturedPiece = board.board[r2][c2];
    out.promotion     = promo;
    return true;
}

// --- helpers AI ---
static inline std::string to_sq(int r, int c); // masz już wyżej
static std::string fen_from_board(const Board& b); // masz już wyżej

struct AiMoveResult {
    bool ok = false;
    Move move{};
    std::string from, to;
    std::string fen_after;
    std::string next_player;       // "white" / "black"
    std::string special_move;      // opcjonalnie "promotion"
    std::string promotion_piece;   // opcjonalnie "queen" / "rook" / ...
};

static AiMoveResult compute_ai_move(Board& board) {
    AiMoveResult res;
    // pobierz wszystkie legalne ruchy aktualnej strony
    auto legals = board.getLegalMoves();
    if (legals.empty()) return res;

    // Prosty wybór: pierwszy legalny (możesz zamienić na losowy)
    const auto m = legals.front();
    res.move = m;
    res.from = to_sq(m.fromRow, m.fromCol);
    res.to   = to_sq(m.toRow,   m.toCol);

    // promocja? Jeśli pion dochodzi do końcowego rzędu – ustaw promocję na hetmana
    char promo = 0;
    const char moved = board.board[m.fromRow][m.fromCol];
    const bool white = std::isupper(static_cast<unsigned char>(moved));
    const bool is_pawn = (std::toupper(static_cast<unsigned char>(moved)) == 'P');
    if (is_pawn) {
        if ((white && m.toRow == 0) || (!white && m.toRow == 7)) {
            promo = white ? 'Q' : 'q';
            res.special_move = "promotion";
            res.promotion_piece = "queen";
        }
    }

    // wykonaj ruch na wewnętrznej planszy
    Board tmp = board;
    Move exec = m;
    exec.promotion = promo;
    if (!tmp.isMoveValid(exec)) return res; // asekuracyjnie
    tmp.makeMove(exec);

    res.fen_after = fen_from_board(tmp);
    res.next_player = (tmp.activeColor == 'w') ? "white" : "black";
    res.ok = true;

    // jeśli OK, przepisz nowy stan do głównej planszy
    board = std::move(tmp);
    return res;
}

static void publish_ai_move(mqttwrap::Client& client, const AiMoveResult& ai) {
    using namespace topics;
    client.publish(MOVE_AI,
        make_ai_move_payload(ai.from, ai.to, ai.fen_after, ai.next_player,
                             ai.special_move, ai.promotion_piece));
}


int main() {
    // ── Inicjalizacja planszy
    Board board;
    board.startBoard();  // startowe ustawienie

    // ── Konfiguracja MQTT
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

    // README backendu: statusy publikujemy na 'status/engine'
    auto publish_engine_status = [&](const std::string& status, const std::string& msg = "") {
        json j = { {"status", status} };
        if (!msg.empty()) j["message"] = msg;
        client.publish("status/engine", j);
    };

    client.set_message_handler([&](const std::string& topic, const std::string& payload) {
        try {
            // =========================================================================
            // POSSIBLE MOVES
            // =========================================================================
            if (topic == topics::POSSIBLE_MOVES_REQ) {
                publish_engine_status("thinking", "generating moves");

                auto req = PossibleMovesReq::parse(payload); // { position, fen }
                if (!req.fen.empty()) {
                    if (!board.loadFEN(req.fen)) {
                        publish_engine_status("error", "bad FEN in possible_moves/request");
                        return;
                    }
                }

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
                const auto fen_before = fen_from_board(board);

                // promocja: akceptuj oba style: "promote": "q" oraz
                // {"special_move":"promotion","promotion_piece":"q"}
                char promo = 0;
                try {
                    auto j = json::parse(payload);
                    if (j.contains("promote") && j["promote"].is_string()) {
                        std::string ps = j["promote"].get<std::string>();
                        if (!ps.empty()) promo = std::toupper(ps[0]);
                    }
                    if (j.contains("special_move") && j["special_move"].is_string()
                        && j["special_move"] == "promotion"
                        && j.contains("promotion_piece") && j["promotion_piece"].is_string()) {
                        std::string pp = j["promotion_piece"].get<std::string>();
                        if (!pp.empty()) promo = std::toupper(pp[0]);
                    }
                } catch (...) {}

                Move m{};

                // jeśli podano figurę promocji – ustaw poprawną literę (biała = wielka, czarna = mała)
                if (promo) {
                    const auto [r, c] = to_rc(req.from);
                    const char moved = board.board[r][c];
                    const bool whiteMove = std::isupper(static_cast<unsigned char>(moved));

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


                // Walidacja PRZED wykonaniem
                const bool legal = board.isMoveValid(m);
                if (!legal) {
                    client.publish(
                        topics::MOVE_REJECTED,
                        make_move_rejected(req.from, req.to, fen_before, req.physical, "Illegal move")
                    );
                    publish_engine_status("ready", "validation done");
                    return;
                }

                // Wykonaj ruch i publikuj confirmed z FEN po ruchu + next_player
                board.makeMove(m);
                const auto fen_after = fen_from_board(board);
                const std::string next_player = (board.activeColor == 'w') ? "white" : "black";

                client.publish(
                    topics::MOVE_CONFIRMED,
                    make_move_confirmed(req.from, req.to, fen_after, req.physical, next_player)
                );

                publish_engine_status("ready", "validation done");
                return;
            }

            if (topic == topics::AI_THINK_REQ) {
            // jeśli podano FEN – załaduj
            try {
                auto j = json::parse(payload);
                if (j.contains("fen") && j["fen"].is_string()) {
                    const auto fen = j["fen"].get<std::string>();
                    board.loadFEN(fen); // ignoruj błąd – jeśli zły FEN, to compute_ai_move i tak zwróci res.ok=false
                }
            } catch (...) {}

            publish_engine_status("thinking", "ai thinking");
            auto ai = compute_ai_move(board);
            if (!ai.ok) {
                publish_engine_status("error", "no legal moves");
                return;
            }
            publish_ai_move(client, ai);
            publish_engine_status("ready", "ai move published");
            return;
        }


            // =========================================================================
            // RESTART (wymagane: control/restart/external)
            // =========================================================================
            if (topic == "control/restart/external") {
                try {
                    auto j = json::parse(payload);
                    if (j.contains("fen") && j["fen"].is_string()) {
                        const auto fen = j["fen"].get<std::string>();
                        if (!board.loadFEN(fen)) {
                            // błąd FEN
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


            // Nieznany temat – status ostrzegawczy
            publish_engine_status("warning", std::string("unhandled topic: ") + topic);
        }
        catch (const std::exception& e) {
            std::cerr << "[Engine] Failed to handle message on " << topic << ": " << e.what() << "\n";
            try {
                publish_engine_status("error", e.what());
            } catch (...) {}
        }
    });

    // Subskrypcje wymagane przez backend
    client.subscribe(topics::POSSIBLE_MOVES_REQ, 1); // engine/possible_moves/request
    client.subscribe(topics::MOVE_ENGINE_REQ, 1);    // move/engine
    client.subscribe("control/restart/external", 1); // reset stanu (fen opcjonalny)
    client.subscribe(topics::AI_THINK_REQ, 1); // engine/ai/think


    std::cout << "Engine MQTT bridge up. Listening… (Ctrl+C to exit)\n";
    client.loop_forever();
    return 0;
}

