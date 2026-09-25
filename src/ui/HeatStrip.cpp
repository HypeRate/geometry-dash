#include "HeatStrip.hpp"
#include "Config.hpp"
#include "HeartMap.hpp"
#include "Zones.hpp"

using namespace geode::prelude;

namespace {
    constexpr float kHeight = 3.f;
    // Redrawing 200 quads is cheap, but there is no need to do it every frame.
    constexpr float kRedrawInterval = 0.25f;
}

HeatStrip* HeatStrip::create(float width) {
    auto ret = new HeatStrip();
    if (ret->init(width)) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

bool HeatStrip::init(float width) {
    if (!CCNode::init()) return false;

    m_width = width;
    this->setID("heat-strip"_spr);
    this->setContentSize({ width, kHeight });

    m_draw = CCDrawNode::create();
    // Our colors are not premultiplied, so blend with the normal alpha.
    m_draw->setBlendFunc({ GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA });
    this->addChild(m_draw);

    this->redraw();
    return true;
}

void HeatStrip::onEnter() {
    CCNode::onEnter();
    CCScheduler::get()->scheduleSelector(schedule_selector(HeatStrip::tick), this, 0.f, false);
}

void HeatStrip::onExit() {
    CCScheduler::get()->unscheduleSelector(schedule_selector(HeatStrip::tick), this);
    CCNode::onExit();
}

void HeatStrip::tick(float dt) {
    m_timeSinceRedraw += dt;
    if (m_timeSinceRedraw < kRedrawInterval) return;

    auto const& map = HeartMap::get();
    auto const& config = Config::get();
    if (map.revision() == m_drawnRevision && config.generation == m_drawnConfigGen) return;

    this->redraw();
}

void HeatStrip::redraw() {
    auto const& map = HeartMap::get();
    m_drawnRevision = map.revision();
    m_drawnConfigGen = Config::get().generation;
    m_timeSinceRedraw = 0.f;

    m_draw->clear();

    auto track = map.allTime();
    float bucketWidth = m_width / HeartTrack::kBuckets;
    for (int i = 0; i < HeartTrack::kBuckets; ++i) {
        if (!track.hasBucket(i)) continue;
        auto color = zones::toColor4F(zones::color(track.bucketAverage(i)), 0.95f);
        m_draw->drawRect(
            CCPoint{ i * bucketWidth, 0.f },
            CCPoint{ (i + 1) * bucketWidth, kHeight },
            color, 0.f, color
        );
    }
}
