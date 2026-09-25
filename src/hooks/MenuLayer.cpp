#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>

#include "UpdateCheck.hpp"

using namespace geode::prelude;

class $modify(HRMenuLayer, MenuLayer) {
    bool init() {
        if (!MenuLayer::init()) return false;

        // The update check runs in the background after the game starts, so
        // keep looking for a few seconds in case the answer isn't there yet.
        this->runAction(CCRepeat::create(CCSequence::create(
            CCDelayTime::create(1.f),
            CallFuncExt::create([] {
                auto update = update_check::takePendingUpdate();
                if (!update) return;

                createQuickPopup(
                    "HypeRate Update",
                    fmt::format(
                        "Version <cg>{}</c> of the HypeRate mod is available.\n"
                        "Do you want to open the download page?",
                        *update
                    ),
                    "Later", "Download",
                    [](auto, bool download) {
                        if (download) web::openLinkInBrowser(update_check::kReleasesPage);
                    }
                );
            }),
            nullptr
        ), 10));
        return true;
    }
};
