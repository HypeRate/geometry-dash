#pragma once

#include <array>
#include <cstdint>
#include <string>

// Records the heart rate over the level progress.
//
// The level (0-100%) is split into buckets. For every frame we add the heart
// rate weighted by the frame time to the bucket at the current position, so a
// bucket's value is the average heart rate while the player was there.
struct HeartTrack {
    static constexpr int kBuckets = 200;  // 0.5% per bucket

    std::array<float, kBuckets> weightedSum{};
    std::array<float, kBuckets> time{};

    int coveredBuckets = 0;
    int peakBpm = 0;
    float peakPercent = 0.f;
    float endPercent = 0.f;
    float totalWeightedSum = 0.f;
    float totalTime = 0.f;

    void clear();
    void record(float percent, int bpm, float dt);
    void add(HeartTrack const& other);

    bool empty() const { return totalTime <= 0.f; }
    // Long enough to be worth showing (a quick death right after spawning
    // should not replace the attempt before it).
    bool meaningful() const { return coveredBuckets >= kMinBuckets; }
    static constexpr int kMinBuckets = 6;  // 3% of the level
    bool hasBucket(int i) const { return time[i] > 0.f; }
    float bucketAverage(int i) const;
    float average() const;
};

// Heart rate data of the level that is currently being played.
// There is only ever one PlayLayer, so a single global instance is enough.
//
// Every finished attempt is also saved to LevelStats (unless it was in
// practice mode or started from a start position).
class HeartMap {
public:
    static HeartMap& get();

    // Called when a level starts. Clears everything from the previous level.
    void beginLevel(std::string levelKey);
    // Called on every (re)spawn. Saves the finished attempt and keeps it as
    // "last attempt".
    void beginAttempt();
    // `persist`: whether this attempt may be saved to the level stats.
    void record(float percent, int bpm, float dt, bool persist);
    // The level was completed: saves the running attempt as a completion.
    void completeAttempt();
    // Leaving the level: saves the running attempt.
    void endLevel();

    // The attempt that is running right now (may be empty right after respawn).
    HeartTrack const& currentAttempt() const { return m_current; }
    // The most recent attempt worth showing: the running one once it got a bit
    // into the level, otherwise the last meaningful one before it.
    HeartTrack const& latestAttempt() const;
    // Heat of every saved attempt of this level plus the running one.
    HeartTrack allTime() const;
    std::string const& levelKey() const { return m_levelKey; }

    // Increases whenever new data was recorded, so views know when to redraw.
    uint32_t revision() const { return m_revision; }

private:
    std::string m_levelKey;
    HeartTrack m_current;
    HeartTrack m_previous;
    // The running attempt was already saved (completed or left).
    bool m_saved = false;
    bool m_persist = true;
    uint32_t m_revision = 0;

    void saveCurrent(bool completed);
};
