#pragma once
#include <string>
#include <vector>
#include <optional>
#include <nlohmann/json.hpp>
using nlohmann::json;

/**
 * ZbiÛr struktur i helperÛw opisujπcych formaty wiadomoúci MQTT
 * wymienianych miÍdzy backendem Symfony a silnikiem szachowym.
 * Kaødy typ øπdania/odpowiedzi ma odpowiadajπcπ strukturÍ C++ i metodÍ JSON parse/serialize.
 */

 // --- STATUS / HEALTH --------------------------------------------------------

inline json make_status(const std::string &status, const std::string &message = "")
{
    // Generuje prosty komunikat statusu silnika (np. "ready", "thinking", "error")
    json j;
    j["status"] = status;
    if (!message.empty())
        j["message"] = message;
    return j;
}

// --- Ø•DANIE WALIDACJI RUCHU -----------------------------------------------

struct MoveEngineReq
{
    // Pola opisujπce ruch przekazany z backendu do silnika
    std::string from, to, current_fen, type;
    bool physical;
    std::string promotion_piece;
    std::vector<std::string> available_pieces;
    std::string captured_piece;
    std::string special_move;

    // Parser JSON -> MoveEngineReq

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

        // Obs≥uga tablicy dostÍpnych bierek do promocji
        if (j.contains("available_pieces") && j["available_pieces"].is_array())
        {
            for (const auto &piece : j["available_pieces"])
            {
                if (piece.is_string())
                {
                    r.available_pieces.push_back(piece.get<std::string>());
                }
            }
        }

        return r;
    }
};

// --- Ø•DANIE LISTY MOØLIWYCH RUCH”W ----------------------------------------


struct PossibleMovesReq
{
    std::string position, fen;
    static PossibleMovesReq parse(const std::string &s)
    {
        // Parser JSON -> PossibleMovesReq

        auto j = json::parse(s);
        PossibleMovesReq r;
        r.position = j.at("position");
        r.fen = j.value("fen", "");
        return r;
    }
};

// --- ODPOWIEDè: RUCH ODRZUCONY ---------------------------------------------

inline json make_move_rejected(const std::string &from, const std::string &to, const std::string &fen_before, bool physical, const std::string &reason)
{
    // Zwraca komunikat o b≥Ídnym lub nielegalnym ruchu

    json j;
    j["from"] = from;
    j["to"] = to;
    j["fen"] = fen_before;
    j["physical"] = physical;
    j["reason"] = reason;
    return j;
}

// --- ODPOWIEDè: MOØLIWE RUCHY (rozszerzona) -------------------------------

inline json make_possible_moves_response_ex(const std::string& position, const std::vector<std::string>& moves,const std::string& fen_used) 
{
    // Zwraca listÍ moøliwych ruchÛw dla danego pola, wraz z FEN uøytym do analizy

    json j;
    j["position"]=position; 
    j["moves"]=moves; 
    j["fen_used"]=fen_used; 
    return j;
}

// --- DODATKOWE RUCHY (np. roszada, bicie w przelocie) ----------------------

struct AdditionalMove
{
    std::string from, to, piece;
};

// --- ODPOWIEDè: RUCH ZAAKCEPTOWANY -----------------------------------------

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
    /**
     * Buduje JSON odpowiedzi Ñmove confirmedî po poprawnym wykonaniu ruchu.
     * Zawiera zaktualizowany FEN, ewentualne ruchy dodatkowe oraz metadane gry.
     */
    json j;
    j["from"] = from;
    j["to"] = to;
    j["fen"] = fen_after;
    j["physical"] = physical;
    j["next_player"] = next_player;

    if (special_move)
        j["special_move"] = *special_move;

    // Dodatkowe ruchy np. roszada, bicie w przelocie
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
