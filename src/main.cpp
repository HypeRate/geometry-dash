#include <Geode/Geode.hpp>
#include <Geode/loader/SettingV3.hpp>

#include "Config.hpp"
#include "HypeRateClient.hpp"

using namespace geode::prelude;

namespace {
    constexpr auto kDiscordInvite = "https://discord.gg/wQZu5HunUF";
}

$on_mod(Loaded) {
    Config::reload();

    listenForAllSettingChanges([](std::string_view, std::shared_ptr<SettingV3>) {
        Config::reload();
        HypeRateClient::get()->applyConfig();
    });

    ButtonSettingPressedEventV3(Mod::get(), "community-actions").listen([](std::string_view button) {
        // Only opened when the player presses the button - never automatically.
        if (button == "discord") {
            web::openLinkInBrowser(kDiscordInvite);
        }
    }).leak();

    HypeRateClient::get()->start();
}
