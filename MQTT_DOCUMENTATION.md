# Komunikacja MQTT – Silnik Szachowy ↔ Backend (strona silnika)

Dokument ten uzupełnia i rozszerza kontrakt komunikacyjny MQTT po stronie **silnika szachowego (C++)**.  
Zawiera **pełną listę topiców**, ich **kierunek**, **formaty payloadów** (minimalne i rozszerzone), opis **statusów silnika** oraz scenariusze szczególne (roszada, promocja, bicie w przelocie, szach, mat).  
Format jest w pełni zgodny z dokumentacją backendu.

---

## Legenda

- **Kierunek**: `BE → ENG` (Backend do Silnika) lub `ENG → BE` (Silnik do Backend)
- **Topic**: nazwa kanału MQTT
- **QoS**: domyślnie 1, `retain = false` (chyba że zaznaczono inaczej)
- **JSON**: wszystkie payloady są w formacie JSON UTF‑8

---

## Model statusu silnika

Silnik publikuje swój status w topicu `status/engine`.

### Wartości statusu
- `ready` — gotowy do przyjęcia zadań
- `thinking` — trwa obliczanie ruchu lub generacja listy ruchów
- `analyzing` — walidacja lub analiza ruchu
- `error` — błąd krytyczny (payload może zawierać pole `message` z opisem)

### Payload (minimalny)
```json
{ "status": "ready" }
```

### Payload (rozszerzony)
```json
{ "status": "thinking", "message": "generowanie listy ruchów" }
```

---

## Przegląd topiców

| Kierunek | Topic | Cel |
|---|---|---|
| **BE → ENG** | `move/engine` | Walidacja i zastosowanie ruchu gracza względem bieżącego FEN |
| **BE → ENG** | `engine/possible_moves/request` | Żądanie listy legalnych ruchów z danego pola |
| **BE → ENG** | `move/engine/request` | Żądanie ruchu AI (silnika) na podstawie FEN |
| **BE → ENG** | `control/restart/external` | Reset silnika (opcjonalnie z zadanym FEN) |
| **ENG → BE** | `engine/move/confirmed` | Ruch zaakceptowany i zastosowany (aktualizacja FEN) |
| **ENG → BE** | `engine/move/rejected` | Ruch odrzucony wraz z powodem |
| **ENG → BE** | `engine/possible_moves/response` | Odpowiedź z listą legalnych ruchów |
| **ENG → BE** | `status/engine` | Aktualizacje statusu pracy silnika |
| **ENG → BE** | `engine/reset/confirmed` | Potwierdzenie resetu silnika |
| **ENG → BE** | `move/ai` | Wynik ruchu obliczonego przez silnik (AI) |

> Uwaga:  
> Topic’i z prefiksem `engine/*` są zarezerwowane dla komunikacji po stronie silnika.  
> Backend pozostaje źródłem prawdy w zakresie sekwencji zadań i synchronizacji z fizyczną planszą.

---

## Format payloadów (C++ ↔ JSON)

Poniżej przedstawiono wzorcowe payloady używane przez silnik. Nazwy pól są **stabilne**, a pola opcjonalne oznaczono opisowo.

### 1) `status/engine` (ENG → BE)

**Minimalny**
```json
{ "status": "ready" }
```

**Rozszerzony**
```json
{ "status": "thinking", "message": "generowanie listy ruchów" }
```

---

### 2) `engine/possible_moves/request` (BE → ENG)

Żądanie listy możliwych ruchów z danego pola.
```json
{
  "position": "e2",
  "fen": "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"
}
```

**Odpowiedź** — `engine/possible_moves/response` (ENG → BE)

**Minimalna**
```json
{
  "position": "e2",
  "moves": ["e3", "e4"]
}
```

**Rozszerzona**
```json
{
  "position": "e2",
  "moves": ["e3", "e4"],
  "fen_used": "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"
}
```

Błędy:
- Jeśli `fen` jest pusty lub niepoprawny – silnik publikuje `status/engine` z `error` i komunikatem opisowym.

---

### 3) `move/engine` (BE → ENG) — Walidacja i zastosowanie ruchu

**Minimalny**
```json
{
  "from": "e2",
  "to": "e4",
  "current_fen": "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
  "type": "normal",
  "physical": false
}
```

**Rozszerzony**
```json
{
  "from": "e7",
  "to": "e8",
  "current_fen": "4k3/4P3/8/8/8/8/8/4K3 w - - 0 1",
  "type": "promotion",
  "physical": true,
  "promotion_piece": "queen",
  "available_pieces": ["queen", "rook", "bishop", "knight"],
  "captured_piece": "rook",
  "special_move": "promotion_capture"
}
```

**Ruch zaakceptowany** — `engine/move/confirmed` (ENG → BE)
```json
{
  "from": "e7",
  "to": "e8",
  "fen": "4k3/4Q3/8/8/8/8/8/4K3 b - - 0 1",
  "physical": true,
  "next_player": "black",
  "special_move": "promotion_capture",
  "promotion_piece": "queen",
  "gives_check": true,
  "notation": "exd8=Q+",
  "game_status": "playing"
}
```

**Ruch odrzucony** — `engine/move/rejected` (ENG → BE)
```json
{
  "from": "e7",
  "to": "e8",
  "fen": "4k3/4P3/8/8/8/8/8/4K3 w - - 0 1",
  "physical": true,
  "reason": "Promotion piece not available on physical board"
}
```

