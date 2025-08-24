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
)

target_include_directories(chess PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_executable(chess_cli src/app/main.cpp)
target_link_libraries(chess_cli PRIVATE chess)
