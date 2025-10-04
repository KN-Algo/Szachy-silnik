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
#include "chess/mqtt/logging.h"

#include "chess/board/Board.h"
#include "chess/model/Move.h"
#include "chess/game/GameState.h"
#include "chess/utils/Notation.h" // coordToAlg / algToCoord
#include "chess/ai/ChessAI.h"


using json = nlohmann::json;
using namespace notation;
using mqttwrap::Client;
using mqttwrap::MqttConfig;

const int DEFAULT_AI_DEPTH   = 5;     // głębokość przeszukiwania
const int DEFAULT_AI_TIME_MS = 5000;  // limit czasu w ms

// ─────────────────────────────────────────────────────────────────────────────
// ENV z defaultem
static std::string env_or(const char *key, const std::string &def)
{
    if (const char *v = std::getenv(key))
        return std::string(v);
    return def;
}

// Walidator „e2”
static inline bool valid_sq(const std::string &s)
{
    int r, c;
    return algToCoord(s, r, c);
}

// Konwersje pól
static inline std::pair<int, int> to_rc(const std::string &s)
{
    int r, c;
    algToCoord(s, r, c);
    return {r, c};
}
static inline std::string to_sq(int row, int col)
{
    return coordToAlg(row, col);
}

// Minimalny FEN z aktualnego Board (bez żadnych cudów)
static std::string fen_from_board(const Board &b)
{
    std::string out;
    for (int row = 0; row < 8; ++row)
    {
        int empty = 0;
        for (int col = 0; col < 8; ++col)
        {
            char p = b.board[row][col];
            if (p == 0)
            {
                empty++;
            }
            else
            {
                if (empty)
                {
                    out += std::to_string(empty);
                    empty = 0;
                }
                out += p;
            }
        }
        if (empty)
            out += std::to_string(empty);
        if (row != 7)
            out += '/';
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
static std::vector<std::string> possibleFrom(Board &board, const std::string &fromSq)
{
    std::vector<std::string> moves;
    if (!valid_sq(fromSq))
        return moves;

    int r0, c0;
    if (!algToCoord(fromSq, r0, c0))
        return moves;

    // Jeśli na polu nie ma naszej bierki – nie ma ruchów
    if (board.board[r0][c0] == 0)
        return moves;

    // Weź legalne ruchy z silnika (uwzględnia: szachy, roszady, EP, promocje)
    const auto legals = board.getLegalMoves();
    const char mover = board.board[r0][c0];
    const bool white = std::isupper(static_cast<unsigned char>(mover));

    for (const auto &m : legals)
    {
        if (m.fromRow == r0 && m.fromCol == c0)
        {
            char target = board.board[m.toRow][m.toCol];
            if (target && ((std::isupper(static_cast<unsigned char>(target)) != 0) == white))
                continue; // odfiltruj bicie własnych figur

            // opcjonalnie: dodatkowe sito
            if (!board.isMoveValid(m)) continue;

            moves.push_back(to_sq(m.toRow, m.toCol));
        }
    }

    std::sort(moves.begin(), moves.end());
    moves.erase(std::unique(moves.begin(), moves.end()), moves.end());
    return moves;
}


// Wypełnij Move (pola i ew. promocję)
static bool fill_move_from_to(Board &board, const std::string &from, const std::string &to, Move &out, char promo = 0)
{
    if (!valid_sq(from) || !valid_sq(to))
        return false;
    int r1, c1, r2, c2;
    algToCoord(from, r1, c1);
    algToCoord(to, r2, c2);

    out.fromRow = r1;
    out.fromCol = c1;
    out.toRow = r2;
    out.toCol = c2;
    out.movedPiece = board.board[r1][c1];
    out.capturedPiece = board.board[r2][c2]; // przy EP cel jest pusty – to normalne
    out.promotion = promo;
    return true;
}

// Czy są inne figury tego samego typu (i koloru) mogące legalnie dojść na to samo pole?
static void san_disambiguation_before_move(
    const Board &board_before,
    const Move &m,                 // wykonywany ruch
    bool &needFile, bool &needRank // wynik: dopisać plik/rząd w SAN?
)
{
    needFile = false;
    needRank = false;

    const char moved = board_before.board[m.fromRow][m.fromCol];
    const bool isPawn = std::toupper(static_cast<unsigned char>(moved)) == 'P';
    if (isPawn)
        return; // piony w SAN nie potrzebują dysambiguacji (plik przy biciu już rozróżnia)

    // Zbierz wszystkie legalne ruchy z pozycji "przed ruchem"
    auto legals = const_cast<Board &>(board_before).getLegalMoves();

    // Szukamy innych ruchów na to samo pole (ta sama figura i kolor, inny "from")
    std::vector<std::pair<int, int>> colliders;
    for (const auto &x : legals)
    {
        if (x.toRow == m.toRow && x.toCol == m.toCol)
        {
            char p = board_before.board[x.fromRow][x.fromCol];
            if (p != 0 &&
                std::toupper(static_cast<unsigned char>(p)) == std::toupper(static_cast<unsigned char>(moved)) &&
                // ten sam kolor:
                (std::isupper(static_cast<unsigned char>(p)) == std::isupper(static_cast<unsigned char>(moved))) &&
                // inny start:
                !(x.fromRow == m.fromRow && x.fromCol == m.fromCol))
            {
                colliders.emplace_back(x.fromRow, x.fromCol);
            }
        }
    }
    if (colliders.empty())
        return; // brak konfliktu

    // Jeśli któryś ma inny plik (kolumnę), zwykle wystarczy podać plik.
    bool sameFileAll = true;
    bool sameRankAll = true;

    for (auto [r, c] : colliders)
    {
        if (c != m.fromCol)
            sameFileAll = false; // są różne pliki — plik rozróżni
        if (r != m.fromRow)
            sameRankAll = false; // są różne rzędy — rząd rozróżni
    }

    // Reguły SAN:
    // 1) jeśli plik rozróżnia jednoznacznie — podaj plik,
    // 2) else jeśli rząd rozróżnia — podaj rząd,
    // 3) else (nie rozróżnia ani plik, ani rząd) — podaj oba.
    // (Uwaga: sameFileAll==true oznacza: wszystkie kolidują W TYM SAMYM pliku -> plik NIE rozróżnia)
    bool fileDistinguishes = !sameFileAll;
    bool rankDistinguishes = !sameRankAll;

    if (fileDistinguishes)
    {
        needFile = true;
        needRank = false;
    }
    else if (rankDistinguishes)
    {
        needFile = false;
        needRank = true;
    }
    else
    {
        needFile = true;
        needRank = true;
    }
}
static std::string san_letter_for_piece(char piece)
{
    switch (std::toupper(static_cast<unsigned char>(piece)))
    {
    case 'N':
        return "N";
    case 'B':
        return "B";
    case 'R':
        return "R";
    case 'Q':
        return "Q";
    case 'K':
        return "K";
    default:
        return "";
    }
}
static std::string make_san_full(
    const Board &board_before, // pozycja PRZED ruchem
    const Move &m,             // ruch (z wypełnionym movedPiece/capturedPiece/promotion)
    bool isCastle, bool isCastleKingSide,
    bool isCapture, bool isEnPassant,
    bool isCheck, bool isMate)
{
    // Roszada
    if (isCastle)
        return isCastleKingSide ? "0-0" : "0-0-0";

    const char moved = m.movedPiece;
    const bool isPawn = std::toupper(static_cast<unsigned char>(moved)) == 'P';

    std::string san;

    // 1) litera figury (pion bez litery)
    if (!isPawn)
        san += san_letter_for_piece(moved);

    // 2) dysambiguacja (tylko dla figur innych niż pion)
    bool needFile = false, needRank = false;
    if (!isPawn)
        san_disambiguation_before_move(board_before, m, needFile, needRank);
    if (!isPawn)
    {
        if (needFile)
            san += static_cast<char>('a' + m.fromCol);
        if (needRank)
            san += static_cast<char>('1' + (7 - m.fromRow));
    }

    // 3) bicie (dla piona przy biciu SAN zaczyna się od pliku piona)
    if (isPawn && isCapture)
        san += static_cast<char>('a' + m.fromCol);
    if (isCapture)
        san += 'x';

    // 4) pole docelowe
    san += coordToAlg(m.toRow, m.toCol);

    // 5) promocja
    if (m.promotion && std::toupper(static_cast<unsigned char>(m.promotion)) != '?')
    {
        san += '=';
        san += static_cast<char>(std::toupper(static_cast<unsigned char>(m.promotion))); // =Q/R/B/N
    }

    // 6) sufiks + / #
    if (isMate)
        san += '#';
    else if (isCheck)
        san += '+';

    return san;
}

// Mapowanie char promocji -> nazwa dla backendu
static const char *promo_name(char p)
{
    switch (std::toupper(static_cast<unsigned char>(p)))
    {
    case 'Q':
        return "queen";
    case 'R':
        return "rook";
    case 'B':
        return "bishop";
    case 'N':
        return "knight";
    default:
        return "";
    }
}

static bool promo_char_from_name(std::string s, char& out) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c){ return std::tolower(c); });
    if      (s == "queen")  { out = 'Q'; return true; }
    else if (s == "rook")   { out = 'R'; return true; }
    else if (s == "bishop") { out = 'B'; return true; }
    else if (s == "knight") { out = 'N'; return true; }
    return false;
}


