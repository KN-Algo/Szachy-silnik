#include "chess/mqtt/mqtt_client.h"
#include <mqtt/async_client.h>
#include <iostream>
#include <thread>
#include <chrono>


namespace mqtt {
    const std::string message::EMPTY_STR{};
    // Definicje brakujących symboli w wariancie static (workaround dla 1.5.2 na MSVC)

}


using namespace std::chrono_literals;

namespace mqttwrap {

struct Client::Impl : public virtual mqtt::callback, public virtual mqtt::iaction_listener {
    MqttConfig cfg;
    std::unique_ptr<mqtt::async_client> cli;
    mqtt::connect_options connOpts;
    std::atomic<bool> connected{false};
    MsgHandler on_msg;
    std::mutex mtx;

    Impl(const MqttConfig& c) : cfg(c) {
        std::string server = "tcp://" + cfg.host + ":" + std::to_string(cfg.port);
        cli = std::make_unique<mqtt::async_client>(server, cfg.client_id);
        cli->set_callback(*this);

        mqtt::connect_options_builder bob;
        bob.clean_session(cfg.clean_session);
        if (!cfg.username.empty()) bob.user_name(cfg.username);
        if (!cfg.password.empty()) bob.password(cfg.password);
        connOpts = bob.finalize();
    }

    void connected_cb(const std::string& cause) {
        (void)cause;
        connected = true;
        std::cout << "[MQTT] Connected\n";
    }

    void connection_lost(const std::string& cause) override {
        connected = false;
        std::cerr << "[MQTT] Connection lost: " << cause << "\n";
    }

    void message_arrived(mqtt::const_message_ptr msg) override {
        std::string topic = msg->get_topic();
        std::string payload = msg->to_string();
        std::lock_guard<std::mutex> lk(mtx);
        if (on_msg) on_msg(topic, payload);
    }

    void delivery_complete(mqtt::delivery_token_ptr) override {}

    void on_success(const mqtt::token& tok) override {
        (void)tok;
    }
    void on_failure(const mqtt::token& tok) override {
        (void)tok;
    }
};

Client::Client(const MqttConfig& cfg) : impl(new Impl(cfg)) {}
Client::~Client() { delete impl; }

bool Client::connect() {
    try {
        impl->cli->connect(impl->connOpts)->wait();
        impl->connected = true;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "[MQTT] Connect failed: " << e.what() << "\n";
        return false;
    }
}

void Client::disconnect() {
    try {
        impl->cli->disconnect()->wait();
        impl->connected = false;
    } catch (...) {}
}

bool Client::is_connected() const { return impl->connected.load(); }

void Client::loop_forever() {
    // Paho C++ async client doesn't need a manual loop; we can just sleep
    while (true) std::this_thread::sleep_for(200ms);
}

bool Client::publish(const std::string& topic, const json& payload) {
    try {
        auto msg = mqtt::make_message(topic, payload.dump());
        msg->set_qos(impl->cfg.qos);
        msg->set_retained(impl->cfg.retain);
        impl->cli->publish(msg);
        return true;
    } catch (const std::exception& e) {
        std::cerr << "[MQTT] Publish failed: " << e.what() << "\n";
        return false;
    }
}

bool Client::subscribe(const std::string& topic, int qos) {
    try {
        impl->cli->subscribe(topic, qos)->wait();
        std::cout << "[MQTT] Subscribed to " << topic << "\n";
        return true;
    } catch (const std::exception& e) {
        std::cerr << "[MQTT] Subscribe failed: " << e.what() << "\n";
        return false;
    }
}

void Client::set_message_handler(MsgHandler handler) {
    std::lock_guard<std::mutex> lk(impl->mtx);
    impl->on_msg = std::move(handler);
}

} // namespace mqttwrap