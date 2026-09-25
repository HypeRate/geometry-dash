#pragma once

#include <Geode/Geode.hpp>

// Cached copy of the mod settings.
//
// Settings are read every frame by the HUD and the challenge mode, so we keep
// a plain struct instead of looking them up each time. `generation` increases
// whenever any setting changes, which lets nodes cheaply check if they need to
// re-apply their layout.
struct Config {
    std::string hyperateId;
    bool demoMode = false;

    bool showDisplay = true;
    bool showInPractice = true;
    bool hideWithoutSignal = true;
    std::string heartStyle = "geometry";
    float hudX = 0.08f;
    float hudY = 0.88f;
    float hudScale = 1.f;
    int hudOpacity = 255;
    bool pulseAnimation = true;
    bool zoneColors = true;
    bool autoZones = true;
    int zoneCalm = 90;
    int zonePanic = 140;

    bool heatStrip = true;
    bool heartMapOnComplete = false;

    bool deathMode = false;
    int deathBpm = 130;
    bool deathInPractice = false;

    uint32_t generation = 0;

    static Config const& get();
    static void reload();

    // Persists the HUD layout (used by the HUD editor). The layout is stored
    // as saved values, not settings - it is only edited in the HUD editor.
    static void saveHud(float x, float y, float scale, int opacity);
};
