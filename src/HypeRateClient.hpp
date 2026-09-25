#pragma once

#include <Geode/Geode.hpp>
#include <chrono>
#include <memory>

namespace ix {
    class WebSocket;
}

enum class ConnectionStatus {
    NoId,        // no HypeRate ID entered yet
    NoApiKey,    // build without API key
    Connecting,
    Waiting,     // channel joined, waiting for the first heart rate
    Live,        // receiving heart rate
    NoSignal,    // connected, but the sensor stopped sending
    InvalidId,
    AuthFailed,  // API key rejected
    Error,
    Demo,
    Disconnected, // stopped by the player
};

// Connection to the HypeRate WebSocket (Phoenix channels).
//
// All public state lives on the main thread. Socket callbacks run on the
// IXWebSocket thread and only forward their data to the main thread via
// queueInMainThread, so the rest of the mod never has to deal with locking.
class HypeRateClient : public cocos2d::CCObject {
public:
    static HypeRateClient* get();

    // Starts the update loop and connects with the current settings.
    void start();
    // Re-reads the settings and reconnects if the ID or demo mode changed.
    void applyConfig();
    void reconnect();
    // Stops the connection until the player connects again (or changes the ID).
    void disconnectByUser();

    // Latest heart rate, or 0 if there is no usable value.
    int bpm() const;
    // True if bpm() is a current value (live or demo).
    bool hasSignal() const;
    ConnectionStatus status() const;
    std::string statusText() const;

private:
    using Clock = std::chrono::steady_clock;

    std::unique_ptr<ix::WebSocket> m_socket;
    // Increased on every (re)connect so callbacks of an old socket are ignored.
    uint64_t m_connectionGen = 0;

    std::string m_activeId;
    bool m_demoActive = false;
    bool m_started = false;

    ConnectionStatus m_status = ConnectionStatus::NoId;
    std::string m_errorText;
    int m_bpm = 0;
    Clock::time_point m_lastBpmTime{};
    Clock::time_point m_lastHeartbeat{};
    Clock::time_point m_lastTick{};
    int m_ref = 0;

    // Demo mode state
    float m_demoTime = 0.f;
    float m_demoValue = 80.f;

    void tick(float);
    void connect();
    void disconnect();
    void send(std::string const& topic, std::string const& event);

    void onOpen(uint64_t gen);
    void onClose(uint64_t gen, std::string reason);
    void onError(uint64_t gen, int httpStatus, std::string reason);
    void onText(uint64_t gen, std::string text);

    void updateDemo(float dt);
};
