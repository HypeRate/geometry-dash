#pragma once

#include <Geode/Geode.hpp>

// Heart rate zone colors (calm -> tense -> panic), shared by the HUD, the
// progress bar heatmap and the Heart Map graph.
namespace zones {
    constexpr cocos2d::ccColor3B kCalm = { 80, 230, 120 };
    constexpr cocos2d::ccColor3B kTense = { 255, 205, 60 };
    constexpr cocos2d::ccColor3B kPanic = { 255, 55, 80 };

    cocos2d::ccColor3B lerp(cocos2d::ccColor3B a, cocos2d::ccColor3B b, float t);

    // Heart rate up to which it's "calm" and from which it's "panic": learned
    // by AutoZones, or the manual settings.
    float calmUpTo();
    float panicFrom();

    // Color for a heart rate, based on the zone limits.
    cocos2d::ccColor3B color(float bpm);

    cocos2d::ccColor4F toColor4F(cocos2d::ccColor3B color, float alpha = 1.f);
}
