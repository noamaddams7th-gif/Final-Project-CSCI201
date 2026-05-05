/* Program name: medici.cpp
 * Author: Student
 * Date last updated: 5/5/2026
 * Purpose: Console-based implementation of the Medici board game
 *          for 2-6 players, demonstrating OOP concepts including
 *          classes, operator overloading, templates, and exception handling.
 */

#include <iostream>
#include <vector>
#include <map>
#include <string>
#include <algorithm>
#include <random>
#include <stdexcept>
#include <iomanip>
#include <sstream>
#include <limits>

// ─────────────────────────────────────────────
//  Forward declarations
// ─────────────────────────────────────────────
class Card;
class Ship;
class Player;
class Deck;
class GameBoard;
class MediciGame;

// ─────────────────────────────────────────────
//  Custom exception classes
// ─────────────────────────────────────────────

class MediciException : public std::exception {
protected:
    std::string message;
public:
    explicit MediciException(const std::string& msg) : message(msg) {}
    const char* what() const noexcept override { return message.c_str(); }
};

class InvalidBidException : public MediciException {
public:
    explicit InvalidBidException(const std::string& msg)
        : MediciException("Invalid Bid: " + msg) {}
};

class ShipFullException : public MediciException {
public:
    explicit ShipFullException()
        : MediciException("Ship is full – cannot load more cards.") {}
};

class InsufficientFundsException : public MediciException {
public:
    explicit InsufficientFundsException(int bid, int available)
        : MediciException("Cannot bid " + std::to_string(bid) +
                          " – only " + std::to_string(available) + " florins available.") {}
};

class InvalidPlayerCountException : public MediciException {
public:
    explicit InvalidPlayerCountException(int n)
        : MediciException("Invalid player count: " + std::to_string(n) +
                          ". Must be 2-6.") {}
};

// ─────────────────────────────────────────────
//  Commodity enum & helpers
// ─────────────────────────────────────────────

enum class Commodity { CLOTH, FUR, GRAIN, DYE, SPICE, GOLD };

std::string commodityName(Commodity c) {
    switch (c) {
        case Commodity::CLOTH: return "Cloth";
        case Commodity::FUR:   return "Fur";
        case Commodity::GRAIN: return "Grain";
        case Commodity::DYE:   return "Dye";
        case Commodity::SPICE: return "Spice";
        case Commodity::GOLD:  return "Gold";
    }
    return "Unknown";
}

std::string commoditySymbol(Commodity c) {
    switch (c) {
        case Commodity::CLOTH: return "[CLO]";
        case Commodity::FUR:   return "[FUR]";
        case Commodity::GRAIN: return "[GRN]";
        case Commodity::DYE:   return "[DYE]";
        case Commodity::SPICE: return "[SPC]";
        case Commodity::GOLD:  return "[GLD]";
    }
    return "[???]";
}

// ─────────────────────────────────────────────
//  Card class
// ─────────────────────────────────────────────

class Card {
private:
    Commodity commodity;
    int value;

public:
    Card(Commodity c, int v) : commodity(c), value(v) {}

    Commodity getCommodity() const { return commodity; }
    int getValue() const { return value; }

    // Operator overloading: compare cards by value
    bool operator<(const Card& other) const { return value < other.value; }
    bool operator>(const Card& other) const { return value > other.value; }
    bool operator==(const Card& other) const {
        return commodity == other.commodity && value == other.value;
    }

    // Operator overloading: add card values
    int operator+(const Card& other) const { return value + other.value; }
    int operator+(int n) const { return value + n; }

    std::string toString() const {
        std::ostringstream oss;
        oss << commoditySymbol(commodity) << " " << std::setw(2) << value;
        return oss.str();
    }

    // Stream output operator
    friend std::ostream& operator<<(std::ostream& os, const Card& card) {
        os << card.toString();
        return os;
    }
};

// ─────────────────────────────────────────────
//  Deck class  (uses STL vector – template usage)
// ─────────────────────────────────────────────

class Deck {
private:
    std::vector<Card> cards;
    std::mt19937 rng;

public:
    Deck() : rng(std::random_device{}()) {}

