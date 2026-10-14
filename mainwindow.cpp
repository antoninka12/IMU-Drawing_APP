#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "imupacketparser.h"

#include <QFileDialog>
#include <QDateTime>
#include <QVBoxLayout>
#include <QPainter>
#include <QPushButton>
#include <QString>


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    m_motionMapper.setSensitivity(8.0, 8.0);
    m_motionMapper.setDeadZone(0.5);

    m_motionTestWidget = new MotionTestWidget();
    m_motionTestWidget->setAttribute(Qt::WA_QuitOnClose, false);
    m_motionTestWidget->show();


    // ===================================================
    // WYKRES
    // ===================================================

    setupChart();


    // ===================================================
    // BLE CLIENT
    // ===================================================

    m_bleClient =
        new BleClient(this);


    // Na początku nie nagrywamy.
    ui->stopRecordingButton->setEnabled(false);


    resetStatistics();


    // ===================================================
    // SCAN
    // ===================================================

    connect(
        ui->scanButton,
        &QPushButton::clicked,
        this,
        [this]()
        {
            m_devices.clear();

            ui->devicesList->clear();


            ui->logEdit->appendPlainText(
                "Starting BLE scan..."
                );


            m_bleClient->startScan();
        }
        );


    // ===================================================
    // ZNALEZIONO URZĄDZENIE
    // ===================================================

    connect(
        m_bleClient,
        &BleClient::deviceFound,
        this,
        [this](
            const QBluetoothDeviceInfo &device
            )
        {
            m_devices.append(device);


            QString name =
                device.name();


            if (name.isEmpty())
            {
                name =
                    "Unknown BLE device";
            }


            ui->devicesList->addItem(
                name
                );


            ui->logEdit->appendPlainText(
                "Found: "
                + name
                );
        }
        );


    // ===================================================
    // CONNECT
    // ===================================================

    connect(
        ui->connectButton,
        &QPushButton::clicked,
        this,
        [this]()
        {
            const int row =
                ui->devicesList->currentRow();


            if (
                row < 0
                ||
                row >= m_devices.size()
                )
            {
                ui->logEdit->appendPlainText(
                    "Select BLE device first"
                    );

                return;
            }


            resetStatistics();


            m_bleClient->connectToDevice(
                m_devices.at(row)
                );
        }
        );


    // ===================================================
    // STATUS BLE
    // ===================================================

    connect(
        m_bleClient,
        &BleClient::statusChanged,
        this,
        [this](
            const QString &status
            )
        {
            ui->statusLabel->setText(
                status
                );


            ui->logEdit->appendPlainText(
                status
                );
        }
        );


    // ===================================================
    // SERVICE FOUND
    // ===================================================

    connect(
        m_bleClient,
        &BleClient::serviceFound,
        this,
        [this](
            const QBluetoothUuid &uuid
            )
        {
            ui->logEdit->appendPlainText(
                "Service: "
                + uuid.toString()
                );
        }
        );


    // ===================================================
    // CHARACTERISTIC FOUND
    // ===================================================

    connect(
        m_bleClient,
        &BleClient::characteristicFound,
        this,
        [this](
            const QString &text
            )
        {
            ui->logEdit->appendPlainText(
                text
                );
        }
        );


    // ===================================================
    // ODEBRANO DANE BLE
    // ===================================================

    connect(
        m_bleClient,
        &BleClient::dataReceived,
        this,
        [this](
            const QByteArray &data
            )
        {
            processBleData(data);
        }
        );


    // ===================================================
    // START RECORDING
    // ===================================================

    connect(
        ui->startRecordingButton,
        &QPushButton::clicked,
        this,
        [this]()
        {
            startRecording();
        }
        );


    // ===================================================
    // STOP RECORDING
    // ===================================================

    connect(
        ui->stopRecordingButton,
        &QPushButton::clicked,
        this,
        [this]()
        {
            stopRecording();
        }
        );
}


// =======================================================
// DESTRUKTOR
// =======================================================

MainWindow::~MainWindow()
{
    if (m_recording)
    {
        m_csvStream.flush();
        m_csvFile.close();
    }

    delete m_motionTestWidget;
    m_motionTestWidget = nullptr;

    delete ui;
}


// =======================================================
// BLE -> BUFOR TEKSTOWY
// =======================================================

