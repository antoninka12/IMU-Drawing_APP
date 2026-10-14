#pragma once
#include <QtGlobal>

struct ImuSample
{
    quint32 sequence = 0;
    qint64 timestampUs = 0;

    // Dane skalibrowane z BMI160.
    float gx = 0, gy = 0, gz = 0;
    float ax = 0, ay = 0, az = 0;

    // Dane po filtracji na ESP32.
    float filteredGx = 0, filteredGy = 0, filteredGz = 0;
    float filteredAx = 0, filteredAy = 0, filteredAz = 0;

    // 15. pole CSV: 0 = rysowanie OFF, 1 = rysowanie ON.
    // To surowy stan PRZELACZNIKA logicznego z ESP32 (jeszcze bez potwierdzenia Qt).
    bool drawingEnabled = false;
};
