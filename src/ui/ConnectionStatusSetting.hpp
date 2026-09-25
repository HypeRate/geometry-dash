#pragma once

#include <Geode/Geode.hpp>
#include <Geode/loader/SettingV3.hpp>

// Read-only "setting" that shows the live connection status and heart rate
// directly in the mod settings, right below the HypeRate ID, together with a
// Connect / Disconnect button.
// Used in mod.json as "type": "custom:connection-status".
class ConnectionStatusSetting : public geode::SettingV3 {
public:
    static geode::Result<std::shared_ptr<geode::SettingV3>> parse(
        std::string key, std::string modID, matjson::Value const& json
    );

    // Nothing to store - this setting has no value.
    bool load(matjson::Value const&) override { return true; }
    bool save(matjson::Value&) const override { return true; }
    bool isDefaultValue() const override { return true; }
    void reset() override {}

    geode::SettingNodeV3* createNode(float width) override;
};

class ConnectionStatusNode : public geode::SettingNodeV3 {
public:
    static ConnectionStatusNode* create(std::shared_ptr<ConnectionStatusSetting> setting, float width);

private:
    cocos2d::CCSprite* m_heart = nullptr;
    cocos2d::CCLabelBMFont* m_label = nullptr;
    cocos2d::CCMenu* m_menu = nullptr;
    CCMenuItemSpriteExtra* m_button = nullptr;
    std::string m_shownText;
    std::string m_heartStyle;
    // Whether the button currently says "Disconnect" (true) or "Connect".
    bool m_buttonDisconnects = false;
    float m_width = 0.f;
    float m_beatPhase = 0.f;

    bool init(std::shared_ptr<ConnectionStatusSetting> setting, float width);
    void onEnter() override;
    void onExit() override;
    void tick(float dt);

    void updateButton(bool disconnects);
    void layout();
    void onToggleConnection();

    void onCommit() override {}
    void onResetToDefault() override {}
    bool hasUncommittedChanges() const override { return false; }
    bool hasNonDefaultValue() const override { return false; }
};
