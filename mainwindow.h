#pragma once

#include <QMainWindow>
#include <QList>
#include <QByteArray>

#include <QFile>
#include <QTextStream>
#include <QElapsedTimer>

#include <QBluetoothDeviceInfo>

#include <QChart>
#include <QChartView>
#include <QLineSeries>
#include <QValueAxis>

#include "bleclient.h"
#include "imusample.h"


QT_BEGIN_NAMESPACE

namespace Ui
{
class MainWindow;
}

QT_END_NAMESPACE


class MainWindow : public QMainWindow
{
    Q_OBJECT


public:

    explicit MainWindow(
        QWidget *parent = nullptr
        );

    ~MainWindow();


private:

    // ===================================================
    // BLE / PARSER
    // ===================================================

    void processBleData(
        const QByteArray &data
        );

    void processImuLine(
        const QByteArray &line
        );


    // ===================================================
    // STATYSTYKI
    // ===================================================

    void resetStatistics();

    void updateStatisticsUi();


    // ===================================================
    // CSV
    // ===================================================

    void startRecording();

    void stopRecording();

    void writeCsvSample(
        const ImuSample &sample,
        qint64 pcRxTimeUs,
        qint64 sensorDtUs
        );


    // ===================================================
    // WYKRES
    // ===================================================

    void setupChart();

    void updateChart(
        const ImuSample &sample
        );


private:

    Ui::MainWindow *ui;


    // ===================================================
    // BLE
    // ===================================================

    BleClient *m_bleClient = nullptr;

    QList<QBluetoothDeviceInfo> m_devices;


    // Bufor odbiorczy.
    //
    // Dane ESP32 kończą się '\n'.
    // Dzięki temu możemy przetwarzać pełne rekordy CSV.
    QByteArray m_rxBuffer;


    // ===================================================
    // STATYSTYKI
    // ===================================================

    quint64 m_receivedSamples = 0;

    quint64 m_lostSamples = 0;


    bool m_havePreviousSample = false;

    quint32 m_previousSequence = 0;

    qint64 m_previousTimestampUs = 0;


    double m_sensorRateHz = 0.0;

    double m_lastSensorDtMs = 0.0;


    // ---------------------------------------------------
    // Częstotliwość odbioru przez Qt
    // ---------------------------------------------------

    QElapsedTimer m_rxRateTimer;

    quint64 m_samplesInRxWindow = 0;

    double m_qtRxRateHz = 0.0;


    // ---------------------------------------------------
    // Czas lokalny sesji Qt
    // ---------------------------------------------------

    QElapsedTimer m_sessionTimer;


    // ===================================================
    // CSV
    // ===================================================

    QFile m_csvFile;

    QTextStream m_csvStream;

    bool m_recording = false;


    // ===================================================
    // WYKRES
    // ===================================================

    QChart *m_chart = nullptr;

    QChartView *m_chartView = nullptr;


    // Dwie linie:
    //
    // calibrated ax
    // filtered ax

    QLineSeries *m_calibratedSeries = nullptr;

    QLineSeries *m_filteredSeries = nullptr;


    QValueAxis *m_axisX = nullptr;

    QValueAxis *m_axisY = nullptr;


    bool m_chartStarted = false;

    qint64 m_chartStartTimestampUs = 0;
};