#include "motionmapper.h"

#include <cmath>

void MotionMapper::reset()
{
    m_sumGx = 0.0;
    m_sumGy = 0.0;
    m_biasGx = 0.0;
    m_biasGy = 0.0;
    m_calibrationCount = 0;
    m_ready = false;
    m_previousTimestampUs = 0;
    m_hasPreviousTimestamp = false;
}

void MotionMapper::setSensitivity(double x, double y)
{
    if (std::isfinite(x) && x > 0.0 && std::isfinite(y) && y > 0.0) {
        m_sensitivityX = x;
        m_sensitivityY = y;
    }
}

void MotionMapper::setDeadZone(double threshold)
{
    if (std::isfinite(threshold) && threshold >= 0.0)
        m_deadZone = threshold;
}

double MotionMapper::applyDeadZone(double value, double threshold)
{
    // Twarda dead-zone: w spoczynku punkt ma stac idealnie nieruchomo.
    return std::abs(value) <= threshold ? 0.0 : value;
}

QPointF MotionMapper::processSample(const ImuSample &sample)
{
    const double gx = static_cast<double>(sample.filteredGx);
    const double gy = static_cast<double>(sample.filteredGy);

    if (!std::isfinite(gx) || !std::isfinite(gy))
        return QPointF(0.0, 0.0);

    // timestamp_us pochodzi z ESP32, nie z czasu odbioru BLE.
    double dt = 0.0;
    if (m_hasPreviousTimestamp) {
        if (sample.timestampUs <= m_previousTimestampUs) {
            // Przeskok wstecz (np. restart firmware) -> nowa kalibracja.
            if (sample.timestampUs < m_previousTimestampUs)
                reset();
            else
                return QPointF(0.0, 0.0);
        } else {
            dt = static_cast<double>(sample.timestampUs - m_previousTimestampUs) / 1'000'000.0;
        }
    }
    m_previousTimestampUs = sample.timestampUs;
    m_hasPreviousTimestamp = true;

    // Startup bias: 50 kolejnych spokojnych probek filteredGx/filteredGy.
    if (!m_ready) {
        // Jesli ktos poruszy urzadzeniem w czasie startu, zacznij od nowa.
        if (std::abs(gx) > kStartupMotionLimit ||
            std::abs(gy) > kStartupMotionLimit) {
            m_sumGx = 0.0;
            m_sumGy = 0.0;
            m_calibrationCount = 0;
            return QPointF(0.0, 0.0);
        }

        m_sumGx += gx;
        m_sumGy += gy;
        ++m_calibrationCount;

        if (m_calibrationCount == kCalibrationSamples) {
            m_biasGx = m_sumGx / kCalibrationSamples;
            m_biasGy = m_sumGy / kCalibrationSamples;
            m_ready = true;
        }
        return QPointF(0.0, 0.0);
    }

    // Nie wykonuj ruchu dla pierwszej probki, duplikatow i dlugich przerw BLE.
    if (dt <= 0.0 || dt > kMaxDtSeconds)
        return QPointF(0.0, 0.0);

    // Offset -> dead-zone -> mapowanie osi i znakow -> sensitivity -> dt.
    const double correctedGx = applyDeadZone(gx - m_biasGx, m_deadZone);
    const double correctedGy = applyDeadZone(gy - m_biasGy, m_deadZone);

    const double dx = -correctedGy * m_sensitivityX * dt;
    const double dy = -correctedGx * m_sensitivityY * dt;

    return QPointF(dx, dy);
}
