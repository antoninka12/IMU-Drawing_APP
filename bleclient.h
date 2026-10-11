#pragma once

#include <QObject>
#include <QByteArray>

#include <QBluetoothDeviceDiscoveryAgent>
#include <QBluetoothDeviceInfo>
#include <QBluetoothUuid>

#include <QLowEnergyController>
#include <QLowEnergyService>
#include <QLowEnergyCharacteristic>
#include <QLowEnergyDescriptor>


class BleClient : public QObject
{
    Q_OBJECT

public:
    explicit BleClient(QObject *parent = nullptr);

    void startScan();

    void connectToDevice(
        const QBluetoothDeviceInfo &device
        );


signals:

    void deviceFound(
        const QBluetoothDeviceInfo &device
        );

    void statusChanged(
        const QString &status
        );

    void serviceFound(
        const QBluetoothUuid &uuid
        );

    void characteristicFound(
        const QString &text
        );

    // NOWE:
    // tym sygnałem będziemy przekazywać
    // odebrane bajty do MainWindow
    void dataReceived(
        const QByteArray &data
        );


private:

    QBluetoothDeviceDiscoveryAgent *m_discoveryAgent;

    QLowEnergyController *m_controller = nullptr;

    QLowEnergyService *m_service = nullptr;

    // NOWE:
    // tutaj zapamiętujemy characteristic
    // służącą do odbierania danych IMU
    QLowEnergyCharacteristic m_dataCharacteristic;
};