// ─────────────────────────────────────────────────────────────────────────────

int main()
{
    std::cout << "[Engine] Starting chess engine MQTT bridge..." << std::endl;

    // Inicjalizacja planszy
    Board board;
    board.startBoard();
    std::cout << "[Engine] Chess board initialized with starting position" << std::endl;

    // Konfiguracja MQTT
    MqttConfig cfg;
    cfg.host = env_or("MQTT_HOST", "localhost");
    cfg.port = std::stoi(env_or("MQTT_PORT", "1883"));
    cfg.client_id = env_or("MQTT_CLIENT_ID", "chess-engine");
    cfg.qos = 1;

   

    Client client(cfg);
    if (!client.connect())
    {
        LOG_E("[MQTT] Connection failed | host=" 
              << cfg.host << " port=" << cfg.port 
              << " client_id=" << cfg.client_id << "\n");

        return 1;
    }

    std::cout << "[MQTT] Successfully connected to broker" << std::endl;

    auto publish_engine_status = [&](const std::string &status, const std::string &msg = "")
    {
        json j = {{"status", status}};
        if (!msg.empty())
            j["message"] = msg;
        client.publish(topics::STATUS_ENGINE, j);
        LOG_D("[STATUS] Published engine status: " << j.dump());
        if (!client.publish(topics::STATUS_ENGINE, j)) {
        LOG_E("[STATUS] Failed to publish engine status to topic=" << topics::STATUS_ENGINE);
}

    };

    auto section_tag = [](const std::string& topic) -> const char* {
        if (topic == topics::POSSIBLE_MOVES_REQ)   return "POSSIBLE_MOVES";
        if (topic == topics::MOVE_ENGINE_REQ)      return "MOVE_ENGINE";
        if (topic == topics::MOVE_ENGINE_AI_REQ)   return "MOVE_AI";
        if (topic == "control/restart/external")   return "RESTART";
        return "UNHANDLED";
    };


    client.set_message_handler([&](const std::string &topic, const std::string &payload)
                               {
        std::cout << "[MQTT] Received message on topic: " << topic << std::endl;
        std::cout << "[MQTT] Payload: " << payload << std::endl;
        LOG_D("[RECV] topic=" << topic 
              << " | payload.len=" << payload.size()
              << " | preview=\"" << preview(payload, 150) << "\"");

        try {
            // =========================================================================
            // POSSIBLE MOVES
            // =========================================================================
            if (topic == topics::POSSIBLE_MOVES_REQ) {
                std::cout << "[Engine] Processing possible moves request" << std::endl;
                publish_engine_status("thinking", "generating moves");

                auto req = PossibleMovesReq::parse(payload); // { position, fen }
                std::cout << "[Engine] Request for position: " << req.position << std::endl;

                if (!req.fen.empty()) {
                    std::cout << "[Engine] Synchronizing with FEN: " << req.fen << std::endl;
                }
                else 
                {
                    LOG_E("[ENGINE][POSSIBLE_MOVES][Missing FEN in possible_moves request] Missing FEN | topic=" << topic
                          << " | payload.len=" << payload.size()
                          << " | preview=\"" << preview(payload, 150) << "\"");
                    publish_engine_status("error", "missing FEN in possible_moves/request");
                    return;
                }

                if (!board.loadFEN(req.fen)) {
                    LOG_E("[ENGINE][POSSIBLE_MOVES][Invalid FEN in possible_moves request] Invalid FEN=\"" << req.fen << "\"");

                    publish_engine_status("error", "bad FEN in possible_moves/request");
                    return;
                }

                // Zwróć ruchy wyłącznie z generatora silnika
                auto moves = possibleFrom(board, req.position);
                std::cout << "[Engine] Found " << moves.size() << " possible moves from " << req.position << std::endl;

                const std::string fen_used = fen_from_board(board);
                client.publish(
                    topics::POSSIBLE_MOVES_RES,
                    make_possible_moves_response_ex(req.position, moves, fen_used)
                );

                publish_engine_status("ready", "moves ready");
                std::cout << "[Engine] Possible moves response published" << std::endl;
                return;
            }

            // =========================================================================
            // MOVE ENGINE (walidacja i wykonanie ruchu)
            // =========================================================================
            if (topic == topics::MOVE_ENGINE_REQ) {
                std::cout << "[Engine] Processing move validation request" << std::endl;
                publish_engine_status("analyzing", "validating move");

                auto req = MoveEngineReq::parse(payload); // { from, to, current_fen, type, physical }
                std::cout << "[Engine] Move request: " << req.from << " -> " << req.to 
                          << " (type: " << req.type << ", physical: " << req.physical << ")" << std::endl;

                if (!req.current_fen.empty()) {
                    std::cout << "[Engine] Synchronizing with FEN: " << req.current_fen << std::endl;
                    if (!board.loadFEN(req.current_fen)) {
                        LOG_E("[ENGINE][MOVE_ENGINE][Invalid current_fen] Bad fen=\"" << req.current_fen << "\"");

                        publish_engine_status("error", "bad current_fen");
                        return;
                    }
                }
                const std::string fen_before = fen_from_board(board);

                // ewentualna figura promocji z requestu (tylko z MoveEngineReq)
                char promo = 0;
                if ((req.special_move == "promotion" || req.special_move == "promotion_capture") && !req.promotion_piece.empty()) {
                    std::cout << "[Engine] Promotion detected: " << req.promotion_piece << " (special_move: " << req.special_move << ")" << std::endl;
                    
                    // Walidacja dostępnych figur (fizyczne ograniczenia planszy)
                    if (!req.available_pieces.empty()) {
                        bool isAllowed = std::find(req.available_pieces.begin(), req.available_pieces.end(), req.promotion_piece) != req.available_pieces.end();
                        if (!isAllowed) {
                            LOG_E("[ENGINE][MOVE_ENGINE][Promotion piece unavailable] | "
                                  << "requested=\"" << req.promotion_piece << "\" "
                                  << "available=" << json(req.available_pieces).dump()
                                  << " | from=" << req.from << " to=" << req.to
                                  << " | fen_before=\"" << fen_before << "\"");

                            for (const auto& piece : req.available_pieces) {
                                std::cerr << piece << " ";
                            }
                            std::cerr << std::endl;
                            
                            client.publish(
                                topics::MOVE_REJECTED,
                                make_move_rejected(req.from, req.to, fen_before, req.physical, "Promotion piece not available on physical board")
                            );
                            publish_engine_status("ready", "validation done");
                            return;
                        }
                        std::cout << "[Engine] Promotion piece " << req.promotion_piece << " validated against available pieces" << std::endl;
                    }
                    
                    char p;
                    if (!promo_char_from_name(req.promotion_piece, p)) {
                        client.publish(topics::MOVE_REJECTED,
                            make_move_rejected(req.from, req.to, fen_before, req.physical, "Unknown promotion_piece"));
                        publish_engine_status("ready", "validation done");
                        return;
                    }
                    const auto [r_from, c_from] = to_rc(req.from);
                    const bool whiteMove = std::isupper(static_cast<unsigned char>(board.board[r_from][c_from]));
                    promo = whiteMove ? p : static_cast<char>(std::tolower(static_cast<unsigned char>(p)));

                }

                Move m{};
                if (!fill_move_from_to(board, req.from, req.to, m, promo)) {
                    LOG_E("[ENGINE][MOVE_ENGINE][Invalid algebraic squares in move] | "
                          << "from=" << req.from << " to=" << req.to
                          << " | fen_before=\"" << fen_before << "\""
                          << " | payload.len=" << payload.size()
                          << " | preview=\"" << preview(payload, 120) << "\"");

                    client.publish(
                        topics::MOVE_REJECTED,
                        make_move_rejected(req.from, req.to, fen_before, req.physical, "Bad algebraic square")
                    );
                    publish_engine_status("ready", "validation done");
                    return;
                }

                // Walidacja silnikiem (zero równoległej logiki)
                const bool legal = board.isMoveValid(m);
                std::cout << "[Engine] Move validation result: " << (legal ? "LEGAL" : "ILLEGAL") << std::endl;
                if (!legal) {
                    LOG_E("[ENGINE][MOVE_ENGINE][Move rejected] Illegal move from=" << req.from << " to=" << req.to
                          << " | fen_before=" << fen_before);

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

                bool in_check = board.isInCheck();
                int legal_count = 0;
                for (const auto& mv : board.getLegalMoves()) {
                    if (board.isMoveValid(mv)) { ++legal_count; if (legal_count > 0) break; }
                }
                GameState gs_old = (legal_count == 0)
                    ? (in_check ? GameState::CHECKMATE : GameState::STALEMATE)
                    : GameState::PLAYING;

                // --- NOWE: pełny stan (używamy TYLKO dla remisów) ---
                GameState gs_full = board.getGameState();
                GameState gs = gs_old;
                if (gs_full == GameState::DRAW_50_MOVES ||
                    gs_full == GameState::DRAW_REPETITION ||
                    gs_full == GameState::DRAW_INSUFFICIENT_MATERIAL) {
                    gs = gs_full;   // domykamy remisy (KK, 50, threefold)
                }

                bool gives_check = board.isInCheck();

                // ── Notation/Special/Additional
                json j = {
                    {"from", req.from},
                    {"to",   req.to},
                    {"fen",  fen_after},
                    {"next_player", next_player},
                    {"physical", req.physical},
                    {"gives_check", gives_check}
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
                    // Rozróżnij promocję z biciem od zwykłej promocji
                    if (isCapture) {
                        j["special_move"] = "promotion_capture";
                        // Dodaj informację o zbitej figurze jeśli dostępna
                        if (!req.captured_piece.empty()) {
                            j["captured_piece"] = req.captured_piece;
                        }
                    } else {
                        j["special_move"] = "promotion";
                    }
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

                j["gives_check"] = gives_check;

                switch (gs) {
                    case GameState::CHECKMATE:
                        j["game_status"] = "checkmate";
                        j["winner"] = whiteMoved ? "white" : "black";
                        j["game_ended"] = true;
                        break;

                    // wszystko, co jest remisem (w tym pat), mapujemy na "draw"
                    case GameState::STALEMATE:
                    case GameState::DRAW_50_MOVES:
                    case GameState::DRAW_REPETITION:
                    case GameState::DRAW_INSUFFICIENT_MATERIAL:
                        j["game_status"] = "draw";
                        j["game_ended"] = true;
                        // opcjonalnie: j["draw_reason"] = "stalemate" / "50move" / "threefold" / "insufficient";
                        break;

                    default:
                        j["game_status"] = "playing";
                        // upewnij się, że NIE zostaje stary "winner" z poprzedniej pozycji
                        j.erase("winner");
                        break;
                }



                client.publish(topics::MOVE_CONFIRMED, j);
                publish_engine_status("ready", "validation done");
                std::cout << "[Engine] Move confirmed and published" << std::endl;
                publish_engine_status("thinking", "ai thinking");
                return;
            }

            // =========================================================================
            // MOVE AI REQUEST (żądanie ruchu AI z backendu)
            // =========================================================================
            if (topic == topics::MOVE_ENGINE_AI_REQ) {
                std::cout << "[AI] AI request received from backend" << std::endl;
                publish_engine_status("thinking", "ai thinking");

                try {
                    auto j = json::parse(payload);
                    if (j.contains("fen") && j["fen"].is_string()) {
                        const auto fen = j["fen"].get<std::string>();
                        std::cout << "[AI] Synchronizing with FEN: " << fen << std::endl;
                        if (!board.loadFEN(fen)) {
                            std::cerr << "[AI] ERROR: Invalid FEN in AI request" << std::endl;
                            LOG_E("[AI][" << __func__ << "] Bad FEN=\"" << fen << "\" | payload.preview=\"" << preview(payload, 150) << "\"");
                            publish_engine_status("error", "bad FEN in AI request");
                            return;
                        }
                    }
                } catch (...) {
                    std::cout << "[AI] No FEN provided, using current board position" << std::endl;
                }

                // zrzut tablicy dla AI
                char arr[8][8];
                for (int r=0;r<8;++r) for (int c=0;c<8;++c) arr[r][c]=board.board[r][c];
                char side = board.activeColor;              // 'w'/'b'
                std::string cast = board.castling;          // "KQkq"/"-"
                std::string ep   = board.enPassant;         // "e3"/"-"

                std::cout << "[AI] Starting search (depth=5, time=5000ms)..." << std::endl;
                ChessAI ai;
                auto res = ai.findBestMove(arr, side, cast, ep, DEFAULT_AI_DEPTH, DEFAULT_AI_TIME_MS);

                std::cout << "[AI] Search completed" << std::endl;

                // brak ruchu?
                if (res.bestMove.fromRow == 0 && res.bestMove.fromCol == 0 &&
                    res.bestMove.toRow == 0 && res.bestMove.toCol == 0 && res.bestMove.promotion == '?') {
                    LOG_E("[AI][MOVE_AI][ No legal moves found]  fen=\"" << fen_from_board(board) << "\"");

                    publish_engine_status("error", "no legal moves");
                    return;
                }

                Move exec = res.bestMove;
                std::cout << "[AI] Best move found: " << to_sq(exec.fromRow, exec.fromCol) 
                          << " -> " << to_sq(exec.toRow, exec.toCol) << std::endl;

                Board board_before = board;

                if (!board.isMoveValid(exec)) {
                    LOG_E("[AI][MOVE_AI][AI produced illegal move!] Illegal move from=" << to_sq(exec.fromRow, exec.fromCol)
                          << " to=" << to_sq(exec.toRow, exec.toCol)
                          << " | fen_before=\"" << fen_from_board(board_before) << "\"");

                    publish_engine_status("error", "ai produced illegal move");
                    return;
                }

                // dane „przed" (do klasyfikacji EP/roszady/itd.)
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
                
                
                // wykonaj
                std::cout << "[AI] Executing AI move..." << std::endl;
                board.makeMove(exec);

                const std::string fen_after = fen_from_board(board);
                const std::string from = to_sq(exec.fromRow, exec.fromCol);
                const std::string to   = to_sq(exec.toRow,   exec.toCol);
                const std::string next_player = (board.activeColor == 'w') ? "white" : "black";
                
                // po board.makeMove(exec);
                bool in_check = board.isInCheck();
                int legal_count = 0;
                for (const auto& mv : board.getLegalMoves()) {
                    if (board.isMoveValid(mv)) { ++legal_count; if (legal_count > 0) break; }
                }
                GameState gs_old = (legal_count == 0)
                    ? (in_check ? GameState::CHECKMATE : GameState::STALEMATE)
                    : GameState::PLAYING;

                // --- NOWE: pełny stan (używamy TYLKO dla remisów) ---
                GameState gs_full = board.getGameState();
                GameState gs = gs_old;
                if (gs_full == GameState::DRAW_50_MOVES ||
                    gs_full == GameState::DRAW_REPETITION ||
                    gs_full == GameState::DRAW_INSUFFICIENT_MATERIAL) {
                    gs = gs_full;   // domykamy remisy (KK, 50, threefold)
                }

                bool gives_check = board.isInCheck();


                std::cout << "[AI] AI move executed: " << from << " -> " << to 
                          << ", Next player: " << next_player 
                          << ", Check: " << (gives_check ? "YES" : "NO") << std::endl;

                json j = {
                    {"from", from},
                    {"to", to},
                    {"fen", fen_after},
                    {"next_player", next_player},
                    {"gives_check", gives_check}
                };

                // special_move + dodatkowy ruch wieży przy roszadzie
                if (isCastle) {
                    j["special_move"] = isCastleKingSide ? "castling_kingside" : "castling_queenside";
                    // additional_moves: ruch wieży wg rzędu króla
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

                // promocja
                if (exec.promotion && std::toupper(static_cast<unsigned char>(exec.promotion)) != '?') {
                    // Rozróżnij promocję z biciem od zwykłej promocji
                    if (isCapture) {
                        j["special_move"] = "promotion_capture";
                        // AI może samodzielnie wygenerować zbitą figurę na podstawie planszy
                        if (!targetEmptyBefore) {
                            const char captured = exec.capturedPiece;
                            const std::string capturedName = [captured]() -> std::string {
                                switch (std::tolower(static_cast<unsigned char>(captured))) {
                                    case 'p': return "pawn";
                                    case 'r': return "rook";
                                    case 'n': return "knight";
                                    case 'b': return "bishop";
                                    case 'q': return "queen";
                                    case 'k': return "king";
                                    default: return "unknown";
                                }
                            }();
                            j["captured_piece"] = capturedName;
                        }
                    } else {
                        j["special_move"] = "promotion";
                    }
                    j["promotion_piece"] = promo_name(exec.promotion);
                }

                // SAN (lekki) + sufiksy
                const bool isMate = (gs == GameState::CHECKMATE);
                if (!j.contains("notation")) {
                    j["notation"] = make_san_full(board_before, exec,
                                  isCastle, isCastleKingSide,
                                  isCapture, isEnPassant,
                                  gives_check, isMate);
                }

                

                switch (gs) {
                    case GameState::CHECKMATE:
                        j["game_status"] = "checkmate";
                        j["winner"] = whiteMoved ? "white" : "black";
                        j["game_ended"] = true;
                        break;

                    // wszystko, co jest remisem (w tym pat), mapujemy na "draw"
                    case GameState::STALEMATE:
                    case GameState::DRAW_50_MOVES:
                    case GameState::DRAW_REPETITION:
                    case GameState::DRAW_INSUFFICIENT_MATERIAL:
                        j["game_status"] = "draw";
                        j["game_ended"] = true;
                        // opcjonalnie: j["draw_reason"] = "stalemate" / "50move" / "threefold" / "insufficient";
                        break;

                    default:
                        j["game_status"] = "playing";
                        // upewnij się, że NIE zostaje stary "winner" z poprzedniej pozycji
                        j.erase("winner");
                        break;
                }


                client.publish(topics::MOVE_AI, j);
                publish_engine_status("ready", "ai move published");
                std::cout << "[AI] AI move published successfully" << std::endl;
                return;
            }

            // =========================================================================
            // RESTART (start / fen)
            // =========================================================================
            if (topic == "control/restart/external") {
                std::cout << "[Engine] Processing restart request" << std::endl;
                try {
                    auto j = json::parse(payload);
                    if (j.contains("fen") && j["fen"].is_string()) {
                        const auto fen = j["fen"].get<std::string>();
                        std::cout << "[Engine] Restarting with custom FEN: " << fen << std::endl;
                        if (!board.loadFEN(fen)) {
                            LOG_E("[ENGINE][RESTART][Invalid FEN in restart request] Bad FEN=\"" << fen << "\" | payload.preview=\"" << preview(payload, 150) << "\"");

                            client.publish(topics::STATUS_ENGINE, json{{"status","error"},{"message","bad FEN"}});
                            return;
                        }
                        // Publikuj potwierdzenie resetu z FEN
                        client.publish(topics::RESET_CONFIRMED, json{
                            {"type", "reset_confirmed"},
                            {"fen", fen}
                        });
                    } else {
                        std::cout << "[Engine] Restarting with starting position" << std::endl;
                        board.startBoard();
                        // Publikuj potwierdzenie resetu z domyślnym FEN
                        const std::string startFen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
                        client.publish(topics::RESET_CONFIRMED, json{
                            {"type", "reset_confirmed"},
                            {"fen", startFen}
                        });
                    }
                    client.publish(topics::STATUS_ENGINE, json{{"status","ready"},{"message","board restarted"}});
                    std::cout << "[Engine] Board restart completed" << std::endl;
                } catch (...) {
                    std::cout << "[Engine] Restart with starting position (parse error)" << std::endl;
                    board.startBoard();
                    // Publikuj potwierdzenie resetu z domyślnym FEN
                    const std::string startFen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
                    client.publish(topics::RESET_CONFIRMED, json{
                        {"type", "reset_confirmed"},
                        {"fen", startFen}
                    });
                    client.publish(topics::STATUS_ENGINE, json{{"status","ready"},{"message","board restarted"}});
                    std::cout << "[Engine] Board restart completed" << std::endl;
                }
                return;
            }

            // Nieznany temat
            std::cout << "[Engine] WARNING: Unhandled topic: " << topic << std::endl;
            publish_engine_status("warning", std::string("unhandled topic: ") + topic);
        }
        catch (const std::exception& e) {
            std::cerr << "[Engine] EXCEPTION: Failed to handle message on " << topic 
                      << ": " << e.what() << std::endl;

            const char* TAG = section_tag(topic);
            LOG_E(std::string("[") + TAG + "][" + __func__ + "] Exception | "
                  + "topic=" + topic
                  + " | what=" + e.what()
                  + " | payload.len=" + std::to_string(payload.size())
                  + " | preview=\"" + preview(payload, 200) + "\"");

            try { publish_engine_status("error", e.what()); } catch (...) {}
        }
        catch (...) {
            const char* TAG = section_tag(topic);
            LOG_E(std::string("[") + TAG + "][" + __func__ + "] Unknown exception | "
                  + "topic=" + topic
                  + " | payload.len=" + std::to_string(payload.size())
                  + " | preview=\"" + preview(payload, 200) + "\"");
            try { publish_engine_status("error", "unknown exception"); } catch (...) {}
        }
         });

    // Subskrypcje wymagane przez backend
    std::cout << "[MQTT] Subscribing to topics..." << std::endl;
    client.subscribe(topics::POSSIBLE_MOVES_REQ, 1);
    std::cout << "[MQTT] Subscribed to: " << topics::POSSIBLE_MOVES_REQ << std::endl;

    client.subscribe(topics::MOVE_ENGINE_REQ, 1);
    std::cout << "[MQTT] Subscribed to: " << topics::MOVE_ENGINE_REQ << std::endl;


    client.subscribe(topics::MOVE_ENGINE_AI_REQ, 1);
    std::cout << "[MQTT] Subscribed to: " << topics::MOVE_ENGINE_AI_REQ << std::endl;

    client.subscribe("control/restart/external", 1);
    std::cout << "[MQTT] Subscribed to: control/restart/external" << std::endl;

    std::cout << "[Engine] Chess engine MQTT bridge is ready and listening..." << std::endl;
    std::cout << "[Engine] Press Ctrl+C to exit" << std::endl;
    client.loop_forever();
    LOG_I("[ENGINE] Entering main MQTT loop (blocking mode)");


    std::cout << "[Engine] Shutting down..." << std::endl;
    return 0;
}
