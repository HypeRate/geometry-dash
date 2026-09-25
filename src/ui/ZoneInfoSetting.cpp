#include "ZoneInfoSetting.hpp"
#include "AutoZones.hpp"
#include "Config.hpp"
#include "Zones.hpp"

using namespace geode::prelude;

namespace {
    constexpr ccColor3B kTextColor = { 220, 220, 220 };
    constexpr float kPadding = 12.f;
    constexpr float kButtonScale = 0.5f;
}

$execute {
    // Custom setting types have to be registered before the settings load.
    (void)Mod::get()->registerCustomSettingType("zone-info", &ZoneInfoSetting::parse);
}

Result<std::shared_ptr<SettingV3>> ZoneInfoSetting::parse(
    std::string key, std::string modID, matjson::Value const& json
) {
    auto ret = std::make_shared<ZoneInfoSetting>();
    auto root = checkJson(json, "ZoneInfoSetting");
    ret->parseBaseProperties(std::move(key), std::move(modID), root);
    root.checkUnknownKeys();
    return root.ok(std::static_pointer_cast<SettingV3>(ret));
}

SettingNodeV3* ZoneInfoSetting::createNode(float width) {
    return ZoneInfoNode::create(std::static_pointer_cast<ZoneInfoSetting>(shared_from_this()), width);
}

ZoneInfoNode* ZoneInfoNode::create(std::shared_ptr<ZoneInfoSetting> setting, float width) {
    auto ret = new ZoneInfoNode();
    if (ret->init(std::move(setting), width)) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

bool ZoneInfoNode::init(std::shared_ptr<ZoneInfoSetting> setting, float width) {
    if (!SettingNodeV3::init(setting, width)) return false;
    m_width = width;
    float centerY = this->getContentHeight() / 2;

    auto menu = CCMenu::create();
    menu->setPosition({ 0.f, 0.f });
    this->addChild(menu);

    auto sprite = ButtonSprite::create("Relearn", "bigFont.fnt", "GJ_button_04.png", 0.8f);
    sprite->setScale(kButtonScale);
    m_relearnButton = CCMenuItemExt::createSpriteExtra(sprite, [](auto) {
        AutoZones::get().reset();
        Config::reload();
    });
    m_relearnButton->setID("relearn-button"_spr);
    m_relearnButton->setPosition({ width - kPadding - m_relearnButton->getScaledContentWidth() / 2, centerY });
    menu->addChild(m_relearnButton);

    m_label = CCLabelBMFont::create("", "bigFont.fnt");
    m_label->setAnchorPoint({ 1.f, .5f });
    m_label->setColor(kTextColor);
    this->addChild(m_label);

    this->tick(0.f);
    return true;
}

void ZoneInfoNode::onEnter() {
    SettingNodeV3::onEnter();
    CCScheduler::get()->scheduleSelector(schedule_selector(ZoneInfoNode::tick), this, 0.5f, false);
}

void ZoneInfoNode::onExit() {
    CCScheduler::get()->unscheduleSelector(schedule_selector(ZoneInfoNode::tick), this);
    SettingNodeV3::onExit();
}

void ZoneInfoNode::tick(float) {
    auto const& config = Config::get();
    auto const& autoZones = AutoZones::get();

    std::string text;
    if (!config.autoZones) {
        text = "Using your own limits below";
    }
    else if (!autoZones.ready()) {
        text = fmt::format("Learning... {:.0f}% - just keep playing", autoZones.progress() * 100.f);
    }
    else {
        text = fmt::format(
            "Normal {:.0f} - calm to {:.0f} - panic from {:.0f}",
            autoZones.normal(), autoZones.calmUpTo(), autoZones.panicFrom()
        );
    }

    m_relearnButton->setVisible(config.autoZones);
    if (text == m_shownText) return;
    m_shownText = text;

    float right = config.autoZones
        ? m_relearnButton->getPositionX() - m_relearnButton->getScaledContentWidth() / 2 - 8.f
        : m_width - kPadding;
    m_label->setString(m_shownText.c_str());
    // Leave room for the name on the left.
    m_label->limitLabelWidth(right - m_width * 0.3f, 0.32f, 0.1f);
    m_label->setPosition({ right, this->getContentHeight() / 2 });
}
