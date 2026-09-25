#include <Geode/Geode.hpp>
#include <Geode/modify/PauseLayer.hpp>

#include "HeartStyle.hpp"
#include "ui/HeartMapPopup.hpp"

using namespace geode::prelude;

class $modify(HRPauseLayer, PauseLayer) {
    void customSetup() {
        PauseLayer::customSetup();

        auto menu = this->getChildByID("right-button-menu");
        if (!menu) {
            log::warn("right-button-menu not found, Heart Map button not added");
            return;
        }

        auto icon = CCSprite::create(heart_style::current());
        icon->setColor({ 255, 55, 80 });
        auto sprite = CircleButtonSprite::create(icon, CircleBaseColor::Green, CircleBaseSize::Small);

        auto button = CCMenuItemExt::createSpriteExtra(sprite, [](auto) {
            HeartMapPopup::create(true)->show();
        });
        button->setID("heart-map-button"_spr);

        menu->addChild(button);
        menu->updateLayout();
    }
};
