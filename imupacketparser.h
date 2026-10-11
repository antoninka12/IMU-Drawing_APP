#pragma once

#include <QByteArray>

#include "imusample.h"


class ImuPacketParser
{
public:

    static bool parse(
        const QByteArray &data,
        ImuSample &sample
        );
};