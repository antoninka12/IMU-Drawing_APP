#pragma once

#include <QPointF>
#include <QtGlobal>

#include "imusample.h"

// Mapuje predkosc katowa zyroskopu na przesuniecie punktu w pikselach.
// Nie okresla bezwzglednej pozycji reki.
class MotionMapper
{
public:
    MotionMapper() = default;

    // Wywolac dla kazdej poprawnie sparsowanej probki.
    // Podczas kalibracji oraz przy niepoprawnym dt zwraca (0, 0).
    QPointF processSample(const ImuSample &sample);

    // Nowa sesja BLE = nowe oszacowanie offsetu.
    void reset();

    bool isReady() const { return m_ready; }
    int calibrationProgress() const { return m_calibrationCount; }
    int calibrationTarget() const { return kCalibrationSamples; }

    // Parametry do pozniejszego strojenia (dla wartosci zyroskopu w deg/s).
    void setSensitivity(double x, double y);
    void setDeadZone(double threshold);

private:
    static double applyDeadZone(double value, double threshold);

    // Okolo 2 s przy 25 Hz. Wymagane nieruchome urzadzenie.
    static constexpr int kCalibrationSamples = 50;
    static constexpr double kStartupMotionLimit = 8.0; // deg/s
    static constexpr double kMaxDtSeconds = 0.20;      // ignoruj zbyt dlugie przerwy

    // Przyjeto: filteredGy -> X z minusem; filteredGx -> Y z minusem.
    double m_sensitivityX = 2.0; // piksele na stopien obrotu
    double m_sensitivityY = 2.0;
    double m_deadZone = 2.5;     // deg/s, po odjeciu biasu

    double m_sumGx = 0.0;
    double m_sumGy = 0.0;
    double m_biasGx = 0.0;
    double m_biasGy = 0.0;
    int m_calibrationCount = 0;
    bool m_ready = false;

    qint64 m_previousTimestampUs = 0;
    bool m_hasPreviousTimestamp = false;
};
