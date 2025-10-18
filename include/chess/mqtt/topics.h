#pragma once

/**
 * @file topics.h
 * @brief Definicje tematów (topics) MQTT używanych w komunikacji między backendem Symfony
 * a silnikiem szachowym (C++).
 *
 * Każda stała reprezentuje konkretny kanał komunikacji (topic) w protokole MQTT.
 * 
 * - **Backend → Engine** – żądania kierowane z aplikacji do silnika (np. walidacja ruchu, restart).
 * - **Engine → Backend** – odpowiedzi i statusy publikowane przez silnik (np. potwierdzenie ruchu, status gry).
 * 
 * Tematy te służą do integracji modułów systemu Szach-Mat 2.0 i stanowią
 * wspólny interfejs wymiany komunikatów JSON.
 */
namespace topics
{
    // ─────────────────────────────────────────────────────────────────────────────
    // BACKEND → ENGINE
    // ─────────────────────────────────────────────────────────────────────────────

    /**
     * @brief Temat MQTT, na który backend wysyła żądania walidacji ruchu.
     *
     * Używany, gdy gracz (człowiek lub plansza fizyczna) wykonuje ruch, który
     * należy sprawdzić w silniku.
     *
     * Payload: obiekt JSON zgodny ze strukturą `MoveEngineReq`.
     * Przykład: `{ "from": "e2", "to": "e4", "current_fen": "...", "physical": true }`
     */
    inline constexpr const char *MOVE_ENGINE_REQ = "move/engine";

    /**
     * @brief Temat MQTT do żądania ruchu od silnika AI.
     *
     * Backend wysyła komunikat z aktualnym FEN, aby silnik AI obliczył najlepszy ruch.
     *
     * Payload: `{ "fen": "<current FEN>" }`
     */
    inline constexpr const char *MOVE_ENGINE_AI_REQ = "move/engine/request";

    /**
     * @brief Temat MQTT do zapytań o możliwe ruchy z danego pola.
     *
     * Backend wysyła żądanie, podając pozycję i aktualny FEN.
     *
     * Payload: obiekt JSON zgodny ze strukturą `PossibleMovesReq`.
     */
    inline constexpr const char *POSSIBLE_MOVES_REQ = "engine/possible_moves/request";

    /**
     * @brief Temat MQTT do zdalnego restartowania silnika (np. z poziomu backendu lub Raspberry Pi).
     *
     * Może zawierać opcjonalny FEN w celu ustawienia niestandardowej pozycji początkowej.
     */
    inline constexpr const char *CONTROL_RESTART_EXTERNAL = "control/restart/external";


    // ─────────────────────────────────────────────────────────────────────────────
    // ENGINE → BACKEND
    // ─────────────────────────────────────────────────────────────────────────────

    /**
     * @brief Temat MQTT z odpowiedzią zawierającą listę możliwych ruchów.
     *
     * Odpowiedź na `POSSIBLE_MOVES_REQ`. Zawiera listę ruchów oraz FEN użyty do analizy.
     */
    inline constexpr const char *POSSIBLE_MOVES_RES = "engine/possible_moves/response";

    /**
     * @brief Temat MQTT publikowany po poprawnym wykonaniu ruchu.
     *
     * Silnik przesyła zaktualizowany FEN, metadane gry i ewentualne ruchy dodatkowe.
     *
     * Payload: JSON generowany przez `make_move_confirmed`.
     */
    inline constexpr const char *MOVE_CONFIRMED = "engine/move/confirmed";

    /**
     * @brief Temat MQTT informujący o odrzuceniu ruchu.
     *
     * Publikowany, gdy silnik wykryje błąd w danych wejściowych lub ruch jest nielegalny.
     *
     * Payload: JSON z `make_move_rejected`.
     */
    inline constexpr const char *MOVE_REJECTED = "engine/move/rejected";

    /**
     * @brief Temat MQTT z informacjami o stanie działania silnika.
     *
     * Silnik okresowo publikuje statusy (np. „ready”, „thinking”, „error”),
     * aby backend mógł monitorować jego pracę.
     *
     * Payload: JSON z `make_status`.
     */
    inline constexpr const char *STATUS_ENGINE = "status/engine";

    /**
     * @brief Temat MQTT z potwierdzeniem restartu silnika.
     *
     * Publikowany po otrzymaniu i wykonaniu polecenia `CONTROL_RESTART_EXTERNAL`.
     * Zawiera informację o nowym FEN.
     */
    inline constexpr const char *RESET_CONFIRMED = "engine/reset/confirmed";


    // ─────────────────────────────────────────────────────────────────────────────
    // AI-SPECYFICZNE
    // ─────────────────────────────────────────────────────────────────────────────

    /**
     * @brief Temat MQTT używany przez moduł AI do publikowania swojego ruchu.
     *
     * Po zakończeniu obliczeń, silnik AI publikuje najlepszy znaleziony ruch w tym temacie.
     */
    inline constexpr const char *MOVE_AI = "move/ai";

    /**
     * @brief Temat testowy wywołujący logikę silnika AI ręcznie.
     *
     * Wykorzystywany głównie do celów debugowania i testów jednostkowych.
     */
    inline constexpr const char *AI_THINK_REQ = "engine/ai/think";
}
