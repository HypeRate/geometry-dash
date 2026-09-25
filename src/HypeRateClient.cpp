// IXWebSocket must come first: on Windows it needs <winsock2.h> before
// anything pulls in <windows.h> (and with it the old <winsock.h>).
// This file is also excluded from the precompiled header, see CMakeLists.txt.
#include <ixwebsocket/IXNetSystem.h>
#include <ixwebsocket/IXWebSocket.h>

#include "HypeRateClient.hpp"
#include "ApiKey.hpp"
#include "AutoZones.hpp"
#include "Config.hpp"

#include <cmath>
#include <random>
#include <thread>

using namespace geode::prelude;

namespace {
    constexpr auto kSocketUrl = "wss://app.hyperate.io/socket/websocket?token={}";
    // Phoenix closes the socket if it does not receive a heartbeat for a while.
    // HypeRate asks for one every 10 seconds, we stay a bit below that.
    constexpr auto kHeartbeatInterval = std::chrono::seconds(8);
    // No update for this long means the sensor/app stopped sending.
    constexpr auto kSignalTimeout = std::chrono::seconds(15);
}

HypeRateClient* HypeRateClient::get() {
    // Lives for the whole game session, never released.
    static auto instance = new HypeRateClient();
    return instance;
}

void HypeRateClient::start() {
    if (m_started) return;
    m_started = true;

    ix::initNetSystem();
    m_lastTick = Clock::now();
    CCScheduler::get()->scheduleSelector(
        schedule_selector(HypeRateClient::tick), this, 0.f, false
    );
    this->applyConfig();
}

void HypeRateClient::applyConfig() {
    auto const& config = Config::get();
    if (config.demoMode == m_demoActive && config.hyperateId == m_activeId) {
        return;
    }
    m_demoActive = config.demoMode;
    m_activeId = config.hyperateId;
    this->reconnect();
}

void HypeRateClient::reconnect() {
    this->disconnect();
    m_bpm = 0;
    m_errorText.clear();

    if (m_demoActive) {
        m_status = ConnectionStatus::Demo;
        m_demoValue = 80.f;
        return;
    }
    if (m_activeId.empty()) {
        m_status = ConnectionStatus::NoId;
        return;
    }
    if (api_key::get().empty()) {
        m_status = ConnectionStatus::NoApiKey;
        return;
    }
    this->connect();
}

void HypeRateClient::disconnectByUser() {
    log::info("Disconnected by user");
    this->disconnect();
    m_bpm = 0;
    m_errorText.clear();
    m_status = ConnectionStatus::Disconnected;
}

void HypeRateClient::connect() {
    auto gen = ++m_connectionGen;
    m_status = ConnectionStatus::Connecting;

    m_socket = std::make_unique<ix::WebSocket>();
    m_socket->setUrl(fmt::format(kSocketUrl, api_key::get()));
    m_socket->setHandshakeTimeout(10);
    m_socket->enableAutomaticReconnection();
    m_socket->setMinWaitBetweenReconnectionRetries(1000);
    m_socket->setMaxWaitBetweenReconnectionRetries(30000);

#ifdef GEODE_IS_ANDROID
    // mbedTLS has no access to the Android trust store, so we ship Mozilla's
    // CA bundle. Windows (mbedTLS) reads the system store, Apple uses its own.
    ix::SocketTLSOptions tls;
    tls.caFile = utils::string::pathToString(Mod::get()->getResourcesDir() / "cacert.pem");
    m_socket->setTLSOptions(tls);
#endif

    // Runs on the socket thread: only copy the data and hop to the main thread.
    m_socket->setOnMessageCallback([this, gen](ix::WebSocketMessagePtr const& msg) {
        switch (msg->type) {
            case ix::WebSocketMessageType::Open:
                queueInMainThread([this, gen] { this->onOpen(gen); });
                break;
            case ix::WebSocketMessageType::Close:
                queueInMainThread([this, gen, reason = msg->closeInfo.reason] {
                    this->onClose(gen, reason);
                });
                break;
            case ix::WebSocketMessageType::Error:
                queueInMainThread([this, gen, status = msg->errorInfo.http_status, reason = msg->errorInfo.reason] {
                    this->onError(gen, status, reason);
                });
                break;
            case ix::WebSocketMessageType::Message:
                queueInMainThread([this, gen, text = msg->str] {
                    this->onText(gen, text);
                });
                break;
            default:
                break;
        }
    });

    log::info("Connecting to HypeRate (ID: {})", m_activeId);
    m_socket->start();
}

void HypeRateClient::disconnect() {
    if (!m_socket) return;

    // Ignore any callbacks that are still queued from this socket.
    ++m_connectionGen;
    m_socket->disableAutomaticReconnection();
    // stop() waits for the socket thread, don't block the game for that.
    std::thread([socket = std::move(m_socket)] {
        socket->stop();
    }).detach();
}

void HypeRateClient::send(std::string const& topic, std::string const& event) {
    if (!m_socket) return;
    auto message = matjson::makeObject({
        { "topic", topic },
        { "event", event },
        { "payload", matjson::Value::object() },
        { "ref", m_ref++ },
    });
    m_socket->send(message.dump(matjson::NO_INDENTATION));
}

void HypeRateClient::onOpen(uint64_t gen) {
    if (gen != m_connectionGen) return;
    log::info("Connected to HypeRate, joining channel");
    m_status = ConnectionStatus::Waiting;
    m_lastHeartbeat = Clock::now();
    this->send("hr:" + m_activeId, "phx_join");
}

