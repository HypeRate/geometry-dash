#include <Geode/Geode.hpp>
#include <Geode/modify/EndLevelLayer.hpp>

#include "Config.hpp"
#include "HeartMap.hpp"
#include "HeartStyle.hpp"
#include "ui/HeartMapPopup.hpp"

using namespace geode::prelude;

class $modify(HREndLevelLayer, EndLevelLayer) {
    void customSetup() {
        EndLevelLayer::customSetup();

        // Nothing to show without any recorded heart rate.
        auto const& map = HeartMap::get();
        if (map.allTime().empty() && map.latestAttempt().empty()) return;

        // Column menu on the left that node IDs prepares for mod buttons.
        auto menu = this->getChildByID("hide-layer-menu");
        if (!menu) {
            log::warn("hide-layer-menu not found, Heart Map button not added");
            return;
        }

        auto icon = CCSprite::create(heart_style::current());
        icon->setColor({ 255, 55, 80 });
        auto sprite = CircleButtonSprite::create(icon, CircleBaseColor::Green, CircleBaseSize::Small);
        sprite->setScale(0.7f);

        auto button = CCMenuItemExt::createSpriteExtra(sprite, [](auto) {
            HeartMapPopup::create(false)->show();
        });
        button->setID("heart-map-button"_spr);

        menu->addChild(button);
        menu->updateLayout();

        if (Config::get().heartMapOnComplete) {
            // Wait for the level complete animation before opening it.
            this->runAction(CCSequence::create(
                CCDelayTime::create(1.2f),
                CallFuncExt::create([] { HeartMapPopup::create(false)->show(); }),
                nullptr
            ));
        }
    }
};
