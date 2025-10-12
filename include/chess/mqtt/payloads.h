#pragma once
#include <string>
#include <vector>
#include <optional>
#include <nlohmann/json.hpp>
using nlohmann::json;

/**
 * @file payloads.h
 * @brief Definicje struktur i funkcji pomocniczych (helperów) opisuj¹cych formaty wiadomoœci MQTT
 * wymienianych miêdzy backendem Symfony a silnikiem szachowym.
 *
 * Ka¿dy typ ¿¹dania lub odpowiedzi ma odpowiadaj¹c¹ strukturê C++ oraz metody
 * konwersji JSON (parse/serialize).
 */

// --- STATUS / HEALTH --------------------------------------------------------

/**
 * @brief Tworzy prosty komunikat statusowy silnika.
 *
 * @param status Aktualny status (np. "ready", "thinking", "error").
 * @param message (opcjonalny) Dodatkowa wiadomoœæ opisowa.
 * @return json Obiekt JSON zawieraj¹cy pola `status` oraz opcjonalnie `message`.
 */
inline json make_status(const std::string &status, const std::string &message = "")
{
    json j;
    j["status"] = status;
    if (!message.empty())
        j["message"] = message;
    return j;
}

// --- ¯¥DANIE WALIDACJI RUCHU -----------------------------------------------

/**
 * @brief Struktura reprezentuj¹ca ¿¹danie walidacji ruchu przekazywane z backendu do silnika.
 *
 * Zawiera wszystkie informacje potrzebne do sprawdzenia poprawnoœci ruchu, w tym
 * pola, bie¿¹cy FEN oraz dane dotycz¹ce promocji.
 */
struct MoveEngineReq
{
    std::string from;                     ///< Pole pocz¹tkowe w notacji algebraicznej (np. "e2").
    std::string to;                       ///< Pole docelowe w notacji algebraicznej (np. "e4").
    std::string current_fen;              ///< Aktualny stan gry w formacie FEN.
    std::string type;                     ///< Typ ruchu (np. "normal", "promotion").
    bool physical;                        ///< Czy ruch pochodzi z fizycznej planszy.
    std::string promotion_piece;          ///< Nazwa figury u¿ytej do promocji (np. "queen").
    std::vector<std::string> available_pieces; ///< Lista figur dostêpnych do promocji na fizycznej planszy.
    std::string captured_piece;           ///< Nazwa zbitej figury (jeœli dotyczy).
    std::string special_move;             ///< Typ specjalnego ruchu (np. "castling_kingside", "en_passant").

    /**
     * @brief Konwertuje obiekt JSON na strukturê MoveEngineReq.
     *
     * @param s Ci¹g JSON reprezentuj¹cy ¿¹danie.
     * @return MoveEngineReq Zdeserializowana struktura ¿¹dania ruchu.
     */
    static MoveEngineReq parse(const std::string &s)
    {
        auto j = json::parse(s);
        MoveEngineReq r;
        r.from = j.at("from");
        r.to = j.at("to");
        r.current_fen = j.value("current_fen", "");
        r.type = j.value("type", "normal");
        r.physical = j.value("physical", false);
        r.promotion_piece = j.value("promotion_piece", "");
        r.captured_piece = j.value("captured_piece", "");
        r.special_move = j.value("special_move", "");

        // Obs³uga tablicy dostêpnych figur do promocji
        if (j.contains("available_pieces") && j["available_pieces"].is_array())
        {
            for (const auto &piece : j["available_pieces"])
            {
                if (piece.is_string())
                    r.available_pieces.push_back(piece.get<std::string>());
            }
        }

        return r;
    }
};

// --- ¯¥DANIE LISTY MO¯LIWYCH RUCHÓW ----------------------------------------

/**
 * @brief Struktura reprezentuj¹ca ¿¹danie listy mo¿liwych ruchów z danego pola.
 */
struct PossibleMovesReq
{
    std::string position; ///< Pole startowe w notacji algebraicznej (np. "e2").
    std::string fen;      ///< Aktualny stan gry w formacie FEN.

    /**
     * @brief Konwertuje obiekt JSON na strukturê PossibleMovesReq.
     *
     * @param s Ci¹g JSON z danymi ¿¹dania.
     * @return PossibleMovesReq Zdeserializowana struktura ¿¹dania mo¿liwych ruchów.
     */
    static PossibleMovesReq parse(const std::string &s)
    {
        auto j = json::parse(s);
        PossibleMovesReq r;
        r.position = j.at("position");
        r.fen = j.value("fen", "");
        return r;
    }
};

// --- ODPOWIED: RUCH ODRZUCONY ---------------------------------------------

