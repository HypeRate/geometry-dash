#pragma once

#include <Geode/Geode.hpp>

// Thin colored strip under the level progress bar: every part of the level
// is colored by the average heart rate the player had there in all attempts.
class HeatStrip : public cocos2d::CCNode {
public:
    static HeatStrip* create(float width);

private:
    cocos2d::CCDrawNode* m_draw = nullptr;
    float m_width = 0.f;
    uint32_t m_drawnRevision = 0;
    uint32_t m_drawnConfigGen = 0;
    float m_timeSinceRedraw = 0.f;

    bool init(float width);
    void onEnter() override;
    void onExit() override;
    void tick(float dt);
    void redraw();
};
