#pragma once
#include <string>
#include <vector>
#include <nlohmann/json.hpp>
using nlohmann::json;

inline json make_status(const std::string &status, const std::string &message = "")
{
    json j;
    j["status"] = status;
    if (!message.empty())
        j["message"] = message;
    return j;
}

struct MoveEngineReq
{
    std::string from, to, current_fen, type;
    bool physical;
    static MoveEngineReq parse(const std::string &s)
    {
        auto j = json::parse(s);
        MoveEngineReq r;
        r.from = j.at("from");
        r.to = j.at("to");
        r.current_fen = j.at("current_fen");
        r.type = j.at("type");
        r.physical = j.at("physical");
        return r;
    }
};

struct PossibleMovesReq
{
    std::string position, fen;
    static PossibleMovesReq parse(const std::string &s)
    {
        auto j = json::parse(s);
        PossibleMovesReq r;
        r.position = j.at("position");
        r.fen = j.value("fen", ""); // opcjonalne pole fen
        return r;
    }
};

inline json make_move_confirmed(const std::string &from, const std::string &to, const std::string &fen_after, bool physical)
{
    nlohmann::json j;
    j["from"] = from;
    j["to"] = to;
    j["fen"] = fen_after;
    j["physical"] = physical;
    return j;
}
inline json make_move_rejected(const std::string &from, const std::string &to, const std::string &fen_before, bool physical, const std::string &reason)
{
    nlohmann::json j;
    j["from"] = from;
    j["to"] = to;
    j["fen"] = fen_before;
    j["physical"] = physical;
    j["reason"] = reason;
    return j;
}
inline json make_possible_moves_response(const std::string &position, const std::vector<std::string> &moves)
{
    nlohmann::json j;
    j["position"] = position;
    j["moves"] = moves;
    return j;
}

inline json make_move_confirmed(const std::string &from,
                                const std::string &to,
                                const std::string &fen,
                                bool physical,
                                const std::string &next_player = {})
{
    json j = {{"from", from}, {"to", to}, {"fen", fen}, {"physical", physical}};
    if (!next_player.empty())
        j["next_player"] = next_player;
    return j;
}

inline json make_ai_move_payload(const std::string &from,
                                 const std::string &to,
                                 const std::string &fen_after,
                                 const std::string &next_player,
                                 const std::string &special_move = "",
                                 const std::string &promotion_piece = "")
{
    json j{
        {"from", from},
        {"to", to},
        {"fen", fen_after},
        {"next_player", next_player}};
    if (!special_move.empty())
        j["special_move"] = special_move;
    if (!promotion_piece.empty())
        j["promotion_piece"] = promotion_piece; // np. "queen"
    return j;
}
