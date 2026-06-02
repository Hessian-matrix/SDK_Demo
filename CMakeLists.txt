cmake_minimum_required(VERSION 3.10)

project(Viobot2SdkDemo LANGUAGES CXX)

find_package(Threads REQUIRED)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT UNIX OR APPLE)
    message(FATAL_ERROR "This CMake project only supports Linux/GCC builds.")
endif()

string(TOLOWER "${CMAKE_SYSTEM_PROCESSOR}" VIOBOT_SYSTEM_PROCESSOR)

if(VIOBOT_SYSTEM_PROCESSOR MATCHES "^(x86_64|amd64)$")
    if(CMAKE_SIZEOF_VOID_P EQUAL 4)
        set(VIOBOT_SDK_ARCH_DIR "i386")
    else()
        set(VIOBOT_SDK_ARCH_DIR "x86_64")
    endif()
elseif(VIOBOT_SYSTEM_PROCESSOR MATCHES "^(i386|i686|x86)$")
    set(VIOBOT_SDK_ARCH_DIR "i386")
elseif(VIOBOT_SYSTEM_PROCESSOR MATCHES "^(aarch64|arm64)$")
    set(VIOBOT_SDK_ARCH_DIR "arm")
else()
    message(FATAL_ERROR
        "Unsupported processor architecture: ${CMAKE_SYSTEM_PROCESSOR}. "
        "Supported architectures: x86_64, i386, aarch64.")
endif()

set(VIOBOT_SDK_LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/lib/linux/${VIOBOT_SDK_ARCH_DIR}")
set(VIOBOT_SDK_LIB "${VIOBOT_SDK_LIB_DIR}/libvio_sdk.so")

if(NOT EXISTS "${VIOBOT_SDK_LIB}")
    message(FATAL_ERROR "Viobot SDK library not found: ${VIOBOT_SDK_LIB}")
endif()

message(STATUS "Processor architecture: ${CMAKE_SYSTEM_PROCESSOR}")
message(STATUS "Using Viobot SDK library: ${VIOBOT_SDK_LIB}")

add_executable(viobot_demo
    src/algorithm_status.cpp
    src/channel1_stream.cpp
    src/console.cpp
    src/demo_app.cpp
    src/pose.cpp
    src/viobot_demo.cpp)

target_include_directories(viobot_demo PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/include")

target_link_libraries(viobot_demo PRIVATE
    "-L${VIOBOT_SDK_LIB_DIR}"
    vio_sdk
    Threads::Threads)

set_target_properties(viobot_demo PROPERTIES
    BUILD_RPATH "${VIOBOT_SDK_LIB_DIR}"
    INSTALL_RPATH "$ORIGIN/../lib/linux/${VIOBOT_SDK_ARCH_DIR}")

install(TARGETS viobot_demo
    RUNTIME DESTINATION bin)

install(FILES "${VIOBOT_SDK_LIB}"
    DESTINATION "lib/linux/${VIOBOT_SDK_ARCH_DIR}")
