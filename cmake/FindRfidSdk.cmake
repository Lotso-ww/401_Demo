include(FindPackageHandleStandardArgs)

set(_rfid_include "${RFID_SDK_ROOT}/inc")
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(_rfid_lib_dir "${RFID_SDK_ROOT}/lib_win32_release")
else()
    set(_rfid_lib_dir "${RFID_SDK_ROOT}/lib_win32_debug")
endif()

find_path(RFID_SDK_INCLUDE_DIR rfidlib.h PATHS "${_rfid_include}" NO_DEFAULT_PATH)
find_library(RFID_READER_LIBRARY rfidlib_reader PATHS "${_rfid_lib_dir}" NO_DEFAULT_PATH)
find_library(RFID_ISO15693_LIBRARY rfidlib_aip_iso15693 PATHS "${_rfid_lib_dir}" NO_DEFAULT_PATH)
find_library(RFID_DRIVER_LIBRARY rfidlib_drv_RL8000 PATHS "${_rfid_lib_dir}/device_driver" "${_rfid_lib_dir}" NO_DEFAULT_PATH)

find_package_handle_standard_args(RfidSdk
    REQUIRED_VARS RFID_SDK_INCLUDE_DIR RFID_READER_LIBRARY RFID_ISO15693_LIBRARY RFID_DRIVER_LIBRARY
    FAIL_MESSAGE "RFID Win32 SDK is incomplete. Set RFID_SDK_ROOT to the c++_lib directory containing inc and lib_win32_{debug,release}.")

if(RfidSdk_FOUND)
    set(RFID_SDK_FOUND TRUE)
    add_library(RfidSdk::Reader UNKNOWN IMPORTED)
    set_target_properties(RfidSdk::Reader PROPERTIES IMPORTED_LOCATION "${RFID_READER_LIBRARY}" INTERFACE_INCLUDE_DIRECTORIES "${RFID_SDK_INCLUDE_DIR}")
    add_library(RfidSdk::Iso15693 UNKNOWN IMPORTED)
    set_target_properties(RfidSdk::Iso15693 PROPERTIES IMPORTED_LOCATION "${RFID_ISO15693_LIBRARY}")
    add_library(RfidSdk::Driver UNKNOWN IMPORTED)
    set_target_properties(RfidSdk::Driver PROPERTIES IMPORTED_LOCATION "${RFID_DRIVER_LIBRARY}")
else()
    set(RFID_SDK_FOUND FALSE)
    message(WARNING "RFID dependency probe disabled: ${RfidSdk_NOT_FOUND_MESSAGE}")
endif()
