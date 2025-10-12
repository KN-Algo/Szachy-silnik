// logging.hpp
#pragma once
#include <iostream>
#include <string>
#include <sstream>
#include <iomanip>

/**
 * @file logging.h
 * @brief Prosty system logowania dla silnika szachowego.
 *
 * Umożliwia dynamiczne sterowanie poziomem szczegółowości logów,
 * automatyczne skracanie długich komunikatów (np. dużych payloadów JSON)
 * oraz formatowanie komunikatów w czytelnej formie.
 *
 * ## Zmienne środowiskowe
 * - **LOG_PAYLOAD_PREVIEW** – maksymalna liczba znaków podglądu payloadu (domyślnie 256).
 * - **LOG_LEVEL** – poziom logowania:
 *   - 0 = ERROR
 *   - 1 = WARN
 *   - 2 = INFO
 *   - 3 = DEBUG
 */

// ─────────────────────────────────────────────────────────────────────────────
// Odczyt ustawień środowiskowych
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Odczytuje limit długości podglądu payloadu do logów.
 *
 * Wartość jest pobierana z zmiennej środowiskowej `LOG_PAYLOAD_PREVIEW`.
 * Jeśli nie jest ustawiona lub niepoprawna, zwraca wartość domyślną (256).
 *
 * @return size_t Maksymalna liczba znaków do podglądu payloadu.
 */
inline size_t log_preview_limit() {
    const char* v = std::getenv("LOG_PAYLOAD_PREVIEW");
    if (!v) return 256;
    try { return static_cast<size_t>(std::stoul(v)); }
    catch (...) { return 256; }
}

/**
 * @brief Zwraca bieżący poziom logowania ustawiony w środowisku.
 *
 * Wartość odczytywana jest ze zmiennej środowiskowej `LOG_LEVEL`.
 * Jeśli zmienna nie jest ustawiona, domyślnie zwraca poziom **INFO (3)**.
 *
 * @return int Poziom logowania (0=ERROR, 1=WARN, 2=INFO, 3=DEBUG).
 */
inline int log_level() {
    const char* v = std::getenv("LOG_LEVEL");
    if (!v) return 3; // INFO
    try { return std::stoi(v); }
    catch (...) { return 3; }
}

// ─────────────────────────────────────────────────────────────────────────────
// Skracanie długich wiadomości (np. JSON payloadów)
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Skraca zbyt długie ciągi znaków w logach, zachowując czytelny podgląd.
 *
 * Jeśli długość tekstu przekracza `maxlen`, końcówka zostaje zastąpiona
 * oznaczeniem w formacie `...(+XYZB)`, gdzie XYZ oznacza liczbę obciętych bajtów.
 *
 * @param s Tekst wejściowy do ewentualnego skrócenia.
 * @param maxlen Maksymalna długość widocznego fragmentu (domyślnie wartość z `log_preview_limit()`).
 * @return std::string Skrócony tekst z adnotacją o długości.
 *
 * @example
 * ```cpp
 * std::string data = "{" + std::string(500, 'x') + "}";
 * std::cout << preview(data);
 * // Wynik: "{xxxxx...(+244B)"
 * ```
 */
inline std::string preview(const std::string& s, size_t maxlen = log_preview_limit())
{
    if (s.size() <= maxlen) return s;
    std::ostringstream oss;
    oss << s.substr(0, maxlen) << "...(+" << (s.size() - maxlen) << "B)";
    return oss.str();
}

// ─────────────────────────────────────────────────────────────────────────────
// Makra logowania dla różnych poziomów szczegółowości
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @def LOG_E(msg)
 * @brief Loguje komunikat błędu (ERROR).
 *
 * Używaj dla poważnych błędów uniemożliwiających dalsze działanie programu.
 * Wypisuje komunikat na strumień **std::cerr**.
 *
 * @param msg Treść komunikatu (może być złożonym wyrażeniem strumieniowym).
 */
#define LOG_E(msg) do { if (log_level() >= 0) std::cerr << "[ERROR]" << msg << "\n"; } while(0)

/**
 * @def LOG_W(msg)
 * @brief Loguje ostrzeżenie (WARN).
 *
 * Używaj dla sytuacji nietypowych, które nie przerywają działania programu.
 * Wypisuje komunikat na **std::cerr**.
 *
 * @param msg Treść komunikatu.
 */
#define LOG_W(msg) do { if (log_level() >= 1) std::cerr << "[WARN ]" << msg << "\n"; } while(0)

/**
 * @def LOG_I(msg)
 * @brief Loguje komunikat informacyjny (INFO).
 *
 * Przeznaczone dla ogólnych informacji o przebiegu działania programu.
 * Wypisuje komunikat na **std::cout**.
 *
 * @param msg Treść komunikatu.
 */
#define LOG_I(msg) do { if (log_level() >= 2) std::cout << "[INFO ]" << msg << "\n"; } while(0)

/**
 * @def LOG_D(msg)
 * @brief Loguje komunikat debugujący (DEBUG).
 *
 * Używane do szczegółowych informacji dla programistów podczas debugowania.
 * Wypisuje komunikat na **std::cout**.
 *
 * @param msg Treść komunikatu.
 */
#define LOG_D(msg) do { if (log_level() >= 3) std::cout << "[DEBUG]" << msg << "\n"; } while(0)

// ─────────────────────────────────────────────────────────────────────────────
// Pomocnicze formatowanie wartości logicznych
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Konwertuje wartość logiczną na napis "true" lub "false".
 *
 * Przydatne przy wypisywaniu flag logicznych w logach.
 *
 * @param b Wartość logiczna.
 * @return std::string "true" jeśli b==true, w przeciwnym razie "false".
 */
inline std::string yesno(bool b) { return b ? "true" : "false"; }
