#pragma once
namespace topics
{
    /**
     * Zestaw stałych identyfikujących tematy MQTT używane w komunikacji
     * pomiędzy backendem Symfony a silnikiem szachowym (C++).
     *
     * Każdy temat reprezentuje określony typ wiadomości:
     *  - Backend → Engine  : żądania z aplikacji (np. wykonaj ruch, zwróć możliwe ruchy)
     *  - Engine  → Backend : odpowiedzi i statusy (np. ruch potwierdzony, stan gotowości)
     */

    // --- BACKEND → ENGINE -----------------------------------------------------

    inline constexpr const char *MOVE_ENGINE_REQ = "move/engine"; 
    // Żądanie walidacji ruchu wykonanego przez gracza (człowieka lub fizyczną planszę)

    inline constexpr const char *MOVE_ENGINE_AI_REQ = "move/engine/request"; 
    // Żądanie wygenerowania ruchu przez silnik AI

    inline constexpr const char *POSSIBLE_MOVES_REQ = "engine/possible_moves/request"; 
    // Zapytanie o możliwe ruchy dla wybranego pola

    inline constexpr const char *CONTROL_RESTART_EXTERNAL = "control/restart/external"; 
    // Zewnętrzne polecenie restartu silnika (np. z backendu lub Raspberry Pi)

    // --- ENGINE → BACKEND -----------------------------------------------------

    inline constexpr const char *POSSIBLE_MOVES_RES = "engine/possible_moves/response"; 
    // Odpowiedź z listą możliwych ruchów (na zapytanie POSSIBLE_MOVES_REQ)

    inline constexpr const char *MOVE_CONFIRMED = "engine/move/confirmed"; 
    // Potwierdzenie poprawnego ruchu i zaktualizowanego FEN

    inline constexpr const char *MOVE_REJECTED = "engine/move/rejected"; 
    // Informacja o niepoprawnym lub nielegalnym ruchu

    inline constexpr const char *STATUS_ENGINE = "status/engine"; 
    // Aktualny status działania silnika (ready, thinking, error itd.)

    inline constexpr const char *RESET_CONFIRMED = "engine/reset/confirmed"; 
    // Potwierdzenie wykonania resetu po stronie silnika

    // --- AI-SPECYFICZNE ------------------------------------------------------

    inline constexpr const char *MOVE_AI = "move/ai";              
    // Publikacja ruchu wygenerowanego przez moduł AI

    inline constexpr const char *AI_THINK_REQ = "engine/ai/think"; 
    // Wewnętrzny temat testowy – ręczne wywołanie logiki AI
}
