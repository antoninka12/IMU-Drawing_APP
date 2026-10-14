#pragma once

#include <cstddef>
#include <cstdint>
#include <deque>

// Filtr stanu rysowania odebranego w kazdej probce BLE.
// Dziala w oknie 500 ms wedlug timestamp_us z ESP32.
// Zmiana stanu: >= 80% probek za stanem przeciwnym do obecnego.
// Dodatkowo wymagamy >= 8 probek i >= 400 ms pokrycia okna,
// by przerwy w BLE nie powodowaly przypadkowych przelaczen.
class DrawingStateFilter
{
public:
    static constexpr std::int64_t WindowUs = 500000;
    static constexpr std::int64_t MinimumCoverageUs = 400000;
    static constexpr std::size_t MinimumSamples = 8;
    static constexpr int RequiredPercent = 80;

    void reset()
    {
        m_samples.clear();
        m_enabled = false;
    }

    bool state() const { return m_enabled; }

    // Zwraca true TYLKO wtedy, gdy zostal zatwierdzony nowy stan.
    bool addSample(bool transmittedState, std::int64_t timestampUs)
    {
        // Restart ESP32 lub powtorzony/stary timestamp: nowa sesja filtrowania.
        if (!m_samples.empty() && timestampUs <= m_samples.back().timestampUs)
            reset();

        m_samples.push_back({timestampUs, transmittedState});

        while (!m_samples.empty() &&
               timestampUs - m_samples.front().timestampUs > WindowUs)
            m_samples.pop_front();

        if (m_samples.size() < MinimumSamples)
            return false;

        if (timestampUs - m_samples.front().timestampUs < MinimumCoverageUs)
            return false;

        std::size_t votesForOpposite = 0;
        for (const auto &s : m_samples) {
            if (s.value != m_enabled)
                ++votesForOpposite;
        }

        if (votesForOpposite * 100 < RequiredPercent * m_samples.size())
            return false;

        m_enabled = !m_enabled;
        return true;
    }

private:
    struct Observation {
        std::int64_t timestampUs;
        bool value;
    };

    std::deque<Observation> m_samples;
    bool m_enabled = false;  // Bezpieczny stan poczatkowy: nie rysuj.
};
