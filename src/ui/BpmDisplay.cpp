#include "BpmDisplay.hpp"
#include "Config.hpp"
#include "HeartStyle.hpp"
#include "HypeRateClient.hpp"
#include "Zones.hpp"

using namespace geode::prelude;

namespace {
    constexpr int kBeatActionTag = 0x4852;  // "HR"
    // Heart sprites are 128px at UHD, i.e. 32 points before scaling.
    constexpr float kHeartScale = 0.8f;
    constexpr float kHeartBeatScale = 0.96f;
    constexpr ccColor3B kOffline = { 130, 130, 130 };
    // Fade in/out speed when the signal comes and goes (per second).
    constexpr float kFadeSpeed = 3.f;
}

BpmDisplay* BpmDisplay::create(bool followConfigPosition) {
    auto ret = new BpmDisplay();
    if (ret->init(followConfigPosition)) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

bool BpmDisplay::init(bool followConfigPosition) {
    if (!CCNodeRGBA::init()) return false;

    m_followConfigPosition = followConfigPosition;
    this->setID("bpm-display"_spr);
    this->setContentSize({ 90.f, 34.f });
    this->setAnchorPoint({ .5f, .5f });
    // Cascade so the opacity setting applies to all children.
    this->setCascadeOpacityEnabled(true);

    m_heartStyle = Config::get().heartStyle;
    m_heart = CCSprite::create(heart_style::file(m_heartStyle));
    m_heart->setScale(kHeartScale);
    m_heart->setPosition({ 16.f, 17.f });
    this->addChild(m_heart);

    m_bpmLabel = CCLabelBMFont::create("--", "bigFont.fnt");
    m_bpmLabel->setAnchorPoint({ 0.f, .5f });
    m_bpmLabel->setScale(0.6f);
    m_bpmLabel->setPosition({ 34.f, 19.f });
    this->addChild(m_bpmLabel);

    m_unitLabel = CCLabelBMFont::create("BPM", "goldFont.fnt");
    m_unitLabel->setAnchorPoint({ 0.f, .5f });
    m_unitLabel->setScale(0.35f);
    m_unitLabel->setPosition({ 35.f, 5.f });
    this->addChild(m_unitLabel);

    // Start hidden right away if there is no signal, instead of fading out.
    m_visibility = this->shouldBeVisible() ? 1.f : 0.f;
    this->applyLayout();
    return true;
}

// Updates from the global scheduler instead of scheduleUpdate(), so the
// display keeps running regardless of how its parent layer pauses children.
void BpmDisplay::onEnter() {
    CCNodeRGBA::onEnter();
    CCScheduler::get()->scheduleSelector(schedule_selector(BpmDisplay::update), this, 0.f, false);
}

void BpmDisplay::onExit() {
    CCScheduler::get()->unscheduleSelector(schedule_selector(BpmDisplay::update), this);
    CCNodeRGBA::onExit();
}

void BpmDisplay::applyLayout() {
    auto const& config = Config::get();
    m_configGen = config.generation;

    this->setScale(config.hudScale);

    if (config.heartStyle != m_heartStyle) {
        m_heartStyle = config.heartStyle;
        heart_style::apply(m_heart, m_heartStyle);
    }

    if (m_followConfigPosition) {
        auto winSize = CCDirector::get()->getWinSize();
        this->setPosition({ config.hudX * winSize.width, config.hudY * winSize.height });
    }
}

bool BpmDisplay::shouldBeVisible() const {
    // The HUD editor preview (not following the config) always stays visible.
    return !m_followConfigPosition
        || !Config::get().hideWithoutSignal
        || HypeRateClient::get()->hasSignal();
}

void BpmDisplay::setWarning(bool warning) {
    if (m_warning == warning) return;
    m_warning = warning;
    m_warningTime = 0.f;
    if (!warning) {
        m_bpmLabel->setScale(0.6f);
    }
}

void BpmDisplay::update(float dt) {
    auto const& config = Config::get();
    if (m_configGen != config.generation) {
        this->applyLayout();
    }

    auto client = HypeRateClient::get();
    int bpm = client->bpm();

    if (bpm != m_shownBpm) {
        m_shownBpm = bpm;
        m_bpmLabel->setString(bpm > 0 ? std::to_string(bpm).c_str() : "--");
    }

    float target = this->shouldBeVisible() ? 1.f : 0.f;
    m_visibility += std::clamp(target - m_visibility, -dt * kFadeSpeed, dt * kFadeSpeed);
    int opacity = m_previewOpacity >= 0 ? m_previewOpacity : config.hudOpacity;
    this->setOpacity(static_cast<GLubyte>(opacity * m_visibility));

    auto color = bpm > 0 ? this->zoneColor(bpm) : kOffline;
    m_heart->setColor(color);
    m_bpmLabel->setColor(config.zoneColors && bpm > 0 ? color : ccWHITE);

    // Advance the beat phase at the real heart rate (beats per second).
    if (bpm > 0 && config.pulseAnimation) {
        m_beatPhase += dt * (bpm / 60.f);
        if (m_beatPhase >= 1.f) {
            m_beatPhase -= std::floor(m_beatPhase);
            this->beat();
        }
    }

    if (m_warning) {
        m_warningTime += dt;
        float wobble = 0.5f + 0.5f * std::sin(m_warningTime * 14.f);
        m_bpmLabel->setScale(0.6f + 0.08f * wobble);
        m_bpmLabel->setColor(zones::lerp(zones::kPanic, ccWHITE, wobble));
    }
}

void BpmDisplay::beat() {
    m_heart->stopActionByTag(kBeatActionTag);
    m_heart->setScale(kHeartScale);
    auto action = CCSequence::create(
        CCEaseOut::create(CCScaleTo::create(0.07f, kHeartBeatScale), 2.f),
        CCEaseIn::create(CCScaleTo::create(0.16f, kHeartScale), 2.f),
        nullptr
    );
    action->setTag(kBeatActionTag);
    m_heart->runAction(action);
}

ccColor3B BpmDisplay::zoneColor(int bpm) const {
    if (!Config::get().zoneColors) return zones::kPanic;
    return zones::color(static_cast<float>(bpm));
}
