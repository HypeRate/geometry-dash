#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>

#include "Config.hpp"
#include "HeartMap.hpp"
#include "HypeRateClient.hpp"
#include "LevelStats.hpp"
#include "ui/BpmDisplay.hpp"
#include "ui/HeatStrip.hpp"

using namespace geode::prelude;

namespace {
    // Ice Cold Mode: short grace period after (re)spawning, so a respawn is
    // at least visible before the next death.
    constexpr float kRespawnGrace = 0.5f;
    // Start warning this many BPM below the limit.
    constexpr int kWarningRange = 10;
}

class $modify(HRPlayLayer, PlayLayer) {
    struct Fields {
        Ref<BpmDisplay> display;
        Ref<HeatStrip> heatStrip;
        float aliveTime = 0.f;
    };

    bool init(GJGameLevel* level, bool useReplay, bool dontCreateObjects) {
        if (!PlayLayer::init(level, useReplay, dontCreateObjects)) return false;

        auto display = BpmDisplay::create();
        // The UI layer is not moved by the camera, so the HUD stays in place.
        if (m_uiLayer) {
            m_uiLayer->addChild(display, 100);
        }
        else {
            this->addChild(display, 1000);
        }
        m_fields->display = display;

        HeartMap::get().beginLevel(LevelStats::keyFor(m_level));
        // Platformer levels have no progress percentage to map the heart rate to.
        if (m_progressBar && !m_isPlatformer) {
            auto barSize = m_progressBar->getContentSize();
            auto strip = HeatStrip::create(barSize.width - 8.f);
            strip->setPosition({ 4.f, -4.f });
            m_progressBar->addChild(strip);
            m_fields->heatStrip = strip;
        }

        this->updateDisplayVisibility();
        return true;
    }

    void levelComplete() {
        HeartMap::get().completeAttempt();
        PlayLayer::levelComplete();
    }

    void onQuit() {
        HeartMap::get().endLevel();
        PlayLayer::onQuit();
    }

    void resetLevel() {
        PlayLayer::resetLevel();
        m_fields->aliveTime = 0.f;
        HeartMap::get().beginAttempt();
    }

    void postUpdate(float dt) {
        PlayLayer::postUpdate(dt);
        m_fields->aliveTime += dt;

        this->updateDisplayVisibility();
        this->recordHeartMap(dt);
        this->updateIceCold();
    }

    void recordHeartMap(float dt) {
        if (m_isPlatformer || !m_player1 || m_player1->m_isDead || m_hasCompletedLevel) return;

        auto client = HypeRateClient::get();
        if (!client->hasSignal()) return;
        // Practice runs and start position tests show up live, but their
        // progress isn't real, so they are not saved to the level stats.
        bool persist = !m_isPracticeMode && !m_isTestMode;
        HeartMap::get().record(this->getCurrentPercent(), client->bpm(), dt, persist);
    }

    void updateDisplayVisibility() {
        auto display = m_fields->display.data();
        if (!display) return;

        auto const& config = Config::get();
        display->setVisible(config.showDisplay && (config.showInPractice || !m_isPracticeMode));

        if (auto strip = m_fields->heatStrip.data()) {
            strip->setVisible(config.heatStrip);
        }
    }

    // Kills the player when the heart rate reaches the limit. This can only
    // make the game harder, never easier.
    void updateIceCold() {
        auto const& config = Config::get();
        auto client = HypeRateClient::get();

        bool active = config.deathMode
            && (config.deathInPractice || !m_isPracticeMode)
            // Never kill without a real reading (e.g. connection lost).
            && client->hasSignal();
        int bpm = client->bpm();

        if (auto display = m_fields->display.data()) {
            display->setWarning(active && bpm >= config.deathBpm - kWarningRange);
        }

        if (!active || bpm < config.deathBpm) return;
        if (!m_player1 || m_player1->m_isDead || m_hasCompletedLevel) return;
        if (m_fields->aliveTime < kRespawnGrace) return;

        log::debug("Ice Cold: {} BPM >= limit {}", bpm, config.deathBpm);
        this->destroyPlayer(m_player1, nullptr);
    }
};
