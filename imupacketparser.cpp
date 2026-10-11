#include "imupacketparser.h"

#include <QString>
#include <QStringList>


bool ImuPacketParser::parse(
    const QByteArray &data,
    ImuSample &sample
    )
{
    const QString text =
        QString::fromUtf8(data).trimmed();


    const QStringList fields =
        text.split(',');


    // ESP32 wysyła:
    //
    // sequence,
    // timestamp,
    // gx, gy, gz,
    // ax, ay, az,
    // filtered_gx, filtered_gy, filtered_gz,
    // filtered_ax, filtered_ay, filtered_az
    //
    // Razem 14 pól.

    if (fields.size() != 14)
    {
        return false;
    }


    bool ok = false;


    // ---------------------------------------------------
    // Sequence
    // ---------------------------------------------------

    sample.sequence =
        fields[0].toUInt(&ok);

    if (!ok)
    {
        return false;
    }


    // ---------------------------------------------------
    // Timestamp [us]
    // ---------------------------------------------------

    sample.timestampUs =
        fields[1].toLongLong(&ok);

    if (!ok)
    {
        return false;
    }


    // ---------------------------------------------------
    // Dane skalibrowane
    // ---------------------------------------------------

    sample.gx =
        fields[2].toFloat(&ok);

    if (!ok)
    {
        return false;
    }


    sample.gy =
        fields[3].toFloat(&ok);

    if (!ok)
    {
        return false;
    }


    sample.gz =
        fields[4].toFloat(&ok);

    if (!ok)
    {
        return false;
    }


    sample.ax =
        fields[5].toFloat(&ok);

    if (!ok)
    {
        return false;
    }


    sample.ay =
        fields[6].toFloat(&ok);

    if (!ok)
    {
        return false;
    }


    sample.az =
        fields[7].toFloat(&ok);

    if (!ok)
    {
        return false;
    }


    // ---------------------------------------------------
    // Dane po filtracji
    // ---------------------------------------------------

    sample.filteredGx =
        fields[8].toFloat(&ok);

    if (!ok)
    {
        return false;
    }


    sample.filteredGy =
        fields[9].toFloat(&ok);

    if (!ok)
    {
        return false;
    }


    sample.filteredGz =
        fields[10].toFloat(&ok);

    if (!ok)
    {
        return false;
    }


    sample.filteredAx =
        fields[11].toFloat(&ok);

    if (!ok)
    {
        return false;
    }


    sample.filteredAy =
        fields[12].toFloat(&ok);

    if (!ok)
    {
        return false;
    }


    sample.filteredAz =
        fields[13].toFloat(&ok);

    if (!ok)
    {
        return false;
    }


    return true;
}