#pragma once

#include <Geode/Geode.hpp>
#include <Geode/loader/SettingV3.hpp>

// Read-only row that shows what Automatic Zones has learned, with a button to
// start learning again. Used in mod.json as "type": "custom:zone-info".
class ZoneInfoSetting : public geode::SettingV3 {
public:
    static geode::Result<std::shared_ptr<geode::SettingV3>> parse(
        std::string key, std::string modID, matjson::Value const& json
    );

    bool load(matjson::Value const&) override { return true; }
    bool save(matjson::Value&) const override { return true; }
    bool isDefaultValue() const override { return true; }
    void reset() override {}

    geode::SettingNodeV3* createNode(float width) override;
};

class ZoneInfoNode : public geode::SettingNodeV3 {
public:
    static ZoneInfoNode* create(std::shared_ptr<ZoneInfoSetting> setting, float width);

private:
    cocos2d::CCLabelBMFont* m_label = nullptr;
    CCMenuItemSpriteExtra* m_relearnButton = nullptr;
    std::string m_shownText;
    float m_width = 0.f;

    bool init(std::shared_ptr<ZoneInfoSetting> setting, float width);
    void onEnter() override;
    void onExit() override;
    void tick(float dt);

    void onCommit() override {}
    void onResetToDefault() override {}
    bool hasUncommittedChanges() const override { return false; }
    bool hasNonDefaultValue() const override { return false; }
};
