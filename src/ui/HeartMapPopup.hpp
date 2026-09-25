#pragma once

#include <Geode/Geode.hpp>
#include <Geode/ui/Popup.hpp>

struct HeartTrack;

// Graph of the heart rate over the level progress: the latest attempt as a
// line, all attempts of the level as a heat band underneath.
class HeartGraph : public cocos2d::CCNode {
public:
    static HeartGraph* create(cocos2d::CCSize size, HeartTrack const& attempt, HeartTrack const& heat);

private:
    bool init(cocos2d::CCSize size, HeartTrack const& attempt, HeartTrack const& heat);
};

// "Heart Map" popup, opened from the pause menu and the level complete screen.
class HeartMapPopup : public geode::Popup {
public:
    // `showHudButton`: adds a button that opens the HUD editor (pause menu).
    static HeartMapPopup* create(bool showHudButton);

private:
    bool initPopup(bool showHudButton);
    void addSubtitle(HeartTrack const& attempt, bool isCurrentAttempt);
    void addBranding(bool nextToButton);
    void addTitleDecoration();
    void addCornerDecoration();
    cocos2d::CCNode* createLegend();
    // Rounded card with an icon on the left and title / value / detail text.
    void addStatCard(
        cocos2d::CCNode* icon, char const* title, std::string const& value,
        std::string const& detail, cocos2d::CCPoint position
    );
};
