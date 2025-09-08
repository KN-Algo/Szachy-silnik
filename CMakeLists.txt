cmake_minimum_required(VERSION 3.20)
project(Szachy-silnik CXX)
set(CMAKE_CXX_STANDARD 20)

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

add_executable(chess_cli src/app/main.cpp)
target_link_libraries(chess_cli PRIVATE chess)

find_package(PahoMqttCpp CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

add_executable(engine_mqtt
        src/mqtt/main_mqtt.cpp
        src/mqtt/mqtt_client.cpp
)

target_include_directories(engine_mqtt PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(engine_mqtt PRIVATE
        chess
        PahoMqttCpp::paho-mqttpp3
        nlohmann_json::nlohmann_json
)

