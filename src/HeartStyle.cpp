#include "HeartStyle.hpp"
#include "Config.hpp"

using namespace geode::prelude;

namespace heart_style {
    static constexpr Style kStyles[] = {
        { "geometry", "Geometry", "heart_geometry.png"_spr },
        { "pixel", "Pixel", "heart_pixel.png"_spr },
        { "crystal", "Crystal", "heart_crystal.png"_spr },
        { "smooth", "Smooth", "heart_smooth.png"_spr },
    };

    std::span<Style const> all() {
        return kStyles;
    }

    bool isValid(std::string_view id) {
        for (auto const& style : kStyles) {
            if (id == style.id) return true;
        }
        return false;
    }

    char const* file(std::string_view id) {
        for (auto const& style : kStyles) {
            if (id == style.id) return style.file;
        }
        return kStyles[0].file;
    }

    char const* current() {
        return file(Config::get().heartStyle);
    }

    void apply(CCSprite* sprite, std::string_view id) {
        auto texture = CCTextureCache::get()->addImage(file(id), false);
        if (!sprite || !texture) return;
        sprite->setTexture(texture);
        sprite->setTextureRect({ CCPointZero, texture->getContentSize() });
    }
}
