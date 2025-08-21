cmake_minimum_required(VERSION 3.30)
project(siavpn LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_SCAN_FOR_MODULES ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC OFF)
set(CMAKE_AUTOUIC OFF)

find_package(Qt6 COMPONENTS Quick REQUIRED)
find_package(openvpn3 REQUIRED)
if(NOT Qt6_FOUND)
    message(FATAL_ERROR "Qt6 not found. Please install Qt6.")
endif()

if(NOT openvpn3_FOUND)
    message(FATAL_ERROR "OpenVPN 3 not found. Please install OpenVPN 3.")
endif()

file(GLOB SRC_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/*.cpp
)

add_executable(${PROJECT_NAME} ${SRC_FILES} ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/main.qml)

target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(${PROJECT_NAME}
    Qt6::Core
    Qt6::Quick
    openvpn3::openvpn3
)

# Set output directory
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin
)
