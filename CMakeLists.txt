cmake_minimum_required(VERSION 3.19)
project(asd LANGUAGES CXX)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets Bluetooth Charts)


qt_standard_project_setup()

qt_add_executable(asd
    WIN32 MACOSX_BUNDLE
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
    bleclient.h bleclient.cpp
    imusample.h
    imupacketparser.h imupacketparser.cpp
    motionmapper.cpp motionmapper.h
    motiontestwidget.cpp motiontestwidget.h
    motionmapper.h motionmapper.cpp
    motiontestwidget.h motiontestwidget.cpp
    drawingstatefilter.h

)

target_link_libraries(asd
    PRIVATE
        Qt::Core
        Qt::Widgets
        Qt::Bluetooth
        Qt6::Charts
)

install(TARGETS asd
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET asd
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
