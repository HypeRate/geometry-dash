#pragma once

#include <Geode/Geode.hpp>
#include <Geode/loader/SettingV3.hpp>
#include <Geode/ui/NineSlice.hpp>

// Heart style picker: the setting stores a style id, the node shows all
// hearts as buttons with the selected one highlighted.
// Used in mod.json as "type": "custom:heart-style".
class HeartStyleSetting : public geode::SettingBaseValueV3<std::string> {
public:
    static geode::Result<std::shared_ptr<geode::SettingV3>> parse(
        std::string key, std::string modID, matjson::Value const& json
    );

    geode::Result<> isValid(std::string value) const override;
    geode::SettingNodeV3* createNode(float width) override;
};

class HeartStyleNode : public geode::SettingValueNodeV3<HeartStyleSetting> {
public:
    static HeartStyleNode* create(std::shared_ptr<HeartStyleSetting> setting, float width);

private:
    std::vector<std::pair<std::string, CCMenuItemSpriteExtra*>> m_buttons;
    geode::NineSlice* m_selection = nullptr;

    bool init(std::shared_ptr<HeartStyleSetting> setting, float width);
    void updateState(cocos2d::CCNode* invoker) override;
};
