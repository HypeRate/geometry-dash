#pragma once

// Learns the player's normal heart rate while playing and derives the zone
// limits from it, so the colors mean something for everybody - whether their
// heart usually sits at 70 or at 110 BPM.
//
// Each heart rate update received during a level (not paused) is one sample.
// The mean and the typical deviation are kept as moving averages and saved,
// so the learned zones survive restarts.
class AutoZones {
public:
    static AutoZones& get();

    void addSample(int bpm);
    void reset();

    // Enough samples to use the learned zones (about two minutes of play).
    bool ready() const;
    // 0..1 while learning.
    float progress() const;

    float normal() const { return m_mean; }
    float calmUpTo() const;
    float panicFrom() const;

private:
    float m_mean = 0.f;
    // Moving average of |bpm - mean|.
    float m_deviation = 0.f;
    int m_samples = 0;
    bool m_loaded = false;

    void load();
    void save();
};
