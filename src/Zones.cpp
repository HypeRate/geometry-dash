#include "Zones.hpp"
#include "AutoZones.hpp"
#include "Config.hpp"

using namespace geode::prelude;

namespace zones {
    ccColor3B lerp(ccColor3B a, ccColor3B b, float t) {
        t = std::clamp(t, 0.f, 1.f);
        return {
            static_cast<GLubyte>(a.r + (b.r - a.r) * t),
            static_cast<GLubyte>(a.g + (b.g - a.g) * t),
            static_cast<GLubyte>(a.b + (b.b - a.b) * t),
        };
    }

    static bool useAutoZones() {
        return Config::get().autoZones && AutoZones::get().ready();
    }

    float calmUpTo() {
        if (useAutoZones()) return AutoZones::get().calmUpTo();
        return static_cast<float>(Config::get().zoneCalm);
    }

    float panicFrom() {
        if (useAutoZones()) return AutoZones::get().panicFrom();
        auto const& config = Config::get();
        return static_cast<float>(std::max(config.zonePanic, config.zoneCalm + 1));
    }

    ccColor3B color(float bpm) {
        float calm = calmUpTo();
        float panic = std::max(panicFrom(), calm + 1.f);
        float mid = (calm + panic) / 2.f;

        if (bpm <= calm) return kCalm;
        if (bpm <= mid) return lerp(kCalm, kTense, (bpm - calm) / (mid - calm));
        return lerp(kTense, kPanic, (bpm - mid) / (panic - mid));
    }

    ccColor4F toColor4F(ccColor3B color, float alpha) {
        return { color.r / 255.f, color.g / 255.f, color.b / 255.f, alpha };
    }
}
