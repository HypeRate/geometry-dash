#pragma once

#include "HeartMap.hpp"

#include <string>
#include <unordered_map>

class GJGameLevel;

// All-time heart rate stats of one level.
struct LevelRecord {
    int attempts = 0;
    int completions = 0;
    // Furthest progress of any attempt (100 once completed).
    float bestPercent = 0.f;
    // Lowest average heart rate of a completed attempt, 0 = never completed.
    float calmestClear = 0.f;
    // Heat of all attempts combined; also holds the all-time peak.
    HeartTrack heat;
};

// Stores LevelRecords per level in "level-stats.json" in the mod's save
// folder. Only local - nothing leaves the player's device.
class LevelStats {
public:
    static LevelStats& get();

    // Key for a level: its ID, or the name for local (unuploaded) levels.
    static std::string keyFor(GJGameLevel* level);

    LevelRecord const& find(std::string const& key);
    void addAttempt(std::string const& key, HeartTrack const& attempt, bool completed);

private:
    std::unordered_map<std::string, LevelRecord> m_records;
    bool m_loaded = false;

    void load();
    void save();
};
