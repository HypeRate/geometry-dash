#include "Config.hpp"
#include "ui/HeartStyleSetting.hpp"

using namespace geode::prelude;

static Config s_config;

Config const& Config::get() {
    return s_config;
}

void Config::reload() {
    auto mod = Mod::get();
    auto& c = s_config;

    c.hyperateId = string::trim(mod->getSettingValue<std::string>("hyperate-id"));
    c.demoMode = mod->getSettingValue<bool>("demo-mode");

    c.showDisplay = mod->getSettingValue<bool>("show-display");
    c.showInPractice = mod->getSettingValue<bool>("show-in-practice");
    c.hideWithoutSignal = mod->getSettingValue<bool>("hide-without-signal");
    // Custom setting type, so it can't be read with getSettingValue.
    if (auto style = std::static_pointer_cast<HeartStyleSetting>(mod->getSetting("heart-style"))) {
        c.heartStyle = style->getValue();
    }
    c.hudX = static_cast<float>(mod->getSavedValue<double>("hud-x", kDefaultHudX));
    c.hudY = static_cast<float>(mod->getSavedValue<double>("hud-y", kDefaultHudY));
    c.hudScale = static_cast<float>(mod->getSavedValue<double>("hud-scale", kDefaultHudScale));
    c.hudOpacity = static_cast<int>(mod->getSavedValue<int64_t>("hud-opacity", kDefaultHudOpacity));
    c.pulseAnimation = mod->getSettingValue<bool>("pulse-animation");
    c.zoneColors = mod->getSettingValue<bool>("zone-colors");
    c.autoZones = mod->getSettingValue<bool>("auto-zones");
    c.zoneCalm = static_cast<int>(mod->getSettingValue<int64_t>("zone-calm"));
    c.zonePanic = static_cast<int>(mod->getSettingValue<int64_t>("zone-panic"));

    c.heatStrip = mod->getSettingValue<bool>("heat-strip");
    c.heartMapOnComplete = mod->getSettingValue<bool>("heart-map-on-complete");

    c.deathMode = mod->getSettingValue<bool>("death-mode");
    c.deathBpm = static_cast<int>(mod->getSettingValue<int64_t>("death-bpm"));
    c.deathInPractice = mod->getSettingValue<bool>("death-in-practice");

    c.generation++;
}

void Config::saveHud(float x, float y, float scale, int opacity) {
    auto mod = Mod::get();
    mod->setSavedValue<double>("hud-x", std::clamp(x, 0.f, 1.f));
    mod->setSavedValue<double>("hud-y", std::clamp(y, 0.f, 1.f));
    mod->setSavedValue<double>("hud-scale", std::clamp(scale, 0.3f, 3.f));
    mod->setSavedValue<int64_t>("hud-opacity", std::clamp(opacity, 30, 255));
    // Saved values don't fire setting change events, so reload manually.
    reload();
}
