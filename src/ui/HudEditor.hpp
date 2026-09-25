#pragma once

#include <Geode/Geode.hpp>
#include <Geode/ui/NineSlice.hpp>
#include <Geode/ui/Popup.hpp>

class BpmDisplay;

// Fullscreen overlay to drag the heart rate display into place and resize it.
// Opened from the pause menu, saves to the mod settings when closed.
// While it is open, the pause menu and the real HUD are hidden, so the display
// is placed over the actual level.
class HudEditor : public geode::Popup {
public:
    static HudEditor* create();

private:
    BpmDisplay* m_preview = nullptr;
    geode::NineSlice* m_highlight = nullptr;
    cocos2d::CCLabelBMFont* m_scaleLabel = nullptr;
    Slider* m_opacitySlider = nullptr;
    cocos2d::CCPoint m_dragOffset;
    float m_scale = 1.f;
    int m_opacity = 255;

    // Nodes hidden while editing, shown again on close.
    std::vector<geode::Ref<cocos2d::CCNode>> m_hiddenNodes;

    bool initEditor();
    void hideWhileEditing(cocos2d::CCNode* node);
    void setHudScale(float scale);
    void setHudOpacity(int opacity);
    void onOpacitySlider(cocos2d::CCObject* sender);
    void updateHighlight();
    void save();
    void onClose(cocos2d::CCObject* sender) override;

    bool ccTouchBegan(cocos2d::CCTouch* touch, cocos2d::CCEvent* event) override;
    void ccTouchMoved(cocos2d::CCTouch* touch, cocos2d::CCEvent* event) override;
    void ccTouchEnded(cocos2d::CCTouch* touch, cocos2d::CCEvent* event) override;
    void ccTouchCancelled(cocos2d::CCTouch* touch, cocos2d::CCEvent* event) override;
};
