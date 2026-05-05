# Medici – C++ Design Document

**Program name:** medici.cpp  
**Author:** Student  
**Date:** 5/5/2026  
**Purpose:** Console-based implementation of the Medici board game for 2–6 players.

---

## 1. Class Diagram

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                          CLASS DIAGRAM – Medici                             │
└─────────────────────────────────────────────────────────────────────────────┘

┌──────────────────────┐
│  <<enum>>            │
│  Commodity           │
│──────────────────────│
│  CLOTH               │
│  FUR                 │
│  GRAIN               │
│  DYE                 │
│  SPICE               │
│  GOLD                │
└──────────────────────┘
         ▲ uses
         │
┌──────────────────────┐        ┌──────────────────────────────┐
│  Card                │        │  Deck                        │
│──────────────────────│        │──────────────────────────────│
│ -commodity: Commodity│        │ -cards: vector<Card>  [STL]  │
│ -value: int          │        │ -rng: mt19937                │
│──────────────────────│        │──────────────────────────────│
│ +getCommodity()      │   ◄────│ +build(numPlayers: int)      │
│ +getValue()          │        │ +shuffle()                   │
│ +toString()          │        │ +draw(n: int): vector<Card>  │
│ +operator<()         │        │ +isEmpty(): bool             │
│ +operator>()         │        │ +remaining(): int            │
│ +operator==()        │        │ +operator+=(Card)            │
│ +operator+(Card)     │        │ +operator<<()                │
│ +operator+(int)      │        └──────────────────────────────┘
│ +operator<<()        │
└──────────────────────┘
         ▲ contains
         │
┌──────────────────────┐
│  Ship                │
│──────────────────────│
│ -cargo: vector<Card> │
│ -capacity: int       │
│──────────────────────│
│ +load(group)         │  ──throws──► ShipFullException
│ +clear()             │
│ +totalValue(): int   │
│ +spaceRemaining()    │
│ +isFull(): bool      │
│ +countCommodity()    │
│ +operator<()         │
│ +operator>()         │
│ +operator==()        │
│ +operator<<()        │
└──────────────────────┘
         ▲ has-a
         │
┌──────────────────────────────────┐      ┌─────────────────────────────┐
│  Player                          │      │  GameBoard                  │
│──────────────────────────────────│      │─────────────────────────────│
│ -name: string                    │      │ MAX_LEVEL: int = 7  [const] │
│ -florins: int                    │      │─────────────────────────────│
│ -ship: Ship                      │      │ +bonusForLevel(lvl): int    │
│ -commodityTrack: map<Commodity,  │      │ +displayMarketTracks()      │
│    int>  [STL map template]      │      └─────────────────────────────┘
│ -eliminated: bool                │
│──────────────────────────────────│
│ +bid(amount)  ──throws──►        │
│     InsufficientFundsException   │
│     InvalidBidException          │
│ +earnFlorins(amount)             │
│ +loadGroup(group)                │
│ +fillShip(cards)                 │
│ +resetForNewDay(capacity)        │
│ +getCommodityCount(c)            │
│ +operator<()                     │
│ +operator>()                     │
│ +operator==()                    │
│ +operator<<()                    │
└──────────────────────────────────┘
         ▲ owns vector of
         │
┌──────────────────────────────────────────────────────────────────────┐
│  MediciGame                                                          │
│──────────────────────────────────────────────────────────────────────│
│ -numPlayers: int                                                     │
│ -currentDay: int                                                     │
│ -players: vector<Player>   [STL vector template]                    │
│ -deck: Deck                                                          │
│ -board: GameBoard                                                    │
│ -shipPayments: vector<vector<int>>  [STL nested template]           │
│ -startPlayerIndex: int                                               │
│──────────────────────────────────────────────────────────────────────│
│ +setup()                                                             │
│ +run()                                                               │
│ -runDay()                                                            │
│ -runAuction(presenterIdx)                                            │
│ -scoreDay()                                                          │
│ -prepareNextDay()                                                    │
│ -endGame()                                                           │
│ -displayAllShips()                                                   │
│ -displayStandings()                                                  │
└──────────────────────────────────────────────────────────────────────┘

