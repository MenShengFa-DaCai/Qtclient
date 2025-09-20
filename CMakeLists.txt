cmake_minimum_required(VERSION 4.0)
project(Qtclient)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(CMAKE_PREFIX_PATH "C:/Qt/QtMax/6.9.2/mingw_64")

find_package(Qt6 COMPONENTS
        Core
        Gui
        Widgets
        Network
        Mqtt
        REQUIRED)

add_executable(Qtclient main.cpp
        uiClass/SearchUpgrade/searchupgrade.cpp
        uiClass/SearchUpgrade/searchupgrade.h
        uiClass/SearchUpgrade/searchupgrade.ui
        uiClass/LinkPush/linkpush.cpp
        uiClass/LinkPush/linkpush.h
        uiClass/LinkPush/linkpush.ui
        uiClass/Login/login.cpp
        uiClass/Login/login.h
        uiClass/Login/login.ui
        uiClass/MainWidget/mainwidget.cpp
        uiClass/MainWidget/mainwidget.h
        uiClass/MainWidget/mainwidget.ui
        src/FileSenderThread.cpp
        inc/FileSenderThread.h
        uiClass/WaterHeater/waterheater.cpp
        uiClass/WaterHeater/waterheater.h
        uiClass/WaterHeater/waterheater.ui
        uiClass/ThermoHygroHistory/thermohygrohistory.cpp
        uiClass/ThermoHygroHistory/thermohygrohistory.h
        uiClass/ThermoHygroHistory/thermohygrohistory.ui
        uiClass/Infrared/infrared.cpp
        uiClass/Infrared/infrared.h
        uiClass/Infrared/infrared.ui)
target_link_libraries(Qtclient
        Qt::Core
        Qt::Gui
        Qt::Widgets
        Qt::Network
        Qt::Mqtt
)

include_directories(inc)

if (WIN32 AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(DEBUG_SUFFIX)
    if (MSVC AND CMAKE_BUILD_TYPE MATCHES "Debug")
        set(DEBUG_SUFFIX "d")
    endif ()
    set(QT_INSTALL_PATH "${CMAKE_PREFIX_PATH}")
    if (NOT EXISTS "${QT_INSTALL_PATH}/bin")
        set(QT_INSTALL_PATH "${QT_INSTALL_PATH}/..")
        if (NOT EXISTS "${QT_INSTALL_PATH}/bin")
            set(QT_INSTALL_PATH "${QT_INSTALL_PATH}/..")
        endif ()
    endif ()
    if (EXISTS "${QT_INSTALL_PATH}/plugins/platforms/qwindows${DEBUG_SUFFIX}.dll")
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E make_directory
                "$<TARGET_FILE_DIR:${PROJECT_NAME}>/plugins/platforms/")
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy
                "${QT_INSTALL_PATH}/plugins/platforms/qwindows${DEBUG_SUFFIX}.dll"
                "$<TARGET_FILE_DIR:${PROJECT_NAME}>/plugins/platforms/")
    endif ()
    foreach (QT_LIB Core Gui Widgets Network Mqtt)
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy
                "${QT_INSTALL_PATH}/bin/Qt6${QT_LIB}${DEBUG_SUFFIX}.dll"
                "$<TARGET_FILE_DIR:${PROJECT_NAME}>")
    endforeach (QT_LIB)
endif ()