void MainWindow::processBleData(
    const QByteArray &data
    )
{
    // Dopisujemy nowo odebrane bajty.
    m_rxBuffer.append(data);


    // ESP32 kończy każdy rekord CSV znakiem '\n'.
    //
    // Dopóki mamy pełną linię, przetwarzamy ją.

    while (true)
    {
        const qsizetype newlineIndex =
            m_rxBuffer.indexOf('\n');


        if (newlineIndex < 0)
        {
            // Nie ma jeszcze pełnego rekordu.
            break;
        }


        const QByteArray line =
            m_rxBuffer.left(
                newlineIndex
                );


        // Usuwamy już przetworzone dane
        // wraz ze znakiem '\n'.

        m_rxBuffer.remove(
            0,
            newlineIndex + 1
            );


        if (!line.trimmed().isEmpty())
        {
            processImuLine(line);
        }
    }
}


// =======================================================
// PRZETWARZANIE JEDNEJ PRÓBKI
// =======================================================

void MainWindow::processImuLine(const QByteArray &line)
{
    ImuSample sample{};

    // ===================================================
    // PARSER CSV
    // ===================================================

    if (!ImuPacketParser::parse(line, sample))
    {
        ui->logEdit->appendPlainText(
            "ERROR: invalid IMU packet"
            );
        return;
    }

    // ===================================================
    // PRZYCISK - POTWIERDZENIE STANU (0.5 s / 80%)
    // ===================================================

    const bool changed = m_drawingStateFilter.addSample(
        sample.drawingEnabled,
        static_cast<std::int64_t>(sample.timestampUs)
        );

    const bool confirmed = m_drawingStateFilter.state();

    if (m_motionTestWidget)
    {
        m_motionTestWidget->setDrawingEnabled(confirmed);
    }

    if (changed)
    {
        ui->logEdit->appendPlainText(
            confirmed
                ? "RYSOWANIE ON (0.5 s / 80%)"
                : "RYSOWANIE OFF (0.5 s / 80%)"
            );
    }

    // ===================================================
    // MAPOWANIE RUCHU I KALIBRACJA
    // ===================================================

    const QPointF delta = m_motionMapper.processSample(sample);
    static int debugCounter = 0;

    if (++debugCounter % 25 == 0)
    {
        ui->logEdit->appendPlainText(
            QString("GYRO gx=%1 gy=%2 | DELTA dx=%3 dy=%4")
                .arg(sample.filteredGx, 0, 'f', 3)
                .arg(sample.filteredGy, 0, 'f', 3)
                .arg(delta.x(), 0, 'f', 3)
                .arg(delta.y(), 0, 'f', 3)
            );
    }

    if (m_motionTestWidget)
    {
        if (m_motionMapper.isReady())
        {
            m_motionTestWidget->setStatusText(
                QStringLiteral("Sterowanie aktywne | C = wyczysc")
                );

            m_motionTestWidget->applyDelta(delta);
        }
        else
        {
            m_motionTestWidget->setStatusText(
                QStringLiteral("Kalibracja: %1/%2 - trzymaj nieruchomo")
                    .arg(m_motionMapper.calibrationProgress())
                    .arg(m_motionMapper.calibrationTarget())
                );
        }
    }

    // ===================================================
    // WYKRES
    // ===================================================

    updateChart(sample);


    // ===================================================
    // CZAS ODBIORU PO STRONIE QT
    // ===================================================

    if (!m_sessionTimer.isValid())
    {
        m_sessionTimer.start();
    }


    const qint64 pcRxTimeUs =
        m_sessionTimer.nsecsElapsed()
        / 1000;


    // ===================================================
    // ODEBRANA PRÓBKA
    // ===================================================

    m_receivedSamples++;


    qint64 sensorDtUs = 0;


    // ===================================================
    // PORÓWNANIE Z POPRZEDNIĄ PRÓBKĄ
    // ===================================================

    if (m_havePreviousSample)
    {
        // ------------------------------------------------
        // CZĘSTOTLIWOŚĆ PRÓBKOWANIA ESP32
        // ------------------------------------------------

        sensorDtUs =
            sample.timestampUs
            - m_previousTimestampUs;


        if (sensorDtUs > 0)
        {
            m_lastSensorDtMs =
                static_cast<double>(
                    sensorDtUs
                    )
                / 1000.0;


            m_sensorRateHz =
                1000000.0
                /
                static_cast<double>(
                    sensorDtUs
                    );
        }


        // ------------------------------------------------
        // SPRAWDZANIE UTRACONYCH PRÓBEK
        // ------------------------------------------------

        const quint32 expectedSequence =
            m_previousSequence + 1;


        if (
            sample.sequence
            > expectedSequence
            )
        {
            m_lostSamples +=
                sample.sequence
                - expectedSequence;
        }
    }


    // Zapamiętujemy obecną próbkę.

    m_previousSequence =
        sample.sequence;


    m_previousTimestampUs =
        sample.timestampUs;


    m_havePreviousSample =
        true;


    // ===================================================
    // CZĘSTOTLIWOŚĆ ODBIORU QT
    // ===================================================

    if (!m_rxRateTimer.isValid())
    {
        m_rxRateTimer.start();

        m_samplesInRxWindow = 0;
    }


    m_samplesInRxWindow++;


    const qint64 elapsedMs =
        m_rxRateTimer.elapsed();


    // Około raz na sekundę liczymy,
    // ile próbek/s faktycznie odebrał Qt.

    if (elapsedMs >= 1000)
    {
        m_qtRxRateHz =
            static_cast<double>(
                m_samplesInRxWindow
                )
            * 1000.0
            /
            static_cast<double>(
                elapsedMs
                );


        m_rxRateTimer.restart();

        m_samplesInRxWindow = 0;
    }


    // ===================================================
    // ZAPIS CSV
    // ===================================================

    if (m_recording)
    {
        writeCsvSample(
            sample,
            pcRxTimeUs,
            sensorDtUs
            );
    }


    // ===================================================
    // GUI
    // ===================================================

    // Nie ma potrzeby zmieniać QLabel 50 razy/s.
    //
    // 50 Hz / 5 = około 10 aktualizacji GUI/s.

    if (
        m_receivedSamples % 5 == 0
        )
    {
        updateStatisticsUi();
    }


    // ===================================================
    // LOG KONTROLNY
    // ===================================================

    // Raz na około sekundę.

    if (
        m_receivedSamples % 50 == 0
        )
    {
        ui->logEdit->appendPlainText(
            QString(
                "seq=%1 | sensor=%2 Hz | Qt RX=%3 Hz | lost=%4"
                )
                .arg(
                    sample.sequence
                    )
                .arg(
                    m_sensorRateHz,
                    0,
                    'f',
                    2
                    )
                .arg(
                    m_qtRxRateHz,
                    0,
                    'f',
                    2
                    )
                .arg(
                    m_lostSamples
                    )
            );
    }
}


