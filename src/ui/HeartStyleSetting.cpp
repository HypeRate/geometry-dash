#include "HeartStyleSetting.hpp"
#include "HeartStyle.hpp"
#include "Zones.hpp"

using namespace geode::prelude;

namespace {
    constexpr float kRowHeight = 40.f;
    constexpr float kButtonSpacing = 34.f;
    constexpr float kHeartScale = 0.75f;
    constexpr GLubyte kDimmedOpacity = 110;
}

$execute {
    // Custom setting types have to be registered before the settings load.
    (void)Mod::get()->registerCustomSettingType("heart-style", &HeartStyleSetting::parse);
}

Result<std::shared_ptr<SettingV3>> HeartStyleSetting::parse(
    std::string key, std::string modID, matjson::Value const& json
) {
    auto ret = std::make_shared<HeartStyleSetting>();
    auto root = checkJson(json, "HeartStyleSetting");
    ret->parseBaseProperties(std::move(key), std::move(modID), root);
    root.checkUnknownKeys();
    return root.ok(std::static_pointer_cast<SettingV3>(ret));
}

Result<> HeartStyleSetting::isValid(std::string value) const {
    if (heart_style::isValid(value)) return Ok();
    return Err("Unknown heart style");
}

SettingNodeV3* HeartStyleSetting::createNode(float width) {
    return HeartStyleNode::create(
        std::static_pointer_cast<HeartStyleSetting>(shared_from_this()), width
    );
}

HeartStyleNode* HeartStyleNode::create(std::shared_ptr<HeartStyleSetting> setting, float width) {
    auto ret = new HeartStyleNode();
    if (ret->init(std::move(setting), width)) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

bool HeartStyleNode::init(std::shared_ptr<HeartStyleSetting> setting, float width) {
    if (!SettingValueNodeV3::init(setting, width)) return false;

    this->setContentHeight(kRowHeight);

    m_selection = NineSlice::create("square02b_small.png");
    m_selection->setColor({ 255, 200, 40 });
    m_selection->setOpacity(120);
    m_selection->setContentSize({ 32.f, 32.f });
    this->addChild(m_selection);

    auto menu = CCMenu::create();
    menu->setPosition({ 0.f, 0.f });
    this->addChild(menu);

    // Right-aligned row of hearts, in the order of the style list.
    auto styles = heart_style::all();
    float x = width - 22.f - kButtonSpacing * (styles.size() - 1);
    for (auto const& style : styles) {
        auto heart = CCSprite::create(style.file);
        heart->setColor(zones::kPanic);
        heart->setScale(kHeartScale);

        std::string id = style.id;
        auto button = CCMenuItemExt::createSpriteExtra(heart, [this, id](auto sender) {
            this->setValue(id, sender);
        });
        button->setID(fmt::format("heart-{}", id));
        button->setPosition({ x, kRowHeight / 2 });
        menu->addChild(button);
        m_buttons.push_back({ id, button });

        x += kButtonSpacing;
    }

    this->updateState(nullptr);
    return true;
}

void HeartStyleNode::updateState(CCNode* invoker) {
    SettingValueNodeV3::updateState(invoker);

    auto selected = this->getValue();
    for (auto const& [id, button] : m_buttons) {
        bool isSelected = id == selected;
        static_cast<CCSprite*>(button->getNormalImage())->setOpacity(isSelected ? 255 : kDimmedOpacity);
        if (isSelected) {
            m_selection->setPosition(button->getPosition());
        }
    }
}
