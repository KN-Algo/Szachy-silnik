#pragma once
namespace topics
{
    // Backend -> Engine
    inline constexpr const char *MOVE_ENGINE_REQ = "move/engine";
    inline constexpr const char *POSSIBLE_MOVES_REQ = "engine/possible_moves/request";

    // Engine -> Backend
    inline constexpr const char *POSSIBLE_MOVES_RES = "engine/possible_moves/response";
    inline constexpr const char *MOVE_CONFIRMED = "engine/move/confirmed";
    inline constexpr const char *MOVE_REJECTED = "engine/move/rejected";
    inline constexpr const char *STATUS_ENGINE = "status/engine";

    inline constexpr const char *MOVE_AI = "move/ai";                  // publikacja ruchu AI
    inline constexpr const char *AI_THINK_REQ = "move/engine/request"; // żądanie ruchu AI od backendu
}
