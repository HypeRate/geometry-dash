#include "ConnectionStatusSetting.hpp"
#include "Config.hpp"
#include "HeartStyle.hpp"
#include "HypeRateClient.hpp"

using namespace geode::prelude;

namespace {
    constexpr ccColor3B kGreen = { 80, 230, 120 };
    constexpr ccColor3B kYellow = { 255, 205, 60 };
    constexpr ccColor3B kRed = { 255, 70, 90 };
    constexpr ccColor3B kGray = { 150, 150, 150 };

    constexpr float kHeartScale = 0.5f;
    constexpr float kHeartBeatScale = 0.6f;
    constexpr float kButtonScale = 0.55f;
    constexpr float kPadding = 12.f;

    ccColor3B statusColor(ConnectionStatus status) {
        switch (status) {
            case ConnectionStatus::Live:
            case ConnectionStatus::Demo:
                return kGreen;
            case ConnectionStatus::Connecting:
            case ConnectionStatus::Waiting:
            case ConnectionStatus::NoSignal:
                return kYellow;
            case ConnectionStatus::NoId:
            case ConnectionStatus::Disconnected:
                return kGray;
            default:
                return kRed;
        }
    }

    // States in which the button offers to stop the connection.
    bool isConnectionActive(ConnectionStatus status) {
        switch (status) {
            case ConnectionStatus::Connecting:
            case ConnectionStatus::Waiting:
            case ConnectionStatus::Live:
            case ConnectionStatus::NoSignal:
            case ConnectionStatus::Error:
                return true;
            default:
                return false;
        }
    }
}

$execute {
    // Custom setting types have to be registered before the settings load.
    (void)Mod::get()->registerCustomSettingType("connection-status", &ConnectionStatusSetting::parse);
}

Result<std::shared_ptr<SettingV3>> ConnectionStatusSetting::parse(
    std::string key, std::string modID, matjson::Value const& json
) {
    auto ret = std::make_shared<ConnectionStatusSetting>();
    auto root = checkJson(json, "ConnectionStatusSetting");
    ret->parseBaseProperties(std::move(key), std::move(modID), root);
    root.checkUnknownKeys();
    return root.ok(std::static_pointer_cast<SettingV3>(ret));
}

SettingNodeV3* ConnectionStatusSetting::createNode(float width) {
    return ConnectionStatusNode::create(
        std::static_pointer_cast<ConnectionStatusSetting>(shared_from_this()), width
    );
}

ConnectionStatusNode* ConnectionStatusNode::create(std::shared_ptr<ConnectionStatusSetting> setting, float width) {
    auto ret = new ConnectionStatusNode();
    if (ret->init(std::move(setting), width)) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

bool ConnectionStatusNode::init(std::shared_ptr<ConnectionStatusSetting> setting, float width) {
    if (!SettingNodeV3::init(setting, width)) return false;

    m_width = width;

    // Right to left: [button] [status text / "104 BPM"] [heart]
    m_menu = CCMenu::create();
    m_menu->setPosition({ 0.f, 0.f });
    this->addChild(m_menu);

    m_label = CCLabelBMFont::create("", "bigFont.fnt");
    m_label->setAnchorPoint({ 1.f, .5f });
    this->addChild(m_label);

    m_heartStyle = Config::get().heartStyle;
    m_heart = CCSprite::create(heart_style::file(m_heartStyle));
    m_heart->setScale(kHeartScale);
    this->addChild(m_heart);

    this->updateButton(isConnectionActive(HypeRateClient::get()->status()));
    this->tick(0.f);
    return true;
}

// The row updates from the global scheduler instead of scheduleUpdate(), so it
// keeps running no matter how the settings popup handles its children.
void ConnectionStatusNode::onEnter() {
    SettingNodeV3::onEnter();
    CCScheduler::get()->scheduleSelector(
        schedule_selector(ConnectionStatusNode::tick), this, 0.f, false
    );
}

void ConnectionStatusNode::onExit() {
    CCScheduler::get()->unscheduleSelector(schedule_selector(ConnectionStatusNode::tick), this);
    SettingNodeV3::onExit();
}

void ConnectionStatusNode::updateButton(bool disconnects) {
    if (m_button && m_buttonDisconnects == disconnects) return;
    m_buttonDisconnects = disconnects;

    if (m_button) {
        m_button->removeFromParent();
    }
    auto sprite = ButtonSprite::create(
        disconnects ? "Disconnect" : "Connect", "bigFont.fnt",
        disconnects ? "GJ_button_06.png" : "GJ_button_01.png", 0.8f
    );
    sprite->setScale(kButtonScale);
    m_button = CCMenuItemExt::createSpriteExtra(sprite, [this](auto) {
        this->onToggleConnection();
    });
    m_button->setID("connection-button"_spr);
    m_menu->addChild(m_button);
    this->layout();
}

void ConnectionStatusNode::layout() {
    float centerY = this->getContentHeight() / 2;
    float x = m_width - kPadding;

    if (m_button) {
        float buttonWidth = m_button->getScaledContentWidth();
        m_button->setPosition({ x - buttonWidth / 2, centerY });
        x -= buttonWidth + 8.f;
    }

    // Leave room for the "Status" name on the left.
    float maxLabelWidth = std::max(40.f, x - m_width * 0.35f - 20.f);
    bool showsBpm = HypeRateClient::get()->bpm() > 0;
    m_label->limitLabelWidth(maxLabelWidth, showsBpm ? 0.5f : 0.3f, 0.1f);
    m_label->setPosition({ x, centerY });
    x -= m_label->getScaledContentWidth() + 12.f;

    m_heart->setPosition({ x, centerY });
}

void ConnectionStatusNode::onToggleConnection() {
    auto client = HypeRateClient::get();
    if (isConnectionActive(client->status())) {
        client->disconnectByUser();
    }
    else {
        client->reconnect();
    }
    this->tick(0.f);
}

void ConnectionStatusNode::tick(float dt) {
    auto client = HypeRateClient::get();
    auto status = client->status();
    int bpm = client->bpm();

    this->updateButton(isConnectionActive(status));
    // Demo mode and a missing ID have nothing to connect or disconnect.
    m_button->setVisible(status != ConnectionStatus::Demo && status != ConnectionStatus::NoId);

    std::string text;
    if (bpm > 0) {
        text = fmt::format("{} BPM", bpm);
    }
    else if (status == ConnectionStatus::NoId) {
        text = "Enter your HypeRate ID above";
    }
    else {
        text = client->statusText();
    }

    if (text != m_shownText) {
        m_shownText = text;
        m_label->setString(m_shownText.c_str());
        this->layout();
    }

    if (Config::get().heartStyle != m_heartStyle) {
        m_heartStyle = Config::get().heartStyle;
        heart_style::apply(m_heart, m_heartStyle);
    }

    auto color = statusColor(status);
    m_label->setColor(color);
    m_heart->setColor(color);

    // Beat along with the live heart rate.
    if (bpm > 0) {
        m_beatPhase += dt * (bpm / 60.f);
        if (m_beatPhase >= 1.f) {
            m_beatPhase -= std::floor(m_beatPhase);
            m_heart->stopAllActions();
            m_heart->setScale(kHeartScale);
            m_heart->runAction(CCSequence::create(
                CCEaseOut::create(CCScaleTo::create(0.07f, kHeartBeatScale), 2.f),
                CCEaseIn::create(CCScaleTo::create(0.16f, kHeartScale), 2.f),
                nullptr
            ));
        }
    }
}
