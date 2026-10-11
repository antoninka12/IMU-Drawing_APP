#pragma once

#include <QtGlobal>


struct ImuSample
{
    quint32 sequence;
    qint64 timestampUs;

    // Dane skalibrowane z BMI160
    float gx;
    float gy;
    float gz;

    float ax;
    float ay;
    float az;

    // Dane po filtrze dolnoprzepustowym
    float filteredGx;
    float filteredGy;
    float filteredGz;

    float filteredAx;
    float filteredAy;
    float filteredAz;
};