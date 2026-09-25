#pragma once

#include <Geode/Geode.hpp>
#include <span>
#include <string_view>

// The selectable heart sprites. All of them are grayscale with a black
// outline, so tinting them with a zone color keeps the outline black.
namespace heart_style {
    // Selected on first use.
    constexpr char const* kDefault = "pixel";

    struct Style {
        char const* id;    // value stored in the settings
        char const* name;  // shown in the picker
        char const* file;  // sprite file
    };

    std::span<Style const> all();
    bool isValid(std::string_view id);

    // Sprite file of a style (falls back to the default style).
    char const* file(std::string_view id);
    // Sprite file of the style selected in the settings.
    char const* current();

    // Swaps the texture of an existing sprite to the given style.
    void apply(cocos2d::CCSprite* sprite, std::string_view id);
}
