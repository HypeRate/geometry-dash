#include "LevelStats.hpp"

#include <Geode/Geode.hpp>

using namespace geode::prelude;

namespace {
    constexpr int kFormatVersion = 1;

    std::filesystem::path statsPath() {
        return Mod::get()->getSaveDir() / "level-stats.json";
    }

    matjson::Value toJsonArray(std::array<float, HeartTrack::kBuckets> const& values) {
        auto array = matjson::Value::array();
        for (float value : values) {
            // Two decimals are plenty and keep the file small.
            array.push(std::round(value * 100.f) / 100.f);
        }
        return array;
    }

    void fromJsonArray(matjson::Value const& json, std::array<float, HeartTrack::kBuckets>& out) {
        auto array = json.asArray();
        if (!array) return;
        auto const& values = array.unwrap();
        for (size_t i = 0; i < values.size() && i < out.size(); ++i) {
            out[i] = static_cast<float>(values[i].asDouble().unwrapOr(0.0));
        }
    }

    matjson::Value toJson(LevelRecord const& record) {
        auto const& heat = record.heat;
        return matjson::makeObject({
            { "attempts", record.attempts },
            { "completions", record.completions },
            { "bestPercent", record.bestPercent },
            { "calmestClear", record.calmestClear },
            { "peakBpm", heat.peakBpm },
            { "peakPercent", heat.peakPercent },
            { "sum", toJsonArray(heat.weightedSum) },
            { "time", toJsonArray(heat.time) },
        });
    }

    LevelRecord fromJson(matjson::Value const& json) {
        LevelRecord record;
        record.attempts = static_cast<int>(json["attempts"].asInt().unwrapOr(0));
        record.completions = static_cast<int>(json["completions"].asInt().unwrapOr(0));
        record.bestPercent = static_cast<float>(json["bestPercent"].asDouble().unwrapOr(0.0));
        record.calmestClear = static_cast<float>(json["calmestClear"].asDouble().unwrapOr(0.0));

        auto& heat = record.heat;
        heat.peakBpm = static_cast<int>(json["peakBpm"].asInt().unwrapOr(0));
        heat.peakPercent = static_cast<float>(json["peakPercent"].asDouble().unwrapOr(0.0));
        fromJsonArray(json["sum"], heat.weightedSum);
        fromJsonArray(json["time"], heat.time);
        for (int i = 0; i < HeartTrack::kBuckets; ++i) {
            if (heat.time[i] > 0.f) heat.coveredBuckets++;
            heat.totalWeightedSum += heat.weightedSum[i];
            heat.totalTime += heat.time[i];
        }
        return record;
    }
}

LevelStats& LevelStats::get() {
    static LevelStats instance;
    return instance;
}

std::string LevelStats::keyFor(GJGameLevel* level) {
    if (!level) return {};
    int id = level->m_levelID.value();
    if (id > 0) return std::to_string(id);
    return "local:" + std::string(level->m_levelName);
}

LevelRecord const& LevelStats::find(std::string const& key) {
    this->load();
    // Creates an empty record for new levels, which is what callers want.
    return m_records[key];
}

void LevelStats::addAttempt(std::string const& key, HeartTrack const& attempt, bool completed) {
    if (key.empty() || attempt.empty()) return;
    this->load();

    auto& record = m_records[key];
    record.attempts++;
    record.bestPercent = std::max(record.bestPercent, completed ? 100.f : attempt.endPercent);
    record.heat.add(attempt);

    if (completed) {
        record.completions++;
        float average = attempt.average();
        if (record.calmestClear <= 0.f || average < record.calmestClear) {
            record.calmestClear = average;
        }
    }

    this->save();
}

void LevelStats::load() {
    if (m_loaded) return;
    m_loaded = true;

    if (!std::filesystem::exists(statsPath())) return;
    auto json = file::readJson(statsPath());
    if (!json) {
        log::warn("Could not read level stats: {}", json.unwrapErr());
        return;
    }
    auto root = json.unwrap();
    for (auto const& [key, value] : root["levels"]) {
        m_records[key] = fromJson(value);
    }
}

void LevelStats::save() {
    auto levels = matjson::Value::object();
    for (auto const& [key, record] : m_records) {
        if (record.attempts > 0) levels[key] = toJson(record);
    }
    auto json = matjson::makeObject({
        { "version", kFormatVersion },
        { "levels", levels },
    });

    auto result = file::writeStringSafe(statsPath(), json.dump(matjson::NO_INDENTATION));
    if (!result) {
        log::warn("Could not save level stats: {}", result.unwrapErr());
    }
}
