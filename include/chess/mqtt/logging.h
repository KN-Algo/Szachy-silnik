// logging.h
#pragma once
#include <iostream>
#include <string>
#include <sstream>
#include <iomanip>
#include "chess/mqtt/config.h"

/**
 * @file logging.h
 * @brief Prosty system logowania dla silnika szachowego.
 *
 * System logowania oparty na wartościach kompilacyjnych zdefiniowanych w `config.h`.
 * Umożliwia kontrolę poziomu szczegółowości logów i maksymalnej długości podglądu payloadu.
 *
 * ## Konfiguracja
 * Parametry są zdefiniowane w pliku `config.h`:
 * - **config::LOG_DEFAULT_LEVEL** – poziom logowania:
 *   - 0 = ERROR
 *   - 1 = WARN
 *   - 2 = INFO
 *   - 3 = DEBUG
 * - **config::LOG_DEFAULT_PREVIEW_LEN** – maksymalna liczba znaków podglądu payloadu (domyślnie 256)
 */

// ─────────────────────────────────────────────────────────────────────────────
// Odczyt ustawień z config.h
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Zwraca maksymalną liczbę znaków wyświetlanych w podglądzie payloadu.
 *
 * Wartość definiowana jest w `config::LOG_DEFAULT_PREVIEW_LEN`.
 *
 * @return size_t Maksymalna liczba znaków podglądu payloadu.
 */
inline size_t log_preview_limit() {
    return config::LOG_DEFAULT_PREVIEW_LEN;
}

/**
 * @brief Zwraca bieżący poziom logowania.
 *
 * Wartość definiowana jest w `config::LOG_DEFAULT_LEVEL` (w pliku `config.h`).
 * Zmiana tej wartości pozwala kontrolować ilość logowanych informacji
 * (np. poziom DEBUG w trybie deweloperskim, INFO w produkcji).
 *
 * @return int Poziom logowania (0=ERROR, 1=WARN, 2=INFO, 3=DEBUG).
 */
inline int log_level() {
    return config::LOG_DEFAULT_LEVEL;
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
