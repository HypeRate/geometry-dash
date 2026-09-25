#include "HudEditor.hpp"
#include "BpmDisplay.hpp"
#include "Config.hpp"

using namespace geode::prelude;

namespace {
    constexpr float kScaleStep = 0.1f;
    constexpr float kMinScale = 0.3f;
    constexpr float kMaxScale = 3.f;

    constexpr int kMinOpacity = 30;
    constexpr int kMaxOpacity = 255;

    constexpr CCSize kToolbarSize = { 470.f, 50.f };
    constexpr float kToolbarY = 32.f;
    // Extra space around the display for the drag highlight.
    constexpr float kHighlightPadding = 6.f;
}

HudEditor* HudEditor::create() {
    auto ret = new HudEditor();
    if (ret->initEditor()) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

bool HudEditor::initEditor() {
    auto winSize = CCDirector::get()->getWinSize();
    if (!Popup::init(winSize.width, winSize.height)) return false;

    this->setID("hud-editor"_spr);
    // No popup frame, close button or bounce - this is a fullscreen overlay.
    m_bgSprite->setVisible(false);
    m_closeBtn->setVisible(false);
    m_noElasticity = true;
    this->setOpacity(60);

    // Show the level itself while editing: hide the pause menu and the real
    // HUD (the preview below replaces it).
    if (auto scene = CCDirector::get()->getRunningScene()) {
        this->hideWhileEditing(scene->getChildByType<PauseLayer>(0));
    }
    if (auto playLayer = PlayLayer::get()) {
        this->hideWhileEditing(playLayer->getChildByIDRecursive("bpm-display"_spr));
    }

    auto const& config = Config::get();
    m_scale = config.hudScale;
    m_opacity = config.hudOpacity;

    // Added to the layer itself (not m_mainLayer), so positions are in
    // screen coordinates.
    m_highlight = NineSlice::create("square02_small.png");
    m_highlight->setOpacity(90);
    m_highlight->setZOrder(9);
    this->addChild(m_highlight);

    m_preview = BpmDisplay::create(false);
    m_preview->setPosition({ config.hudX * winSize.width, config.hudY * winSize.height });
    m_preview->setZOrder(10);
    this->addChild(m_preview);

    auto hint = CCLabelBMFont::create("Drag the heart rate display anywhere", "goldFont.fnt");
    hint->setScale(0.6f);
    m_mainLayer->addChildAtPosition(hint, Anchor::Top, { 0.f, -22.f });

    // Toolbar: [-] 100% [+]   Opacity slider   Reset   Done
    auto toolbar = NineSlice::create("square02_001.png");
    toolbar->setColor({ 0, 0, 0 });
    toolbar->setOpacity(150);
    toolbar->setContentSize(kToolbarSize);
    m_mainLayer->addChildAtPosition(toolbar, Anchor::Bottom, { 0.f, kToolbarY });

    auto smaller = CCMenuItemExt::createSpriteExtraWithFrameName("GJ_zoomOutBtn_001.png", 0.55f, [this](auto) {
        this->setHudScale(m_scale - kScaleStep);
    });
    m_buttonMenu->addChildAtPosition(smaller, Anchor::Bottom, { -205.f, kToolbarY });

    m_scaleLabel = CCLabelBMFont::create("", "bigFont.fnt");
    m_mainLayer->addChildAtPosition(m_scaleLabel, Anchor::Bottom, { -167.f, kToolbarY });

    auto bigger = CCMenuItemExt::createSpriteExtraWithFrameName("GJ_zoomInBtn_001.png", 0.55f, [this](auto) {
        this->setHudScale(m_scale + kScaleStep);
    });
    m_buttonMenu->addChildAtPosition(bigger, Anchor::Bottom, { -129.f, kToolbarY });

    auto opacityLabel = CCLabelBMFont::create("Opacity", "goldFont.fnt");
    opacityLabel->setScale(0.45f);
    m_mainLayer->addChildAtPosition(opacityLabel, Anchor::Bottom, { -35.f, kToolbarY + 11.f });

    m_opacitySlider = Slider::create(this, menu_selector(HudEditor::onOpacitySlider), 0.55f);
    m_mainLayer->addChildAtPosition(m_opacitySlider, Anchor::Bottom, { -35.f, kToolbarY - 7.f });

    auto reset = CCMenuItemExt::createSpriteExtra(ButtonSprite::create("Reset", "goldFont.fnt", "GJ_button_04.png", 0.8f), [this](auto) {
        auto winSize = CCDirector::get()->getWinSize();
        m_preview->setPosition({ Config::kDefaultHudX * winSize.width, Config::kDefaultHudY * winSize.height });
        this->setHudScale(Config::kDefaultHudScale);
        this->setHudOpacity(Config::kDefaultHudOpacity);
    });
    reset->setScale(0.7f);
    reset->m_baseScale = 0.7f;
    m_buttonMenu->addChildAtPosition(reset, Anchor::Bottom, { 95.f, kToolbarY });

    auto done = CCMenuItemExt::createSpriteExtra(ButtonSprite::create("Done", "goldFont.fnt", "GJ_button_01.png", 0.8f), [this](auto sender) {
        this->onClose(sender);
    });
    done->setScale(0.7f);
    done->m_baseScale = 0.7f;
    m_buttonMenu->addChildAtPosition(done, Anchor::Bottom, { 180.f, kToolbarY });

    this->setHudScale(m_scale);
    this->setHudOpacity(m_opacity);
    return true;
}

void HudEditor::hideWhileEditing(CCNode* node) {
    if (!node || !node->isVisible()) return;
    node->setVisible(false);
    m_hiddenNodes.push_back(node);
}

void HudEditor::setHudScale(float scale) {
    // Round to avoid values like 1.0000001 from repeated steps.
    m_scale = std::round(std::clamp(scale, kMinScale, kMaxScale) * 100.f) / 100.f;
    m_preview->setScale(m_scale);
    m_scaleLabel->setString(fmt::format("{:.0f}%", m_scale * 100.f).c_str());
    m_scaleLabel->limitLabelWidth(40.f, 0.45f, 0.1f);
    this->updateHighlight();
}

void HudEditor::setHudOpacity(int opacity) {
    m_opacity = std::clamp(opacity, kMinOpacity, kMaxOpacity);
    m_preview->setPreviewOpacity(m_opacity);
    m_opacitySlider->setValue(static_cast<float>(m_opacity - kMinOpacity) / (kMaxOpacity - kMinOpacity));
}

void HudEditor::onOpacitySlider(CCObject* sender) {
    float value = static_cast<SliderThumb*>(sender)->getValue();
    m_opacity = kMinOpacity + static_cast<int>(std::round(value * (kMaxOpacity - kMinOpacity)));
    m_preview->setPreviewOpacity(m_opacity);
}

void HudEditor::updateHighlight() {
    auto size = m_preview->getScaledContentSize();
    m_highlight->setContentSize(size + CCSize{ kHighlightPadding * 2, kHighlightPadding * 2 });
    m_highlight->setPosition(m_preview->getPosition());
}

void HudEditor::save() {
    auto winSize = CCDirector::get()->getWinSize();
    auto pos = m_preview->getPosition();
    Config::saveHud(pos.x / winSize.width, pos.y / winSize.height, m_scale, m_opacity);
}

void HudEditor::onClose(CCObject* sender) {
    this->save();
    for (auto& node : m_hiddenNodes) {
        node->setVisible(true);
    }
    m_hiddenNodes.clear();
    Popup::onClose(sender);
}

bool HudEditor::ccTouchBegan(CCTouch* touch, CCEvent*) {
    // Menu buttons have a higher touch priority, so everything that reaches
    // this point is a drag.
    m_dragOffset = m_preview->getPosition() - touch->getLocation();
    return true;
}

void HudEditor::ccTouchMoved(CCTouch* touch, CCEvent*) {
    auto winSize = CCDirector::get()->getWinSize();
    auto pos = touch->getLocation() + m_dragOffset;
    pos.x = std::clamp(pos.x, 0.f, winSize.width);
    pos.y = std::clamp(pos.y, 0.f, winSize.height);
    m_preview->setPosition(pos);
    this->updateHighlight();
}

void HudEditor::ccTouchEnded(CCTouch*, CCEvent*) {}

void HudEditor::ccTouchCancelled(CCTouch*, CCEvent*) {}