// =======================================================
// RESET STATYSTYK
// =======================================================

void MainWindow::resetStatistics()
{
    m_drawingStateFilter.reset();
    m_motionMapper.reset();
    if (m_motionTestWidget) {
        m_motionTestWidget->setDrawingEnabled(false);
        m_motionTestWidget->resetPoint();
    }

    m_receivedSamples = 0;

    m_lostSamples = 0;


    m_havePreviousSample = false;

    m_previousSequence = 0;

    m_previousTimestampUs = 0;


    m_sensorRateHz = 0.0;

    m_lastSensorDtMs = 0.0;

    m_qtRxRateHz = 0.0;


    m_samplesInRxWindow = 0;


    m_rxRateTimer.invalidate();

    m_sessionTimer.invalidate();


    m_rxBuffer.clear();


    // ---------------------------------------------------
    // Reset wykresu
    // ---------------------------------------------------

    m_chartStarted = false;

    m_chartStartTimestampUs = 0;


    if (m_calibratedSeries != nullptr)
    {
        m_calibratedSeries->clear();
    }


    if (m_filteredSeries != nullptr)
    {
        m_filteredSeries->clear();
    }


    if (m_axisX != nullptr)
    {
        m_axisX->setRange(
            0.0,
            5.0
            );
    }


    updateStatisticsUi();
    m_motionMapper.reset();
    if (m_motionTestWidget) {
        m_motionTestWidget->resetPoint();
        m_motionTestWidget->setStatusText(
            QStringLiteral("Polacz BLE i trzymaj czujnik nieruchomo"));
    }
}


// =======================================================
// STATYSTYKI -> GUI
// =======================================================