void HypeRateClient::onClose(uint64_t gen, std::string reason) {
    if (gen != m_connectionGen) return;
    log::info("HypeRate connection closed: {}", reason);
    // Automatic reconnection is still running unless we stopped on purpose.
    if (m_status != ConnectionStatus::InvalidId && m_status != ConnectionStatus::AuthFailed) {
        m_status = ConnectionStatus::Connecting;
    }
}

void HypeRateClient::onError(uint64_t gen, int httpStatus, std::string reason) {
    if (gen != m_connectionGen) return;
    log::warn("HypeRate connection error ({}): {}", httpStatus, reason);

    // HypeRate answers an invalid key with a plain HTTP 200 instead of the
    // WebSocket upgrade (101). Retrying with the same key will not help.
    if (httpStatus == 200 || httpStatus == 401 || httpStatus == 403) {
        m_status = ConnectionStatus::AuthFailed;
        this->disconnect();
        return;
    }
    m_status = ConnectionStatus::Error;
    m_errorText = reason;
}

void HypeRateClient::onText(uint64_t gen, std::string text) {
    if (gen != m_connectionGen) return;

    auto parsed = matjson::parse(text);
    if (!parsed) {
        log::warn("Invalid message from HypeRate: {}", text);
        return;
    }
    auto const& json = parsed.unwrap();
    auto event = json["event"].asString().unwrapOr("");
    auto topic = json["topic"].asString().unwrapOr("");
    auto const channel = "hr:" + m_activeId;

    if (event == "hr_update") {
        auto hr = json["payload"]["hr"].asInt().unwrapOr(0);
        log::debug("hr_update: {}", hr);
        if (hr > 0) {
            m_bpm = static_cast<int>(hr);
            m_lastBpmTime = Clock::now();
            m_status = ConnectionStatus::Live;

            // Learn the normal heart rate while actually playing a level.
            auto playLayer = PlayLayer::get();
            if (playLayer && !playLayer->m_isPaused) {
                AutoZones::get().addSample(m_bpm);
            }
        }
    }
    else if (event == "phx_reply" && topic == channel) {
        auto replyStatus = json["payload"]["status"].asString().unwrapOr("");
        if (replyStatus == "error") {
            log::warn("HypeRate rejected channel join: {}", text);
            m_status = ConnectionStatus::InvalidId;
            this->disconnect();
        }
    }
    else if ((event == "phx_error" || event == "phx_close") && topic == channel) {
        // The channel died on the server, join it again.
        this->send(channel, "phx_join");
    }
}

void HypeRateClient::tick(float) {
    auto now = Clock::now();
    auto dt = std::chrono::duration<float>(now - m_lastTick).count();
    m_lastTick = now;

    if (m_status == ConnectionStatus::Demo) {
        this->updateDemo(dt);
        return;
    }

    bool joined = m_status == ConnectionStatus::Waiting
        || m_status == ConnectionStatus::Live
        || m_status == ConnectionStatus::NoSignal;

    if (joined && now - m_lastHeartbeat >= kHeartbeatInterval) {
        m_lastHeartbeat = now;
        this->send("phoenix", "heartbeat");
    }

    if (m_status == ConnectionStatus::Live && now - m_lastBpmTime > kSignalTimeout) {
        m_status = ConnectionStatus::NoSignal;
    }
}

void HypeRateClient::updateDemo(float dt) {
    static std::mt19937 rng{ std::random_device{}() };
    std::normal_distribution<float> noise(0.f, 1.5f);

    // Slow "calm -> tense -> calm" wave plus some jitter.
    m_demoTime += dt;
    float target = 100.f + 40.f * std::sin(m_demoTime * 0.12f);
    m_demoValue += (target - m_demoValue) * std::min(1.f, dt * 0.8f) + noise(rng) * dt * 4.f;
    m_demoValue = std::clamp(m_demoValue, 55.f, 190.f);
    m_bpm = static_cast<int>(std::round(m_demoValue));
}

int HypeRateClient::bpm() const {
    return this->hasSignal() ? m_bpm : 0;
}

bool HypeRateClient::hasSignal() const {
    return (m_status == ConnectionStatus::Live || m_status == ConnectionStatus::Demo) && m_bpm > 0;
}

ConnectionStatus HypeRateClient::status() const {
    return m_status;
}

std::string HypeRateClient::statusText() const {
    switch (m_status) {
        case ConnectionStatus::NoId: return "Enter your HypeRate ID in the mod settings";
        case ConnectionStatus::NoApiKey: return "This build has no API key";
        case ConnectionStatus::Connecting: return "Connecting...";
        // HypeRate does not reject unknown IDs, they just never send data.
        case ConnectionStatus::Waiting: return "Waiting for heart rate - check your ID and sensor";
        case ConnectionStatus::Live: return "Live";
        case ConnectionStatus::NoSignal: return "No signal - is your sensor on?";
        case ConnectionStatus::InvalidId: return "Unknown HypeRate ID";
        case ConnectionStatus::AuthFailed: return "HypeRate refused the connection";
        case ConnectionStatus::Error: return m_errorText.empty() ? "Connection error" : "Connection error: " + m_errorText;
        case ConnectionStatus::Demo: return "Demo mode";
        case ConnectionStatus::Disconnected: return "Disconnected";
    }
    return "";
}