    void build(int numPlayers) {
        cards.clear();
        // 7 cards per commodity (values 0,1,2,3,4,5,5) + 1 gold(10)
        std::vector<int> values = {0, 1, 2, 3, 4, 5, 5};
        std::vector<Commodity> commodities = {
            Commodity::CLOTH, Commodity::FUR, Commodity::GRAIN,
            Commodity::DYE, Commodity::SPICE
        };
        for (auto c : commodities)
            for (int v : values)
                cards.emplace_back(c, v);
        cards.emplace_back(Commodity::GOLD, 10);  // total = 36

        shuffle();

        // Remove cards based on player count
        int toRemove;
        switch (numPlayers) {
            case 2: toRemove = 18; break;
            case 3: toRemove = 18; break;
            case 4: toRemove = 12; break;
            case 5: toRemove =  6; break;
            case 6: toRemove =  0; break;
            default: toRemove = 0;
        }
        for (int i = 0; i < toRemove && !cards.empty(); ++i)
            cards.pop_back();
    }

    void shuffle() {
        std::shuffle(cards.begin(), cards.end(), rng);
    }

    // Draw up to n cards from top; returns actual cards drawn
    std::vector<Card> draw(int n) {
        std::vector<Card> drawn;
        for (int i = 0; i < n && !cards.empty(); ++i) {
            drawn.push_back(cards.back());
            cards.pop_back();
        }
        return drawn;
    }

    bool isEmpty() const { return cards.empty(); }
    int remaining() const { return static_cast<int>(cards.size()); }

    // Operator overloading: += adds cards back (for rebuilding)
    Deck& operator+=(const Card& card) {
        cards.push_back(card);
        return *this;
    }

    friend std::ostream& operator<<(std::ostream& os, const Deck& d) {
        os << "Deck [" << d.cards.size() << " cards remaining]";
        return os;
    }
};

// ─────────────────────────────────────────────
//  Ship class
// ─────────────────────────────────────────────

class Ship {
private:
    std::vector<Card> cargo;
    int capacity;

public:
    explicit Ship(int cap) : capacity(cap) {}

    void load(const std::vector<Card>& group) {
        if (static_cast<int>(cargo.size()) + static_cast<int>(group.size()) > capacity)
            throw ShipFullException();
        for (const auto& c : group)
            cargo.push_back(c);
    }

    void clear() { cargo.clear(); }

    int totalValue() const {
        int total = 0;
        for (const auto& c : cargo) total += c.getValue();
        return total;
    }

    int spaceRemaining() const {
        return capacity - static_cast<int>(cargo.size());
    }

    bool isFull() const { return static_cast<int>(cargo.size()) >= capacity; }
    int getCapacity() const { return capacity; }
    int cardCount() const { return static_cast<int>(cargo.size()); }

    // Count cards of a specific commodity
    int countCommodity(Commodity c) const {
        int count = 0;
        for (const auto& card : cargo)
            if (card.getCommodity() == c) ++count;
        return count;
    }

    const std::vector<Card>& getCargo() const { return cargo; }

    // Operator overloading: compare ships by total value
    bool operator<(const Ship& other) const {
        return totalValue() < other.totalValue();
    }
    bool operator>(const Ship& other) const {
        return totalValue() > other.totalValue();
    }
    bool operator==(const Ship& other) const {
        return totalValue() == other.totalValue();
    }

    std::string display() const {
        std::ostringstream oss;
        oss << "| ";
        for (const auto& c : cargo)
            oss << c.toString() << " | ";
        // Empty slots
        for (int i = static_cast<int>(cargo.size()); i < capacity; ++i)
            oss << "  [---]  | ";
        oss << "  Total: " << totalValue();
        return oss.str();
    }

    friend std::ostream& operator<<(std::ostream& os, const Ship& ship) {
        os << ship.display();
        return os;
    }
};

// ─────────────────────────────────────────────
//  Player class
// ─────────────────────────────────────────────

class Player {
private:
    std::string name;
    int florins;
    Ship ship;
    // Cumulative commodity counts across all days (for market track)
    std::map<Commodity, int> commodityTrack;
    bool eliminated;  // eliminated for current day (ship full)

public:
    Player(const std::string& n, int startFlorins, int shipCapacity)
        : name(n), florins(startFlorins), ship(shipCapacity), eliminated(false) {
        for (auto c : {Commodity::CLOTH, Commodity::FUR, Commodity::GRAIN,
                       Commodity::DYE, Commodity::SPICE})
            commodityTrack[c] = 0;
    }