void MainWindow::updateStatisticsUi()
{
    ui->sampleRateLabel->setText(
        QString(
            "Sensor rate: %1 Hz"
            )
            .arg(
                m_sensorRateHz,
                0,
                'f',
                2
                )
        );


    ui->rxRateLabel->setText(
        QString(
            "Qt RX rate: %1 Hz"
            )
            .arg(
                m_qtRxRateHz,
                0,
                'f',
                2
                )
        );


    ui->lastDtLabel->setText(
        QString(
            "Sensor dt: %1 ms"
            )
            .arg(
                m_lastSensorDtMs,
                0,
                'f',
                3
                )
        );


    ui->receivedLabel->setText(
        QString(
            "Received: %1"
            )
            .arg(
                m_receivedSamples
                )
        );


    ui->lostLabel->setText(
        QString(
            "Lost: %1"
            )
            .arg(
                m_lostSamples
                )
        );
}


// =======================================================
// START RECORDING CSV
// =======================================================

void MainWindow::startRecording()
{
    if (m_recording)
    {
        return;
    }


    const QString defaultName =
        "imu_filter_"
        +
        QDateTime::currentDateTime()
            .toString(
                "yyyyMMdd_HHmmss"
                )
        +
        ".csv";


    const QString filePath =
        QFileDialog::getSaveFileName(
            this,
            "Save IMU data",
            defaultName,
            "CSV files (*.csv)"
            );


    if (filePath.isEmpty())
    {
        return;
    }


    m_csvFile.setFileName(
        filePath
        );


    if (
        !m_csvFile.open(
            QIODevice::WriteOnly
            |
            QIODevice::Text
            )
        )
    {
        ui->logEdit->appendPlainText(
            "ERROR: cannot open CSV file"
            );

        return;
    }


    m_csvStream.setDevice(
        &m_csvFile
        );


    // ===================================================
    // NAGŁÓWEK CSV
    // ===================================================

    m_csvStream

        << "sequence,"
        << "timestamp_us,"
        << "pc_rx_time_us,"
        << "sensor_dt_us,"
        << "sensor_rate_hz,"
        << "qt_rx_rate_hz,"

        << "gx,"
        << "gy,"
        << "gz,"
        << "ax,"
        << "ay,"
        << "az,"

        << "filtered_gx,"
        << "filtered_gy,"
        << "filtered_gz,"
        << "filtered_ax,"
        << "filtered_ay,"
        << "filtered_az"

        << "\n";


    m_recording = true;


    ui->startRecordingButton
        ->setEnabled(false);


    ui->stopRecordingButton
        ->setEnabled(true);


    ui->logEdit->appendPlainText(
        "CSV recording started: "
        + filePath
        );
}


// =======================================================
// STOP RECORDING CSV
// =======================================================

void MainWindow::stopRecording()
{
    if (!m_recording)
    {
        return;
    }


    m_csvStream.flush();

    m_csvFile.close();


    m_recording = false;


    ui->startRecordingButton
        ->setEnabled(true);


    ui->stopRecordingButton
        ->setEnabled(false);


    ui->logEdit->appendPlainText(
        "CSV recording stopped"
        );
}


// =======================================================
// ZAPIS PRÓBKI DO CSV
// =======================================================

void MainWindow::writeCsvSample(
    const ImuSample &sample,
    qint64 pcRxTimeUs,
    qint64 sensorDtUs
    )
{
    m_csvStream

        << QString::number(
               sample.sequence
               )
        << ","

        << QString::number(
               sample.timestampUs
               )
        << ","

        << QString::number(
               pcRxTimeUs
               )
        << ","

        << QString::number(
               sensorDtUs
               )
        << ","

        << QString::number(
               m_sensorRateHz,
               'f',
               3
               )
        << ","

        << QString::number(
               m_qtRxRateHz,
               'f',
               3
               )
        << ","


        // ------------------------------------------------
        // CALIBRATED
        // ------------------------------------------------

        << QString::number(
               sample.gx,
               'f',
               4
               )
        << ","

        << QString::number(
               sample.gy,
               'f',
               4
               )
        << ","

        << QString::number(
               sample.gz,
               'f',
               4
               )
        << ","

        << QString::number(
               sample.ax,
               'f',
               4
               )
        << ","

        << QString::number(
               sample.ay,
               'f',
               4
               )
        << ","

        << QString::number(
               sample.az,
               'f',
               4
               )
        << ","


        // ------------------------------------------------
        // FILTERED
        // ------------------------------------------------

        << QString::number(
               sample.filteredGx,
               'f',
               4
               )
        << ","

        << QString::number(
               sample.filteredGy,
               'f',
               4
               )
        << ","

        << QString::number(
               sample.filteredGz,
               'f',
               4
               )
        << ","

        << QString::number(
               sample.filteredAx,
               'f',
               4
               )
        << ","

        << QString::number(
               sample.filteredAy,
               'f',
               4
               )
        << ","

        << QString::number(
               sample.filteredAz,
               'f',
               4
               )

        << "\n";
}


