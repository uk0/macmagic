cmake_minimum_required(VERSION 3.21)
project(MacMagic VERSION 2.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# Enable Objective-C++ for .mm files
enable_language(OBJCXX)
set(CMAKE_OBJCXX_STANDARD 17)

# Default deployment target if not set externally (CI sets this)
if(NOT CMAKE_OSX_DEPLOYMENT_TARGET)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "13.0" CACHE STRING "Minimum macOS version")
endif()

# Qt6 path — prefer CMAKE_PREFIX_PATH from CLI/CI, fallback to Homebrew
if(NOT DEFINED CMAKE_PREFIX_PATH OR CMAKE_PREFIX_PATH STREQUAL "")
    list(APPEND CMAKE_PREFIX_PATH "/opt/homebrew/opt/qt")
endif()

find_package(Qt6 REQUIRED COMPONENTS Widgets Network)

set(SOURCES
    src/main.cpp
    src/app.h
    src/app.cpp
    src/traymanager.h
    src/traymanager.cpp
    src/macchanger.h
    src/macchanger.cpp
    src/networkinterface.h
    src/networkinterface.cpp
    src/macgenerator.h
    src/macgenerator.cpp
    src/networkinfo.h
    src/networkinfo.cpp
    src/arpspoofer.h
    src/arpspoofer.cpp
    src/dhcpclient.h
    src/dhcpclient.cpp
    src/wifiinfo.h
    src/wifiinfo.mm
    src/iconprovider.h
    src/iconprovider.cpp
)

set(RESOURCES
    resources/macmagic.qrc
)

qt_add_executable(MacMagic MACOSX_BUNDLE
    ${SOURCES}
    ${RESOURCES}
)

set_target_properties(MacMagic PROPERTIES
    MACOSX_BUNDLE TRUE
    MACOSX_BUNDLE_INFO_PLIST "${CMAKE_SOURCE_DIR}/resources/Info.plist"
    MACOSX_BUNDLE_BUNDLE_NAME "MacMagic"
    MACOSX_BUNDLE_GUI_IDENTIFIER "com.macmagic.app"
)

target_link_libraries(MacMagic PRIVATE Qt6::Widgets Qt6::Network pcap
    "-framework CoreWLAN" "-framework Foundation")

add_subdirectory(tests)
