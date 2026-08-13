cmake_minimum_required(VERSION 3.19)

project(Tls401RfidCcdDemo VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

if(MSVC)
    # Keep source and execution character sets explicit for VS2017.  The
    # shorter /utf-8 switch is not consistently honored by older Qt Creator
    # kits when compiling QStringLiteral text.
    add_compile_options(/source-charset:utf-8 /execution-charset:utf-8)
endif()

set(IDS_PEAK_ROOT "$ENV{IDS_PEAK_ROOT}" CACHE PATH "IDS peak SDK root")
set(RFID_SDK_ROOT "$ENV{RFID_SDK_ROOT}" CACHE PATH "RFID C++ SDK root")
set(RFID_RUNTIME_ROOT "$ENV{RFID_RUNTIME_ROOT}" CACHE PATH "RFID x86 runtime root containing rfidlib_reader.dll and Drivers")

# The target kit is Qt 5.14.2 built with MSVC 2017 for Win32.
find_package(Qt5 5.14 REQUIRED COMPONENTS Core Gui Widgets Sql Test)
include(CTest)
include(cmake/FindRfidSdk.cmake)
include(cmake/FindIdsPeakSdk.cmake)

add_executable(tls401_demo
    WIN32
    src/main.cpp
    src/domain/models.h
    src/devices/deviceinterfaces.h
    src/devices/mockdevices.h
    src/devices/mockdevices.cpp
    src/app/workflowservices.h
    src/app/workflowservices.cpp
    src/app/applicationcontroller.h
    src/app/applicationcontroller.cpp
    src/storage/database.h
    src/storage/database.cpp
    src/storage/repository.h
    src/storage/repository.cpp
    src/storage/imagefilestore.h
    src/storage/imagefilestore.cpp
    src/devices/realdevices.h
    src/devices/realdevices.cpp
    src/devices/rfidpayloadcodec.h
    src/devices/rfidpayloadcodec.cpp
    src/ui/mainwindow.h
    src/ui/mainwindow.cpp
    src/ui/mainwindow.ui
    resources/resources.qrc
)
target_include_directories(tls401_demo PRIVATE src)
target_link_libraries(tls401_demo PRIVATE Qt5::Core Qt5::Gui Qt5::Widgets Qt5::Sql)
target_compile_definitions(tls401_demo PRIVATE
    $<$<BOOL:${RFID_SDK_FOUND}>:TLS401_HAS_RFID_SDK>
    $<$<BOOL:${IDS_PEAK_FOUND}>:TLS401_HAS_IDS_PEAK>)
if(RFID_SDK_FOUND)
    target_link_libraries(tls401_demo PRIVATE RfidSdk::Reader RfidSdk::Iso15693 RfidSdk::Driver)
endif()
if(IDS_PEAK_FOUND)
    target_link_libraries(tls401_demo PRIVATE IdsPeak::Sdk)
endif()

if(WIN32)
    find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "${Qt5_DIR}/../../../bin")
    if(WINDEPLOYQT_EXECUTABLE)
        add_custom_target(deploy_runtime
            COMMAND "${WINDEPLOYQT_EXECUTABLE}" --no-translations --compiler-runtime "$<TARGET_FILE:tls401_demo>"
            COMMENT "Deploying Qt runtime for tls401_demo")
    else()
        add_custom_target(deploy_runtime
            COMMAND ${CMAKE_COMMAND} -E echo "windeployqt was not found; set CMAKE_PREFIX_PATH to a Qt SDK containing it.")
    endif()
    add_dependencies(deploy_runtime tls401_demo)

    if(IDS_PEAK_FOUND)
        add_custom_command(TARGET tls401_demo POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory "${IDS_PEAK_API_RUNTIME_DIR}" "$<TARGET_FILE_DIR:tls401_demo>"
            COMMAND ${CMAKE_COMMAND} -E copy_directory "${IDS_PEAK_IPL_RUNTIME_DIR}" "$<TARGET_FILE_DIR:tls401_demo>"
            COMMAND ${CMAKE_COMMAND} -E copy_directory "${IDS_PEAK_COMFORT_RUNTIME_DIR}" "$<TARGET_FILE_DIR:tls401_demo>"
            COMMENT "Deploying IDS Peak x86 runtime")
    endif()

    if(RFID_SDK_FOUND AND RFID_RUNTIME_ROOT)
        if(EXISTS "${RFID_RUNTIME_ROOT}/rfidlib_reader.dll" AND EXISTS "${RFID_RUNTIME_ROOT}/Drivers")
            add_custom_command(TARGET tls401_demo POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_directory "${RFID_RUNTIME_ROOT}" "$<TARGET_FILE_DIR:tls401_demo>"
                COMMENT "Deploying RFID runtime")
        else()
            message(WARNING "RFID_RUNTIME_ROOT is set but does not contain x86 rfidlib_reader.dll and Drivers.")
        endif()
    elseif(RFID_SDK_FOUND)
        message(STATUS "RFID x86 runtime is not configured; set RFID_RUNTIME_ROOT before packaging a real-device build.")
    endif()
endif()

add_executable(dependency_probe probes/dependencyprobe.cpp)
target_link_libraries(dependency_probe PRIVATE Qt5::Core Qt5::Widgets Qt5::Sql)
target_compile_definitions(dependency_probe PRIVATE
    $<$<BOOL:${RFID_SDK_FOUND}>:TLS401_HAS_RFID_SDK>
    $<$<BOOL:${IDS_PEAK_FOUND}>:TLS401_HAS_IDS_PEAK>
)
if(RFID_SDK_FOUND)
    target_link_libraries(dependency_probe PRIVATE RfidSdk::Reader RfidSdk::Iso15693 RfidSdk::Driver)
endif()
if(IDS_PEAK_FOUND)
    target_link_libraries(dependency_probe PRIVATE IdsPeak::Sdk)
endif()

if(BUILD_TESTING)
    add_executable(tst_workflow
        tests/tst_workflow.cpp
        src/domain/models.h
        src/devices/deviceinterfaces.h
        src/devices/mockdevices.h
        src/devices/mockdevices.cpp
        src/app/workflowservices.h
        src/app/workflowservices.cpp
        src/app/applicationcontroller.h
        src/app/applicationcontroller.cpp
        src/devices/realdevices.h
        src/devices/realdevices.cpp
        src/devices/rfidpayloadcodec.h
        src/devices/rfidpayloadcodec.cpp
    )
    target_include_directories(tst_workflow PRIVATE src)
    target_sources(tst_workflow PRIVATE
        src/storage/database.h
        src/storage/database.cpp
        src/storage/repository.h
        src/storage/repository.cpp
        src/storage/imagefilestore.h
        src/storage/imagefilestore.cpp)
    target_link_libraries(tst_workflow PRIVATE Qt5::Core Qt5::Gui Qt5::Sql Qt5::Test)
    add_test(NAME workflow COMMAND tst_workflow)
    set_tests_properties(workflow PROPERTIES ENVIRONMENT "PATH=${Qt5_DIR}/../../../bin;$ENV{PATH}")
endif()
