cmake_minimum_required(VERSION 3.20)
project(ChessEngine CXX)
set(CMAKE_CXX_STANDARD 20)


# Find required packages
find_package(OpenSSL REQUIRED)
find_package(nlohmann_json REQUIRED)

# Use pkg-config to find Paho MQTT
find_package(PkgConfig REQUIRED)
pkg_check_modules(PAHO_MQTT_C libpaho-mqtt3as)
pkg_check_modules(PAHO_MQTT_CPP libpaho-mqttpp3)

if(NOT PAHO_MQTT_C_FOUND OR NOT PAHO_MQTT_CPP_FOUND)
    message(STATUS "Paho MQTT libraries not found via pkg-config, trying direct linking...")
    set(PAHO_MQTT_C_LIBRARIES paho-mqtt3as)
    set(PAHO_MQTT_CPP_LIBRARIES paho-mqttpp3)
    set(PAHO_MQTT_C_INCLUDE_DIRS /usr/include)
    set(PAHO_MQTT_CPP_INCLUDE_DIRS /usr/include)
endif()

# Chess library
add_library(chess STATIC
        src/board/Board.cpp
        src/rules/Attack.cpp
        src/rules/Castling.cpp
        src/rules/MoveValid.cpp
        src/rules/MoveExec.cpp
        src/model/Move.cpp
        src/rules/MoveGenerator.cpp
        src/game/GameState.cpp
        src/ai/Evaluator.cpp
        src/ai/TranspositionTable.cpp
        src/ai/ZobristHash.cpp
        src/ai/ChessAI.cpp
)

target_include_directories(chess PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

# MQTT Client executable
add_executable(chess_cli 
        src/mqtt/main_mqtt.cpp
        src/mqtt/mqtt_client.cpp
)

target_include_directories(chess_cli PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${PAHO_MQTT_C_INCLUDE_DIRS}
        ${PAHO_MQTT_CPP_INCLUDE_DIRS}
)

target_link_libraries(chess_cli PRIVATE
    chess
    OpenSSL::SSL
    OpenSSL::Crypto
    ${PAHO_MQTT_C_LIBRARIES}
    ${PAHO_MQTT_CPP_LIBRARIES}
    nlohmann_json::nlohmann_json
    pthread
)

# Set RPATH for the binary
set_target_properties(chess_cli PROPERTIES
    INSTALL_RPATH "$ORIGIN/../lib"
    BUILD_WITH_INSTALL_RPATH TRUE)