**Typowe powody odrzucenia**
- `Bad algebraic square` — błędne pole w notacji
- `Illegal move` — nielegalny ruch
- `Unknown promotion_piece` — nieznana figura promocji
- `Promotion piece not available on physical board` — brak figury na planszy
- `Bad current_fen` — błędny zapis FEN

---

### 4) Ruchy specjalne (`engine/move/confirmed`)

**Roszada krótka**
```json
{
  "from": "e1",
  "to": "g1",
  "fen": "rnbqkb1r/pppppppp/5n2/8/8/8/PPPPPPPP/RNBQK2R b KQkq - 1 1",
  "physical": false,
  "next_player": "black",
  "special_move": "castling_kingside",
  "additional_moves": [{ "from": "h1", "to": "f1", "piece": "rook" }],
  "notation": "0-0",
  "game_status": "playing"
}
```

**Roszada długa**
```json
{
  "from": "e1",
  "to": "c1",
  "fen": "rnbqk2r/pppppppp/5n2/8/8/8/PPPPPPPP/RNBQ1RK1 b kq - 1 1",
  "physical": false,
  "next_player": "black",
  "special_move": "castling_queenside",
  "additional_moves": [{ "from": "a1", "to": "d1", "piece": "rook" }],
  "notation": "0-0-0",
  "game_status": "playing"
}
```

**Bicie w przelocie**
```json
{
  "from": "e5",
  "to": "d6",
  "fen": "8/8/3P4/8/8/8/8/8 b - - 0 1",
  "physical": false,
  "special_move": "en_passant",
  "notation": "exd6",
  "game_status": "playing"
}
```

**Mat**
```json
{
  "from": "Qh5",
  "to": "xf7",
  "fen": "rnb1kbnr/pppp1Qpp/8/4p3/8/8/PPPPPPPP/RNB1KBNR b KQkq - 0 3",
  "notation": "Qxf7#",
  "game_status": "checkmate",
  "winner": "white",
  "game_ended": true
}
```

---

### 5) `move/engine/request` (BE → ENG) — Żądanie ruchu AI

```json
{
  "fen": "r1bqkbnr/pppp1ppp/2n5/4p3/4P3/5N2/PPPP1PPP/RNBQKB1R w KQkq - 2 3"
}
```

**Odpowiedź** — `move/ai` (ENG → BE)
```json
{
  "from": "d2",
  "to": "d4",
  "fen": "r1bqkbnr/pppp1ppp/2n5/4p3/3PP3/5N2/PPP2PPP/RNBQKB1R b KQkq d3 0 3",
  "next_player": "black",
  "notation": "d4",
  "game_status": "playing"
}
```

---

### 6) `control/restart/external` (BE → ENG)

Reset silnika do pozycji początkowej lub zadanego FEN.

**Minimalny**
```json
{}
```

**Z FEN**
```json
{ "fen": "startpos" }
```

**Odpowiedź** — `engine/reset/confirmed`
```json
{
  "type": "engine",
  "fen": "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
  "status": "ready"
}
```

---

## Obsługa błędów i cykl statusów

Silnik zawsze publikuje `status/engine` przy rozpoczęciu i zakończeniu zadania:
- `thinking` — generowanie ruchów lub obliczenia AI
- `analyzing` — analiza lub walidacja ruchu
- `ready` — po zakończeniu operacji
- `error` — w przypadku błędu z polem `message`

Backend po otrzymaniu `engine/move/rejected` powinien:
- powiadomić UI i/lub wywołać procedurę cofnięcia ruchu na fizycznej planszy,
- zachować spójność stanu FEN pomiędzy UI a silnikiem.

---

## Referencja pól

### MoveEngineReq (BE → ENG)
`from`, `to`, `current_fen`, `type`, `physical`, opcjonalnie:  
`promotion_piece`, `available_pieces`, `captured_piece`, `special_move`

### PossibleMovesReq (BE → ENG)
`position`, `fen`

### Move Confirmed (ENG → BE)
`from`, `to`, `fen`, `physical`, `next_player`, opcjonalnie:  
`special_move`, `additional_moves`, `promotion_piece`, `notation`, `gives_check`, `game_status`, `winner`, `game_ended`

### Move Rejected (ENG → BE)
`from`, `to`, `fen`, `physical`, `reason`

### PossibleMoves Response (ENG → BE)
`position`, `moves`, opcjonalnie `fen_used`

### Status (ENG → BE)
`status`, opcjonalnie `message`

---

## Uwagi końcowe

- FEN w komunikatach wychodzących z silnika jest źródłem prawdy o stanie gry.  
- W przypadku promocji z fizycznej planszy backend powinien przekazywać `available_pieces`.  
- Notacja SAN generowana jest po wykonaniu ruchu.  
- Backend odpowiada za deduplikację i kolejność komunikacji (silnik może wysłać ten sam stan FEN więcej niż raz).

---

## Ustawienia domyślne

- QoS: 1  
- retain: false  
- Client ID: `chess-engine` (można zmienić w zmiennych środowiskowych)  
- Logi skracają długie payloady dla czytelności.

---

## Wersjonowanie

- Wersja dokumentu: **1.0 – 2025‑10‑12**  
- Zmiany łamiące kompatybilność będą publikowane w changelogu repozytorium.