/**
 * @brief Tworzy odpowiedŸ JSON informuj¹c¹ o odrzuceniu ruchu.
 *
 * @param from Pole pocz¹tkowe ruchu.
 * @param to Pole docelowe ruchu.
 * @param fen_before FEN pozycji przed wykonaniem ruchu.
 * @param physical Czy ruch pochodzi³ z fizycznej planszy.
 * @param reason Powód odrzucenia (np. "Illegal move", "Bad FEN").
 * @return json Obiekt JSON opisuj¹cy odrzucony ruch.
 */
inline json make_move_rejected(const std::string &from, const std::string &to, const std::string &fen_before, bool physical, const std::string &reason)
{
    json j;
    j["from"] = from;
    j["to"] = to;
    j["fen"] = fen_before;
    j["physical"] = physical;
    j["reason"] = reason;
    return j;
}

// --- ODPOWIED: MO¯LIWE RUCHY (rozszerzona) -------------------------------

/**
 * @brief Tworzy odpowiedŸ JSON zawieraj¹c¹ listê mo¿liwych ruchów dla danego pola.
 *
 * @param position Pole startowe w notacji algebraicznej.
 * @param moves Lista mo¿liwych pól docelowych.
 * @param fen_used FEN u¿yty do generowania listy ruchów.
 * @return json Obiekt JSON z list¹ ruchów i u¿ytym FEN-em.
 */
inline json make_possible_moves_response_ex(const std::string& position, const std::vector<std::string>& moves, const std::string& fen_used)
{
    json j;
    j["position"] = position;
    j["moves"] = moves;
    j["fen_used"] = fen_used;
    return j;
}

// --- DODATKOWE RUCHY (np. roszada, bicie w przelocie) ----------------------

/**
 * @brief Struktura opisuj¹ca dodatkowy ruch (np. ruch wie¿y przy roszadzie).
 */
struct AdditionalMove
{
    std::string from;  ///< Pole pocz¹tkowe dodatkowego ruchu.
    std::string to;    ///< Pole docelowe dodatkowego ruchu.
    std::string piece; ///< Typ figury wykonuj¹cej ruch (np. "rook").
};

// --- ODPOWIED: RUCH ZAAKCEPTOWANY -----------------------------------------

/**
 * @brief Buduje JSON odpowiedzi „move confirmed” po poprawnym wykonaniu ruchu.
 *
 * Zawiera zaktualizowany FEN, ewentualne ruchy dodatkowe (np. przy roszadzie),
 * metadane gry oraz informacje o promocji, szachu i koñcu partii.
 *
 * @param from Pole pocz¹tkowe ruchu.
 * @param to Pole docelowe ruchu.
 * @param fen_after Nowy stan gry w formacie FEN.
 * @param physical Czy ruch pochodzi³ z fizycznej planszy.
 * @param next_player Nastêpny gracz ("white" lub "black").
 * @param special_move (opcjonalne) Typ specjalnego ruchu (np. "castling_queenside").
 * @param additional_moves (opcjonalne) Lista dodatkowych ruchów (np. ruch wie¿y przy roszadzie).
 * @param promotion_piece (opcjonalne) Figura promocji (np. "queen").
 * @param notation (opcjonalne) Notacja SAN ruchu.
 * @param gives_check (opcjonalne) Czy ruch daje szacha.
 * @param game_status (opcjonalne) Aktualny stan gry ("playing", "checkmate", "draw").
 * @param winner (opcjonalne) Zwyciêzca partii ("white" lub "black").
 * @return json Obiekt JSON reprezentuj¹cy zaakceptowany ruch.
 */
inline json make_move_confirmed(
    const std::string &from, const std::string &to, const std::string &fen_after,
    bool physical, const std::string &next_player,
    const std::optional<std::string> &special_move = std::nullopt,
    const std::vector<AdditionalMove> &additional_moves = {},
    const std::optional<std::string> &promotion_piece = std::nullopt,
    const std::optional<std::string> &notation = std::nullopt,
    const std::optional<bool> &gives_check = std::nullopt,
    const std::optional<std::string> &game_status = std::nullopt,
    const std::optional<std::string> &winner = std::nullopt)
{
    json j;
    j["from"] = from;
    j["to"] = to;
    j["fen"] = fen_after;
    j["physical"] = physical;
    j["next_player"] = next_player;

    if (special_move)
        j["special_move"] = *special_move;

    if (!additional_moves.empty())
    {
        j["additional_moves"] = json::array();
        for (const auto &am : additional_moves)
            j["additional_moves"].push_back({{"from", am.from}, {"to", am.to}, {"piece", am.piece}});
    }

    if (promotion_piece)
        j["promotion_piece"] = *promotion_piece;
    if (notation)
        j["notation"] = *notation;
    if (gives_check)
        j["gives_check"] = *gives_check;
    if (game_status)
        j["game_status"] = *game_status;
    if (winner)
        j["winner"] = *winner;

    return j;
}
