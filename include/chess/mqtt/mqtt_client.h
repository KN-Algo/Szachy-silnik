#pragma once
#include <string>
#include <functional>
#include <mutex>
#include <atomic>
#include <nlohmann/json.hpp>

namespace mqttwrap {
    using json = nlohmann::json;

    struct MqttConfig {
        std::string host = "localhost";
        int         port = 1883;
        std::string client_id = "chess-engine";
        std::string username;
        std::string password;
        std::string topic_prefix = ""; // e.g. ""
        bool        clean_session = true;
        int         qos = 1;
        bool        retain = false;
    };

    class Client {
    public:
        using MsgHandler = std::function<void(const std::string& topic, const std::string& payload)>;

        explicit Client(const MqttConfig& cfg);
        ~Client();

        bool connect();
        void disconnect();
        bool is_connected() const;

        void loop_forever(); // blocking

        bool publish(const std::string& topic, const json& payload);
        bool subscribe(const std::string& topic, int qos = 1);

        void set_message_handler(MsgHandler handler);

    private:
        struct Impl;
        Impl* impl;
    };
}