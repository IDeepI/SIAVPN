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
if(NOT Qt6_FOUND)
    message(FATAL_ERROR "Qt6 not found. Please install Qt6.")
endif()

# Explicitly list source files for better control
set(CORE_SRC_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/openVpnProtocol.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/vpnConnectionManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/vpnConfigManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/vpnSecurityManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/openVpnClient.cpp
)

set(UI_SRC_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/vpnController.cpp
)

set(MAIN_SRC_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp
)

add_executable(${PROJECT_NAME} 
    ${MAIN_SRC_FILES}
    ${CORE_SRC_FILES}
    ${UI_SRC_FILES}
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/main.qml
)

target_include_directories(${PROJECT_NAME} PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui
)

target_link_libraries(${PROJECT_NAME}
    Qt6::Core
    Qt6::Quick
)

# Set output directory
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin
)
