#include "chess/mqtt/mqtt_client.h"
#include <mqtt/async_client.h>
#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>
#include "chess/mqtt/logging.h"

using namespace std::chrono_literals;

namespace mqttwrap
{

    struct Client::Impl : public virtual mqtt::callback, public virtual mqtt::iaction_listener
    {
        MqttConfig cfg;
        std::unique_ptr<mqtt::async_client> cli;
        mqtt::connect_options connOpts;
        std::atomic<bool> is_connected_{false};
        MsgHandler on_msg;
        std::mutex mtx;

        Impl(const MqttConfig &c) : cfg(c)
        {
            std::string server = "tcp://" + cfg.host + ":" + std::to_string(cfg.port);
            cli = std::make_unique<mqtt::async_client>(server, cfg.client_id);
            cli->set_callback(*this);

            mqtt::connect_options_builder bob;
            bob.clean_session(cfg.clean_session);
            if (!cfg.username.empty())
                bob.user_name(cfg.username);
            if (!cfg.password.empty())
                bob.password(cfg.password);
            connOpts = bob.finalize();
        }

        void connected(const std::string &cause) override
        {
            (void)cause;
            is_connected_ = true;
            // std::cout << "[MQTT] Connected\n";
            LOG_I("[MQTT] connected host=" << cfg.host << ":" << cfg.port
                << " client_id=" << cfg.client_id << " clean_session=" << yesno(cfg.clean_session)<< "\n");
        }

        void connection_lost(const std::string &cause) override
        {
            is_connected_ = false;
            if (cause.empty()) {
                LOG_E("[MQTT] connection lost (no cause provided)");
            } else {
                LOG_E("[MQTT] connection lost: " << cause);
            }
            }

        void message_arrived(mqtt::const_message_ptr msg) override
        {
            std::string topic = msg->get_topic();
            std::string payload = msg->to_string();
            LOG_D("[MQTT] arrived topic=\"" << topic << "\" qos=" << msg->get_qos()
                << " retained=" << yesno(msg->is_retained())
                << " payload.len=" << payload.size()
                << " payload.preview=\"" << preview(payload) << "\"");
            std::lock_guard<std::mutex> lk(mtx);
            if (on_msg)
                on_msg(topic, payload);
        }

        void delivery_complete(mqtt::delivery_token_ptr) override {}

        void on_success(const mqtt::token &tok) override {
            try {
                LOG_D("[MQTT] action success token=" << tok.get_message_id());
            } catch (...) 
            { 
                LOG_W("[MQTT] action success (token has no ID)");
            }
        }
        void on_failure(const mqtt::token &tok) override {
            try {
                LOG_W("[MQTT] action failure token=" << tok.get_message_id());
            } catch (...) 
            {
                LOG_W("[MQTT] action failure (token unavailable)"); 
            }
        }

    };

    Client::Client(const MqttConfig &cfg) : impl(new Impl(cfg)) {}
    Client::~Client() { delete impl; }

    bool Client::connect()
    {
        try
        {
            LOG_I("[MQTT] connecting to " << impl->cfg.host << ":" << impl->cfg.port
             << " client_id=" << impl->cfg.client_id);
            impl->cli->connect(impl->connOpts)->wait();
            impl->is_connected_ = true;
            LOG_I("[MQTT] Successfully connected to broker");
            return true;
        }
        catch (const std::exception &e)
        {
            LOG_E(std::string("[MQTT] connect failed: ") + e.what());
            return false;
        }
    }

    void Client::disconnect()
{
    try
    {
        LOG_I("[MQTT] disconnecting...");
        impl->cli->disconnect()->wait();
        impl->is_connected_ = false;
        LOG_I("[MQTT] disconnected");
    }
    catch (const std::exception& e)
    {
        LOG_W(std::string("[MQTT] disconnect exception: ") + e.what());
    }
    catch (...)
    {
        LOG_W("[MQTT] disconnect unknown exception");
    }
}

    bool Client::is_connected() const { return impl->is_connected_.load(); }

    void Client::loop_forever()
    {
        // Paho C++ async client doesn't need a manual loop; we can just sleep
        while (true)
            std::this_thread::sleep_for(200ms);
    }

    bool Client::publish(const std::string &topic, const json &payload)
    {
        try {
        const std::string dumped = payload.dump();
        LOG_D("[MQTT] publish topic=\"" << topic << "\" qos=" << impl->cfg.qos
             << " retain=" << yesno(impl->cfg.retain)
             << " payload.len=" << dumped.size()
             << " payload.preview=\"" << preview(dumped) << "\"");
        auto msg = mqtt::make_message(topic, dumped);
        msg->set_qos(impl->cfg.qos);
        msg->set_retained(impl->cfg.retain);
        impl->cli->publish(msg);
        return true;
    } catch (const std::exception &e) {
        LOG_E(std::string("[MQTT] publish failed topic=\"") + topic + "\": " + e.what());
        return false;
    }
    }

    bool Client::subscribe(const std::string &topic, int qos)
    {
         try {
            LOG_I("[MQTT] subscribe topic=\"" << topic << "\" qos=" << qos);
            impl->cli->subscribe(topic, qos)->wait();
            LOG_I("[MQTT] subscribed topic=\"" << topic << "\"");
            return true;
        } catch (const std::exception &e) {
            LOG_E(std::string("[MQTT] subscribe failed topic=\"") + topic + "\" : " + e.what());
            return false;
        }
    }

    void Client::set_message_handler(MsgHandler handler)
    {
        std::lock_guard<std::mutex> lk(impl->mtx);
        impl->on_msg = std::move(handler);
    }

} // namespace mqttwrap