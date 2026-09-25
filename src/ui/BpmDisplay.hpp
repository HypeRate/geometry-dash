#pragma once

#include <Geode/Geode.hpp>

// The heart rate HUD: a heart that beats at the real BPM plus the number.
//
// The node polls HypeRateClient and Config every frame, so it can simply be
// added anywhere and needs no further wiring.
class BpmDisplay : public cocos2d::CCNodeRGBA {
public:
    // `followConfigPosition`: place the node from the saved HUD settings.
    // The HUD editor turns this off and positions the node itself.
    static BpmDisplay* create(bool followConfigPosition = true);

    // Highlights the display when the heart rate gets close to the
    // Ice Cold limit.
    void setWarning(bool warning);

    void applyLayout();
    // HUD editor: show this opacity instead of the saved one (-1 = saved).
    void setPreviewOpacity(int opacity) { m_previewOpacity = opacity; }

private:
    cocos2d::CCSprite* m_heart = nullptr;
    cocos2d::CCLabelBMFont* m_bpmLabel = nullptr;
    cocos2d::CCLabelBMFont* m_unitLabel = nullptr;

    bool m_followConfigPosition = true;
    uint32_t m_configGen = 0;
    int m_shownBpm = -1;
    std::string m_heartStyle;
    // 0..1, fades the display out while there is no heart rate.
    float m_visibility = 1.f;
    int m_previewOpacity = -1;
    float m_beatPhase = 0.f;
    bool m_warning = false;
    float m_warningTime = 0.f;

    bool init(bool followConfigPosition);
    bool shouldBeVisible() const;
    void onEnter() override;
    void onExit() override;
    void update(float dt) override;
    void beat();
    cocos2d::ccColor3B zoneColor(int bpm) const;
};
