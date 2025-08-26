# Chess Engine

Silnik szachowy z implementacją AI gracza.

---

## Implementacja

### Algorytm AI
- **NegaMax** z **Alfa-Beta Pruning**
- **Iterative Deepening** (głębokość od 1 do maksymalnej)
- **Transposition Tables** dla optymalizacji wyszukiwania
- **Move Ordering** dla lepszego Alfa-Beta Pruning

### Funkcjonalności
- Pełna implementacja reguł szachowych
- Roszada (krótka i długa)
- Bicie w przelocie (en passant)
- Promocja pionów
- Wykrywanie mata, pata i remisu
- Ocena pozycji z uwzględnieniem materiału i pozycji

### Notacja
Projekt używa FEN - Forsyth–Edwards Notation

[Read more about FEN](https://en.wikipedia.org/wiki/Forsyth–Edwards_Notation)

---

## Kompilacja

```bash
g++ -std=c++20 -I include -o chess_cli src/app/main.cpp src/board/Board.cpp src/rules/Attack.cpp src/rules/Castling.cpp src/rules/MoveValid.cpp src/rules/MoveExec.cpp src/model/Move.cpp src/rules/MoveGenerator.cpp src/game/GameState.cpp src/ai/Evaluator.cpp src/ai/TranspositionTable.cpp src/ai/ZobristHash.cpp src/ai/ChessAI.cpp
```

## Użytkowanie

### Uruchomienie
```bash
./chess_cli
```

### Komendy
- `e2e4` - wykonaj ruch w notacji LAN
- `e7e8Q` - promocja piona do hetmana
- `perft <depth>` - test perft dla danej głębokości
- `ai` - AI znajdzie i wykona najlepszy ruch
- `quit` - wyjście z programu

### Przykład gry z AI
1. Uruchom program: `./chess_cli`
2. Wykonaj swój ruch: `e2e4`
3. Poproś AI o ruch: `ai`
4. AI znajdzie najlepszy ruch i go wykona
5. Kontynuuj grę naprzemiennie

### Parametry AI
- **Głębokość wyszukiwania**: domyślnie 4 (można zmienić)
- **Czas wyszukiwania**: domyślnie 5000ms (można zmienić)
- **Sortowanie ruchów**: bicia, promocje, ruchy do centrum
- **Tablica transpozycji**: cache dla odwiedzonych pozycji

---

## Architektura

### Struktura plików
```
include/chess/
├── ai/           # AI i algorytmy wyszukiwania
├── board/        # Reprezentacja planszy
├── game/         # Stan gry i zarządzanie
├── model/        # Modele danych (Move)
├── rules/        # Reguły gry i generowanie ruchów
└── utils/        # Narzędzia (notacja)

src/              # Implementacje
```

### Klasy AI
- `ChessAI` - główna klasa AI
- `Evaluator` - ocena pozycji
- `TranspositionTable` - tablica transpozycji
- `ZobristHash` - hash pozycji

---

## Testy

Program przechodzi testy perft:
- perft(3) = 8902 ✓
- perft(4) = 197281 ✓

---

## Licencja

MIT License - zobacz plik LICENSE
