#!/bin/bash

echo "Kompilacja silnika szachowego..."

# Sprawdź czy g++ jest dostępny
if ! command -v g++ &> /dev/null; then
    echo "Błąd: g++ nie jest zainstalowany"
    echo "Zainstaluj: sudo apt install build-essential"
    exit 1
fi

# Kompilacja
g++ -std=c++20 -O2 -I include -o chess_cli \
    src/app/main.cpp \
    src/board/Board.cpp \
    src/rules/Attack.cpp \
    src/rules/Castling.cpp \
    src/rules/MoveValid.cpp \
    src/rules/MoveExec.cpp \
    src/model/Move.cpp \
    src/rules/MoveGenerator.cpp \
    src/game/GameState.cpp \
    src/ai/Evaluator.cpp \
    src/ai/TranspositionTable.cpp \
    src/ai/ZobristHash.cpp \
    src/ai/ChessAI.cpp

if [ $? -eq 0 ]; then
    echo "Kompilacja zakończona sukcesem!"
    echo "Uruchom program: ./chess_cli"
    echo "Lub przetestuj AI: echo 'ai' | ./chess_cli"
else
    echo "Błąd kompilacji!"
    exit 1
fi
