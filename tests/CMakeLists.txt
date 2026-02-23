cmake_minimum_required(VERSION 3.21)

find_package(Qt6 REQUIRED COMPONENTS Test Widgets Network)

enable_testing()

qt_add_executable(test_macgenerator
    test_macgenerator.cpp
    ${CMAKE_SOURCE_DIR}/src/macgenerator.cpp
)
target_include_directories(test_macgenerator PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(test_macgenerator PRIVATE Qt6::Test Qt6::Widgets)
add_test(NAME test_macgenerator COMMAND test_macgenerator)

qt_add_executable(test_networkinterface
    test_networkinterface.cpp
    ${CMAKE_SOURCE_DIR}/src/networkinterface.cpp
)
target_include_directories(test_networkinterface PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(test_networkinterface PRIVATE Qt6::Test Qt6::Widgets)
add_test(NAME test_networkinterface COMMAND test_networkinterface)

qt_add_executable(test_networkinfo
    test_networkinfo.cpp
    ${CMAKE_SOURCE_DIR}/src/networkinfo.cpp
)
target_include_directories(test_networkinfo PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(test_networkinfo PRIVATE Qt6::Test Qt6::Widgets)
add_test(NAME test_networkinfo COMMAND test_networkinfo)

qt_add_executable(test_arpspoofer
    test_arpspoofer.cpp
    ${CMAKE_SOURCE_DIR}/src/arpspoofer.cpp
    ${CMAKE_SOURCE_DIR}/src/networkinfo.cpp
)
target_include_directories(test_arpspoofer PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(test_arpspoofer PRIVATE Qt6::Test Qt6::Widgets pcap)
add_test(NAME test_arpspoofer COMMAND test_arpspoofer)

qt_add_executable(test_dhcpclient
    test_dhcpclient.cpp
    ${CMAKE_SOURCE_DIR}/src/dhcpclient.cpp
)
target_include_directories(test_dhcpclient PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(test_dhcpclient PRIVATE Qt6::Test Qt6::Widgets Qt6::Network pcap)
add_test(NAME test_dhcpclient COMMAND test_dhcpclient)