    // Getters
    const std::string& getName() const { return name; }
    int getFlorins() const { return florins; }
    Ship& getShip() { return ship; }
    const Ship& getShip() const { return ship; }
    bool isEliminated() const { return eliminated; }
    int getCommodityCount(Commodity c) const {
        auto it = commodityTrack.find(c);
        return (it != commodityTrack.end()) ? it->second : 0;
    }
    const std::map<Commodity, int>& getCommodityTrack() const { return commodityTrack; }

    // Bid: deduct florins, throw if insufficient
    void bid(int amount) {
        if (amount > florins)
            throw InsufficientFundsException(amount, florins);
        if (amount < 1)
            throw InvalidBidException("Bid must be at least 1 florin.");
        florins -= amount;
    }

    void earnFlorins(int amount) { florins += amount; }

    void loadGroup(const std::vector<Card>& group) {
        ship.load(group);  // may throw ShipFullException
        if (ship.isFull()) eliminated = true;
        // Update commodity track
        for (const auto& card : group)
            if (card.getCommodity() != Commodity::GOLD)
                commodityTrack[card.getCommodity()] += 1;
    }

    // Receive free cards to fill ship at end of day
    void fillShip(std::vector<Card> freeCards) {
        for (const auto& card : freeCards) {
            if (ship.isFull()) break;
            std::vector<Card> single = {card};
            try {
                ship.load(single);
                if (card.getCommodity() != Commodity::GOLD)
                    commodityTrack[card.getCommodity()] += 1;
            } catch (...) { break; }
        }
        eliminated = true;
    }

    void resetForNewDay(int newCapacity) {
        ship.clear();
        ship = Ship(newCapacity);
        eliminated = false;
    }

    void setEliminated(bool e) { eliminated = e; }

    // Operator overloading: compare players by florins
    bool operator<(const Player& other) const { return florins < other.florins; }
    bool operator>(const Player& other) const { return florins > other.florins; }
    bool operator==(const Player& other) const { return florins == other.florins; }

    friend std::ostream& operator<<(std::ostream& os, const Player& p) {
        os << std::left << std::setw(12) << p.name
           << " | Florins: " << std::setw(4) << p.florins
           << " | Ship: " << p.ship;
        return os;
    }
};

// ─────────────────────────────────────────────
//  GameBoard: tracks commodity market positions
// ─────────────────────────────────────────────

class GameBoard {
public:
    // Max level per commodity track is 7 (indices 0..7, gold frame = 0)
    enum { MAX_LEVEL = 7 };

    // Bonus florins for top 3 levels (levels 5,6,7 from bottom → indices 5,6,7)
    // The board shows bonuses 5, 10, 20 at the top three levels
    static int bonusForLevel(int level) {
        if (level == MAX_LEVEL)     return 20;
        if (level == MAX_LEVEL - 1) return 10;
        if (level == MAX_LEVEL - 2) return  5;
        return 0;
    }

    void displayMarketTracks(const std::vector<Player>& players,
                              const std::vector<Commodity>& commodities) const {
        std::cout << "\n=== COMMODITY MARKET TRACKS ===\n";
        std::vector<Commodity> trackCommodities = {
            Commodity::CLOTH, Commodity::FUR, Commodity::GRAIN,
            Commodity::DYE, Commodity::SPICE
        };
        for (auto c : trackCommodities) {
            std::cout << std::left << std::setw(6) << commodityName(c) << ": ";
            for (const auto& p : players) {
                // Track position = cumulative count of this commodity, capped at MAX_LEVEL
                int pos = std::min(p.getCommodityCount(c), (int)MAX_LEVEL);
                std::cout << p.getName() << "=" << pos << "  ";
            }
            std::cout << "\n";
        }
        std::cout << "\n";
    }
};

// ─────────────────────────────────────────────
//  Template utility: generic leaderboard
// ─────────────────────────────────────────────

template <typename T>
std::vector<size_t> rankDescending(const std::vector<T>& values) {
    std::vector<size_t> indices(values.size());
    for (size_t i = 0; i < indices.size(); ++i) indices[i] = i;
    std::sort(indices.begin(), indices.end(),
              [&](size_t a, size_t b) { return values[a] > values[b]; });
    return indices;
}

// ─────────────────────────────────────────────
//  Input helpers
// ─────────────────────────────────────────────

int getInt(const std::string& prompt, int min, int max) {
    int val;
    while (true) {
        std::cout << prompt;
        if (std::cin >> val && val >= min && val <= max) {
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            return val;
        }
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        std::cout << "  Please enter a number between " << min << " and " << max << ".\n";
    }
}