┌───────────────────────────────────────────────────────────────┐
│  Exception Hierarchy                                          │
│                                                               │
│  std::exception                                               │
│       └── MediciException                                     │
│               ├── InvalidBidException                         │
│               ├── ShipFullException                           │
│               ├── InsufficientFundsException                  │
│               └── InvalidPlayerCountException                 │
└───────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────┐
│  Template Usage                                             │
│                                                             │
│  STL Templates used:                                        │
│    vector<Card>            – Deck cargo, Ship cargo, groups │
│    vector<Player>          – Player list in MediciGame      │
│    vector<vector<int>>     – Payment table                  │
│    map<Commodity, int>     – Commodity track in Player      │
│                                                             │
│  Custom Template Function:                                  │
│    rankDescending<T>(vector<T>) -> vector<size_t>           │
│      Generic ranking utility used for ship values,         │
│      commodity positions, and final standings.             │
└─────────────────────────────────────────────────────────────┘
```

---

## 2. Behavioral Diagram – Game Flow (Activity Diagram)

```
┌─────────────────────────────────────────────────────────────────────┐
│                   MEDICI – GAME FLOW                                │
└─────────────────────────────────────────────────────────────────────┘

         [START]
            │
            ▼
    ┌───────────────┐
    │  setup()      │  Enter player count (2-6), names, assign starting florins
    │               │  Random start player chosen
    └───────┬───────┘
            │
            ▼
    ╔═══════════════════════════════════════════════╗
    ║           REPEAT 3 TIMES  (Day 1, 2, 3)       ║
    ║                                               ║
    ║   ┌───────────────────────────────────────┐   ║
    ║   │  runDay()                             │   ║
    ║   │  - Reset all ships                    │   ║
    ║   │  - Build & shuffle deck               │   ║
    ║   └────────────┬──────────────────────────┘   ║
    ║                │                              ║
    ║    ╔═══════════╧════════════════════╗         ║
    ║    ║  REPEAT until day ends         ║         ║
    ║    ║                                ║         ║
    ║    ║  ┌──────────────────────────┐  ║         ║
    ║    ║  │  runAuction(presenterIdx)│  ║         ║
    ║    ║  │                          │  ║         ║
    ║    ║  │  Presenter draws 1-3     │  ║         ║
    ║    ║  │  cards from deck         │  ║         ║
    ║    ║  │         │                │  ║         ║
    ║    ║  │  [Each other player]─────┤  ║         ║
    ║    ║  │   bid or pass            │  ║         ║
    ║    ║  │   (clockwise order)      │  ║         ║
    ║    ║  │         │                │  ║         ║
    ║    ║  │  [Presenter bids last]   │  ║         ║
    ║    ║  │         │                │  ║         ║
    ║    ║  │  ┌──────┴────────┐       │  ║         ║
    ║    ║  │  │ Highest bid   │       │  ║         ║
    ║    ║  │  │ wins?         │       │  ║         ║
    ║    ║  │  └──┬────────┬───┘       │  ║         ║
    ║    ║  │   YES        NO          │  ║         ║
    ║    ║  │    │         │           │  ║         ║
    ║    ║  │ winner     discard       │  ║         ║
    ║    ║  │ pays bid   group         │  ║         ║
    ║    ║  │ loads ship               │  ║         ║
    ║    ║  └──────────────────────────┘  ║         ║
    ║    ║                                ║         ║
    ║    ║  DAY ENDS when:                ║         ║
    ║    ║   - Deck is empty, OR          ║         ║
    ║    ║   - All but 1 ship full        ║         ║
    ║    ╚════════════════════════════════╝         ║
    ║                                               ║
    ║   ┌───────────────────────────────────────┐   ║
    ║   │  scoreDay()                           │   ║
    ║   │  1. Rank ships by total value         │   ║
    ║   │     Pay florins (30/20/15/10/5/0)     │   ║
    ║   │     (ties split the pool)             │   ║
    ║   │                                       │   ║
    ║   │  2. Update commodity market tracks    │   ║
    ║   │     1st on track → +10 florins        │   ║
    ║   │     2nd on track → +5 florins         │   ║
    ║   │     (ties split; 2p: 2nd gets 0)      │   ║
    ║   │                                       │   ║
    ║   │  3. Level bonuses on track (5/10/20)  │   ║
    ║   └───────────────────────────────────────┘   ║
    ║                                               ║
    ║   If day < 3: prepareNextDay()                ║
    ║     - Fewest florins = new start player       ║
    ║     - Rebuild & re-shuffle all 36 cards       ║
    ╚═══════════════════════════════════════════════╝
            │
            ▼
    ┌───────────────┐
    │  endGame()    │  Display final standings, announce winner
    └───────┬───────┘
            │
          [END]
```

---

## 3. OOP Concepts Demonstrated

| Concept | Where Used |
|---|---|
| **Classes** | `Card`, `Ship`, `Deck`, `Player`, `GameBoard`, `MediciGame`, exception classes |
| **Encapsulation** | Private data members with public getters; `Ship::load()` validates internally |
| **Operator Overloading** | `Card`: `<`, `>`, `==`, `+`, `<<`; `Ship`: `<`, `>`, `==`, `<<`; `Player`: `<`, `>`, `==`, `<<`; `Deck`: `+=`, `<<` |
| **Templates** | STL `vector<Card>`, `vector<Player>`, `map<Commodity,int>`, `vector<vector<int>>`; custom `rankDescending<T>()` |
| **Exception Handling** | `MediciException` hierarchy; try/catch in `runAuction()` and `main()`; exceptions thrown from `bid()`, `load()` |
| **Inheritance** | All game exceptions inherit from `MediciException` → `std::exception` |

---

## 4. Key Design Decisions

**Card representation:** Each card stores its Commodity enum and integer value. Operator overloading on `+` lets auction code naturally sum a group's value.

**Ship capacity:** Controlled by `MediciGame::shipCapacity()` which returns 7 for 2-player and 5 otherwise, matching the rulebook.

**Tie-breaking in scoring:** Both ship-value ranking and commodity-track ranking use the same `rankDescending<T>` template, then a sliding-window loop groups ties and splits the prize pool (rounded down), matching the rulebook.

**Commodity track persistence:** `Player::commodityTrack` accumulates counts across all three days and is never reset — only the ship is cleared between days.

**Start player rotation:** The player with the fewest florins starts each new day (random tiebreak). The presenter always bids last in their own auction.