// =======================================================
// TWORZENIE WYKRESU
// =======================================================

void MainWindow::setupChart()
{
    // ===================================================
    // SERIE
    // ===================================================

    m_calibratedSeries =
        new QLineSeries();


    m_filteredSeries =
        new QLineSeries();


    m_calibratedSeries->setName(
        "ax calibrated"
        );


    m_filteredSeries->setName(
        "ax filtered"
        );


    // ===================================================
    // CHART
    // ===================================================

    m_chart =
        new QChart();


    m_chart->addSeries(
        m_calibratedSeries
        );


    m_chart->addSeries(
        m_filteredSeries
        );


    m_chart->setTitle(
        "Accelerometer X - calibrated vs filtered"
        );


    // ===================================================
    // OŚ X
    // ===================================================

    m_axisX =
        new QValueAxis();


    m_axisX->setTitleText(
        "Time [s]"
        );


    m_axisX->setRange(
        0.0,
        5.0
        );


    // ===================================================
    // OŚ Y
    // ===================================================

    m_axisY =
        new QValueAxis();


    m_axisY->setTitleText(
        "ax"
        );


    // Na razie stały zakres.
    //
    // Później możemy zrobić automatyczne skalowanie.

    m_axisY->setRange(
        -15.0,
        15.0
        );


    // ===================================================
    // PODPINAMY OSIE
    // ===================================================

    m_chart->addAxis(
        m_axisX,
        Qt::AlignBottom
        );


    m_chart->addAxis(
        m_axisY,
        Qt::AlignLeft
        );


    m_calibratedSeries->attachAxis(
        m_axisX
        );


    m_calibratedSeries->attachAxis(
        m_axisY
        );


    m_filteredSeries->attachAxis(
        m_axisX
        );


    m_filteredSeries->attachAxis(
        m_axisY
        );


    // ===================================================
    // CHART VIEW
    // ===================================================

    m_chartView =
        new QChartView(
            m_chart,
            ui->chartContainer
            );


    m_chartView->setRenderHint(
        QPainter::Antialiasing
        );


    // ===================================================
    // LAYOUT
    // ===================================================

    auto *layout =
        new QVBoxLayout(
            ui->chartContainer
            );


    layout->setContentsMargins(
        0,
        0,
        0,
        0
        );


    layout->addWidget(
        m_chartView
        );
}


// =======================================================
// AKTUALIZACJA WYKRESU
// =======================================================

void MainWindow::updateChart(
    const ImuSample &sample
    )
{
    // ===================================================
    // PIERWSZA PRÓBKA
    // ===================================================

    if (!m_chartStarted)
    {
        m_chartStartTimestampUs =
            sample.timestampUs;


        m_chartStarted = true;
    }


    // ===================================================
    // CZAS OD POCZĄTKU WYKRESU [s]
    // ===================================================

    const double timeSeconds =
        static_cast<double>(
            sample.timestampUs
            -
            m_chartStartTimestampUs
            )
        /
        1000000.0;


    // ===================================================
    // CALIBRATED
    // ===================================================

    m_calibratedSeries->append(
        timeSeconds,
        sample.ax
        );


    // ===================================================
    // FILTERED
    // ===================================================

    m_filteredSeries->append(
        timeSeconds,
        sample.filteredAx
        );


    // ===================================================
    // OKNO 5 SEKUND
    // ===================================================

    constexpr double windowSeconds =
        5.0;


    if (
        timeSeconds
        > windowSeconds
        )
    {
        m_axisX->setRange(
            timeSeconds
                - windowSeconds,

            timeSeconds
            );
    }
    else
    {
        m_axisX->setRange(
            0.0,
            windowSeconds
            );
    }


    // ===================================================
    // USUWANIE STARYCH PUNKTÓW
    // ===================================================

    const double oldestTime =
        timeSeconds
        - windowSeconds;


    while (
        m_calibratedSeries->count() > 0
        &&
        m_calibratedSeries->at(0).x()
            < oldestTime
        )
    {
        m_calibratedSeries->remove(0);

        m_filteredSeries->remove(0);
    }
}