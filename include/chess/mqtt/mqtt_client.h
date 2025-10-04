#pragma once
#include <string>
#include <functional>
#include <mutex>
#include <atomic>
#include <nlohmann/json.hpp>

namespace mqttwrap {
    using json = nlohmann::json;

    /**
     * Konfiguracja po³¹czenia MQTT.
     * Wartoœci mog¹ byæ nadpisywane przez zmienne œrodowiskowe (np. MQTT_HOST, MQTT_PORT).
     */

    struct MqttConfig {
        std::string host = "localhost";     // Adres brokera MQTT
        int         port = 1883;            // Port (domyœlnie 1883)
        std::string client_id = "chess-engine"; // Identyfikator klienta MQTT
        std::string username;               // Uwierzytelnianie (opcjonalne)
        std::string password;
        std::string topic_prefix = "";      // Prefiks dla nazw topiców (opcjonalny)
        bool        clean_session = true;   // Czy po³¹czenie jest czyste (bez historii)
        int         qos = 1;                // Domyœlny poziom QoS
        bool        retain = false;         // Czy wiadomoœci maj¹ byæ zapamiêtywane przez brokera
    };

    /**
     * Klasa opakowuj¹ca asynchronicznego klienta MQTT (Paho MQTT C++).
     * Umo¿liwia publikowanie i subskrybowanie komunikatów JSON.
     */
    class Client {
    public:
    // Typ funkcji wywo³ywanej przy nadejœciu wiadomoœci
        using MsgHandler = std::function<void(const std::string& topic, const std::string& payload)>;

        explicit Client(const MqttConfig& cfg);
        ~Client();

        // Nawi¹zanie / zakoñczenie po³¹czenia z brokerem
        bool connect();
        void disconnect();
        bool is_connected() const;

        // Pêtla blokuj¹ca (utrzymuje dzia³anie procesu)
        void loop_forever(); // blocking

        // Publikacja i subskrypcja tematów MQTT
        bool publish(const std::string& topic, const json& payload);
        bool subscribe(const std::string& topic, int qos = 1);

        // Rejestracja callbacka do obs³ugi odebranych wiadomoœci
        void set_message_handler(MsgHandler handler);

    private:
    // Implementacja ukryta (Pimpl idiom – oddziela interfejs od szczegó³ów)
        struct Impl;
        Impl* impl;
    };
}