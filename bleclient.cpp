#include "bleclient.h"


// =======================================================
// KONSTRUKTOR
// =======================================================

BleClient::BleClient(QObject *parent)
    : QObject(parent)
{
    // Obiekt odpowiedzialny za wyszukiwanie urządzeń BLE.
    m_discoveryAgent =
        new QBluetoothDeviceDiscoveryAgent(this);


    // Wywoływane za każdym razem,
    // gdy znaleziono urządzenie Bluetooth.
    connect(
        m_discoveryAgent,
        &QBluetoothDeviceDiscoveryAgent::deviceDiscovered,
        this,
        [this](const QBluetoothDeviceInfo &device)
        {
            emit deviceFound(device);
        }
        );


    // Wywoływane po zakończeniu skanowania.
    connect(
        m_discoveryAgent,
        &QBluetoothDeviceDiscoveryAgent::finished,
        this,
        [this]()
        {
            emit statusChanged("Scan finished");
        }
        );


    // Obsługa błędu skanowania.
    connect(
        m_discoveryAgent,
        &QBluetoothDeviceDiscoveryAgent::errorOccurred,
        this,
        [this](QBluetoothDeviceDiscoveryAgent::Error)
        {
            emit statusChanged(
                "BLE error: "
                + m_discoveryAgent->errorString()
                );
        }
        );
}


// =======================================================
// SKANOWANIE BLE
// =======================================================

void BleClient::startScan()
{
    emit statusChanged("Scanning...");

    m_discoveryAgent->start(
        QBluetoothDeviceDiscoveryAgent::LowEnergyMethod
        );
}


// =======================================================
// POŁĄCZENIE Z WYBRANYM URZĄDZENIEM
// =======================================================

