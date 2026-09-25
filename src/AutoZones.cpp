#include "AutoZones.hpp"

#include <Geode/Geode.hpp>
#include <algorithm>
#include <cmath>

using namespace geode::prelude;

namespace {
    // HypeRate sends roughly one update every 5 seconds: 24 samples = ~2 min.
    constexpr int kReadySamples = 24;
    // Moving average over roughly the last 60 samples (~5 minutes of play).
    constexpr int kWindow = 60;
    constexpr int kSaveEvery = 6;

    // Mean absolute deviation -> standard deviation (normal distribution).
    constexpr float kDeviationToSigma = 1.25f;
    constexpr float kMinCalmOffset = 3.f;
    constexpr float kMinPanicOffset = 12.f;
}

AutoZones& AutoZones::get() {
    static AutoZones instance;
    instance.load();
    return instance;
}

void AutoZones::addSample(int bpm) {
    if (bpm <= 0) return;

    // Plain average while there are few samples, moving average afterwards.
    float alpha = 1.f / static_cast<float>(std::min(m_samples + 1, kWindow));
    if (m_samples == 0) {
        m_mean = static_cast<float>(bpm);
    }
    else {
        m_deviation += (std::abs(bpm - m_mean) - m_deviation) * alpha;
        m_mean += (bpm - m_mean) * alpha;
    }
    m_samples++;

    if (m_samples % kSaveEvery == 0) this->save();
}

void AutoZones::reset() {
    m_mean = 0.f;
    m_deviation = 0.f;
    m_samples = 0;
    this->save();
}

bool AutoZones::ready() const {
    return m_samples >= kReadySamples;
}

float AutoZones::progress() const {
    return std::min(1.f, static_cast<float>(m_samples) / kReadySamples);
}

float AutoZones::calmUpTo() const {
    float sigma = m_deviation * kDeviationToSigma;
    return m_mean + std::max(kMinCalmOffset, 0.5f * sigma);
}

float AutoZones::panicFrom() const {
    float sigma = m_deviation * kDeviationToSigma;
    return m_mean + std::max(kMinPanicOffset, 3.f * sigma);
}

void AutoZones::load() {
    if (m_loaded) return;
    m_loaded = true;
    auto mod = Mod::get();
    m_mean = static_cast<float>(mod->getSavedValue<double>("auto-zones-mean", 0.0));
    m_deviation = static_cast<float>(mod->getSavedValue<double>("auto-zones-deviation", 0.0));
    m_samples = static_cast<int>(mod->getSavedValue<int64_t>("auto-zones-samples", 0));
}

void AutoZones::save() {
    auto mod = Mod::get();
    mod->setSavedValue<double>("auto-zones-mean", m_mean);
    mod->setSavedValue<double>("auto-zones-deviation", m_deviation);
    // The count only matters until the zones are ready, keep it small.
    mod->setSavedValue<int64_t>("auto-zones-samples", std::min(m_samples, kWindow * 10));
}
