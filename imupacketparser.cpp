#include "imupacketparser.h"

#include <QString>
#include <QStringList>

bool ImuPacketParser::parse(const QByteArray &data, ImuSample &sample)
{
    const QStringList fields = QString::fromUtf8(data).trimmed().split(',');

    // seq, timestamp, 6 wartosci skalibrowanych, 6 filtrowanych, drawingEnabled
    if (fields.size() != 15)
        return false;

    ImuSample parsed{};
    bool ok = false;

    parsed.sequence = fields.at(0).trimmed().toUInt(&ok);
    if (!ok) return false;

    parsed.timestampUs = fields.at(1).trimmed().toLongLong(&ok);
    if (!ok) return false;

    auto readFloat = [&](int index, float &destination) -> bool {
        destination = fields.at(index).trimmed().toFloat(&ok);
        return ok;
    };

    if (!readFloat(2, parsed.gx)) return false;
    if (!readFloat(3, parsed.gy)) return false;
    if (!readFloat(4, parsed.gz)) return false;
    if (!readFloat(5, parsed.ax)) return false;
    if (!readFloat(6, parsed.ay)) return false;
    if (!readFloat(7, parsed.az)) return false;

    if (!readFloat(8, parsed.filteredGx)) return false;
    if (!readFloat(9, parsed.filteredGy)) return false;
    if (!readFloat(10, parsed.filteredGz)) return false;
    if (!readFloat(11, parsed.filteredAx)) return false;
    if (!readFloat(12, parsed.filteredAy)) return false;
    if (!readFloat(13, parsed.filteredAz)) return false;

    const int drawingValue = fields.at(14).trimmed().toInt(&ok);
    if (!ok || (drawingValue != 0 && drawingValue != 1))
        return false;
    parsed.drawingEnabled = drawingValue == 1;

    sample = parsed;
    return true;
}