void BleClient::connectToDevice(
    const QBluetoothDeviceInfo &device
    )
{
    // ---------------------------------------------------
    // Sprzątanie po ewentualnym poprzednim połączeniu
    // ---------------------------------------------------

    if (m_service != nullptr)
    {
        // Odłączamy sygnały starego service od BleClient.
        m_service->disconnect(this);

        m_service->deleteLater();
        m_service = nullptr;
    }


    // Czyścimy poprzednio zapamiętaną characteristic.
    m_dataCharacteristic =
        QLowEnergyCharacteristic();


    if (m_controller != nullptr)
    {
        // Odłączamy sygnały starego controllera.
        m_controller->disconnect(this);

        m_controller->disconnectFromDevice();
        m_controller->deleteLater();

        m_controller = nullptr;
    }


    emit statusChanged(
        "Connecting to "
        + device.name()
        + "..."
        );


    // ---------------------------------------------------
    // Tworzymy BLE Central.
    //
    // PC/Qt  = Central
    // ESP32   = Peripheral
    // ---------------------------------------------------

    m_controller =
        QLowEnergyController::createCentral(
            device,
            this
            );


    // ===================================================
    // 1. POŁĄCZONO Z ESP32
    // ===================================================

    connect(
        m_controller,
        &QLowEnergyController::connected,
        this,
        [this]()
        {
            emit statusChanged(
                "Connected - discovering services..."
                );

            // Pytamy ESP32 o dostępne usługi GATT.
            m_controller->discoverServices();
        }
        );


    // ===================================================
    // 2. ZNALEZIONO SERVICE
    // ===================================================

    connect(
        m_controller,
        &QLowEnergyController::serviceDiscovered,
        this,
        [this](const QBluetoothUuid &uuid)
        {
            // Wysyłamy UUID do MainWindow,
            // żeby pojawił się w logu.
            emit serviceFound(uuid);
        }
        );


    // ===================================================
    // 3. ZAKOŃCZONO WYSZUKIWANIE SERVICES
    // ===================================================

    connect(
        m_controller,
        &QLowEnergyController::discoveryFinished,
        this,
        [this]()
        {
            emit statusChanged(
                "Service discovery finished"
                );


            // -------------------------------------------
            // UUID naszego własnego Service z ESP32
            // -------------------------------------------

            const QBluetoothUuid imuServiceUuid(
                QUuid(
                    QStringLiteral(
                        "{323c3f52-e193-e1ae-324c-32858741567e}"
                        )
                    )
                );


            // -------------------------------------------
            // Tworzymy obiekt reprezentujący
            // konkretny Service.
            // -------------------------------------------

            m_service =
                m_controller->createServiceObject(
                    imuServiceUuid,
                    this
                    );


            if (m_service == nullptr)
            {
                emit statusChanged(
                    "IMU service not found"
                    );

                return;
            }


            emit statusChanged(
                "IMU service found - discovering characteristics..."
                );


            // =================================================
            // 4. CZEKAMY NA ODKRYCIE CHARACTERISTICS
            // =================================================

            connect(
                m_service,
                &QLowEnergyService::stateChanged,
                this,
                [this](
                    QLowEnergyService::ServiceState state
                    )
                {
                    // Dopóki service nie został w pełni
                    // odkryty, nic więcej nie robimy.
                    if (
                        state !=
                        QLowEnergyService::
                        RemoteServiceDiscovered
                        )
                    {
                        return;
                    }


                    emit statusChanged(
                        "Characteristics discovered"
                        );


                    // -----------------------------------------
                    // UUID characteristic przesyłającej dane
                    // -----------------------------------------

                    const QBluetoothUuid
                        dataCharacteristicUuid(
                            QUuid(
                                QStringLiteral(
                                    "{7754e8c8-c45e-5394-1140-f45d92e69af9}"
                                    )
                                )
                            );


                    // Pobieramy wszystkie characteristics
                    // należące do naszego Service.
                    const auto characteristics =
                        m_service->characteristics();


                    // =================================================
                    // 5. PRZEGLĄDAMY CHARACTERISTICS
                    // =================================================

                    for (
                        const auto &characteristic :
                        characteristics
                        )
                    {
                        QString properties;


                        if (
                            characteristic.properties()
                            & QLowEnergyCharacteristic::Read
                            )
                        {
                            properties += "READ ";
                        }


                        if (
                            characteristic.properties()
                            & QLowEnergyCharacteristic::Write
                            )
                        {
                            properties += "WRITE ";
                        }


                        if (
                            characteristic.properties()
                            & QLowEnergyCharacteristic::
                            WriteNoResponse
                            )
                        {
                            properties += "WRITE_NR ";
                        }


                        if (
                            characteristic.properties()
                            & QLowEnergyCharacteristic::Notify
                            )
                        {
                            properties += "NOTIFY ";
                        }


                        if (
                            characteristic.properties()
                            & QLowEnergyCharacteristic::Indicate
                            )
                        {
                            properties += "INDICATE ";
                        }


                        // Pokazujemy znalezioną characteristic
                        // w logu aplikacji.
                        emit characteristicFound(
                            "Characteristic: "
                            + characteristic.uuid().toString()
                            + " | "
                            + properties
                            );


                        // Czy to dokładnie characteristic
                        // odpowiedzialna za dane IMU?
                        if (
                            characteristic.uuid()
                            == dataCharacteristicUuid
                            )
                        {
                            m_dataCharacteristic =
                                characteristic;
                        }
                    }


                    // =================================================
                    // 6. SPRAWDZAMY, CZY ZNALEŹLIŚMY
                    //    CHARACTERISTIC DANYCH
                    // =================================================

                    if (!m_dataCharacteristic.isValid())
                    {
                        emit statusChanged(
                            "Data characteristic not found"
                            );

                        return;
                    }


                    // =================================================
                    // 7. ODBIÓR NOTIFICATIONS
                    // =================================================

                    connect(
                        m_service,
                        &QLowEnergyService::
                        characteristicChanged,
                        this,
                        [this](
                            const QLowEnergyCharacteristic
                                &characteristic,
                            const QByteArray &value
                            )
                        {
                            // characteristicChanged może
                            // potencjalnie dotyczyć różnych
                            // characteristics.
                            //
                            // Sprawdzamy więc UUID.
                            if (
                                characteristic.uuid()
                                ==
                                m_dataCharacteristic.uuid()
                                )
                            {
                                // value zawiera surowe bajty
                                // przesłane przez ESP32.
                                emit dataReceived(value);
                            }
                        }
                        );


                    // =================================================
                    // 8. SZUKAMY DESCRIPTORA CCCD
                    // =================================================

                    const QLowEnergyDescriptor
                        notificationDescriptor =
                        m_dataCharacteristic.descriptor(
                            QBluetoothUuid(
                                QBluetoothUuid::
                                DescriptorType::
                                ClientCharacteristicConfiguration
                                )
                            );


                    // CCCD jest potrzebny,
                    // żeby klient zapisał się na NOTIFY.
                    if (!notificationDescriptor.isValid())
                    {
                        emit statusChanged(
                            "Notification descriptor not found"
                            );

                        return;
                    }


                    // =================================================
                    // 9. POTWIERDZENIE ZAPISANIA CCCD
                    // =================================================

                    connect(
                        m_service,
                        &QLowEnergyService::descriptorWritten,
                        this,
                        [this](
                            const QLowEnergyDescriptor &,
                            const QByteArray &
                            )
                        {
                            emit statusChanged(
                                "Notifications enabled"
                                );
                        }
                        );


                    // =================================================
                    // 10. WŁĄCZAMY NOTIFICATIONS
                    // =================================================

                    m_service->writeDescriptor(
                        notificationDescriptor,
                        QByteArray::fromHex("0100")
                        );
                }
                );


            // Bardzo ważne:
            //
            // najpierw connect(stateChanged),
            // a dopiero potem discoverDetails().
            m_service->discoverDetails();
        }
        );


    // ===================================================
    // 11. ROZŁĄCZENIE
    // ===================================================

    connect(
        m_controller,
        &QLowEnergyController::disconnected,
        this,
        [this]()
        {
            emit statusChanged(
                "Disconnected"
                );
        }
        );


    // ===================================================
    // 12. BŁĄD POŁĄCZENIA
    // ===================================================

    connect(
        m_controller,
        &QLowEnergyController::errorOccurred,
        this,
        [this](
            QLowEnergyController::Error
            )
        {
            emit statusChanged(
                "Connection error: "
                + m_controller->errorString()
                );
        }
        );


    // ===================================================
    // 13. FAKTYCZNE ROZPOCZĘCIE POŁĄCZENIA
    // ===================================================

    m_controller->connectToDevice();
}