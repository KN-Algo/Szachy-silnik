#pragma once
#include <string>
#include <functional>
#include <mutex>
#include <atomic>
#include <nlohmann/json.hpp>

namespace mqttwrap {
    using json = nlohmann::json;

    /**
     * @brief Struktura konfiguracji po³¹czenia MQTT.
     * 
     * Zawiera podstawowe parametry konfiguracyjne u¿ywane przy tworzeniu klienta MQTT.
     * Wartoœci mog¹ byæ nadpisywane przez zmienne œrodowiskowe (np. MQTT_HOST, MQTT_PORT).
     */
    struct MqttConfig {
        std::string host = "localhost";      ///< Adres brokera MQTT (np. "localhost" lub "mosquitto").
        int         port = 1883;             ///< Port serwera MQTT (domyœlnie 1883).
        std::string client_id = "chess-engine"; ///< Identyfikator klienta MQTT (unikalny w obrêbie brokera).
        std::string username;                ///< Nazwa u¿ytkownika (opcjonalna, dla uwierzytelniania).
        std::string password;                ///< Has³o u¿ytkownika (opcjonalne).
        std::string topic_prefix = "";       ///< Prefiks dla nazw tematów MQTT (np. "chess/").
        bool        clean_session = true;    ///< Czy po³¹czenie jest czyste (bez historii subskrypcji).
        int         qos = 1;                 ///< Domyœlny poziom QoS (Quality of Service), 0–2.
        bool        retain = false;          ///< Czy wiadomoœci maj¹ byæ utrzymywane (retain) przez brokera.
    };

    /**
     * @brief Klasa opakowuj¹ca asynchronicznego klienta MQTT (biblioteka Paho MQTT C++).
     * 
     * Umo¿liwia ³¹czenie siê z brokerem MQTT, publikowanie i subskrybowanie komunikatów
     * w formacie JSON oraz rejestrowanie callbacków do obs³ugi wiadomoœci.
     * 
     * Klasa wykorzystuje wzorzec Pimpl (ukryta implementacja), aby oddzieliæ interfejs
     * od szczegó³ów technicznych.
     */
    class Client {
    public:
        /**
         * @brief Typ funkcji wywo³ywanej przy nadejœciu wiadomoœci.
         * 
         * @param topic Nazwa tematu (topic), na którym odebrano wiadomoœæ.
         * @param payload Treœæ wiadomoœci (payload), zazwyczaj w formacie JSON.
         */
        using MsgHandler = std::function<void(const std::string& topic, const std::string& payload)>;

        /**
         * @brief Tworzy nowego klienta MQTT z podan¹ konfiguracj¹.
         * 
         * @param cfg Struktura konfiguracyjna zawieraj¹ca parametry po³¹czenia.
         */
        explicit Client(const MqttConfig& cfg);

        /**
         * @brief Destruktor – zamyka po³¹czenie i zwalnia zasoby klienta.
         */
        ~Client();

        /**
         * @brief Nawi¹zuje po³¹czenie z brokerem MQTT.
         * 
         * @return true Jeœli po³¹czenie zosta³o nawi¹zane pomyœlnie.
         * @return false Jeœli nie uda³o siê po³¹czyæ z brokerem.
         */
        bool connect();

        /**
         * @brief Bezpiecznie roz³¹cza klienta z brokerem MQTT.
         */
        void disconnect();

        /**
         * @brief Sprawdza, czy klient jest aktualnie po³¹czony z brokerem.
         * 
         * @return true Jeœli po³¹czenie jest aktywne.
         * @return false Jeœli klient jest roz³¹czony.
         */
        bool is_connected() const;

        /**
         * @brief Utrzymuje dzia³anie procesu w trybie nieskoñczonej pêtli.
         * 
         * Funkcja blokuj¹ca (blocking). Typowo wywo³ywana na koñcu programu
         * w celu utrzymania aktywnego po³¹czenia MQTT.
         */
        void loop_forever();

        /**
         * @brief Publikuje wiadomoœæ JSON na okreœlony temat MQTT.
         * 
         * @param topic Temat (topic), na który publikowana jest wiadomoœæ.
         * @param payload Obiekt JSON reprezentuj¹cy treœæ wiadomoœci.
         * @return true Jeœli wiadomoœæ zosta³a wys³ana pomyœlnie.
         * @return false Jeœli wyst¹pi³ b³¹d podczas publikacji.
         */
        bool publish(const std::string& topic, const json& payload);

        /**
         * @brief Subskrybuje wskazany temat MQTT.
         * 
         * Po pomyœlnej subskrypcji, wszystkie wiadomoœci przychodz¹ce na dany topic
         * bêd¹ przekazywane do zarejestrowanego handlera (`set_message_handler`).
         * 
         * @param topic Nazwa tematu do subskrypcji.
         * @param qos Poziom Quality of Service (0, 1 lub 2). Domyœlnie 1.
         * @return true Jeœli subskrypcja powiod³a siê.
         * @return false Jeœli wyst¹pi³ b³¹d.
         */
        bool subscribe(const std::string& topic, int qos = 1);

        /**
         * @brief Rejestruje funkcjê callback do obs³ugi odebranych wiadomoœci.
         * 
         * @param handler Funkcja przyjmuj¹ca (topic, payload), wywo³ywana przy nadejœciu wiadomoœci.
         */
        void set_message_handler(MsgHandler handler);

    private:
        /**
         * @brief Ukryta implementacja klienta (Pimpl idiom).
         * 
         * Oddziela interfejs publiczny od szczegó³ów implementacyjnych biblioteki MQTT.
         */
        struct Impl;
        Impl* impl; ///< WskaŸnik do wewnêtrznej implementacji klienta.
    };
}
