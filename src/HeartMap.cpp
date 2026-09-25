#include "HeartMap.hpp"
#include "LevelStats.hpp"

#include <algorithm>

void HeartTrack::clear() {
    *this = HeartTrack{};
}

void HeartTrack::record(float percent, int bpm, float dt) {
    if (bpm <= 0 || dt <= 0.f) return;

    percent = std::clamp(percent, 0.f, 100.f);
    int bucket = std::min(kBuckets - 1, static_cast<int>(percent / 100.f * kBuckets));

    if (time[bucket] <= 0.f) coveredBuckets++;
    weightedSum[bucket] += bpm * dt;
    time[bucket] += dt;
    totalWeightedSum += bpm * dt;
    totalTime += dt;
    endPercent = percent;

    if (bpm > peakBpm) {
        peakBpm = bpm;
        peakPercent = percent;
    }
}

void HeartTrack::add(HeartTrack const& other) {
    for (int i = 0; i < kBuckets; ++i) {
        if (time[i] <= 0.f && other.time[i] > 0.f) coveredBuckets++;
        weightedSum[i] += other.weightedSum[i];
        time[i] += other.time[i];
    }
    totalWeightedSum += other.totalWeightedSum;
    totalTime += other.totalTime;
    endPercent = std::max(endPercent, other.endPercent);
    if (other.peakBpm > peakBpm) {
        peakBpm = other.peakBpm;
        peakPercent = other.peakPercent;
    }
}

float HeartTrack::bucketAverage(int i) const {
    return time[i] > 0.f ? weightedSum[i] / time[i] : 0.f;
}

float HeartTrack::average() const {
    return totalTime > 0.f ? totalWeightedSum / totalTime : 0.f;
}

HeartMap& HeartMap::get() {
    static HeartMap instance;
    return instance;
}

void HeartMap::beginLevel(std::string levelKey) {
    m_levelKey = std::move(levelKey);
    m_current.clear();
    m_previous.clear();
    m_saved = false;
    m_revision++;
}

void HeartMap::beginAttempt() {
    if (!m_current.empty()) {
        this->saveCurrent(false);
        if (m_current.meaningful()) {
            m_previous = m_current;
        }
    }
    m_current.clear();
    m_saved = false;
    m_revision++;
}

void HeartMap::record(float percent, int bpm, float dt, bool persist) {
    if (m_saved) return;
    m_current.record(percent, bpm, dt);
    m_persist = persist;
    m_revision++;
}

void HeartMap::completeAttempt() {
    this->saveCurrent(true);
}

void HeartMap::endLevel() {
    this->saveCurrent(false);
}

void HeartMap::saveCurrent(bool completed) {
    if (m_saved || m_current.empty()) return;
    m_saved = true;
    if (m_persist) {
        LevelStats::get().addAttempt(m_levelKey, m_current, completed);
    }
    m_revision++;
}

HeartTrack HeartMap::allTime() const {
    auto track = LevelStats::get().find(m_levelKey).heat;
    // Saved attempts are already part of the level stats.
    if (!m_saved && m_persist) track.add(m_current);
    return track;
}

HeartTrack const& HeartMap::latestAttempt() const {
    return m_current.meaningful() || m_previous.empty() ? m_current : m_previous;
}