std::string getString(const std::string& prompt) {
    std::string s;
    std::cout << prompt;
    std::getline(std::cin, s);
    if (s.empty()) s = "Player";
    return s;
}

void pressEnter(const std::string& msg = "Press ENTER to continue...") {
    std::cout << msg;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

void printLine(char ch = '-', int len = 60) {
    std::cout << std::string(len, ch) << "\n";
}

void printHeader(const std::string& title) {
    printLine('=');
    std::cout << "  " << title << "\n";
    printLine('=');
}

// ─────────────────────────────────────────────
//  MediciGame – main game controller
// ─────────────────────────────────────────────

class MediciGame {
private:
    int numPlayers;
    int currentDay;
    std::vector<Player> players;
    Deck deck;
    GameBoard board;

    // Payment tables indexed by [numPlayers-2][place(0-based)]
    // place 0 = highest value ship
    const std::vector<std::vector<int>> shipPayments = {
        {20,  0},           // 2 players
        {30, 15,  0},       // 3 players
        {30, 20, 10,  0},   // 4 players
        {30, 20, 10,  5, 0},// 5 players
        {30, 20, 15, 10, 5, 0} // 6 players
    };

    int startPlayerIndex;

    // Ship capacity: 5 normally, 7 for 2-player
    int shipCapacity() const { return (numPlayers == 2) ? 7 : 5; }

    // Starting florins
    int startingFlorins() const { return (numPlayers <= 4) ? 40 : 30; }

public:
    MediciGame() : numPlayers(0), currentDay(1), startPlayerIndex(0) {}

    // ── Setup ──────────────────────────────────────────────────────────
    void setup() {
        printHeader("MEDICI  –  The Merchant Game");
        std::cout << "  Designed by Reiner Knizia\n\n";

        numPlayers = getInt("Enter number of players (2-6): ", 2, 6);
        if (numPlayers < 2 || numPlayers > 6)
            throw InvalidPlayerCountException(numPlayers);

        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

        players.clear();
        int sf = startingFlorins();
        int sc = shipCapacity();
        for (int i = 0; i < numPlayers; ++i) {
            std::string name = getString("Enter name for Player " +
                                         std::to_string(i + 1) + ": ");
            players.emplace_back(name, sf, sc);
        }

        // Random starting player
        std::mt19937 rng(std::random_device{}());
        startPlayerIndex = static_cast<int>(rng() % numPlayers);

        std::cout << "\nStarting player: " << players[startPlayerIndex].getName() << "\n";
        std::cout << "Each player starts with " << sf << " florins.\n";
        pressEnter();
    }

    // ── Main game loop ─────────────────────────────────────────────────
    void run() {
        for (currentDay = 1; currentDay <= 3; ++currentDay) {
            runDay();
            scoreDay();
            if (currentDay < 3) prepareNextDay();
        }
        endGame();
    }

    // ── One full day ───────────────────────────────────────────────────
    void runDay() {
        printHeader("DAY " + std::to_string(currentDay) + " of 3");

        // Reset ships for this day
        int sc = shipCapacity();
        for (auto& p : players)
            p.resetForNewDay(sc);

        // Build & shuffle deck
        deck.build(numPlayers);
        std::cout << deck << "\n\n";
        pressEnter();

        int currentPlayerIdx = startPlayerIndex;

        while (true) {
            // Check if day ends: deck empty, or all but one ship full
            int activeCount = 0;
            int lastActiveIdx = -1;
            for (int i = 0; i < numPlayers; ++i) {
                if (!players[i].isEliminated()) {
                    ++activeCount;
                    lastActiveIdx = i;
                }
            }

            if (deck.isEmpty()) {
                std::cout << "\nDeck is empty – day ends!\n";
                break;
            }

            // All ships full / eliminated
            if (activeCount == 0) break;

            // If only one active player remains, fill their ship from deck
            if (activeCount == 1) {
                std::cout << "\nOnly " << players[lastActiveIdx].getName()
                          << " remains. Filling ship from deck...\n";
                int need = players[lastActiveIdx].getShip().spaceRemaining();
                auto freeCards = deck.draw(need);
                players[lastActiveIdx].fillShip(freeCards);
                std::cout << players[lastActiveIdx].getName()
                          << "'s ship: " << players[lastActiveIdx].getShip() << "\n";
                break;
            }

            // Skip eliminated players for their turn
            // (they still bid when others present groups)
            while (players[currentPlayerIdx].isEliminated()) {
                currentPlayerIdx = (currentPlayerIdx + 1) % numPlayers;
            }

            runAuction(currentPlayerIdx);

            // Advance to next non-eliminated player
            currentPlayerIdx = (currentPlayerIdx + 1) % numPlayers;
        }
    }

    // ── One auction turn ───────────────────────────────────────────────
    void runAuction(int presenterIdx) {
        Player& presenter = players[presenterIdx];
        printLine('-');
        std::cout << presenter.getName() << "'s turn to select a group.\n";
        displayAllShips();
        std::cout << "Cards remaining in deck: " << deck.remaining() << "\n\n";

        // Presenter draws 1-3 cards
        std::vector<Card> group;
        int maxDraw = std::min(3, (int)deck.remaining());
        for (int drawn = 1; drawn <= maxDraw; ++drawn) {
            auto newCard = deck.draw(1);
            if (newCard.empty()) break;
            group.push_back(newCard[0]);

            std::cout << "  Card " << drawn << ": " << group.back() << "\n";

            // Must stop if group now exceeds what any player can buy
            // Check if at least one player (not just presenter) can receive this many cards
            bool anyCanTake = false;
            for (int i = 0; i < numPlayers; ++i) {
                if (players[i].getShip().spaceRemaining() >= static_cast<int>(group.size()))
                    anyCanTake = true;
            }
            if (!anyCanTake) {
                std::cout << "  No player can take a group of " << group.size()
                          << " – stopping draw.\n";
                break;
            }

            if (drawn == maxDraw) break;  // drew max

            // Ask presenter if they want to draw another
            if (drawn < 3 && deck.remaining() > 0) {
                std::cout << "  Current group value: ";
                int total = 0;
                for (const auto& c : group) total += c.getValue();
                std::cout << total << "\n";
                int choice = getInt("  Draw another card? (1=Yes, 0=No): ", 0, 1);
                if (choice == 0) break;
            }
        }

        if (group.empty()) {
            std::cout << "No cards available.\n";
            return;
        }

        int groupValue = 0;
        for (const auto& c : group) groupValue += c.getValue();
        std::cout << "\n  Group for auction (" << group.size() << " card"
                  << (group.size()>1?"s":"") << ", total value " << groupValue << "):\n  ";
        for (const auto& c : group) std::cout << c << "  ";
        std::cout << "\n\n";

        // ── Auction round ──────────────────────────────────────────────
        // Bidding starts with player to LEFT of presenter (clockwise)
        int highestBid = 0;
        int winnerIdx  = -1;

        // Order: start left of presenter, presenter bids last
        std::vector<int> bidOrder;
        for (int i = 1; i <= numPlayers; ++i)
            bidOrder.push_back((presenterIdx + i) % numPlayers);
        // presenter is last
        bidOrder.push_back(presenterIdx);  // actually already included via i=numPlayers
        // Fix: rebuild so presenter is truly last
        bidOrder.clear();
        for (int i = 1; i < numPlayers; ++i)
            bidOrder.push_back((presenterIdx + i) % numPlayers);
        bidOrder.push_back(presenterIdx);

        std::cout << "  --- BIDDING ---\n";
        for (int idx : bidOrder) {
            Player& p = players[idx];
            int spaceLeft = p.getShip().spaceRemaining();
            int groupSize = static_cast<int>(group.size());

            // Player cannot bid if they can't hold the whole group
            if (spaceLeft < groupSize) {
                std::cout << "  " << std::left << std::setw(12) << p.getName()
                          << " must PASS (not enough space).\n";
                continue;
            }

            // Player cannot bid more than they have, or go below 0
            int maxBid = p.getFlorins();
            if (maxBid < 1) {
                std::cout << "  " << std::left << std::setw(12) << p.getName()
                          << " must PASS (no florins).\n";
                continue;
            }

            std::cout << "  " << p.getName()
                      << " (florins: " << p.getFlorins()
                      << ", space: " << spaceLeft << ")\n";
            std::cout << "  Current highest bid: " << highestBid << "\n";

            int minBid = highestBid + 1;
            if (minBid > maxBid) {
                std::cout << "  " << p.getName() << " must PASS (cannot outbid).\n";
                continue;
            }

            int choice = getInt("  Bid (enter 0 to pass, " +
                                std::to_string(minBid) + "-" +
                                std::to_string(maxBid) + " to bid): ",
                                0, maxBid);

            if (choice == 0) {
                std::cout << "  " << p.getName() << " passes.\n";
            } else if (choice < minBid) {
                std::cout << "  Bid too low – " << p.getName() << " passes.\n";
            } else {
                highestBid = choice;
                winnerIdx  = idx;
                std::cout << "  " << p.getName() << " bids " << choice << " florins.\n";
            }
        }

        // ── Resolve auction ────────────────────────────────────────────
        if (winnerIdx == -1 || highestBid == 0) {
            std::cout << "\n  No bids – group discarded.\n";
        } else {
            Player& winner = players[winnerIdx];
            try {
                winner.bid(highestBid);
                winner.loadGroup(group);
                std::cout << "\n  " << winner.getName() << " wins the auction for "
                          << highestBid << " florins!\n";
                std::cout << "  Ship: " << winner.getShip() << "\n";
            } catch (const InsufficientFundsException& e) {
                std::cout << "  ERROR: " << e.what() << " Auction cancelled.\n";
            } catch (const ShipFullException& e) {
                std::cout << "  ERROR: " << e.what() << " Auction cancelled.\n";
            } catch (const MediciException& e) {
                std::cout << "  ERROR: " << e.what() << "\n";
            }
        }
        pressEnter();
    }

    // ── Score the day ──────────────────────────────────────────────────
    void scoreDay() {
        printHeader("SCORING – DAY " + std::to_string(currentDay));

        // 1. Ship value ranking
        std::cout << "--- Ship Values ---\n";
        std::vector<int> shipValues(numPlayers);
        for (int i = 0; i < numPlayers; ++i)
            shipValues[i] = players[i].getShip().totalValue();

        auto shipRanking = rankDescending(shipValues);

        // Display
        for (size_t r = 0; r < shipRanking.size(); ++r) {
            int i = static_cast<int>(shipRanking[r]);
            std::cout << "  " << (r+1) << ". " << std::left << std::setw(12)
                      << players[i].getName()
                      << " – Ship value: " << shipValues[i] << "\n";
        }
        std::cout << "\n";

        // Pay for ship rankings, handling ties
        const auto& payTable = shipPayments[numPlayers - 2];
        // Group players by value for tie-splitting
        // Walk through ranked list, batch ties
        size_t i = 0;
        while (i < shipRanking.size()) {
            int val = shipValues[shipRanking[i]];
            size_t j = i;
            while (j < shipRanking.size() && shipValues[shipRanking[j]] == val) ++j;
            // Players i..j-1 are tied
            int totalPay = 0;
            for (size_t k = i; k < j; ++k)
                if (k < payTable.size()) totalPay += payTable[k];
            int perPlayer = (j > i) ? totalPay / static_cast<int>(j - i) : 0;
            for (size_t k = i; k < j; ++k) {
                players[shipRanking[k]].earnFlorins(perPlayer);
                std::cout << "  " << players[shipRanking[k]].getName()
                          << " earns " << perPlayer << " florins (ship ranking).\n";
            }
            i = j;
        }

        // 2. Commodity market tracks
        std::cout << "\n--- Commodity Market Bonuses ---\n";
        std::vector<Commodity> trackCommodities = {
            Commodity::CLOTH, Commodity::FUR, Commodity::GRAIN,
            Commodity::DYE, Commodity::SPICE
        };

        for (auto c : trackCommodities) {
            std::cout << "  " << commodityName(c) << ":\n";

            // Collect positions (cumulative counts, capped)
            std::vector<int> positions(numPlayers);
            for (int pi = 0; pi < numPlayers; ++pi)
                positions[pi] = std::min((int)players[pi].getCommodityCount(c),
                                         (int)GameBoard::MAX_LEVEL);

            auto ranking = rankDescending(positions);

            // 1st: 10 florins, 2nd: 5 florins (0 in 2p)
            // Handle ties
            size_t ri = 0;
            while (ri < ranking.size()) {
                int pos = positions[ranking[ri]];
                size_t rj = ri;
                while (rj < ranking.size() && positions[ranking[rj]] == pos) ++rj;

                int prize = 0;
                if (ri == 0) {
                    // 1st place prize pool
                    int pool = 10;
                    // If tie spans 1st and 2nd, add both
                    if (rj > 1 && numPlayers > 2) pool += 5;
                    else if (rj > 1) pool = 10; // 2p: 2nd gets 0
                    prize = pool / static_cast<int>(rj - ri);
                } else if (ri == 1 && numPlayers > 2) {
                    prize = 5 / static_cast<int>(rj - ri);
                }

                for (size_t k = ri; k < rj; ++k) {
                    players[ranking[k]].earnFlorins(prize);
                    if (prize > 0)
                        std::cout << "    " << players[ranking[k]].getName()
                                  << " (pos " << pos << ") earns " << prize << " florins.\n";
                }
                ri = rj;
            }

            // Level bonus for top 3 levels
            for (int pi = 0; pi < numPlayers; ++pi) {
                int bonus = GameBoard::bonusForLevel(positions[pi]);
                if (bonus > 0) {
                    players[pi].earnFlorins(bonus);
                    std::cout << "    " << players[pi].getName()
                              << " bonus +" << bonus << " florins (level "
                              << positions[pi] << ").\n";
                }
            }
        }

        // Show updated totals
        std::cout << "\n--- Florin Totals After Day " << currentDay << " ---\n";
        displayStandings();
    }

    // ── Prepare next day ───────────────────────────────────────────────
    void prepareNextDay() {
        printHeader("PREPARING DAY " + std::to_string(currentDay + 1));

        // Player with fewest florins starts
        int minFlorins = players[0].getFlorins();
        int minIdx = 0;
        for (int i = 1; i < numPlayers; ++i) {
            if (players[i].getFlorins() < minFlorins) {
                minFlorins = players[i].getFlorins();
                minIdx = i;
            }
        }
        startPlayerIndex = minIdx;
        std::cout << players[startPlayerIndex].getName()
                  << " has fewest florins (" << minFlorins
                  << ") and starts Day " << (currentDay + 1) << ".\n";
        pressEnter();
    }

    // ── End game ───────────────────────────────────────────────────────
    void endGame() {
        printHeader("GAME OVER – FINAL RESULTS");
        std::cout << "After three days, the richest merchant wins!\n\n";
        displayStandings();

        // Find winner(s)
        int maxFlorins = 0;
        for (const auto& p : players)
            maxFlorins = std::max(maxFlorins, p.getFlorins());

        std::cout << "\n*** WINNER";
        std::vector<std::string> winners;
        for (const auto& p : players)
            if (p.getFlorins() == maxFlorins) winners.push_back(p.getName());
        if (winners.size() > 1) std::cout << "S";
        std::cout << ": ";
        for (size_t i = 0; i < winners.size(); ++i) {
            if (i > 0) std::cout << " & ";
            std::cout << winners[i];
        }
        std::cout << " with " << maxFlorins << " florins! ***\n";
        printLine('=');
    }

    // ── Display helpers ────────────────────────────────────────────────
    void displayAllShips() const {
        std::cout << "\n  --- Ships ---\n";
        for (const auto& p : players) {
            std::cout << "  " << std::left << std::setw(12) << p.getName()
                      << " [" << p.getFlorins() << " fl] "
                      << (p.isEliminated() ? "[FULL] " : "       ")
                      << p.getShip() << "\n";
        }
        std::cout << "\n";
    }

    void displayStandings() const {
        std::vector<int> fl(numPlayers);
        for (int i = 0; i < numPlayers; ++i) fl[i] = players[i].getFlorins();
        auto ranked = rankDescending(fl);
        for (size_t r = 0; r < ranked.size(); ++r) {
            int i = static_cast<int>(ranked[r]);
            std::cout << "  " << (r+1) << ". " << std::left << std::setw(14)
                      << players[i].getName() << " – " << fl[i] << " florins\n";
        }
        std::cout << "\n";
        board.displayMarketTracks(players,
            {Commodity::CLOTH, Commodity::FUR, Commodity::GRAIN,
             Commodity::DYE, Commodity::SPICE});
        pressEnter();
    }
};

// ─────────────────────────────────────────────
//  main
// ─────────────────────────────────────────────

int main() {
    try {
        MediciGame game;
        game.setup();
        game.run();
    } catch (const InvalidPlayerCountException& e) {
        std::cerr << "Setup error: " << e.what() << "\n";
        return 1;
    } catch (const MediciException& e) {
        std::cerr << "Game error: " << e.what() << "\n";
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "Unexpected error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}